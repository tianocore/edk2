/** @file
 CompressHeader class.

 This class is to generate the compressed section header.
 
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
  
  Internal class: This class is to generate the compressed section header.

**/
public class CompressHeader {
    
    /**
      CommonSectionHeader
      
      This class define the compressed header structor.
    
    **/
    public class CommonSectionHeader {
        byte[] Size = new byte[3];
        byte   type;
    }
    
    ///
    /// Section header.
    ///
    public CommonSectionHeader SectionHeader = new CommonSectionHeader();
    
    ///
    /// Length of uncompress section in byte.
    ///
    public int                 UncompressLen;
    /// 
    /// Compress type.
    ///
    public byte                CompressType;
    
    ///
    /// The size of compress header in byte. 
    ///
    public int GetSize (){
        return 9;
    }
    
    ///
    /// Write class member to buffer. 
    ///
    public void StructToBuffer (byte[] Buffer){
        if (Buffer.length != GetSize()) {
            throw new BuildException ("CompressHeader Buffer size is not correct!");
        }
        for (int i = 0; i < 3; i++){
            Buffer[i] = SectionHeader.Size[i];
        }
        Buffer[3] = SectionHeader.type;
        Buffer[4] = (byte)(UncompressLen  & 0xff);
        Buffer[5] = (byte)((UncompressLen & 0xff00)>>8);
        Buffer[6] = (byte)((UncompressLen & 0xff0000)>>16);
        Buffer[7] = (byte)((UncompressLen & 0xff000000)>>24);
        Buffer[8] = CompressType;
    }
    
}