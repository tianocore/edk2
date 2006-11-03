/** @file
 
 The file is used to save basic information of module
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.module.Identifications;

import java.io.IOException;

import org.apache.xmlbeans.XmlException;
import org.tianocore.LibraryUsage;
import org.tianocore.LibraryClassDefinitionsDocument.LibraryClassDefinitions;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.OpenFile;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.Identification;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;

public class ModuleIdentification extends Identification {

    private PackageIdentification packageId;

    private String moduleType;

    private boolean isLibrary;

    public ModuleIdentification(String name, String guid, String version, String path) {
        super(name, guid, version, path);
        setModuleType();
    }

    public ModuleIdentification(String name, String guid, String version, String path, boolean library) {
        super(name, guid, version, path);
        this.isLibrary = library;
    }

    public ModuleIdentification(Identification id) {
        super(id.getName(), id.getGuid(), id.getVersion(), id.getPath());
    }

    public ModuleIdentification(Identification id, boolean library) {
        super(id.getName(), id.getGuid(), id.getVersion(), id.getPath());
        this.isLibrary = library;
    }

    public ModuleIdentification(String name, String guid, String version, String path, PackageIdentification packageId) {
        super(name, guid, version, path);
        this.packageId = packageId;
        setModuleType();
    }

    public ModuleIdentification(String name, String guid, String version, String path, PackageIdentification packageId,
                                String type) {
        super(name, guid, version, path);
        this.packageId = packageId;
        this.moduleType = type;
    }

    public ModuleIdentification(Identification id, PackageIdentification packageId) {
        super(id.getName(), id.getGuid(), id.getVersion(), id.getPath());
        this.packageId = packageId;
        setModuleType();
    }

    public ModuleIdentification(Identification id, PackageIdentification packageId, boolean library) {
        super(id.getName(), id.getGuid(), id.getVersion(), id.getPath());
        this.packageId = packageId;
        this.isLibrary = library;
    }

    public ModuleIdentification(Identification id, PackageIdentification packageId, String type) {
        super(id.getName(), id.getGuid(), id.getVersion(), id.getPath());
        this.packageId = packageId;
        this.moduleType = type;
    }

    public String toString() {
        return "Module " + this.getName() + "[" + this.getGuid() + "] in package " + packageId;
    }

    public PackageIdentification getPackageId() {
        return packageId;
    }

    public void setPackageId(PackageIdentification packageId) {
        this.packageId = packageId;
    }

    public String getModuleType() {
        return moduleType;
    }

    public void setModuleType(String moduleType) {
        this.moduleType = moduleType;
    }

    private void setModuleType() {
        ModuleSurfaceArea msa = null;
        try {
            msa = OpenFile.openMsaFile(this.getPath());
        } catch (IOException e) {
            // TODO Auto-generated catch block

        } catch (XmlException e) {
            // TODO Auto-generated catch block

        } catch (Exception e) {
            // TODO Auto-generated catch block

        }
        setModuleType(DataType.MODULE_TYPE_MODULE);
        setLibrary(false);
        if (msa != null) {
            LibraryClassDefinitions lib = msa.getLibraryClassDefinitions();
            if (lib != null) {
                for (int index = 0; index < lib.getLibraryClassList().size(); index++) {
                    if (lib.getLibraryClassList().get(index).getUsage().equals(LibraryUsage.ALWAYS_PRODUCED)) {
                        setModuleType(DataType.MODULE_TYPE_LIBRARY);
                        setLibrary(true);
                        break;
                    }
                }
            }
        }
    }

    public boolean equals(String moduleGuid, String moduleVersion, String packageGuid, String packageVersion) {
        boolean b = false;
        if (this.getGuid().equals(moduleGuid) && this.getPackageId().getGuid().equals(packageGuid)) {
            b = true;
            //
            // Check Version
            //
            if (moduleVersion != null) {
                if (!Tools.isEmpty(moduleVersion)) {
                    if (!moduleVersion.equals(this.getVersion())) {
                        b = false;
                    }
                }
            }
            if (packageVersion != null) {
                if (!Tools.isEmpty(packageVersion)) {
                    if (!packageVersion.equals(this.getPackageId().getVersion())) {
                        b = false;
                    }
                }
            }
        }
        return b;
    }

    public boolean isLibrary() {
        return isLibrary;
    }

    public void setLibrary(boolean isLibrary) {
        this.isLibrary = isLibrary;
    }
}
