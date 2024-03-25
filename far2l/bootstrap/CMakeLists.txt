cmake_minimum_required (VERSION 3.0.2)

add_custom_target(bootstrap 
    DEPENDS farlang.templ lang.inc
)

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
set(LNGFILES
    "${BOOTSTRAP}/FarEng.lng"
    "${BOOTSTRAP}/FarRus.lng"
    "${BOOTSTRAP}/FarHun.lng"
    "${BOOTSTRAP}/FarUkr.lng"
)

add_custom_command(OUTPUT "${BOOTSTRAP}/farlang.templ"
    COMMAND cat "${SCRIPTS}/farlang.templ.m4" | perl -I. ${M4PL} > "${BOOTSTRAP}/farlang.templ"
    WORKING_DIRECTORY "${SCRIPTS}"
    DEPENDS ${DEPENDENCIES} "${SCRIPTS}/farlang.templ.m4" "${SCRIPTS}/far2l_m4.pl"
    COMMENT generating language template
)

add_custom_command(OUTPUT "${BOOTSTRAP}/lang.inc"
    COMMAND perl -I. "${SCRIPTS}/farlng.pl" "${BOOTSTRAP}/farlang.templ" "${BOOTSTRAP}"
    WORKING_DIRECTORY "${SCRIPTS}"
    DEPENDS "${BOOTSTRAP}/farlang.templ"
    COMMENT generating languages
)

add_custom_command(TARGET bootstrap PRE_BUILD
    COMMAND perl -I. "${SCRIPTS}/farver.pl" "${BOOTSTRAP}/farversion.inc" "${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}" "${VERSION_MAJOR}" "${VERSION_MINOR}" "${VERSION_PATCH}"
    WORKING_DIRECTORY "${SCRIPTS}"
    COMMENT generating version information
)

add_custom_command(TARGET bootstrap
    COMMAND perl -I. "${SCRIPTS}/mkhlf.pl" "${SCRIPTS}/FarEng.hlf.m4" | perl ${M4PL} > "${BOOTSTRAP}/FarEng.hlf"

    WORKING_DIRECTORY "${SCRIPTS}"
    DEPENDS ${DEPENDENCIES}
    COMMENT generating help eng
)
add_custom_command(TARGET bootstrap
    COMMAND perl -I. "${SCRIPTS}/mkhlf.pl" "${SCRIPTS}/FarRus.hlf.m4" | perl ${M4PL} > "${BOOTSTRAP}/FarRus.hlf"
    WORKING_DIRECTORY "${SCRIPTS}"
    DEPENDS ${DEPENDENCIES}
    COMMENT generating help rus
)
add_custom_command(TARGET bootstrap
    COMMAND perl -I. "${SCRIPTS}/mkhlf.pl" "${SCRIPTS}/FarHun.hlf.m4" | perl ${M4PL} > "${BOOTSTRAP}/FarHun.hlf"
    WORKING_DIRECTORY "${SCRIPTS}"
    DEPENDS ${DEPENDENCIES}
    COMMENT generating help hun
)
add_custom_command(TARGET bootstrap
    COMMAND perl -I. "${SCRIPTS}/mkhlf.pl" "${SCRIPTS}/FarUkr.hlf.m4" | perl ${M4PL} > "${BOOTSTRAP}/FarUkr.hlf"
    WORKING_DIRECTORY "${SCRIPTS}"
    DEPENDS ${DEPENDENCIES}
    COMMENT generating help ukr
)

# older cmake versions does not support copying of multiple files,
# so we use simple stupid /bin/cp and wildcards

add_custom_command(TARGET bootstrap
    POST_BUILD
    DEPENDS "${LNGFILES}"
    COMMAND "cp" "${BOOTSTRAP}/*.lng" "${INSTALL_DIR}"
)

add_custom_command(TARGET bootstrap
    POST_BUILD
    DEPENDS "${HLFFILES}"
    COMMAND "cp" "${BOOTSTRAP}/*.hlf" "${INSTALL_DIR}"
)

add_custom_command(TARGET bootstrap
    POST_BUILD
    COMMAND "cp" "${CMAKE_CURRENT_SOURCE_DIR}/*.sh" "${INSTALL_DIR}"
    COMMAND "cp" "${CMAKE_CURRENT_SOURCE_DIR}/*.vbs" "${INSTALL_DIR}"
    COMMAND "cp" "${CMAKE_CURRENT_SOURCE_DIR}/*.ini" "${INSTALL_DIR}"
)
