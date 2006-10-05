/** @file
This file is to define  PackageIdentification class.

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
  This class is used to identify a package. 

  @since GenBuild 1.0
**/
public class PackageIdentification extends Identification{
    
    //
    // SPD file
    //
    private File spdFile;
    
    /**
      @param guid Guid
      @param version Version
    **/
    public PackageIdentification(String guid, String version){
        super(guid, version);
    }
    
    /**
      @param name Name
      @param guid Guid
      @param version Version
    **/
    public PackageIdentification(String name, String guid, String version){
        super(name, guid, version);
    }
    
    /**
      @param name Name
      @param guid Guid
      @param version Version
      @param spdFilename SPD file name
    **/
    public PackageIdentification(String name, String guid, String version, String spdFilename){
        super(name, guid, version);
        this.spdFile = new File(spdFilename);
    }
    
    /**
      @param name Name
      @param guid Guid
      @param version Version
      @param spdFile SPD file
    **/
    public PackageIdentification(String name, String guid, String version, File spdFile){
        super(name, guid, version);
        this.spdFile = spdFile;
    }
    
    /**
      set SPD file.
      @param spdFile SPD file
    **/
    public void setSpdFile(File spdFile) {
        this.spdFile = spdFile;
    }

    /**
      get SPD file
      @return SPD file
    **/
    public File getSpdFile() {
        return spdFile;
    }

    public String toString(){
        if (version == null || version.trim().equalsIgnoreCase("")) {
            return "package [" + name + "]";
        }
        else {
            return "package [" + name + " " + version + "]";
        }
    }
    
    /**
      get package directory
      @return Package Directory
    **/
    public String getPackageDir(){
        return spdFile.getParent();
    }
    
    /**
      get package relative directory. 
      @return package relative directory
    **/
    public String getPackageRelativeDir(){
        String relativeDir =spdFile.getParent().substring(GlobalData.getWorkspacePath().length());
        if(relativeDir.startsWith("\\") || relativeDir.startsWith("/")) {
          relativeDir = relativeDir.substring(1);
        }
        return relativeDir;
    }
    
    public String getName() {
        return name;
    }
}
