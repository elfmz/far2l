#include "CharClasses.cpp"
#include "stdio.h"
#include <locale.h>

#include "utf8proc.h"

// uses https://github.com/JuliaStrings/utf8proc
// gcc -c utf8proc.c -o utf8proc.o && ar rcs libutf8proc.a utf8proc.o && gcc ./CharClasses_test2.cpp -o CharClasses_test2 -I./ -L./ -lutf8proc

int main() {

	// Simple test showing differences between char width detected by wcwidth() and by CharClasses.cpp functions

	// wcwidth() works good only with en locale in my Linux Mint 23.1
    char *prev_locale = setlocale(LC_CTYPE, NULL);
    setlocale(LC_CTYPE, "en_US.UTF-8");

	int ct = 0, last = 0x10ffff;
	for (int i=0; i<=last; i++) {

	    //int code;
	    //utf8proc_ssize_t length = utf8proc_iterate((uint8_t *)i, -1, &code);
    
	    int width = utf8proc_charwidth(i);

		utf8proc_category_t category = utf8proc_category(i);

		// UTF8PROC_CATEGORY_CN Other, not assigned
		// https://juliastrings.github.io/utf8proc/doc/utf8proc_8h.html
		bool valid = (category != UTF8PROC_CATEGORY_CN);
		
		if (IsCharFullWidth(i) && (width != 2) && valid) {
			printf("i=%i IsCharFullWidth(i)=true width(i)=%i\n", i, width);
		}
		if (IsCharPrefix(i) && (width != 0) && valid) {
			printf("i=%i IsCharPrefix(i)=true width(i)=%i\n", i, width);
		}
		if (IsCharSuffix(i) && (width != 0) && valid) {
			printf("i=%i IsCharSuffix(i)=true width(i)=%i\n", i, width);
		}
		if ((width != 1) && !IsCharXxxfix(i) && !IsCharFullWidth(i) && valid) {
			printf("i=%i IsCharFullWidth(i)=false IsCharXxxfix(i)=false width(i)=%i\n", i, width);
		}
	}

    setlocale(LC_CTYPE, prev_locale);
}
