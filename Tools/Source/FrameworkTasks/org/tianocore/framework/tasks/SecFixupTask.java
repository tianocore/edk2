/** @file
 SecFixupTask class.

 SecFixupTask is used to call SecFixup.exe to fix up sec image.


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
 * SecFixupTask class.
 *
 * SecFixupTask is used to call SecFixup.exe to fix up sec image.
 */
public class SecFixupTask extends Task implements EfiDefine {
    //
    // tool name
    //
    private String toolName = "SecFixup";

    //
    // input file
    //
    private FileArg secExeFile = new FileArg();

    //
    // output file
    //
    private FileArg resetVectorDataFile = new FileArg();

    //
    // output file
    //
    private FileArg outputFile = new FileArg();

    //
    // output directory, this variable is added by jave wrap
    //
    private String outputDir = ".";

    /**
      execute
     
      SecFixupTask execute function is to assemble tool command line & execute
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
        argument = "" + secExeFile + resetVectorDataFile + outputFile;

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
            EdkLog.log(this, EdkLog.EDK_INFO, secExeFile.toFileList() 
                + resetVectorDataFile.toFileList() + " => " + outputFile.toFileList());

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
                EdkLog.log(this, EdkLog.EDK_INFO, "ERROR = "+ Integer.toHexString(revl));
                throw new BuildException(toolName + " failed!");
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }
    }

    /**
      getSecExeFile
     
      This function is to get class member "secExeFile".
     
      @return string of sectExe file name.
     **/
    public String getSecExeFile() {
        return this.secExeFile.getValue();
    }

    /**
      setSecExeFile
     
      This function is to set class member "secExeFile".
     
      @param secExeFile
                 string of secExe file name.
     **/
    public void setSecExeFile(String secExeFile) {
        this.secExeFile.setArg(" ", secExeFile);
    }

    /**
      getResetVectorDataFile
     
      This function is to get class member "resetVectorDataFile"
     
      @return resetVectorDataFile string of resetVectorData file name.
     **/
    public String getResetVectorDataFile() {
        return this.resetVectorDataFile.getValue();
    }

    /**
      setResetVectorDataFile
     
      This function is to set class member "resetVectorDataFile"
     
      @param resetVectorDataFile
                 string of resetVectorData file name.
     **/
    public void setResetVectorDataFile(String resetVectorDataFile) {
        this.resetVectorDataFile.setArg(" ", resetVectorDataFile);
    }

    /**
      getOutputFile
     
      This function is to get class member "outputFile"
     
      @return outputFile string of output file name.
     **/
    public String getOutputFile() {
        return this.outputFile.getValue();
    }

    /**
      setOutputFile
     
      This function is to set class member "outputFile"
     
      @param outputFile
                 string of output file name.
     **/
    public void setOutputFile(String outputFile) {
        this.outputFile.setArg(" ", outputFile);
    }

    /**
      getOutputDir
     
      This function is to get class member "outputDir"
     
      @return outputDir name of output directory
     **/
    public String getOutputDir() {
        return outputDir;
    }

    /**
      setOutputDir
     
      This function is to set class member "outputDir"
     
      @param outputDir
                 name of output directory
     **/
    public void setOutputDir(String outputDir) {
        this.outputDir = outputDir;
    }
}
