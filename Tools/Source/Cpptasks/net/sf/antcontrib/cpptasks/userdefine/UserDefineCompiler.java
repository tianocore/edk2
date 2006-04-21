/*
 * 
 * Copyright 2002-2006 The Ant-Contrib project
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

import org.apache.tools.ant.Project;

import net.sf.antcontrib.cpptasks.CCTask;
import org.tianocore.build.toolchain.*;

public class UserDefineCompiler extends CommandLineUserDefine {

    public UserDefineCompiler(CCTask cctask, UserDefineDef userdefineDef) {
        String arch = null;
        String os = null;
        String vendor = null;
        String commandType = null;
        Project project = cctask.getProject();
        // get command string
        if (cctask.getArch() == null) {
            arch = project.getProperty("ARCH");
            if (arch == null) {
                arch = System.getProperty("os.arch");
            }
        } else {
            arch = cctask.getArch();
        }
        arch = arch.toUpperCase();
        if (cctask.getOs() == null) {
            os = project.getProperty("OS");
            if (os == null) {
                os = System.getProperty("os.name");
            }
        } else {
            os = cctask.getOs();
        }

        commandType = userdefineDef.getType();

        if (commandType != null) {
            if (ToolChainFactory.getValue(arch + "_" + commandType + "_VENDOR") != null
                    && ToolChainFactory.getValue(
                            arch + "_" + commandType + "_VENDOR").trim()
                            .length() > 0) {
                vendor = ToolChainFactory.getValue(arch + "_" + commandType
                        + "_VENDOR");
            } else if (ToolChainFactory.getValue(arch + "_VENDOR") != null) {
                vendor = ToolChainFactory.getValue(arch + "_VENDOR");
            }
        }

        // look if ARCH_VENDOR_OS_COMMANDTYPE is existed
        if (arch != null && vendor != null && os != null && commandType != null) {
            command = project.getProperty(arch + "_" + vendor + "_" + os + "_"
                    + commandType);
        }
        // look if ARCH_VENDOR_COMMANDTYPE is existed
        if (command == null) {
            if (arch != null && vendor != null && commandType != null) {
                command = project.getProperty(arch + "_" + vendor + "_"
                        + commandType);
            }
        }
        // look if ARCH_COMMANDTYPE is existed
        if (command == null) {
            if (arch != null && commandType != null) {
                command = project.getProperty(arch + "_" + commandType);
            }
        }
        // look if COMMANDTYPE is existed
        if (command == null) {
            if (commandType != null) {
                command = project.getProperty(commandType);
            }
        }
        // using the default value from VENDOR_OS_COMMANDTYPE or
        // VENDOR_COMMANDTYPE
        if (command == null) {
            if (vendor != null && os != null && commandType != null) {
                String str = vendor + "_" + os + "_" + commandType;
                command = UserDefineMapping.getDefaultCommand(str);
            }
        }
        // VENDOR_COMMANDTYPE
        if (command == null) {
            if (vendor != null && commandType != null) {
                String str = vendor + "_" + commandType;
                command = UserDefineMapping.getDefaultCommand(str);
            }
        }
        // just give the name whatever
        if (command == null) {
            command = "cl";
        }

        // initialize the includePathDelimiter
        if (userdefineDef.getIncludepathDelimiter() != null) {
            includePathDelimiter = userdefineDef.getIncludepathDelimiter();
        }
        // else find VENDOR
        else {
            if (vendor != null) {
                includePathDelimiter = UserDefineMapping
                        .getIncludePathDelimiter(vendor, commandType);
            }
        }
        if (includePathDelimiter == null) {
            includePathDelimiter = "-I";
        }
        /*
         * Set libSet.
         */
        if (userdefineDef.getLibSet() != null
                && userdefineDef.getLibSet().size() > 0) {
            String[] libList;
            if (vendor.equalsIgnoreCase("GCC")) {
                libSetList.add("-(");
                for (int i = 0; i < userdefineDef.getLibSet().size(); i++) {
                    libList = userdefineDef.getLibSet().get(i).getLibs();
                    for (int j = 0; j < libList.length; j++) {
                        libSetList.add(libList[j]);
                    }
                }
                libSetList.add("-)");
            } else {
                for (int i = 0; i < userdefineDef.getLibSet().size(); i++) {
                    libList = userdefineDef.getLibSet().get(i).getLibs();
                    for (int j = 0; j < libList.length; j++) {
                        libSetList.add(libList[j]);
                    }
                }
            }
        }
        /*
         * set includeFileFlag
         */
        if (userdefineDef.getIncludeFile() != null) {
            if (userdefineDef.getIncludeFileFlag() != null) {
                includeFileFlag = userdefineDef.getIncludeFileFlag();
            } else {
                includeFileFlag = UserDefineMapping.getCompellingIncFileFlag(
                        vendor, commandType);
            }
        }
        /*
         * set entryPointFlag
         */
        if (userdefineDef.getEntryPointvalue() != null) {
            if (userdefineDef.getEntryPointFlag() != null) {
                entryPointFlag = userdefineDef.getEntryPointFlag();
            } else {
                entryPointFlag = UserDefineMapping.getEntryPointFlag(vendor,
                        commandType);
            }
        }
        /*
         * set subSystemFlag
         */
        if (userdefineDef.getSubSystemvalue() != null) {
            if (userdefineDef.getSubSystemFlag() != null) {
                subSystemFlag = userdefineDef.getSubSystemFlag();
            } else {
                subSystemFlag = UserDefineMapping.getSubSystemFlag(vendor,
                        commandType);
            }
        }
        /*
         * set mapFlag
         */
        if (userdefineDef.getMapvalue() != null) {
            if (userdefineDef.getMapFlag() != null) {
                mapFlag = userdefineDef.getMapFlag();
            } else {
                mapFlag = UserDefineMapping.getMapFlag(vendor, commandType);
            }
        }
        /*
         * set pdbFlag
         */
        if (userdefineDef.getPdbvalue() != null) {
            if (userdefineDef.getPdbFlag() != null) {
                pdbFlag = userdefineDef.getPdbFlag();
            } else {
                pdbFlag = UserDefineMapping.getPdbFlag(vendor, commandType);
            }
        }
        /*
         * set outputFileFlag
         */
        if (userdefineDef.getOutputFile() != null) {
            if (userdefineDef.getOutPutFlag() != null) {
                outputFileFlag = userdefineDef.getOutPutFlag();
            } else {
                outputFileFlag = UserDefineMapping.getOutputFileFlag(vendor,
                        arch, commandType);
            }
        }

        /*
         * set fileList
         */
        if (userdefineDef.getFileList() != null) {
            fileList = userdefineDef.getFileList();
        }
    }
}
