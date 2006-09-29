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
import java.io.FileReader;
import java.io.BufferedReader;

import java.util.List;
import java.util.ArrayList;
import java.util.regex.Pattern;
import java.util.regex.Matcher;

import org.apache.tools.ant.Task;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;

import org.tianocore.common.logger.EdkLog;

/**
 * FlashMapTask class.
 *
 * FlashMapTask is used to call FlashMap.exe to generate flash map defition files and fd files.
 */
public class FlashMapTask extends Task implements EfiDefine {
    //
    // tool name
    //
    private static final String toolName = "FlashMap";

    //
    // 
    // 
    private static Pattern fileBlock = Pattern.compile("\\s*File\\s*\\{([^\\{\\}]+)\\}");
    private static Pattern fileNameDef = Pattern.compile("\\bName\\s*=\\s*\"([^\"]+)\"");
    
    //
    // Flash definition file
    //
    private FileArg flashDefFile = new FileArg();

    //
    // Flash device
    //
    private ToolArg flashDevice = new ToolArg();

    //
    // Flash device Image
    //
    private ToolArg flashDeviceImage = new ToolArg();

    //
    // MCI file
    //
    private FileArg mciFile = new FileArg();

    //
    // MCO file
    //
    private FileArg mcoFile = new FileArg();

    //
    // Discover FD image
    //
    private ToolArg fdImage = new ToolArg();

    //
    // Dsc file
    //
    private FileArg dscFile = new FileArg();

    //
    // Asm INC file
    //
    private FileArg asmIncFile = new FileArg();

    //
    // Image out file
    //
    private FileArg imageOutFile = new FileArg();

    //
    // Header file
    //
    private FileArg headerFile = new FileArg();

    //
    // Input string file
    //
    private String inStrFile = "";

    //
    // Output string file
    //
    private String outStrFile = "";

    //
    //
    //
    private FileArg strFile = new FileArg();
    //
    // Base address
    //
    private ToolArg baseAddr = new ToolArg();

    //
    // Aligment
    //
    private ToolArg aligment = new ToolArg();

    //
    // Padding value
    //
    private ToolArg padValue = new ToolArg();

    //
    // output directory
    //
    private String outputDir = ".";

    //
    // MCI file array
    //
    FileArg mciFileArray = new FileArg();

    /**
      execute

      FlashMapTask execute function is to assemble tool command line & execute
      tool command line

      @throws BuidException
     **/
    public void execute() throws BuildException {
        if (isUptodate()) {
            EdkLog.log(this, EdkLog.EDK_VERBOSE, headerFile.toFileList()
                                                 + imageOutFile.toFileList()
                                                 + mcoFile.toFileList()
                                                 + dscFile.toFileList()
                                                 + asmIncFile.toFileList()
                                                 + outStrFile
                                                 + " is up-to-date!");
            return;
        }

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

        //
        // add substituted input file and output file
        //
        if (this.inStrFile != null && this.outStrFile != null
            && this.inStrFile.length() > 0 && this.outStrFile.length() > 0) {
            strFile.setPrefix(" -strsub ");
            strFile.insValue(this.inStrFile);
            strFile.insValue(this.outStrFile);
        }

        String argument = "" + flashDefFile + flashDevice + flashDeviceImage
                             + mciFile + mcoFile + fdImage + dscFile + asmIncFile
                             + imageOutFile + headerFile + strFile + baseAddr
                             + aligment + padValue + mciFileArray;


        //
        // lauch the program
        //
        // ProcessBuilder pb = new ProcessBuilder(argList);
        // pb.directory(new File(outputDir));
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

            if (outputDir != null) {
                runner.setWorkingDirectory(new File(outputDir));
            }
            //
            // log command line string.
            //
            EdkLog.log(this, EdkLog.EDK_VERBOSE, Commandline.toString(cmdline.getCommandline()));
            EdkLog.log(this, flashDefFile.toFileList()
                             + mciFile.toFileList()
                             + mciFileArray.toFileList()
                             + fdImage.toFileList()
                             + inStrFile
                             + " => "
                             + headerFile.toFileList()
                             + imageOutFile.toFileList()
                             + mcoFile.toFileList()
                             + dscFile.toFileList()
                             + asmIncFile.toFileList()
                             + outStrFile);

            exitCode = runner.execute();
            if (exitCode != 0) {
                EdkLog.log(this, "ERROR = " + Integer.toHexString(exitCode));
            } else {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, "FlashMap succeeded!");
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
      getFlashDefFile

      This function is to get class member "flashDefFile"

      @return flashDeFile Name of flash definition file.
     **/
    public String getFlashDefFile() {
        return this.flashDefFile.getValue();
    }

    /**
      setFlashDefFile

      This function is to set class member "flashDefFile"

      @param flashDefFile
                 Name of flash definition file.
     **/
    public void setFlashDefFile(String flashDefFile) {
        this.flashDefFile.setArg(" -fdf ", flashDefFile);
    }

    /**
      getAligment

      This function is to get class member "aligment"

      @return aligment String of aligment value.
     **/
    public String getAligment() {
        return this.aligment.getValue();
    }

    /**
      setAligment

      This function is to set class member "aligment"

      @param aligment
                 String of aligment value.
     **/
    public void setAligment(String aligment) {
        this.aligment.setArg(" -align ", aligment);
    }

    /**
      getAsmIncFile

      This function is to get class member "asmIncFile"

      @return asmIncFile String of ASM include file.
     **/
    public String getAsmIncFile() {
        return this.asmIncFile.getValue();
    }

    /**
      setAsmIncFile

      This function is to set class member "asmIncFile"

      @param asmIncFile
                 String of ASM include file.
     **/
    public void setAsmIncFile(String asmIncFile) {
        this.asmIncFile.setArg(" -asmincfile ", asmIncFile);
    }

    /**
      getBaseAddr

      This function is to get class member "baseAddr"

      @return baseAddr String of base address value.
     **/
    public String getBaseAddr() {
        return this.baseAddr.getValue();
    }

    /**
      setBaseAddr

      This function is to set class member "baseAddr"

      @param baseAddr
                 String of base address value.
     **/
    public void setBaseAddr(String baseAddr) {
        this.baseAddr.setArg(" -baseaddr ", baseAddr);
    }

    /**
      getDscFile

      This function is to get class member "dscFile"

      @return dscFile name of DSC file
     **/
    public String getDscFile() {
        return this.dscFile.getValue();
    }

    /**
      setDscFile

      This function is to set class member "dscFile"

      @param dscFile
                 name of DSC file
     **/
    public void setDscFile(String dscFile) {
        this.dscFile.setArg(" -dsc ", dscFile);
    }

    /**
      getFdImage

      This function is to get class member "fdImage"

      @return fdImage name of input FDI image file.
     **/
    public String getFdImage() {
        return this.fdImage.getValue();
    }

    /**
      setFdImage

      This function is to set class member "fdImage"

      @param fdImage
                 name of input FDI image file.
     **/
    public void setFdImage(String fdImage) {
        this.fdImage.setArg(" -discover ", fdImage);
    }

    /**
      getFlashDevice

      This function is to get class member "flashDevice".

      @return flashDevice name of flash device.
     **/
    public String getFlashDevice() {
        return this.flashDevice.getValue();
    }

    /**
      setFlashDevice

      This function is to set class member "flashDevice"

      @param flashDevice
                 name of flash device.
     **/
    public void setFlashDevice(String flashDevice) {
        this.flashDevice.setArg(" -flashdevice ", flashDevice);
    }

    /**
      getFlashDeviceImage

      This function is to get class member "flashDeviceImage"

      @return flashDeviceImage name of flash device image
     **/
    public String getFlashDeviceImage() {
        return this.flashDeviceImage.getValue();
    }

    /**
      setFlashDeviceImage

      This function is to set class member "flashDeviceImage"

      @param flashDeviceImage
                 name of flash device image
     **/
    public void setFlashDeviceImage(String flashDeviceImage) {
        this.flashDeviceImage.setArg(" -flashdeviceimage ", flashDeviceImage);

    }

    /**
      getHeaderFile

      This function is to get class member "headerFile"

      @return headerFile name of include file
     **/
    public String getHeaderFile() {
        return this.headerFile.getValue();
    }

    /**
      setHeaderFile

      This function is to set class member "headerFile"

      @param headerFile
                 name of include file
     **/
    public void setHeaderFile(String headerFile) {
        this.headerFile.setArg(" -hfile ", headerFile);
    }

    /**
      getImageOutFile

      This function is to get class member "imageOutFile"

      @return imageOutFile name of output image file
     **/
    public String getImageOutFile() {
        return this.imageOutFile.getValue();
    }

    /**
      setImageOutFile

      This function is to set class member "ImageOutFile"

      @param imageOutFile
                 name of output image file
     **/
    public void setImageOutFile(String imageOutFile) {
        this.imageOutFile.setArg(" -imageout ", imageOutFile);
    }

    /**
      getInStrFile

      This function is to get class member "inStrFile"

      @return inStrFile name of input file which used to replace symbol names.
     **/
    public String getInStrFile() {
        return this.inStrFile;
    }

    /**
      setInStrFile

      This function is to set class member "inStrFile"

      @param inStrFile
                 name of input file which used to replace symbol names.
     **/
    public void setInStrFile(String inStrFile) {
        this.inStrFile = inStrFile;
    }

    /**
      getMciFile

      This function is to get class member "mciFile"

      @return mciFile name of input microcode file
     **/
    public String getMciFile() {
        return this.mciFile.getValue();
    }

    /**
      setMciFile

      This function is to set class member "mciFile"

      @param mciFile
                 name of input microcode file
     **/
    public void setMciFile(String mciFile) {
        this.mciFile.setArg(" -mci ", mciFile);
    }

    /**
      getMcoFile

      This function is to get class member "mcoFile"

      @return mcoFile name of output binary microcode image
     **/
    public String getMcoFile() {
        return this.mcoFile.getValue();
    }

    /**
      setMcoFile

      This function is to set class member "mcoFile"

      @param mcoFile
                 name of output binary microcode image
     **/
    public void setMcoFile(String mcoFile) {
        this.mcoFile.setArg(" -mco ", mcoFile);
    }

    /**
      getOutStrFile

      This function is to get class member "outStrFile"

      @return outStrFile name of output string substitution file
     **/
    public String getOutStrFile() {
        return this.outStrFile;
    }

    /**
      setOutStrFile

      This function is to set class member "outStrFile"

      @param outStrFile
                 name of output string substitution file
     **/
    public void setOutStrFile(String outStrFile) {
        this.outStrFile = outStrFile;
    }

    /**
      getPadValue

      This function is to get class member "padValue"

      @return padValue string of byte value to use as padding
     **/
    public String getPadValue() {
        return this.padValue.getValue();
    }

    /**
      setPadValue

      This function is to set class member "padValue"

      @param padValue
                 string of byte value to use as padding
     **/
    public void setPadValue(String padValue) {
        this.padValue.setArg(" -padvalue ", padValue);
    }

    /**
      addMciFile

      This function is to add Microcode binary file

      @param mciFile
                 instance of input class
     **/
    public void addConfiguredMciFile(FileArg mciFile) {
        this.mciFileArray.setPrefix(" -mcmerge ");
        this.mciFileArray.insert(mciFile);
    }

    /**
      getOutputDir

      This function is to get class member "outputDir"

      @return outputDir string of output directory
     **/
    public String getOutputDir() {
        return outputDir;
    }

    /**
      setOutputDir

      This function is to set class member "outputDir"

      @param outputDir
                 string of output directory
     **/
    public void setOutputDir(String outputDir) {
        this.outputDir = outputDir;
    }

    //
    // Dependency check
    //
    private boolean isUptodate() {
        long srcTimeStamp = 0;
        String srcName = "";
        String dstName = "";
        long timeStamp = 0;

        if (!flashDefFile.isEmpty()) {
            srcName = flashDefFile.getValue();
            timeStamp = new File(srcName).lastModified();
            if (timeStamp > srcTimeStamp) {
                srcTimeStamp = timeStamp;
            }
        }

        if (!mciFile.isEmpty()) {
            srcName = mciFile.getValue();
            timeStamp = new File(srcName).lastModified();
            if (timeStamp > srcTimeStamp) {
                srcTimeStamp = timeStamp;
            }
        }

        if (!fdImage.isEmpty()) {
            srcName = fdImage.getValue();
            timeStamp = new File(srcName).lastModified();
            if (timeStamp > srcTimeStamp) {
                srcTimeStamp = timeStamp;
            }
        }

        if (inStrFile.length() != 0) {
            srcName = inStrFile;
            timeStamp = new File(srcName).lastModified();
            if (timeStamp > srcTimeStamp) {
                srcTimeStamp = timeStamp;
            }
        }

        if (!mciFileArray.isEmpty()) {
            for (int i = 0; i < mciFileArray.nameList.size(); ++i) {
                srcName += mciFileArray.nameList.get(i) + " ";
                timeStamp = new File(mciFileArray.nameList.get(i)).lastModified();
                if (timeStamp > srcTimeStamp) {
                    srcTimeStamp = timeStamp;
                }
            }
        }

        if (!headerFile.isEmpty()) {
            dstName = headerFile.getValue();
            File dstFile = new File(dstName);
            if (!dstFile.isAbsolute()) {
                dstName = outputDir + File.separator + dstName;
                dstFile = new File(dstName);
            }

            if (srcTimeStamp > dstFile.lastModified()) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, srcName + " has been changed since last build!");
                return false;
            }
        }

        if (!imageOutFile.isEmpty()) {
            dstName = imageOutFile.getValue();
            File dstFile = new File(dstName);
            if (!dstFile.isAbsolute()) {
                dstName = outputDir + File.separator + dstName;
                dstFile = new File(dstName);
            }

            if (srcTimeStamp > dstFile.lastModified()) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, srcName + " has been changed since last build!");
                return false;
            }

            //
            // we need to check the time stamp of each FV file specified in fdf file
            // 
            if (!isFdUptodate(dstName, getFvFiles(flashDefFile.getValue()))) {
                return false;
            }
        }

        if (!mcoFile.isEmpty()) {
            dstName = mcoFile.getValue();
            File dstFile = new File(dstName);
            if (!dstFile.isAbsolute()) {
                dstName = outputDir + File.separator + dstName;
                dstFile = new File(dstName);
            }

            if (srcTimeStamp > dstFile.lastModified()) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, srcName + " has been changed since last build!");
                return false;
            }
        }

        if (!dscFile.isEmpty()) {
            dstName = dscFile.getValue();
            File dstFile = new File(dstName);
            if (!dstFile.isAbsolute()) {
                dstName = outputDir + File.separator + dstName;
                dstFile = new File(dstName);
            }

            if (srcTimeStamp > dstFile.lastModified()) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, srcName + " has been changed since last build!");
                return false;
            }
        }

        if (!asmIncFile.isEmpty()) {
            dstName = asmIncFile.getValue();
            File dstFile = new File(dstName);
            if (!dstFile.isAbsolute()) {
                dstName = outputDir + File.separator + dstName;
                dstFile = new File(dstName);
            }

            if (srcTimeStamp > dstFile.lastModified()) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, srcName + " has been changed since last build!");
                return false;
            }
        }

        if (outStrFile.length() != 0) {
            dstName = outStrFile;
            File dstFile = new File(dstName);
            if (!dstFile.isAbsolute()) {
                dstName = outputDir + File.separator + dstName;
                dstFile = new File(dstName);
            }

            if (srcTimeStamp > dstFile.lastModified()) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, srcName + " has been changed since last build!");
                return false;
            }
        }

        return true;
    }

    //
    // Parse the flash definition file and find out the FV file names
    // 
    private List<String> getFvFiles(String fdfFileName) {
        File fdfFile = new File(fdfFileName);
        int fileLength = (int)fdfFile.length();
        char[] fdfContent = new char[fileLength];
        List<String> fileList = new ArrayList<String>();

        try {
            FileReader reader = new FileReader(fdfFile);
            BufferedReader in = new BufferedReader(reader);

            in.read(fdfContent, 0, fileLength);
            String str = new String(fdfContent);

            //
            // match the 
            //      File {
            //        ...
            //      }
            // block
            // 
            Matcher matcher = fileBlock.matcher(str);
            while (matcher.find()) {
                String fileBlockContent = str.substring(matcher.start(1), matcher.end(1));
                //
                // match the definition like
                //      Name = "..."
                //  
                Matcher nameMatcher = fileNameDef.matcher(fileBlockContent);
                if (nameMatcher.find()) {
                    fileList.add(fileBlockContent.substring(nameMatcher.start(1), nameMatcher.end(1)));
                }
            }

            in.close();
            reader.close();
        } catch (Exception ex) {
            throw new BuildException(ex.getMessage());
        }

        return fileList;
    }

    private boolean isFdUptodate(String fdFile, List<String> fvFileList) {
        String fvDir = ".";
        File fd = new File(fdFile);

        if (outputDir.equals(".")) {
            if (!fd.isAbsolute()) {
                //
                // If we cannot get the absolute path of fd file, we caanot
                // get its time stamp. Re-generate it always in such situation.
                // 
                EdkLog.log(this, EdkLog.EDK_VERBOSE, "Cannot retrieve the time stamp of " + fdFile);
                return false;
            }
            fvDir = fd.getParent();
        } else {
            fvDir = outputDir;
            if (!fd.isAbsolute()) {
                fd = new File(fvDir + File.separator + fdFile);
            }
        }

        long fdTimeStamp = fd.lastModified();
        for (int i = 0; i < fvFileList.size(); ++i) {
            File fv = new File(fvDir + File.separator + fvFileList.get(i));
            if (fv.lastModified() > fdTimeStamp) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, fv.getPath() + " has been changed since last build!");
                return false;
            }
        }

        return true;
    }
}
