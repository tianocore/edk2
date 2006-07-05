/** @file
 
 The file is used to save basic information of package
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.packaging;
import java.io.File;

import org.tianocore.frameworkwizard.common.Identifications.Identification;

public class PackageIdentification extends Identification{
    
    //
    // It is optional
    //
    private File spdFile;
    
    public PackageIdentification(String name, String guid, String version){
        super(name, guid, version);
    }
    
    public PackageIdentification(String name, String guid, String version, String path){
        super(name, guid, version, path);
    }
    
    public PackageIdentification(Identification id){
        super(id.getName(), id.getGuid(), id.getVersion(), id.getPath());
    }
    
    public PackageIdentification(String name, String guid, String version, File spdFile){
        super(name, guid, version);
        this.spdFile = spdFile;
    }
    
    public File getSpdFile() {
        return spdFile;
    }

    public String toString(){
        return "Package " + this.getName() + "[" + this.getGuid() + "]";
    }

    public void setSpdFile(File spdFile) {
        this.spdFile = spdFile;
    }
    
    public String getPackageDir(){
        return spdFile.getParent();
    }
}
