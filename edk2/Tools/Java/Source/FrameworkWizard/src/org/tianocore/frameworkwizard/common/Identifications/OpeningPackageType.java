/** @file
 
 The file is used to define opening package type
  
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.common.Identifications;

import java.io.IOException;

import org.apache.xmlbeans.XmlException;
import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.OpenFile;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;

public class OpeningPackageType extends OpeningFileType {
    //
    // Define class members
    //
    private PackageSurfaceAreaDocument.PackageSurfaceArea xmlSpd = null;
    
    private PackageIdentification id = null;
    
    public OpeningPackageType() {
        
    }
    
    public OpeningPackageType(PackageIdentification identification, PackageSurfaceAreaDocument.PackageSurfaceArea spd) {
        this.id = identification;
        this.xmlSpd = spd;
    }

    public PackageSurfaceAreaDocument.PackageSurfaceArea getXmlSpd() {
        return xmlSpd;
    }

    public void setXmlSpd(PackageSurfaceAreaDocument.PackageSurfaceArea xmlSpd) {
        this.xmlSpd = xmlSpd;
    }

    public PackageIdentification getId() {
        return id;
    }

    public void setId(PackageIdentification id) {
        this.id = id;
    }
    
    public void reload() {
        if (this.id != null) {
            String path = id.getPath();
            if (path.length() > 0) {
                try {
                    this.xmlSpd = OpenFile.openSpdFile(id.getPath());
                } catch (IOException e) {
                    Log.err("Open Package Surface Area " + path, e.getMessage());
                } catch (XmlException e) {
                    Log.err("Open Package Surface Area " + path, e.getMessage());
                } catch (Exception e) {
                    Log.err("Open Package Surface Area " + path, "Invalid file type");
                }
            }
        }
    }
}
