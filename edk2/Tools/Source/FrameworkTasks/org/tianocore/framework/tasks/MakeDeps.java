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

import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.LineNumberReader;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import java.util.StringTokenizer;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;
import org.apache.tools.ant.types.Path;
import org.tianocore.logger.EdkLog;

/**
 Class MakeDeps is used to wrap MakeDeps.exe as an ANT task.
 **/
public class MakeDeps extends Task {

    //
    // private members, use set/get to access them
    //
    private static final String cmdName = "MakeDeps";
    private static final String target  = "dummy";
    private String              includePath = null;
    private String              depsFile = null;
    private String              subDir = null;
    private boolean             quietMode = true;
    private boolean             ignoreError = true;
    private String              extraDeps = "";
    private List<IncludePath>   includePathList = new ArrayList<IncludePath>();
    private List<Input>         inputFileList = new ArrayList<Input>();

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
        ///
        /// check if the dependency list file is uptodate or not
        ///
        if (isUptodate()) {
            return;
        }

        Project prj  = this.getOwningTarget().getProject();
        String  toolPath = prj.getProperty("env.FRAMEWORK_TOOLS_PATH");
        FrameworkLogger logger = new FrameworkLogger(prj, "makedeps");
        EdkLog.setLogLevel(prj.getProperty("env.LOGLEVEL"));
        EdkLog.setLogger(logger);

        ///
        /// compose full tool path
        ///
        if (toolPath == null || toolPath.length() == 0) {
            toolPath = "./" + cmdName;
        } else {
            if (toolPath.endsWith("/") || toolPath.endsWith("\\")) {
                toolPath = toolPath + cmdName;
            } else {
                toolPath = toolPath + "/" + cmdName;
            }
        }

        ///
        /// compose tool arguments
        ///
        StringBuffer args = new StringBuffer(4096);
        if (ignoreError) {
            args.append(" -ignorenotfound");
        }
        if (quietMode) {
            args.append(" -q");
        }
        if (subDir != null && subDir.length() > 0) {
            args.append(" -s ");
            args.append(subDir);
        }

        ///
        /// if there's no source files, we can do nothing about dependency
        /// 
        if (inputFileList.size() == 0) {
            throw new BuildException("No source files specified to scan");
        }

        ///
        /// compose source file arguments
        ///
        Iterator iterator = inputFileList.iterator();
        while (iterator.hasNext()) {
            Input inputFile = (Input)iterator.next();
            String inputFileString = cleanupPathName(inputFile.getFile());
            args.append(" -f ");
            args.append(inputFileString);
        }

        ///
        /// compose search pathes argument
        ///
        StringBuffer includePathArg = new StringBuffer(4096);
        if (includePath != null && includePath.length() > 0) {
            StringTokenizer pathTokens = new StringTokenizer(includePath, ";");
            while (pathTokens.hasMoreTokens()) {
                String tmpPath = pathTokens.nextToken().trim();
                if (tmpPath.length() == 0) {
                    continue;
                }

                includePathArg.append(" -i ");
                includePathArg.append(cleanupPathName(tmpPath));
            }
        }
        iterator = includePathList.iterator();
        while (iterator.hasNext()) {
            IncludePath path = (IncludePath)iterator.next();
            includePathArg.append(cleanupPathName(path.getPath()));
        }
        args.append(includePathArg);

        ///
        /// We don't need a real target. So just a "dummy" is given
        ///
        args.append(" -target dummy");
        args.append(" -o ");
        args.append(cleanupPathName(depsFile));

        ///
        /// prepare to execute the tool
        ///
        Commandline cmd = new Commandline();
        cmd.setExecutable(toolPath);
        cmd.createArgument().setLine(args.toString());

        LogStreamHandler streamHandler = new LogStreamHandler(this, Project.MSG_INFO, Project.MSG_WARN);
        Execute runner = new Execute(streamHandler, null);

        runner.setAntRun(prj);
        runner.setCommandline(cmd.getCommandline());

        EdkLog.log(EdkLog.EDK_VERBOSE, Commandline.toString(cmd.getCommandline()));

        int result = 0;
        try {
            result = runner.execute();
        } catch (IOException e) {
            throw new BuildException(e.getMessage());
        }

        if (result != 0) {
            EdkLog.log(EdkLog.EDK_INFO, "MakeDeps failed!");
            return;
        }

        // change the old DEP file format (makefile compatible) to just file list
        if (!cleanup()) {
            throw new BuildException(depsFile + " was not generated");
        }
    }

    ///
    /// Remove any duplicated path separator or inconsistent path separator
    ///
    private String cleanupPathName(String path) {
        String separator = "\\" + File.separator;
        String duplicateSeparator = separator + "{2}";
        path = Path.translateFile(path);
        path = path.replaceAll(duplicateSeparator, separator);
        return path;
    }

    /**
     Set method for "DepsFile" attribute

     @param     name    The name of dependency list file
     **/
    public void setDepsFile(String name) {
        depsFile = cleanupPathName(name);
    }

    /**
     Get method for "DepsFile" attribute

     @returns   The name of dependency list file
     **/
    public String getDepsFile() {
        return depsFile;
    }

    /**
     Set method for "IgnoreError" attribute

     @param     ignore    flag to control error handling (true/false)
     **/
    public void setIgnoreError(boolean ignore) {
        ignoreError = ignore;
    }

    /**
     Get method for "IgnoreError" attribute

     @returns   The value of current IgnoreError flag
     **/
    public boolean getIgnoreError() {
        return ignoreError;
    }

    /**
     Set method for "QuietMode" attribute

     @param     quiet   flag to control the output information (true/false)
     **/
    public void setQuietMode(boolean quiet) {
        quietMode = quiet;
    }

    /**
     Get method for "QuietMode" attribute

     @returns   value of current QuietMode flag
     **/
    public boolean getQuietMode() {
        return quietMode;
    }

    /**
     Set method for "SubDir" attribute

     @param     dir     The name of sub-directory in which source files will be scanned
     **/
    public void setSubDir(String dir) {
        subDir = dir;
    }

    /**
     Get method for "SubDir" attribute

     @returns   The name of sub-directory
     **/
    public String getSubDir() {
        return subDir;
    }

    /**
     Set method for "IncludePath" attribute

     @param     path    The name of include path
     **/
    public void setIncludePath(String path) {
        includePath = cleanupPathName(path);
    }

    /**
     Get method for "IncludePath" attribute

     @returns   The name of include path
     **/
    public String getIncludePath() {
        return includePath;
    }

    /**
     Set method for "ExtraDeps" attribute

     @param     deps    The name of dependency file specified separately
     **/
    public void setExtraDeps(String deps) {
        extraDeps = deps;
    }

    /**
     Get method for "ExtraDeps" attribute

     @returns   The name of dependency file specified separately
     **/
    public String getExtraDeps () {
        return extraDeps;
    }

    /**
     Add method for "IncludePath" nested element

     @param     path    The IncludePath object from nested IncludePath type of element
     **/
    public void addIncludepath(IncludePath path) {
        includePathList.add(path);
    }

    /**
     Add method for "Input" nested element

     @param     input   The Input object from nested Input type of element
     **/
    public void addInput(Input inputFile) {
        inputFileList.add(inputFile);
    }

    /**
     The original file generated by MakeDeps.exe is for makefile uses. The target
     part (before :) is not useful for ANT. This method will do the removal.

     @returns   true    if cleaned files is saved successfully
     @returns   false   if error occurs in file I/O system
     **/
    private boolean cleanup() {
        File df = new File(depsFile);

        if (!df.exists()) {
            return false;
        }

        LineNumberReader    lineReader = null;
        FileReader          fileReader = null;
        Set<String>         lineSet = new HashSet<String>(100); // used to remove duplicated lines
        try {
            fileReader = new FileReader(df);
            lineReader = new LineNumberReader(fileReader);

            ///
            /// clean-up each line in deps file
            //
            String line = null;
            while ((line = lineReader.readLine()) != null) {
                Pattern pattern = Pattern.compile(target + "[ ]*:[ ]*(.+)");
                Matcher matcher = pattern.matcher(line);

                while (matcher.find()) {
                    ///
                    /// keep the file name after ":"
                    ///
                    String filePath = line.substring(matcher.start(1), matcher.end(1));
                    filePath = cleanupPathName(filePath);
                    lineSet.add(filePath);
                }
            }
            lineReader.close();
            fileReader.close();

            ///
            /// we may have explicitly specified dependency files
            ///
            StringTokenizer fileTokens = new StringTokenizer(extraDeps, ";");
            while (fileTokens.hasMoreTokens()) {
                lineSet.add(cleanupPathName(fileTokens.nextToken()));
            }

            ///
            /// compose the final file content
            /// 
            StringBuffer cleanedLines = new StringBuffer(40960);
            Iterator<String> it = lineSet.iterator();
            while (it.hasNext()) {
                String filePath = it.next();
                cleanedLines.append(filePath);
                cleanedLines.append("\n");
            }
            ///
            /// overwrite old dep file with new content
            ///
            FileWriter fileWriter = null;
            fileWriter = new FileWriter(df);
            fileWriter.write(cleanedLines.toString());
            fileWriter.close();
        } catch (IOException e) {
            log (e.getMessage());
        }

        return true;
    }

    /**
     Check if the dependency list file should be (re-)generated or not.

     @returns   true    The dependency list file is uptodate. No re-generation is needed.
     @returns   false   The dependency list file is outofdate. Re-generation is needed.
     **/
    private boolean isUptodate() {
        File df = new File(depsFile);
        if (!df.exists()) {
            return false;
        }

        ///
        /// If the source file(s) is newer than dependency list file, we need to
        /// re-generate the dependency list file
        ///
        long depsFileTimeStamp = df.lastModified();
        Iterator iterator = inputFileList.iterator();
        while (iterator.hasNext()) {
            Input inputFile = (Input)iterator.next();
            File sf = new File(inputFile.getFile());
            if (sf.lastModified() > depsFileTimeStamp) {
                return false;
            }
        }

        ///
        /// If the source files haven't been changed since last time the dependency
        /// list file was generated, we need to check each file in the file list to
        /// see if any of them is changed or not. If anyone of them is newer than
        /// the dependency list file, MakeDeps.exe is needed to run again.
        ///
        LineNumberReader    lineReader = null;
        FileReader          fileReader = null;
        boolean             ret = true;
        try {
            fileReader = new FileReader(df);
            lineReader = new LineNumberReader(fileReader);

            String line = null;
            while ((line = lineReader.readLine()) != null) {
                File sourceFile = new File(line);
                if (sourceFile.lastModified() > depsFileTimeStamp) {
                    ret = false;
                    break;
                }
            }
            lineReader.close();
            fileReader.close();
        } catch (IOException e) {
            log (e.getMessage());
        }

        return ret;
    }
}

