/** @file
 FlashMapTask class.

 FlashMapTask is used to call FlashMap.exe to lay out the flash.


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
 * FlashMapTask class.
 *
 * FlashMapTask is used to call FlashMap.exe to generate flash map defition files and fd files.
 */
public class FlashMapTask extends Task implements EfiDefine {
    // /
    // / tool name
    // /
    private final String toolName = "FlashMap";

    // /
    // / Flash definition file
    // /
    private String flashDefFile = "";

    // /
    // / Flash device
    // /
    private String flashDevice = "";

    // /
    // / Flash device Image
    // /
    private String flashDeviceImage = "";

    // /
    // / MCI file
    // /
    private String mciFile = "";

    // /
    // / MCO file
    // /
    private String mcoFile = "";

    // /
    // / Discover FD image
    // /
    private String fdImage = "";

    // /
    // / Dsc file
    // /
    private String dscFile = "";

    // /
    // / Asm INC file
    // /
    private String asmIncFile = "";

    // /
    // / Image out file
    // /
    private String imageOutFile = "";

    // /
    // / Header file
    // /
    private String headerFile = "";

    // /
    // / Input string file
    // /
    private String inStrFile = "";

    // /
    // / Output string file
    // /
    private String outStrFile = "";

    // /
    // / Base address
    // /
    private String baseAddr = "";

    // /
    // / Aligment
    // /
    private String aligment = "";

    // /
    // / Padding value
    // /
    private String padValue = "";

    // /
    // / output directory
    // /
    private String outputDir = ".";

    // /
    // / MCI file array
    // /
    List<Input> mciFileArray = new ArrayList<Input>();

    // /
    // / command and argument list
    // /
    LinkedList<String> argList = new LinkedList<String>();

    /**
     * execute
     *
     * FlashMapTask execute function is to assemble tool command line & execute
     * tool command line
     *
     * @throws BuidException
     */
    public void execute() throws BuildException {

        Project project = this.getOwningTarget().getProject();
        //
        // set Logger
        //
        FrameworkLogger logger = new FrameworkLogger(project, "flashmap");
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
        // add substituted input file and output file
        //
        if (this.inStrFile != null && this.outStrFile != null
                && !this.inStrFile.equalsIgnoreCase("")
                && !this.inStrFile.equalsIgnoreCase("")) {
            argList.add("-strsub");
            argList.add(this.inStrFile);
            argList.add(this.outStrFile);
        }


        //
        // add microcode binary files
        //
        if (mciFileArray.size() > 0) {
            argList.add("-mcmerge");
            Iterator mciList = mciFileArray.iterator();
            while (mciList.hasNext()) {
                argList.addAll(((Input) mciList.next()).getNameList());
            }
        }

        //
        // lauch the program
        //
        ProcessBuilder pb = new ProcessBuilder(argList);
        pb.directory(new File(outputDir));
        int exitCode = 0;
        try {
            Process cmdProc = pb.start();
            InputStreamReader cmdOut = new InputStreamReader(cmdProc
                    .getInputStream());
            char[] buf = new char[1024];

            exitCode = cmdProc.waitFor();
            //
            // log command line string.
            //
            EdkLog.log(EdkLog.EDK_VERBOSE, cmdProc.getOutputStream().toString());
            EdkLog.log(EdkLog.EDK_INFO, (new File(this.flashDefFile)).getName());
            if (exitCode != 0) {
                int len = cmdOut.read(buf, 0, 1024);
                EdkLog.log(EdkLog.EDK_INFO, new String(buf, 0, len));
            } else {
                EdkLog.log(EdkLog.EDK_VERBOSE, "FlashMap succeeded!");
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        } finally {
            if (exitCode != 0) {
                throw new BuildException("FlashMap failed!");
            }
        }
    }

    /**
     * getFlashDefFile
     *
     * This function is to get class member "flashDefFile"
     *
     * @return flashDeFile Name of flash definition file.
     */
    public String getFlashDefFile() {
        return flashDefFile;
    }

    /**
     * setFlashDefFile
     *
     * This function is to set class member "flashDefFile"
     *
     * @param flashDefFile
     *            Name of flash definition file.
     */
    public void setFlashDefFile(String flashDefFile) {
        this.flashDefFile = flashDefFile;
        argList.add("-fdf");
        argList.add(this.flashDefFile);
    }

    /**
     * getAligment
     *
     * This function is to get class member "aligment"
     *
     * @return aligment String of aligment value.
     */
    public String getAligment() {
        return aligment;
    }

    /**
     * setAligment
     *
     * This function is to set class member "aligment"
     *
     * @param aligment
     *            String of aligment value.
     */
    public void setAligment(String aligment) {
        this.aligment = aligment;
        argList.add("-align");
        argList.add(this.aligment);
    }

    /**
     * getAsmIncFile
     *
     * This function is to get class member "asmIncFile"
     *
     * @return asmIncFile String of ASM include file.
     */
    public String getAsmIncFile() {
        return asmIncFile;
    }

    /**
     * setAsmIncFile
     *
     * This function is to set class member "asmIncFile"
     *
     * @param asmIncFile
     *            String of ASM include file.
     */
    public void setAsmIncFile(String asmIncFile) {
        this.asmIncFile = asmIncFile;
        argList.add("-asmincfile");
        argList.add(this.asmIncFile);
    }

    /**
     * getBaseAddr
     *
     * This function is to get class member "baseAddr"
     *
     * @return baseAddr String of base address value.
     */
    public String getBaseAddr() {
        return baseAddr;
    }

    /**
     * setBaseAddr
     *
     * This function is to set class member "baseAddr"
     *
     * @param baseAddr
     *            String of base address value.
     */
    public void setBaseAddr(String baseAddr) {
        this.baseAddr = baseAddr;
        argList.add("-baseaddr");
        argList.add(this.baseAddr);
    }

    /**
     * getDscFile
     *
     * This function is to get class member "dscFile"
     *
     * @return dscFile name of DSC file
     */
    public String getDscFile() {
        return dscFile;
    }

    /**
     * setDscFile
     *
     * This function is to set class member "dscFile"
     *
     * @param dscFile
     *            name of DSC file
     */
    public void setDscFile(String dscFile) {
        this.dscFile = dscFile;
        argList.add("-dsc");
        argList.add(this.dscFile);
    }

    /**
     * getFdImage
     *
     * This function is to get class member "fdImage"
     *
     * @return fdImage name of input FDI image file.
     */
    public String getFdImage() {
        return fdImage;
    }

    /**
     * setFdImage
     *
     * This function is to set class member "fdImage"
     *
     * @param fdImage
     *            name of input FDI image file.
     */
    public void setFdImage(String fdImage) {
        this.fdImage = fdImage;
        argList.add("-discover");
        argList.add(this.fdImage);
    }

    /**
     * getFlashDevice
     *
     * This function is to get class member "flashDevice".
     *
     * @return flashDevice name of flash device.
     */
    public String getFlashDevice() {
        return flashDevice;
    }

    /**
     * setFlashDevice
     *
     * This function is to set class member "flashDevice"
     *
     * @param flashDevice
     *            name of flash device.
     */
    public void setFlashDevice(String flashDevice) {
        this.flashDevice = flashDevice;
        argList.add("-flashdevice");
        argList.add(this.flashDevice);
    }

    /**
     * getFlashDeviceImage
     *
     * This function is to get class member "flashDeviceImage"
     *
     * @return flashDeviceImage name of flash device image
     */
    public String getFlashDeviceImage() {
        return flashDeviceImage;
    }

    /**
     * setFlashDeviceImage
     *
     * This function is to set class member "flashDeviceImage"
     *
     * @param flashDeviceImage
     *            name of flash device image
     */
    public void setFlashDeviceImage(String flashDeviceImage) {
        this.flashDeviceImage = flashDeviceImage;
        argList.add("-flashdeviceimage");
        argList.add(this.flashDeviceImage);

    }

    /**
     * getHeaderFile
     *
     * This function is to get class member "headerFile"
     *
     * @return headerFile name of include file
     */
    public String getHeaderFile() {
        return headerFile;
    }

    /**
     * setHeaderFile
     *
     * This function is to set class member "headerFile"
     *
     * @param headerFile
     *            name of include file
     */
    public void setHeaderFile(String headerFile) {
        this.headerFile = headerFile;
        argList.add("-hfile");
        argList.add(this.headerFile);
    }

    /**
     * getImageOutFile
     *
     * This function is to get class member "imageOutFile"
     *
     * @return imageOutFile name of output image file
     */
    public String getImageOutFile() {
        return imageOutFile;
    }

    /**
     * setImageOutFile
     *
     * This function is to set class member "ImageOutFile"
     *
     * @param imageOutFile
     *            name of output image file
     */
    public void setImageOutFile(String imageOutFile) {
        this.imageOutFile = imageOutFile;
        argList.add("-imageout");
        argList.add(this.imageOutFile);
    }

    /**
     * getInStrFile
     *
     * This function is to get class member "inStrFile"
     *
     * @return inStrFile name of input file which used to replace symbol names.
     */
    public String getInStrFile() {
        return inStrFile;
    }

    /**
     * setInStrFile
     *
     * This function is to set class member "inStrFile"
     *
     * @param inStrFile
     *            name of input file which used to replace symbol names.
     */
    public void setInStrFile(String inStrFile) {
        this.inStrFile = inStrFile;
    }

    /**
     * getMciFile
     *
     * This function is to get class member "mciFile"
     *
     * @return mciFile name of input microcode file
     */
    public String getMciFile() {
        return mciFile;
    }

    /**
     * setMciFile
     *
     * This function is to set class member "mciFile"
     *
     * @param mciFile
     *            name of input microcode file
     */
    public void setMciFile(String mciFile) {
        this.mciFile = mciFile;
        argList.add("-mci");
        argList.add(this.mciFile);
    }

    /**
     * getMcoFile
     *
     * This function is to get class member "mcoFile"
     *
     * @return mcoFile name of output binary microcode image
     */
    public String getMcoFile() {
        return mcoFile;
    }

    /**
     * setMcoFile
     *
     * This function is to set class member "mcoFile"
     *
     * @param mcoFile
     *            name of output binary microcode image
     */
    public void setMcoFile(String mcoFile) {
        this.mcoFile = mcoFile;
        argList.add("-mco");
        argList.add(this.mcoFile);
    }

    /**
     * getOutStrFile
     *
     * This function is to get class member "outStrFile"
     *
     * @return outStrFile name of output string substitution file
     */
    public String getOutStrFile() {
        return outStrFile;
    }

    /**
     * setOutStrFile
     *
     * This function is to set class member "outStrFile"
     *
     * @param outStrFile
     *            name of output string substitution file
     */
    public void setOutStrFile(String outStrFile) {
        this.outStrFile = outStrFile;
    }

    /**
     * getPadValue
     *
     * This function is to get class member "padValue"
     *
     * @return padValue string of byte value to use as padding
     */
    public String getPadValue() {
        return padValue;
    }

    /**
     * setPadValue
     *
     * This function is to set class member "padValue"
     *
     * @param padValue
     *            string of byte value to use as padding
     */
    public void setPadValue(String padValue) {
        this.padValue = padValue;
        argList.add("-padvalue");
        argList.add(this.padValue);
    }

    /**
     * addMciFile
     *
     * This function is to add Microcode binary file
     *
     * @param mciFile
     *            instance of input class
     */
    public void addMciFile(Input mciFile) {
        this.mciFileArray.add(mciFile);
    }

    /**
     * getOutputDir
     *
     * This function is to get class member "outputDir"
     *
     * @return outputDir string of output directory
     */
    public String getOutputDir() {
        return outputDir;
    }

    /**
     * setOutputDir
     *
     * This function is to set class member "outputDir"
     *
     * @param outputDir
     *            string of output directory
     */
    public void setOutputDir(String outputDir) {
        this.outputDir = outputDir;
    }
}
