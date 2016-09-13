m4_include(`vbuild.m4')m4_dnl
m4_include(`tools.m4')m4_dnl
m4_define(BUILDTYPE,`')m4_dnl
m4_define(MAJOR,2)m4_dnl
m4_define(MINOR,0)m4_dnl
m4_define(DATE,m4_esyscmd(CMDAWK -f CMDDATE))m4_dnl
m4_define(BLD_YEAR,m4_substr(DATE,6,4))m4_dnl
m4_define(BLD_MONTH,m4_substr(DATE,3,2))m4_dnl
m4_define(BLD_DAY,m4_substr(DATE,0,2))m4_dnl
m4_define(COPYRIGHTYEARS,m4_ifelse(`2000',BLD_YEAR,`2000',`2000-'BLD_YEAR))m4_dnl
m4_define(MAKEFULLVERSION,`m4_ifelse(
`',$1,`MAJOR.MINOR (build BUILD) $2',
`RC',$1,`MAJOR.MINOR RC (build BUILD) $2',
`alpha',$1,`MAJOR.MINOR alpha (build BUILD) $2',
`beta',$1,`MAJOR.MINOR beta (build BUILD) $2',
`MAJOR.MINOR alpha ($1 based on build BUILD) $2')')m4_dnl
m4_define(FULLVERSION,MAKEFULLVERSION(BUILDTYPE,ARCH))m4_dnl
m4_define(FULLVERSIONNOBRACES,`m4_patsubst(FULLVERSION,`[\(\)]',`')')m4_dnl
