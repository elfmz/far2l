project(far2l_gui)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_program(LSB_RELEASE_EXEC lsb_release)
    execute_process(COMMAND ${LSB_RELEASE_EXEC} -ds
        OUTPUT_VARIABLE LSB_RELEASE
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (LSB_RELEASE MATCHES "Ubuntu 20|Linux Mint 20")
        add_compile_definitions(WX304PATCH)
    endif()
endif()

set(SOURCES
    Paint.cpp
    CustomDrawChar.cpp
    wxMain.cpp
    wxClipboardBackend.cpp
    ExclusiveHotkeys.cpp
    wxWinTranslations.cpp
    wxConsoleInputShim.cpp
)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(SOURCES
        ${SOURCES}
        Mac/touchbar.mm
        Mac/dockicon.cpp
        Mac/pasteboard.mm
        Mac/hide.mm
        Mac/init.mm
        Mac/displaynotify.mm
    )
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    find_package(wxWidgets REQUIRED core base adv)
else()
    find_package(wxWidgets REQUIRED core base)
endif()

include(${wxWidgets_USE_FILE})

add_library (far2l_gui MODULE ${SOURCES})

target_include_directories(far2l_gui PRIVATE ..)
target_include_directories(far2l_gui PRIVATE ../..)
target_include_directories(far2l_gui PRIVATE ../../..)
target_include_directories(far2l_gui PRIVATE ../../../../utils/include)
target_include_directories(far2l_gui PRIVATE ${wxWidgets_INCLUDE_DIRS})


#${wxWidgets_CXX_FLAGS}

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(far2l_gui utils far2l "-framework IOKit -framework Carbon -framework Cocoa -framework AudioToolbox -framework System -framework OpenGL" ${wxWidgets_LIBRARIES})
else()
    target_link_libraries(far2l_gui utils far2l ${wxWidgets_LIBRARIES})
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${wxWidgets_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${wxWidgets_CXX_FLAGS}")

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
    target_compile_options(far2l_gui PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(far2l_gui "${CMAKE_THREAD_LIBS_INIT}")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set_property (TARGET far2l_gui APPEND_STRING PROPERTY COMPILE_FLAGS "-fobjc-arc")
endif()

find_package(X11)
if(X11_FOUND)
    link_directories(${X11_LIBRARIES})
    include_directories(${X11_INCLUDE_DIR})
    target_link_libraries(far2l_gui ${X11_LIBRARIES})

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__HASX11__")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__HASX11__")
endif()

set_target_properties(far2l_gui
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${INSTALL_DIR}"
        PREFIX ""
        SUFFIX ".so")
