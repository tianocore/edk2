/** @file
 GenCRC32SectionTask class.

 GenCRC32SectionTask is to call GenCRC32Section.exe to generate crc32 section.
 
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
  GenCRC32SectionTask
  
  GenCRC32SectionTask is to call GenCRC32Section.exe to generate crc32 section. 
  
**/
public class GenCRC32SectionTask extends Task implements EfiDefine {
    //
    // Tool name
    //
    private static String toolName = "GenCRC32Section";
    //
    // output file
    //
    private FileArg outputFile = new FileArg();
    //
    // inputFile list
    //
    private InputFile inputFileList = new InputFile();
    
    //
    // Project
    //
    static private Project project;
    
    /**
      execute
      
      GenCRC32SectionTask execute is to assemble tool command line & execute
      tool command line
      
      @throws BuildException
    **/
    public void execute() throws BuildException {
        
        project = this.getOwningTarget().getProject(); 
        ///
        /// absolute path of efi tools
        ///
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH"); 
        String command;
        if (path == null) {
            command = toolName;
        } else {
            command = path + File.separator + toolName ;
        }
        // 
        // assemble argument 
        //
        String argument =  "" + inputFileList.toStringWithSinglepPrefix(" -i ") + outputFile; 
        // 
        // return value of fwimage execution 
        //
        int revl = -1; 
        
        try {
            Commandline cmdline = new Commandline();
            cmdline.setExecutable(command);
            cmdline.createArgument().setLine(argument);
            
            LogStreamHandler streamHandler = new LogStreamHandler(this, Project.MSG_INFO, Project.MSG_WARN);
            Execute runner = new Execute(streamHandler, null);
            
            runner.setAntRun(project);
            runner.setCommandline(cmdline.getCommandline());

            EdkLog.log(this, EdkLog.EDK_VERBOSE, Commandline.toString(cmdline.getCommandline()));
            EdkLog.log(this, inputFileList.toFileList() + " => " + outputFile.toFileList());

            revl = runner.execute();
            if (EFI_SUCCESS == revl){
                //
                //  command execution success 
                //
                EdkLog.log(this, toolName + " succeeded!");
            } else {
                // 
                // command execution fail
                //
                EdkLog.log(this, "ERROR = " + Integer.toHexString(revl));
                // LAH Added This Line
                throw new BuildException(toolName + " failed!");
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }        
    }

    /**
      addInputFile
     
      This function is to add a inputFile element into list
      @param inputFile : inputFile element
    **/
    public void addConfiguredInputfile(InputFile inputFile) {
        inputFileList.insert(inputFile);
    }
    
    /**
      get class member "outputFile"
      @return name of output file
     **/
    public String getOutputFile() {
        return this.outputFile.getValue();
    }
    /**
      set class member "outputFile"
      @param outputFile : outputFile parameter 
     **/
    public void setOutputFile(String outputFile) {
        this.outputFile.setArg(" -o ", outputFile);
    }
}
