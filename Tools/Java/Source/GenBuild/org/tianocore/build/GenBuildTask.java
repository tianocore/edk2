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
import java.util.Vector;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.BuildListener;
import org.apache.tools.ant.Location;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.taskdefs.Ant;
import org.apache.tools.ant.taskdefs.Property;
import org.apache.xmlbeans.XmlObject;
import org.tianocore.build.autogen.AutoGen;
import org.tianocore.build.exception.AutoGenException;
import org.tianocore.build.exception.GenBuildException;
import org.tianocore.build.exception.PcdAutogenException;
import org.tianocore.build.exception.PlatformPcdPreprocessBuildException;
import org.tianocore.build.fpd.FpdParserTask;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.OutputManager;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.id.FpdModuleIdentification;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.build.id.PackageIdentification;
import org.tianocore.build.id.PlatformIdentification;
import org.tianocore.build.tools.ModuleItem;
import org.tianocore.common.definitions.ToolDefinitions;
import org.tianocore.common.exception.EdkException;
import org.tianocore.common.logger.EdkLog;

/**
  <p>
  <code>GenBuildTask</code> is an ANT task that can be used in ANT build
  system. 
  
  <p>The main function of this task is to parse module's surface area (MSA),
  then generate the corresponding <em>BaseName_build.xml</em> (the real ANT
  build script) and call this to build the module. The whole process including:
  
  <pre>
  1. generate AutoGen.c and AutoGen.h; 
  2. build all dependent library instances;
  3. build all source files inlcude AutoGen.c; 
  4. generate sections;
  5. generate FFS file if it is driver module while LIB file if it is Library module.
  </pre>


  <p>
  The usage is (take module <em>HelloWorld</em> for example):
  </p>

  <pre>
    &lt;GenBuild 
       msaFile="${PACKAGE_DIR}/Application/HelloWorld/HelloWorld.msa"
       type="cleanall" /&gt;
  </pre>

  <p>
  This task calls <code>AutoGen</code> to generate <em>AutoGen.c</em> and
  <em>AutoGen.h</em>. 
  </p>

  <p>
  This task will also set properties for current module, such as PACKAGE, 
  PACKAGE_GUID, PACKAGE_VERSION, PACKAGE_DIR, PACKAGE_RELATIVE_DIR 
  (relative to Workspace), MODULE or BASE_NAME, GUID, VERSION, MODULE_DIR, 
  MODULE_RELATIVE_DIR (relative to Package), CONFIG_DIR, BIN_DIR, 
  DEST_DIR_DEBUG, DEST_DIR_OUTPUT, TARGET, ARCH, TOOLCHAIN, TOOLCHAIN_FAMILY, 
  SUBSYSTEM, ENTRYPOINT, EBC_TOOL_LIB_PATH, all compiler command related 
  properties (CC, CC_FLAGS, CC_DPATH, CC_SPATH, CC_FAMILY, CC_EXT). 
  </p>
  
  @since GenBuild 1.0
**/
public class GenBuildTask extends Ant {

    ///
    /// Module surface area file.
    ///
    File msaFile;
    
    public ModuleIdentification parentId;
    
    private String type = "all"; 
    
    ///
    /// Module's Identification.
    ///
    private ModuleIdentification moduleId;

    private Vector<Property> properties = new Vector<Property>();

    private boolean isSingleModuleBuild = false;
    
    private SurfaceAreaQuery saq = null;

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
        this.setTaskName("GenBuild");
        try {
            processGenBuild();
        } catch (PcdAutogenException e) {
            BuildException buildException = new BuildException(e.getMessage());
            buildException.setStackTrace(e.getStackTrace());
            throw buildException;
        } catch (AutoGenException e) {
            BuildException buildException = new BuildException(e.getMessage());
            buildException.setStackTrace(e.getStackTrace());
            throw buildException;
        } catch (PlatformPcdPreprocessBuildException e) {
            BuildException buildException = new BuildException(e.getMessage());
            buildException.setStackTrace(e.getStackTrace());
            throw buildException;
        } catch (GenBuildException e) {
            BuildException buildException = new BuildException(e.getMessage());
            buildException.setStackTrace(e.getStackTrace());
            throw buildException;
        } catch (EdkException e) {
            BuildException buildException = new BuildException(e.getMessage());
            buildException.setStackTrace(e.getStackTrace());
            throw buildException;
        } catch (Exception e) {
            BuildException buildException = new BuildException(e.getMessage());
            buildException.setStackTrace(e.getStackTrace());
            throw buildException;
        }
    }

    private void processGenBuild() throws EdkException, BuildException, GenBuildException, AutoGenException, PcdAutogenException, PlatformPcdPreprocessBuildException {
    	if (!FrameworkBuildTask.multithread) {
            cleanupProperties();
        }

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
            //
            // If one of module Guid or package Guid is not specified, report error
            //
            if (moduleGuid == null || packageGuid == null) {
                throw new BuildException("GenBuild parameter error.");
            }
            
            PackageIdentification packageId = new PackageIdentification(packageGuid, packageVersion);
            GlobalData.refreshPackageIdentification(packageId);
            moduleId = new ModuleIdentification(moduleGuid, moduleVersion);
            moduleId.setPackage(packageId);
            GlobalData.refreshModuleIdentification(moduleId);
            Map<String, XmlObject> doc = GlobalData.getNativeMsa(moduleId);
            saq = new SurfaceAreaQuery(doc);
        } else {
            Map<String, XmlObject> doc = GlobalData.getNativeMsa(msaFile);
            saq = new SurfaceAreaQuery(doc);
            moduleId = saq.getMsaHeader();
            moduleId.setMsaFile(msaFile);
        }
        
        String[] producedLibraryClasses = saq.getLibraryClasses("ALWAYS_PRODUCED", null, null);
        if (producedLibraryClasses.length == 0) {
            moduleId.setLibrary(false);
        } else {
            moduleId.setLibrary(true);
        }
        moduleId.setBinary(saq.getBinaryModule());

        //
        // Judge whether it is single module build or not
        //
        if (isSingleModuleBuild) {
            //
            // Single Module build
            //
            prepareSingleModuleBuild();
        }

        //
        // If single module : get arch from pass down, otherwise intersection MSA 
        // supported ARCHs and tools def
        //
        Set<String> archListSupByToolChain = new LinkedHashSet<String>();
        String[] archs = GlobalData.getToolChainInfo().getArchs();

        for (int i = 0; i < archs.length; i ++) {
            archListSupByToolChain.add(archs[i]);
        }

        Set<String> archSet = new LinkedHashSet<String>();
        String archString = getProject().getProperty("ARCH");
        if (archString != null && archString.length() > 0) {
            String[] fpdArchList = archString.split(" ");

            for (int i = 0; i < fpdArchList.length; i++) {
                if (archListSupByToolChain.contains(fpdArchList[i])) {
                    archSet.add(fpdArchList[i]);
                }
            }
        } else {
            archSet = archListSupByToolChain; 
        }

        String[] archList = archSet.toArray(new String[archSet.size()]);

        //
        // Judge if arch is all supported by current module. If not, throw Exception.
        //
        List moduleSupportedArchs = saq.getModuleSupportedArchs();
        if (moduleSupportedArchs != null) {
            for (int k = 0; k < archList.length; k++) {
                if (!moduleSupportedArchs.contains(archList[k])) {
                    EdkLog.log(this, EdkLog.EDK_WARNING, "Specified architecture [" + archList[k] + "] is not supported by " + moduleId + ". The module " + moduleId + " only supports [" + moduleSupportedArchs + "] architectures.");
                    archList[k] = "";
                }
            }
        }

        if (archList.length == 0) {
            EdkLog.log(this, EdkLog.EDK_WARNING, "Warning: " + "[" + archString + "] is not supported for " + moduleId + " in this build!\n");
        }

        for (int k = 0; k < archList.length; k++) {
            if (archList[k] == "") {
                continue;
            }

            getProject().setProperty("ARCH", archList[k]);

            FpdModuleIdentification fpdModuleId = new FpdModuleIdentification(moduleId, archList[k]);

            //
            // Whether the module is built before
            //
            if ((moduleId.isLibrary() == false || isSingleModuleBuild) && GlobalData.hasFpdModuleSA(fpdModuleId) == false) {
                if (isSingleModuleBuild) {
                    EdkLog.log(this, EdkLog.EDK_ERROR, "Error: " + moduleId + " for " + archList[k] + " was not found in current platform FPD file!\n");
                    throw new BuildException("No platform containing this module!");
                } else {
                    EdkLog.log(this, EdkLog.EDK_WARNING, "Warning: " + moduleId + " for " + archList[k] + " was not found in current platform FPD file!\n");
                }
                continue;
            } else if (GlobalData.isModuleBuilt(fpdModuleId)) {
                break;
            } else {
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
                        EdkLog.log(this, EdkLog.EDK_WARNING, "Warning: No build issued.  No tools found for [target=" + targetList[i] + " toolchain=" + toolchainList[j] + " arch=" + archList[k] + "]\n");
                        continue;
                    }

                    //
                    // Prepare for toolchain related common properties
                    // TOOLCHAIN
                    //
                    getProject().setProperty("TOOLCHAIN", toolchainList[j]);

                    EdkLog.log(this, "Build " + moduleId + " start >>>");
                    EdkLog.log(this, "Target: " + targetList[i] + " Tagname: " + toolchainList[j] + " Arch: " + archList[k]);
                    saq.push(GlobalData.getDoc(fpdModuleId));

                    //
                    // Prepare for all other common properties
                    // PACKAGE, PACKAGE_GUID, PACKAGE_VERSION, PACKAGE_DIR, PACKAGE_RELATIVE_DIR
                    // MODULE or BASE_NAME, GUID or FILE_GUID, VERSION, MODULE_TYPE
                    // MODULE_DIR, MODULE_RELATIVE_DIR
                    // SUBSYSTEM, ENTRYPOINT, EBC_TOOL_LIB_PATH
                    //
                    setModuleCommonProperties(archList[k]);

                    //
                    // OutputManage prepare for
                    // BIN_DIR, DEST_DIR_DEBUG, DEST_DIR_OUTPUT, BUILD_DIR, FV_DIR
                    //
                    OutputManager.getInstance().update(getProject());

                    if (type.equalsIgnoreCase("all") || type.equalsIgnoreCase("build")) {
                        applyBuild(targetList[i], toolchainList[j], fpdModuleId);
                    } else {
                        applyNonBuildTarget(fpdModuleId);
                    }
                }
            }
        }
    }

    /**
      This method is used to prepare Platform-related information.

      <p>In Single Module Build mode, platform-related information is not ready.
      The method read the system environment variable <code>ACTIVE_PLATFORM</code>
      and search in the Framework Database. Note that platform name in the Framework
      Database must be unique. </p>

    **/
    private void prepareSingleModuleBuild() throws EdkException {
        //
        // Find out the package which the module belongs to
        //
        PackageIdentification packageId = GlobalData.getPackageForModule(moduleId);
        GlobalData.refreshPackageIdentification(packageId);
        moduleId.setPackage(packageId);
        GlobalData.refreshModuleIdentification(moduleId);

        //
        // Read ACTIVE_PLATFORM's FPD file 
        //
        String filename = getProject().getProperty("PLATFORM_FILE");

        if (filename == null){
            throw new BuildException("Please set ACTIVE_PLATFORM in the file: Tools/Conf/target.txt if you want to build a single module!");
        }

        PlatformIdentification platformId = GlobalData.getPlatform(filename);

        //
        // Read FPD file (Call FpdParserTask's method)
        //
        FpdParserTask fpdParser = new FpdParserTask();
        fpdParser.setProject(getProject());
        fpdParser.parseFpdFile(platformId.getFpdFile());
        getProject().setProperty("ARCH", fpdParser.getAllArchForModule(moduleId));
    }

    private void cleanupProperties() {
        Project newProject = new Project();

        Hashtable<String, String> passdownProperties = FrameworkBuildTask.originalProperties;
        Iterator<String> iter = passdownProperties.keySet().iterator();
        while (iter.hasNext()) {
            String item = iter.next();
            newProject.setProperty(item, passdownProperties.get(item));
        }

        newProject.setInputHandler(getProject().getInputHandler());

        Iterator listenerIter = getProject().getBuildListeners().iterator();
        while (listenerIter.hasNext()) {
            newProject.addBuildListener((BuildListener) listenerIter.next());
        }

        getProject().initSubProject(newProject);

        setProject(newProject);
    }

    /**
      Set Module-Related information to properties.
      
      @param arch current build ARCH
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
        String baseName = saq.getModuleOutputFileBasename();
        if (baseName == null) {
            getProject().setProperty("BASE_NAME", moduleId.getName());
        } else {
            getProject().setProperty("BASE_NAME", baseName);
        }
        getProject().setProperty("GUID", moduleId.getGuid());
        getProject().setProperty("FILE_GUID", moduleId.getGuid());
        getProject().setProperty("VERSION", moduleId.getVersion());
        getProject().setProperty("MODULE_TYPE", moduleId.getModuleType());
        File msaFile = moduleId.getMsaFile();
        String msaFileName = msaFile.getName();
        getProject().setProperty("MODULE_DIR", msaFile.getParent().replaceAll("(\\\\)", "/"));
        getProject().setProperty("MODULE_RELATIVE_DIR", moduleId.getModuleRelativePath().replaceAll("(\\\\)", "/") 
            + File.separatorChar + msaFileName.substring(0, msaFileName.lastIndexOf('.')));

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
        } else {
            getProject().setProperty("ENTRYPOINT", "_ModuleEntryPoint");
        }

        getProject().setProperty("OBJECTS", "");
    }

    private void getCompilerFlags(String target, String toolchain, FpdModuleIdentification fpdModuleId) throws EdkException {
        String[] cmd = GlobalData.getToolChainInfo().getCommands();
        for ( int m = 0; m < cmd.length; m++) {
            //
            // Set cmd, like CC, DLINK
            //
            String[] key = new String[]{target, toolchain, fpdModuleId.getArch(), cmd[m], null};
            key[4] = ToolDefinitions.TOOLS_DEF_ATTRIBUTE_PATH;
            String cmdPath = GlobalData.getCommandSetting(key, fpdModuleId);
            key[4] = ToolDefinitions.TOOLS_DEF_ATTRIBUTE_NAME;
            String cmdName = GlobalData.getCommandSetting(key, fpdModuleId);
            if (cmdName.length() == 0) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, "Warning: " + cmd[m] + " hasn't been defined!");
                getProject().setProperty(cmd[m], "");
                continue;
            }
            File cmdFile = new File(cmdPath + File.separatorChar + cmdName);
            getProject().setProperty(cmd[m], cmdFile.getPath().replaceAll("(\\\\)", "/"));

            //
            // set CC_FLAGS
            //
            key[4] = ToolDefinitions.TOOLS_DEF_ATTRIBUTE_FLAGS;
            String cmdFlags = GlobalData.getCommandSetting(key, fpdModuleId);
            if (cmdFlags != null) 
            {
              getProject().setProperty(cmd[m] + "_FLAGS", cmdFlags);
            } 
            else 
            {
              getProject().setProperty(cmd[m] + "_FLAGS", "");
            }

            //
            // Set CC_EXT
            //
            key[4] = ToolDefinitions.TOOLS_DEF_ATTRIBUTE_EXT;
            String extName = GlobalData.getCommandSetting(key, fpdModuleId);
            if ( extName != null && ! extName.equalsIgnoreCase("")) {
                getProject().setProperty(cmd[m] + "_EXT", extName);
            } else {
                getProject().setProperty(cmd[m] + "_EXT", "");
            }

            //
            // set CC_FAMILY
            //
            key[4] = ToolDefinitions.TOOLS_DEF_ATTRIBUTE_FAMILY;
            String toolChainFamily = GlobalData.getCommandSetting(key, fpdModuleId);
            if (toolChainFamily != null) {
                getProject().setProperty(cmd[m] + "_FAMILY", toolChainFamily);
            }

            //
            // set CC_SPATH
            //
            key[4] = ToolDefinitions.TOOLS_DEF_ATTRIBUTE_SPATH;
            String spath = GlobalData.getCommandSetting(key, fpdModuleId);
            if (spath != null) {
                getProject().setProperty(cmd[m] + "_SPATH", spath.replaceAll("(\\\\)", "/"));
            } else {
                getProject().setProperty(cmd[m] + "_SPATH", "");
            }

            //
            // set CC_DPATH
            //
            key[4] = ToolDefinitions.TOOLS_DEF_ATTRIBUTE_DPATH;
            String dpath = GlobalData.getCommandSetting(key, fpdModuleId);
            if (dpath != null) {
                getProject().setProperty(cmd[m] + "_DPATH", dpath.replaceAll("(\\\\)", "/"));
            } else {
                getProject().setProperty(cmd[m] + "_DPATH", "");
            }
            
            //
            // Set CC_LIBPATH
            //
            key[4] = ToolDefinitions.TOOLS_DEF_ATTRIBUTE_LIBPATH;
            String libpath = GlobalData.getCommandSetting(key, fpdModuleId);
            if (libpath != null) {
                getProject().setProperty(cmd[m] + "_LIBPATH", libpath.replaceAll("(\\\\)", "/"));
            } else {
                getProject().setProperty(cmd[m] + "_LIBPATH", "");
            }
            
            //
            // Set CC_INCLUDEPATH
            //
            key[4] = ToolDefinitions.TOOLS_DEF_ATTRIBUTE_INCLUDEPATH;
            String includepath = GlobalData.getCommandSetting(key, fpdModuleId);
            if (dpath != null) {
                getProject().setProperty(cmd[m] + "_INCLUDEPATH", includepath.replaceAll("(\\\\)", "/"));
            } else {
                getProject().setProperty(cmd[m] + "_INCLUDEPATH", "");
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

    private void applyBuild(String buildTarget, String buildTagname, FpdModuleIdentification fpdModuleId) throws EdkException {
        //
        // Call AutoGen to generate AutoGen.c and AutoGen.h for non-binary module
        //
        if (!fpdModuleId.getModule().isBinary()) {
            AutoGen autogen = new AutoGen(getProject().getProperty("FV_DIR"), getProject().getProperty("DEST_DIR_DEBUG"), fpdModuleId.getModule(),fpdModuleId.getArch(), saq, parentId);
            autogen.genAutogen();
        }

        //
        // Get compiler flags
        //
        try {
            getCompilerFlags(buildTarget, buildTagname, fpdModuleId);
        }
        catch (EdkException ee) {
            throw new BuildException(ee.getMessage());
        }
        
        //
        // Prepare LIBS
        //
        ModuleIdentification[] libinstances = saq.getLibraryInstance(fpdModuleId.getArch());
        String propertyLibs = "";
        for (int i = 0; i < libinstances.length; i++) {
            propertyLibs += getProject().getProperty("BIN_DIR") + File.separatorChar + libinstances[i].getName() + ".lib" + " ";
        }
        getProject().setProperty("LIBS", propertyLibs.replaceAll("(\\\\)", "/"));

        //
        // Get all includepath and set to INCLUDE_PATHS
        //
        String[] includes = prepareIncludePaths(fpdModuleId);
        
        //
        // if it is CUSTOM_BUILD
        // then call the exist BaseName_build.xml directly.
        //
        String buildFilename = "";
        if ((buildFilename = GetCustomizedBuildFile(fpdModuleId.getArch())) != "") {
            EdkLog.log(this, "Call user-defined " + buildFilename);
            
            String antFilename = getProject().getProperty("MODULE_DIR") + File.separatorChar + buildFilename;
            antCall(antFilename, null);
            
            return ;
        }

        //
        // Generate ${BASE_NAME}_build.xml
        // TBD
        //
        String ffsKeyword = saq.getModuleFfsKeyword();
        ModuleBuildFileGenerator fileGenerator = new ModuleBuildFileGenerator(getProject(), ffsKeyword, fpdModuleId, includes, saq);
        buildFilename = getProject().getProperty("DEST_DIR_OUTPUT") + File.separatorChar + moduleId.getName() + "_build.xml";
        fileGenerator.genBuildFile(buildFilename);

        //
        // Ant call ${BASE_NAME}_build.xml
        //
        String antFilename = getProject().getProperty("DEST_DIR_OUTPUT") + File.separatorChar + moduleId.getName() + "_build.xml";
        antCall(antFilename, null);
    }

    private void applyNonBuildTarget(FpdModuleIdentification fpdModuleId){
        //
        // if it is CUSTOM_BUILD
        // then call the exist BaseName_build.xml directly.
        //
        if (moduleId.getModuleType().equalsIgnoreCase("USER_DEFINED")) {
            EdkLog.log(this, "Calling user-defined " + moduleId.getName() + "_build.xml");
            
            String antFilename = getProject().getProperty("MODULE_DIR") + File.separatorChar + moduleId.getName() + "_build.xml";
            antCall(antFilename, this.type);
            
            return ;
        }

        String antFilename = getProject().getProperty("DEST_DIR_OUTPUT") + File.separatorChar + moduleId.getName() + "_build.xml";
        antCall(antFilename, this.type);
    }

    private void applyClean(FpdModuleIdentification fpdModuleId){
        //
        // if it is CUSTOM_BUILD
        // then call the exist BaseName_build.xml directly.
        //
        if (moduleId.getModuleType().equalsIgnoreCase("USER_DEFINED")) {
            EdkLog.log(this, "Calling user-defined " + moduleId.getName() + "_build.xml");
            
            String antFilename = getProject().getProperty("MODULE_DIR") + File.separatorChar + moduleId.getName() + "_build.xml";
            antCall(antFilename, "clean");
            
            return ;
        }

        String antFilename = getProject().getProperty("DEST_DIR_OUTPUT") + File.separatorChar + moduleId.getName() + "_build.xml";
        antCall(antFilename, "clean");
    }

    private void applyCleanall(FpdModuleIdentification fpdModuleId){
        //
        // if it is CUSTOM_BUILD
        // then call the exist BaseName_build.xml directly.
        //
        if (moduleId.getModuleType().equalsIgnoreCase("USER_DEFINED")) {
            EdkLog.log(this, "Calling user-defined " + moduleId.getName() + "_build.xml");

            String antFilename = getProject().getProperty("MODULE_DIR") + File.separatorChar + moduleId.getName() + "_build.xml";
            antCall(antFilename, "cleanall");
            
            return ;
        }
        
        String antFilename = getProject().getProperty("DEST_DIR_OUTPUT") + File.separatorChar + moduleId.getName() + "_build.xml";
        antCall(antFilename, "cleanall");
    }

    private void antCall(String antFilename, String target) {
        Ant ant = new Ant();
        ant.setProject(getProject());
        ant.setAntfile(antFilename);
        if (target != null) {
            ant.setTarget(target);
        }
        ant.setInheritAll(true);
        ant.init();
        ant.execute();
    }

    public void setSingleModuleBuild(boolean isSingleModuleBuild) {
        this.isSingleModuleBuild = isSingleModuleBuild;
    }
    
    private String[] prepareIncludePaths(FpdModuleIdentification fpdModuleId) throws EdkException{
        //
        // Prepare the includes: PackageDependencies and Output debug direactory
        //
        Set<String> includes = new LinkedHashSet<String>();
        String arch = fpdModuleId.getArch();
        
        //
        // WORKSPACE
        //
        includes.add("${WORKSPACE_DIR}" + File.separatorChar);
        
        //
        // Module iteself
        //
        includes.add("${MODULE_DIR}");
        includes.add("${MODULE_DIR}" + File.separatorChar + archDir(arch));
        
        //
        // Packages in PackageDenpendencies
        //
        PackageIdentification[] packageDependencies = saq.getDependencePkg(fpdModuleId.getArch());
        for (int i = 0; i < packageDependencies.length; i++) {
            GlobalData.refreshPackageIdentification(packageDependencies[i]);
            File packageFile = packageDependencies[i].getSpdFile();
            includes.add(packageFile.getParent() + File.separatorChar + "Include");
            includes.add(packageFile.getParent() + File.separatorChar + "Include" + File.separatorChar + archDir(arch));
        }

        //
        // All Dependency Library Instance's PackageDependencies
        //
        ModuleIdentification[] libinstances = saq.getLibraryInstance(fpdModuleId.getArch());
        for (int i = 0; i < libinstances.length; i++) {
            saq.push(GlobalData.getDoc(libinstances[i], fpdModuleId.getArch()));
            PackageIdentification[] libraryPackageDependencies = saq.getDependencePkg(fpdModuleId.getArch());
            for (int j = 0; j < libraryPackageDependencies.length; j++) {
                GlobalData.refreshPackageIdentification(libraryPackageDependencies[j]);
                File packageFile = libraryPackageDependencies[j].getSpdFile();
                includes.add(packageFile.getParent() + File.separatorChar + "Include");
                includes.add(packageFile.getParent() + File.separatorChar + "Include" + File.separatorChar + archDir(arch));
            }
            saq.pop();
        }
        
        
        //
        // The package which the module belongs to
        // TBD
        includes.add(fpdModuleId.getModule().getPackage().getPackageDir() + File.separatorChar + "Include");
        includes.add(fpdModuleId.getModule().getPackage().getPackageDir() + File.separatorChar + "Include" + File.separatorChar + archDir(arch));

        //
        // Debug files output directory
        //
        includes.add("${DEST_DIR_DEBUG}");
        
        //
        // set to INCLUDE_PATHS property
        //
        Iterator<String> iter = includes.iterator();
        StringBuffer includePaths = new StringBuffer();
        while (iter.hasNext()) {
            includePaths.append(iter.next());
            includePaths.append("; ");
        }
        getProject().setProperty("INCLUDE_PATHS", getProject().replaceProperties(includePaths.toString()).replaceAll("(\\\\)", "/"));
        
        return includes.toArray(new String[includes.size()]);
    }
    
    /**
     Return the name of the directory that corresponds to the architecture.
     This is a translation from the XML Schema tag to a directory that
     corresponds to our directory name coding convention.
    
     **/
   private String archDir(String arch) {
       return arch.replaceFirst("X64", "x64")
                  .replaceFirst("IPF", "Ipf")
                  .replaceFirst("IA32", "Ia32")
                  .replaceFirst("ARM", "Arm")
                  .replaceFirst("EBC", "Ebc");
   }
   
   
   public void setExternalProperties(Vector<Property> v) {
       this.properties = v;
   }

   private String GetCustomizedBuildFile(String arch) {
       String[][] files = saq.getSourceFiles(arch);
       for (int i = 0; i < files.length; ++i) {
           if (files[i][1].endsWith("build.xml")) {
               return files[i][1];
           }
       }

       return "";
   }
}
