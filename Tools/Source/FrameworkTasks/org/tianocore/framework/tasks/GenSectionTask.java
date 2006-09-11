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

public class GenSectionTask extends Task implements EfiDefine, Section,FfsTypes {
    ///
    /// inputfile name
    ///
    private String inputFile = "";
    ///
    /// 
    /// 
    private String inputFileName = "";
    ///
    /// outputfile name
    ///
    private String outputFile = "";
    ///
    /// section type
    ///
    private String sectionType = "";
    ///
    /// version number
    ///
    private String versionNum = "";
    ///
    /// interface string
    ///
    private String interfaceString = "";
    ///
    /// Section file list
    ///
    private List<Section> sectFileList = new ArrayList<Section>();
   
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
            command = "GenSection";
        } else {
            command = path + "/" + "GenSection";
        }
        //
        // argument of tools
        //
        String argument = inputFile + outputFile + " -s "+ sectionType + versionNum
                + interfaceString;
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

            EdkLog.log(this, EdkLog.EDK_INFO, inputFileName);
            EdkLog.log(this, EdkLog.EDK_DEBUG, Commandline.toString(cmdline.getCommandline()));
            revl = runner.execute();
            if (EFI_SUCCESS == revl) {
                log("GenSection succeeded!", Project.MSG_VERBOSE);
            } else {
                //
                // command execution fail
                //
                log("ERROR = " + Integer.toHexString(revl));
                throw new BuildException("GenSection failed!");
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
        return this.inputFile;
    }

    /**
      setInputFile
      
      This function is to set class member "inputFile".
      
      @param inputFile            name of input file
    **/
    public void setInputFile(String inputFile) {
        this.inputFileName = (new File(inputFile)).getName();
        this.inputFile = " -i " + inputFile;
    }

    /**
      getOutputFile
      
      This function is to get class member "outputFile".
      
      @return                      name of output file
    **/
    public String getOutputFile() {
        return this.outputFile;
    }

    /**
      setOutputfile
      
      This function is to set class member "outputFile".
      @param  outputFile            name of output file
    **/
    public void setOutputfile(String outputFile) {
        this.outputFile = " -o " + outputFile;
    }

    /**
      getSectionType
      
      This function is to get class member "sectionType".
      
      @return                        sectoin type
    **/
    public String getSectionType() {
        return this.sectionType;
    }

    /**
      setSectionType
      
      This function is to set class member "sectionType".
      
      @param sectionType              section type
    **/
    public void setSectionType(String sectionType) {
        this.sectionType = sectionType;
    }

    /**
      getVersionNum
      
      This function is to get class member "versionNum".
      @return                         version number
    **/
    public String getVersionNum() {
        return this.versionNum;
    }

    /**
      setVersionNume
      
      This function is to set class member "versionNum".
      @param versionNum               version number
    **/
    public void setVersionNum(String versionNum) {
        this.versionNum = " -v " + versionNum;
    }

    /**
      getInterfaceString
      
      This function is to get class member "interfaceString".
      @return                         interface string
    **/
    public String getInterfaceString() {
        return this.interfaceString;
    }

    /**
      setInterfaceString
      
      This funcion is to set class member "interfaceString".
      @param interfaceString            interface string
    **/
    public void setInterfaceString(String interfaceString) {
        this.interfaceString = " -a " + "\"" + interfaceString + "\"";
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
    }
    
    /**
      addGenSection
      
      This function is to add GenSectin element to list
      @param task         Instance of genSection
    **/
    public void addGenSection(GenSectionTask task){
        this.sectFileList.add(task);
    }
    
    public void toBuffer(DataOutputStream buffer){
        //
        //  Search SectionList find earch section and call it's 
        //  ToBuffer function.
        //
        if (this.sectionType.equalsIgnoreCase("EFI_SECTION_COMPRESSION")){
            Section    sect;
            
            //
            //  Get section file in compress node.
            //
            try{
                ByteArrayOutputStream bo = new ByteArrayOutputStream ();
                DataOutputStream Do = new DataOutputStream (bo);
                
                //
                //  Get each section which under the compress {};
                //  And add it is contains to File;
                //
                Iterator SectionIter = this.sectFileList.iterator();
                while (SectionIter.hasNext()){
                    sect = (Section)SectionIter.next();
                    
                    //
                    //  Call each section class's toBuffer function.
                    //
                    try {
                        sect.toBuffer(Do);
                    }
                    catch (BuildException e) {
                        System.out.print(e.getMessage());
                        throw new BuildException ("Compress.toBuffer failed at section");
                    }    
                                
                }
                Do.close();    
                
                //
                //  Call compress
                //
                byte[] fileBuffer = bo.toByteArray();
                
                synchronized (CompressSection.semaphore) {
                Compress myCompress = new Compress(fileBuffer, fileBuffer.length);            
                
                //
                //  Add Compress header
                //
                CompressHeader Ch        = new CompressHeader();
                Ch.SectionHeader.Size[0] = (byte)((myCompress.outputBuffer.length +
                                                   Ch.GetSize()) &
                                                   0xff
                                                  );
                Ch.SectionHeader.Size[1] = (byte)(((myCompress.outputBuffer.length + 
                                                    Ch.GetSize())&
                                                   0xff00) >> 8
                                                  );
                Ch.SectionHeader.Size[2] = (byte)(((myCompress.outputBuffer.length + 
                                                    Ch.GetSize()) & 
                                                   0xff0000) >> 16
                                                 );
                Ch.SectionHeader.type    = (byte) EFI_SECTION_COMPRESSION;
                
                //
                //  Note: The compressName was not efsfective now. Using the
                //  EFI_STANDARD_COMPRSSION for compressType .
                //  That is follow old Genffsfile tools. Some code will be added for 
                //  the different compressName;
                //
                Ch.UncompressLen         = fileBuffer.length;
                Ch.CompressType          = EFI_STANDARD_COMPRESSION; 
                
                //
                //  Change header struct to byte buffer
                //
                byte [] headerBuffer = new byte[Ch.GetSize()];
                Ch.StructToBuffer(headerBuffer);
                
                //
                //  First add CompressHeader to Buffer, then add Compress data.
                //
                buffer.write (headerBuffer);
                buffer.write(myCompress.outputBuffer);            
                    
                //
                //  Buffer 4 Byte aligment 
                //
                int size = Ch.GetSize() + myCompress.outputBuffer.length;
                
                while ((size & 0x03) != 0){
                    size ++;
                    buffer.writeByte(0);
                }
                }
            }
            catch (Exception e){
                throw new BuildException("compress.toBuffer failed!\n");
            }    
        } else {
            Section sect;
            Iterator sectionIter = this.sectFileList.iterator();
            while (sectionIter.hasNext()) {
                sect = (Section)sectionIter.next(); 
                try {
                    //
                    //  The last section don't need 4 byte ffsAligment.
                    //
                    sect.toBuffer(buffer);
                } catch (Exception e) {
                    throw new BuildException (e.getMessage());
                }
            }
        }
    }
}
