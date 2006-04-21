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
package net.sf.antcontrib.cpptasks.os390;
import java.util.Vector;
/**
 * A add-in class for IBM (r) OS/390 compilers and linkers
 * 
 * @author Hiram Chirino (cojonudo14@hotmail.com)
 */
public class OS390Processor {
    public static void addWarningSwitch(Vector args, int level) {
        switch (level) {
        /*
         * case 0: args.addElement("/W0"); break;
         * 
         * case 1: args.addElement("/W1"); break;
         * 
         * case 2: break;
         * 
         * case 3: args.addElement("/W3"); break;
         * 
         * case 4: args.addElement("/W4"); break;
         */
        }
    }
    public static String getCommandFileSwitch(String cmdFile) {
        StringBuffer buf = new StringBuffer("@");
        if (cmdFile.indexOf(' ') >= 0) {
            buf.append('\"');
            buf.append(cmdFile);
            buf.append('\"');
        } else {
            buf.append(cmdFile);
        }
        return buf.toString();
    }
    public static String getIncludeDirSwitch(String includeDir) {
        return "-I" + includeDir;
    }
    public static String[] getOutputFileSwitch(String outPath) {
        StringBuffer buf = new StringBuffer("-o ");
        if (outPath.indexOf(' ') >= 0) {
            buf.append('\"');
            buf.append(outPath);
            buf.append('\"');
        } else {
            buf.append(outPath);
        }
        String[] retval = new String[]{buf.toString()};
        return retval;
    }
    public static boolean isCaseSensitive() {
        return true;
    }
    private OS390Processor() {
    }
}
