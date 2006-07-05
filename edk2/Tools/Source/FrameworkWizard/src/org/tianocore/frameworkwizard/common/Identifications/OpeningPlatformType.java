/** @file
 
 The file is used to define opening platform type
  
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

import org.tianocore.PlatformSurfaceAreaDocument;

public class OpeningPlatformType extends OpeningFileType {
    //
    // Define class members
    //
    private PlatformSurfaceAreaDocument.PlatformSurfaceArea xmlFpd = null;
    
    public OpeningPlatformType() {
        
    }
    
    public OpeningPlatformType(Identification identification, PlatformSurfaceAreaDocument.PlatformSurfaceArea fpd) {
        super(identification);
        this.xmlFpd = fpd;
    }
    
    public OpeningPlatformType(Identification identification, PlatformSurfaceAreaDocument.PlatformSurfaceArea fpd, TreePath treePath) {
        super(identification, treePath);
        this.xmlFpd = fpd;
    }

    public PlatformSurfaceAreaDocument.PlatformSurfaceArea getXmlFpd() {
        return xmlFpd;
    }

    public void setXmlFpd(PlatformSurfaceAreaDocument.PlatformSurfaceArea xmlFpd) {
        this.xmlFpd = xmlFpd;
    }
}
