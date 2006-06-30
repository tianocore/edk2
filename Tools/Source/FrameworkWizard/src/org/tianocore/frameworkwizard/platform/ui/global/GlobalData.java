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
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.Stack;
import java.util.TreeMap;
import java.util.TreeSet;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

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
    private static boolean globalFlag = false;
    
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
    
    ///
    /// PCD memory database stored all PCD information which collected from FPD,MSA and SPD.
    ///
//    private static final MemoryDatabaseManager pcdDbManager = new MemoryDatabaseManager();

    ///
    /// build target + tool chain family/tag name + arch + command types + command options
    ///
    private static Map<String, Object> toolChainOptions;
    private static Map<String, Object> toolChainFamilyOptions;
    private static Map<String, String> toolChainDefinitions;
    private static Map<FpdModuleIdentification, Map<String, Object>> moduleToolChainOptions = new HashMap<FpdModuleIdentification, Map<String, Object>>();;
    private static Map<FpdModuleIdentification, Map<String, Object>> moduleToolChainFamilyOptions = new HashMap<FpdModuleIdentification, Map<String, Object>>();;
    ///
    ///
    ///
    private static Set<String> targets;
    ///
    ///
    ///
    private static Set<String> toolChainFamilies;
    ///
    ///
    ///
    private static Set<String> toolChains;
    ///
    /// keep track which toolchain family a toolchain tag belongs to
    ///
    private static Map<String, Set<String>> toolChainFamilyMap;
    private static Map<String, Set<String>> toolChainCommandMap;
    
    ///
    /// list of Arch: EBC, ARM, IA32, X64, IPF, PPC
    ///
    private static Set<String> archs;

    ///
    /// list of Command Type: CC, LIB, LINK, ASL, ASM, ASMLINK, PP
    ///
    private static Set<String> commandTypes;
    
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
        // ensure this method will be revoked only once
        //
        if (globalFlag) {
            return;
        }
        globalFlag = true;
        
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
        if (! msaFile.exists()) {
            throw new Exception("Surface Area file [" + msaFile.getPath() + "] can't found.");
        }
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

    /**
      The header file path is relative to workspace dir
    **/
    public static String[] getLibraryClassHeaderFiles(PackageIdentification[] packages, String name) {
        if (packages == null ){
            // throw Exception or not????
            return new String[0];
        }
        String[] result = null;
        for (int i = 0; i < packages.length; i++){
            Spd spd = spdTable.get(packages[i]);
            //
            // If find one package defined the library class
            //
            if( (result = spd.getLibClassIncluder(name)) != null){
                return result;
            } 
        }
        return null;
        
    }
    
    /**
      The header file path is relative to workspace dir
    **/    
    public static String getPackageHeaderFiles(PackageIdentification packages, String moduleType) throws Exception {
        if (packages == null ){
            return new String("");
        }
        Spd spd = spdTable.get(packages);
        //
        // If can't find package header file, skip it
        //
        String temp = null;
        if (spd != null){
        	if( (temp = spd.getPackageIncluder(moduleType)) != null){
                return temp;
            }else {
            	temp = "";
            	return temp;
            }
        }else {
        	return null;
        }
    }   
    
    /**
      return two values: {cName, GuidValue}
    **/
    public static String[] getGuid(PackageIdentification[] packages, String name) throws Exception {
        if (packages == null ){
            // throw Exception or not????
            return new String[0];
        }
        String[] result = null;
        for (int i = 0; i < packages.length; i++){
            Spd spd = spdTable.get(packages[i]);
            //
            // If find one package defined the GUID
            //
            if( (result = spd.getGuid(name)) != null){
                return result;
            }
        }
        return null;
    }
    
    /**
      return two values: {cName, GuidValue}
    **/    
    public static String[] getPpiGuid(PackageIdentification[] packages, String name) throws Exception {
        if (packages == null ){
            return new String[0];
        }
        String[] result = null;
        for (int i = 0; i < packages.length; i++){
            Spd spd = spdTable.get(packages[i]);
            //
            // If find one package defined the Ppi GUID
            //
            if( (result = spd.getPpi(name)) != null){
                return result;
            }
        }
        return null;
        
    }
    
    /**
      return two values: {cName, GuidValue}
    **/   
    public static String[] getProtocolGuid(PackageIdentification[] packages, String name) throws Exception {
        if (packages == null ){
            return new String[0];
        }
        String[] result = null;
        for (int i = 0; i < packages.length; i++){
            Spd spd = spdTable.get(packages[i]);
            //
            // If find one package defined the protocol GUID
            //
            if( (result = spd.getProtocol(name)) != null){
                return result;
            }
        }
        return null;
        
    }
    
    /////////////////////////// Update!! Update!! Update!!
//    public synchronized static MemoryDatabaseManager getPCDMemoryDBManager() {
//        return pcdDbManager;
//    }
    ///////////////////////////
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

    ////// Tool Chain Related, try to refine and put some logic process to ToolChainFactory
    public static void setBuildToolChainFamilyOptions(Map<String, Object> map) {
        toolChainFamilyOptions = map;
    }

    public static Map<String, Object> getToolChainFamilyOptions() {
        return toolChainFamilyOptions;
    }

    public static void setBuildToolChainOptions(Map<String, Object> map) {
        toolChainOptions = map;
    }

    public static Map<String, Object> getToolChainOptions() {
        return toolChainOptions;
    }

    public static void setTargets(Set<String> targetSet) {
        GlobalData.log.info("TargetSet: " + targetSet);
        targets = targetSet;
    }

    public static String[] getTargets() {
        return (String[])targets.toArray(new String[targets.size()]);
    }

    public static void setToolChains(Set<String> toolChainSet) {
        toolChains = toolChainSet;
    }

    public static String[] getToolChains() {
        String[] toolChainList = new String[toolChains.size()];
        return (String[])toolChains.toArray(toolChainList);
    }

    public static void setToolChainFamilies(Set<String> toolChainFamilySet) {
        toolChainFamilies = toolChainFamilySet;
    }

    public static void setToolChainFamiliyMap(Map<String, Set<String>> map) {
        /*
        Set<String> keys = map.keySet();
        Iterator it = keys.iterator();
        while (it.hasNext()) {
            String toolchain = (String)it.next();
            Set<String> familyMap = (Set<String>)map.get(toolchain);
            Iterator fit = familyMap.iterator();
            System.out.print(toolchain + ": ");
            while (fit.hasNext()) {
                System.out.print((String)fit.next() + " ");
            }
            System.out.println("");
        }
        */
        toolChainFamilyMap = map;
    }

    public static String[] getToolChainFamilies() {
        String[] toolChainFamilyList = new String[toolChainFamilies.size()];
        return (String[])toolChainFamilies.toArray(toolChainFamilyList);
    }

    public static String[] getToolChainFamilies(String toolChain) {
        Set<String> familySet = (Set<String>)toolChainFamilyMap.get(toolChain);
        String[] toolChainFamilyList = new String[familySet.size()];
        return (String[])familySet.toArray(toolChainFamilyList);
    }

    public static Set<String> getToolChainFamilySet(String toolChain) {
        return (Set<String>)toolChainFamilyMap.get(toolChain);
    }

    public static void setArchs(Set<String> archSet) {
        archs = archSet;
    }

    public static String[] getArchs() {
        String[] archList = new String[archs.size()];
        return (String[])archs.toArray(archList);
    }
    /*

     */
    public static void SetCommandTypes(Set<String> commandTypeSet) {
        commandTypes = commandTypeSet;
    }
    /*

     */
    public static void SetCommandTypes(Map<String, Set<String>> commandTypeMap) {
        toolChainCommandMap = commandTypeMap;
    }
    /*

     */
    public static String[] getCommandTypes() {
        String[] commandList = new String[commandTypes.size()];
        return (String[])commandTypes.toArray(commandList);
    }
    /*

     */
    public static String[] getCommandTypes(String toolChain) {
        Set<String> commands = (Set<String>)toolChainCommandMap.get(toolChain);
        if (commands == null) {
            return new String[0];
        }

        String[] commandList = new String[commands.size()];
        return (String[])commands.toArray(commandList);
    }
    /*

     */
    public static String getCommandSetting(String target, String toolChain, 
        String arch, String command, String attribute, FpdModuleIdentification fpdModuleId) {
        String[] commandDescription = new String[] {target, toolChain, arch, command, attribute};
        return getCommandSetting(commandDescription, fpdModuleId);
    }
    /*

     */
    public static String getCommandSetting(String[] commandDescription, FpdModuleIdentification fpdModuleId) {
        if (commandDescription[4].equals("FLAGS")) {
            return getCommandFlags(commandDescription, fpdModuleId);
        }

        StringBuffer commandDescString = new StringBuffer(32);

        int i = 0;
        while (true) {
            commandDescString.append(commandDescription[i++]);
            if (i >= commandDescription.length) {
                break;
            }
            commandDescString.append("_");
        }

        return getCommandSetting(commandDescString.toString());
    }
    /*

     */
    public static String getCommandSetting(String commandDescString) {
        return (String)toolChainDefinitions.get(commandDescString);
    }
    /*

     */
    public static String getCommandFlags(String[] commandDescription, FpdModuleIdentification fpdModuleId) {
        String setting = getSetting(toolChainOptions, commandDescription, fpdModuleId, false);

        if (setting == null) {
            String commandDesc = commandDescription[4];
            commandDescription[4] = "FAMILY";
            String toolChainFamily = getCommandSetting(commandDescription, fpdModuleId);
            commandDescription[4] = commandDesc;

            commandDesc = commandDescription[1];
            commandDescription[1] = toolChainFamily;
            setting = getSetting(toolChainFamilyOptions, commandDescription, fpdModuleId, true);
            commandDescription[1] = commandDesc;
        }

        if (setting == null) {
            setting = "";
        }
        
        
        Set<String> addFlagsSet = new LinkedHashSet<String>();
        Set<String> subFlagsSet = new LinkedHashSet<String>();
        putFlagsToSet(addFlagsSet, setting);

        return getFlags(addFlagsSet, subFlagsSet);
    }
    /*

     */
    private static String getSetting(Map<String, Object> optionMap, String[] commandDescription, 
        FpdModuleIdentification fpdModuleId, boolean toolChainFamilyFlag) {

        String setting = (String)getOption(optionMap, commandDescription);
        if (fpdModuleId == null) {
            return setting;
        }
        //
        // module overrides
        //
        //
        // get module xml doc
        //
        Map<String, XmlObject> fpdModule = (Map<String, XmlObject>)fpdModuleSA.get(fpdModuleId);
        if (fpdModuleId == null) {
            return setting;
        }
        SurfaceAreaQuery.push(fpdModule);
        //
        // check if the module has been parsed
        //
        Map<String, Object> moduleOptions = (Map<String, Object>)moduleToolChainOptions.get(fpdModuleId);
        if (moduleOptions == null) {
            //
            // get all the build options of this module
            //
            moduleOptions = new TreeMap<String, Object>(comparator);
            parseBuildOptions(moduleOptions, SurfaceAreaQuery.getOptions(toolChainFamilyFlag));
        }
        //
        // get setting for current qualified command
        //
        Set<String> addSet = new TreeSet<String>();
        Set<String> subSet = new TreeSet<String>();
        putFlagsToSet(addSet, setting);
        String moduleSetting = getOption(moduleOptions, commandDescription);
        if (moduleSetting != null) {
            moduleSetting = parseOptionString(moduleSetting, addSet, subSet);
        }
        //
        // do necessary setting override
        //
        if (moduleSetting == null) {
            setting = getRawFlags(addSet, subSet);
        } else {
            setting = moduleSetting;
        }

        SurfaceAreaQuery.pop();
        return setting;
    }
    /*

     */
    public static void setToolChainDefinitions(Map<String, String> def) {
        toolChainDefinitions = def;
    }

    public static Map<String, String> getToolChainDefinitions() {
        return toolChainDefinitions;
    }

    /**
      Separate the string and instore in set.
       
      <p> String is separated by Java Regulation Expression 
      "[^\\\\]?(\".*?[^\\\\]\")[ \t,]+". </p>
      
      <p>For example: </p>
      
      <pre>
        "/nologo", "/W3", "/WX"
        "/C", "/DSTRING_DEFINES_FILE=\"BdsStrDefs.h\""
      </pre>
      
      @param set store the separated string
      @param str string to separate
    **/
    private static void putFlagsToSet(Set<String> set, String str) {
        if (str == null || str.length() == 0) {
            return;
        }

        Pattern myPattern = Pattern.compile("[^\\\\]?(\".*?[^\\\\]\")[ \t,]+");
        Matcher matcher = myPattern.matcher(str + " ");
        while (matcher.find()) {
            String item = str.substring(matcher.start(1), matcher.end(1));
            set.add(item);
        }
    }
    
    /**
      Generate the final flags string will be used by compile command. 
      
      @param add the add flags set
      @param sub the sub flags set
      @return final flags after add set substract sub set
    **/
    private static String getFlags(Set<String> add, Set<String> sub) {
        String result = "";
        add.removeAll(sub);
        Iterator iter = add.iterator();
        while (iter.hasNext()) {
            String str = (String) iter.next();
            result += str.substring(1, str.length() - 1) + " ";
        }
        return result;
    }

    /**
      Generate the flags string with original format. The format is defined by 
      Java Regulation Expression "[^\\\\]?(\".*?[^\\\\]\")[ \t,]+". </p>
      
      <p>For example: </p>
      
      <pre>
        "/nologo", "/W3", "/WX"
        "/C", "/DSTRING_DEFINES_FILE=\"BdsStrDefs.h\""
      </pre>
      
      @param add the add flags set
      @param sub the sub flags set
      @return flags with original format
    **/
    private static String getRawFlags(Set<String> add, Set<String> sub) {
        String result = null;
        add.removeAll(sub);
        Iterator iter = add.iterator();
        while (iter.hasNext()) {
            String str = (String) iter.next();
            result += "\"" + str.substring(1, str.length() - 1) + "\", ";
        }
        return result;
    }

    private static String parseOptionString(String optionString, Set<String> addSet, Set<String> subSet) {
        boolean overrideOption = false;
        Pattern pattern = Pattern.compile("ADD\\.\\[(.+)\\]");
        Matcher matcher = pattern.matcher(optionString);

        while (matcher.find()) {
            overrideOption = true;
            String addOption = optionString.substring(matcher.start(1), matcher.end(1)).trim();
            putFlagsToSet(addSet, addOption);
            
        }

        pattern = Pattern.compile("SUB\\.\\[(.+)\\]");
        matcher = pattern.matcher(optionString);

        while (matcher.find()) {
            overrideOption = true;
            String subOption = optionString.substring(matcher.start(1), matcher.end(1)).trim();
            putFlagsToSet(subSet, subOption);
        }

        if (overrideOption == true) {
            return null;
        }

        return optionString;
    }

    public static String getOption(Map<String, Object> options, String[] toolDefString) {
        Stack<Map<String, Object>> stack = new Stack<Map<String, Object>>();
        Map<String, Object> map = options;
        Map<String, Object> lastMap;
        String option = null;
        int length = toolDefString.length - 2;

        int i = 0;
        String key = null;
        boolean backtrack = false;
        while (true) {
            if (map == null) {
                if (stack.empty()) {
                    break;
                }
                map = (Map<String, Object>)stack.pop();
                if (backtrack) {
                    --i;
                }
                key = "*";
                backtrack = true;
            } else {
                if (i >= length) {
                    break;
                }
                key = toolDefString[i];
                stack.push(map);
                ++i;
                backtrack = false;
            }
            lastMap = map;
            map = (Map<String, Object>)lastMap.get(key);
        }

        if (map != null) {
            option = (String)map.get(toolDefString[i]);
        }

        return option;
    }

    private static void parseBuildOptions(Map<String, Object> optionMap, String[][] options) {
        Map<String, Object> map;
        Map<String, Object> nextMap;

        for (int i = 0; i < options.length; ++i) {
            map = optionMap;

            int flagIndex = options[i].length - 1;
            int cmdIndex = flagIndex - 1;
            int archIndex = cmdIndex - 1;
            for (int j = 0; j < cmdIndex; ++j) {
                String s = options[i][j];
                if (s == null || s.trim().length() == 0) {
                    s = "*";
                }
                s = s.trim().toUpperCase();

                nextMap = (Map<String, Object>)map.get(s);
                if (nextMap == null) {
                    nextMap = new HashMap<String, Object>();
                    map.put(s, nextMap);
                }

                map = nextMap;
            }

            String cmd = options[i][cmdIndex];
            String flag = options[i][flagIndex];
            if (cmd == null || cmd.trim().length() == 0) {
                cmd = "*";
            }
            if (flag == null) {
                flag = "";
            }
            map.put(cmd.trim().toUpperCase(), flag.trim().toUpperCase());
        }
    }

}

final class KeyComparator implements Comparator<String> {
    public int compare(String x, String y) {
        return x.compareToIgnoreCase(y);
    }
    
}

