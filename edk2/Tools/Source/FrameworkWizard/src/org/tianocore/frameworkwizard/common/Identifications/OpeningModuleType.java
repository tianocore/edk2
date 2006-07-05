/** @file
 
 The file is used to define opening module type
  
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

import org.tianocore.ModuleSurfaceAreaDocument;

public class OpeningModuleType extends OpeningFileType{
    //
    // Define class members
    //
    private ModuleSurfaceAreaDocument.ModuleSurfaceArea xmlMsa = null;
    
    public OpeningModuleType() {
        
    }
    
    public OpeningModuleType(Identification identification, ModuleSurfaceAreaDocument.ModuleSurfaceArea msa) {
        super(identification);
        this.xmlMsa = msa;
    }
    
    public OpeningModuleType(Identification identification, ModuleSurfaceAreaDocument.ModuleSurfaceArea msa, TreePath treePath) {
        super(identification, treePath);
        this.xmlMsa = msa;
    }

    public ModuleSurfaceAreaDocument.ModuleSurfaceArea getXmlMsa() {
        return xmlMsa;
    }

    public void setXmlMsa(ModuleSurfaceAreaDocument.ModuleSurfaceArea xmlMsa) {
        this.xmlMsa = xmlMsa;
    }
}
