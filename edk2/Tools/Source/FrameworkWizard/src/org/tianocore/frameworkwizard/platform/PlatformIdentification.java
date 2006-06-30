/** @file
 
 The file is used to save basic information of platform
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.platform;

import java.io.File;

import org.tianocore.frameworkwizard.common.Identification;

public class PlatformIdentification extends Identification{
    
    private File fpdFile;
    
    public PlatformIdentification(String name, String guid, String version, String path){
        super(name, guid, version, path);
    }
    
    public PlatformIdentification(String name, String guid, String version, File fpdFile){
        super(name, guid, version);
        this.fpdFile = fpdFile;
    }
    
    public PlatformIdentification(Identification id){
        super(id.getName(), id.getGuid(), id.getVersion(), id.getPath());
    }
    
    public String toString(){
        return "Platform " + this.getName() + "[" + this.getGuid() + "]";
    }

    public void setFpdFile(File fpdFile) {
        this.fpdFile = fpdFile;
    }

    public File getFpdFile() {
        return fpdFile;
    }
}