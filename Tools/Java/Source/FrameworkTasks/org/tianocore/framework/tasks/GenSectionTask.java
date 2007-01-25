/** @file
 GenSectionTask class.

 GenSectionTask is to call GenSection.exe to generate Section.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
**/

package org.tianocore.framework.tasks;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;
import org.tianocore.common.logger.EdkLog;

public class GenSectionTask extends Task implements EfiDefine, Section, FfsTypes {
    private int alignment = 0;
    //
    // Tool name
    // 
    private final static String toolName = "GenSection";
    //
    // inputfile name
    //
    private FileArg inputFile = new FileArg();

    //
    // outputfile name
    //
    private FileArg outputFile = new FileArg();

    //
    // section type
    //
    private ToolArg sectionType = new ToolArg();

    //
    // version number
    //
    private ToolArg versionNum = new ToolArg();

    //
    // interface string
    //
    private ToolArg interfaceString = new ToolArg();

    //
    // Section file list
    //
    private List<Section> sectFileList = new ArrayList<Section>();

    //
    // flag indicated the <tool> element
    //
    private boolean haveTool = false;

    /**
      execute
      
      GenSectionTaks execute is to assemble tool command line & execute tool
      command line.
      
      @throws BuildException
    **/
    public void execute() throws BuildException {
        String command;
        Project project = this.getOwningTarget().getProject();
        //
        // absolute path of efi tools
        //
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH");
        if (path == null) {
            command = toolName;
        } else {
            command = path + File.separator + toolName;
        }
        //
        // argument of tools
        //
        String argument = "" + inputFile + outputFile + sectionType + versionNum + interfaceString;
        //
        // return value of gensection execution
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

            EdkLog.log(this, inputFile.toFileList() + versionNum.getValue() 
                + interfaceString.getValue() + " => " + outputFile.toFileList());
            EdkLog.log(this, EdkLog.EDK_VERBOSE, Commandline.toString(cmdline.getCommandline()));

            revl = runner.execute();
            if (EFI_SUCCESS == revl) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, toolName + " succeeded!");
            } else {
                //
                // command execution fail
                //
                EdkLog.log(this, EdkLog.EDK_INFO, "ERROR = " + Integer.toHexString(revl));
                throw new BuildException(toolName + " failed!");
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }
    }

    /**
      getInputFile
      
      This function is to get class member "inputFile".
      
      @return                    name of input file
    **/
    public String getInputFile() {
        return this.inputFile.getValue();
    }

    /**
      setInputFile
      
      This function is to set class member "inputFile".
      
      @param inputFile            name of input file
    **/
    public void setInputFile(String inputFile) {
        this.inputFile.setArg(" -i ", inputFile);
    }

    /**
      getOutputFile
      
      This function is to get class member "outputFile".
      
      @return                      name of output file
    **/
    public String getOutputFile() {
        return this.outputFile.getValue();
    }

    /**
      setOutputfile
      
      This function is to set class member "outputFile".
      @param  outputFile            name of output file
    **/
    public void setOutputfile(String outputFile) {
        this.outputFile.setArg(" -o ", outputFile);
    }

    /**
      getSectionType
      
      This function is to get class member "sectionType".
      
      @return                        sectoin type
    **/
    public String getSectionType() {
        return this.sectionType.getValue();
    }

    /**
      setSectionType
      
      This function is to set class member "sectionType".
      
      @param sectionType              section type
    **/
    public void setSectionType(String sectionType) {
        this.sectionType.setArg(" -s ", sectionType);
    }

    /**
      getVersionNum
      
      This function is to get class member "versionNum".
      @return                         version number
    **/
    public String getVersionNum() {
        return this.versionNum.getValue();
    }

    /**
      setVersionNume
      
      This function is to set class member "versionNum".
      @param versionNum               version number
    **/
    public void setVersionNum(String versionNum) {
        this.versionNum.setArg(" -v ", versionNum);
    }

    /**
      getInterfaceString
      
      This function is to get class member "interfaceString".
      @return                         interface string
    **/
    public String getInterfaceString() {
        return this.interfaceString.getValue();
    }

    /**
      setInterfaceString
      
      This funcion is to set class member "interfaceString".
      @param interfaceString            interface string
    **/
    public void setInterfaceString(String interfaceString) {
        this.interfaceString.setArg(" -a ", "\"" + interfaceString + "\"");
    }
    
    /**
      addSectFile
      
      This function is to add sectFile to list.
      
      @param sectFile     instance of sectFile.
    **/
    public void addSectFile(SectFile sectFile){
        this.sectFileList.add(sectFile);
    }

    /**
      setTool
      
      This function is to set the class member "Tool";
      
      @param tool 
    **/
    public void addTool(Tool tool) {
        this.sectFileList.add(tool);
        this.haveTool = true;
    }
    
    /**
      addGenSection
      
      This function is to add GenSectin element to list
      @param task         Instance of genSection
    **/
    public void addGenSection(GenSectionTask task){
        this.sectFileList.add(task);
    }
    
    public int getAlignment() {
        return alignment;
    }

    public void setAlignment(int alignment) {
        if (alignment > 7) {
            this.alignment = 7;
        } else {
            this.alignment = alignment;
        }
    }

    public void toBuffer(DataOutputStream buffer){
        //
        // Search SectionList find earch section and call it's
        // ToBuffer function.
        //
        if (this.sectionType.getValue().equalsIgnoreCase(
                "EFI_SECTION_COMPRESSION")
                && !this.haveTool) {
            Section sect;

            //
            // Get section file in compress node.
            //
            try {
                ByteArrayOutputStream bo = new ByteArrayOutputStream();
                DataOutputStream Do = new DataOutputStream(bo);

                //
                // Get each section which under the compress {};
                // And add it is contains to File;
                //
                Iterator SectionIter = this.sectFileList.iterator();
                while (SectionIter.hasNext()) {
                    sect = (Section) SectionIter.next();

                    //
                    // Call each section class's toBuffer function.
                    //
                    try {
                        sect.toBuffer(Do);
                    } catch (BuildException e) {
                        System.out.print(e.getMessage());
                        throw new BuildException(
                                "Compress.toBuffer failed at section");
                    } finally {
                        if (Do != null){
                            Do.close();
                        }
                    }
                }
                //
                // Call compress
                //
                byte[] fileBuffer = bo.toByteArray();

                synchronized (CompressSection.semaphore) {
                    Compress myCompress = new Compress(fileBuffer,
                            fileBuffer.length);

                    //
                    // Add Compress header
                    //
                    CompressHeader Ch = new CompressHeader();
                    Ch.SectionHeader.Size[0] = (byte) ((myCompress.outputBuffer.length + Ch
                            .GetSize()) & 0xff);
                    Ch.SectionHeader.Size[1] = (byte) (((myCompress.outputBuffer.length + Ch
                            .GetSize()) & 0xff00) >> 8);
                    Ch.SectionHeader.Size[2] = (byte) (((myCompress.outputBuffer.length + Ch
                            .GetSize()) & 0xff0000) >> 16);
                    Ch.SectionHeader.type = (byte) EFI_SECTION_COMPRESSION;

                    //
                    // Note: The compressName was not efsfective now. Using the
                    // EFI_STANDARD_COMPRSSION for compressType .
                    // That is follow old Genffsfile tools. Some code will be
                    // added for
                    // the different compressName;
                    //
                    Ch.UncompressLen = fileBuffer.length;
                    Ch.CompressType = EFI_STANDARD_COMPRESSION;

                    //
                    // Change header struct to byte buffer
                    //
                    byte[] headerBuffer = new byte[Ch.GetSize()];
                    Ch.StructToBuffer(headerBuffer);

                    //
                    // First add CompressHeader to Buffer, then add Compress
                    // data.
                    //
                    buffer.write(headerBuffer);
                    buffer.write(myCompress.outputBuffer);

                    //
                    // Buffer 4 Byte aligment
                    //
                    int size = Ch.GetSize() + myCompress.outputBuffer.length;

                    while ((size & 0x03) != 0) {
                        size++;
                        buffer.writeByte(0);
                    }
                }
            } catch (Exception e) {
                throw new BuildException("GenSection<SectionType=EFI_SECTION_COMPRESSION> to Buffer  failed!\n");
            } 
        } else {
            Section sect;
            Iterator sectionIter = this.sectFileList.iterator();
            while (sectionIter.hasNext()) {
                sect = (Section) sectionIter.next();
                try {
                    //
                    // The last section don't need 4 byte ffsAligment.
                    //
                    sect.toBuffer(buffer);
                } catch (Exception e) {
                    throw new BuildException(e.getMessage());
                }
            }
        }
    }
}
