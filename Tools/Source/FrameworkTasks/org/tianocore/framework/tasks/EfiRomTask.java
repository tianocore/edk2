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
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

import org.apache.tools.ant.Task;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.BuildException;

import org.tianocore.common.logger.EdkLog;

/**
 * SecFixupTask class.
 *
 * SecFixupTask is used to call SecFixup.exe to fix up sec image.
 */
public class EfiRomTask extends Task implements EfiDefine {
    ///
    /// tool name
    ///
    private final String toolName = "EfiRom";

    ///
    /// Flash default file
    ///
    private String verbose = "";

    ///
    /// Flash device
    ///
    private String venderId = "";

    ///
    /// Flash device Image
    ///
    private String deviceId = "";

    ///
    /// MCI file
    ///
    private String outputFile = "";

    ///
    /// MCO file
    ///
    private List<Input> binaryFileList = new ArrayList<Input>();

    ///
    /// Efi PE32 image file
    ///
    private List<Input> pe32FileList = new ArrayList<Input>();

    ///
    /// Compress efi PE32 image file
    ///
    private List<Input> pe32ComprFileList = new ArrayList<Input>();

    ///
    /// Hex class code in the PCI data strutor header
    ///
    private String classCode = "";

    ///
    /// Hex revision in the PCI data header.
    ///
    private String revision = "";

    ///
    /// Dump the headers of an existing option rom image.
    ///
    private String dump = "";


    ///
    /// output directory
    ///
    private String outputDir = ".";


    ///
    /// command and argument list
    ///
    LinkedList<String> argList = new LinkedList<String>();
    /**
     * execute
     *
     * EfiRomTask execute function is to assemble tool command line & execute
     * tool command line
     *
     * @throws BuidException
     */
    public void execute() throws BuildException {

        Project project = this.getOwningTarget().getProject();
        //
        // set Logger
        //
        FrameworkLogger logger = new FrameworkLogger(project, "efirom");
        EdkLog.setLogLevel(project.getProperty("env.LOGLEVEL"));
        EdkLog.setLogger(logger);
        //
        // absolute path of efi tools
        //
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH");
        String command;
        if (path == null) {
            command = toolName;
        } else {
            command = path + File.separatorChar + toolName;
        }
        argList.addFirst(command);

        //
        // add microcode binary files
        //
        if (this.binaryFileList.size() > 0){
            argList.add("-b");
            Iterator binList = this.binaryFileList.iterator();
            while (binList.hasNext()){
                argList.addAll(((Input)binList.next()).getNameList());
            }
        }

        //
        // add pe32 file
        //
        if (this.pe32FileList.size() > 0){
            argList.add("-e");
            Iterator pe32List = this.pe32FileList.iterator();
            while (pe32List.hasNext()){
                argList.addAll(((Input)pe32List.next()).getNameList());
            }
        }

        //
        // add compressed pe32 file
        //
        if (this.pe32ComprFileList.size() > 0){
            argList.add("-ec");
            Iterator pe32ComprList = this.pe32ComprFileList.iterator();
            while (pe32ComprList.hasNext()){
                argList.addAll(((Input)pe32ComprList.next()).getNameList());
            }
        }

        EdkLog.log(EdkLog.EDK_VERBOSE, argList.toString().replaceAll(",",""));
        EdkLog.log(EdkLog.EDK_INFO, " ");

        //
        // lauch the program
        //
        ProcessBuilder pb = new ProcessBuilder(argList);
        pb.directory(new File(outputDir));
        int exitCode = 0;
        try {
            Process cmdProc = pb.start();
            InputStreamReader cmdOut = new InputStreamReader(cmdProc.getInputStream());
            char[] buf = new char[1024];

            exitCode = cmdProc.waitFor();
            if (exitCode != 0) {
                int len = cmdOut.read(buf, 0, 1024);
                EdkLog.log(EdkLog.EDK_INFO, new String(buf, 0, len));
            } else {
                EdkLog.log(EdkLog.EDK_VERBOSE, "EfiRom succeeded!");
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        } finally {
            if (exitCode != 0) {
                throw new BuildException("EfiRom failed!");
            }
        }
    }

    /**
     * getVerbose
     *
     * This function is to get class member "verbose"
     *
     * @return verbose   for verbose output.
     */
    public String getVerbose() {
        return verbose;
    }

    /**
     * setVerbose
     *
     * This function is to set class member "verbose"
     *
     * @param verbose    for verbose output.
     */
    public void setVerbose(boolean verbose) {
        if (verbose){
            this.verbose = "-p";
            argList.add(this.verbose);
        }
    }

    /**
     * getVenderId
     *
     * This function is to get class member "venderId"
     *
     * @return venderId     String of venderId.
     */
    public String getVenderId() {
        return venderId;
    }

    /**
     * setVenderId
     *
     * This function is to set class member "venderId"
     *
     * @param venderId      String of venderId.
     */
    public void setVenderId(String VenderId) {
        this.venderId = VenderId;
        argList.add("-v");
        argList.add(this.venderId);
    }

    /**
     * getDeviceId
     *
     * This function is to get class member "deviceId"
     *
     * @return deviceId   String of device ID.
     */
    public String getDeviceId() {
        return this.deviceId;
    }

    /**
     * setDeviceId
     *
     * This function is to set class member "deviceId"
     *
     * @param deviceId   String of device ID.
     */
    public void setDeviceId(String deviceId) {
        this.deviceId = deviceId;
        argList.add("-d");
        argList.add(this.deviceId);
    }


    /**
     * getOutputFile
     *
     * This function is to get class member "outputFile"
     *
     * @return outputFile     name of output directory.
     */
    public String getOutputFile() {
        return outputFile;
    }

    /**
     * setOutputFile
     *
     * This function is to set class member "dscFile"
     *
     * @param outputFile      name of DSC file
     */
    public void setOutputFile(String outputFile) {
        this.outputFile = outputFile;

    }

    /**
     * getClassCode
     *
     * This function is to get class member "classCode"
     *
     * @return fdImage       name of class code file.
     */
    public String getClassCode() {
        return classCode;
    }

    /**
     * setclassCode
     *
     * This function is to set class member "classCode"
     *
     * @param fdImage        name of class code file.
     */
    public void setclassCode(String classCode) {
        this.classCode = classCode;
        argList.add("-cc");
        argList.add(this.classCode);
    }

    /**
     * getRevision
     *
     * This function is to get class member "revision".
     *
     * @return revision     hex revision in the PDI data header.
     */
    public String getRevision() {
        return revision;
    }

    /**
     * setRevision
     *
     * This function is to set class member "revision"
     *
     * @param revision     hex revision in the PDI data header.
     */
    public void setRevision(String revision) {
        this.revision = revision;
        argList.add("-rev");
        argList.add(this.revision);
    }

    /**
     * getFlashDeviceImage
     *
     * This function is to get class member "dump"
     *
     * @return flashDeviceImage      name of flash device image
     */
    public String getDump() {
        return dump;
    }

    /**
     * setFlashDeviceImage
     *
     * This function is to set class member "dump"
     *
     * @param flashDeviceImage        name of flash device image
     */
    public void setDump(boolean dump) {
        if (dump){
            this.dump = "-dump";
            argList.add(this.dump);
        }
    }

    /**
     * getOutputDir
     *
     * This function is to get class member "outputDir"
     *
     * @return outputDir       string of output directory
     */
    public String getOutputDir() {
        return outputDir;
    }

    /**
     * setOutputDir
     *
     * This function is to set class member "outputDir"
     *
     * @param outputDir         string of output directory
     */
    public void setOutputDir(String outputDir) {
        this.outputDir = outputDir;
    }
    /**
     * addBinaryFile
     *
     * This function is to add binary file to binaryFile list.
     *
     * @param binaryFile         name of binary file.
     */
    public void addBinaryFile(Input binaryFile){
        this.binaryFileList.add(binaryFile);
    }

    /**
     * addPe32File
     *
     * This function is to add pe32 file to pe32File list.
     *
     * @param pe32File            name of pe32 file.
     */
    public void addPe32File(Input pe32File){
        this.pe32FileList.add(pe32File);
    }

    /**
     * addPe32ComprFile
     *
     * This function os to add compressed pe32 file to pe32ComprFile list.
     *
     * @param pe32ComprFile        name of compressed pe32 file.
     */
    public void addPe32ComprFile(Input pe32ComprFile){
        this.pe32ComprFileList.add(pe32ComprFile);
    }
}
