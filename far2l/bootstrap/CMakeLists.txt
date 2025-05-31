message(STATUS "generating headers and languages")

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(LBITS 64)
    set(DIRBIT 64)
else ()
    set(LBITS 32)
    set(DIRBIT 32)
endif ()

set(BOOTSTRAP "${PROJECT_BINARY_DIR}/bootstrap")
set(SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/scripts")

set(M4PL ${SCRIPTS}/far2l_m4.pl "${VERSION_MAJOR}" "${VERSION_MINOR}" "${VERSION_PATCH}" "${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")

set(HLFFILES
    "${BOOTSTRAP}/FarEng.hlf"
    "${BOOTSTRAP}/FarRus.hlf"
    "${BOOTSTRAP}/FarHun.hlf"
    "${BOOTSTRAP}/FarUkr.hlf"
)

add_custom_target(bootstrap
        DEPENDS farlang.templ lang.inc ${HLFFILES}
)

add_custom_command(OUTPUT "${BOOTSTRAP}/farlang.templ"
    COMMAND cat "${SCRIPTS}/farlang.templ.m4" | perl -I. ${M4PL} > "${BOOTSTRAP}/farlang.templ"
    WORKING_DIRECTORY "${SCRIPTS}"
    DEPENDS "${SCRIPTS}/farlang.templ.m4" "${SCRIPTS}/far2l_m4.pl"
    COMMENT "generating language template"
)

add_custom_command(OUTPUT "${BOOTSTRAP}/lang.inc"
    COMMAND perl -I. "${SCRIPTS}/farlng.pl" "${BOOTSTRAP}/farlang.templ" "${BOOTSTRAP}"
    WORKING_DIRECTORY "${SCRIPTS}"
    DEPENDS "${BOOTSTRAP}/farlang.templ"
    COMMENT "generating languages"
)

add_custom_command(TARGET bootstrap PRE_BUILD
    COMMAND perl -I. "${SCRIPTS}/farver.pl" "${BOOTSTRAP}/farversion.inc" "${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}" "${VERSION_MAJOR}" "${VERSION_MINOR}" "${VERSION_PATCH}"
    WORKING_DIRECTORY "${SCRIPTS}"
    COMMENT "generating version information"
)

# List of langs for help files
set(LANGS "Eng" "Rus" "Hun" "Ukr")
foreach(LANG ${LANGS})
    add_custom_command(OUTPUT "${BOOTSTRAP}/Far${LANG}.hlf"
            COMMAND perl -I. "${SCRIPTS}/mkhlf.pl" "${SCRIPTS}/Far${LANG}.hlf.m4" | perl ${M4PL} > "${BOOTSTRAP}/Far${LANG}.hlf"
            WORKING_DIRECTORY "${SCRIPTS}"
            DEPENDS "${SCRIPTS}/Far${LANG}.hlf.m4"
            COMMENT "generating help ${LANG}"
    )
endforeach()

# older cmake versions does not support copying of multiple files,
# so we use simple stupid /bin/cp and wildcards

add_custom_command(TARGET bootstrap
        POST_BUILD
        COMMAND "cp" "${BOOTSTRAP}/*.lng" "${INSTALL_DIR}"
)

add_custom_command(TARGET bootstrap
        POST_BUILD
        COMMAND "cp" "${BOOTSTRAP}/*.hlf" "${INSTALL_DIR}"
)

add_custom_command(TARGET bootstrap
    POST_BUILD
    COMMAND "cp" "${CMAKE_CURRENT_SOURCE_DIR}/*.sh" "${INSTALL_DIR}"
    COMMAND "cp" "${CMAKE_CURRENT_SOURCE_DIR}/*.vbs" "${INSTALL_DIR}"
    COMMAND "cp" "${CMAKE_CURRENT_SOURCE_DIR}/*.ini" "${INSTALL_DIR}"
)
