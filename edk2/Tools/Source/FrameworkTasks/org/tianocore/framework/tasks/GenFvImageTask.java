/** @file
 GenFvImageTask class.

 GenFvImageTask is to call GenFvImage.exe to generate FvImage.
 
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

import java.io.File;

import org.tianocore.common.logger.EdkLog;

/**
  GenFvImageTask
  
  GenFvImageTask is to call GenFvImage.exe to generate the FvImage.
  
**/
public class GenFvImageTask extends Task implements EfiDefine{
    //
    // tool name
    //
    static final private String toolName = "GenFvImage";
    //
    // The name of input inf file
    //
    private FileArg infFile = new FileArg();
    //
    // Output directory
    //
    private String outputDir = ".";

    /**
      execute
      
      GenFvImageTask execute is to assemble tool command line & execute tool
      command line.
    **/
    public void execute() throws BuildException  {
        Project project = this.getOwningTarget().getProject();
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH");

        String command;
        if (path == null) {
            command = toolName;
        } else {
            command = path + File.separator + toolName;
        }

        String argument = "" + infFile;
        //
        // lauch the program
        //
        int exitCode = 0;
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
            // log command line string.
            //
            EdkLog.log(this, EdkLog.EDK_VERBOSE, Commandline.toString(cmdline.getCommandline()));
            EdkLog.log(this, infFile.toFileList());

            exitCode = runner.execute();
            if (exitCode != 0) {
                EdkLog.log(this, "ERROR = " + Integer.toHexString(exitCode));
            } else {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, "GenFvImage succeeded!");
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        } finally {
            if (exitCode != 0) {
                throw new BuildException("GenFvImage: failed to generate FV file!");
            }
        }
    }
    /**
      getInfFile
      
      This function is to get class member of infFile
      @return String    name of infFile
    **/
    public String getInfFile() {
        return infFile.getValue();
    }
    
    /**
      setInfFile
      
      This function is to set class member of infFile.
      
      @param infFile  name of infFile
    **/
    public void setInfFile(String infFile) {
        this.infFile.setArg(" -I ", infFile);
    }
    
    /**
      getOutputDir
      
      This function is to get output directory.
      
      @return                Path of output directory.
    **/
    public String getOutputDir() {
        return outputDir;
    }

    /**
      setOutputDir
      
      This function is to set output directory.
      
      @param outputDir        The output direcotry.
    **/
    public void setOutputDir(String outputDir) {
        this.outputDir = outputDir;
    }
}
