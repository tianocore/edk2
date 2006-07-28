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

public class PackageIdentification extends Identification{
    
    //
    // It is optional
    //
    private File spdFile;
    
    public PackageIdentification(String guid, String version){
        super(guid, version);
    }
    
    public PackageIdentification(String name, String guid, String version){
        super(name, guid, version);
    }
    
    public PackageIdentification(String name, String guid, String version, String spdFilename){
        super(name, guid, version);
        this.spdFile = new File(spdFilename);
    }
    
    public PackageIdentification(String name, String guid, String version, File spdFile){
        super(name, guid, version);
        this.spdFile = spdFile;
    }
    
    public void setSpdFile(File spdFile) {
        this.spdFile = spdFile;
    }

    public File getSpdFile() {
        return spdFile;
    }

    public String toString(){
        if (name == null) {
            GlobalData.refreshPackageIdentification(this);
        }
        if (version == null || version.trim().equalsIgnoreCase("")) {
            return "package [" + name + "]";
        }
        else {
            return "package [" + name + " " + version + "]";
        }
    }
    
    public String getPackageDir(){
        prepareSpdFile();
        return spdFile.getParent();
    }
    
    public String getPackageRelativeDir(){
        prepareSpdFile();
        String relativeDir =spdFile.getParent().substring(GlobalData.getWorkspacePath().length());
        if(relativeDir.startsWith("\\") || relativeDir.startsWith("/")) {
          relativeDir = relativeDir.substring(1);
        }
        return relativeDir;
    }
    
    private void prepareSpdFile(){
        if (spdFile == null) {
            GlobalData.refreshPackageIdentification(this);
        }
    }
    
    public String getName() {
        if (name == null) {
            GlobalData.refreshPackageIdentification(this);
        }
        return name;
    }
}
