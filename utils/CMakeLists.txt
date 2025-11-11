
project(utils)

find_package(PkgConfig REQUIRED)

if(NOT DEFINED ICU_MODE)
    message(STATUS "Using prebuilt character classes by default")
    set(CHAR_CLASSES_CPP src/CharClasses.cpp)

elseif("${ICU_MODE}" STREQUAL "prebuilt")
    message(STATUS "Using prebuilt character classes due to ICU_MODE=${ICU_MODE}")
    set(CHAR_CLASSES_CPP src/CharClasses.cpp)

elseif("${ICU_MODE}" STREQUAL "build")
    message(STATUS "Buld-time character classes generaction due to ICU_MODE=${ICU_MODE}")
    # Not using CMAKE_CXX_COMPILER due to it can be cross-compilation but here we want to compile native executable
    # TODO: there must be some less lame way to use host's compiler and host's libicuuc
    set(CHAR_CLASSES_CPP ${CMAKE_CURRENT_BINARY_DIR}/tmpCharClasses.cpp)
    add_custom_command(
        OUTPUT ${CHAR_CLASSES_CPP}
        COMMAND c++ ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}} ${CMAKE_CURRENT_SOURCE_DIR}/src/CharClasses_mk.cpp -licuuc -o ${CMAKE_CURRENT_BINARY_DIR}/tmpCharClasses_mk && ${CMAKE_CURRENT_BINARY_DIR}/tmpCharClasses_mk > ${CHAR_CLASSES_CPP}
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT ${CHAR_CLASSES_CPP}
    )
    add_custom_target(utils_icu
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/tmpCharClasses.cpp
    )

elseif("${ICU_MODE}" STREQUAL "runtime")
    find_package(ICU COMPONENTS uc)
    if(NOT ICU_FOUND)
        message(FATAL_ERROR "${ColorRed}ICUUC not found but required by ICU_MODE=${ICU_MODE}${ColorNormal}")
    endif()
    message(STATUS "Using runtime ICUUC dependency due to ICU_MODE=${ICU_MODE}, ICU_LIBRARIES=${ICU_LIBRARIES}, ICU_INICLUDE_DIRS=${ICU_INCLUDE_DIRS}")
    set(CHAR_CLASSES_CPP src/CharClassesICU.cpp)

else()
    message(FATAL_ERROR "${ColorRed}Bad ICU_MODE=${ICU_MODE}, use one of: prebuilt build runtime${ColorNormal}")
endif()

set(SOURCES
    src/Threaded.cpp
    src/ThreadedWorkQueue.cpp
    src/SharedResource.cpp
    src/KeyFileHelper.cpp
    src/utils.cpp
    src/InstallPath.cpp
    src/StrPrintf.cpp
    src/TimeUtils.cpp
    src/StringConfig.cpp
    src/InMy.cpp
    src/ZombieControl.cpp
    src/base64.cpp
    src/Event.cpp
    src/StackSerializer.cpp
    src/ScopeHelpers.cpp
    src/crc64.c
    src/TTYRawMode.cpp
    src/LocalSocket.cpp
    src/FilePathHashSuffix.cpp
    src/Environment.cpp
    src/Escaping.cpp
    src/WideMB.cpp
    src/FSNotify.cpp
    src/TestPath.cpp
    src/PipeIPC.cpp
    src/PathParts.cpp
    src/POpen.cpp
    src/VT256ColorTable.cpp
    src/ReadWholeFile.cpp
    src/WriteWholeFile.cpp
    src/ThrowPrintf.cpp
    src/FcntlHelpers.cpp
    src/Panic.cpp
    src/IntStrConv.cpp
    src/EnsureDir.cpp
    src/RandomString.cpp
    src/MakePTYAndFork.cpp
    src/LookupDebugSymbol.cpp
    src/ExecAsync.cpp
    ${CHAR_CLASSES_CPP}
)

add_library (utils STATIC ${SOURCES})

target_include_directories(utils PRIVATE include)

if("${ICU_MODE}" STREQUAL "build")
    add_dependencies(utils utils_icu)
elseif("${ICU_MODE}" STREQUAL "runtime")
    if (APPLE)
        target_include_directories(utils PRIVATE
         "$<BUILD_INTERFACE:/opt/homebrew/include>"
        )
    endif ()
    target_link_libraries(utils ${ICU_LIBRARIES})
endif()

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
    target_compile_options(utils PUBLIC "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(utils "${CMAKE_THREAD_LIBS_INIT}")
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Haiku")
    target_link_libraries(utils network)
endif()
