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
import java.io.IOException;
import java.io.LineNumberReader;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;
import org.apache.tools.ant.types.Path;

import org.tianocore.common.logger.EdkLog;
import org.tianocore.common.cache.FileTimeStamp;

/**
 Class MakeDeps is used to wrap MakeDeps.exe as an ANT task.
 **/
public class MakeDeps extends Task {

    //
    // private members, use set/get to access them
    //
    private static final String toolName = "MakeDeps";
    private FileArg              depsFile = new FileArg();
    private ToolArg              subDir = new ToolArg();
    private ToolArg              quietMode = new ToolArg(" -", "q");
    private ToolArg              ignoreError = new ToolArg(" -", "ignorenotfound");
    private IncludePath          includePathList = new IncludePath();
    private Input                inputFileList = new Input();
    private ToolArg              target = new FileArg(" -target ", "dummy");

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

        ///
        /// compose full tool path
        ///
        if (toolPath == null || toolPath.length() == 0) {
            toolPath = toolName;
        } else {
            if (toolPath.endsWith("/") || toolPath.endsWith("\\")) {
                toolPath = toolPath + toolName;
            } else {
                toolPath = toolPath + File.separator + toolName;
            }
        }

        ///
        /// compose tool arguments
        ///
        String argument = "" + inputFileList + includePathList + subDir
                             + quietMode + ignoreError + target + depsFile;

        ///
        /// prepare to execute the tool
        ///
        Commandline cmd = new Commandline();
        cmd.setExecutable(toolPath);
        cmd.createArgument().setLine(argument);

        LogStreamHandler streamHandler = new LogStreamHandler(this, Project.MSG_INFO, Project.MSG_WARN);
        Execute runner = new Execute(streamHandler, null);

        runner.setAntRun(prj);
        runner.setCommandline(cmd.getCommandline());

        EdkLog.log(this, EdkLog.EDK_VERBOSE, Commandline.toString(cmd.getCommandline()));

        int result = 0;
        try {
            result = runner.execute();
        } catch (IOException e) {
            throw new BuildException(e.getMessage());
        }

        if (result != 0) {
            EdkLog.log(this, EdkLog.EDK_INFO, toolName + " failed!");
            throw new BuildException(toolName + ": failed to generate dependency file!");
        } else {
            EdkLog.log(this, EdkLog.EDK_VERBOSE, toolName + " succeeded!");
        }
    }

    /**
     Set method for "DepsFile" attribute

     @param     name    The name of dependency list file
     **/
    public void setDepsFile(String name) {
        depsFile.setArg(" -o ", name);
    }

    /**
     Get method for "DepsFile" attribute

     @returns   The name of dependency list file
     **/
    public String getDepsFile() {
        return depsFile.getValue();
    }

    /**
     Set method for "IgnoreError" attribute

     @param     ignore    flag to control error handling (true/false)
     **/
    public void setIgnoreError(boolean ignore) {
        if (!ignore) {
            ignoreError.setArg(" ", " ");
        }
    }

    /**
     Get method for "IgnoreError" attribute

     @returns   The value of current IgnoreError flag
     **/
    public boolean getIgnoreError() {
        return ignoreError.getValue().length() > 0;
    }

    /**
     Set method for "QuietMode" attribute

     @param     quiet   flag to control the output information (true/false)
     **/
    public void setQuietMode(boolean quiet) {
        if (!quiet) {
            quietMode.setArg(" ", " ");
        }
    }

    /**
     Get method for "QuietMode" attribute

     @returns   value of current QuietMode flag
     **/
    public boolean getQuietMode() {
        return quietMode.getValue().length() > 0;
    }

    /**
     Set method for "SubDir" attribute

     @param     dir     The name of sub-directory in which source files will be scanned
     **/
    public void setSubDir(String dir) {
        subDir.setArg(" -s ", dir);
    }

    /**
     Get method for "SubDir" attribute

     @returns   The name of sub-directory
     **/
    public String getSubDir() {
        return subDir.getValue();
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
        String dfName = depsFile.getValue();
        File df = new File(dfName);
        if (!df.exists()) {
            EdkLog.log(this, EdkLog.EDK_VERBOSE, dfName + " doesn't exist!");
            return false;
        }

        //
        // If the source file(s) is newer than dependency list file, we need to
        // re-generate the dependency list file
        //
        long depsFileTimeStamp = FileTimeStamp.get(dfName);
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
                EdkLog.log(this, EdkLog.EDK_VERBOSE, dfName + " is empty!");
                ret = false;
            }

            lineReader.close();
            fileReader.close();
        } catch (IOException e) {
            throw new BuildException(e.getMessage());
        }

        return ret;
    }
}

