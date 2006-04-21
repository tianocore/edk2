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
package net.sf.antcontrib.cpptasks.borland;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.Reader;
import java.util.Vector;
import java.io.FileWriter;

import net.sf.antcontrib.cpptasks.CUtil;
import net.sf.antcontrib.cpptasks.types.LibraryTypeEnum;
/**
 * A add-in class for Borland(r) processor adapters
 * 
 *  
 */
public final class BorlandProcessor {
    public static void addWarningSwitch(Vector args, int level) {
        switch (level) {
            case 0 :
                args.addElement("-w-");
                break;
            case 5 :
                args.addElement("-w!");
                break;
            default :
                args.addElement("-w");
                break;
        }
    }
    public static String getCommandFileSwitch(String cmdFile) {
        StringBuffer buf = new StringBuffer("@");
        quoteFile(buf, cmdFile);
        return buf.toString();
    }
    public static void getDefineSwitch(StringBuffer buffer, String define,
            String value) {
        buffer.append("-D");
        buffer.append(define);
        if (value != null && value.length() > 0) {
            buffer.append('=');
            buffer.append(value);
        }
    }
    /**
     * This method extracts path information from the appropriate .cfg file in
     * the install directory.
     * 
     * @param toolName
     *            Tool name, for example, "bcc32", "brc32", "ilink32"
     * @param switchChar
     *            Command line switch character, for example "L" for libraries
     * @param defaultRelativePaths
     *            default paths relative to executable directory
     * @return path
     */
    public static File[] getEnvironmentPath(String toolName, char switchChar,
            String[] defaultRelativePath) {
        if (toolName == null) {
            throw new NullPointerException("toolName");
        }
        if (defaultRelativePath == null) {
            throw new NullPointerException("defaultRelativePath");
        }
        String[] path = defaultRelativePath;
        File exeDir = CUtil.getExecutableLocation(toolName + ".exe");
        if (exeDir != null) {
            File cfgFile = new File(exeDir, toolName + ".cfg");
            if (cfgFile.exists()) {
                try {
                    Reader reader = new BufferedReader(new FileReader(cfgFile));
                    BorlandCfgParser cfgParser = new BorlandCfgParser(
                            switchChar);
                    path = cfgParser.parsePath(reader);
                    reader.close();
                } catch (IOException ex) {
                    //
                    //  could be logged
                    //
                }
            }
        } else {
            //
            //  if can't find the executable,
            //     assume current directory to resolve relative paths
            //
            exeDir = new File(System.getProperty("user.dir"));
        }
        int nonExistant = 0;
        File[] resourcePath = new File[path.length];
        for (int i = 0; i < path.length; i++) {
            resourcePath[i] = new File(path[i]);
            if (!resourcePath[i].isAbsolute()) {
                resourcePath[i] = new File(exeDir, path[i]);
            }
            //
            //  if any of the entries do not exist or are
            //     not directories, null them out
            if (!(resourcePath[i].exists() && resourcePath[i].isDirectory())) {
                resourcePath[i] = null;
                nonExistant++;
            }
        }
        //
        //  if there were some non-existant or non-directory
        //    entries in the configuration file then
        //    create a shorter array
        if (nonExistant > 0) {
            File[] culled = new File[resourcePath.length - nonExistant];
            int index = 0;
            for (int i = 0; i < resourcePath.length; i++) {
                if (resourcePath[i] != null) {
                    culled[index++] = resourcePath[i];
                }
            }
            resourcePath = culled;
        }
        return resourcePath;
    }
    public static String getIncludeDirSwitch(String includeOption,
            String includeDir) {
        StringBuffer buf = new StringBuffer(includeOption);
        quoteFile(buf, includeDir);
        return buf.toString();
    }
    public static String[] getLibraryPatterns(String[] libnames, LibraryTypeEnum libType) {
        StringBuffer buf = new StringBuffer();
        String[] patterns = new String[libnames.length];
        for (int i = 0; i < libnames.length; i++) {
            buf.setLength(0);
            buf.append(libnames[i]);
            buf.append(".lib");
            patterns[i] = buf.toString();
        }
        return patterns;
    }
    public static String[] getOutputFileSwitch(String outFile) {
        return new String[0];
    }
    public static void getUndefineSwitch(StringBuffer buffer, String define) {
        buffer.append("-U");
        buffer.append(define);
    }
    public static boolean isCaseSensitive() {
        return false;
    }
    private static void quoteFile(StringBuffer buf, String outPath) {
        if (outPath.indexOf(' ') >= 0) {
            buf.append('\"');
            buf.append(outPath);
            buf.append('\"');
        } else {
            buf.append(outPath);
        }
    }
    
    /**
     * Prepares argument list to execute the linker using a response file.
     * 
     * @param outputFile
     *            linker output file
     * @param args
     *            output of prepareArguments
     * @return arguments for runTask
     */
    public static String[] prepareResponseFile(File outputFile, 
    		String[] args,
			String continuation)
            throws IOException {
        String baseName = outputFile.getName();
        File commandFile = new File(outputFile.getParent(), baseName + ".lnk");
        FileWriter writer = new FileWriter(commandFile);
        for (int i = 1; i < args.length - 1; i++) {
            writer.write(args[i]);
            //
            //  if either the current argument ends with
            //     or next argument starts with a comma then
            //      don't split the line
            if (args[i].endsWith(",") || args[i + 1].startsWith(",")) {
                writer.write(' ');
            } else {
                //
                //  split the line to make it more readable
                //
                writer.write(continuation);
            }
        }
        //
        //  write the last argument
        //
        if (args.length > 1) {
            writer.write(args[args.length - 1]);
        }
        writer.close();
        String[] execArgs = new String[2];
        execArgs[0] = args[0];
        execArgs[1] = getCommandFileSwitch(commandFile.toString());
        return execArgs;
    }
    
    private BorlandProcessor() {
    }
}
