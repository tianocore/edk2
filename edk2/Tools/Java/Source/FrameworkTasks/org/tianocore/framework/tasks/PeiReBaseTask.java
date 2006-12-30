/** @file
 PeiReBaseTask class.

 PeiReBaseTask is used to call PeiReBase.exe to rebase efi fv file.


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
  PeiReBaseTask class.

  PeiReBaseTask is used to call PeiReBase.exe to rebase efi fv file.
**/
public class PeiReBaseTask extends Task implements EfiDefine {
    //
    // tool name
    //
    private String toolName = "PeiReBase";
    //
    // Input file
    //
    private FileArg inputFile = new FileArg();
    //
    // Output file
    //
    private FileArg outputFile = new FileArg();
    //
    // Base address
    //
    private ToolArg baseAddr = new ToolArg();
    //
    // Fv.inf file
    //
    private FileArg fvinfFile = new FileArg();
    //
    // map file
    //
    private FileArg mapFile = new FileArg();
    //
    // Architecture
    //
    private String arch = "IA32";

    /**
      execute

      PeiReBaseTask execute function is to assemble tool command line & execute
      tool command line

      @throws BuidException
     **/
    public void execute() throws BuildException {
        if (isUptodate()) {
            EdkLog.log(this, EdkLog.EDK_VERBOSE, outputFile.toFileList() + " is up-to-date!");
            return;
        }

        Project project = this.getOwningTarget().getProject();

        //
        // absolute path of efi tools
        //
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH");
        String command;
        String argument;
        if (this.arch.equalsIgnoreCase("IA32")){
            command = toolName + "_IA32";
        } else if (this.arch.equalsIgnoreCase("X64")){
            command = toolName + "_X64";
        } else if (this.arch.equalsIgnoreCase("IPF")){
            command = toolName + "_IPF";
        } else {
            command = toolName + "_IA32";
        }
        if (path != null) {
            command = path + File.separator + command;
        }

        //
        // argument of tools
        //
        if (mapFile.getValue().length() == 0) {
            mapFile.setArg(" -M ", outputFile.getValue() + ".map");
        }
        argument = "" + inputFile + outputFile + baseAddr + fvinfFile;

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
            //
            // Set debug log information.
            //
            EdkLog.log(this, EdkLog.EDK_VERBOSE, Commandline.toString(cmdline.getCommandline()));
            EdkLog.log(this, EdkLog.EDK_INFO, inputFile.toFileList() + " => "
                                              + outputFile.toFileList()
                                              + mapFile.toFileList());

            revl = runner.execute();

            if (EFI_SUCCESS == revl) {
                //
                // command execution success
                //
                EdkLog.log(this, EdkLog.EDK_VERBOSE, "PeiReBase succeeded!");
            } else {
                //
                // command execution fail
                //
                EdkLog.log(this, EdkLog.EDK_INFO, "ERROR = " + Integer.toHexString(revl));
                throw new BuildException("PeiReBase failed!");
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
        this.inputFile.setArg(" -I ", inputFile);
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
        this.outputFile.setArg(" -O ", outputFile);
    }

    /**
      getBaseAddr

      This function is to get class member "baseAddr"

      @return baseAddr   string of base address.
     **/
    public String getBaseAddr() {
        return baseAddr.getValue();
    }

    /**
      setBaseAddr

      This function is to set class member "baseAddr"

      @param baseAddr    string of base address
     **/
    public void setBaseAddr(String baseAddr) {
        this.baseAddr.setArg(" -B ", baseAddr);
    }

    /**
      getArch

      This function is to get class member "arch".

      @return arch       Architecture
     **/
    public String getArch() {
        return arch;
    }

    /**
      setArch

      This function is to set class member "arch"

      @param arch         Architecture
     **/
    public void setArch(String arch) {
        this.arch = arch;
    }

    /**
       Get the value of fv.inf file

       @return String   The fv.inf file path
     **/
    public String getFvInfFile() {
        return fvinfFile.getValue();
    }

    /**
       Set "-F FvinfFile" argument

       @param fvinfFile   The path of fv.inf file
     **/
    public void setFvInfFile(String fvinfFile) {
        this.fvinfFile.setArg(" -F ", fvinfFile);
    }

    /**
       Get the value of map file

       @return String   The map file path
     **/
    public String getMapFile() {
        return mapFile.getValue();
    }

    /**
       Set "-M MapFile" argument

       @param mapFile   The path of map file
     **/
    public void setMapFile(String mapFile) {
        this.mapFile.setArg(" -M ", mapFile);
    }

    //
    // Dependency check
    //
    private boolean isUptodate() {
        File srcFile = new File(inputFile.getValue());
        File dstFile = new File(outputFile.getValue());

        if (srcFile.lastModified() > dstFile.lastModified()) {
            return false;
        }

        return true;
    }
}
