/** @file
 FfsHeader 

 FfsHeader class describe the struct of Ffs file header.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.framework.tasks;

import org.apache.tools.ant.BuildException;

/**
  FfsHeader
  
  FfsHeader class describe the struct of Ffs file header.
**/
public class FfsHeader {

    /**
      FfsGuid
      
      FfsGuid is interal class of FfsHeader, it describe the struct of Guid.
    **/
    public class FfsGuid {

        int    data1      = 0;
        short  data2      = 0;
        short  data3      = 0;
        byte[] data4      = new byte[8];        
        byte[] dataBuffer = new byte[16];

        /**
          bufferToStruct 
          
          This function is to convert GUID to ffsGuid class member.
          
          @param   dataBuffer   Buffer contained the GUID value in byte.
                                For example: if the input string as : "A6F691AC
                                31C8 4444 854C E2C1A6950F92"
                                Then Data1: AC91F6A6
                                Data2: C831
                                Data3: 4444
                                Data4: 4C85E2C1A6950F92
        **/     
        public void bufferToStruct (byte[] dataBuffer){
            if (dataBuffer.length != 16) {
                throw new BuildException ("Buffer is not sized [" + dataBuffer.length + "] for data type, GUID!");
            }

            data1 = (int)(dataBuffer[3]& 0xff);
            data1 = data1 << 8;     
            data1 = (int)data1 | (dataBuffer[2]& 0xff);         
            data1 = ((data1 << 8) & 0xffff00)   | (dataBuffer[1]& 0xff);
            data1 = ((data1 << 8) & 0xffffff00) | (dataBuffer[0]& 0xff);


            data2 = (short) (dataBuffer[5] & 0xff);
            data2 = (short)((data2 << 8) | (dataBuffer[4]& 0xff));

            data3 = (short)(dataBuffer[7] & 0xff);
            data3 = (short)((data3 << 8) | (dataBuffer[6] & 0xff));

            for (int i = 0; i < 8; i++) {
                data4[i] = dataBuffer[i+8];
            }

        }

        /**
          structToBuffer
          
          This function is to store ffsHeader class member to buffer.
          
          @return        Byte buffer which contained the ffsHeader class member
        **/
        public byte[] structToBuffer (){

            byte[] buffer = new byte [16];  

            buffer[3] = (byte)(data1 & 0x000000ff);
            buffer[2] = (byte)((data1 & 0x0000ff00)>> 8);
            buffer[1] = (byte)((data1 & 0x00ff0000)>> 16); 
            buffer[0] = (byte)((data1 & 0xff000000)>> 24);

            buffer[5] = (byte)(data2 & 0x00ff);
            buffer[4] = (byte)((data2 & 0xff00)>> 8);

            buffer[7] = (byte)(data3 & 0x00ff);
            buffer[6] = (byte)((data3 & 0xff00)>> 8);

            for (int i = 8; i < 16; i++) {
                buffer[i] = data4[i-8];
            }               
            return buffer;
        }


    }

    /**
      integrityCheckSum
      
      This class is used to record the struct of checksum.
    **/
    public class IntegrityCheckSum {
        byte header;
        byte file;
    }

    ///
    /// Guid
    ///
    FfsGuid name = new FfsGuid();

    ///
    /// CheckSum
    ///
    IntegrityCheckSum integrityCheck = new IntegrityCheckSum();

    ///
    /// File type
    ///
    byte   fileType;
    ///
    /// Ffs attributes.
    ///
    byte   ffsAttributes;
    ///
    /// Ffs file size
    ///
    byte[] ffsFileSize = new byte[3];
    ///
    /// Ffs state.
    ///
    byte   ffsState;

    /**
      structToBuffer
      
      This function is to store FfsHeader class member to buffer.
      
      @return   Byte buffer which contained the FfsHeader class member.
    **/
    public byte[] structToBuffer () {
        int i;
        byte[] buffer1;         
        byte[] buffer = new byte[24];
        buffer1       = name.structToBuffer();

        for (i = 0; i < 16; i++) {
            buffer[i] = buffer1[i];
        }

        buffer[16] = integrityCheck.header;
        buffer[17] = integrityCheck.file;
        buffer[18] = fileType;
        buffer[19] = ffsAttributes;

        for (i=20; i < 23; i++) {
            buffer[i] = ffsFileSize[i-20];
        }

        buffer[23] = ffsState;
        return buffer;
    }

    /**
      getSize
      
      This function is to get the size of FfsHeader in byte.
      
      @return          The size of FfsHeader.
    **/
    public int getSize(){
        return 24;
    }
}
