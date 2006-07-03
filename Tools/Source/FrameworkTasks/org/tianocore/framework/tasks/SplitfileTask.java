/** @file
 SplitfileTask class.

 SplitfileTask is used to call splitfile.exe to split input file to 2 output 
 file.
 
 
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
import org.tianocore.logger.EdkLog;

/**
  SplitfileTask class.

  SplitfileTask is used to call splitfile.exe to split input file to 2 output 
  file.
**/
public class SplitfileTask extends Task implements EfiDefine {
    ///
    /// input file
    ///
    private String inputFile = "";

    ///
    /// offset value
    ///
    private String offset = "";

  
    /**
     * execute
     * 
     * SplitfleTask execute function is to assemble tool command line & execute
     * tool command line
     * 
     * @throws BuidException
     */
    public void execute() throws BuildException {

        Project project = this.getOwningTarget().getProject();
        
        //
        // set Logger
        //
        FrameworkLogger logger = new FrameworkLogger(project, "splitfile");
        EdkLog.setLogLevel(project.getProperty("env.LOGLEVEL"));
        EdkLog.setLogger(logger);
        
        //
        // absolute path of efi tools
        //
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH");
        String command;
        String argument;
        if (path == null) {
            command = "SplitFile";
        } else {
            command = path + File.separatorChar + "SplitFile";
        }
        
        //
        // argument of tools
        //
        argument = inputFile + " " + offset;
        
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
            
            EdkLog.log(EdkLog.EDK_INFO, Commandline.toString(cmdline.getCommandline()));
            revl = runner.execute();
            if (EFI_SUCCESS == revl) {
                //
                // command execution success
                //
                EdkLog.log(EdkLog.EDK_INFO, "splitfile succeeded!");
            } else {
                //
                // command execution fail
                //
                EdkLog.log(EdkLog.EDK_ERROR, "splitfile failed. (error="
                        + Integer.toHexString(revl) + ")");
                throw new BuildException("splitfile failed. (error="
                        + Integer.toHexString(revl) + ")");

            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }
    }

    /**
     * getInputFile
     * 
     * This function is to get class member "inputFile".
     * 
     * @return string of input file name.
     */
    public String getInputFile() {
        return inputFile;
    }

    /**
     * setComponentType
     * 
     * This function is to set class member "inputFile".
     * 
     * @param inputFile
     *            string of input file name.
     */
    public void setInputFile(String inputFile) {
        this.inputFile = inputFile;
    }

    /**
      getOffset
      
      This function is to get class member "offset"
      
      @return offset value of string.
    **/
    public String getOffset() {
        return offset;
    }

    /**
      setOffset
      
      This function is to set class member "offset"
      
      @param offset
                 string of offset value.
    **/
    public void setOffset(String offset) {
        this.offset = offset;
    }
    
}
