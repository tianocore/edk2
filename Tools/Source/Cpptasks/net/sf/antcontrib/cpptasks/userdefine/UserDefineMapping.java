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

/**
 * Relationship between {family, command type} and flags
 * 
 */
public class UserDefineMapping {

    /**
     * Mapping info: include path delimiter <--> family (vendor) + command type
     */
    public static final String[][] includePathFlag = { { "MSFT_CC", "/I" },
                    { "GCC_CC", "-I" }, { "INTEL_CC", "/I" },
                    { "WINDDK_CC", "/I" }, { "MSFT_ASM", "/I" },
                    { "GCC_ASM", "-I" }, { "WINDDK_CC", "/I" },
                    { "MSFT_PP", "/I" }, { "GCC_PP", "-I" },
                    { "WINDDK_PP", "/I" } };

    /**
     * Mapping info: output file flag <--> family (vendor) + command type
     */
    public static final String[][] outputFileFlag = { { "MSFT_CC", "/Fo" },
                    { "GCC_CC", "-o" }, { "INTEL_CC", "/Fo" },
                    { "WINDDK_CC", "/Fo" }, { "MSFT_SLINK", "/OUT:" },
                    { "GCC_SLINK", "-cr" }, { "INTEL_SLINK", "/OUT:" },
                    { "WINDDK_SLINK", "/OUT:" }, { "MSFT_DLINK", "/OUT:" },
                    { "GCC_DLINK", "-o" }, { "INTEL_DLINK", "/OUT:" },
                    { "WINDDK_DLINK", "/OUT:" }, { "MSFT_ASM", "/Fo" },
                    { "GCC_ASM", "-o" }, { "WINDDK_ASM", "/Fo" },
                    { "WINDDK_IPF_ASM", "-o" } };

    /**
     * Get include delimiter with vendow and command type.
     * 
     * @param vendor
     *            Vendor
     * @param commandType
     *            Command Type
     * @return include path delimiter
     */
    public static String getIncludePathDelimiter(String vendor,
                    String commandType) {
        String key = vendor + "_" + commandType;
        for (int i = 0; i < includePathFlag.length; i++) {
            if (includePathFlag[i][0].equalsIgnoreCase(key)) {
                return includePathFlag[i][1];
            }
        }
        return "/I";
    }

    /**
     * Get Output Flag with vendor and command type.
     * 
     * @param vendor
     *            Vendor
     * @param commandType
     *            Command Type
     * @return Output File Flag
     */
    public static String getOutputFileFlag(String vendor, String commandType) {
        String key = vendor + "_" + commandType;
        for (int i = 0; i < outputFileFlag.length; i++) {
            if (outputFileFlag[i][0].equalsIgnoreCase(key)) {
                return outputFileFlag[i][1];
            }
        }
        return "/Fo";
    }

}