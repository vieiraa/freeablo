cmake_minimum_required (VERSION 3.8)

set(
    HUNTER_CACHE_SERVERS
    "https://github.com/wheybags/hunter-cache"
    CACHE
    STRING
    "Default cache server"
)

set(
    HUNTER_PASSWORDS_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Hunter/passwords.cmake"
    CACHE
    FILEPATH
    "Hunter passwords"
)

string(COMPARE EQUAL "$ENV{GITHUB_USER_PASSWORD}" "" password_is_empty)

if (NOT password_is_empty)
    option(HUNTER_RUN_UPLOAD "Upload cache binaries" ON)
endif()

## Windows Config ##
if (WIN32)
    set (CMAKE_SYSTEM_VERSION 8.1 CACHE TYPE INTERNAL FORCE)

    set(desired_generators "Visual Studio 14 2015 Win64" "Visual Studio 15 2017 Win64")

    if (NOT "${CMAKE_GENERATOR}" IN_LIST desired_generators)
        list(GET desired_generators 0 VS2015)
        list(GET desired_generators 1 VS2017)
        message(FATAL_ERROR "Incorrect generator, please run cmake with: -G \"${VS2015}\"\nor with -G \"${VS2017}\"")
    endif()

    set(HUNTER_CMAKE_GENERATOR "${CMAKE_GENERATOR}")
endif()


# cmake policies
IF (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.11)
	cmake_policy(SET CMP0074 NEW) # find_library calls use $LIB_ROOT variables. Shouldn't affect us much/at all, but silences warnings
ENDIF()

include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.20.64.tar.gz"
    SHA1 "7b830dfda7a094b2af15f44b24ebd2489404d880"
    LOCAL
)

project (Freeablo)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

hunter_add_package(SDL2)
find_Package(SDL2 CONFIG REQUIRED)

hunter_add_package(SDL_image)
find_package(SDL_image CONFIG REQUIRED)

hunter_add_package(SDL_mixer)
find_package(SDL_mixer CONFIG REQUIRED)

hunter_add_package(PNG)
find_package(PNG CONFIG REQUIRED)

hunter_add_package(enet)
find_package(enet CONFIG REQUIRED)

hunter_add_package(ZLIB)
find_package(ZLIB CONFIG REQUIRED)

hunter_add_package(stormlib)
find_package(stormlib CONFIG REQUIRED)

hunter_add_package(Boost COMPONENTS filesystem system program_options date_time regex)
find_package(Boost CONFIG REQUIRED  filesystem system program_options date_time regex)
set(HUNTER_BOOST_LIBS Boost::filesystem Boost::system Boost::program_options Boost::date_time Boost::regex)

Find_Package(OpenGL REQUIRED)

set(FA_COMPILER_FLAGS "")

# Use C++11
if (CMAKE_VERSION VERSION_LESS "3.1")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set (CMAKE_CXX_FLAGS "--std=gnu++11 ${CMAKE_CXX_FLAGS}")
    endif ()
else ()
    set (CMAKE_CXX_STANDARD 11)
endif ()
if(WIN32)
	add_definitions(-DNOMINMAX) # http://stackoverflow.com/questions/5004858/stdmin-gives-error
endif()

option (FA_TREAT_WARNINGS_AS_ERRORS "Treat warnings as errors")

if(UNIX)
    set(FA_COMPILER_FLAGS "${FA_COMPILER_FLAGS} -Wall -pedantic -Wextra -Wno-unknown-pragmas -Wno-missing-braces")
    SET(CMAKE_INCLUDE_SYSTEM_FLAG_CXX "-isystem ")
    if (FA_TREAT_WARNINGS_AS_ERRORS)
        set(FA_COMPILER_FLAGS "${FA_COMPILER_FLAGS} -Werror")
    endif()
endif()

if(MSVC)
    set(FA_COMPILER_FLAGS "${FA_COMPILER_FLAGS} /MP /W4 /D_CRT_SECURE_NO_WARNINGS /wd4244 /wd4100 /wd4611 /wd4458 /wd4459 /wd4127 /wd4800 /wd4267 /wd4172")
    # 4244 - e.g. 'argument': conversion from 'const long double' to 'double' -- boost headers
    # 4100 - unreferenced formal parameter -- boost headers
    # 4172 - boost headers
    # 4611 - interaction between '_setjmp' and C++ object destruction is non-portable -- savepng.cpp
    # 4458 - shadow for class member -- librocket headers
    # 4459 - declaration of 'self' hides global declaration
    # 4127 - conditional expression is constant, could be useful to check that both branches compilable.
    # 4267 -  conversion from 'size_t' to 'int', possible loss of data -- too many such warnings in 64-bit build currently
    # 4800 - forcing value to bool, stupid warning
    set(FA_COMPILER_FLAGS "${FA_COMPILER_FLAGS} /w44062")
    # 4061 - not all enumerator values are handled by the switch statement
    add_definitions (-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS )
    # SCL - because current boost is too old and emits this warning
    # CRT - something in nuklear.h
    if (FA_TREAT_WARNINGS_AS_ERRORS)
        set(FA_COMPILER_FLAGS "${FA_COMPILER_FLAGS} /WX")
    endif()

endif()

#TODO: get rid of this?
include_directories("${PROJECT_SOURCE_DIR}/components/")

# allow the user to specify extra include dirs and defines on the command line
if(CLI_INCLUDE_DIRS)
    include_directories(${CLI_INCLUDE_DIRS})
endif()
if(CLI_DEFINES)
	add_definitions(${CLI_DEFINES})
endif()


add_subdirectory (extern)
add_subdirectory (components)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(apps/freeablo)
add_subdirectory(apps/celview)
add_subdirectory(apps/mpqtool)
add_subdirectory(apps/exedump)
add_subdirectory(apps/launcher)
add_subdirectory(apps/fontgenerator)
add_subdirectory(test)

if(MSVC)
    set_property(TARGET freeablo PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
    set_property(TARGET celview PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
    set_property(TARGET exedump PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
    set_property(TARGET launcher PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
    set_property(TARGET unit_tests PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()


# TODO: use the new FILE(CREATE_LINK) comand in newer cmake (+upgrade cmake on ci workers)
if (WIN32)
    string(REPLACE "/" "\\" SYM_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}\\resources")
    string(REPLACE "/" "\\" SYM_DEST "${CMAKE_CURRENT_BINARY_DIR}\\resources")
    
    if (NOT EXISTS "${SYM_DEST}")
        exec_program("cmd" ARGS "/wait" "/c" "mklink" "/j" "${SYM_DEST}" "${SYM_SOURCE}")
    endif()
else()
    if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/resources")
        execute_process(COMMAND "ln" "-s" "${CMAKE_CURRENT_SOURCE_DIR}/resources" "${CMAKE_CURRENT_BINARY_DIR}/resources")
    endif()
endif()

