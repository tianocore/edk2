/*
 * 
 * Copyright 2002-2004 The Ant-Contrib project
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
package net.sf.antcontrib.cpptasks.intel;
import java.util.Vector;

import net.sf.antcontrib.cpptasks.devstudio.DevStudioProcessor;
/**
 * A add-in class for Intel (r) compilers and linkers
 * 
 *  
 */
public class IntelProcessor {
    public static void addWarningSwitch(Vector args, int level) {
        DevStudioProcessor.addWarningSwitch(args, level);
    }
    public static String getCommandFileSwitch(String cmdFile) {
        return DevStudioProcessor.getCommandFileSwitch(cmdFile);
    }
    public static void getDefineSwitch(StringBuffer buffer, String define,
            String value) {
        DevStudioProcessor.getDefineSwitch(buffer, define, value);
    }
    public static String getIncludeDirSwitch(String includeDir) {
        return DevStudioProcessor.getIncludeDirSwitch(includeDir);
    }
    public static String[] getOutputFileSwitch(String outPath) {
        return DevStudioProcessor.getOutputFileSwitch(outPath);
    }
    public static void getUndefineSwitch(StringBuffer buffer, String define) {
        DevStudioProcessor.getUndefineSwitch(buffer, define);
    }
    public static boolean isCaseSensitive() {
        return false;
    }
    private IntelProcessor() {
    }
}
