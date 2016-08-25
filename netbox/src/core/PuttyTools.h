#pragma once

enum TKeyType
{
  ktUnopenable,
  ktUnknown,
  ktSSH1,
  ktSSH2,
  ktOpenSSHAuto,
  ktOpenSSHPEM,
  ktOpenSSHNew,
  ktSSHCom,
  ktSSH1Public,
  ktSSH2PublicRFC4716,
  ktSSH2PublicOpenSSH,
};

TKeyType GetKeyType(const UnicodeString & AFileName);
UnicodeString GetKeyTypeName(TKeyType KeyType);
bool IsKeyEncrypted(TKeyType KeyType, const UnicodeString & FileName, UnicodeString & Comment);
struct TPrivateKey;
TPrivateKey * LoadKey(TKeyType KeyType, const UnicodeString & FileName, const UnicodeString & Passphrase);
void ChangeKeyComment(TPrivateKey * PrivateKey, const UnicodeString & Comment);
void SaveKey(TKeyType KeyType, const UnicodeString & FileName,
  const UnicodeString & Passphrase, TPrivateKey * PrivateKey);
void FreeKey(TPrivateKey * PrivateKey);

int64_t ParseSize(const UnicodeString & SizeStr);

bool HasGSSAPI(const UnicodeString & CustomPath);

void AES256EncodeWithMAC(char * Data, size_t Len, const char * Password,
  size_t PasswordLen, const char * Salt);

UnicodeString NormalizeFingerprint(const UnicodeString & AFingerprint);
UnicodeString GetKeyTypeFromFingerprint(const UnicodeString & AFingerprint);

UnicodeString GetPuTTYVersion();

UnicodeString Sha256(const char * Data, size_t Size);
