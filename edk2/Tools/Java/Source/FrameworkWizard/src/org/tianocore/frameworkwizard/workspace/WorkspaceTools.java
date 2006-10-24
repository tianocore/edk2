/** @file

 The file is used to init workspace and get basic information of workspace
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.workspace;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.Vector;

import org.apache.xmlbeans.XmlCursor;
import org.apache.xmlbeans.XmlException;
import org.tianocore.DbPathAndFilename;
import org.tianocore.IndustryStdIncludesDocument.IndustryStdIncludes;
import org.tianocore.LibraryClassDeclarationsDocument.LibraryClassDeclarations;
import org.tianocore.LibraryClassDefinitionsDocument.LibraryClassDefinitions;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.MsaFilesDocument.MsaFiles;
import org.tianocore.PackageDependenciesDocument.PackageDependencies;
import org.tianocore.PackageSurfaceAreaDocument.PackageSurfaceArea;
import org.tianocore.PlatformSurfaceAreaDocument.PlatformSurfaceArea;
import org.tianocore.SourceFilesDocument.SourceFiles;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.GlobalData;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.OpenFile;
import org.tianocore.frameworkwizard.common.SaveFile;
import org.tianocore.frameworkwizard.common.Sort;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.Identification;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPackageType;
import org.tianocore.frameworkwizard.far.FarHeader;
import org.tianocore.frameworkwizard.far.FarIdentification;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.module.Identifications.PcdCoded.PcdIdentification;
import org.tianocore.frameworkwizard.module.Identifications.PcdCoded.PcdVector;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;

public class WorkspaceTools {

    public void addFarToDb(List<String> packageList, List<String> platformList, FarHeader far) {
        //FrameworkDatabase fdb = openFrameworkDb();

        for (int i = 0; i < packageList.size(); i++) {
            DbPathAndFilename item = DbPathAndFilename.Factory.newInstance();
            item.setFarGuid(far.getGuidValue());
            item.setStringValue(packageList.get(i));
            GlobalData.fdb.getPackageList().getFilenameList().add(item);
        }

        for (int i = 0; i < platformList.size(); i++) {
            DbPathAndFilename item = DbPathAndFilename.Factory.newInstance();
            item.setFarGuid(far.getGuidValue());
            item.setStringValue(platformList.get(i));
            GlobalData.fdb.getPlatformList().getFilenameList().add(item);
        }

        DbPathAndFilename farItem = DbPathAndFilename.Factory.newInstance();
        farItem.setFarGuid(far.getGuidValue());
        farItem.setStringValue(far.getFarName());
        GlobalData.fdb.getFarList().getFilenameList().add(farItem);

        String strFrameworkDbFilePath = Workspace.getCurrentWorkspace() + Workspace.getStrWorkspaceDatabaseFile();
        strFrameworkDbFilePath = Tools.convertPathToCurrentOsType(strFrameworkDbFilePath);

        try {
            SaveFile.saveDbFile(strFrameworkDbFilePath, GlobalData.fdb);
        } catch (Exception e) {
            Log.err("Save Database File", e.getMessage());
        }
    }

    public void removeFarFromDb(FarIdentification far) {
        //
        // Remove Packages
        //
        XmlCursor cursor = GlobalData.fdb.getPackageList().newCursor();
        cursor.toFirstChild();
        do {
            DbPathAndFilename item = (DbPathAndFilename) cursor.getObject();

            if (item.getFarGuid() != null && item.getFarGuid().equalsIgnoreCase(far.getGuid())) {
                cursor.removeXml();
            }
        } while (cursor.toNextSibling());
        cursor.dispose();

        //
        // Remove Platforms
        //
        cursor = GlobalData.fdb.getPlatformList().newCursor();
        cursor.toFirstChild();
        do {
            DbPathAndFilename item = (DbPathAndFilename) cursor.getObject();
            if (item.getFarGuid() != null && item.getFarGuid().equalsIgnoreCase(far.getGuid())) {
                cursor.removeXml();
            }
        } while (cursor.toNextSibling());

        //
        // Remove Far
        //
        cursor = GlobalData.fdb.getFarList().newCursor();
        cursor.toFirstChild();
        do {
            DbPathAndFilename item = (DbPathAndFilename) cursor.getObject();
            if (item.getFarGuid() != null && item.getFarGuid().equalsIgnoreCase(far.getGuid())) {
                cursor.removeXml();
            }
        } while (cursor.toNextSibling());
        cursor.dispose();

        String strFrameworkDbFilePath = Workspace.getCurrentWorkspace() + Workspace.getStrWorkspaceDatabaseFile();
        strFrameworkDbFilePath = Tools.convertPathToCurrentOsType(strFrameworkDbFilePath);
        try {
            SaveFile.saveDbFile(strFrameworkDbFilePath, GlobalData.fdb);
        } catch (Exception e) {
            Log.err("Save Database File", e.getMessage());
        }
    }

    public String getPackageFarGuid(Identification packageId) {
        for (int index = 0; index < GlobalData.fdb.getPackageList().getFilenameList().size(); index++) {
            DbPathAndFilename item = GlobalData.fdb.getPackageList().getFilenameArray(index);
            String path = Workspace.getCurrentWorkspace() + DataType.FILE_SEPARATOR + item.getStringValue();
            File tempFile = new File(path);
            if (tempFile.getPath().equalsIgnoreCase(packageId.getPath())) {
                return GlobalData.fdb.getPackageList().getFilenameArray(index).getFarGuid();
            }
        }

        return null;
    }

    public String getPlatformFarGuid(Identification platformId) {
        for (int index = 0; index < GlobalData.fdb.getPlatformList().getFilenameList().size(); index++) {
            DbPathAndFilename item = GlobalData.fdb.getPlatformList().getFilenameArray(index);
            String path = Workspace.getCurrentWorkspace() + DataType.FILE_SEPARATOR + item.getStringValue();
            File tempFile = new File(path);
            if (tempFile.getPath().equalsIgnoreCase(platformId.getPath())) {
                return GlobalData.fdb.getPlatformList().getFilenameArray(index).getFarGuid();
            }
        }

        return null;
    }

    public String getModuleFarGuid(Identification moduleId) {
        PackageIdentification packageId = getPackageIdByModuleId(moduleId);
        if (packageId != null) {
            return getPackageFarGuid(packageId);
        } else {
            return null;
        }
    }

    /**
     Get all modules' paths from one package
     
     @return a Vector with all modules' path
     
     **/
    public Vector<String> getAllModulesOfPackage(String path) {
        Vector<String> modulePath = new Vector<String>();
        PackageIdentification id = new PackageIdentification(null, null, null, path);
        MsaFiles files = GlobalData.openingPackageList.getPackageSurfaceAreaFromId(id).getMsaFiles();
        if (files != null) {
            for (int index = 0; index < files.getFilenameList().size(); index++) {
                String msaPath = files.getFilenameList().get(index);
                msaPath = Tools.addFileSeparator(Tools.getFilePathOnly(path)) + msaPath;
                msaPath = Tools.convertPathToCurrentOsType(msaPath);
                modulePath.addElement(msaPath);
            }
        }

        return modulePath;
    }

    /**
     Get all Industry Std Includes' paths from one package
     
     @return a Vector with all paths
     
     **/
    public Vector<String> getAllIndustryStdIncludesOfPackage(String path) {
        Vector<String> includePath = new Vector<String>();
        PackageIdentification id = new PackageIdentification(null, null, null, path);
        IndustryStdIncludes files = GlobalData.openingPackageList.getPackageSurfaceAreaFromId(id)
                                                                 .getIndustryStdIncludes();
        if (files != null) {
            for (int index = 0; index < files.getIndustryStdHeaderList().size(); index++) {
                String temp = files.getIndustryStdHeaderList().get(index).getIncludeHeader();
                temp = Tools.addFileSeparator(Tools.getFilePathOnly(path)) + temp;
                temp = Tools.convertPathToCurrentOsType(temp);
                includePath.addElement(temp);
            }
        }
        return includePath;
    }

    /**
     Get all package basic information form the FrameworkDatabase.db file
     
     @return vPackageList A vector includes all packages' basic information
     
     */
    public Vector<PackageIdentification> getAllPackages() {
        return GlobalData.vPackageList;
    }

    /**
     Get all package which match parameter isRepackagable
     
     @param isRepackagable
     @return
     
     **/
    public Vector<PackageIdentification> getAllRepackagablePackages() {
        Vector<PackageIdentification> v = new Vector<PackageIdentification>();
        for (int index = 0; index < GlobalData.openingPackageList.size(); index++) {
            OpeningPackageType opt = GlobalData.openingPackageList.getOpeningPackageByIndex(index);
            if (opt.getXmlSpd() != null) {
                if (opt.getXmlSpd().getPackageDefinitions() != null) {
                    if (opt.getXmlSpd().getPackageDefinitions().getRePackage()) {
                        v.addElement(opt.getId());
                    }
                } else {
                    v.addElement(opt.getId());
                }
            } else {
                v.addElement(opt.getId());
            }
        }
        return v;
    }

    public Vector<FarIdentification> getAllFars() {

        Vector<FarIdentification> v = new Vector<FarIdentification>();
        for (int index = 0; index < GlobalData.fdb.getFarList().getFilenameList().size(); index++) {
            DbPathAndFilename item = GlobalData.fdb.getFarList().getFilenameList().get(index);
            FarIdentification far = new FarIdentification(item.getFarGuid(), item.getMd5Sum(), item.getStringValue());
            v.addElement(far);
        }
        return v;
    }

    public Vector<PackageIdentification> getPackagesByFar(FarIdentification far) {
        Identification id = null;

        Vector<PackageIdentification> v = new Vector<PackageIdentification>();

        for (int index = 0; index < GlobalData.fdb.getPackageList().getFilenameList().size(); index++) {
            DbPathAndFilename item = GlobalData.fdb.getPackageList().getFilenameArray(index);
            String path = Workspace.getCurrentWorkspace() + DataType.FILE_SEPARATOR + item.getStringValue();
            path = Tools.convertPathToCurrentOsType(path);

            if (item.getFarGuid() != null && item.getFarGuid().equalsIgnoreCase(far.getGuid())) {

                try {
                    id = Tools.getId(path, OpenFile.openSpdFile(path));
                    v.addElement(new PackageIdentification(id));
                } catch (IOException e) {
                    Log.err("Open Package Surface Area " + path, e.getMessage());
                } catch (XmlException e) {
                    Log.err("Open Package Surface Area " + path, e.getMessage());
                } catch (Exception e) {
                    Log.err("Open Package Surface Area " + path, "Invalid file type");
                }
            }
        }
        return v;
    }

    public Vector<PlatformIdentification> getPlatformsByFar(FarIdentification far) {
        Identification id = null;

        Vector<PlatformIdentification> v = new Vector<PlatformIdentification>();

        for (int index = 0; index < GlobalData.fdb.getPlatformList().getFilenameList().size(); index++) {
            DbPathAndFilename item = GlobalData.fdb.getPlatformList().getFilenameArray(index);
            String path = Workspace.getCurrentWorkspace() + DataType.FILE_SEPARATOR + item.getStringValue();
            path = Tools.convertPathToCurrentOsType(path);

            if (item.getFarGuid() != null && item.getFarGuid().equalsIgnoreCase(far.getGuid())) {
                try {
                    id = Tools.getId(path, OpenFile.openFpdFile(path));
                    v.addElement(new PlatformIdentification(id));
                } catch (IOException e) {
                    Log.err("Open Platform Surface Area " + path, e.getMessage());
                } catch (XmlException e) {
                    Log.err("Open Platform Surface Area " + path, e.getMessage());
                } catch (Exception e) {
                    Log.err("Open Platform Surface Area " + path, "Invalid file type");
                }
            }
        }
        return v;
    }

    /**
     Get all module basic information from a package
     
     @param id Package id
     @return A vector includes all modules' basic information
     
     **/
    public Vector<ModuleIdentification> getAllModules(PackageIdentification pid) {
        Vector<ModuleIdentification> v = new Vector<ModuleIdentification>();
        Vector<String> modulePaths = this.getAllModulesOfPackage(pid.getPath());
        String modulePath = null;

        for (int index = 0; index < modulePaths.size(); index++) {
            modulePath = modulePaths.get(index);
            ModuleIdentification id = GlobalData.openingModuleList.getIdByPath(modulePath);
            if (id != null) {
                v.addElement(id);
            }
        }
        Sort.sortModules(v, DataType.SORT_TYPE_ASCENDING);
        return v;
    }

    /**
     Get all module basic information from a platform
     
     @param id Package id
     @return A vector includes all modules' basic information
     
     **/
    public Vector<ModuleIdentification> getAllModules(PlatformIdentification fid) {
        Vector<ModuleIdentification> v = new Vector<ModuleIdentification>();
        PlatformSurfaceArea fpd = GlobalData.openingPlatformList.getOpeningPlatformById(fid).getXmlFpd();
        if (fpd.getFrameworkModules() != null) {
            for (int index = 0; index < fpd.getFrameworkModules().getModuleSAList().size(); index++) {
                String guid = fpd.getFrameworkModules().getModuleSAList().get(index).getModuleGuid();
                String version = fpd.getFrameworkModules().getModuleSAList().get(index).getModuleVersion();
                ModuleIdentification id = GlobalData.openingModuleList.getIdByGuidVersion(guid, version);
                if (id != null) {
                    boolean isFind = false;
                    for (int indexOfModules = 0; indexOfModules < v.size(); indexOfModules++) {
                        if (v.elementAt(indexOfModules).equals(id)) {
                            isFind = true;
                            break;
                        }
                    }
                    if (!isFind) {
                        v.addElement(id);
                    }
                }
            }
        }
        Sort.sortModules(v, DataType.SORT_TYPE_ASCENDING);
        return v;
    }

    /**
     Get all module basic information form the FrameworkDatabase.db file
     
     @return vModuleList A vector includes all modules' basic information
     
     */
    public Vector<ModuleIdentification> getAllModules() {
        return GlobalData.vModuleList;
    }

    /**
     Get all platform basic information form the FrameworkDatabase.db file
     
     @return vplatformList A vector includes all platforms' basic information
     
     */
    public Vector<PlatformIdentification> getAllPlatforms() {
        return GlobalData.vPlatformList;
    }

    /**
     Get all Library Class Definitions from a package
     
     @return Vector
     **/
    public Vector<String> getAllLibraryClassDefinitionsFromPackage(PackageSurfaceArea spd) {
        Vector<String> vector = new Vector<String>();
        if (spd.getLibraryClassDeclarations() != null) {
            if (spd.getLibraryClassDeclarations().getLibraryClassList().size() > 0) {
                for (int index = 0; index < spd.getLibraryClassDeclarations().getLibraryClassList().size(); index++) {
                    vector.addElement(spd.getLibraryClassDeclarations().getLibraryClassList().get(index).getName());
                }
            }
        }
        Sort.sortVectorString(vector, DataType.SORT_TYPE_ASCENDING);
        return vector;
    }

    /**
     Get all Protocol Definitions from a package
     
     @return Vector
     **/
    public Vector<String> getAllProtocolDeclarationsFromPackage(PackageSurfaceArea spd) {
        Vector<String> vector = new Vector<String>();
        if (spd.getProtocolDeclarations() != null) {
            if (spd.getProtocolDeclarations().getEntryList().size() > 0) {
                for (int index = 0; index < spd.getProtocolDeclarations().getEntryList().size(); index++) {
                    vector.addElement(spd.getProtocolDeclarations().getEntryList().get(index).getCName());
                }
            }
        }
        Sort.sortVectorString(vector, DataType.SORT_TYPE_ASCENDING);
        return vector;
    }

    /**
     Get all Ppi Definitions from a package
     
     @return Vector
     **/
    public Vector<String> getAllPpiDeclarationsFromPackage(PackageSurfaceArea spd) {
        Vector<String> vector = new Vector<String>();
        if (spd.getPpiDeclarations() != null) {
            if (spd.getPpiDeclarations().getEntryList().size() > 0) {
                for (int index = 0; index < spd.getPpiDeclarations().getEntryList().size(); index++) {
                    vector.addElement(spd.getPpiDeclarations().getEntryList().get(index).getCName());
                }
            }
        }
        Sort.sortVectorString(vector, DataType.SORT_TYPE_ASCENDING);
        return vector;
    }

    /**
     Get all Guid Definitions from a package
     
     @return Vector
     **/
    public Vector<String> getAllGuidDeclarationsFromPackage(PackageSurfaceArea spd, String type) {
        Vector<String> vector = new Vector<String>();
        boolean isFound = false;
        if (spd.getGuidDeclarations() != null) {
            if (spd.getGuidDeclarations().getEntryList().size() > 0) {
                for (int index = 0; index < spd.getGuidDeclarations().getEntryList().size(); index++) {
                    Vector<String> vArch = Tools.convertListToVector(spd.getGuidDeclarations().getEntryList()
                                                                        .get(index).getGuidTypeList());
                    for (int indexOfArch = 0; indexOfArch < vArch.size(); indexOfArch++) {
                        if (vArch.get(indexOfArch).equals(type)) {
                            isFound = true;
                            break;
                        }
                    }
                    if ((isFound) || (vArch == null) || (vArch.size() < 1)) {
                        vector.addElement(spd.getGuidDeclarations().getEntryList().get(index).getCName());
                        isFound = false;
                    }
                }
            }
        }
        Sort.sortVectorString(vector, DataType.SORT_TYPE_ASCENDING);
        return vector;
    }

    /**
     Get all Pcd Definitions from a package
     
     @return Vector
     **/
    public PcdVector getAllPcdDeclarationsFromPackage(PackageSurfaceArea spd) {
        PcdVector vector = new PcdVector();
        if (spd.getPcdDeclarations() != null) {
            if (spd.getPcdDeclarations().getPcdEntryList().size() > 0) {
                for (int index = 0; index < spd.getPcdDeclarations().getPcdEntryList().size(); index++) {
                    String name = spd.getPcdDeclarations().getPcdEntryList().get(index).getCName();
                    String guidCName = spd.getPcdDeclarations().getPcdEntryList().get(index).getTokenSpaceGuidCName();
                    String help = spd.getPcdDeclarations().getPcdEntryList().get(index).getHelpText();
                    Vector<String> type = Tools.convertListToVector(spd.getPcdDeclarations().getPcdEntryList()
                                                                       .get(index).getValidUsage());
                    //
                    // The algorithm for PCD of msa should be:
                    // 1. If the type of PCD from Spd is FEATURE_FLAG, 
                    //    the type of Msa only can be FEATURE_FLAG.
                    // 2. If the type of PCD from Spd is not FEATURE_FLAG, 
                    //    the type of Msa could be selected from the PCD's all types and "DYNAMIC" type.
                    //
                    boolean hasFEATURE_FLAG = false;
                    boolean hasDYNAMIC = false;
                    for (int indexOfType = 0; indexOfType < type.size(); indexOfType++) {
                        if (type.elementAt(indexOfType).equals(DataType.PCD_ITEM_TYPE_DYNAMIC)) {
                            hasDYNAMIC = true;
                        }
                        if (type.elementAt(indexOfType).equals(DataType.PCD_ITEM_TYPE_FEATURE_FLAG)) {
                            hasFEATURE_FLAG = true;
                        }
                    }
                    if (hasFEATURE_FLAG) {
                        type.removeAllElements();
                        type.addElement(DataType.PCD_ITEM_TYPE_FEATURE_FLAG);
                    } else {
                        if (!hasDYNAMIC) {
                            type.addElement(DataType.PCD_ITEM_TYPE_DYNAMIC);
                        }
                    }
                    vector.addPcd(new PcdIdentification(name, guidCName, help, type));
                }
            }
        }
        Sort.sortPcds(vector, DataType.SORT_TYPE_ASCENDING);
        return vector;
    }

    public Vector<String> getAllLibraryClassDefinitionsFromWorkspace() {
        Vector<String> vector = new Vector<String>();
        for (int index = 0; index < GlobalData.vPackageList.size(); index++) {
            Vector<String> v = getAllLibraryClassDefinitionsFromPackage(GlobalData.openingPackageList
                                                                                                     .getPackageSurfaceAreaFromId(GlobalData.vPackageList
                                                                                                                                                         .get(index)));
            if (v != null && v.size() > 0) {
                vector.addAll(v);
            }

        }
        Sort.sortVectorString(vector, DataType.SORT_TYPE_ASCENDING);
        return vector;
    }

    public Vector<String> getAllLibraryClassDefinitionsFromPackages(Vector<PackageIdentification> vpid) {
        Vector<String> vector = new Vector<String>();
        for (int index = 0; index < vpid.size(); index++) {
            Vector<String> v = getAllLibraryClassDefinitionsFromPackage(GlobalData.openingPackageList
                                                                                                     .getPackageSurfaceAreaFromId(vpid
                                                                                                                                      .get(index)));
            if (v != null && v.size() > 0) {
                vector.addAll(v);
            }

        }
        Sort.sortVectorString(vector, DataType.SORT_TYPE_ASCENDING);
        return vector;
    }

    public Vector<String> getAllProtocolDeclarationsFromWorkspace() {
        Vector<String> vector = new Vector<String>();
        for (int index = 0; index < GlobalData.vPackageList.size(); index++) {
            Vector<String> v = getAllProtocolDeclarationsFromPackage(GlobalData.openingPackageList
                                                                                                  .getPackageSurfaceAreaFromId(GlobalData.vPackageList
                                                                                                                                                      .get(index)));
            if (v != null && v.size() > 0) {
                vector.addAll(v);
            }
        }
        Sort.sortVectorString(vector, DataType.SORT_TYPE_ASCENDING);
        return vector;
    }

    public Vector<String> getAllProtocolDeclarationsFromPackages(Vector<PackageIdentification> vpid) {
        Vector<String> vector = new Vector<String>();
        for (int index = 0; index < vpid.size(); index++) {
            Vector<String> v = getAllProtocolDeclarationsFromPackage(GlobalData.openingPackageList
                                                                                                  .getPackageSurfaceAreaFromId(vpid
                                                                                                                                   .get(index)));
            if (v != null && v.size() > 0) {
                vector.addAll(v);
            }
        }
        Sort.sortVectorString(vector, DataType.SORT_TYPE_ASCENDING);
        return vector;
    }

    public Vector<String> getAllPpiDeclarationsFromWorkspace() {
        Vector<String> vector = new Vector<String>();
        for (int index = 0; index < GlobalData.vPackageList.size(); index++) {
            Vector<String> v = getAllPpiDeclarationsFromPackage(GlobalData.openingPackageList
                                                                                             .getPackageSurfaceAreaFromId(GlobalData.vPackageList
                                                                                                                                                 .get(index)));
            if (v != null && v.size() > 0) {
                vector.addAll(v);
            }
        }
        Sort.sortVectorString(vector, DataType.SORT_TYPE_ASCENDING);
        return vector;
    }

    public Vector<String> getAllPpiDeclarationsFromPackages(Vector<PackageIdentification> vpid) {
        Vector<String> vector = new Vector<String>();
        for (int index = 0; index < vpid.size(); index++) {
            Vector<String> v = getAllPpiDeclarationsFromPackage(GlobalData.openingPackageList
                                                                                             .getPackageSurfaceAreaFromId(vpid
                                                                                                                              .get(index)));
            if (v != null && v.size() > 0) {
                vector.addAll(v);
            }
        }
        Sort.sortVectorString(vector, DataType.SORT_TYPE_ASCENDING);
        return vector;
    }

    public Vector<String> getAllGuidDeclarationsFromWorkspace(String type) {
        Vector<String> vector = new Vector<String>();
        for (int index = 0; index < GlobalData.vPackageList.size(); index++) {
            Vector<String> v = getAllGuidDeclarationsFromPackage(
                                                                 GlobalData.openingPackageList
                                                                                              .getPackageSurfaceAreaFromId(GlobalData.vPackageList
                                                                                                                                                  .get(index)),
                                                                 type);
            if (v != null && v.size() > 0) {
                vector.addAll(v);
            }

        }
        Sort.sortVectorString(vector, DataType.SORT_TYPE_ASCENDING);
        return vector;
    }

    public Vector<String> getAllGuidDeclarationsFromPackages(Vector<PackageIdentification> vpid, String type) {
        Vector<String> vector = new Vector<String>();
        for (int index = 0; index < vpid.size(); index++) {
            Vector<String> v = getAllGuidDeclarationsFromPackage(
                                                                 GlobalData.openingPackageList
                                                                                              .getPackageSurfaceAreaFromId(vpid
                                                                                                                               .get(index)),
                                                                 type);
            if (v != null && v.size() > 0) {
                vector.addAll(v);
            }

        }
        Sort.sortVectorString(vector, DataType.SORT_TYPE_ASCENDING);
        return vector;
    }

    public PcdVector getAllPcdDeclarationsFromWorkspace() {
        PcdVector vector = new PcdVector();
        for (int index = 0; index < GlobalData.openingPackageList.size(); index++) {
            PcdVector v = getAllPcdDeclarationsFromPackage(GlobalData.openingPackageList
                                                                                        .getPackageSurfaceAreaFromId(GlobalData.vPackageList
                                                                                                                                            .get(index)));
            if (v != null && v.size() > 0) {
                vector.addAll(v);
            }

        }
        Sort.sortPcds(vector, DataType.SORT_TYPE_ASCENDING);
        return vector;
    }

    public PcdVector getAllPcdDeclarationsFromPackages(Vector<PackageIdentification> vpid) {
        PcdVector vector = new PcdVector();
        for (int index = 0; index < vpid.size(); index++) {
            PcdVector v = getAllPcdDeclarationsFromPackage(GlobalData.openingPackageList
                                                                                        .getPackageSurfaceAreaFromId(vpid
                                                                                                                         .get(index)));
            if (v != null && v.size() > 0) {
                vector.addAll(v);
            }

        }
        Sort.sortPcds(vector, DataType.SORT_TYPE_ASCENDING);
        return vector;
    }

    /**
     Find a module's package's id
     
     @param id input module id
     @return package id
     
     **/
    public PackageIdentification getPackageIdByModuleId(Identification id) {
        Vector<String> modulePaths = new Vector<String>();
        Identification mid = null;
        String packagePath = null;
        String modulePath = null;

        //
        // For each package, get all modules list
        //
        for (int indexI = 0; indexI < GlobalData.vPackageList.size(); indexI++) {
            packagePath = GlobalData.vPackageList.elementAt(indexI).getPath();
            modulePaths = this.getAllModulesOfPackage(packagePath);
            for (int indexJ = 0; indexJ < modulePaths.size(); indexJ++) {
                modulePath = modulePaths.get(indexJ);
                mid = GlobalData.openingModuleList.getIdByPath(modulePath);
                //
                // Check id
                //
                if (mid != null) {
                    if (mid.equals(id)) {
                        return GlobalData.vPackageList.elementAt(indexI);
                    }
                }
            }
        }

        return null;
    }

    /**
     Add module information to package surface area
     
     @param mid
     @throws IOException
     @throws XmlException
     @throws Exception
     
     **/
    public void addModuleToPackage(ModuleIdentification mid, PackageSurfaceArea spd) throws IOException, XmlException,
                                                                                    Exception {
        String packagePath = mid.getPackageId().getPath();
        String modulePath = mid.getPath();
        if (spd == null) {
            spd = OpenFile.openSpdFile(packagePath);
        }
        MsaFiles msaFile = spd.getMsaFiles();
        if (msaFile == null) {
            msaFile = MsaFiles.Factory.newInstance();
        }
        packagePath = packagePath.substring(0, packagePath.lastIndexOf(DataType.FILE_SEPARATOR));
        String fn = Tools.getRelativePath(modulePath, packagePath);
        msaFile.addNewFilename();
        msaFile.setFilenameArray(msaFile.getFilenameList().size() - 1, fn);
        spd.setMsaFiles(msaFile);
        SaveFile.saveSpdFile(mid.getPackageId().getPath(), spd);
        //
        // Update GlobalData
        //
        GlobalData.openingPackageList.getPackageSurfaceAreaFromId(mid.getPackageId()).setMsaFiles(msaFile);
    }

    /**
     Add input package into database
     
     @param id
     * @throws Exception 
     
     **/
    public void addPackageToDatabase(PackageIdentification id) throws Exception {
        String path = id.getPath();
        path = Tools.getRelativePath(path, Workspace.getCurrentWorkspace());

        DbPathAndFilename filename = DbPathAndFilename.Factory.newInstance();
        filename.setStringValue(path);
        GlobalData.fdb.getPackageList().addNewFilename();
        GlobalData.fdb.getPackageList().setFilenameArray(GlobalData.fdb.getPackageList().sizeOfFilenameArray() - 1,
                                                         filename);
        String strFrameworkDbFilePath = Workspace.getCurrentWorkspace() + Workspace.getStrWorkspaceDatabaseFile();
        strFrameworkDbFilePath = Tools.convertPathToCurrentOsType(strFrameworkDbFilePath);
        SaveFile.saveDbFile(strFrameworkDbFilePath, GlobalData.fdb);
    }

    /**
     Add input package into database
     
     @param id
     * @throws Exception 
     
     **/
    public void addPlatformToDatabase(PlatformIdentification id) throws Exception {
        String path = id.getPath();
        path = Tools.getRelativePath(path, Workspace.getCurrentWorkspace());

        DbPathAndFilename filename = DbPathAndFilename.Factory.newInstance();
        filename.setStringValue(path);
        GlobalData.fdb.getPlatformList().addNewFilename();
        GlobalData.fdb.getPlatformList().setFilenameArray(GlobalData.fdb.getPlatformList().sizeOfFilenameArray() - 1,
                                                          filename);
        String strFrameworkDbFilePath = Workspace.getCurrentWorkspace() + Workspace.getStrWorkspaceDatabaseFile();
        strFrameworkDbFilePath = Tools.convertPathToCurrentOsType(strFrameworkDbFilePath);
        SaveFile.saveDbFile(strFrameworkDbFilePath, GlobalData.fdb);
    }

    /**
     Get all file's path from one module
     
     @param path
     @return
     @throws IOException
     @throws XmlException
     @throws Exception
     
     **/
    public Vector<String> getAllFilesPathOfModule(String path) {
        Vector<String> v = new Vector<String>();
        path = Tools.convertPathToCurrentOsType(path);

        //
        // First add msa file's path
        //
        v.addElement(path);

        ModuleSurfaceArea msa = GlobalData.openingModuleList
                                                            .getModuleSurfaceAreaFromId(GlobalData.openingModuleList
                                                                                                                    .getIdByPath(path));
        //
        // Get common defined files of module
        //
        if (msa != null) {
            //
            // Get all files' path of a module
            //
            SourceFiles sf = msa.getSourceFiles();
            if (sf != null) {
                for (int index = 0; index < sf.getFilenameList().size(); index++) {
                    String temp = sf.getFilenameList().get(index).getStringValue();
                    temp = Tools.addFileSeparator(Tools.getFilePathOnly(path)) + temp;
                    v.addElement(Tools.convertPathToCurrentOsType(temp));
                }
            }
        }

        //
        // Get include header files for this module
        //
        if (msa.getLibraryClassDefinitions() != null) {
            LibraryClassDefinitions lcd = msa.getLibraryClassDefinitions();
            for (int index = 0; index < lcd.sizeOfLibraryClassArray(); index++) {
                if (lcd.getLibraryClassList().get(index).getUsage().toString()
                       .equals(DataType.USAGE_TYPE_ALWAYS_PRODUCED)
                    || lcd.getLibraryClassList().get(index).getUsage().toString()
                          .equals(DataType.USAGE_TYPE_SOMETIMES_PRODUCED)) {
                    //
                    // Get library class name
                    //
                    String name = lcd.getLibraryClassList().get(index).getKeyword();

                    //
                    // Find file path for this class
                    //
                    PackageIdentification pid = GlobalData.openingModuleList.getIdByPath(path).getPackageId();
                    PackageSurfaceArea spd = GlobalData.openingPackageList.getPackageSurfaceAreaFromId(pid);
                    String headerFile = getHeaderFileFromPackageByLibraryClassName(spd, name);
                    if (!Tools.isEmpty(headerFile)) {
                        v.addElement(Tools.convertPathToCurrentOsType(Tools.getFilePathOnly(pid.getPath())
                                                                      + DataType.FILE_SEPARATOR + headerFile));
                    }
                }
            }
        }

        return v;
    }

    /**
     Get all file's path from one package
     
     @param path
     @return
     @throws IOException
     @throws XmlException
     @throws Exception
     
     **/
    public Vector<String> getAllFilesPathOfPakcage(String path) {
        Vector<String> v = new Vector<String>();
        path = Tools.convertPathToCurrentOsType(path);
        //
        // First add package
        //
        v.addElement(path);

        //
        // Add the package's industry std includes one by one
        //
        Vector<String> f1 = new Vector<String>();
        f1 = getAllIndustryStdIncludesOfPackage(path);
        for (int index = 0; index < f1.size(); index++) {
            v.addElement(f1.get(index));
        }

        //
        // Add module's files one by one
        //
        f1 = new Vector<String>();
        f1 = getAllModulesOfPackage(path);
        for (int indexI = 0; indexI < f1.size(); indexI++) {
            Vector<String> f2 = getAllFilesPathOfModule(f1.get(indexI));
            for (int indexJ = 0; indexJ < f2.size(); indexJ++) {
                v.addElement(f2.get(indexJ));
            }
        }

        return v;
    }

    /**
     Get a module's all package dependencies
     
     @param mid The module id
     @return A vector of all package dependency ids
     
     **/
    public Vector<PackageIdentification> getPackageDependenciesOfModule(ModuleIdentification mid) {
        Vector<PackageIdentification> vpid = new Vector<PackageIdentification>();
        ModuleSurfaceArea msa = GlobalData.openingModuleList.getModuleSurfaceAreaFromId(mid);
        if (msa != null) {
            PackageDependencies pd = msa.getPackageDependencies();
            if (pd != null) {
                for (int index = 0; index < pd.getPackageList().size(); index++) {
                    String guid = pd.getPackageList().get(index).getPackageGuid();
                    String version = pd.getPackageList().get(index).getPackageVersion();
                    PackageIdentification pid = GlobalData.openingPackageList.getIdByGuidVersion(guid, version);
                    if (pid != null) {
                        vpid.addElement(pid);
                    }
                }
            }
        }
        return vpid;
    }

    public Vector<String> getAllModuleGuidXref() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < GlobalData.openingModuleList.size(); index++) {
            ModuleIdentification id = GlobalData.openingModuleList.getOpeningModuleByIndex(index).getId();
            v.addElement(id.getGuid() + " " + id.getName());
        }
        return v;
    }

    public Vector<String> getModuleArch(ModuleIdentification id) {
        Vector<String> v = new Vector<String>();
        ModuleSurfaceArea msa = null;
        if (id != null) {
            msa = GlobalData.openingModuleList.getModuleSurfaceAreaFromId(id);
        }
        if (msa != null) {
            if (msa.getModuleDefinitions() != null) {
                v = Tools.convertListToVector(msa.getModuleDefinitions().getSupportedArchitectures());
            }
        }
        return v;
    }

    public String getHeaderFileFromPackageByLibraryClassName(PackageSurfaceArea spd, String name) {
        String headerFile = "";
        if (spd != null) {
            if (spd.getLibraryClassDeclarations() != null) {
                LibraryClassDeclarations lcdl = spd.getLibraryClassDeclarations();
                for (int indexOfLibOfSpd = 0; indexOfLibOfSpd < lcdl.sizeOfLibraryClassArray(); indexOfLibOfSpd++) {
                    if (lcdl.getLibraryClassList().get(indexOfLibOfSpd).getName().equals(name)) {
                        return lcdl.getLibraryClassList().get(indexOfLibOfSpd).getIncludeHeader();
                    }
                }
            }
        }

        return headerFile;
    }
}
