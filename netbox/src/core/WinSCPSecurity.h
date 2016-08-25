#pragma once

#include <Classes.hpp>

#define PWALG_SIMPLE 1
#define PWALG_SIMPLE_MAGIC 0xA3
#define PWALG_SIMPLE_STRING ((RawByteString)"0123456789ABCDEF")
#define PWALG_SIMPLE_MAXLEN 50
#define PWALG_SIMPLE_FLAG 0xFF

RawByteString EncryptPassword(const UnicodeString & APassword, const UnicodeString & AKey, Integer AAlgorithm = PWALG_SIMPLE);
UnicodeString DecryptPassword(const RawByteString & APassword, const UnicodeString & AKey, Integer AAlgorithm = PWALG_SIMPLE);
RawByteString SetExternalEncryptedPassword(const RawByteString & APassword);
bool GetExternalEncryptedPassword(const RawByteString & AEncrypted, RawByteString & APassword);

bool WindowsValidateCertificate(const uint8_t * Certificate, size_t Len);

