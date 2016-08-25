#pragma once

#include <Classes.hpp>

void CryptographyInitialize();
void CryptographyFinalize();
RawByteString ScramblePassword(const UnicodeString & Password);
bool UnscramblePassword(const RawByteString & Scrambled, UnicodeString & Password);
void AES256EncyptWithMAC(const RawByteString & Input, const UnicodeString & Password,
  RawByteString & Output);
bool AES256DecryptWithMAC(const RawByteString & Input, const UnicodeString & Password,
  RawByteString & Output);
void AES256CreateVerifier(const UnicodeString & Input, RawByteString & Verifier);
bool AES256Verify(const UnicodeString & Input, const RawByteString & Verifier);
int IsValidPassword(const UnicodeString & Password);
int PasswordMaxLength();

