#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#ifndef __linux__
#include <WinCrypt.h>
#endif
#include "WinSCPSecurity.h"

#define PWALG_SIMPLE_INTERNAL 0x00
#define PWALG_SIMPLE_EXTERNAL 0x01

static int random(int range)
{
  return static_cast<int>(ToDouble(rand()) / (ToDouble(RAND_MAX) / range));
}

RawByteString SimpleEncryptChar(uint8_t Ch)
{
  Ch = (uint8_t)((~Ch) ^ PWALG_SIMPLE_MAGIC);
  return
    PWALG_SIMPLE_STRING.SubString(((Ch & 0xF0) >> 4) + 1, 1) +
    PWALG_SIMPLE_STRING.SubString(((Ch & 0x0F) >> 0) + 1, 1);
}

uint8_t SimpleDecryptNextChar(RawByteString & Str)
{
  if (Str.Length() > 0)
  {
    uint8_t Result = (uint8_t)
      ~((((PWALG_SIMPLE_STRING.Pos(Str.c_str()[0])-1) << 4) +
         ((PWALG_SIMPLE_STRING.Pos(Str.c_str()[1])-1) << 0)) ^ PWALG_SIMPLE_MAGIC);
    Str.Delete(1, 2);
    return Result;
  }
  else
  {
    return 0x00;
  }
}

RawByteString EncryptPassword(const UnicodeString & APassword, const UnicodeString & AKey, Integer /*Algorithm*/)
{
  UTF8String Password = UTF8String(APassword);
  UTF8String Key = UTF8String(AKey);

  RawByteString Result("");
  intptr_t Shift, Index;

  if (!::RandSeed)
  {
    ::Randomize();
    ::RandSeed = 1;
  }
  Password = Key + Password;
  Shift = (Password.Length() < PWALG_SIMPLE_MAXLEN) ?
    static_cast<uint8_t>(random(PWALG_SIMPLE_MAXLEN - static_cast<int>(Password.Length()))) : 0;
  Result += SimpleEncryptChar(static_cast<uint8_t>(PWALG_SIMPLE_FLAG)); // Flag
  Result += SimpleEncryptChar(static_cast<uint8_t>(PWALG_SIMPLE_INTERNAL)); // Dummy
  Result += SimpleEncryptChar(static_cast<uint8_t>(Password.Length()));
  Result += SimpleEncryptChar(static_cast<uint8_t>(Shift));
  for (Index = 0; Index < Shift; ++Index)
    Result += SimpleEncryptChar(static_cast<uint8_t>(random(256)));
  for (Index = 0; Index < Password.Length(); ++Index)
    Result += SimpleEncryptChar(static_cast<uint8_t>(Password.c_str()[Index]));
  while (Result.Length() < PWALG_SIMPLE_MAXLEN * 2)
    Result += SimpleEncryptChar(static_cast<uint8_t>(random(256)));
  return Result;
}

UnicodeString DecryptPassword(const RawByteString & APassword, const UnicodeString & AKey, Integer /*Algorithm*/)
{
  RawByteString Password = APassword;
  UTF8String Key = UTF8String(AKey);
  UTF8String Result("");
  Integer Index;
  uint8_t Length, Flag;

  Flag = SimpleDecryptNextChar(Password);
  if (Flag == PWALG_SIMPLE_FLAG)
  {
    /* Dummy = */ SimpleDecryptNextChar(Password);
    Length = SimpleDecryptNextChar(Password);
  }
  else
    Length = Flag;
  Password.Delete(1, (static_cast<Integer>(SimpleDecryptNextChar(Password)) * 2));
  for (Index = 0; Index < Length; ++Index)
    Result += static_cast<char>(SimpleDecryptNextChar(Password));
  if (Flag == PWALG_SIMPLE_FLAG)
  {
    if (Result.SubString(1, Key.Length()) != Key)
      Result = "";
    else
      Result.Delete(1, Key.Length());
  }
  return UnicodeString(Result);
}

RawByteString SetExternalEncryptedPassword(const RawByteString & APassword)
{
  RawByteString Result;
  Result += SimpleEncryptChar(static_cast<uint8_t>(PWALG_SIMPLE_FLAG));
  Result += SimpleEncryptChar(static_cast<uint8_t>(PWALG_SIMPLE_EXTERNAL));
  Result += UTF8String(BytesToHex(reinterpret_cast<const uint8_t *>(APassword.c_str()), APassword.Length()));
  return Result;
}

bool GetExternalEncryptedPassword(const RawByteString & AEncrypted, RawByteString & APassword)
{
  RawByteString Encrypted = AEncrypted;
  bool Result =
    (SimpleDecryptNextChar(Encrypted) == PWALG_SIMPLE_FLAG) &&
    (SimpleDecryptNextChar(Encrypted) == PWALG_SIMPLE_EXTERNAL);
  if (Result)
  {
    APassword = HexToBytes(UTF8ToString(Encrypted));
  }
  return Result;
}

#ifndef __linux__
bool WindowsValidateCertificate(const uint8_t * Certificate, size_t Len)
{
  bool Result = false;

  // Parse the certificate into a context.
  const CERT_CONTEXT * CertContext =
    CertCreateCertificateContext(
      X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, Certificate, static_cast<DWORD>(Len));

  if (CertContext != nullptr)
  {
    CERT_CHAIN_PARA ChainPara;
    // Retrieve the certificate chain of the certificate
    // (a certificate without a valid root does not have a chain).
    ClearStruct(ChainPara);
    ChainPara.cbSize = sizeof(ChainPara);

    CERT_CHAIN_ENGINE_CONFIG ChainConfig;

    ClearStruct(ChainConfig);
    ChainConfig.cbSize = sizeof(CERT_CHAIN_ENGINE_CONFIG);
    ChainConfig.hRestrictedRoot = nullptr;
    ChainConfig.hRestrictedTrust = nullptr;
    ChainConfig.hRestrictedOther = nullptr;
    ChainConfig.cAdditionalStore = 0;
    ChainConfig.rghAdditionalStore = nullptr;
    ChainConfig.dwFlags = CERT_CHAIN_CACHE_END_CERT;
    ChainConfig.dwUrlRetrievalTimeout = 0;
    ChainConfig.MaximumCachedCertificates = 0;
    ChainConfig.CycleDetectionModulus = 0;

    HCERTCHAINENGINE ChainEngine;
    if (CertCreateCertificateChainEngine(&ChainConfig, &ChainEngine))
    {
      const CERT_CHAIN_CONTEXT * ChainContext = nullptr;
      if (CertGetCertificateChain(ChainEngine, CertContext, nullptr, nullptr, &ChainPara,
            CERT_CHAIN_CACHE_END_CERT |
            CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT,
            nullptr, &ChainContext))
      {
        CERT_CHAIN_POLICY_PARA PolicyPara;

        PolicyPara.cbSize = sizeof(PolicyPara);
        PolicyPara.dwFlags = 0;
        PolicyPara.pvExtraPolicyPara = nullptr;

        CERT_CHAIN_POLICY_STATUS PolicyStatus;
        PolicyStatus.cbSize = sizeof(PolicyStatus);

        if (CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL,
              ChainContext, &PolicyPara, &PolicyStatus))
        {
          // Windows thinks the certificate is valid.
          Result = (PolicyStatus.dwError == S_OK);
        }

        CertFreeCertificateChain(ChainContext);
      }
      CertFreeCertificateChainEngine(ChainEngine);
    }
    CertFreeCertificateContext(CertContext);
  }
  return Result;
}
#endif
