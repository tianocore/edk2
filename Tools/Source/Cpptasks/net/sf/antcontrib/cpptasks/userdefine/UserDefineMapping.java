/*
 * 
 * Copyright 2001-2004 The Ant-Contrib project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
package net.sf.antcontrib.cpptasks.userdefine;
public class UserDefineMapping {
    // list of Arch: EBC, ARM, IA32, X64, IPF, PPC, NT32
    public final static String[] arch = { "EBC", "ARM", "IA32", "X64", "IPF",
            "PPC", "NT32" };

    // list of OS: Linux, Windows
    public final static String[] os = { "WINDOWS", "LINUX" };

    // list of Vendor: Microsoft, Intel, Cygwin, Gcc
    public final static String[] vendor = { "MSFT", "INTEL", "GCC", "CYGWIN" };

    // list of Command Type: CC, LIB, LINK, ASL, ASM, ASMLINK
    public final static String[] commandType = { "CC", "LIB", "LINK", "ASL",
            "ASM", "ASMLINK", "PP" };

    //
    // flags mapping (Include compellingIncFileFlag,Path Delimiter, Output file
    // flag,
    // includepathfalge,
    // )
    // Examples: '/I' for MSFT cl.exe while '-I' for GCC
    // '/Fo' for MSFT cl.exe while '-o' for GCC
    //
    public final static String[][] compellingIncFileFlag = {
            { "MSFT_CC", "/FI" }, { "GCC_CC", "-include" },
            { "INTEL_CC", "-FI" }, { "WINDDK_CC", "/FI" },
            { "MSFT_ASM", "/FI" }, { "GCC_ASM", "-include" },
            { "WINDDK_ASM", "/FI" } };

    public final static String[][] includePathFlag = { { "MSFT_CC", "/I" },
            { "GCC_CC", "-I" }, { "INTEL_CC", "/I" }, { "WINDDK_CC", "/I" },
            { "MSFT_ASM", "/I" }, { "GCC_ASM", "-I" }, { "WINDDK_CC", "/I" },
            { "MSFT_PP", "/I" }, { "GCC_PP", "-I" }, { "WINDDK_PP", "/I" } };

    public final static String[][] outputFileFlag = { { "MSFT_CC", "/Fo" },
            { "GCC_CC", "-o" }, { "INTEL_CC", "/Fo" }, { "WINDDK_CC", "/Fo" },
            { "MSFT_LIB", "/OUT:" }, { "GCC_LIB", "-cr" },
            { "INTEL_LIB", "/OUT:" }, { "WINDDK_LIB", "/OUT:" },
            { "MSFT_LINK", "/OUT:" }, { "GCC_LINK", "-o" },
            { "INTEL_LINK", "/OUT:" }, { "WINDDK_LINK", "/OUT:" },
            { "MSFT_ASM", "/Fo" }, { "GCC_ASM", "-o" },
            { "WINDDK_ASM", "/Fo" },{"WINDDK_IPF_ASM", "-o"} };

    public final static String[][] subSystemFlag = {
            { "MSFT_LIB", "/SUBSYSTEM:" }, { "GCC_LIB", "--subsystem=" },
            { "WINDDk_LIB", "/SUBSYSTEM:" }, { "INTEL_LIB", "/SUBSYSTEM:" },
            { "MSFT_LINK", "/SUBSYSTEM:" }, { "GCC_LINK", "--subsystem=" },
            { "INTEL_LINK", "/SUBSYSTEM:" }, { "WINDDK_LINK", "/SUBSYSTEM:" } };

    public final static String[][] outputFileSuffix = {
            { "WINDOWS_ASM", ".obj" }, { "WINDOWS_CC", ".obj" },
            { "LINUX_ASM", ".o" }, { "LINUX_CC", ".o" } };

    public final static String[][] entryPointFlag = {
            { "MSFT_LINK", "/ENTRY:" }, { "GCC_LINK", "-e" },
            { "INTEL_LINK", "/ENTRY:" },
            { "WINDDK_LINK", "/ENTRY:" } };

    public final static String[][] mapFlag = { { "MSFT_LINK", "/MAP:" },
            { "GCC_LINK", "" }, { "INTEL_LINK", "/MAP:" },
            { "WINDDK_LINK", "/MAP:" } };

    public final static String[][] pdbFlag = { { "MSFT_LINK", "/PDB:" },
            { "GCC_LINK", "" }, { "INTEL_LINK", "" }, { "WINDDK_LINK", "/PDB:"} };

    public static String getIncludePathDelimiter(String vendor,
            String commandType) {
        String key = vendor + "_" + commandType;
        for (int i = 0; i < includePathFlag.length; i++) {
            if (includePathFlag[i][0].equalsIgnoreCase(key)) {
                return includePathFlag[i][1];
            }
        }
        return null;
    }

    public static String getOutputFileFlag(String vendor, String arch, String commandType) {
        /*
         * First find outputfileFlag by vendor_arch_commandType
         */
        String key = vendor + "_" + arch + "_" + commandType;
        for (int i = 0; i < outputFileFlag.length; i++) {
            if (outputFileFlag[i][0].equalsIgnoreCase(key)) {
                return outputFileFlag[i][1];
            }
        }
        key = vendor + "_" + commandType;
        for (int i = 0; i < outputFileFlag.length; i++) {
            if (outputFileFlag[i][0].equalsIgnoreCase(key)) {
                return outputFileFlag[i][1];
            }
        }
        return null;
    }

    public static String getCompellingIncFileFlag(String vendor,
            String commandType) {
        String key = vendor + "_" + commandType;
        for (int i = 0; i < compellingIncFileFlag.length; i++) {
            if (compellingIncFileFlag[i][0].equalsIgnoreCase(key)) {
                return compellingIncFileFlag[i][1];
            }
        }
        return null;
    }

    public static String getSubSystemFlag(String vendor, String commandType) {
        String key = vendor + "_" + commandType;
        for (int i = 0; i < subSystemFlag.length; i++) {
            if (subSystemFlag[i][0].equalsIgnoreCase(key)) {
                return subSystemFlag[i][1];
            }
        }
        return null;
    }

    public static String getEntryPointFlag(String vendor, String commandType) {
        String key = vendor + "_" + commandType;
        for (int i = 0; i < entryPointFlag.length; i++) {
            if (entryPointFlag[i][0].equalsIgnoreCase(key)) {
                return entryPointFlag[i][1];
            }
        }
        return null;
    }

    public static String getMapFlag(String vendor, String commandType) {
        String key = vendor + "_" + commandType;
        for (int i = 0; i < mapFlag.length; i++) {
            if (mapFlag[i][0].equalsIgnoreCase(key)) {
                return mapFlag[i][1];
            }
        }
        return null;
    }

    public static String getPdbFlag(String vendor, String commandType) {
        String key = vendor + "_" + commandType;
        for (int i = 0; i < pdbFlag.length; i++) {
            if (pdbFlag[i][0].equalsIgnoreCase(key)) {
                return pdbFlag[i][1];
            }
        }
        return null;
    }

    //
    // Well-known source file suffix and output file suffix relationship
    // sourceExtension(Multiple),
    // headExtension(Multiple) and outputSuffix(Single)
    //

    //
    // Default command string such as 'cl' in MSFT while 'gcc' in GCC
    //
    public final static String[][] defaultCommand = { { "GCC", "gcc" },
            { "MSFT_CC", "cl" }, { "MSFT_LIB", "lib" },
            { "MSFT_LINK", "link" }, };

    public static String getDefaultCommand(String toolchain) {
        for (int i = 0; i < defaultCommand.length; i++) {
            if (defaultCommand[i][0].equalsIgnoreCase(toolchain)) {
                return defaultCommand[i][1];
            }
        }
        return null;
    }

}