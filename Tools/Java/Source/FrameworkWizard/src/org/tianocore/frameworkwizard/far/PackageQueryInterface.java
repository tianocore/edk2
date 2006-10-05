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
import java.io.InputStream;
import java.util.List;

import org.tianocore.frameworkwizard.packaging.PackageIdentification;

public interface PackageQueryInterface {

    public PackageIdentification getPackageIdentification(File spdFile);

    public List<File> getPackageMsaList(File spdFile);

    public List<String> getPackageMsaList(InputStream spdInput);

    public List<PackageIdentification> getPackageDependencies(File spdFile);

    public List<PackageIdentification> getPackageDependencies(List<File> msaFiles);

    public List<PackageIdentification> getModuleDependencies(InputStream msaInput);

    public List<PackageIdentification> getModuleDependencies(File msaFile);

}
