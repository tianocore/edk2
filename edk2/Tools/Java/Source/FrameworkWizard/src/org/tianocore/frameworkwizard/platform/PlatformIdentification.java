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

import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.Identification;
import org.tianocore.frameworkwizard.workspace.Workspace;

public class PlatformIdentification extends Identification{

    public PlatformIdentification(String name, String guid, String version, String path){
        super(name, guid, version, path);
    }
    
    public PlatformIdentification(Identification id){
        super(id.getName(), id.getGuid(), id.getVersion(), id.getPath());
    }
    
    public File getFpdFile(){
      File fpdFile = new File(this.getPath());
      return fpdFile;
  }
    
    public String toString() {
      return getName() + " " + getVersion() + " [" + Tools.getRelativePath(getFpdFile().getPath(), Workspace.getCurrentWorkspace()) + "]";
    }
    
    public boolean equals(String platformGuid, String platformVersion) {
        boolean b = false;
        if (this.getGuid().equals(platformGuid)) {
            b = true;
            //
            // Check Version
            //
            if (platformVersion != null) {
                if (!Tools.isEmpty(platformVersion)) {
                    if (!platformVersion.equals(this.getVersion())) {
                        b = false;
                    }
                }
            }
        }
        return b;
    }
}