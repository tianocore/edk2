/** @file
 GenTeImageTask class.

 GenTeImageTask is used to call GenTEImage.exe to generate TE image .


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
 * GenTeImageTask class.
 *
 * GenTeImageTask is used to call GenAcpiTable.exe to generate ACPI Table image .
 */
public class GenTeImageTask extends Task implements EfiDefine {
    //
    // tool name
    //
    private String toolName = "GenTeImage";
    //
    // input file
    //
    private FileArg inputFile = new FileArg();

    //
    // output file
    //
    private FileArg outputFile = new FileArg();

    //
    // output directory, this variable is added by jave wrap
    //
    private String outputDir = "";

    //
    // Verbose flag
    //
    private ToolArg verbose = new ToolArg();

    //
    // Dump flag
    //
    private ToolArg dump = new ToolArg();

    /**
      execute
     
      GenTeImgaeTask execute function is to assemble tool command line & execute
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
            command = path + File.separator + toolName;
        }
        //
        // argument of tools
        //
        argument = "" + this.verbose + this.dump + this.outputFile + this.inputFile;
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
            EdkLog.log(this, EdkLog.EDK_INFO, this.inputFile.toFileList()
                + " => " + this.outputFile.toFileList());

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
      getInputFile
     
      This function is to get class member "inputFile".
     
      @return string of input file name.
     **/
    public String getInputFile() {
        return inputFile.getValue();
    }

    /**
      setComponentType
     
      This function is to set class member "inputFile".
     
      @param inputFile
                 string of input file name.
     **/
    public void setInputFile(String inputFile) {
        this.inputFile.setArg(" ", inputFile);
    }

    /**
      getOutputFile
     
      This function is to get class member "outputFile"
     
      @return outputFile string of output file name.
     **/
    public String getOutputFile() {
        return outputFile.getValue();
    }

    /**
      setOutputFile
     
      This function is to set class member "outputFile"
     
      @param outputFile
                 string of output file name.
     **/
    public void setOutputFile(String outputFile) {
        this.outputFile.setArg(" -o ", outputFile);
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

    /**
      getVerbose
     
      This function is to get class member "verbose"
     
      @return verbose the flag of verbose.
     **/
    public String getVerbose() {
        return this.verbose.getValue();
    }

    /**
      setVerbose
     
      This function is to set class member "verbose"
     
      @param verbose
                 True or False.
     **/
    public void setVerbose(boolean verbose) {
        if (verbose) {
            this.verbose.setArg(" -", "v");
        }
    }

    /**
      getDump
     
      This function is to get class member "dump"
     
      @return verbose the flag of dump.
     **/
    public String getDump() {
        return dump.getValue();
    }

    /**
      setDump
     
      This function is to set class member "dump"
     
      @param dump
                 True or False.
     **/
    public void setDump(boolean dump) {
        if (dump) {
            this.dump.setArg(" -", "dump");
        }
    }
}
