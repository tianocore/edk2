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

public class PlatformIdentification extends Identification{
    
    private File fpdFile;
    
    public PlatformIdentification(String guid, String version){
        super(guid, version);
    }
    
    public PlatformIdentification(String name, String guid, String version){
        super(name, guid, version);
    }
    
    public PlatformIdentification(String name, String guid, String version, String fpdFilename){
        super(name, guid, version);
        this.fpdFile = new File(fpdFilename);
    }
    
    public PlatformIdentification(String name, String guid, String version, File fpdFile){
        super(name, guid, version);
        this.fpdFile = fpdFile;
    }
    
    public String toString(){
        return "Platform " + name + "["+guid+"]";
    }

    public void setFpdFile(File fpdFile) {
        this.fpdFile = fpdFile;
    }

    public File getFpdFile() {
        return fpdFile;
    }
    
    public String getRelativeFpdFile (){
        return fpdFile.getPath().substring(GlobalData.getWorkspacePath().length() + 1);
    }
    
    public String getPlatformRelativeDir(){
        return fpdFile.getParent().substring(GlobalData.getWorkspacePath().length() + 1);
    }
}