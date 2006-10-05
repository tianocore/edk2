/** @file
This file is to define  PlatformIdentification class.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

package org.tianocore.build.id;
import java.io.File;

import org.tianocore.build.global.GlobalData;

/**
  This class is used to identify a platform. 

  @since GenBuild 1.0
**/
public class PlatformIdentification extends Identification{
    
    ///
    /// FPD file
    ///
    private File fpdFile;
    
    /**
      @param guid Guid
      @param version Version
    **/
    public PlatformIdentification(String guid, String version){
        super(guid, version);
    }
    
    /**
      @param name Name
      @param guid Guid
      @param version Version
    **/
    public PlatformIdentification(String name, String guid, String version){
        super(name, guid, version);
    }
    
    /**
      @param name Name
      @param guid Guid
      @param version Version
      @param fpdFilename Fpd File Name
    **/
    public PlatformIdentification(String name, String guid, String version, String fpdFilename){
        super(name, guid, version);
        this.fpdFile = new File(fpdFilename);
    }
    
    /**
      @param name Name
      @param guid Guid
      @param version Version
      @param fpdFile Fpd File
    **/
    public PlatformIdentification(String name, String guid, String version, File fpdFile){
        super(name, guid, version);
        this.fpdFile = fpdFile;
    }
    
    public String toString(){
        return "Platform " + name + "["+guid+"]";
    }

    /**
     Set FPD file. 
      @param fpdFile FPD File
    **/
    public void setFpdFile(File fpdFile) {
        this.fpdFile = fpdFile;
    }

    /**
      Get FPD file. 
      @return Fpd File
    **/
    public File getFpdFile() {
        return fpdFile;
    }
    
    /**
      Get FPD relative file to workspace. 
      @return Fpd Relative file. 
    **/
    public String getRelativeFpdFile (){
        String relativeDir = fpdFile.getPath().substring(GlobalData.getWorkspacePath().length());
        if(relativeDir.startsWith("\\") || relativeDir.startsWith("/")) {
            relativeDir = relativeDir.substring(1);
        }
        return relativeDir;
    }
    
    /**
      Get Platform relative directory to workspace. 
      @return Platform relative directory
    **/
    public String getPlatformRelativeDir(){
        String relativeDir = fpdFile.getParent().substring(GlobalData.getWorkspacePath().length());
        if(relativeDir.startsWith("\\") || relativeDir.startsWith("/")) {
            relativeDir = relativeDir.substring(1);
        }
        return relativeDir;
    }
}