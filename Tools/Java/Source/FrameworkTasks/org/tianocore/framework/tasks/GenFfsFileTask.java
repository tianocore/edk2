/** @file
 GenFfsFileTask class.

 GenFfsFileTaks is to generate ffs file.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.framework.tasks;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.tianocore.common.logger.EdkLog;

/**
  GenFfsFileTask
  
  GenFfsFileTaks is to generate ffs file.

**/
public class GenFfsFileTask extends Task implements EfiDefine, FfsTypes {
    /**
      *  GenFfsFile Task Class
      *  class member 
      *      -baseName               : module baseName
      *      -ffsFileGuid            : module Guid.
      *      -ffsFileType            : Ffs file type.  
      *      -ffsAttributeRecovery   : The file is required for recovery.
      *      -ffsAligment            : The file data alignment (0 if none required).  See FFS 
      *                                specification for supported alignments (0-7 are only possible 
      *                                 values).     *
      *      -ffsAttributeCheckSum   : The file data is checksummed.  If this is FALSE a 
      *                                value of 0x5A will be inserted in the file 
      *                                checksum field of the file header.     *
      *      -sectFileDir            : specifies the full path to the component build directory.
      *                                Required.
      *      -ffsAttrib              : Data recorde attribute added result.
      *      -sectionList            : List recorded all section elemet in task.
      */
    ///
    /// module baseName
    ///
    String  baseName        = "";
    ///
    /// 
    /// 
    String moduleType;
    ///
    /// module Guid
    ///
    String  ffsFileGuid        = "";
    ///
    /// Ffs file type
    ///
    String  ffsFileType        = "";
    ///
    /// ffsAttribHeaderExtension value is used to set the corresponding bit in 
    /// the output FFS file header 
    ///
    boolean ffsAttribHeaderExtension = false;
    ///
    /// ffsAttribTailPresent value is used to set the corresponding bit in the 
    /// output FFS file header
    ///
    boolean ffsAttribTailPresent     = false;
    ///
    /// ffsAttribRecovery value is used to set the corresponding bit in the 
    /// output FFS file header
    ///
    boolean ffsAttribRecovery        = false;
    ///
    /// ffsAttribDataAlignment value is used to set the corresponding bit in the output 
    /// FFS file header.The specified FFS alignment must be a value between 0 
    /// and 7 inclusive
    ///
    int     ffsAttribDataAlignment       = 0;
    ///
    /// ffsAttribChecksum value is used to set the corresponding bit in the 
    /// output FFS file header
    ///
    boolean FfsAttribChecksum        = false;
    ///
    /// Attribute is used to record the sum of all bit in the output FFS file.
    ///
    byte    attributes      = 0;
    ///
    /// The output directory of ffs file.
    ///
    String  outputDir       = "";
    ///
    /// List of section.
    ///
    List<Object> sectionList  = new ArrayList<Object>();

    ///
    /// The path of Framewor_Tools_Paht.
    ///
    static String path = "";  
    
    ///
    /// Gensection
    ///
    List<GenSectionTask> genSectList = new ArrayList<GenSectionTask>();
    
    /**
      execute
      
      GenFfsFileTask execute is to generate ffs file according to input section 
      dscriptive information.
    **/
    public void execute() throws BuildException {

        String            ffsSuffix = "";
        String            outputPath = "";
        
        //
        //  Get Fraemwork_Tools_Path
        //
        Project pj = this.getOwningTarget().getProject();
        path       = pj.getProperty("env.FRAMEWORK_TOOLS_PATH");

        //
        //  Check does the BaseName, Guid, FileType set value.
        //
        if (this.baseName.equals("")) {
            throw new BuildException ("Must set OutputFileBasename!\n");
        }

        if (this.ffsFileGuid.equals("")) {
            throw new BuildException ("Must set ffsFileGuid!\n");
        }

        if (this.ffsFileType.equals("")) {
            throw new BuildException ("Must set ffsFileType!\n");
        }
        //
        //  Create ffs file. File name = FfsFileGuid + BaseName + ffsSuffix.
        //  If outputDir's value was set,  file will output to the outputDir.
        //
        ffsSuffix = TypeToSuffix (this.moduleType);
        if (!this.outputDir.equals("")) {
            String temp;
            outputPath = this.outputDir;
            temp       = outputPath.replace('\\', File.separatorChar);
            outputPath = temp.replace('/', File.separatorChar);
            if (outputPath.charAt(outputPath.length()-1) != File.separatorChar) {
                outputPath = outputPath + File.separator;
            }

        }

        String ffsFilePath = outputPath + this.ffsFileGuid + '-' + this.baseName + ffsSuffix;
        File ffsFile = new File (ffsFilePath);
        try{
            genFfs(ffsFile);
        }catch (BuildException e){
            if (ffsFile != null && ffsFile.exists()){
                ffsFile.deleteOnExit();
            }
            throw new BuildException(e.getMessage());
            
        }
        
    }   

    /**
      addCompress
      
      This function is to add compress section to section list.
      @param compress   Section of compress   
    **/
    public void addCompress(CompressSection compress) {
        this.sectionList.add(compress);
    }

    /**
      addTool
      
      This function is to add tool section to section list.
      @param tool       Section of tool
    **/
    public void addTool(Tool tool) {
        this.sectionList.add(tool);
    }

    /**
      addSectionFile
      
      This function is to add sectFile section to section list.
      @param sectFile    Section of sectFile.
    **/
    public void addSectFile (SectFile sectFile) {
        this.sectionList.add(sectFile);   
    }

    /**
      getBaseName
      
      This function is to get basename
      
      @return              String of base name
    **/
    public String getBaseName() {
        return this.baseName;
    }

    /**
      setBaseName
      
      This function is to set base name.
      @param  baseName
    **/
    public void setBaseName(String baseName) {
        this.baseName = baseName.trim();
    }

    /**
      getFfsAligment
      
      This function is to get the ffsAligment
      @return  The value of ffsAligment.
    **/
    public int getFfsAttribDataAlignment() {
        return this.ffsAttribDataAlignment;
    }

    /**
      setFfsAligment
      
      This function is to set ffsAligment 
      @param  ffsAligment     The value of ffsAligment.
    **/
    public void setFfsAttribDataAlignment(String ffsAligment) {
        this.ffsAttribDataAlignment = stringToInt(ffsAligment.replaceAll(" ", "").toLowerCase());
        if (this.ffsAttribDataAlignment < 0 || this.ffsAttribDataAlignment > 7) {
            throw new BuildException ("FFS_ATTRIB_DATA_ALIGMENT must be an integer value from 0 through 7, inclusive");
        } else {
            attributes |= (((byte)this.ffsAttribDataAlignment) << 3);
        }
    }

    /**
      getFfsAttribCheckSum
      
      This function is to get ffsAttribCheckSum
      
      @return                      Value of ffsAttribChecksum 
    **/
    public boolean getFfsAttribChecksum() {
        return this.FfsAttribChecksum;
    }

    /**
      setFfsAttribChecksum
      
      This function is to set ffsAttribChecksum
      @param ffsAttributeCheckSum  Value of ffsAttribCheckSum
    **/
    public void setFfsAttribChecksum(boolean ffsAttributeCheckSum) {
        this.FfsAttribChecksum = ffsAttributeCheckSum;
        if (ffsAttributeCheckSum) {
            attributes |= FFS_ATTRIB_CHECKSUM;
        }
    }

    /**
      getFfsAttribRecovery
      
      This function is to get ffsAttribRecovery
      @return                        Value of ffsAttribRecovery
    **/
    public boolean getFfsAttribRecovery() {
        return this.ffsAttribRecovery;
    }

    /**
      setRecovery
      
      This function is to set ffsAttributeRecovery
      
      @param  ffsAttributeRecovery    Value of ffsAttributeRecovery
    **/
    public void setRecovery(boolean ffsAttributeRecovery) {
        this.ffsAttribRecovery = ffsAttributeRecovery;
        if (ffsAttributeRecovery) {
            attributes |= FFS_ATTRIB_RECOVERY;
        }
    }

    /**
      getFileGuid
      
      This function is to get fileGuid
      @return          Guid
    **/
    public String getFileGuid() {
        return this.ffsFileGuid;
    }

    /**
      setFileGuid
      
      This function is to set fileGuid
      @param ffsFileGuid    String of GUID
    **/
    public void setFileGuid(String ffsFileGuid) {
        this.ffsFileGuid = ffsFileGuid.trim();
    }

    /**
      getFfsFileType
      
      This function is to get ffsFileType.
      
      @return               value of ffsFileType
    **/
    public String getFfsFileType() {
        return this.ffsFileType;
    }

    /**
      setFfsFileType
      
      This function is to set ffsFileType.
      
      @param ffsFileType      
    **/
    public void setFfsFileType(String ffsFileType) {
        this.ffsFileType = ffsFileType.trim();
    }

    /**
      ffsAttribHeaderExtension
      
      This function is to get ffsAttribHeaderExtension
      
      @return             Value of ffsAttribHeaderExtension
    **/
    public boolean isFfsAttribHeaderExtension() {
        return this.ffsAttribHeaderExtension;
    }

    /**
      setHeaderExension
      
      This function is to set headerExtension
      @param headerExtension     Value of headerExension
    **/
    public void setHeaderExtension(boolean headerExtension) {
        this.ffsAttribHeaderExtension = headerExtension;
        if (headerExtension) {
            attributes |= FFS_ATTRIB_HEADER_EXTENSION;
        }
    }

    /**
      isFfsAttribTailPresent
      
      This function is to get ffsAttribTailPresent value.
      @return          Value of ffsAttribTailPresent.
    **/
    public boolean isFfsAttribTailPresent() {
        return this.ffsAttribTailPresent;
    }

    /**
      setFfsAttribTailPresent
      
      This function is to set ffsAttribTailPresent.
      @param tailPresent  Value of ffsAttribTailPresent.
    **/
    public void setFfsAttribTailPresent(boolean tailPresent) {
        this.ffsAttribTailPresent = tailPresent;
        if (tailPresent) {
            attributes |= FFS_ATTRIB_TAIL_PRESENT;
        }
    }   


    /**
      stringToGuid
      
      This function is to convert string to GUID.
     * @param GuidStr         String of GUID.
     * @param Guid            GUID form.
     */
    private void stringToGuid (String GuidStr, FfsHeader.FfsGuid Guid){   

        int i  = 0;
        int j  = 0;
        int k  = 0;
        char   [] charArry;
        String [] SplitStr;

        byte[] buffer = new byte[16];
        if (GuidStr.length()!=36) {
            throw new BuildException ("The GUID length [" + GuidStr.length() + "] is not correct!");
        }


        SplitStr = GuidStr.split("-");
        if (SplitStr.length != 5) {
            throw new BuildException ("The GUID format is not correct!");
        }



        for (i= 0; i < SplitStr.length; i++) {
            String str = SplitStr[i];         
            charArry   = str.toCharArray();

            for (j =0; j < (str.toCharArray().length)/2; j++) {

                buffer[k] = hexCharToByte (charArry[j*2]);        
                buffer[k] = (byte)( buffer[k]& 0x0f);       
                buffer[k] = (byte)((buffer[k]<< 4));
                buffer[k] = (byte)( buffer[k]& 0xf0);       
                buffer[k] = (byte)( buffer[k]|hexCharToByte(charArry[j*2+1]));
                k++;            
            }
        }
        Guid.bufferToStruct(buffer);
    }

    /**
      typeToSuffix
      
      This function is to get suffix of ffs file according to ffsFileType.
      
      @param  ffsFileType    ffsFileType
      @return                The suffix of ffs file
    **/
    private String TypeToSuffix (String ffsFileType){
        String[][] suffix = { { "BASE", ".FFS"},
                              { "SEC", ".SEC" }, { "PEI_CORE", ".PEI" }, 
                              { "PEIM", ".PEI" }, { "DXE_CORE", ".DXE" },
                              { "DXE_DRIVER", ".DXE" }, { "DXE_RUNTIME_DRIVER", ".DXE" }, 
                              { "DXE_SAL_DRIVER", ".DXE" }, { "DXE_SMM_DRIVER", ".DXE" }, 
                              { "TOOL", ".FFS" }, { "UEFI_DRIVER", ".DXE" },
                              { "UEFI_APPLICATION", ".APP" }, { "USER_DEFINED", ".FFS" } };

        for (int i = 0; i < suffix.length; i++) {
            if (suffix[i][0].equalsIgnoreCase(moduleType)) {
                return suffix[i][1];
            }
        }

        return ".FFS";
    }


    /**
      stringToType
      
      This function is to get ffsFileType integer value according to ffsFileType.
      @param   ffsFileType       String value of ffsFileType
      @return                    Integer value of ffsFileType.
    **/
    private byte stringToType (String ffsFileType){

        if (ffsFileType.equals("EFI_FV_FILETYPE_ALL")) {
            return(byte)EFI_FV_FILETYPE_ALL;
        }

        if (ffsFileType.equals("EFI_FV_FILETYPE_RAW")) {
            return(byte)EFI_FV_FILETYPE_RAW;
        }

        if (ffsFileType.equals("EFI_FV_FILETYPE_FREEFORM")) {
            return(byte)EFI_FV_FILETYPE_FREEFORM;
        }

        if (ffsFileType.equals("EFI_FV_FILETYPE_SECURITY_CORE")) {
            return(byte)EFI_FV_FILETYPE_SECURITY_CORE;
        }

        if (ffsFileType.equals("EFI_FV_FILETYPE_PEI_CORE")) {
            return(byte) EFI_FV_FILETYPE_PEI_CORE;
        }

        if (ffsFileType.equals("EFI_FV_FILETYPE_DXE_CORE")) {
            return(byte)EFI_FV_FILETYPE_DXE_CORE;
        }

        if (ffsFileType.equals("EFI_FV_FILETYPE_PEIM")) {
            return(byte)EFI_FV_FILETYPE_PEIM;
        }

        if (ffsFileType.equals("EFI_FV_FILETYPE_DRIVER")) {
            return(byte) EFI_FV_FILETYPE_DRIVER;
        }

        if (ffsFileType.equals("EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER")) {
            return(byte)EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER;
        }

        if (ffsFileType.equals("EFI_FV_FILETYPE_APPLICATION")) {
            return(byte)EFI_FV_FILETYPE_APPLICATION;
        }

        if (ffsFileType.equals("EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE")) {
            return(byte)EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE;
        }
        if (ffsFileType.equals("EFI_FV_FILETYPE_FFS_PAD")) {
            return(byte) EFI_FV_FILETYPE_FFS_PAD;
        }

        return -1;
    }



    /**
      calculateCheckSum8
      
      This function is to calculate the value needed for a valid UINT8 checksum
      @param  buffer  Byte buffer containing byte data of component.
      @param  size    Size of the buffer.
      @return         The 8 bit checksum value needed.
    **/
    private byte calculateChecksum8 (byte[] buffer, int size){
        return(byte) (0x100 - calculateSum8 (buffer, size));
    }


    /**
      calculateSum8
      
      This function is to calculate the UINT8 sum for the requested region.
      @param buffer   Byte buffer containing byte data of component
      @param size     Size of the buffer.
      @return         The 8 bit checksum value needed.
    **/ 
    private short calculateSum8 (byte[] buffer, int size){
        int   Index;
        byte  Sum;
        Sum   = 0;

        //
        // Perform the word sum for buffer
        //
        for (Index = 0; Index < size; Index++) {
            Sum = (byte) (Sum + buffer[Index]);     
        }

        return(byte) Sum; 
    }

    /**
      hexCharToByte
      
      This function is to convert hex character to byte
      
      @param     hexChar          hex character
      @return                     Byte which corresponding to the character.
    **/
    private byte hexCharToByte (char hexChar){
        switch (hexChar) {
        case '0':
            return(byte)0x00;
        case '1':
            return(byte)0x01;
        case '2':
            return(byte)0x02;
        case '3':
            return(byte)0x03;
        case '4':
            return(byte)0x04;
        case '5':
            return(byte)0x05;
        case '6':
            return(byte)0x06;
        case '7':
            return(byte)0x07;
        case '8':
            return(byte)0x08;
        case '9':
            return(byte)0x09;
        case 'a':
        case 'A':
            return(byte)0x0a;
        case 'b':
        case 'B':
            return(byte)0x0b;
        case 'c':
        case 'C':
            return(byte)0x0c;

        case 'd':
        case 'D':           
            return(byte)0x0d;

        case 'e':
        case 'E':
            return(byte)0x0e;
        case 'f':
        case 'F':
            return(byte)0x0f;

        default:
            return(byte)0xff;  
        }
    }

    /**
      adjustFileSize
      
      This function is used to adjusts file size to insure sectioned file is exactly the right length such
      that it ends on exactly the last byte of the last section.  ProcessScript()
      may have padded beyond the end of the last section out to a 4 byte boundary.
      This padding is stripped.  
      
      @param    buffer  Byte buffer contains a section stream
      @return           Corrected size of file.
    **/
    private int adjustFileSize (byte[] buffer){ 

        int orignalLen         = buffer.length;
        int adjustLen          = 0;
        int sectionPoint       = 0;
        int nextSectionPoint   = 0;
        int sectionLen         = 0;
        int totalLen           = 0;
        int firstSectionHeader = 0;


        firstSectionHeader = buffer[0]& 0xff;
        firstSectionHeader = ((buffer[1]&0xff)<<8) | firstSectionHeader;
        firstSectionHeader = ((buffer[2]&0xff)<<16)| firstSectionHeader;


        while (sectionPoint < buffer.length) {
            sectionLen = buffer[0 + sectionPoint]& 0xff;
            sectionLen = ((buffer[1 + sectionPoint]&0xff)<<8)| sectionLen;
            sectionLen = ((buffer[2 + sectionPoint]&0xff)<<16)| sectionLen; 
            totalLen   = totalLen + sectionLen;

            if (totalLen == orignalLen) {
                return totalLen;
            }

            sectionPoint     = sectionPoint + sectionLen;
            adjustLen        = sectionPoint;

            nextSectionPoint = (sectionPoint + 0x03) & (~0x03);
            totalLen         = totalLen + nextSectionPoint - sectionLen;
            sectionPoint     = nextSectionPoint;
        }
        return adjustLen;
    }

    /**
      getOutputDir
      
      This function is to get output directory.
      
      @return                Path of output directory.
    **/
    public String getOutputDir() {
        return outputDir;
    }

    /**
      setOutputDir
      
      This function is to set output directory.
      
      @param outputDir        The output direcotry.
    **/
    public void setOutputDir(String outputDir) {
        this.outputDir = outputDir;
    }

    /**
      getModuleTyp
      
      This function is to get string of module type.
     
      @return moduleType      The string of module type.
    **/
    public String getModuleType() {
        return this.moduleType;
    }

    /**
      setModuleType
      
      This function is to set moduleType.
      
      @param moduleType       The string of module type.
    **/
    public void setModuleType(String moduleType) {
        this.moduleType = moduleType;
    }
    
    /**
    Convert a string to a integer.
    
    @param     intString   The string representing a integer
    
    @retval    int     The value of integer represented by the
                       given string; -1 is returned if the format
                       of the string is wrong.
    **/
   private int stringToInt(String intString) {
       int value;
       int hexPrefixPos = intString.indexOf("0x");
       int radix = 10;
       String intStringNoPrefix;

       if (hexPrefixPos >= 0) {
           radix = 16;
           intStringNoPrefix = intString.substring(hexPrefixPos + 2, intString.length());
       } else {
           intStringNoPrefix = intString;
       }

       try {
           value = Integer.parseInt(intStringNoPrefix, radix);
       } catch (NumberFormatException e) {
           EdkLog.log(this, EdkLog.EDK_ERROR, "Incorrect format of int [" + intString + "]. -1 is assumed");
           return -1;
       }

       return value;
   }

   private void alignSection(DataOutputStream dataBuffer, int dataSize, int alignment) throws BuildException {
       if (alignment == 0) {
           return;
       }
       dataSize += 4; // take the section header into account
       int[] alignedBytes = {0, 16, 128, 512, 1024, 4096, 32768, 65536};
       int padSize = (alignedBytes[alignment] - dataSize) & (alignedBytes[alignment] - 1);
       if (padSize == 0) {
           //
           // already aligned
           // 
           return;
       }
       //
       // if the pad size is not times of 4, there must be something wrong in previous sections
       // 
       if (((4 - padSize) & (4 - 1)) != 0) {
           EdkLog.log(this, EdkLog.EDK_ERROR, "PAD section size must be 4-byte aligned (" + padSize + ")!");
           throw new BuildException ("Alignment can't be satisfied!");
       }
       byte[] pad = new byte[padSize];
       //
       // first three byte stores the section size
       // 
       pad[0] = (byte)(padSize & 0xff);
       pad[1] = (byte)((padSize >> 8) & 0xff);
       pad[2] = (byte)((padSize >> 16) & 0xff);
       //
       // the fourth byte are section type. use raw type (0x19)
       // 
       pad[3] = 0x19;
       try {
           dataBuffer.write(pad);
       } catch (Exception e) {
           throw new BuildException(e.getMessage());
       }
   }

   /**
     genFfs
     
     This function is to generate FFS file.
     
      @param ffsFile          Name of FFS file.
      @param isOrg            Flag to indicate generate ORG ffs file or not.
   **/
    private void genFfs(File ffsFile) throws BuildException {
        Section           sect;
        int               fileSize;
        int               fileDataSize;
        FfsHeader         ffsHeader = new FfsHeader();  
        FfsHeader         orgFfsHeader = new FfsHeader();
           
        EdkLog.log(this, EdkLog.EDK_INFO, ffsFile.getName());
      
        try {
            //
            //  Create file output stream -- dataBuffer.
            //
            FileOutputStream dataFs     = new FileOutputStream (ffsFile.getAbsolutePath());
            DataOutputStream dataBuffer = new DataOutputStream (dataFs);
            
            //
            //  Search SectionList find earch section and call it's 
            //  ToBuffer function.
            //
            Iterator sectionIter = this.sectionList.iterator();
            while (sectionIter.hasNext()) {
                sect = (Section)sectionIter.next(); 

                try {
                    int alignment = sect.getAlignment();
                    if (this.ffsAttribDataAlignment < alignment) {
                        this.ffsAttribDataAlignment = alignment;
                    }
                    alignSection(dataBuffer, dataBuffer.size(), alignment);
                    //
                    //  The last section don't need 4 byte ffsAligment.
                    //
                    sect.toBuffer((DataOutputStream)dataBuffer);
                } catch (Exception e) {
                    if (dataBuffer != null){
                        dataBuffer.close();
                    }
                    throw new BuildException (e.getMessage());
                }
            }
            dataBuffer.close();
        } catch (Exception e) {
            throw new BuildException (e.getMessage());
        }

        //
        //  Creat Ffs file header
        //
        try {

            //
            //  create input stream to read file data
            //
            byte[] fileBuffer  = new byte[(int)ffsFile.length()];
            FileInputStream fi = new FileInputStream (ffsFile.getAbsolutePath());
            DataInputStream di = new DataInputStream (fi);
            di.read(fileBuffer);
            di.close();
            
            //
            //  Add GUID to header struct
            //
            if (this.ffsFileGuid != null) {
                stringToGuid (this.ffsFileGuid, ffsHeader.name);
            }

            //
            // because we may have changed the ffsAttribDataAlignment, we need to refresh attributes
            // 
            this.attributes &= ~(((byte)7) << 3);
            this.attributes |= (((byte)this.ffsAttribDataAlignment) << 3);

            ffsHeader.ffsAttributes = this.attributes;
            if ((ffsHeader.fileType = stringToType(this.ffsFileType))== -1) {
                throw new BuildException ("FFS_FILE_TYPE unknow!\n");
            }
            
            //
            // Copy ffsHeader.ffsAttribute and fileType to orgFfsHeader.ffsAttribute
            // and fileType
            //            
            orgFfsHeader.ffsAttributes = ffsHeader.ffsAttributes;
            orgFfsHeader.fileType      = ffsHeader.fileType;
            
            //
            //  Adjust file size. The function is used to tripe the last 
            //  section padding of 4 binary boundary. 
            //  
            //
            if (ffsHeader.fileType != EFI_FV_FILETYPE_RAW) {

                fileDataSize = adjustFileSize (fileBuffer);
            } else {
                fileDataSize = fileBuffer.length;
            }

            //
            //  1. add header size to file size
            //
            fileSize = fileDataSize + ffsHeader.getSize();

            if ((ffsHeader.ffsAttributes & FFS_ATTRIB_TAIL_PRESENT) != 0) {
                if (ffsHeader.fileType == EFI_FV_FILETYPE_FFS_PAD) {

                    throw new BuildException (
                                             "FFS_ATTRIB_TAIL_PRESENT=TRUE is " +
                                             "invalid for PAD files"
                                             );
                }
                if (fileSize == ffsHeader.getSize()) {
                    throw new BuildException (
                                             "FFS_ATTRIB_TAIL_PRESENT=TRUE is " +
                                             "invalid for 0-length files"
                                             );            
                }
                fileSize = fileSize + 2;
            }

            //
            //  2. set file size to header struct
            //
            ffsHeader.ffsFileSize[0] = (byte)(fileSize & 0x00FF);
            ffsHeader.ffsFileSize[1] = (byte)((fileSize & 0x00FF00)>>8);
            ffsHeader.ffsFileSize[2] = (byte)(((int)fileSize & 0xFF0000)>>16);
            
            //
            //  Fill in checksums and state, these must be zero for checksumming
            //
            ffsHeader.integrityCheck.header = calculateChecksum8 (
                                                                 ffsHeader.structToBuffer(),
                                                                 ffsHeader.getSize()
                                                                 );
            
            if ((this.attributes & FFS_ATTRIB_CHECKSUM) != 0) {
                if ((this.attributes & FFS_ATTRIB_TAIL_PRESENT) != 0) {
                    ffsHeader.integrityCheck.file = calculateChecksum8 (
                                                                       fileBuffer, 
                                                                       fileDataSize
                                                                       );
                } else {
                    ffsHeader.integrityCheck.file = calculateChecksum8 (
                                                                       fileBuffer,
                                                                       fileDataSize
                                                                       );
                }
            } else {
                ffsHeader.integrityCheck.file = FFS_FIXED_CHECKSUM;
                orgFfsHeader.integrityCheck.file = FFS_FIXED_CHECKSUM;
            }

            //
            //   Set the state now. Spec says the checksum assumes the state is 0.
            //
            ffsHeader.ffsState = EFI_FILE_HEADER_CONSTRUCTION | 
                                 EFI_FILE_HEADER_VALID | 
                                 EFI_FILE_DATA_VALID;
            
            //
            // create output stream to first write header data in file, then write sect data in file.
            //
            FileOutputStream headerFfs = new FileOutputStream (ffsFile.getAbsolutePath());
            DataOutputStream ffsBuffer = new DataOutputStream (headerFfs);
            
            //
            //  Add header struct and file data to FFS file
            //
            ffsBuffer.write(ffsHeader.structToBuffer());
            ffsBuffer.write(fileBuffer, 0, fileDataSize);
            


            //
            //  If there is a tail, then set it
            //
            if ((this.attributes & FFS_ATTRIB_TAIL_PRESENT) != 0) {
                short tailValue ;
                byte [] tailByte = new byte[2];

                //
                //  reverse tailvalue , integritycheck.file as hight byte, and 
                //  integritycheck.header as low byte.
                //
                tailValue = (short)(ffsHeader.integrityCheck.header & 0xff);
                tailValue = (short)((tailValue) | ((ffsHeader.integrityCheck.file << 8) & 0xff00)); 
                tailValue = (short)~tailValue;

                //
                //  Change short to byte[2]
                //
                tailByte[0] = (byte)(tailValue & 0xff);
                tailByte[1] = (byte)((tailValue & 0xff00)>>8);  
                ffsBuffer.write(tailByte[0]);
                ffsBuffer.write(tailByte[1]);

            }

            //
            //  close output stream. Note if don't close output stream 
            //  the buffer can't be rewritten to file. 
            //
            ffsBuffer.close();
        } catch (Exception e) {
            EdkLog.log(this, EdkLog.EDK_ERROR, "genffsfile failed!");
            throw new BuildException (e.getMessage());
        }

    }
    
    /**
      addGenSection
      
      This function is to add gensection instance to list
      
      @param task    Instance of GenSectionTask
    **/
    public void addGenSection (GenSectionTask task){
        this.sectionList.add(task);
    }
}
