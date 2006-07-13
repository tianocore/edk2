/** @file

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.far;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.Set;

import org.apache.xmlbeans.XmlException;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;

public interface ManifestInterface {

    public void createManifest(List<PackageIdentification> pkgList, List<PlatformIdentification> plfList,
                               Set<String> fileFilter) throws Exception;

    public void setManifestFile(File manifestFile) throws Exception;

    public List<PackageIdentification> getPackageList() throws Exception;

    public List<PlatformIdentification> getPlatformList() throws Exception, IOException, XmlException;

    public List<FarFileItem> getPackageContents(PackageIdentification packageId);

    public String getPackageDefaultPath(PackageIdentification packageId);

}
