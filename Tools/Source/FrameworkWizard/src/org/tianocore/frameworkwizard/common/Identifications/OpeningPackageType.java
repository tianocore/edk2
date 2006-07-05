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

import javax.swing.tree.TreePath;

import org.tianocore.PackageSurfaceAreaDocument;

public class OpeningPackageType extends OpeningFileType {
    //
    // Define class members
    //
    private PackageSurfaceAreaDocument.PackageSurfaceArea xmlSpd = null;
    
    public OpeningPackageType() {
        
    }
    
    public OpeningPackageType(Identification identification, PackageSurfaceAreaDocument.PackageSurfaceArea spd) {
        super(identification);
        this.xmlSpd = spd;
    }
    
    public OpeningPackageType(Identification identification, PackageSurfaceAreaDocument.PackageSurfaceArea spd, TreePath treePath) {
        super(identification, treePath);
        this.xmlSpd = spd;
    }

    public PackageSurfaceAreaDocument.PackageSurfaceArea getXmlSpd() {
        return xmlSpd;
    }

    public void setXmlSpd(PackageSurfaceAreaDocument.PackageSurfaceArea xmlSpd) {
        this.xmlSpd = xmlSpd;
    }
}
