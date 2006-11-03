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

import java.io.IOException;

import org.apache.xmlbeans.XmlException;
import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.OpenFile;
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
    
    public void reload() {
        if (this.id != null) {
            String path = id.getPath();
            if (path.length() > 0) {
                try {
                    this.xmlMsa = OpenFile.openMsaFile(id.getPath());
                } catch (IOException e) {
                    Log.err("Open Module Surface Area " + path, e.getMessage());
                } catch (XmlException e) {
                    Log.err("Open Module Surface Area " + path, e.getMessage());
                } catch (Exception e) {
                    Log.err("Open Module Surface Area " + path, "Invalid file type");
                }
            }
        }
    }
}
