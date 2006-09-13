/** @file
 GenDepexTask class.

 GenDepexTask is to call GenDepex.exe to generate depex section.
 
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
  GenDepexTask
  
  GenDepexTask is to call GenDepex.exe to generate depex section.

**/
public class GenDepexTask extends Task implements EfiDefine {
    private static String toolName = "GenDepex";
    //
    // output binary dependency files name
    //
    private FileArg outputFile = new FileArg();
    //
    // input pre-processed dependency text files name
    //
    private FileArg inputFile = new FileArg();
    //
    // padding integer value
    //
    private ToolArg padding = new FileArg();
    /**
      execute
      
      GenDepexTask execute is to assemble tool command line & execute tool
      command line.
     */
    public void execute() throws BuildException {

        Project project = this.getOwningTarget().getProject();
        //
        // absolute path of edk tools
        //
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH");
        String command;
        if (path == null) {
            command = toolName;
        } else {
            command = path + File.separator + toolName;
        }
        //
        // argument of GenDepex tool
        //
        String argument = "" + inputFile + outputFile + padding;
        //
        // reture value of GenDepex execution
        //
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

            EdkLog.log(this, EdkLog.EDK_VERBOSE, Commandline.toString(commandLine.getCommandline()));
            EdkLog.log(this, inputFile.toFileList() + " => " + outputFile.toFileList());

            returnVal = runner.execute();
            if (EFI_SUCCESS == returnVal) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, "GenDepex succeeded!");
            } else {
                //
                // command execution fail
                //
                EdkLog.log(this, "ERROR = " + Integer.toHexString(returnVal));
                throw new BuildException("GenDepex failed!");
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }
    }

    /**
      setOutputFile
      
      This function is to set class member "outputFile"
      @param outputFileName        name of output file
    **/
    public void setOutputFile(String outputFileName) {
        this.outputFile.setArg(" -O ", outputFileName);
    }

    /**
      getOutputFile
      
      This function is to get class member "outputFile".
     
      @return name of ouput file
    **/
    public String getOutputFile() {
        return this.outputFile.getValue();
    }

    /**
      setInputFile
      
      This function is to set class member "inputFile".
      @param inputFileName          name of inputFile
    **/
    public void setInputFile(String inputFileName) {
        this.inputFile.setArg(" -I ", inputFileName);
    }

    /**
      getInputFile
      
      This function is to get class member "inputFile"
      @return                       name of input file
    **/
    public String getInputFile() {
        return this.inputFile.getValue();
    }

    /**
      setPadding
      
      This function is to set class member "padding"
      @param paddingNum             padding value
    **/
    public void setPadding(String paddingNum) {
        this.padding.setArg(" -P ", paddingNum);
    }

    /**
      getPadding
      
      This function is to get class member "padding"
      @return                       value of padding
    **/
    public String getPadding() {
        return this.padding.getValue();
    }
}
