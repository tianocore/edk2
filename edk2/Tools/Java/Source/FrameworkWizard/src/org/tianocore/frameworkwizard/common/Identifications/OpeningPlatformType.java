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

import java.io.IOException;

import org.apache.xmlbeans.XmlException;
import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.OpenFile;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;

public class OpeningPlatformType extends OpeningFileType {
    //
    // Define class members
    //
    private PlatformSurfaceAreaDocument.PlatformSurfaceArea xmlFpd = null;
    
    private PlatformIdentification id = null;
    
    public OpeningPlatformType() {
        
    }
    
    public OpeningPlatformType(PlatformIdentification identification, PlatformSurfaceAreaDocument.PlatformSurfaceArea fpd) {
        this.id = identification;
        this.xmlFpd = fpd;
    }
    
    public PlatformSurfaceAreaDocument.PlatformSurfaceArea getXmlFpd() {
        return xmlFpd;
    }

    public void setXmlFpd(PlatformSurfaceAreaDocument.PlatformSurfaceArea xmlFpd) {
        this.xmlFpd = xmlFpd;
    }

    public PlatformIdentification getId() {
        return id;
    }

    public void setId(PlatformIdentification id) {
        this.id = id;
    }
    
    public void reload() {
        if (this.id != null) {
            String path = id.getPath();
            if (path.length() > 0) {
                try {
                    this.xmlFpd = OpenFile.openFpdFile(id.getPath());
                } catch (IOException e) {
                    Log.err("Open Platform Surface Area " + path, e.getMessage());
                } catch (XmlException e) {
                    Log.err("Open Platform Surface Area " + path, e.getMessage());
                } catch (Exception e) {
                    Log.err("Open Platform Surface Area " + path, "Invalid file type");
                }
            }
        }
    }
}
