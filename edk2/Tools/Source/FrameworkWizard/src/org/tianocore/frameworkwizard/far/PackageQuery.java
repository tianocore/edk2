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
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import org.apache.xmlbeans.XmlObject;
import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.PackageDependenciesDocument;
import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.frameworkwizard.common.OpenFile;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

public class PackageQuery implements PackageQueryInterface {

    public PackageIdentification getPackageIdentification(File spdFile) {
        PackageIdentification packageId = null;
        try {
            String path = spdFile.getPath();
            packageId = Tools.getId(path, OpenFile.openSpdFile(path));
        } catch (Exception e) {
            e.printStackTrace();
        }
        return packageId;
    }

    public List<String> getPackageMsaList(InputStream spdInput) {
        List<String> result = new ArrayList<String>();
        try {
            PackageSurfaceAreaDocument spd = (PackageSurfaceAreaDocument) XmlObject.Factory.parse(spdInput);
            result = spd.getPackageSurfaceArea().getMsaFiles().getFilenameList();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return result;
    }

    public List<PackageIdentification> getModuleDependencies(InputStream msaInput) {
        List<PackageIdentification> result = new ArrayList<PackageIdentification>();
        try {
            ModuleSurfaceAreaDocument msa = (ModuleSurfaceAreaDocument) XmlObject.Factory.parse(msaInput);
            ModuleSurfaceAreaDocument.ModuleSurfaceArea sa = msa.getModuleSurfaceArea();
            if (sa == null) {
                return result;
            }
            PackageDependenciesDocument.PackageDependencies pkgDep = sa.getPackageDependencies();
            if (pkgDep == null) {
                return result;
            }
            List<PackageDependenciesDocument.PackageDependencies.Package> list = pkgDep.getPackageList();
            Iterator<PackageDependenciesDocument.PackageDependencies.Package> iter = list.iterator();
            while (iter.hasNext()) {
                PackageDependenciesDocument.PackageDependencies.Package item = iter.next();
                PackageIdentification packageId = new PackageIdentification(null, item.getPackageGuid(),
                                                                            item.getPackageVersion());
                result.add(packageId);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return result;
    }

    public List<File> getPackageMsaList(File spdFile) {
        List<File> result = new Vector<File>();
        WorkspaceTools wt = new WorkspaceTools();
        List<String> v = wt.getAllModulesOfPackage(spdFile.getPath());
        Iterator<String> iter = v.iterator();
        while (iter.hasNext()) {
            result.add(new File(iter.next()));
        }
        return result;
    }

    public List<PackageIdentification> getPackageDependencies(File spdFile) {
        List<File> msaFiles = getPackageMsaList(spdFile);
        return getPackageDependencies(msaFiles);
    }

    public List<PackageIdentification> getPackageDependencies(List<File> msaFiles) {
        List<PackageIdentification> result = new ArrayList<PackageIdentification>();
        Iterator<File> iter = msaFiles.iterator();
        while (iter.hasNext()) {
            result = AggregationOperation.union(result, getModuleDependencies(iter.next()));
        }
        return result;
    }

    public List<PackageIdentification> getModuleDependencies(File msaFile) {
        List<PackageIdentification> result = new ArrayList<PackageIdentification>();
        try {
            ModuleSurfaceArea msa = OpenFile.openMsaFile(msaFile.getPath());
            List<PackageDependenciesDocument.PackageDependencies.Package> p = msa.getPackageDependencies()
                                                                                 .getPackageList();
            Iterator<PackageDependenciesDocument.PackageDependencies.Package> iter = p.iterator();
            while (iter.hasNext()) {
                PackageDependenciesDocument.PackageDependencies.Package item = iter.next();
                PackageIdentification packageId = new PackageIdentification(null, item.getPackageGuid(),
                                                                            item.getPackageVersion());
                if (!AggregationOperation.belongs(packageId, result)) {
                    result.add(packageId);
                }
            }
        } catch (Exception e) {
        }
        return result;
    }
}
