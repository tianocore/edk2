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
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.logging.Logger;

import org.apache.tools.ant.BuildException;
import org.apache.xmlbeans.XmlObject;

import org.tianocore.common.exception.EdkException;
import org.tianocore.common.logger.EdkLog;
import org.tianocore.pcd.entity.MemoryDatabaseManager;
import org.tianocore.DbPathAndFilename;
import org.tianocore.FrameworkDatabaseDocument;
import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.build.id.FpdModuleIdentification;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.build.id.PackageIdentification;
import org.tianocore.build.id.PlatformIdentification;
import org.tianocore.build.toolchain.ToolChainAttribute;
import org.tianocore.build.toolchain.ToolChainConfig;
import org.tianocore.build.toolchain.ToolChainElement;
import org.tianocore.build.toolchain.ToolChainInfo;
import org.tianocore.build.toolchain.ToolChainKey;
import org.tianocore.build.toolchain.ToolChainMap;

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
    private static final MemoryDatabaseManager pcdDbManager = new MemoryDatabaseManager();

    ///
    /// build target + tool chain family/tag name + arch + command types + command options
    ///
    ///
    /// Tool Chain Data
    /// toolsDef - build tool program information
    /// fpdBuildOption - all modules's build options for tool tag or tool chain families
    /// moduleSaBuildOption - build options for a specific module
    ///
    private static ToolChainConfig toolsDef;

    private static ToolChainInfo toolChainInfo;
    private static ToolChainInfo toolChainEnvInfo;
    private static ToolChainInfo toolChainPlatformInfo;

    private static ToolChainMap platformToolChainOption;
    private static ToolChainMap platformToolChainFamilyOption;

    private static Map<FpdModuleIdentification, ToolChainMap> moduleToolChainOption = new HashMap<FpdModuleIdentification, ToolChainMap>();
    private static Map<FpdModuleIdentification, ToolChainMap> moduleToolChainFamilyOption = new HashMap<FpdModuleIdentification, ToolChainMap>();

    /**
      Parse framework database (DB) and all SPD files listed in DB to initialize
      the environment for next build. This method will only be executed only once
      in the whole build process.

      @param workspaceDatabaseFile the file name of framework database
      @param workspaceDir current workspace directory path
      @throws BuildException
            Framework Dababase or SPD or MSA file is not valid
    **/
    public synchronized static void initInfo(String workspaceDatabaseFile, String workspaceDir, String toolsDefFilename ) throws BuildException {
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

        //
        // Parse tools definition file
        //
        //
        // If ToolChain has been set up before, do nothing.
        // CONF dir + tools definition file name
        //
        File toolsDefFile = new File(workspaceDir + File.separatorChar + toolsDefFilename);
        System.out.println("Using tool definiton file [" + toolsDefFile.getPath() + "].");
        try {
            toolsDef = new ToolChainConfig(toolsDefFile);
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }

        //
        // Parse Framework Database
        //
        File dbFile = new File(workspaceDir + File.separatorChar + workspaceDatabaseFile);
        try {
            FrameworkDatabaseDocument db = (FrameworkDatabaseDocument) XmlObject.Factory.parse(dbFile);
            //
            // validate FrameworkDatabaseFile
            //
            if (!db.validate()) {
                throw new BuildException("Framework Database file [" + dbFile.getPath() + "] format is invalid!");
            }
            //
            // Get package list
            //
            if (db.getFrameworkDatabase().getPackageList() != null ) {
                List<DbPathAndFilename> packages = db.getFrameworkDatabase().getPackageList().getFilenameList();
                Iterator<DbPathAndFilename> iter = packages.iterator();
                while (iter.hasNext()) {
                    String fileName = iter.next().getStringValue();
                    Spd spd = new Spd(new File(workspaceDir + File.separatorChar + fileName));
                    packageList.add(spd.getPackageId());
                    spdTable.put(spd.getPackageId(), spd);
                }
            }

            //
            // Get platform list
            //
            if (db.getFrameworkDatabase().getPlatformList() != null) {
                List<DbPathAndFilename> platforms = db.getFrameworkDatabase().getPlatformList().getFilenameList();
                Iterator<DbPathAndFilename> iter = platforms.iterator();
                while (iter.hasNext()) {
                    String fileName = iter.next().getStringValue();
                    File fpdFile = new File(workspaceDir + File.separatorChar + fileName);
                    if ( !fpdFile.exists() ) {
                        throw new BuildException("Platform file [" + fpdFile.getPath() + "] not exists. ");
                    }
                    XmlObject fpdDoc = XmlObject.Factory.parse(fpdFile);
                    //
                    // Verify FPD file, if is invalid, throw Exception
                    //
                    if (!fpdDoc.validate()) {
                        throw new BuildException("Framework Platform Surface Area file [" + fpdFile.getPath() + "] format is invalid!");
                    }
                    //
                    // We can change Map to XmlObject
                    //
                    //
                    // TBD check SPD or FPD is existed in FS
                    //
                    Map<String, XmlObject> fpdDocMap = new HashMap<String, XmlObject>();
                    fpdDocMap.put("PlatformSurfaceArea", fpdDoc);
                    SurfaceAreaQuery.setDoc(fpdDocMap);
                    PlatformIdentification platformId = SurfaceAreaQuery.getFpdHeader();
                    platformId.setFpdFile(fpdFile);
                    platformList.add(platformId);
                }
            }
        } catch (Exception e) {
            throw new BuildException("Parse WORKSPACE Database file [" + dbFile.getPath() + "] Error.\n" + e.getMessage());
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
    public synchronized static File getMsaFile(ModuleIdentification moduleId) throws BuildException {
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
            throw new BuildException("Can't find Module [" + moduleId.getName() + "] in any SPD package!");
        } else {
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
            moduleId.setPackage(packageId);
            Spd spd = spdTable.get(packageId);
            if (spd.getModuleFile(moduleId) != null ) {
                break ;
            }
        }
        if (packageId == null){
            throw new BuildException("Can't find Module [" + moduleId.getName() + "] in any SPD package!");
        } else {
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


    public synchronized static void registerFpdModuleSA(FpdModuleIdentification fpdModuleId, Map<String, XmlObject> doc) {
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

    public synchronized static boolean hasFpdModuleSA(FpdModuleIdentification fpdModuleId) {
        return fpdModuleSA.containsKey(fpdModuleId);
    }

    /**
      Query module surface area information.

      <p>Note that surface area parsing is incremental. That means the method will
      only parse the MSA files if necessary. </p>
    
      @param fpdModuleId Module ID with arch
      @return ModuleSA info and MSA info for fpdModuleId
      @throws BuildException Can't find MSA
    **/
    public synchronized static Map<String, XmlObject> getDoc(FpdModuleIdentification fpdModuleId) throws BuildException {
        if (parsedModules.containsKey(fpdModuleId)) {
            return parsedModules.get(fpdModuleId);
        }
        Map<String, XmlObject> doc = new HashMap<String, XmlObject>();
        ModuleIdentification moduleId = fpdModuleId.getModule();
        //
        // First part: get the MSA files info
        //
        doc.putAll(getNativeMsa(moduleId));

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

    public synchronized static Map<String, XmlObject> getDoc(ModuleIdentification moduleId, String arch) throws BuildException {
        FpdModuleIdentification fpdModuleId = new FpdModuleIdentification(moduleId, arch);
        return getDoc(fpdModuleId);
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
    public synchronized static Map<String, XmlObject> getNativeMsa(ModuleIdentification moduleId) throws BuildException {
        if (nativeMsa.containsKey(moduleId)) {
            return nativeMsa.get(moduleId);
        }
        File msaFile = getMsaFile(moduleId);
        Map<String, XmlObject> msaMap = getNativeMsa(msaFile);
        nativeMsa.put(moduleId, msaMap);
        return msaMap;
    }

    public synchronized static Map<String, XmlObject> getNativeMsa(File msaFile) throws BuildException {
        if (!msaFile.exists()) {
            throw new BuildException("Module Surface Area file [" + msaFile.getPath() + "] can't be found!");
        }
        try {
            ModuleSurfaceAreaDocument doc = (ModuleSurfaceAreaDocument)XmlObject.Factory.parse(msaFile);
            //
            // Validate File if they accord with XML Schema
            //
            if ( !doc.validate()){
                throw new BuildException("Module Surface Area file [" + msaFile.getPath() + "] format is invalid!");
            }
            //
            // parse MSA file
            //
            ModuleSurfaceArea msa= doc.getModuleSurfaceArea();
            Map<String, XmlObject> msaMap = new HashMap<String, XmlObject>();
            msaMap.put("MsaHeader", cloneXmlObject(msa.getMsaHeader(), true));
            msaMap.put("ModuleDefinitions", cloneXmlObject(msa.getModuleDefinitions(), true));
            msaMap.put("LibraryClassDefinitions", cloneXmlObject(msa.getLibraryClassDefinitions(), true));
            msaMap.put("SourceFiles", cloneXmlObject(msa.getSourceFiles(), true));
            msaMap.put("PackageDependencies", cloneXmlObject(msa.getPackageDependencies(), true));
            msaMap.put("Protocols", cloneXmlObject(msa.getProtocols(), true));
            msaMap.put("PPIs", cloneXmlObject(msa.getPPIs(), true));
            msaMap.put("Guids", cloneXmlObject(msa.getGuids(), true));
            msaMap.put("Externs", cloneXmlObject(msa.getExterns(), true));
            msaMap.put("PcdCoded", cloneXmlObject(msa.getPcdCoded(), true));
            return msaMap;
        }
        catch (Exception ex){
            throw new BuildException(ex.getMessage());
        }
    }

    public static Map<String, XmlObject> getFpdBuildOptions() {
        Map<String, XmlObject> map = new HashMap<String, XmlObject>();
        map.put("BuildOptions", fpdBuildOptions);
        return map;
    }

    public static void setFpdBuildOptions(XmlObject fpdBuildOptions) {
        GlobalData.fpdBuildOptions = cloneXmlObject(fpdBuildOptions, true);
    }

    public static XmlObject getFpdDynamicPcds() {
        return fpdDynamicPcds;
    }

    public static void setFpdDynamicPcds(XmlObject fpdDynamicPcds) {
        GlobalData.fpdDynamicPcds = fpdDynamicPcds;
    }

    public static Set<ModuleIdentification> getModules(PackageIdentification packageId){
        Spd spd = spdTable.get(packageId);
        if (spd == null ) {
            Set<ModuleIdentification> dummy = new HashSet<ModuleIdentification>();
            return dummy;
        } else {
            return spd.getModules();
        }
    }

    /**
     * The header file path is relative to workspace dir
     */
    public static String[] getLibraryClassHeaderFiles(
            PackageIdentification[] packages, String name)
            throws BuildException {
        if (packages == null) {
            // throw Exception or not????
            return new String[0];
        }
        String[] result = null;
        for (int i = 0; i < packages.length; i++) {
            Spd spd = spdTable.get(packages[i]);
            //
            // If find one package defined the library class
            //
            if ((result = spd.getLibClassIncluder(name)) != null) {
                return result;
            }
        }
        //
        // If can't find library class declaration in every package
        //
        throw new BuildException("Can not find library class [" + name
                + "] declaration in any SPD package!");
    }

    /**
     * The header file path is relative to workspace dir
     */
    public static String getPackageHeaderFiles(PackageIdentification packages,
            String moduleType) throws BuildException {
        if (packages == null) {
            return new String("");
        }
        Spd spd = spdTable.get(packages);
        //
        // If can't find package header file, skip it
        //
        String temp = null;
        if (spd != null) {
            if ((temp = spd.getPackageIncluder(moduleType)) != null) {
                return temp;
            } else {
                temp = "";
                return temp;
            }
        } else {
            return null;
        }
    }

    /**
     * return two values: {cName, GuidValue}
     */
    public static String[] getGuid(List<PackageIdentification> packages, String name)
            throws BuildException {
        if (packages == null) {
            // throw Exception or not????
            return new String[0];
        }
        String[] result = null;
        Iterator item = packages.iterator();
        while (item.hasNext()){
            Spd spd = spdTable.get(item.next());
            //
            // If find one package defined the GUID
            //
            if ((result = spd.getGuid(name)) != null) {
                return result;
            }
        }

        return null;
    }

    /**
     * return two values: {cName, GuidValue}
     */
    public static String[] getPpiGuid(List<PackageIdentification> packages,
            String name) throws BuildException {
        if (packages == null) {
            return new String[0];
        }
        String[] result = null;
        Iterator item = packages.iterator();
        while (item.hasNext()){
            Spd spd = spdTable.get(item.next());
            //
            // If find one package defined the Ppi GUID
            //
            if ((result = spd.getPpi(name)) != null) {
                return result;
            }
        }
        return null;

    }

    /**
     * return two values: {cName, GuidValue}
     */
    public static String[] getProtocolGuid(List<PackageIdentification> packages,
            String name) throws BuildException {
        if (packages == null) {
            return new String[0];
        }
        String[] result = null;
        Iterator item = packages.iterator();
        while (item.hasNext()){
            Spd spd = spdTable.get(item.next());
            //
            // If find one package defined the protocol GUID
            //
            if ((result = spd.getProtocol(name))!= null){
                return result;
            }
        }
        return null;

    }

    public synchronized static PlatformIdentification getPlatformByName(String name) throws BuildException {
        Iterator iter = platformList.iterator();
        while(iter.hasNext()){
            PlatformIdentification platformId = (PlatformIdentification)iter.next();
            if (platformId.getName().equalsIgnoreCase(name)) {
                return platformId;
            }
        }
        throw new BuildException("Can't find platform [" + name + "] in the current WORKSPACE database!");
    }

    public synchronized static PlatformIdentification getPlatform(String filename) throws BuildException {
        File file = new File(workspaceDir + File.separatorChar + filename);
        Iterator iter = platformList.iterator();
        while(iter.hasNext()){
            PlatformIdentification platformId = (PlatformIdentification)iter.next();
            if (platformId.getFpdFile().getPath().equalsIgnoreCase(file.getPath())) {
                return platformId;
            }
        }
        throw new BuildException("Can't find platform file [" + filename + "] in the current WORKSPACE database!");
    }

    public synchronized static PackageIdentification refreshPackageIdentification(PackageIdentification packageId) throws BuildException {
        Iterator iter = packageList.iterator();
        while(iter.hasNext()){
            PackageIdentification packageItem = (PackageIdentification)iter.next();
            if (packageItem.equals(packageId)) {
                packageId.setName(packageItem.getName());
                packageId.setSpdFile(packageItem.getSpdFile());
                return packageId;
            }
        }
        throw new BuildException("Can't find package GUID value " + packageId.getGuid() + " in the current workspace!");
    }

    public synchronized static ModuleIdentification refreshModuleIdentification(ModuleIdentification moduleId) throws BuildException {
        PackageIdentification packageId = getPackageForModule(moduleId);
        moduleId.setPackage(packageId);
        Spd spd = spdTable.get(packageId);
        if (spd == null) {
            throw new BuildException("Can't find package GUID value " + packageId.getGuid() + " in the current workspace!");
        }
        Set<ModuleIdentification> modules = spd.getModules();
        Iterator<ModuleIdentification> iter = modules.iterator();
        while (iter.hasNext()) {
            ModuleIdentification item = iter.next();
            if (item.equals(moduleId)) {
                moduleId.setName(item.getName());
                moduleId.setModuleType(item.getModuleType());
                moduleId.setMsaFile(item.getMsaFile());
                return moduleId;
            }
        }
        throw new BuildException("Can't find module GUID value " + moduleId.getGuid() + " in package, " + packageId + ", in the current workspace!");
    }

    public synchronized static Set<PackageIdentification> getPackageList(){
        return packageList;
    }

    /**
      BUGBUG: It is a walk around method. If do not clone, can't query info with
      XPath correctly. 
      
      @param object XmlObject
      @param deep flag for deep clone
      @return XmlObject after clone
      @throws BuildException parse original XmlObject error. 
    **/
    private static XmlObject cloneXmlObject(XmlObject object, boolean deep) throws BuildException {
        if ( object == null) {
            return null;
        }
        XmlObject result = null;
        try {
            result = XmlObject.Factory.parse(object.getDomNode()
                            .cloneNode(deep));
        } catch (Exception ex) {
            throw new BuildException(ex.getMessage());
        }
        return result;
    }

    ////// Tool Chain Related, try to refine and put some logic process to ToolChainFactory

    public static ToolChainInfo getToolChainInfo() {
//        GlobalData.log.info(toolsDef.getConfigInfo() + "" + toolChainEnvInfo + toolChainPlatformInfo);
        if (toolChainInfo == null) {
            toolChainInfo = toolsDef.getConfigInfo().intersection(toolChainEnvInfo);
            if (toolChainPlatformInfo != null) {
                toolChainInfo = toolChainInfo.intersection(toolChainPlatformInfo);
            }
            toolChainInfo.addCommands(toolsDef.getConfigInfo().getCommands());
            toolChainInfo.normalize();
            GlobalData.log.info(toolChainInfo + "");
        }
        return toolChainInfo;
    }



    public static void setPlatformToolChainFamilyOption(ToolChainMap map) {
        platformToolChainFamilyOption = map;
    }

    public static void setPlatformToolChainOption(ToolChainMap map) {
        platformToolChainOption = map;
    }

    public static void addModuleToolChainOption(FpdModuleIdentification fpdModuleId,
        ToolChainMap toolChainOption) {
        moduleToolChainOption.put(fpdModuleId, toolChainOption);
    }

    public static void addModuleToolChainFamilyOption(FpdModuleIdentification fpdModuleId,
        ToolChainMap toolChainOption) {
        moduleToolChainFamilyOption.put(fpdModuleId, toolChainOption);
    }

    public static boolean isCommandSet(String target, String toolchain, String arch) {
        String[] commands = getToolChainInfo().getCommands();

        for (int i = 0; i < commands.length; ++i) {
            String cmdName = toolsDef.getConfig().get(new String[] {target, toolchain, arch, commands[i], ToolChainAttribute.NAME.toString()});
            if (cmdName != null && cmdName.length() != 0) {
                return true;
            }
        }

        return false;
    }

    public static String getCommandSetting(String[] commandDescription, FpdModuleIdentification fpdModuleId) throws EdkException {
        ToolChainKey toolChainKey = new ToolChainKey(commandDescription);
        ToolChainMap toolChainConfig = toolsDef.getConfig();
        String setting = null;

        if (!commandDescription[ToolChainElement.ATTRIBUTE.value].equals(ToolChainAttribute.FLAGS.toString())) {
            setting = toolChainConfig.get(toolChainKey);
            if (setting == null) {
                setting = "";
            }
            return setting;
        }

        //
        // get module specific options, if any
        //
        // tool tag first
        ToolChainMap option = moduleToolChainOption.get(fpdModuleId);
        ToolChainKey toolChainFamilyKey = null;

        if ((option == null) || (option != null && (setting = option.get(toolChainKey)) == null)) {
            //
            // then tool chain family
            //
            toolChainFamilyKey = new ToolChainKey(commandDescription);
            toolChainFamilyKey.setKey(ToolChainAttribute.FAMILY.toString(), ToolChainElement.ATTRIBUTE.value);
            String family = toolChainConfig.get(toolChainFamilyKey);
            toolChainFamilyKey.setKey(family, ToolChainElement.TOOLCHAIN.value);
            toolChainFamilyKey.setKey(ToolChainAttribute.FLAGS.toString(), ToolChainElement.ATTRIBUTE.value);

            option = moduleToolChainFamilyOption.get(fpdModuleId);
            if (option != null) {
                setting = option.get(toolChainFamilyKey);
            }
        }

        //
        // get platform options, if any
        //
        if (setting == null) {
            // tool tag first
            if (platformToolChainOption == null || (setting = platformToolChainOption.get(toolChainKey)) == null) {
                // then tool chain family
                if (toolChainFamilyKey == null) {
                    toolChainFamilyKey = new ToolChainKey(commandDescription);
                    toolChainFamilyKey.setKey(ToolChainAttribute.FAMILY.toString(), ToolChainElement.ATTRIBUTE.value);
                    String family = toolChainConfig.get(toolChainFamilyKey);
                    toolChainFamilyKey.setKey(family, ToolChainElement.TOOLCHAIN.value);
                    toolChainFamilyKey.setKey(ToolChainAttribute.FLAGS.toString(), ToolChainElement.ATTRIBUTE.value);
                }

                setting = platformToolChainFamilyOption.get(toolChainFamilyKey);
            }
        }

        if (setting == null) {
            setting = "";
        }

        return setting;
    }

    public static void setToolChainEnvInfo(ToolChainInfo envInfo) {
        toolChainEnvInfo = envInfo;
    }
    public static void setToolChainPlatformInfo(ToolChainInfo platformInfo) {
        toolChainPlatformInfo = platformInfo;
    }

    //
    // for PCD
    //
    public synchronized static MemoryDatabaseManager getPCDMemoryDBManager() {
        return pcdDbManager;
    }

    //
    // For PCD get tokenSpaceGUid
    //
    public synchronized static String getGuidInfoFromCname(String cName){
        String cNameGuid = null;
        String guid = null;
        Set set = spdTable.keySet();
        Iterator iter = set.iterator();

        if (iter == null) {
            return null;
        }

        while (iter.hasNext()){
            Spd spd = (Spd) spdTable.get(iter.next());
            guid = spd.getGuidFromCname(cName);
            if (guid != null){
                cNameGuid = guid;
                break;
            }
        }
        return cNameGuid;
    }

    //
    // For PCD
    //
    public synchronized static Map<FpdModuleIdentification, XmlObject>
                               getFpdModuleSaXmlObject(String xmlObjectName) {
        Set<FpdModuleIdentification> fpdModuleSASet = fpdModuleSA.keySet();
        Iterator item = fpdModuleSASet.iterator();


        Map<FpdModuleIdentification, XmlObject> SAPcdBuildDef = new HashMap<FpdModuleIdentification, XmlObject>();
        Map<String, XmlObject> SANode = new HashMap<String, XmlObject>();
        FpdModuleIdentification moduleId;
        while (item.hasNext()) {

            moduleId = (FpdModuleIdentification) item.next();
            SANode = fpdModuleSA.get(moduleId);
            try{
                if (SANode.get(xmlObjectName)!= null){
                    SAPcdBuildDef.put(moduleId,
                            (XmlObject) SANode.get(xmlObjectName));

                }
            } catch (Exception e){
                EdkLog.log(EdkLog.EDK_INFO, e.getMessage());
            }
        }
        return SAPcdBuildDef;
    }

    public synchronized static Map<FpdModuleIdentification,XmlObject> getFpdPcdBuildDefinitions() {
        Map<FpdModuleIdentification,XmlObject> pcdBuildDef = getFpdModuleSaXmlObject ("PcdBuildDefinition");

        return pcdBuildDef;
    }
}

