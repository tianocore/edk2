/** @file 
 The file is used to provide initializing global data.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 **/
package org.tianocore.frameworkwizard.common;

import java.io.IOException;
import java.util.Vector;

import org.apache.xmlbeans.XmlException;
import org.tianocore.FrameworkDatabaseDocument.FrameworkDatabase;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.MsaFilesDocument.MsaFiles;
import org.tianocore.PackageSurfaceAreaDocument.PackageSurfaceArea;
import org.tianocore.PlatformSurfaceAreaDocument.PlatformSurfaceArea;
import org.tianocore.frameworkwizard.common.Identifications.Identification;
import org.tianocore.frameworkwizard.common.Identifications.OpeningModuleList;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPackageList;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPlatformList;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;
import org.tianocore.frameworkwizard.workspace.Workspace;

public class GlobalData {

    public static FrameworkDatabase fdb = null;

    public static OpeningModuleList openingModuleList = new OpeningModuleList();

    public static OpeningPackageList openingPackageList = new OpeningPackageList();

    public static OpeningPlatformList openingPlatformList = new OpeningPlatformList();

    public static Vector<ModuleIdentification> vModuleList = new Vector<ModuleIdentification>();

    public static Vector<PackageIdentification> vPackageList = new Vector<PackageIdentification>();

    public static Vector<PlatformIdentification> vPlatformList = new Vector<PlatformIdentification>();

    public static void init() {
        initDatabase();
        initPackage();
        initPlatform();
        initModule();
    }

    public static void initDatabase() {
        String strFrameworkDbFilePath = Workspace.getCurrentWorkspace() + Workspace.getStrWorkspaceDatabaseFile();
        strFrameworkDbFilePath = Tools.convertPathToCurrentOsType(strFrameworkDbFilePath);
        try {
            fdb = OpenFile.openFrameworkDb(strFrameworkDbFilePath);
        } catch (XmlException e) {
            Log.err("Open Framework Database " + strFrameworkDbFilePath, e.getMessage());
            return;
        } catch (Exception e) {
            Log.err("Open Framework Database " + strFrameworkDbFilePath, "Invalid file type");
            return;
        }
    }

    public static void initModule() {
        vModuleList = new Vector<ModuleIdentification>();
        openingModuleList = new OpeningModuleList();

        ModuleSurfaceArea msa = null;
        Vector<String> modulePaths = new Vector<String>();
        Identification id = null;
        ModuleIdentification mid = null;
        String packagePath = null;
        String modulePath = null;

        //
        // For each package, get all modules list
        //
        if (vPackageList.size() > 0) {
            for (int indexI = 0; indexI < vPackageList.size(); indexI++) {
                packagePath = vPackageList.elementAt(indexI).getPath();
                modulePaths = getAllModulesOfPackage(packagePath);

                for (int indexJ = 0; indexJ < modulePaths.size(); indexJ++) {
                    try {
                        modulePath = modulePaths.get(indexJ);
                        msa = OpenFile.openMsaFile(modulePath);

                    } catch (IOException e) {
                        Log.err("Open Module Surface Area " + modulePath, e.getMessage());
                        continue;
                    } catch (XmlException e) {
                        Log.err("Open Module Surface Area " + modulePath, e.getMessage());
                        continue;
                    } catch (Exception e) {
                        Log.err("Open Module Surface Area " + modulePath, "Invalid file type");
                        continue;
                    }
                    id = Tools.getId(modulePath, msa);
                    mid = new ModuleIdentification(id, vPackageList.elementAt(indexI));
                    vModuleList.addElement(mid);
                    openingModuleList.insertToOpeningModuleList(mid, msa);
                }
            }
            Sort.sortModules(vModuleList, DataType.SORT_TYPE_ASCENDING);
        }
    }

    public static void initPackage() {
        vPackageList = new Vector<PackageIdentification>();
        openingPackageList = new OpeningPackageList();
        if (fdb != null) {
            for (int index = 0; index < fdb.getPackageList().getFilenameList().size(); index++) {
                String path = Workspace.getCurrentWorkspace() + DataType.FILE_SEPARATOR
                              + fdb.getPackageList().getFilenameArray(index).getStringValue();
                path = Tools.convertPathToCurrentOsType(path);
                PackageSurfaceArea spd = null;
                PackageIdentification id = null;
                try {
                    spd = OpenFile.openSpdFile(path);
                } catch (IOException e) {
                    Log.err("Open Package Surface Area " + path, e.getMessage());
                    continue;
                } catch (XmlException e) {
                    Log.err("Open Package Surface Area " + path, e.getMessage());
                    continue;
                } catch (Exception e) {
                    Log.err("Open Package Surface Area " + path, "Invalid file type");
                    continue;
                }
                id = Tools.getId(path, spd);
                vPackageList.addElement(id);
                openingPackageList.insertToOpeningPackageList(id, spd);
            }
            Sort.sortPackages(vPackageList, DataType.SORT_TYPE_ASCENDING);
        }
    }

    public static void initPlatform() {
        vPlatformList = new Vector<PlatformIdentification>();
        openingPlatformList = new OpeningPlatformList();

        if (fdb != null) {
            for (int index = 0; index < fdb.getPlatformList().getFilenameList().size(); index++) {
                String path = Workspace.getCurrentWorkspace() + DataType.FILE_SEPARATOR
                              + fdb.getPlatformList().getFilenameArray(index).getStringValue();
                path = Tools.convertPathToCurrentOsType(path);
                PlatformSurfaceArea fpd = null;
                PlatformIdentification id = null;
                try {
                    fpd = OpenFile.openFpdFile(path);
                } catch (IOException e) {
                    Log.err("Open Platform Surface Area " + path, e.getMessage());
                    continue;
                } catch (XmlException e) {
                    Log.err("Open Platform Surface Area " + path, e.getMessage());
                    continue;
                } catch (Exception e) {
                    Log.err("Open Platform Surface Area " + path, "Invalid file type");
                    continue;
                }
                id = Tools.getId(path, fpd);
                vPlatformList.addElement(new PlatformIdentification(id));
                openingPlatformList.insertToOpeningPlatformList(id, fpd);
            }
            Sort.sortPlatforms(vPlatformList, DataType.SORT_TYPE_ASCENDING);
        }
    }

    /**
     Get all modules' paths from one package
     
     @return a Vector with all modules' path
     
     **/
    public static Vector<String> getAllModulesOfPackage(String path) {
        Vector<String> modulePath = new Vector<String>();
        try {
            MsaFiles files = OpenFile.openSpdFile(path).getMsaFiles();
            if (files != null) {
                for (int index = 0; index < files.getFilenameList().size(); index++) {
                    String msaPath = files.getFilenameList().get(index);
                    msaPath = Tools.addFileSeparator(Tools.getFilePathOnly(path)) + msaPath;
                    msaPath = Tools.convertPathToCurrentOsType(msaPath);
                    modulePath.addElement(msaPath);
                }
            }
        } catch (IOException e) {
            Log.err("Get all modules from a package " + path, e.getMessage());
        } catch (XmlException e) {
            Log.err("Get all modules from a package " + path, e.getMessage());
        } catch (Exception e) {
            Log.err("Get all modules from a package " + path, e.getMessage());
        }
        return modulePath;
    }

    /**
     Get a module id
     
     @param moduleGuid
     @param moduleVersion
     @param packageGuid
     @param packageVersion
     @return
     
     **/
    public static ModuleIdentification findModuleId(String moduleGuid, String moduleVersion, String packageGuid,
                                                    String packageVersion) {
        ModuleIdentification mid = null;
        for (int index = 0; index < vModuleList.size(); index++) {
            if (vModuleList.elementAt(index).equals(moduleGuid, moduleVersion, packageGuid, packageVersion)) {
                mid = vModuleList.elementAt(index);
                break;
            }
        }
        return mid;
    }

    /**
     Get a package id
     
     @param packageGuid
     @param packageVersion
     @return
     
     **/
    public static PackageIdentification findPackageId(String packageGuid, String packageVersion) {
        PackageIdentification pid = null;
        for (int index = 0; index < vPackageList.size(); index++) {
            if (vPackageList.elementAt(index).equals(packageGuid, packageVersion)) {
                pid = vPackageList.elementAt(index);
                break;
            }
        }
        return pid;
    }

    /**
     Get a platform id 
     
     @param platformGuid
     @param platformVersion
     @return
     
     **/
    public static PlatformIdentification findPlatformId(String platformGuid, String platformVersion) {
        PlatformIdentification pid = null;
        for (int index = 0; index < vPlatformList.size(); index++) {
            if (vPlatformList.elementAt(index).equals(platformGuid, platformVersion)) {
                pid = vPlatformList.elementAt(index);
                break;
            }
        }
        return pid;
    }

    /**
    
     @param relativePath
     @param mode
     @return
    
    **/
    public static boolean isDuplicateRelativePath(String relativePath, int mode) {
        if (mode == DataType.RETURN_TYPE_MODULE_SURFACE_AREA) {
            for (int index = 0; index < vModuleList.size(); index++) {
                String path = vModuleList.elementAt(index).getPath();
                if (Tools.getFilePathOnly(path).equals(relativePath)) {
                    return true;
                }
            }
        }

        if (mode == DataType.RETURN_TYPE_PACKAGE_SURFACE_AREA) {
            for (int index = 0; index < vPackageList.size(); index++) {
                String path = vPackageList.elementAt(index).getPath();
                if (Tools.getFilePathOnly(path).equals(relativePath)) {
                    return true;
                }
            }
        }

        if (mode == DataType.RETURN_TYPE_PLATFORM_SURFACE_AREA) {
            for (int index = 0; index < vPlatformList.size(); index++) {
                String path = vPlatformList.elementAt(index).getPath();
                if (Tools.getFilePathOnly(path).equals(relativePath)) {
                    return true;
                }
            }
        }

        return false;
    }
}
