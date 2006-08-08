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
package org.tianocore.frameworkwizard.platform.ui.global;

import org.apache.xmlbeans.XmlObject;
import org.tianocore.DbPathAndFilename;
import org.tianocore.FrameworkDatabaseDocument;
import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.PcdCodedDocument;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.frameworkwizard.platform.ui.id.FpdModuleIdentification;
import org.tianocore.frameworkwizard.platform.ui.id.ModuleIdentification;
import org.tianocore.frameworkwizard.platform.ui.id.PackageIdentification;
import org.tianocore.frameworkwizard.platform.ui.id.PlatformIdentification;

import java.io.File;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.Set;
import java.util.Vector;
import java.util.logging.Logger;

/**
  GlobalData provide initializing, instoring, querying and update global data.
  It is a bridge to intercommunicate between multiple component, such as AutoGen,
  PCD and so on. 
  
  <p>Note that all global information are initialized incrementally. All data will 
  parse and record only of necessary during build time. </p>
  
  @since GenBuild 1.0
**/
public class GlobalData {


    public static Logger log = Logger.getAnonymousLogger();
    public static KeyComparator comparator = new KeyComparator();
    ///
    /// Record current WORKSPACE Directory
    ///
    private static String workspaceDir = "";
    
    ///
    /// Be used to ensure Global data will be initialized only once.
    ///
//    private static boolean globalFlag = false;
    
    ///
    /// Framework Database information: package list and platform list
    ///
    private static Set<PackageIdentification> packageList = new HashSet<PackageIdentification>();  

    private static Set<PlatformIdentification> platformList = new HashSet<PlatformIdentification>();

    ///
    /// Every detail SPD informations: Module list, Library class definition,
    ///   Package header file, GUID/PPI/Protocol definitions
    ///
    private static final Map<PackageIdentification, Spd> spdTable = new HashMap<PackageIdentification, Spd>();

    ///
    /// Build informations are divided into three parts:
    /// 1. From MSA 2. From FPD 3. From FPD' ModuleSA
    ///
    private static Map<ModuleIdentification, Map<String, XmlObject>> nativeMsa = new HashMap<ModuleIdentification, Map<String, XmlObject>>();

    private static Map<FpdModuleIdentification, Map<String, XmlObject>> fpdModuleSA= new HashMap<FpdModuleIdentification, Map<String, XmlObject>>();

    private static XmlObject fpdBuildOptions;

    private static XmlObject fpdDynamicPcds;
    
    ///
    /// Parsed modules list
    ///
    private static Map<FpdModuleIdentification, Map<String, XmlObject>> parsedModules = new HashMap<FpdModuleIdentification, Map<String, XmlObject>>();
    
    ///
    /// built modules list with ARCH, TARGET, TOOLCHAIN
    ///
    private static Set<FpdModuleIdentification> builtModules = new HashSet<FpdModuleIdentification>();
    
    /**
      Parse framework database (DB) and all SPD files listed in DB to initialize
      the environment for next build. This method will only be executed only once
      in the whole build process.  
    
      @param workspaceDatabaseFile the file name of framework database
      @param workspaceDir current workspace directory path
      @throws Exception
            Framework Dababase or SPD or MSA file is not valid
    **/
    public synchronized static void initInfo(String workspaceDatabaseFile, String workspaceDir) throws Exception {
        
        //
        // Backup workspace directory. It will be used by other method
        //
        GlobalData.workspaceDir = workspaceDir.replaceAll("(\\\\)", "/");
        File dbFile = new File(workspaceDir + File.separatorChar + workspaceDatabaseFile);
        try {
            FrameworkDatabaseDocument db = (FrameworkDatabaseDocument) XmlObject.Factory.parse(dbFile);
            //
            // validate FrameworkDatabaseFile
            //
//            if (! db.validate()) {
//                throw new Exception("Framework Database file [" + dbFile.getPath() + "] is invalid.");
//            }
            //
            // Get package list
            //
            packageList.clear();
            List<DbPathAndFilename> packages = db.getFrameworkDatabase().getPackageList().getFilenameList();
            
            Iterator iter = packages.iterator();
            while (iter.hasNext()) {
                DbPathAndFilename dbPath = (DbPathAndFilename)iter.next();
                String fileName = dbPath.getStringValue();
                Spd spd = new Spd(new File(workspaceDir + File.separatorChar + fileName));
                packageList.add(spd.getPackageId());
                spdTable.put(spd.getPackageId(), spd);
            }

            
        } catch (Exception e) {
            e.printStackTrace();
            throw new Exception("Parse workspace Database [" + dbFile.getPath() + "] Error.\n" + e.getMessage());
        }
    }
    
    /**
      Get the current WORKSPACE Directory. 
      
      @return current workspace directory
    **/
    public synchronized static String getWorkspacePath() {
        return workspaceDir;
    }


    /**
      Get the MSA file name with absolute path
     */
    public synchronized static File getMsaFile(ModuleIdentification moduleId) throws Exception {
        File msaFile = null;
        //
        // TBD. Do only when package is null. 
        //
        Iterator iter = packageList.iterator();
        while (iter.hasNext()) {
            PackageIdentification packageId = (PackageIdentification)iter.next();
            Spd spd = spdTable.get(packageId);
            msaFile = spd.getModuleFile(moduleId);
            if (msaFile != null ) {
                break ;
            }
        }
        if (msaFile == null){
            throw new Exception("Can't find Module [" + moduleId.getName() + "] in all packages. ");
        }
        else {
            return msaFile;
        }
    }

    public synchronized static PackageIdentification getPackageForModule(ModuleIdentification moduleId) {
        //
        // If package already defined in module
        //
        if (moduleId.getPackage() != null) {
            return moduleId.getPackage();
        }
        
        PackageIdentification packageId = null;
        Iterator iter = packageList.iterator();
        while (iter.hasNext()) {
            packageId = (PackageIdentification)iter.next();
            
            Spd spd = spdTable.get(packageId);
            if (spd.getModuleFile(moduleId) != null ) {
                moduleId.setPackage(packageId);
                break ;
            }
        }
        if (packageId == null){
            return null;
        }
        else {
            return packageId;
        }
    }
    
    /**
      Difference between build and parse: ToolChain and Target
    **/
    public synchronized static boolean isModuleBuilt(FpdModuleIdentification moduleId) {
        return builtModules.contains(moduleId);
    }
    
    public synchronized static void registerBuiltModule(FpdModuleIdentification fpdModuleId) {
        builtModules.add(fpdModuleId);
    }

    
    public synchronized static void registerFpdModuleSA(FpdModuleIdentification fpdModuleId, Map<String, XmlObject> doc) throws Exception{
        Map<String, XmlObject> result = new HashMap<String, XmlObject>();
        Set keySet = doc.keySet();
        Iterator iter = keySet.iterator();
        while (iter.hasNext()){
            String key = (String)iter.next();
            XmlObject item = cloneXmlObject(doc.get(key), true);
            result.put(key, item);
        }
        fpdModuleSA.put(fpdModuleId, result);
    }
    
    /**
      Query overrided module surface area information. If current is Package
      or Platform build, also include the information from FPD file. 
      
      <p>Note that surface area parsing is incremental. That means the method will 
      only parse the MSA and MBD files if necessary. </p>
    
      @param moduleName the base name of the module
      @return the overrided module surface area information
      @throws Exception
              MSA or MBD is not valid
    **/
    public synchronized static Map<String, XmlObject> getDoc(FpdModuleIdentification fpdModuleId) throws Exception {
        if (parsedModules.containsKey(fpdModuleId)) {
            return parsedModules.get(fpdModuleId);
        }
        Map<String, XmlObject> doc = new HashMap<String, XmlObject>();
        ModuleIdentification moduleId = fpdModuleId.getModule();
        //
        // First part: get the MSA files info
        //
        doc = getNativeMsa(moduleId);
        
        //
        // Second part: put build options
        //
        doc.put("BuildOptions", fpdBuildOptions);
        
        //
        // Third part: get Module info from FPD, such as Library instances, PCDs
        //
        if (fpdModuleSA.containsKey(fpdModuleId)){
            //
            // merge module info in FPD to final Doc
            // For Library Module, do nothing here
            //
            doc.putAll(fpdModuleSA.get(fpdModuleId));
        }
        parsedModules.put(fpdModuleId, doc);
        return doc;
    }

    public synchronized static Map<String, XmlObject> getDoc(ModuleIdentification moduleId, String arch) throws Exception {
        FpdModuleIdentification fpdModuleId = new FpdModuleIdentification(moduleId, arch);
        return getDoc(fpdModuleId);
    }
    /**
      Query the native MSA information with module base name. 
      
      <p>Note that MSA parsing is incremental. That means the method will 
      only to parse the MSA files when never parsed before. </p>
      
      @param moduleName the base name of the module
      @return the native MSA information
      @throws Exception
              MSA file is not valid
    **/
    public synchronized static Map<String, XmlObject> getNativeMsa(ModuleIdentification moduleId) throws Exception {
        if (nativeMsa.containsKey(moduleId)) {
            return nativeMsa.get(moduleId);
        }
        File msaFile = getMsaFile(moduleId);
        Map<String, XmlObject> msaMap = getNativeMsa(msaFile);
        nativeMsa.put(moduleId, msaMap);
        return msaMap;
    }
    
    public synchronized static Map<String, XmlObject> getNativeMsa(File msaFile) throws Exception {
        
        try {
            ModuleSurfaceAreaDocument doc = (ModuleSurfaceAreaDocument)XmlObject.Factory.parse(msaFile);
            //
            // Validate File if they accord with XML Schema
            //
//            if ( ! doc.validate()){
//                throw new Exception("Module Surface Area file [" + msaFile.getPath() + "] is invalid.");
//            }
            //
            // parse MSA file
            //
            ModuleSurfaceArea msa= doc.getModuleSurfaceArea();
            Map<String, XmlObject> msaMap = new HashMap<String, XmlObject>();
            msaMap.put("ModuleSurfaceArea", msa);
            msaMap.put("MsaHeader", cloneXmlObject(msa.getMsaHeader(), true));
            msaMap.put("LibraryClassDefinitions", cloneXmlObject(msa.getLibraryClassDefinitions(), true));
            msaMap.put("SourceFiles", cloneXmlObject(msa.getSourceFiles(), true));
            msaMap.put("PackageDependencies", cloneXmlObject(msa.getPackageDependencies(), true));
            msaMap.put("Protocols", cloneXmlObject(msa.getProtocols(), true));
            msaMap.put("PPIs", cloneXmlObject(msa.getPPIs(), true));
            msaMap.put("Guids", cloneXmlObject(msa.getGuids(), true));
            msaMap.put("Externs", cloneXmlObject(msa.getExterns(), true));
            return msaMap;
        }
        catch (Exception ex){
            throw new Exception(ex.getMessage());
        }
    }
    
    public static Map<String, XmlObject> getFpdBuildOptions() {
        Map<String, XmlObject> map = new HashMap<String, XmlObject>();
        map.put("BuildOptions", fpdBuildOptions);
        return map;
    }
    
    public static void setFpdBuildOptions(XmlObject fpdBuildOptions) throws Exception{
        GlobalData.fpdBuildOptions = cloneXmlObject(fpdBuildOptions, true);
    }

    public static XmlObject getFpdDynamicPcds() {
        return fpdDynamicPcds;
    }

    public static void setFpdDynamicPcds(XmlObject fpdDynamicPcds) {
        GlobalData.fpdDynamicPcds = fpdDynamicPcds;
    }

    //////////////////////////////////////////////
    //////////////////////////////////////////////
    
    public static Set<ModuleIdentification> getModules(PackageIdentification packageId){
        Spd spd = spdTable.get(packageId);
        if (spd == null ) {
            Set<ModuleIdentification> dummy = new HashSet<ModuleIdentification>();
            return dummy;
        }
        else {
            return spd.getModules();
        }
    }

    public synchronized static PlatformIdentification getPlatform(String name) throws Exception {
        Iterator iter = platformList.iterator();
        while(iter.hasNext()){
            PlatformIdentification platformId = (PlatformIdentification)iter.next();
            if (platformId.getName().equalsIgnoreCase(name)) {
                GlobalData.log.info("Platform: " + platformId + platformId.getFpdFile());
                return platformId;
            }
        }
        throw new Exception("Can't find platform [" + name + "] in current workspace. ");
    }
    
    public synchronized static File getPackageFile(PackageIdentification packageId) throws Exception {
        Iterator iter = packageList.iterator();
        while(iter.hasNext()){
            PackageIdentification packageItem = (PackageIdentification)iter.next();
            if (packageItem.equals(packageId)) {
                packageId.setName(packageItem.getName());
                return packageItem.getSpdFile();
            }
        }
        throw new Exception("Can't find " + packageId + " in current workspace. ");
    }
    
    public synchronized static File getModuleFile(ModuleIdentification moduleId) throws Exception {
        PackageIdentification packageId = getPackageForModule(moduleId);
        moduleId.setPackage(packageId);
        Spd spd = spdTable.get(packageId);
        return spd.getModuleFile(moduleId);
    }
    //
    // expanded by FrameworkWizard
    //
    public synchronized static XmlObject getModuleXmlObject(ModuleIdentification moduleId) throws Exception {
        PackageIdentification packageId = getPackageForModule(moduleId);
        moduleId.setPackage(packageId);
        Spd spd = spdTable.get(packageId);
        return spd.msaDocMap.get(moduleId);
    }
    
    public synchronized static XmlObject getPackageXmlObject(PackageIdentification packageId) {
        Spd spd = spdTable.get(packageId);
        if (spd != null){
            return spd.spdDocMap.get("PackageSurfaceArea");
        }
        return null;
    }
    
    public synchronized static Set<PackageIdentification> getPackageList(){
        return packageList;
    }
    ///// remove!!
    private static XmlObject cloneXmlObject(XmlObject object, boolean deep) throws Exception {
        if ( object == null) {
            return null;
        }
        XmlObject result = null;
        try {
            result = XmlObject.Factory.parse(object.getDomNode()
                            .cloneNode(deep));
        } catch (Exception ex) {
            throw new Exception(ex.getMessage());
        }
        return result;
    }

    public static ModuleIdentification getModuleId(String key){
        //
        // Get ModuleGuid, ModuleVersion, PackageGuid, PackageVersion, Arch into string array.
        //
        String[] keyPart = key.split(" ");
        Set<PackageIdentification> spi = GlobalData.getPackageList();
        Iterator ispi = spi.iterator();
        
        while(ispi.hasNext()) {
            PackageIdentification pi = (PackageIdentification)ispi.next();
            if ( !pi.getGuid().equalsIgnoreCase(keyPart[2])){ 

                continue;
            }
            if (keyPart[3] != null && keyPart[3].length() > 0 && !keyPart[3].equals("null")){
                if(!pi.getVersion().equals(keyPart[3])){
                    continue;
                }
            }
            Set<ModuleIdentification> smi = GlobalData.getModules(pi);
            Iterator ismi = smi.iterator();
            while(ismi.hasNext()) {
                ModuleIdentification mi = (ModuleIdentification)ismi.next();
                if (mi.getGuid().equalsIgnoreCase(keyPart[0])){
                    if (keyPart[1] != null && keyPart[1].length() > 0 && !keyPart[1].equals("null")){
                        if(!mi.getVersion().equals(keyPart[1])){
                            continue;
                        }
                    }

                    return mi;
                }
            }
        }
        return null;
    }
    
    public static Vector<String> getModuleSupArchs(ModuleIdentification mi) throws Exception {
        Vector<String> vArchs = null;
        ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = (ModuleSurfaceAreaDocument.ModuleSurfaceArea)getModuleXmlObject(mi);
        if (msa.getModuleDefinitions() == null || msa.getModuleDefinitions().getSupportedArchitectures() == null) {
            return vArchs;
        }
        ListIterator li = msa.getModuleDefinitions().getSupportedArchitectures().listIterator();
        while (li.hasNext()) {
            if (vArchs == null) {
                vArchs = new Vector<String>();
            }
            vArchs.add((String)li.next());
        }
        
        return vArchs;
    }
    
    public static boolean pcdInMsa (String cName, String tsGuid, ModuleIdentification mi) throws Exception {
        ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = (ModuleSurfaceAreaDocument.ModuleSurfaceArea)getModuleXmlObject(mi);
        if (msa.getPcdCoded() == null || msa.getPcdCoded().getPcdEntryList() == null) {
            return false;
        }
        ListIterator li = msa.getPcdCoded().getPcdEntryList().listIterator();
        while (li.hasNext()) {
            PcdCodedDocument.PcdCoded.PcdEntry msaPcd = (PcdCodedDocument.PcdCoded.PcdEntry)li.next();
            if (msaPcd.getCName().equals(cName) && msaPcd.getTokenSpaceGuidCName().equals(tsGuid)) {
                return true;
            }
        }
        return false;
    }
    
}

final class KeyComparator implements Comparator<String> {
    public int compare(String x, String y) {
        return x.compareToIgnoreCase(y);
    }
    
}

