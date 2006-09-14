/** @file
 ModifyInfTask class.

 ModifyInfTask is used to call Modify.exe to generate inf file.


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

import org.apache.tools.ant.Task;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;

import org.tianocore.common.logger.EdkLog;

/**
  ModifyInfTask class.

  ModifyInfTask is used to call Modify.exe to generate inf file.
**/
public class ModifyInfTask extends Task implements EfiDefine {
    //
    // tool name
    //
    private String toolName = "ModifyInf";

    //
    // input FV inf file
    //
    private FileArg inputFVInfFile = new FileArg();

    //
    // output FV inf file
    //
    private FileArg outputFVInfFile = new FileArg();

    //
    // pattern string
    //
    private ToolArg patternStr = new ToolArg();

	//
	//  Output dir
	//
	private String outputDir = ".";

    /**
      execute
     
      ModifyInfTask execute function is to assemble tool command line & execute
      tool command line
     
      @throws BuidException
     **/
    public void execute() throws BuildException {

        Project project = this.getOwningTarget().getProject();
        //
        // absolute path of efi tools
        //
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH");
        String command;
        String argument;
        if (path == null) {
            command = toolName;
        } else {
            command = path + File.separatorChar + toolName;
        }
        //
        // argument of tools
        //
        argument = "" + this.inputFVInfFile + this.outputFVInfFile + this.patternStr;
        //
        // return value of fwimage execution
        //
        int revl = -1;

        try {
            Commandline cmdline = new Commandline();
            cmdline.setExecutable(command);
            cmdline.createArgument().setLine(argument);

            LogStreamHandler streamHandler = new LogStreamHandler(this,
                    Project.MSG_INFO, Project.MSG_WARN);
            Execute runner = new Execute(streamHandler, null);

            runner.setAntRun(project);
            runner.setCommandline(cmdline.getCommandline());
            runner.setWorkingDirectory(new File(outputDir));

            //
            // Set debug log information.
            //
            EdkLog.log(this, EdkLog.EDK_VERBOSE, Commandline.toString(cmdline.getCommandline()));
            EdkLog.log(this, EdkLog.EDK_INFO, this.inputFVInfFile.toFileList()
                + " => " + this.inputFVInfFile.toFileList());

            revl = runner.execute();
            if (EFI_SUCCESS == revl) {
                //
                // command execution success
                //
                EdkLog.log(this, EdkLog.EDK_VERBOSE, toolName + " succeeded!");
            } else {
                //
                // command execution fail
                //
                EdkLog.log(this, EdkLog.EDK_INFO, "ERROR = " + Integer.toHexString(revl));
                throw new BuildException(toolName + " failed!");
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }
    }

    /**
      getinputFVInfFile
     
      This function is to get class member "inputFVInfFile".
     
      @return string of input inf file name.
     **/
    public String getinputFVInfFile() {
        return this.inputFVInfFile.getValue();
    }

    /**
      setinputFVInfFile
     
      This function is to set class member "inputFVInfFile".
     
      @param inputFile
                 string of input inf file name.
     **/
    public void setinputFVInfFile(String inputFVInfFileName) {
        this.inputFVInfFile.setArg(" ", inputFVInfFileName);
    }

    /**
      getoutputFVInfFile
     
      This function is to get class member "outputFVInfFile"
     
      @return outputFVInfFile string of output inf file name.
     **/
    public String getoutputFVInfFile() {
        return this.outputFVInfFile.getValue();
    }

    /**
      setoutputFVInfFile
     
      This function is to set class member "outputFVInfFile"
     
      @param outputFVInfFile
                 string of output  inf file name.
     **/
    public void setoutputFVInfFile(String outputFVInfFileName) {
        this.outputFVInfFile.setArg(" ", outputFVInfFileName);
    }

    /**
      getpatternStr
     
      This function is to get class member "patternStr"
     
      @return patternStr string of pattern.
     **/
    public String getpatternStr() {
        return this.patternStr.getValue();
    }

    /**
      setpatternStr
     
      This function is to set class member "patternStr"
     
      @param patternStr
                 string of patternStr.
     **/
    public void setpatternStr(String patternStr) {
        this.patternStr.setArg(" ", patternStr);
    }

	/**
      getoutputDir
     
      This function is to get class member "outputDir"
     
      @return outputDir string of output directory.
     **/
    public String getoutputDir() {
        return this.outputDir;
    }

    /**
      setoutputDir
     
      This function is to set class member "outputDir"
     
      @param patternStr
                 string of output directory.
     **/
    public void setoutputDir(String outputDir) {
        this.outputDir = outputDir;
    }
}
