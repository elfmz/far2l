
message(STATUS "generating headers and languages")

# todo: detect architecture 64 or 32
# LBITS := $(shell getconf LONG_BIT)

set(DIRBIT 64)


# set(RM rm) actually not needed: use $(CMAKE_COMMAND) -E remove
set(GAWK gawk)
set(M4 m4)
set(M4AGS -P -DFARBIT=${DIRBIT})

set(TOOLS ${CMAKE_BUILD_DIR}/tools)
set(BOOTSTRAP ${CMAKE_BUILD_DIR}/bootstrap)
set(SCRIPTS $(CMAKE_CURRENT_SOURCE_DIR)/scripts)

add_custom_target(bootstrap)

add_custom_command(TARGET bootstrap PRE_BUILD
   DEPENDS copyright.inc.m4 farversion.m4 tools.m4 vbuild.m4
   COMMAND ${M4} ${M4ARGS} copyright.inc.m4 | ${GAWK} -f $(SCRIPTS)/enc.awk > "${BOOTSTRAP}/copyright.inc"
)

add_custom_command(TARGET bootstrap PRE_BUILD
   COMMAND ${M4} ${M4ARGS} farversion.inc.m4 > "${BOOTSTRAP}/farversion.inc"
   DEPENDS copyright.inc.m4 farversion.m4 tools.m4 vbuild.m4
)

add_custom_command(TARGET bootstrap PRE_BUILD
   COMMAND ${M4} ${M4ARGS} farlang.templ.m4 > ${BOOTSTRAP}/farlang.templ
   DEPENDS farlang.templ.m4 farversion.m4 tools.m4 vbuild.m4
)

add_custom_command(TARGET bootstrap PRE_BUILD
   COMMAND ${GAWK} -f ${SCRIPTS}/mkhlf.awk FarEng.hlf.m4 | ${M4} > FarEng.hlf
   DEPENDS FarEng.hlf.m4 farversion.m4 tools.m4 vbuild.m4
)
add_custom_command(TARGET bootstrap PRE_BUILD
   COMMAND ${GAWK} -f ${SCRIPTS}/mkhlf.awk FarRus.hlf.m4 | ${M4} > FarRus.hlf
   DEPENDS FarRus.hlf.m4 farversion.m4 tools.m4 vbuild.m4
)
add_custom_command(TARGET bootstrap PRE_BUILD
   COMMAND ${GAWK} -f ${SCRIPTS}/mkhlf.awk FarHun.hlf.m4 | ${M4} > FarHun.hlf
   DEPENDS FarHun.hlf.m4 farversion.m4 tools.m4 vbuild.m4
)

