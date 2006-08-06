/** @file
  ModulePcdInfoFromFpd class.

  The interface parameter from <ModuleSA>'s Pcd information got from global data.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.pcd.entity;

import org.tianocore.PcdBuildDefinitionDocument;
import org.tianocore.PcdBuildDefinitionDocument.PcdBuildDefinition;
import org.apache.xmlbeans.XmlObject;

/**
   PCD build information in <ModuleSA> in FPD file.  
**/
public class ModulePcdInfoFromFpd {
    ///
    /// Usage identification for a module
    /// 
    public UsageIdentification   usageId;

    ///
    /// <ModuleSA>'s <PcdBuildDefinition> information.
    /// 
    public PcdBuildDefinition    pcdBuildDefinition;

    /**
       Construct function.

       @param usageId               The usage instance's identification
       @param pcdBuildDefinition    The <PcdBuildDefinition> information in <ModuleSA> in FPD file.

    **/
    public ModulePcdInfoFromFpd(UsageIdentification usageId,
                                PcdBuildDefinition  pcdBuildDefinition) {
        this.usageId            = usageId;
        this.pcdBuildDefinition = pcdBuildDefinition;
    }
}
