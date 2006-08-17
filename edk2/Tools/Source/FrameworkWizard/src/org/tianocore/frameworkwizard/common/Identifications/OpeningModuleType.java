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
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;

public class OpeningModuleType extends OpeningFileType{
    //
    // Define class members
    //
    private ModuleSurfaceAreaDocument.ModuleSurfaceArea xmlMsa = null;
    
    private ModuleIdentification id = null;
    
    public OpeningModuleType() {
        
    }
    
    public OpeningModuleType(ModuleIdentification identification, ModuleSurfaceAreaDocument.ModuleSurfaceArea msa) {
        this.id = identification;
        this.xmlMsa = msa;
    }
    
    public OpeningModuleType(ModuleIdentification identification, ModuleSurfaceAreaDocument.ModuleSurfaceArea msa, TreePath treePath) {
        super(treePath);
        
        this.id = identification;
        this.xmlMsa = msa;
    }

    public ModuleSurfaceAreaDocument.ModuleSurfaceArea getXmlMsa() {
        return xmlMsa;
    }

    public void setXmlMsa(ModuleSurfaceAreaDocument.ModuleSurfaceArea xmlMsa) {
        this.xmlMsa = xmlMsa;
    }

    public ModuleIdentification getId() {
        return id;
    }

    public void setId(ModuleIdentification id) {
        this.id = id;
    }
}
