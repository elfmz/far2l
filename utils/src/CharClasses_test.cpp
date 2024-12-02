#include "CharClasses.cpp"
#include "stdio.h"
#include <locale.h>

int main() {

	// Simple test showing differences between char width detected by wcwidth() and by CharClasses.cpp functions

	// wcwidth() works good only with en locale in my Linux Mint 23.1
    char *prev_locale = setlocale(LC_CTYPE, NULL);
    setlocale(LC_CTYPE, "en_US.UTF-8");

	int ct = 0, last = 0x10ffff;
	for (int i=0; i<=last; i++) {
		CharClasses cc(i);
		if (cc.FullWidth() && (wcwidth(i) != 2) && (wcwidth(i) != -1)) {
			printf("i=%i IsCharFullWidth(i)=true wcwidth(i)=%i\n", i, wcwidth(i));
		}
		if (cc.Prefix() && (wcwidth(i) != 0) && (wcwidth(i) != -1)) {
			printf("i=%i IsCharPrefix(i)=true wcwidth(i)=%i\n", i, wcwidth(i));
		}
		if (cc.Suffix() && (wcwidth(i) != 0) && (wcwidth(i) != -1)) {
			printf("i=%i IsCharSuffix(i)=true wcwidth(i)=%i\n", i, wcwidth(i));
		}
		if ((wcwidth(i) != 1) && !cc.Xxxfix() && !cc.FullWidth() && (wcwidth(i) != -1)) {
			printf("i=%i IsCharFullWidth(i)=false IsCharXxxfix(i)=false wcwidth(i)=%i\n", i, wcwidth(i));
		}
	}

    setlocale(LC_CTYPE, prev_locale);
}
