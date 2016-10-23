m4_include(`farversion.m4')m4_dnl
const DWORD FAR_VERSION=m4_format(`0x%04X%04X;',MAJOR,MINOR)
const char *FAR_BUILD="BUILD";
