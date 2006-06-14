/** @file
  GlobalData class. 
  
  GlobalData provide initializing, instoring, querying and update global data.
  It is a bridge to intercommunicate between multiple component, such as AutoGen,
  PCD and so on.   
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.build.global;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.apache.tools.ant.BuildException;
import org.apache.xmlbeans.XmlObject;
import org.tianocore.FilenameDocument;
import org.tianocore.FilenameDocument.Filename;
import org.tianocore.FrameworkDatabaseDocument;
import org.tianocore.MsaFilesDocument;
import org.tianocore.MsaFilesDocument.MsaFiles.MsaFile;
import org.tianocore.MsaHeaderDocument.MsaHeader;
import org.tianocore.MsaLibHeaderDocument.MsaLibHeader;
import org.tianocore.PackageListDocument;
import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.build.autogen.CommonDefinition;
import org.tianocore.build.fpd.FpdParserTask;
import org.tianocore.build.pcd.entity.MemoryDatabaseManager;

/**
  GlobalData provide initializing, instoring, querying and update global data.
  It is a bridge to intercommunicate between multiple component, such as AutoGen,
  PCD and so on. 
  
  <p>Note that all global information are initialized incrementally. All data will 
  parse and record only it is necessary during build time. </p>
  
  @since GenBuild 1.0
**/
public class GlobalData {

    ///
    /// means no surface area information for module
    ///
    public static final int NO_SA = 0;

    ///
    /// means only MSA
    ///
    public static final int ONLY_MSA = 1;

    ///
    /// means only Library MSA
    ///
    public static final int ONLY_LIBMSA = 2;

    ///
    /// means both MSA and MBD
    ///
    public static final int MSA_AND_MBD = 3;

    ///
    /// means both Library MSA and Library MBD
    ///
    public static final int LIBMSA_AND_LIBMBD = 4;

    ///
    /// Be used to ensure Global data will be initialized only once.
    ///
    public static boolean globalFlag = false;

    ///
    /// Record current WORKSPACE Directory
    ///
    private static String workspaceDir = "";

    ///
    /// Two columns: Package Name (Key), Package Path(ralative to WORKSPACE)
    ///
    private static final Map<String, String> packageInfo = new HashMap<String, String>();

    ///
    /// spdTable
    /// Key: Package Name, Value: SPD detail info
    ///
    private static final Map<String, Spd> spdTable = new HashMap<String, Spd>();

    ///
    /// Three columns:
    /// 1. Module Name | BaseName (Key)
    /// 2. Module Path + Msa file name (relative to Package)
    /// 3. Package Name (This module belong to which package)
    ///
    private static final Map<String, String[]> moduleInfo = new HashMap<String, String[]>();

    ///
    /// List all libraries for current build module
    /// Key: Library BaseName, Value: output library path+name
    ///
    private static final Map<String, String> libraries = new HashMap<String, String>();

    ///
    /// Store every module's relative library instances BaseName
    /// Key: Module BaseName, Value: All library instances module depends on.
    ///
    private static final Map<String, Set<String> > moduleLibraryMap = new HashMap<String, Set<String> >();

    ///
    /// Key: Module BaseName, Value: original MSA info
    ///
    private static final Map<String, Map<String, XmlObject> > nativeMsa = new HashMap<String, Map<String, XmlObject> >();

    ///
    /// Key: Module BaseName, Value: original MBD info
    ///
    private static final Map<String, Map<String, XmlObject> > nativeMbd = new HashMap<String, Map<String, XmlObject> >();

    ///
    /// Two columns: Module Name or Base Name as Key
    /// Value is a HashMap with overridden data from MSA/MBD or/and Platform
    ///
    private static final Map<String, Map<String, XmlObject> > parsedModules = new HashMap<String, Map<String, XmlObject> >();

    ///
    /// List all built Module; Value is Module BaseName + Arch. TBD
    ///
    private static final Set<String> builtModules = new HashSet<String>();

    ///
    /// Library instance information table which recored the library and it's
    /// constructor and distructor function
    ///
    private static final Map<String, String[]> libInstanceInfo = new HashMap<String, String[]>();

    ///
    /// PCD memory database stored all PCD information which collected from FPD,MSA and SPD.
    ///
    private static final MemoryDatabaseManager pcdDbManager = new MemoryDatabaseManager();

    /**
      Query the module's absolute path with module base name. 
      
      @param moduleName the base name of the module
      @return the absolute module path
    **/
    public synchronized static String getModulePath(String moduleName) {
        String[] info = moduleInfo.get(moduleName);
        String packagePath = (String) packageInfo.get(info[1]);
        File convertFile = new File(workspaceDir + File.separatorChar + packagePath + File.separatorChar + info[0]);
        return convertFile.getParent();
    }

    /**
      Query the module's absolute MSA file path with module base name. 
    
      @param moduleName the base name of the module
      @return the absolute MSA file name
      @throws BuildException
              Base name is not registered in any SPD files
    **/
    private synchronized static String getMsaFilename(String moduleName) throws BuildException {
        String[] info = moduleInfo.get(moduleName);
        if (info == null) {
            throw new BuildException("Module base name [" + moduleName + "] can't found in all SPD.");
        }
        String packagePath = (String) packageInfo.get(info[1]);
        File convertFile = new File(workspaceDir + File.separatorChar + packagePath + File.separatorChar + info[0]);
        return convertFile.getPath();
    }

    /**
      Query the module's absolute MBD file path with module base name. 
    
      @param moduleName the base name of the module
      @return the absolute MBD file name
      @throws BuildException
              Base name is not registered in any SPD files
    **/
    private synchronized static String getMbdFilename(String moduleName) throws BuildException {
        String[] info = moduleInfo.get(moduleName);
        if (info == null) {
            throw new BuildException("Info: Module base name [" + moduleName + "] can't found in all SPD.");
        }
        String packagePath = (String) packageInfo.get(info[1]);
        File convertFile = new File(workspaceDir + File.separatorChar + packagePath + File.separatorChar + info[0]);
        return convertFile.getPath().substring(0, convertFile.getPath().length() - 4) + ".mbd";
    }

    /**
      Get the current WORKSPACE Directory. 
      @return current workspace directory
    **/
    public synchronized static String getWorkspacePath() {
        return workspaceDir;
    }

    /**
      Query package relative path to WORKSPACE_DIR with package name. 
      
      @param packageName the name of the package
      @return the path relative to WORKSPACE_DIR 
    **/
    public synchronized static String getPackagePath(String packageName) {
        return (String) packageInfo.get(packageName);
    }

    /**
      Query package (which the module belongs to) relative path to WORSPACE_DIR.  
    
      @param moduleName the base name of the module
      @return the relative path to WORKSPACE_DIR of the package which the module belongs to
    **/
    public synchronized static String getPackagePathForModule(String moduleName) {
        String[] info = moduleInfo.get(moduleName);
        String packagePath = (String) packageInfo.get(info[1]);
        return packagePath;
    }

    /**
      Query the package name which the module belongs to with the module's base name.
    
      @param moduleName the base name of the module
      @return the package name which the module belongs to
    **/
    public synchronized static String getPackageNameForModule(String moduleName) {
        return moduleInfo.get(moduleName)[1];
    }

    /**
      Parse framework database (DB) and all SPD files listed in DB to initialize
      the environment for next build. This method will only be executed only once
      in the whole build process.  
      
      @param workspaceDatabaseFile the file name of framework database
      @param workspaceDir current workspace directory path
      @throws BuildException
              Framework Dababase or SPD or MSA file is not valid
    **/
    public synchronized static void initInfo(String workspaceDatabaseFile, String workspaceDir) throws BuildException {
        if (globalFlag) {
            return;
        }
        globalFlag = true;
        GlobalData.workspaceDir = workspaceDir;
        File dbFile = new File(workspaceDir + File.separatorChar + workspaceDatabaseFile);
        try {
            FrameworkDatabaseDocument db = (FrameworkDatabaseDocument) XmlObject.Factory.parse(dbFile);
            List<PackageListDocument.PackageList.Package> packages = db.getFrameworkDatabase().getPackageList()
                                                                       .getPackageList();
            Iterator iter = packages.iterator();
            while (iter.hasNext()) {
                PackageListDocument.PackageList.Package packageItem = (PackageListDocument.PackageList.Package) iter
                                                                                                                    .next();
                String name = packageItem.getPackageNameArray(0).getStringValue();
                String path = packageItem.getPathArray(0).getStringValue();
                packageInfo.put(name, path);
                File spdFile = new File(workspaceDir + File.separatorChar + path + File.separatorChar + name + ".spd");
                initPackageInfo(spdFile.getPath(), name);
                // 
                // SPD Parse.
                //
                PackageSurfaceAreaDocument spdDoc = (PackageSurfaceAreaDocument) XmlObject.Factory.parse(spdFile);
                Spd spd = new Spd(spdDoc, path);
                spdTable.put(name, spd);

            }
        } catch (Exception e) {
            throw new BuildException("Parse workspace Database [" + dbFile.getPath() + "] Error.\n" + e.getMessage());
        }
    }

    /**
      Parse every MSA files, get base name from MSA Header. And record those
      values to ModuleInfo.
      
      @param packageFilename the file name of the package
      @param packageName the name of the package
      @throws BuildException
              SPD or MSA file is not valid
    **/
    private synchronized static void initPackageInfo(String packageFilename, String packageName) throws BuildException {
        File packageFile = new File(packageFilename);
        try {
            PackageSurfaceAreaDocument spd = (PackageSurfaceAreaDocument) XmlObject.Factory.parse(packageFile);
            List<FilenameDocument.Filename> msaFilenameList;

            List<MsaFilesDocument.MsaFiles.MsaFile> msasList = spd.getPackageSurfaceArea().getMsaFiles()
                                                                  .getMsaFileList();
            if (msasList.size() == 0) {
                msaFilenameList = spd.getPackageSurfaceArea().getMsaFiles().getFilenameList();
            } else {
                msaFilenameList = new ArrayList<FilenameDocument.Filename>(msasList.size());
                Iterator msasIter = msasList.iterator();
                while (msasIter.hasNext()) {
                    MsaFilesDocument.MsaFiles.MsaFile msaFile = (MsaFilesDocument.MsaFiles.MsaFile)msasIter.next();
                    msaFilenameList.add(msaFile.getFilename());
                }
            }

            Iterator msaFilenameIter = msaFilenameList.iterator();
            while (msaFilenameIter.hasNext()) {
                FilenameDocument.Filename msaFilename = (FilenameDocument.Filename)msaFilenameIter.next();
                String filename = msaFilename.getStringValue();
                File msaFile = new File(workspaceDir + File.separatorChar + GlobalData.getPackagePath(packageName)
                                        + File.separatorChar + filename);
                SurfaceAreaParser surfaceAreaParser = new SurfaceAreaParser();
                Map<String, XmlObject> map = surfaceAreaParser.parseFile(msaFile);
                String baseName = "";
                XmlObject header = null;
                if ((header = map.get("MsaHeader")) != null) {
                    if (((MsaHeader) header).isSetBaseName()) {
                        baseName = ((MsaHeader) header).getBaseName().getStringValue();
                    } else {
                        baseName = ((MsaHeader) header).getModuleName();
                    }
                } else if ((header = map.get("MsaLibHeader")) != null) {
                    baseName = ((MsaLibHeader) header).getBaseName().getStringValue();
                } else {
                    continue;
                }
                nativeMsa.put(baseName, map);
                String[] info = { filename, packageName };
                moduleInfo.put(baseName, info);
            }
        } catch (Exception e) {
            throw new BuildException("Parse package description file [" + packageFile.getPath() + "] Error.\n"
                                     + e.getMessage());
        }
    }

    /**
      Query the libraries which the module depends on.
    
      @param moduleName the base name of the module
      @return the libraries which the module depends on
    **/
    public synchronized static String[] getModuleLibrary(String moduleName, String arch) {
        Set<String> set = moduleLibraryMap.get(moduleName + "-" + arch);
        return set.toArray(new String[set.size()]);
    }

    /**
      Register module's library list which it depends on for later use. 
      
      @param moduleName the base name of the module
      @param libraryList the libraries which the module depends on
    **/
    public synchronized static void addModuleLibrary(String moduleName, String arch, Set<String> libraryList) {
        moduleLibraryMap.put(moduleName + "-" + arch, libraryList);
    }

    /**
      Query the library absolute file name with library name. 
      
      @param library the base name of the library
      @return the library absolute file name
    **/
    public synchronized static String getLibrary(String library, String arch) {
        return libraries.get(library + "-" + arch);
    }

    /**
      Register library absolute file name for later use.
      
      @param library the base name of the library
      @param resultPath the library absolute file name
    **/
    public synchronized static void addLibrary(String library, String arch, String resultPath) {
        libraries.put(library + "-" + arch, resultPath);
    }

    /**
      Whether the module with ARCH has built in the previous build. 
      
      @param moduleName the base name of the module
      @param arch current build ARCH
      @return true if the module has built in previous, otherwise return false
    **/
    public synchronized static boolean isModuleBuilt(String moduleName, String arch) {
        return builtModules.contains(moduleName + "-" + arch);
    }

    /**
      Register the module with ARCH has built. 
    
      @param moduleName the base name of the module
      @param arch current build ARCH
    **/
    public synchronized static void registerBuiltModule(String moduleName, String arch) {
        builtModules.add(moduleName + "-" + arch);
    }

    /**
      Whether the module's surface area has parsed in the previous build.
      
      @param moduleName the base name of the module
      @return true if the module's surface area has parsed in previous, otherwise
      return false
    **/
    public synchronized static boolean isModuleParsed(String moduleName) {
        return parsedModules.containsKey(moduleName);
    }

    /**
      Query overrided module surface area information. If current is Package
      or Platform build, also include the information from FPD file. 
      
      <p>Note that surface area parsing is incremental. That means the method will 
      only to parse the MSA and MBD files when never parsed before. </p>
    
      @param moduleName the base name of the module
      @return the overrided module surface area information
      @throws BuildException
              MSA or MBD is not valid
    **/
    public synchronized static Map<String, XmlObject> getDoc(String moduleName) throws BuildException {
        if (parsedModules.containsKey(moduleName)) {
            return parsedModules.get(moduleName);
        }
        Map<String, XmlObject> msaMap = getNativeMsa(moduleName);
        Map<String, XmlObject> mbdMap = getNativeMbd(moduleName);
        OverrideProcess op = new OverrideProcess();
        Map<String, XmlObject> map = op.override(mbdMap, msaMap);
        //
        // IF IT IS A PALTFORM BUILD, OVERRIDE FROM PLATFORM
        //
        if (FpdParserTask.platformBuildOptions != null) {
            Map<String, XmlObject> platformMap = new HashMap<String, XmlObject>();
            platformMap.put("BuildOptions", FpdParserTask.platformBuildOptions);
            Map<String, XmlObject> overrideMap = op.override(platformMap, OverrideProcess.deal(map));
            GlobalData.registerModule(moduleName, overrideMap);
            return overrideMap;
        } else {
            parsedModules.put(moduleName, map);
            return map;
        }
    }

    /**
      Query the native MSA information with module base name. 
      
      <p>Note that MSA parsing is incremental. That means the method will 
      only to parse the MSA files when never parsed before. </p>
      
      @param moduleName the base name of the module
      @return the native MSA information
      @throws BuildException
              MSA file is not valid
    **/
    public synchronized static Map<String, XmlObject> getNativeMsa(String moduleName) throws BuildException {
        if (nativeMsa.containsKey(moduleName)) {
            return nativeMsa.get(moduleName);
        }
        String msaFilename = getMsaFilename(moduleName);
        File msaFile = new File(msaFilename);
        if (!msaFile.exists()) {
            throw new BuildException("Info: Surface Area file [" + msaFile.getPath() + "] can't found.");
        }
        SurfaceAreaParser surfaceAreaParser = new SurfaceAreaParser();
        Map<String, XmlObject> map = surfaceAreaParser.parseFile(msaFile);
        nativeMsa.put(moduleName, map);
        return map;
    }
    
    /**
      Query the native MBD information with module base name. 
      
      <p>Note that MBD parsing is incremental. That means the method will 
      only to parse the MBD files when never parsed before. </p>
    
      @param moduleName the base name of the module
      @return the native MBD information
      @throws BuildException
              MBD file is not valid
    **/
    public synchronized static Map<String, XmlObject> getNativeMbd(String moduleName) throws BuildException {
        if (nativeMbd.containsKey(moduleName)) {
            return nativeMbd.get(moduleName);
        }
        String mbdFilename = getMbdFilename(moduleName);
        File mbdFile = new File(mbdFilename);
        if (!mbdFile.exists()) {
            return null;
            //throw new BuildException("Info: Surface Area file [" + mbdFile.getPath() + "] can't found.");
        }
        SurfaceAreaParser surfaceAreaParser = new SurfaceAreaParser();
        Map<String, XmlObject> map = surfaceAreaParser.parseFile(mbdFile);
        nativeMbd.put(moduleName, map);
        return map;
    }

    /**
      Register module overrided surface area information. If has existed, then update.
      
      @param moduleName the base name of the module
      @param map the overrided surface area information
    **/
    public synchronized static void registerModule(String moduleName, Map<String, XmlObject> map) {
        parsedModules.put(moduleName, map);
    }

    /**
     * 
     * @param protocolName
     * @return
     */
    public synchronized static String[] getProtocolInfoGuid(String protocolName) {
        Set set = spdTable.keySet();
        Iterator iter = set.iterator();
        String[] cNameGuid = null;

        while (iter.hasNext()) {
            Spd spd = (Spd) spdTable.get(iter.next());
            cNameGuid = spd.getProtocolNameGuidArray(protocolName);
            if (cNameGuid != null) {
                break;
            }
        }
        return cNameGuid;
    }

    public synchronized static String[] getPpiInfoGuid(String ppiName) {
        Set set = spdTable.keySet();
        Iterator iter = set.iterator();
        String[] cNameGuid = null;

        while (iter.hasNext()) {
            Spd spd = (Spd) spdTable.get(iter.next());
            cNameGuid = spd.getPpiCnameGuidArray(ppiName);

            if (cNameGuid != null) {
                break;
            }
        }
        return cNameGuid;
    }

    /**
     * 
     * @param guidName
     * @return
     */
    public synchronized static String[] getGuidInfoGuid(String guidName) {
        String[] cNameGuid = null;
        Set set = spdTable.keySet();
        Iterator iter = set.iterator();

        while (iter.hasNext()) {
            Spd spd = (Spd) spdTable.get(iter.next());
            cNameGuid = spd.getGuidNameArray(guidName);
            if (cNameGuid != null) {
                break;
            }
        }
        return cNameGuid;
    }

    public synchronized static String getLibClassIncluder(String libName) {
        String libIncluder = null;
        Set set = spdTable.keySet();
        Iterator iter = set.iterator();

        while (iter.hasNext()) {
            String packageName = (String) iter.next();
            Spd spd = (Spd) spdTable.get(packageName);
            libIncluder = spd.getLibClassIncluder(libName);
            String packagePath = spd.packagePath;
            if (packagePath != null) {
                packagePath = packagePath.replace('\\', File.separatorChar);
                packagePath = packagePath.replace('/', File.separatorChar);
            } else {
                packagePath = packageName;
            }
            if (libIncluder != null) {
                libIncluder = libIncluder.replace('\\', File.separatorChar);
                libIncluder = libIncluder.replace('/', File.separatorChar);
                libIncluder = packageName + File.separatorChar + libIncluder;
                break;
            }
        }
        return libIncluder;
    }

    public synchronized static String getModuleInfoByPackageName(String packageName, String moduleType) {
        Spd spd;
        String includeFile = null;
        String includeStr = "";
        String cleanPath = "";

        spd = (Spd) spdTable.get(packageName);
        includeFile = spd.getModuleTypeIncluder(moduleType);
        if (includeFile != null) {
            includeFile = includeFile.replace('\\', File.separatorChar);
            includeFile = includeFile.replace('/', File.separatorChar);
            includeStr = CommonDefinition.include + " <" + includeStr;
            cleanPath = spd.packagePath;
            cleanPath = cleanPath.replace('\\', File.separatorChar);
            cleanPath = cleanPath.replace('/', File.separatorChar);

            if (cleanPath.charAt(spd.packagePath.length() - 1) != File.separatorChar) {
                cleanPath = cleanPath + File.separatorChar;
            }
            includeStr = includeStr + cleanPath;
            includeStr = includeStr + includeFile;
            includeStr = includeStr + ">\r\n";
        }

        return includeStr;
    }

    public synchronized static void setLibInstanceInfo(String libName, String libConstructor, String libDesturctor) {
        String[] libConsDes = new String[2];
        libConsDes[0] = libConstructor;
        libConsDes[1] = libDesturctor;

        libInstanceInfo.put(libName, libConsDes);
    }

    public synchronized static boolean isHaveLibInstance(String libName) {
        return libInstanceInfo.containsKey(libName);
    }

    public synchronized static String getLibInstanceConstructor(String libName) {
        String[] libInstanceValue;
        libInstanceValue = libInstanceInfo.get(libName);
        if (libInstanceValue != null) {
            return libInstanceValue[0];
        } else {
            return null;
        }
    }

    public synchronized static String getLibInstanceDestructor(String libName) {
        String[] libInstanceValue;
        libInstanceValue = libInstanceInfo.get(libName);
        if (libInstanceValue != null) {
            return libInstanceValue[1];
        } else {
            return null;
        }
    }

    public synchronized static MemoryDatabaseManager getPCDMemoryDBManager() {
        return pcdDbManager;
    }
}
