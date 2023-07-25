m4_define(`get_date', `m4_esyscmd(date "+%d/%m/%Y")')dnl
m4_define(`DATE', get_date)dnl
m4_define(BLD_YEAR,m4_substr(DATE,6,4))m4_dnl
m4_define(BLD_MONTH,m4_substr(DATE,3,2))m4_dnl
m4_define(BLD_DAY,m4_substr(DATE,0,2))m4_dnl
m4_define(`DATE', BLD_DAY`/'BLD_MONTH`/'BLD_YEAR)m4_dnl
m4_define(COPYRIGHTYEARS,m4_ifelse(`2016',BLD_YEAR,`2016',`2016-'BLD_YEAR))m4_dnl
m4_define(FULLVERSION,`MAJOR.MINOR.PATCH ARCH')m4_dnl
m4_define(FULLVERSIONNOBRACES,`m4_patsubst(FULLVERSION,`[\(\)]',`')')m4_dnl
