/** @file
 EfiRomTask class.

 EfiRomTask is used to call FlashMap.exe to lay out the flash.


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
import java.util.LinkedList;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.tianocore.common.logger.EdkLog;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;

/**
  SecFixupTask class.
 
  SecFixupTask is used to call SecFixup.exe to fix up sec image.
 **/
public class EfiRomTask extends Task implements EfiDefine {
    //
    // tool name
    //
    private final static String toolName = "EfiRom";

    //
    // Flash default file
    //
    private ToolArg verbose = new ToolArg();

    //
    // Flash device
    //
    private ToolArg venderId = new ToolArg();

    //
    // Flash device Image
    //
    private ToolArg deviceId = new ToolArg();

    //
    // output file
    //
    private FileArg outputFile = new FileArg();

    //
    // binary file
    //
    private Input binaryFileList = new Input();

    //
    // Efi PE32 image file
    //
    private Input pe32FileList = new Input();

    //
    // Compress efi PE32 image file
    //
    private Input pe32ComprFileList = new Input();

    //
    // Hex class code in the PCI data strutor header
    //
    private ToolArg classCode = new ToolArg();

    //
    // Hex revision in the PCI data header.
    //
    private ToolArg revision = new ToolArg();

    //
    // Dump the headers of an existing option rom image.
    //
    private ToolArg dump = new ToolArg();

    //
    // output directory
    //
    private String outputDir = ".";

    //
    // command and argument list
    //
    LinkedList<String> argList = new LinkedList<String>();

    /**
      execute
     
      EfiRomTask execute function is to assemble tool command line & execute
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
        if (path == null) {
            command = toolName;
        } else {
            command = path + File.separator + toolName;
        }

        String argument = "" + verbose + venderId + deviceId + dump + revision + classCode 
                             + binaryFileList.toStringWithSinglepPrefix(" -b ")
                             + pe32FileList.toStringWithSinglepPrefix(" -e ")
                             + pe32ComprFileList.toStringWithSinglepPrefix(" -ec ")
                             + outputFile;

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

            EdkLog.log(this, EdkLog.EDK_VERBOSE, Commandline.toString(cmdline.getCommandline()));
            EdkLog.log(this, EdkLog.EDK_INFO, binaryFileList.toFileList() 
                       + pe32FileList.toFileList() + pe32ComprFileList.toFileList()
                       + " => " + outputFile.toFileList());

            int exitCode = runner.execute();
            if (exitCode != 0) {
                //
                // command execution fail
                //
                EdkLog.log(this, "ERROR = " + Integer.toHexString(exitCode));
                throw new BuildException(toolName + " failed!");
            } else {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, toolName + " succeeded!");
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }
    }

    /**
      getVerbose
     
      This function is to get class member "verbose"
     
      @return verbose   for verbose output.
     **/
    public String getVerbose() {
        return verbose.getValue();
    }

    /**
      setVerbose
     
      This function is to set class member "verbose"
     
      @param verbose    for verbose output.
     **/
    public void setVerbose(boolean verbose) {
        if (verbose){
            this.verbose.setArg(" -", "p");
        }
    }

    /**
      getVenderId
     
      This function is to get class member "venderId"
     
      @return venderId     String of venderId.
     **/
    public String getVenderId() {
        return venderId.getValue();
    }

    /**
      setVenderId
     
      This function is to set class member "venderId"
     
      @param venderId      String of venderId.
     **/
    public void setVenderId(String venderId) {
        this.venderId.setArg(" -v ", venderId);
    }

    /**
      getDeviceId
     
      This function is to get class member "deviceId"
     
      @return deviceId   String of device ID.
     **/
    public String getDeviceId() {
        return this.deviceId.getValue();
    }

    /**
      setDeviceId
     
      This function is to set class member "deviceId"
     
      @param deviceId   String of device ID.
     **/
    public void setDeviceId(String deviceId) {
        this.deviceId.setArg(" -d ", deviceId);
    }


    /**
      getOutputFile
     
      This function is to get class member "outputFile"
     
      @return outputFile     name of output directory.
     **/
    public String getOutputFile() {
        return outputFile.getValue();
    }

    /**
      setOutputFile
     
      This function is to set class member "dscFile"
     
      @param outputFile      name of DSC file
     **/
    public void setOutputFile(String outputFile) {
        this.outputFile.setArg(" -o ", outputFile);
    }

    /**
      getClassCode
     
      This function is to get class member "classCode"
     
      @return fdImage       name of class code file.
     **/
    public String getClassCode() {
        return classCode.getValue();
    }

    /**
      setclassCode
     
      This function is to set class member "classCode"
     
      @param fdImage        name of class code file.
     **/
    public void setclassCode(String classCode) {
        this.classCode.setArg(" -cc ", classCode);
    }

    /**
      getRevision
     
      This function is to get class member "revision".
     
      @return revision     hex revision in the PDI data header.
     **/
    public String getRevision() {
        return revision.getValue();
    }

    /**
      setRevision
     
      This function is to set class member "revision"
     
      @param revision     hex revision in the PDI data header.
     **/
    public void setRevision(String revision) {
        this.revision.setArg(" -rev ", revision);
    }

    /**
      getFlashDeviceImage
     
      This function is to get class member "dump"
     
      @return flashDeviceImage      name of flash device image
     **/
    public String getDump() {
        return dump.getValue();
    }

    /**
      setFlashDeviceImage
     
      This function is to set class member "dump"
     
      @param flashDeviceImage        name of flash device image
     **/
    public void setDump(boolean dump) {
        if (dump) {
            this.dump.setArg(" -", "dump");
        }
    }

    /**
      getOutputDir
     
      This function is to get class member "outputDir"
     
      @return outputDir       string of output directory
     **/
    public String getOutputDir() {
        return outputDir;
    }

    /**
      setOutputDir
     
      This function is to set class member "outputDir"
     
      @param outputDir         string of output directory
     **/
    public void setOutputDir(String outputDir) {
        this.outputDir = outputDir;
    }

    /**
      addBinaryFile
     
      This function is to add binary file to binaryFile list.
     
      @param binaryFile         name of binary file.
     **/
    public void addConfiguredBinaryFile(Input binaryFile){
        this.binaryFileList.insert(binaryFile);
    }

    /**
      addPe32File
     
      This function is to add pe32 file to pe32File list.
     
      @param pe32File            name of pe32 file.
     **/
    public void addConfiguredPe32File(Input pe32File){
        this.pe32FileList.insert(pe32File);
    }

    /**
      addPe32ComprFile
     
      This function os to add compressed pe32 file to pe32ComprFile list.
     
      @param pe32ComprFile        name of compressed pe32 file.
     **/
    public void addConfiguredPe32ComprFile(Input pe32ComprFile){
        this.pe32ComprFileList.insert(pe32ComprFile);
    }
}
