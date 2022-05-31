
add_subdirectory(codepages)

set(SOURCES
casemap.c
collation.c
compose.c
cpsymbol.c
cptable.c
decompose.c
locale.c
mbtowc.c
sortkey.c
utf8.c
wctomb.c
wctype.c

codepages/c_037.c
codepages/c_424.c
codepages/c_437.c
codepages/c_500.c
codepages/c_737.c
codepages/c_775.c
codepages/c_850.c
codepages/c_852.c
codepages/c_855.c
codepages/c_856.c
codepages/c_857.c
codepages/c_860.c
codepages/c_861.c
codepages/c_862.c
codepages/c_863.c
codepages/c_864.c
codepages/c_865.c
codepages/c_866.c
codepages/c_866.uk.c
codepages/c_869.c
codepages/c_874.c
codepages/c_875.c
codepages/c_878.c
codepages/c_1006.c
codepages/c_1026.c
codepages/c_1250.c
codepages/c_1251.c
codepages/c_1252.c
codepages/c_1253.c
codepages/c_1254.c
codepages/c_1255.c
codepages/c_1256.c
codepages/c_1257.c
codepages/c_1258.c
codepages/c_10000.c
codepages/c_10004.c
codepages/c_10005.c
codepages/c_10006.c
codepages/c_10007.c
codepages/c_10010.c
codepages/c_10017.c
codepages/c_10021.c
codepages/c_10029.c
codepages/c_10079.c
codepages/c_10081.c
codepages/c_10082.c
codepages/c_20127.c
codepages/c_20866.c
codepages/c_20880.c
codepages/c_21866.c
codepages/c_28591.c
codepages/c_28592.c
codepages/c_28593.c
codepages/c_28594.c
codepages/c_28595.c
codepages/c_28596.c
codepages/c_28597.c
codepages/c_28598.c
codepages/c_28599.c
codepages/c_28600.c
codepages/c_28603.c
codepages/c_28604.c
codepages/c_28605.c
codepages/c_28606.c
)

if (EACP)
	set(SOURCES
		${SOURCES}
		codepages/c_932.c
		codepages/c_936.c
		codepages/c_949.c
		codepages/c_950.c
		codepages/c_1361.c
		codepages/c_10001.c
		codepages/c_10002.c
		codepages/c_10003.c
		codepages/c_10008.c
		codepages/c_20932.c
	)
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNO_EACP")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_EACP")
endif()


add_library (wineguts ${SOURCES})

