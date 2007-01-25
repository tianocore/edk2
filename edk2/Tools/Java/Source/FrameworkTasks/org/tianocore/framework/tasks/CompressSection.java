/** @file
 CompressSection class.

 CompressSection indicate that all section which in it should be compressed.
 
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
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.apache.tools.ant.BuildException;


/**
  CompressSection
  
  CompressSection indicate that all section which in it should be compressed. 
 
**/
public class CompressSection implements Section, FfsTypes {
    private int alignment = 0;
    //
    // The attribute of compressName.
    //
    private String compressName = "";
    //
    // The list contained the SectFile element.
    //
    private List<Section> sectList = new ArrayList<Section>();

    public static Object semaphore = new Object();
    /**
      toBuffer
      
      This function is to collect all sectFile and compress it , then output
      the result to buffer.
      
      @param Buffer     The point of output buffer
      
    **/
    public void toBuffer (DataOutputStream buffer){
        
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
            Iterator SectionIter = sectList.iterator();
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
            
            synchronized (semaphore) {
            //
            //  Call compress
            //
            byte[] fileBuffer = bo.toByteArray();
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
            //
            //  Delete temp file
            //
            //di.close();
            //compressOut.delete();
            }
                
        }
        catch (Exception e){
            throw new BuildException("compress.toBuffer failed!\n");
        }    
    }

    /**
      getCompressName
      
      This function is to get compressName.
      
      @return            The compressName.
    **/
    public String getCompressName() {
        return compressName;
    }

    /**
      setCompressName
      
      This function is to set compressName.
      
      @param compressName The string of compressName
    **/
    public void setCompressName(String compressName) {
        this.compressName = compressName;
    }

    /**
      addSectFile
      
      This function is to add sectFile element to SectList.
      
      @param sectFile    SectFile element which succeed from section class.
    **/
    public void addSectFile (SectFile sectFile) {
        sectList.add(sectFile);
            
    }    
    
    /**
      addTool
      
      This function is to add tool element to SectList.
      @param tool        Tool element which succeed from section class.
    **/
    public void addTool (Tool tool) {
        sectList.add(tool);
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
}