
cmake_minimum_required (VERSION 3.3.2)
add_custom_target(bootstrap 
  DEPENDS copyright.inc farversion.inc
)

message(STATUS "generating headers and languages")

# todo: detect architecture 64 or 32
# LBITS := $(shell getconf LONG_BIT)

set(DIRBIT 64)


# set(RM rm) actually not needed: use $(CMAKE_COMMAND) -E remove
set(GAWK gawk)
set(M4 m4 -P -DFARBIT=${DIRBIT})

set(TOOLS "${CMAKE_BUILD_DIR}/tools")
set(BOOTSTRAP "${PROJECT_BINARY_DIR}/bootstrap")
set(SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
set(DEPENDENCIES
  "${SCRIPTS}/copyright.inc.m4"
  "${SCRIPTS}/farversion.m4"
  "${SCRIPTS}/tools.m4"
  "${SCRIPTS}/vbuild.m4"
)



add_custom_command(OUTPUT "${BOOTSTRAP}/copyright.inc"
   COMMAND ${M4} "${SCRIPTS}/copyright.inc.m4" | ${GAWK} -f "${SCRIPTS}/enc.awk" > "${BOOTSTRAP}/copyright.inc"
   DEPENDS ${DEPENDENCIES}
   WORKING_DIRECTORY "${SCRIPTS}"
   COMMENT generating header
)

add_custom_command(OUTPUT "${BOOTSTRAP}/farversion.inc"
   COMMAND ${M4} "${SCRIPTS}/farversion.inc.m4" > "${BOOTSTRAP}/farversion.inc"
   DEPENDS ${DEPENDENCIES}
   WORKING_DIRECTORY "${SCRIPTS}"
   COMMENT generating header
)

add_custom_command(OUTPUT "${BOOTSTRAP}/farlang.templ"
   COMMAND ${M4} "${SCRIPTS}/farlang.templ.m4" > "${BOOTSTRAP}/farlang.templ"
   DEPENDS "${SCRIPTS}/farlang.templ.m4" ${DEPENDENCIES}
   WORKING_DIRECTORY "${SCRIPTS}"
   COMMENT generating language template
)

add_custom_command(TARGET bootstrap
   POST_BUILD
   COMMAND ${GAWK} -f ${SCRIPTS}/mkhlf.awk "${SCRIPTS}/FarEng.hlf.m4" | ${M4} > FarEng.hlf
   DEPENDS "${SCRIPTS}/FarEng.hlf.m4" ${DEPENDENCIES}
   WORKING_DIRECTORY "${SCRIPTS}"
   COMMENT generating lang eng
)
add_custom_command(TARGET bootstrap
   POST_BUILD
   COMMAND ${GAWK} -f ${SCRIPTS}/mkhlf.awk "${SCRIPTS}/FarRus.hlf.m4" | ${M4} > FarRus.hlf
   DEPENDS "${SCRIPTS}/FarRus.hlf.m4" ${DEPENDENCIES}
   WORKING_DIRECTORY "${SCRIPTS}"
   COMMENT generating lang rus
)
add_custom_command(TARGET bootstrap
   POST_BUILD
   COMMAND ${GAWK} -f ${SCRIPTS}/mkhlf.awk "${SCRIPTS}/FarHun.hlf.m4" | ${M4} > FarHun.hlf
   DEPENDS "${SCRIPTS}/FarHun.hlf.m4" ${DEPENDENCIES}
   WORKING_DIRECTORY "${SCRIPTS}"
   COMMENT generating lang hun
)
