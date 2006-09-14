/** @file
This file is to define an ANT task to wrap setstamp.exe tool

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

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;

import org.tianocore.common.logger.EdkLog;

/**
 Class SetStampTask is a wrap class for setstamp.exe.
 **/
public class SetStampTask extends Task implements EfiDefine {
    /**
     SetStamp Task Class
     class member
         -peFile  : file of PE
         -timeFile: Txt file of time
     **/ 

    private static String toolName = "SetStamp";

    private FileArg peFile = new FileArg();

    private FileArg timeFile = new FileArg();

    private String outputDir = ".";

    /**
     assemble tool command line & execute tool command line
     
     @throws BuildException
     **/
    public void execute() throws BuildException {

        Project project = this.getOwningTarget().getProject();
        ///
        /// absolute path of edk tools
        ///
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH");
        String command;
        if (path == null) {
            command = toolName;
        } else {
            command = path + File.separator + toolName;
        }
        ///
        /// argument of SetStamp tool
        ///
        String argument = "" + peFile + timeFile;
        ///
        /// reture value of SetStamp execution
        ///
        int returnVal = -1;

        try {
            Commandline commandLine = new Commandline();
            commandLine.setExecutable(command);
            commandLine.createArgument().setLine(argument);

            LogStreamHandler streamHandler = new LogStreamHandler(this,
                    Project.MSG_INFO, Project.MSG_WARN);

            Execute runner = new Execute(streamHandler, null);
            runner.setAntRun(project);
            runner.setCommandline(commandLine.getCommandline());
            runner.setWorkingDirectory(new File(outputDir));

            EdkLog.log(this, EdkLog.EDK_VERBOSE, Commandline.toString(commandLine.getCommandline()));
            EdkLog.log(this, peFile.toFileList() + " < " + timeFile.toFileList());

            returnVal = runner.execute();
            if (EFI_SUCCESS == returnVal) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, toolName + " succeeded!");
            } else {
                ///
                /// command execution fail
                ///
                EdkLog.log(this, "ERROR = " + Integer.toHexString(returnVal));
                throw new BuildException(toolName + " failed!");
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }
    }

    /**
     set operation of class member "peFile"
     
     @param     peFile  name of PE File
     **/
    public void setPeFile(String peFile) {
        this.peFile.setArg(" ", peFile);
    }

    /**
     get operation of class member "peFile"
     
     @return    peFile  name of PE file
     **/
    public String getPeFile() {
        return this.peFile.getValue();
    }

    /**
     set operation of class member "timeFile"
     
     @param     timeFile    name of time file
     **/
    public void setTimeFile(String timeFile) {
        this.timeFile.setArg(" ", timeFile);
    }

    /**
     get class member "timeFile"
     
     @returns   name of time file
     **/
    public String getTimeFile() {
        return this.timeFile.getValue();
    }

    /**
      getOutputDir
     
      This function is to get class member "outputDir"
     
      @return outputDir string of output directory.
     **/
    public String getOutputDir() {
        return outputDir;
    }

    /**
      setOutputDir
     
      This function is to set class member "outputDir"
     
      @param outputDir
                 string of output directory.
     **/
    public void setOutputDir(String outputDir) {
        this.outputDir = outputDir;
    }
}
