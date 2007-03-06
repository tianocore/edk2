/** @file
This file is to wrap MakeDeps.exe tool as ANT task, which is used to generate
dependency files for source code.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.framework.tasks;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.LineNumberReader;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.Stack;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;
import org.tianocore.common.cache.FileTimeStamp;
import org.tianocore.common.logger.EdkLog;

/**
 Class MakeDeps is used to wrap MakeDeps.exe as an ANT task.
 **/
public class MakeDeps extends Task {

    //
    // private members, use set/get to access them
    //
    private String      targetFile = "";
    private String      depsFilePath = "";
    private IncludePath includePathList = new IncludePath();
    private Input       inputFileList = new Input();
    //
    // cache the including files to speed up dependency check
    // 
    private static HashMap<String, Set<String>> includesCache = new HashMap<String, Set<String>>();
    //
    // regular expression for "#include ..." directive
    // 
    private static final Pattern incPattern = Pattern.compile("[\n\r \t#]*include[ \t]+[\"<]*([^\n\r\"<>]+)[>\" \t]*");

    public MakeDeps() {

    }

    /**
     The Standard execute method for ANT task. It will check if it's necessary
     to generate the dependency list file. If no file is found or the dependency
     is changed, it will compose the command line and call MakeDeps.exe to
     generate the dependency list file.

     @throws    BuildException
     **/
    public void execute() throws BuildException {
        //
        // if target file is specified and it hasn't been generated, don't generate
        // dep file
        // 
        if (targetFile.length() != 0 && (new File(targetFile)).exists() == false) {
            return;
        }
        //
        // check if the dependency list file is uptodate or not
        //
        if (isUptodate()) {
            return;
        }

        //
        // if no include path is specified, try locally
        // 
        if (includePathList.isEmpty()) {
            includePathList.insPath(".");
        }

        Set<String> depFiles = getDependencies(inputFileList.toArray());

        File depsFile = new File(depsFilePath);
        FileWriter fileWriter = null;
        BufferedWriter bufWriter = null;

        try {
            fileWriter = new FileWriter(depsFile);
            bufWriter = new BufferedWriter(fileWriter);


            for (Iterator it = depFiles.iterator(); it.hasNext();) {
                String depFile = (String)it.next();
                bufWriter.write(depFile, 0, depFile.length());
                bufWriter.write("\n", 0, 1);
            }
            //
            // put a "tab" at the end of file as file ending flag
            // 
            bufWriter.write("\t", 0, 1);
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        } finally {
            try {
                if (bufWriter != null) {
                    bufWriter.close();
                }
                if (fileWriter != null) {
                    fileWriter.close();
                }
            } catch (Exception e) {
                throw new BuildException(e.getMessage());
            }
        }

        //
        // update time stamp of dependency file
        // 
        FileTimeStamp.update(depsFilePath, depsFile.lastModified());
    }

    public void setTargetFile(String name) {
        targetFile = name;
    }

    public String getTargetFile() {
        return targetFile;
    }

    /**
     Set method for "DepsFile" attribute

     @param     name    The name of dependency list file
     **/
    public void setDepsFile(String name) {
        depsFilePath = name;
    }

    /**
     Get method for "DepsFile" attribute

     @returns   The name of dependency list file
     **/
    public String getDepsFile() {
        return depsFilePath;
    }

    /**
     Add method for "IncludePath" nested element

     @param     path    The IncludePath object from nested IncludePath type of element
     **/
    public void addConfiguredIncludepath(IncludePath path) {
        includePathList.insert(path);
    }

    /**
     Add method for "Input" nested element

     @param     input   The Input object from nested Input type of element
     **/
    public void addConfiguredInput(Input inputFile) {
        inputFileList.insert(inputFile);
    }

    /**
     Check if the dependency list file should be (re-)generated or not.

     @returns   true    The dependency list file is uptodate. No re-generation is needed.
     @returns   false   The dependency list file is outofdate. Re-generation is needed.
     **/
    private boolean isUptodate() {
        File df = new File(depsFilePath);
        if (!df.exists()) {
            EdkLog.log(this, EdkLog.EDK_VERBOSE, depsFilePath + " doesn't exist!");
            return false;
        }

        //
        // If the source file(s) is newer than dependency list file, we need to
        // re-generate the dependency list file
        //
        long depsFileTimeStamp = FileTimeStamp.get(depsFilePath);
        List<String> fileList = inputFileList.getNameList();
        for (int i = 0, length = fileList.size(); i < length; ++i) {
            String sf = fileList.get(i);
            if (FileTimeStamp.get(sf) > depsFileTimeStamp) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, sf + " has been changed since last build!");
                return false;
            }
        }

        //
        // If the source files haven't been changed since last time the dependency
        // list file was generated, we need to check each file in the file list to
        // see if any of them is changed or not. If anyone of them is newer than
        // the dependency list file, MakeDeps.exe is needed to run again.
        //
        LineNumberReader    lineReader = null;
        FileReader          fileReader = null;
        boolean             ret = false;
        try {
            fileReader = new FileReader(df);
            lineReader = new LineNumberReader(fileReader);

            String line = null;
            int lines = 0;
            while ((line = lineReader.readLine()) != null) {
                //
                // check file end flag "\t" to see if the .dep was generated correctly
                // 
                if (line.equals("\t")) {
                    ret = true;
                    continue;
                }
                line = line.trim();
                //
                // skip empty line
                // 
                if (line.length() == 0) {
                    continue;
                }
                ++lines;

                //
                // If a file cannot be found (moved or removed) or newer, regenerate the dep file
                // 
                File sourceFile = new File(line);
                if ((!sourceFile.exists()) || (FileTimeStamp.get(line) > depsFileTimeStamp)) {
                    EdkLog.log(this, EdkLog.EDK_VERBOSE, sourceFile.getPath() + " has been (re)moved or changed since last build!");
                    ret = false;
                    break;
                }
            }

            //
            // check if the .dep file is empty
            // 
            if (lines == 0) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, depsFilePath + " is empty!");
                ret = false;
            }
        } catch (IOException e) {
            throw new BuildException(e.getMessage());
        } finally {
            try {
                if (lineReader != null) {
                    lineReader.close();
                }
                if (fileReader != null) {
                    fileReader.close();
                }
            } catch (Exception e) {
                throw new BuildException(e.getMessage());
            }
        }

        return ret;
    }

    //
    // get dependent files list by parsing "#include" directive
    // 
    private synchronized Set<String> getDependencies(String[] sourceFiles) {
        Set<String> dependencies = new LinkedHashSet<String>();
        String[] searchPathList = includePathList.toArray();

        Stack<String> pendingFiles = new Stack<String>();
        for (int i = 0; i < sourceFiles.length; ++i) {
            File srcFile = new File(sourceFiles[i]);
            if (srcFile.exists()) {
                //
                // a file must depend itself
                // 
                dependencies.add(srcFile.getAbsolutePath());
                pendingFiles.push(sourceFiles[i]);
            }
        }

        while (!pendingFiles.empty()) {
            String src = pendingFiles.pop();
            File srcFile = new File(src);
			int fileLength = (int)srcFile.length();
            if (!srcFile.exists() || fileLength == 0) {
                continue;
            }
            //
            // try cache first
            // 
            Set<String> incFiles = includesCache.get(src);
            if (incFiles == null) {
                incFiles = new HashSet<String>();
                FileInputStream fileReader = null;
                BufferedInputStream bufReader = null;
                String fileContent = "";

                try {
                    fileReader = new FileInputStream(srcFile);
                    bufReader  = new BufferedInputStream(fileReader);
                    byte[] buf = new byte[fileLength];

                    bufReader.read(buf, 0, fileLength);
                    //
                    // check if the file is utf-16 encoded
                    // 
                    if (buf[0] == (byte)0xff || buf[0] == (byte)0xfe) {
                        fileContent = new String(buf, "UTF-16");
                        buf = fileContent.getBytes("UTF-8");
                    }
                    fileContent = new String(buf);
                } catch (IOException e) {
                    throw new BuildException(e.getMessage());
                } finally {
                    try {
                        if (bufReader != null) {
                            bufReader.close();
                        }
                        if (fileReader != null) {
                            fileReader.close();
                        }
                    } catch (Exception e) {
                        throw new BuildException(e.getMessage());
                    }
                }

                //
                // find out all "#include" lines
                // 
                Matcher matcher = incPattern.matcher(fileContent);
                while (matcher.find()) {
                    String incFilePath = fileContent.substring(matcher.start(1), matcher.end(1));
                    incFiles.add(incFilePath);
                }

                //
                // put the includes in cache to avoid re-parsing
                // 
                includesCache.put(src, incFiles);
            }

            //
            // try each include search path to see if the include file exists or not
            // 
            for (Iterator<String> it = incFiles.iterator(); it.hasNext();) {
                String depFilePath = it.next();

                for (int i = 0; i < searchPathList.length; ++i) {
                    File depFile = new File(searchPathList[i] + File.separator + depFilePath);
                    String filePath = depFile.getAbsolutePath();
                    //
                    // following check is a must. it can prevent dead loop if two
                    // files include each other
                    // 
                    if (depFile.exists() && !dependencies.contains(filePath)) {
                        dependencies.add(filePath);
                        pendingFiles.push(filePath);
                        break;
                    }
                }
            }
        }

        return dependencies;
    }
}

