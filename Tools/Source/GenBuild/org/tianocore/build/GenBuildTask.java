/** @file
  This file is ANT task GenBuild. 
 
  The file is used to parse a specified Module, and generate its build time 
  ANT script build.xml, then call the the ANT script to build the module.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.build;

import java.io.File;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.Stack;
import java.util.Vector;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.taskdefs.Ant;
import org.apache.tools.ant.taskdefs.Property;
import org.apache.xmlbeans.XmlObject;

import org.tianocore.build.autogen.AutoGen;
import org.tianocore.build.fpd.FpdParserTask;
import org.tianocore.build.global.GenBuildLogger;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.OutputManager;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.id.FpdModuleIdentification;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.build.id.PackageIdentification;
import org.tianocore.build.id.PlatformIdentification;
import org.tianocore.build.tools.ModuleItem;
import org.tianocore.exception.EdkException;
import org.tianocore.logger.EdkLog;

/**
  <p>
  <code>GenBuildTask</code> is an ANT task that can be used in ANT build
  system. The main function of this task is to parse module's surface area,
  then generate the corresponding <em>BaseName_build.xml</em> (the real ANT
  build script) and call this to build the module. The whole process including:
  1. generate AutoGen.c and AutoGen.h; 2. build all dependent library instances; 
  3. build all source files inlcude AutoGen.c; 4. generate sections;
  5. generate FFS file if it is driver module while LIB file if it is Library module. 
  </p>
  
  <p>
  The usage is (take module <em>HelloWorld</em> for example):
  </p>
  
  <pre>
   &lt;GenBuild  
             msaFilename=&quot;HelloWorld.msa&quot;/&gt; 
             processTo=&quot;ALL&quot;/&gt;
  </pre>
  
  <p><code>processTo</code> provides a way to customize the whole build process. 
  processTo can be one value of ALL, AUTOGEN, FILES, LIBRARYINSTANCES, SECTIONS, NONE. 
  Default is ALL, means whole 
  </p>
  
  <p>
  This task calls <code>AutoGen</code> to generate <em>AutoGen.c</em> and
  <em>AutoGen.h</em>. The task also parses the development environment
  configuration files, such as collecting package information, setting compiler
  flags and so on.
  </p>
  
  
  @since GenBuild 1.0
**/
public class GenBuildTask extends Ant {
    
    ///
    /// Module surface area file.
    ///
    File msaFile;

    ///
    /// 
    ///
    private String type = "all"; // = "build";
    
    ///
    /// Module's Identification.
    ///
    private ModuleIdentification moduleId;

    private Vector<Property> properties = new Vector<Property>();

    private static Stack<Hashtable> backupPropertiesStack = new Stack<Hashtable>();
    
    private boolean isSingleModuleBuild = false;
    
    /**
      Public construct method. It is necessary for ANT task.
    **/
    public GenBuildTask() {
    }

    /**
  
      @throws BuildException
              From module build, exception from module surface area invalid.
    **/
    public void execute() throws BuildException {
    	//
        // set Logger
        //
        GenBuildLogger logger = new GenBuildLogger(getProject());
        EdkLog.setLogLevel(getProject().getProperty("env.LOGLEVEL"));
        EdkLog.setLogger(logger);
        // remove !!
        try {
        pushProperties();
        //
        // Enable all specified properties
        //
        Iterator<Property> iter = properties.iterator();
        while (iter.hasNext()) {
            Property item = iter.next();
            getProject().setProperty(item.getName(), item.getValue());
        }
        
        //
        // GenBuild should specify either msaFile or moduleGuid & packageGuid
        //
        if (msaFile == null ) {
            String moduleGuid = getProject().getProperty("MODULE_GUID");
            String moduleVersion = getProject().getProperty("MODULE_VERSION");
            String packageGuid = getProject().getProperty("PACKAGE_GUID");
            String packageVersion = getProject().getProperty("PACKAGE_VERSION");
            if (moduleGuid == null || packageGuid == null) {
                throw new BuildException("GenBuild parameters error. ");
            }
            PackageIdentification packageId = new PackageIdentification(packageGuid, packageVersion);
            moduleId = new ModuleIdentification(moduleGuid, moduleVersion);
            moduleId.setPackage(packageId);
            Map<String, XmlObject> doc = GlobalData.getNativeMsa(moduleId);
            SurfaceAreaQuery.setDoc(doc);
            moduleId = SurfaceAreaQuery.getMsaHeader();
        }
        else {
            Map<String, XmlObject> doc = GlobalData.getNativeMsa(msaFile);
            SurfaceAreaQuery.setDoc(doc);
            moduleId = SurfaceAreaQuery.getMsaHeader();
        }
        String[] producedLibraryClasses = SurfaceAreaQuery.getLibraryClasses("ALWAYS_PRODUCED",null);
        if (producedLibraryClasses.length == 0) {
            moduleId.setLibrary(false);
        }
        else {
            moduleId.setLibrary(true);
        }
        
        //
        // Judge whether it is single module build or not
        //
        if (isSingleModuleBuild) {
            //
            // Single Module build
            //
            prepareSingleModuleBuild();
        }
        else {
            //
            // Platform build. Restore the platform related info
            //
            String filename = getProject().getProperty("PLATFORM_FILE");
            PlatformIdentification platformId = GlobalData.getPlatform(filename);
            getProject().setProperty("PLATFORM_DIR", platformId.getFpdFile().getParent().replaceAll("(\\\\)", "/"));
            getProject().setProperty("PLATFORM_RELATIVE_DIR", platformId.getPlatformRelativeDir().replaceAll("(\\\\)", "/"));
            
            String packageGuid = getProject().getProperty("PACKAGE_GUID");
            String packageVersion = getProject().getProperty("PACKAGE_VERSION");
            PackageIdentification packageId = new PackageIdentification(packageGuid, packageVersion);
            moduleId.setPackage(packageId);
        }
        
        //
        // If single module : intersection MSA supported ARCHs and tools def!!
        // else, get arch from pass down
        //
        String[] archList = new String[0];
        if ( getProject().getProperty("ARCH") != null ) {
            archList = getProject().getProperty("ARCH").split(" ");
        }
        else {
            archList = GlobalData.getToolChainInfo().getArchs();
        }
        
        
        //
        // Judge if arch is all supported by current module. If not, throw Exception.
        //
        List moduleSupportedArchs = SurfaceAreaQuery.getModuleSupportedArchs();
        if (moduleSupportedArchs != null) {
            for (int k = 0; k < archList.length; k++) {
                if ( ! moduleSupportedArchs.contains(archList[k])) {
                    throw new BuildException("ARCH [" + archList[k] + "] is not supported by " + moduleId + ". " + moduleId + " only supports [" + moduleSupportedArchs + "].");
                }
            }
        }
        
        for (int k = 0; k < archList.length; k++) {
            getProject().setProperty("ARCH", archList[k]);
            
            FpdModuleIdentification fpdModuleId = new FpdModuleIdentification(moduleId, archList[k]);
            
            //
            // Whether the module is built before
            //
            if (GlobalData.isModuleBuilt(fpdModuleId)) {
                return ;
            }
            else {
                GlobalData.registerBuiltModule(fpdModuleId);
            }
            
            //
            // For Every TOOLCHAIN, TARGET
            //
            String[] targetList =  GlobalData.getToolChainInfo().getTargets();
            for (int i = 0; i < targetList.length; i ++){
                //
                // Prepare for target related common properties
                // TARGET
                //
                getProject().setProperty("TARGET", targetList[i]);
                String[] toolchainList = GlobalData.getToolChainInfo().getTagnames();
                for(int j = 0; j < toolchainList.length; j ++){
                    //
                    // check if any tool is defined for current target + toolchain + arch
                    // don't do anything if no tools found
                    // 
                    if (GlobalData.isCommandSet(targetList[i], toolchainList[j], archList[k]) == false) {
                        System.out.println("!!!Warning: No build issued. No tools found for [target=" + targetList[i] + " toolchain=" + toolchainList[j] + " arch=" + archList[k] + "]\n");
                        continue;
                    }

                    //
                    // Prepare for toolchain related common properties
                    // TOOLCHAIN
                    //
                    getProject().setProperty("TOOLCHAIN", toolchainList[j]);

                    System.out.println("Build " + moduleId + " start >>>");
                    System.out.println("Target: " + targetList[i] + " Tagname: " + toolchainList[j] + " Arch: " + archList[k]);
                    SurfaceAreaQuery.setDoc(GlobalData.getDoc(fpdModuleId));
                    
                    //
                    // Prepare for all other common properties
                    // PACKAGE, PACKAGE_GUID, PACKAGE_VERSION, PACKAGE_DIR, PACKAGE_RELATIVE_DIR
                    // MODULE or BASE_NAME, GUID or FILE_GUID, VERSION, MODULE_TYPE
                    // MODULE_DIR, MODULE_RELATIVE_DIR
                    // SUBSYSTEM, ENTRYPOINT, EBC_TOOL_LIB_PATH
                    // LIBS, OBJECTS, SDB_FILES
                    //
                    setModuleCommonProperties(archList[k]);
                    
                    //
                    // OutputManage prepare for 
                    // BIN_DIR, DEST_DIR_DEBUG, DEST_DIR_OUTPUT, BUILD_DIR, FV_DIR
                    //
                    OutputManager.getInstance().update(getProject());
                    
                    if (type.equalsIgnoreCase("all") || type.equalsIgnoreCase("build")) {
                        applyBuild(targetList[i], toolchainList[j], fpdModuleId);
                    }
                    else if (type.equalsIgnoreCase("clean")) {
                        applyClean(fpdModuleId);
                    }
                    else if (type.equalsIgnoreCase("cleanall")) {
                        applyCleanall(fpdModuleId);
                    }
                }
            }
        }
        popProperties();
        }catch (Exception e){
            e.printStackTrace();
            throw new BuildException(e.getMessage());
        }
    }

    /**
      This method is used to prepare Platform-related information. 
      
      <p>In Single Module Build mode, platform-related information is not ready.
      The method read the system environment variable <code>ACTIVE_PLATFORM</code> 
      and search in the Framework Database. Note that platform name in the Framework
      Database must be unique. </p>
     
    **/
    private void prepareSingleModuleBuild(){
        //
        // Find out the package which the module belongs to
        // TBD: Enhance it!!!!
        //
        PackageIdentification packageId = GlobalData.getPackageForModule(moduleId);
        
        moduleId.setPackage(packageId);
        
        //
        // Read ACTIVE_PLATFORM's FPD file (Call FpdParserTask's method)
        //
        String filename = getProject().getProperty("PLATFORM_FILE");
        
        if (filename == null){
            throw new BuildException("Plese set ACTIVE_PLATFORM if you want to build a single module. ");
        }
        
        PlatformIdentification platformId = GlobalData.getPlatform(filename);
        
        //
        // Read FPD file
        //
        FpdParserTask fpdParser = new FpdParserTask();
        fpdParser.setProject(getProject());
        fpdParser.parseFpdFile(platformId.getFpdFile());
        
        //
        // Prepare for Platform related common properties
        // PLATFORM, PLATFORM_DIR, PLATFORM_RELATIVE_DIR
        //
        getProject().setProperty("PLATFORM", platformId.getName());
        getProject().setProperty("PLATFORM_DIR", platformId.getFpdFile().getParent().replaceAll("(\\\\)", "/"));
        getProject().setProperty("PLATFORM_RELATIVE_DIR", platformId.getPlatformRelativeDir().replaceAll("(\\\\)", "/"));
    }


    /**
      Set Module-Related information to properties.
    **/
    private void setModuleCommonProperties(String arch) {
        //
        // Prepare for all other common properties
        // PACKAGE, PACKAGE_GUID, PACKAGE_VERSION, PACKAGE_DIR, PACKAGE_RELATIVE_DIR
        //
        PackageIdentification packageId = moduleId.getPackage();
        getProject().setProperty("PACKAGE", packageId.getName());
        getProject().setProperty("PACKAGE_GUID", packageId.getGuid());
        getProject().setProperty("PACKAGE_VERSION", packageId.getVersion());
        getProject().setProperty("PACKAGE_DIR", packageId.getPackageDir().replaceAll("(\\\\)", "/"));
        getProject().setProperty("PACKAGE_RELATIVE_DIR", packageId.getPackageRelativeDir().replaceAll("(\\\\)", "/"));
        
        //
        // MODULE or BASE_NAME, GUID or FILE_GUID, VERSION, MODULE_TYPE
        // MODULE_DIR, MODULE_RELATIVE_DIR
        //
        getProject().setProperty("MODULE", moduleId.getName());
        String baseName = SurfaceAreaQuery.getModuleOutputFileBasename();
        if (baseName == null) {
            getProject().setProperty("BASE_NAME", moduleId.getName());
        }
        else {
            getProject().setProperty("BASE_NAME", baseName);
        }
        getProject().setProperty("GUID", moduleId.getGuid());
        getProject().setProperty("FILE_GUID", moduleId.getGuid());
        getProject().setProperty("VERSION", moduleId.getVersion());
        getProject().setProperty("MODULE_TYPE", moduleId.getModuleType());
        getProject().setProperty("MODULE_DIR", moduleId.getMsaFile().getParent().replaceAll("(\\\\)", "/"));
        getProject().setProperty("MODULE_RELATIVE_DIR", moduleId.getModuleRelativePath().replaceAll("(\\\\)", "/"));
        
        //
        // SUBSYSTEM
        //
        String[][] subsystemMap = { { "BASE", "EFI_BOOT_SERVICE_DRIVER"},
                                    { "SEC", "EFI_BOOT_SERVICE_DRIVER" }, 
                                    { "PEI_CORE", "EFI_BOOT_SERVICE_DRIVER" }, 
                                    { "PEIM", "EFI_BOOT_SERVICE_DRIVER" }, 
                                    { "DXE_CORE", "EFI_BOOT_SERVICE_DRIVER" },
                                    { "DXE_DRIVER", "EFI_BOOT_SERVICE_DRIVER" }, 
                                    { "DXE_RUNTIME_DRIVER", "EFI_RUNTIME_DRIVER" }, 
                                    { "DXE_SAL_DRIVER", "EFI_BOOT_SERVICE_DRIVER" }, 
                                    { "DXE_SMM_DRIVER", "EFI_BOOT_SERVICE_DRIVER" }, 
                                    { "TOOL", "EFI_BOOT_SERVICE_DRIVER" }, 
                                    { "UEFI_DRIVER", "EFI_BOOT_SERVICE_DRIVER" },
                                    { "UEFI_APPLICATION", "EFI_APPLICATION" }, 
                                    { "USER_DEFINED", "EFI_BOOT_SERVICE_DRIVER"} }; 
        
        String subsystem = "EFI_BOOT_SERVICE_DRIVER";
        for (int i = 0; i < subsystemMap.length; i++) {
            if (moduleId.getModuleType().equalsIgnoreCase(subsystemMap[i][0])) {
                subsystem = subsystemMap[i][1];
                break ;
            }
        }
        getProject().setProperty("SUBSYSTEM", subsystem);
        
        //
        // ENTRYPOINT
        //
        if (arch.equalsIgnoreCase("EBC")) {
            getProject().setProperty("ENTRYPOINT", "EfiStart");
        }
        else {
            getProject().setProperty("ENTRYPOINT", "_ModuleEntryPoint");
        }
        
        //
        // LIBS, OBJECTS, SDB_FILES
        //
        getProject().setProperty("OBJECTS", "");
        getProject().setProperty("SDB_FILES", "");
        getProject().setProperty("LIBS", "");
    }

    private void getCompilerFlags(String target, String toolchain, FpdModuleIdentification fpdModuleId) throws EdkException {
        String[] cmd = GlobalData.getToolChainInfo().getCommands();
        for ( int m = 0; m < cmd.length; m++) {
            //
            // Set cmd, like CC, DLINK
            //
            String[] key = new String[]{target, toolchain, fpdModuleId.getArch(), cmd[m], null};
            key[4] = "PATH";
            String cmdPath = GlobalData.getCommandSetting(key, fpdModuleId);
            key[4] = "NAME";
            String cmdName = GlobalData.getCommandSetting(key, fpdModuleId);
            File cmdFile = new File(cmdPath + File.separatorChar + cmdName);
//            GlobalData.log.info("PATH: " + cmdFile.getPath());
            getProject().setProperty(cmd[m], cmdFile.getPath().replaceAll("(\\\\)", "/"));
            
            //
            // set CC_FLAGS
            //
            key[4] = "FLAGS";
            String cmdFlags = GlobalData.getCommandSetting(key, fpdModuleId);
//            GlobalData.log.info("Flags: " + cmdFlags);
            Set<String> addset = new LinkedHashSet<String>();
            Set<String> subset = new LinkedHashSet<String>();
            putFlagsToSet(addset, cmdFlags);
            getProject().setProperty(cmd[m] + "_FLAGS", getProject().replaceProperties(getFlags(addset, subset)));
            
            //
            // Set CC_EXT
            //
            key[4] = "EXT";
            String extName = GlobalData.getCommandSetting(key, fpdModuleId);
//            GlobalData.log.info("Ext: " + extName);
            if ( extName != null && ! extName.equalsIgnoreCase("")) {
                getProject().setProperty(cmd[m] + "_EXT", extName);
            }
            else {
                getProject().setProperty(cmd[m] + "_EXT", "");
            }
            
            //
            // set CC_FAMILY
            //
            key[4] = "FAMILY";
            String toolChainFamily = GlobalData.getCommandSetting(key, fpdModuleId);
//            GlobalData.log.info("FAMILY: " + toolChainFamily);
            if (toolChainFamily != null) {
                getProject().setProperty(cmd[m] + "_FAMILY", toolChainFamily);
            }
            
            //
            // set CC_SPATH
            //
            key[4] = "SPATH";
            String spath = GlobalData.getCommandSetting(key, fpdModuleId);
//            GlobalData.log.info("SPATH: " + spath);
            if (spath != null) {
                getProject().setProperty(cmd[m] + "_SPATH", spath.replaceAll("(\\\\)", "/"));
            }
            else {
                getProject().setProperty(cmd[m] + "_SPATH", "");
            }
            
            //
            // set CC_DPATH
            //
            key[4] = "DPATH";
            String dpath = GlobalData.getCommandSetting(key, fpdModuleId);
//            GlobalData.log.info("DPATH: " + dpath);
            if (dpath != null) {
                getProject().setProperty(cmd[m] + "_DPATH", dpath.replaceAll("(\\\\)", "/"));
            }
            else {
                getProject().setProperty(cmd[m] + "_DPATH", "");
            }
        }
    }
    
    public void setMsaFile(File msaFile) {
        this.msaFile = msaFile;
    }

    /**
      Method is for ANT to initialize MSA file. 
      
      @param msaFilename MSA file name
    **/
    public void setMsaFile(String msaFilename) {
        String moduleDir = getProject().getProperty("MODULE_DIR");
        
        //
        // If is Single Module Build, then use the Base Dir defined in build.xml
        //
        if (moduleDir == null) {
            moduleDir = getProject().getBaseDir().getPath();
        }
        msaFile = new File(moduleDir + File.separatorChar + msaFilename);
    }
    
    public void addConfiguredModuleItem(ModuleItem moduleItem) {
        PackageIdentification packageId = new PackageIdentification(moduleItem.getPackageGuid(), moduleItem.getPackageVersion());
        ModuleIdentification moduleId = new ModuleIdentification(moduleItem.getModuleGuid(), moduleItem.getModuleVersion());
        moduleId.setPackage(packageId);
        this.moduleId = moduleId;
    }
    
    /**
      Add a property. 
    
      @param p property
    **/
    public void addProperty(Property p) {
        properties.addElement(p);
    }

    public void setType(String type) {
        this.type = type;
    }
    
    private void applyBuild(String buildTarget, String buildTagname, FpdModuleIdentification fpdModuleId) throws EdkException{
        //
        // AutoGen
        //
        
        AutoGen autogen = new AutoGen(getProject().getProperty("DEST_DIR_DEBUG"), fpdModuleId.getModule(),fpdModuleId.getArch());
        autogen.genAutogen();
        
        
        //
        // Get compiler flags
        //
        getCompilerFlags(buildTarget, buildTagname, fpdModuleId);
        
        //
        // Prepare LIBS
        //
        ModuleIdentification[] libinstances = SurfaceAreaQuery.getLibraryInstance(fpdModuleId.getArch());
        String propertyLibs = "";
        for (int i = 0; i < libinstances.length; i++) {
            propertyLibs += " " + getProject().getProperty("BIN_DIR") + File.separatorChar + libinstances[i].getName() + ".lib";
        }
        getProject().setProperty("LIBS", propertyLibs.replaceAll("(\\\\)", "/"));
        
        //
        // if it is CUSTOM_BUILD
        // then call the exist BaseName_build.xml directly.
        //
        if (moduleId.getModuleType().equalsIgnoreCase("USER_DEFINED")) {
            GlobalData.log.info("Call user-defined " + moduleId.getName() + "_build.xml");
            Ant ant = new Ant();
            ant.setProject(getProject());
            ant.setAntfile(getProject().getProperty("MODULE_DIR") + File.separatorChar + moduleId.getName() + "_build.xml");
            ant.setInheritAll(true);
            ant.init();
            ant.execute();
            return ;
        }
        
        //
        // Generate ${BASE_NAME}_build.xml
        // TBD
        //
        String ffsKeyword = SurfaceAreaQuery.getModuleFfsKeyword();
        ModuleBuildFileGenerator fileGenerator = new ModuleBuildFileGenerator(getProject(), ffsKeyword, fpdModuleId);
        String buildFilename = getProject().getProperty("DEST_DIR_OUTPUT") + File.separatorChar + moduleId.getName() + "_build.xml";
        fileGenerator.genBuildFile(buildFilename);
        
        //
        // Ant call ${BASE_NAME}_build.xml
        //
        Ant ant = new Ant();
        ant.setProject(getProject());
        ant.setAntfile(getProject().getProperty("DEST_DIR_OUTPUT") + File.separatorChar + moduleId.getName() + "_build.xml");
        ant.setInheritAll(true);
        ant.init();
        ant.execute();
    }
    
    private void applyClean(FpdModuleIdentification fpdModuleId){
        //
        // if it is CUSTOM_BUILD
        // then call the exist BaseName_build.xml directly.
        //
        if (moduleId.getModuleType().equalsIgnoreCase("USER_DEFINED")) {
            GlobalData.log.info("Call user-defined " + moduleId.getName() + "_build.xml");
            Ant ant = new Ant();
            ant.setProject(getProject());
            ant.setAntfile(getProject().getProperty("MODULE_DIR") + File.separatorChar + moduleId.getName() + "_build.xml");
            ant.setTarget("clean");
            ant.setInheritAll(true);
            ant.init();
            ant.execute();
            return ;
        }
        
        Ant ant = new Ant();
        ant.setProject(getProject());
        ant.setAntfile(getProject().getProperty("DEST_DIR_OUTPUT") + File.separatorChar + moduleId.getName() + "_build.xml");
        ant.setTarget("clean");
        ant.setInheritAll(true);
        ant.init();
        ant.execute();
        
        //
        // Delete current module's DEST_DIR_OUTPUT
        // TBD
    }
    
    private void applyCleanall(FpdModuleIdentification fpdModuleId){
        //
        // if it is CUSTOM_BUILD
        // then call the exist BaseName_build.xml directly.
        //
        if (moduleId.getModuleType().equalsIgnoreCase("USER_DEFINED")) {
            GlobalData.log.info("Call user-defined " + moduleId.getName() + "_build.xml");
            Ant ant = new Ant();
            ant.setProject(getProject());
            ant.setAntfile(getProject().getProperty("MODULE_DIR") + File.separatorChar + moduleId.getName() + "_build.xml");
            ant.setTarget("cleanall");
            ant.setInheritAll(true);
            ant.init();
            ant.execute();
            return ;
        }
        
        Ant ant = new Ant();
        ant.setProject(getProject());
        ant.setAntfile(getProject().getProperty("DEST_DIR_OUTPUT") + File.separatorChar + moduleId.getName() + "_build.xml");
        ant.setTarget("cleanall");
        ant.setInheritAll(true);
        ant.init();
        ant.execute();
        
        //
        // Delete current module's DEST_DIR_OUTPUT
        // TBD
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
    private void putFlagsToSet(Set<String> set, String str) {
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
    private String getFlags(Set<String> add, Set<String> sub) {
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
    private String getRawFlags(Set<String> add, Set<String> sub) {
        String result = null;
        add.removeAll(sub);
        Iterator iter = add.iterator();
        while (iter.hasNext()) {
            String str = (String) iter.next();
            result += "\"" + str.substring(1, str.length() - 1) + "\", ";
        }
        return result;
    }

    private String parseOptionString(String optionString, Set<String> addSet, Set<String> subSet) {
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
    
    private void pushProperties() {
        backupPropertiesStack.push(getProject().getProperties());
    }
    
    private void popProperties() {
        Hashtable backupProperties = backupPropertiesStack.pop();
        Set keys = backupProperties.keySet();
        Iterator iter = keys.iterator();
        while (iter.hasNext()) {
            String item = (String)iter.next();
            getProject().setProperty(item, (String)backupProperties.get(item));
        }
    }

    public void setSingleModuleBuild(boolean isSingleModuleBuild) {
        this.isSingleModuleBuild = isSingleModuleBuild;
    }
}
