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

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;

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
    
    private String peFile = "";

    private String timeFile = "";

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
            command = "SetStamp";
        } else {
            command = path + "/" + "SetStamp";
        }
        ///
        /// argument of SetStamp tool
        ///
        String argument = peFile + timeFile;
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

            System.out.println(Commandline.toString(commandLine
                    .getCommandline()));

            returnVal = runner.execute();
            if (EFI_SUCCESS == returnVal) {
                ///
                /// command execution success
                ///
                System.out.println("SetStamp execute succeeded!");
            } else {
                ///
                /// command execution fail
                ///
                System.out.println("SetStamp failed. (error="
                        + Integer.toHexString(returnVal) + ")");
                throw new BuildException("SetStamp failed. (error="
                        + Integer.toHexString(returnVal) + ")");
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
        this.peFile = " " + peFile;
    }

    /**
     get operation of class member "peFile"
     
     @return    peFile  name of PE file
     **/
    public String getPeFile() {
        return this.peFile;
    }

    /**
     set operation of class member "timeFile"
     
     @param     timeFile    name of time file
     **/
    public void setTimeFile(String timeFile) {
        this.timeFile = " " + timeFile;
    }

    /**
     get class member "timeFile"
     
     @returns   name of time file
     **/
    public String getTimeFile() {
        return this.timeFile;
    }

}
