project(far2l_gui)

include(${wxWidgets_USE_FILE})

set(SOURCES
	Paint.cpp
	CustomDrawChar.cpp
	wxMain.cpp
	wxClipboardBackend.cpp
	ExclusiveHotkeys.cpp
	wxWinTranslations.cpp
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(SOURCES
		${SOURCES}
		Mac/touchbar.mm
		Mac/dockicon.cpp
		Mac/pasteboard.mm
	)

	set_property (TARGET far2l_gui APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc")
endif()


add_library (far2l_gui MODULE ${SOURCES})

target_include_directories(far2l_gui PRIVATE ..)
target_include_directories(far2l_gui PRIVATE ../..)
target_include_directories(far2l_gui PRIVATE ../../..)
target_include_directories(far2l_gui PRIVATE ../../../../utils/include)
target_include_directories(far2l_gui PRIVATE ${USEWX_INCS})
target_link_libraries(far2l_gui utils ${USEWX_LIBS})

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(far2l_gui PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(far2l_gui "${CMAKE_THREAD_LIBS_INIT}")
endif()

set_target_properties(far2l_gui
  PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}"
    PREFIX ""
    SUFFIX ".so")

#message(FATAL_ERROR "!!!ABI_HASH=${ABI_HASH}!!!")
target_compile_definitions(far2l_gui PUBLIC -DABI_HASH=${ABI_HASH})

