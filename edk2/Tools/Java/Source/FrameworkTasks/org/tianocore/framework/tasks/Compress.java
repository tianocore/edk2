/** @file
 Compress class.

 This class is to call CompressDll.dll to compress section.
 
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

/**

  This class is to call CompressDll.dll to compress section.
 
**/
public class Compress {
    byte[] inputBuffer;
    byte[] outputBuffer;
    int    size;

    static {
        String dllPath;

        dllPath = GenFfsFileTask.path;
        dllPath = dllPath + 
                  File.separator + 
                  "CompressDll.dll";

        System.load(dllPath);
    }

    /**
      CallCompress
    
      This function is to call the compressDll.dll to compress the contents in
      buffer.
      
      @param  inputBuffer       The input buffer.
      @param  size              The size of buffer in byte.
      @param  dllPath           The compressDll.dll path.
      @return                   The buffer contained the comrpessed input.
    **/
    public native byte[] CallCompress (byte[] inputBuffer, int size, String dllPath);

    /**
      Construct function
      
      This function is to initialize the class member and call the compress 
      function.
      
      @param inBuffer           The input buffer.         
      @param size               The size of buffer in byte.
    **/
    public Compress (byte[] inBuffer, int size){
        this.inputBuffer   = inBuffer;
        this.size          = size;        
        String path        = GenFfsFileTask.path;

        //
        //  Call Compress function.
        //
        this.outputBuffer  = CallCompress (
                                          this.inputBuffer, 
                                          this.size,
                                          path                          
                                          );
    }
}