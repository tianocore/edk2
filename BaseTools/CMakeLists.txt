#
# MIT License
#
# Copyright (c) 2010 Joel Winarske
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
cmake_minimum_required(VERSION 3.12.4)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "MinSizeRel" CACHE STRING "Choose the type of build, options are: Debug, Release, or MinSizeRel." FORCE)
    message(STATUS "CMAKE_BUILD_TYPE not set, defaulting to MinSizeRel.")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(NOT BUILD_NUMBER)
    set(BUILD_NUMBER 0)
endif()

set(BASETOOLS_VERSION 1.0.${BUILD_NUMBER})

set(PACKAGE_NAME basetools)

project(${PACKAGE_NAME} VERSION "${BASETOOLS_VERSION}" LANGUAGES CXX C)

message(STATUS "Generator .............. ${CMAKE_GENERATOR}")
message(STATUS "Build Type ............. ${CMAKE_BUILD_TYPE}")


#
# Installation Directory
#
if(MSVC)
    set(INSTALL_FOLDER ${CMAKE_HOST_SYSTEM_NAME}-${CMAKE_VS_PLATFORM_NAME})
else()
    set(INSTALL_FOLDER ${CMAKE_HOST_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR})
endif()

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/Bin/${INSTALL_FOLDER})

message(STATUS "Install Dir ............ ${CMAKE_INSTALL_PREFIX}")


#
# Build Version
#
include(FindGit)
if(Git_FOUND)

    # commit hash
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" log -1 --format=%h
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    # branch
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --abbrev-ref HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE res
        OUTPUT_VARIABLE GIT_BRANCH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if(NOT res STREQUAL 0)
        set(BUILD_VERSION "Developer Build based on Revision: ${CMAKE_PROJECT_VERSION}")
    else()
        set(BUILD_VERSION "commit ${GIT_COMMIT_HASH} (${GIT_BRANCH})")
    endif()
else()
    set(BUILD_VERSION "Developer Build based on Revision: ${CMAKE_PROJECT_VERSION}")
endif()
message(STATUS "Build Version .......... ${BUILD_VERSION}")


configure_file(cmake/BuildVersion.h.in ${CMAKE_CURRENT_SOURCE_DIR}/Source/C/Include/Common/BuildVersion.h)


#
# Common Build Flags
#
if(MSVC)
    message(STATUS "Compiler ............... MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /nologo /wd4996 /wd4267 /wd4244 /wd4068 /wd4477 /wd4334 /wd4313 /D _CRT_SECURE_NO_DEPRECATE /D _CRT_NONSTDC_NO_DEPRECATE " CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} /nologo /wd4996" CACHE STRING "" FORCE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /nologo /OPT:REF /OPT:ICF=10 /incremental:no /nodefaultlib:libc.lib" CACHE STRING "" FORCE)
elseif (CMAKE_C_COMPILER_ID MATCHES "Clang")
    message(STATUS "Compiler ............... Clang")
    set(CLANG_C_FLAGS "-MD -fshort-wchar -fno-strict-aliasing -Wall -Werror -Wno-unused-label")
    set(CLANG_C_FLAGS "${CLANG_C_FLAGS} -Wno-unused-variable -Wno-uninitialized -Wno-format -Wno-self-assign -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CLANG_C_FLAGS}" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-register" CACHE STRING "" FORCE)
else()
    message(STATUS "Compiler ............... GCC")
    set(GCC_C_FLAGS "-MD -fshort-wchar -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks")
    set(GCC_C_FLAGS "${GCC_C_FLAGS} -Wall -Werror -Wno-unused-but-set-variable -Wno-unused-result")
    set(GCC_C_FLAGS "${GCC_C_FLAGS} -Wno-maybe-uninitialized -Wno-unknown-pragmas -Wno-uninitialized")
    set(GCC_C_FLAGS "${GCC_C_FLAGS} -Wno-format -Wno-unused-label -Wno-unused-variable -Wno-maybe-uninitialized")
    set(GCC_C_FLAGS "${GCC_C_FLAGS} -Wno-misleading-indentation -Wno-unused-but-set-variable -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_C_FLAGS}" CACHE STRING "" FORCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_C_FLAGS}" CACHE STRING "" FORCE)
endif()


#
# Target Architecture
#
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
    set(TARGET_ARCH Ia32)
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win64")
    set(TARGET_ARCH X64)
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    set(TARGET_ARCH AArch64)
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
    set(TARGET_ARCH Arm)
else()  # Default to checking host arch
    if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "IA32" OR
        ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "i386")
        set(TARGET_ARCH Ia32)
    elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "x86_64" OR
        ${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
        set(TARGET_ARCH X64)
    elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AARCH64")
        set(TARGET_ARCH AArch64)
    elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "ARM")
        set(TARGET_ARCH Arm)
    endif()
endif()

message(STATUS "Architecture ........... ${TARGET_ARCH}")


#
# Common Library
#
file(GLOB lib_src_files Source/C/Common/*.c)
add_library(common-lib STATIC ${lib_src_files})
set_target_properties(common-lib PROPERTIES OUTPUT_NAME Common)
target_include_directories(common-lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/C
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/C/Include
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/C/Include/${TARGET_ARCH}
  ${CMAKE_CURRENT_SOURCE_DIR}/Source/C/Common
)


#
# Macros
#
macro(add_headers_and_source proj path source_type)
    file(GLOB ${proj}_headers ${path}/*.h)
    foreach(h ${${proj}_headers})
        get_filename_component(header ${h} NAME)
    endforeach(h)

    file(GLOB ${proj}_src ${path}/*.${source_type})
    foreach(s ${${proj}_src})
        get_filename_component(src ${s} NAME)
    endforeach(s)
endmacro(add_headers_and_source)

macro(add_exe proj path)
    add_headers_and_source(${proj} ${path} c)
    add_executable(${proj} ${${proj}_headers} ${${proj}_src})
    target_include_directories(${proj} PUBLIC ${path})
    target_link_libraries(${proj} common-lib)
    add_dependencies(${proj} common-lib)
    install(TARGETS ${proj} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
endmacro(add_exe)


#
# Executables
#
add_exe(EfiRom Source/C/EfiRom)
add_exe(GenFfs Source/C/GenFfs)
add_exe(GenFv Source/C/GenFv)
add_exe(GenFw Source/C/GenFw)
add_exe(GenSec Source/C/GenSec)
add_exe(GenCrc32 Source/C/GenCrc32)
add_exe(Split Source/C/Split)
add_exe(TianoCompress Source/C/TianoCompress)
add_exe(VolInfo Source/C/VolInfo)
add_exe(DevicePath Source/C/DevicePath)


#
# BrotliCompress
#
set(proj1 Brotli)
set(path1 ${CMAKE_CURRENT_SOURCE_DIR}/Source/C/BrotliCompress)
add_headers_and_source(brotli_tools  ${path1}/tools c)
add_headers_and_source(brotli_common ${path1}/common c)
add_headers_and_source(brotli_dec    ${path1}/dec c)
add_headers_and_source(brotli_enc    ${path1}/enc c)
add_executable(${proj1}
    ${brotli_tools_src}  ${brotli_tools_headers}
    ${brotli_common_src} ${brotli_common_headers}
    ${brotli_dec_src}    ${brotli_dec_headers}
    ${brotli_enc_src}    ${brotli_enc_headers})
target_include_directories(${proj1} PUBLIC ${path1}/include)
if(NOT MSVC)
    target_link_libraries(${proj1} -lm)
endif()

install(TARGETS ${proj1} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})


#
# LzmaCompress
#
set(proj2 LzmaCompress)
set(path2 ${CMAKE_CURRENT_SOURCE_DIR}/Source/C/${proj2})
add_headers_and_source(LzmaCompress_SDK ${path2}/Sdk/C c)
if(NOT MSVC)
    list(REMOVE_ITEM LzmaCompress_SDK_src ${path2}/Sdk/C/LzFindMt.c)
    list(REMOVE_ITEM LzmaCompress_SDK_src ${path2}/Sdk/C/Threads.c)
endif()
add_executable(${proj2} ${path2}/LzmaCompress.c ${LzmaCompress_SDK_src})
target_link_libraries(${proj2} common-lib)
add_dependencies(${proj2} common-lib)
if(NOT MSVC)
    target_compile_definitions(${proj2} PUBLIC _7ZIP_ST)
endif()

install(TARGETS ${proj2} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
if(MSVC)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/C/LzmaCompress/LzmaF86Compress.bat
        DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()


#
# VfrCompile
#
set(proj3 VfrCompile)
set(path3 ${CMAKE_CURRENT_SOURCE_DIR}/Source/C/${proj3})
set(PCCTS_HOME ${path3}/Pccts)
add_headers_and_source(pccts ${path3}/Pccts/h c)

# dlg
add_headers_and_source(dlg ${path3}/Pccts/dlg c)
add_executable(dlg ${PCCTS_HOME}/support/set/set.c ${dlg_src})
target_include_directories(dlg PUBLIC ${path3} ${PCCTS_HOME}/h ${PCCTS_HOME}/dlg ${PCCTS_HOME}/support/set)
target_compile_definitions(dlg PUBLIC USER_ZZSYN PC ZZLEXBUFSIZE=65536 LONGFILENAMES)

# antlr
add_headers_and_source(dlg ${path3}/Pccts/antlr c)
add_executable(antlr ${PCCTS_HOME}/support/set/set.c ${dlg_src})
target_include_directories(antlr PUBLIC ${path3} ${PCCTS_HOME}/h ${PCCTS_HOME}/antlr ${PCCTS_HOME}/support/set)
target_compile_definitions(antlr PUBLIC USER_ZZSYN PC ZZLEXBUFSIZE=65536 LONGFILENAMES __USE_PROTOS)
add_dependencies(antlr dlg)

if(NOT MSVC)
    target_compile_definitions(antlr PUBLIC stricmp=strcasecmp)
    set(ANTLR_EXE ${CMAKE_BINARY_DIR}/antlr)
    set(DLG_EXE ${CMAKE_BINARY_DIR}/dlg)
else()
    set(ANTLR_EXE ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/antlr.exe)
    set(DLG_EXE ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/dlg.exe)
endif()

add_custom_command(OUTPUT VfrLexer.dlg
    WORKING_DIRECTORY ${path3}
    COMMENT "Generating VfrLexer.dlg"
    COMMAND "${ANTLR_EXE}" -CC -e3 -ck 3 -k 2 -fl VfrParser.dlg -ft VfrTokens.h -o . VfrSyntax.g
    DEPENDS antlr)
add_custom_target(generate_dlg DEPENDS VfrLexer.dlg)

add_custom_command(OUTPUT EfiVfrParser.cpp EfiVfrParser.h VfrLexer.cpp VfrLexer.h VfrSyntax.cpp VfrTokens.h
    WORKING_DIRECTORY ${path3}
    COMMENT "Generating Lexer Files"
    COMMAND "${DLG_EXE}" -C2 -i -CC -cl VfrLexer -o . VfrParser.dlg
    DEPENDS generate_dlg)
add_custom_target(generate_lexer DEPENDS EfiVfrParser.cpp VfrLexer.cpp VfrSyntax.cpp)

add_executable(${proj3}
    ${path3}/Pccts/h/AParser.cpp
    ${path3}/Pccts/h/DLexerBase.cpp
    ${path3}/Pccts/h/ATokenBuffer.cpp
    ${path3}/EfiVfrParser.cpp
    ${path3}/VfrLexer.cpp
    ${path3}/VfrSyntax.cpp
    ${path3}/VfrFormPkg.cpp
    ${path3}/VfrError.cpp
    ${path3}/VfrUtilityLib.cpp
    ${path3}/VfrCompiler.cpp)

set_source_files_properties(
    ${path3}/EfiVfrParser.cpp
    ${path3}/VfrLexer.cpp
    ${path3}/VfrSyntax.cpp
    PROPERTIES GENERATED TRUE)

target_include_directories(${proj3} PUBLIC ${path3} ${PCCTS_HOME}/h)
target_compile_definitions(${proj3} PUBLIC PCCTS_USE_NAMESPACE_STD)
if(NOT MSVC)
    target_compile_options(${proj3} PUBLIC -Wno-sign-compare -Wno-char-subscripts)
else()
    target_compile_options(${proj3} PUBLIC /wd4102 /wd4101 /wd4018 /EHsc)
endif()
target_link_libraries(${proj3} common-lib)
add_dependencies(${proj3} common-lib generate_lexer)

install(TARGETS ${proj3} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})


#
# Tests
#
include (FindPython2)
if(Python2_FOUND)

    include(CTest)
    add_test(NAME RunTests
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND "${Python2_EXECUTABLE}" Tests/RunTests.py)

    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --build-config ${CMAKE_BUILD_TYPE} --verbose)

    set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/Tests/CheckPythonSyntax.pyc"
        "${CMAKE_CURRENT_SOURCE_DIR}/Tests/CheckUnicodeSourceFiles.pyc"
        "${CMAKE_CURRENT_SOURCE_DIR}/Tests/CToolsTests.pyc"
        "${CMAKE_CURRENT_SOURCE_DIR}/Tests/PythonToolsTests.pyc"
        "${CMAKE_CURRENT_SOURCE_DIR}/Tests/TestTools.pyc"
        "${CMAKE_CURRENT_SOURCE_DIR}/Tests/TianoCompress.pyc")

endif()
