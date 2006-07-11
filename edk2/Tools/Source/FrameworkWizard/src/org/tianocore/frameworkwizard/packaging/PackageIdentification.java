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

import org.tianocore.frameworkwizard.common.Identifications.Identification;

public class PackageIdentification extends Identification{
    
    public PackageIdentification(String name, String guid, String version){
        super(name, guid, version);
    }
    
    public PackageIdentification(String name, String guid, String version, String path){
        super(name, guid, version, path);
    }
    
    public PackageIdentification(Identification id){
        super(id.getName(), id.getGuid(), id.getVersion(), id.getPath());
    }
}
