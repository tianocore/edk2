/** @file
 This file is ANT task FpdParserTask.

 FpdParserTask is used to parse FPD (Framework Platform Description) and generate
 build.out.xml. It is for Package or Platform build use.

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 **/
package org.tianocore.build.fpd;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;
import java.util.Vector;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.taskdefs.Ant;
import org.apache.tools.ant.taskdefs.Property;
import org.apache.xmlbeans.XmlException;
import org.apache.xmlbeans.XmlObject;

import org.tianocore.common.definitions.EdkDefinitions;
import org.tianocore.common.definitions.ToolDefinitions;
import org.tianocore.common.exception.EdkException;
import org.tianocore.common.logger.EdkLog;
import org.tianocore.build.FrameworkBuildTask;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.OutputManager;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.id.FpdModuleIdentification;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.build.id.PackageIdentification;
import org.tianocore.build.id.PlatformIdentification;
import org.tianocore.build.pcd.action.PlatformPcdPreprocessActionForBuilding;
import org.tianocore.build.toolchain.ToolChainElement;
import org.tianocore.build.toolchain.ToolChainMap;
import org.tianocore.build.toolchain.ToolChainInfo;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

/**
  <code>FpdParserTask</code> is an ANT task. The main function is parsing Framework
  Platform Descritpion (FPD) XML file and generating its ANT build script for
  corresponding platform.

  <p>The task sets global properties PLATFORM, PLATFORM_DIR, PLATFORM_RELATIVE_DIR
  and BUILD_DIR. </p>

  <p>The task generates ${PLATFORM}_build.xml file which will be called by top level
  build.xml. The task also generate Fv.inf files (File is for Tool GenFvImage). </p>

  <p>FpdParserTask task stores all FPD information to GlobalData. And parse
  tools definition file to set up compiler options for different Target and
  different ToolChainTag. </p>

  <p>The method parseFpdFile is also prepared for single module build. </p>

  @since GenBuild 1.0
**/
public class FpdParserTask extends Task {

    ///
    /// Be used to ensure Global data will be initialized only once.
    ///
    private static boolean parsed = false;

    private File fpdFile = null;

    PlatformIdentification platformId;

    private String type;

    ///
    /// Mapping from modules identification to out put file name
    ///
    Map<FpdModuleIdentification, String> outfiles = new LinkedHashMap<FpdModuleIdentification, String>();

    ///
    /// Mapping from FV name to its modules
    ///
    Map<String, Set<FpdModuleIdentification>> fvs = new HashMap<String, Set<FpdModuleIdentification>>();

    ///
    /// Mapping from FV apriori file to its type (PEI or DXE)
    ///
    Map<String, String> aprioriType = new HashMap<String, String>();
    
    ///
    /// FpdParserTask can specify some ANT properties.
    ///
    private Vector<Property> properties = new Vector<Property>();

    SurfaceAreaQuery saq = null;

    boolean isUnified = true;
    
    public static String PEI_APRIORI_GUID = "1b45cc0a-156a-428a-af62-49864da0e6e6";
    
    public static String DXE_APRIORI_GUID = "fc510ee7-ffdc-11d4-bd41-0080c73c8881";
    
    /**
      Public construct method. It is necessary for ANT task.
    **/
    public FpdParserTask() {
    }

    /**
     ANT task's entry method. The main steps is described as following:

     <ul>
     <li>Initialize global information (Framework DB, SPD files and all MSA files
     listed in SPD). This step will execute only once in whole build process;</li>
     <li>Parse specified FPD file; </li>
     <li>Generate FV.inf files; </li>
     <li>Generate PlatformName_build.xml file for Flatform build; </li>
     <li>Collect PCD information. </li>
     </ul>

     @throws BuildException
     Surface area is not valid.
    **/
    public void execute() throws BuildException {
        this.setTaskName("FpdParser");
        
        //
        // Parse FPD file
        //
        parseFpdFile();

        //
        // Prepare BUILD_DIR
        //
        isUnified = OutputManager.getInstance().prepareBuildDir(getProject());

        String buildDir = getProject().getProperty("BUILD_DIR");
        //
        // For every Target and ToolChain
        //
        String[] targetList = GlobalData.getToolChainInfo().getTargets();
        for (int i = 0; i < targetList.length; i++) {
            String[] toolchainList = GlobalData.getToolChainInfo().getTagnames();
            for(int j = 0; j < toolchainList.length; j++) {
                //
                // Prepare FV_DIR
                //
                String ffsCommonDir = buildDir + File.separatorChar
                                + targetList[i] + "_"
                                + toolchainList[j];
                File fvDir = new File(ffsCommonDir + File.separatorChar + "FV");
                fvDir.mkdirs();
                getProject().setProperty("FV_DIR", fvDir.getPath().replaceAll("(\\\\)", "/"));

                //
                // Gen Fv.inf files
                //
                genFvInfFiles(ffsCommonDir);
            }
        }

        //
        // Gen build.xml
        //
        String platformBuildFile = buildDir + File.separatorChar + platformId.getName() + "_build.xml";
        PlatformBuildFileGenerator fileGenerator = new PlatformBuildFileGenerator(getProject(), outfiles, fvs, isUnified, saq, platformBuildFile, aprioriType);
        fileGenerator.genBuildFile();

        //
        // Ant call ${PLATFORM}_build.xml
        //
        Ant ant = new Ant();
        ant.setProject(getProject());
        ant.setAntfile(platformBuildFile);
        ant.setTarget(type);
        ant.setInheritAll(true);
        ant.init();
        ant.execute();
    }

    /**
      Generate Fv.inf files. The Fv.inf file is composed with four
      parts: Options, Attributes, Components and Files. The Fv.inf files
      will be under FV_DIR.

      @throws BuildException
                  File write FV.inf files error.
    **/
    void genFvInfFiles(String ffsCommonDir) throws BuildException {
        String[] validFv = saq.getFpdValidImageNames();
        for (int i = 0; i < validFv.length; i++) {
            //
            // Get all global variables from FPD and set them to properties
            //
            String[][] globalVariables = saq.getFpdGlobalVariable();
            for (int j = 0; j < globalVariables.length; j++) {
                getProject().setProperty(globalVariables[j][0], globalVariables[j][1]);
            }

            getProject().setProperty("FV_FILENAME", validFv[i]);

            File fvFile = new File(getProject().replaceProperties( getProject().getProperty("FV_DIR") + File.separatorChar + validFv[i] + ".inf"));
            if (fvFile.exists() && (fvFile.lastModified() >= fpdFile.lastModified())) {
                //
                // don't re-generate FV.inf if fpd has not been changed
                // 
                continue;
            }
            fvFile.getParentFile().mkdirs();

            try {
                FileWriter fw = new FileWriter(fvFile);
                BufferedWriter bw = new BufferedWriter(fw);

                //
                // Options
                //
                String[][] options = saq.getFpdOptions(validFv[i]);
                if (options.length > 0) {
                    bw.write("[options]");
                    bw.newLine();
                    for (int j = 0; j < options.length; j++) {
                        StringBuffer str = new StringBuffer(100);
                        str.append(options[j][0]);
                        while (str.length() < 40) {
                            str.append(' ');
                        }
                        str.append("=  ");
                        str.append(options[j][1]);
                        bw.write(getProject().replaceProperties(str.toString()));
                        bw.newLine();
                    }
                    bw.newLine();
                }

                //
                // Attributes;
                //
                String[][] attributes = saq.getFpdAttributes(validFv[i]);
                if (attributes.length > 0) {
                    bw.write("[attributes]");
                    bw.newLine();
                    for (int j = 0; j < attributes.length; j++) {
                        StringBuffer str = new StringBuffer(100);
                        str.append(attributes[j][0]);
                        while (str.length() < 40) {
                            str.append(' ');
                        }
                        str.append("=  ");
                        str.append(attributes[j][1]);
                        bw.write(getProject().replaceProperties(str.toString()));
                        bw.newLine();
                    }
                    bw.newLine();
                }

                //
                // Components
                //
                String[][] components = saq.getFpdComponents(validFv[i]);
                if (components.length > 0) {
                    bw.write("[components]");
                    bw.newLine();
                    for (int j = 0; j < components.length; j++) {
                        StringBuffer str = new StringBuffer(100);
                        str.append(components[j][0]);
                        while (str.length() < 40) {
                            str.append(' ');
                        }
                        str.append("=  ");
                        str.append(components[j][1]);
                        bw.write(getProject().replaceProperties(str.toString()));
                        bw.newLine();
                    }
                    bw.newLine();
                }
                
                //
                // Files
                //
                Set<FpdModuleIdentification> moduleSeqSet = getModuleSequenceForFv(validFv[i]);
                
                Set<FpdModuleIdentification> filesSet = fvs.get(validFv[i]);
                
                FpdModuleIdentification[] files = null;
                
                if (moduleSeqSet == null) {
                    if (filesSet != null) {
                        files = filesSet.toArray(new FpdModuleIdentification[filesSet.size()]);
                    }
                } else if (filesSet == null) {
                    if (moduleSeqSet.size() != 0) {
                        throw new BuildException("Can not find any modules belongs to FV[" + validFv[i] + "], but listed some in BuildOptions.UserExtensions[@UserID='IMAGES' @Identifier='1']");
                    }
                } else {
                    //
                    // if moduleSeqSet and filesSet is inconsistent, report error
                    //
                    if(moduleSeqSet.size() != filesSet.size()){
                        throw new BuildException("Modules for FV[" + validFv[i] + "] defined in FrameworkModules and in BuildOptions.UserExtensions[@UserID='IMAGES' @Identifier='1'] are inconsistent. ");
                    } else {
                        //
                        // whether all modules in moduleSeqSet listed in filesSet
                        //
                        Iterator<FpdModuleIdentification> iter = moduleSeqSet.iterator();
                        while (iter.hasNext()) {
                            FpdModuleIdentification item = iter.next();
                            if (!filesSet.contains(item)) {
                                throw new BuildException("Can not find " + item + " belongs to FV[" + validFv[i] + "]");
                            }
                        }
                    }
                    
                    files = moduleSeqSet.toArray(new FpdModuleIdentification[moduleSeqSet.size()]);
                }
                
                
                if (files != null) {
                    bw.write("[files]");
                    bw.newLine();
                    
                    Set<FpdModuleIdentification> modules = null;
                    
                    if ( (modules = getPeiApriori(validFv[i])) != null) {
                        //
                        // Special GUID - validFv[i].FFS
                        //
                        String str = ffsCommonDir + File.separatorChar + "FV" + File.separatorChar + PEI_APRIORI_GUID + "-" + validFv[i] + ".FFS";
                        bw.write(getProject().replaceProperties("EFI_FILE_NAME = " + str));
                        bw.newLine();
                        
                        File aprioriFile = new File(getProject().getProperty("FV_DIR") + File.separatorChar + validFv[i] + ".apr");
                        aprioriType.put(validFv[i], PEI_APRIORI_GUID);
                        genAprioriFile(modules, aprioriFile);
                    } else if((modules = getDxeApriori(validFv[i])) != null) {
                        //
                        // Special GUID - validFv[i].FFS
                        //
                        String str = ffsCommonDir + File.separatorChar + "FV" + File.separatorChar + DXE_APRIORI_GUID + "-" + validFv[i] + ".FFS";
                        bw.write(getProject().replaceProperties("EFI_FILE_NAME = " + str));
                        bw.newLine();
                        
                        File aprioriFile = new File(getProject().getProperty("FV_DIR") + File.separatorChar + validFv[i] + ".apr");
                        aprioriType.put(validFv[i], DXE_APRIORI_GUID);
                        genAprioriFile(modules, aprioriFile);
                    }
                    
                    for (int j = 0; j < files.length; j++) {
                        String str = ffsCommonDir + File.separatorChar + outfiles.get(files[j]);
                        bw.write(getProject().replaceProperties("EFI_FILE_NAME = " + str));
                        bw.newLine();
                    }
                }
                bw.flush();
                bw.close();
                fw.close();
            } catch (IOException ex) {
                BuildException buildException = new BuildException("Generation of the FV file [" + fvFile.getPath() + "] failed!\n" + ex.getMessage());
                buildException.setStackTrace(ex.getStackTrace());
                throw buildException;
            } catch (EdkException ex) {
                BuildException buildException = new BuildException("Generation of the FV file [" + fvFile.getPath() + "] failed!\n" + ex.getMessage());
                buildException.setStackTrace(ex.getStackTrace());
                throw buildException;
            }
        }
    }
    
    /**
      This method is used for Single Module Build.

      @throws BuildException
                  FPD file is not valid.
    **/
    public void parseFpdFile(File fpdFile) throws BuildException, EdkException {
        this.fpdFile = fpdFile;
        parseFpdFile();
        
        //
        // Call Platform_build.xml prebuild firstly in stand-alone build
        // Prepare BUILD_DIR
        //
        isUnified = OutputManager.getInstance().prepareBuildDir(getProject());

        String buildDir = getProject().getProperty("BUILD_DIR");
        //
        // For every Target and ToolChain
        //
        String[] targetList = GlobalData.getToolChainInfo().getTargets();
        for (int i = 0; i < targetList.length; i++) {
            String[] toolchainList = GlobalData.getToolChainInfo().getTagnames();
            for(int j = 0; j < toolchainList.length; j++) {
                //
                // Prepare FV_DIR
                //
                String ffsCommonDir = buildDir + File.separatorChar
                                + targetList[i] + "_"
                                + toolchainList[j];
                File fvDir = new File(ffsCommonDir + File.separatorChar + "FV");
                fvDir.mkdirs();
            }
        }

        String platformBuildFile = buildDir + File.separatorChar + platformId.getName() + "_build.xml";
        PlatformBuildFileGenerator fileGenerator = new PlatformBuildFileGenerator(getProject(), outfiles, fvs, isUnified, saq, platformBuildFile, aprioriType);
        fileGenerator.genBuildFile();
        
        Ant ant = new Ant();
        ant.setProject(getProject());
        ant.setAntfile(platformBuildFile);
        ant.setTarget("prebuild");
        ant.setInheritAll(true);
        ant.init();
        ant.execute();
    }

    /**
      Parse FPD file.

      @throws BuildException
                  FPD file is not valid.
     **/
    void parseFpdFile() throws BuildException {
        try {
            XmlObject doc = XmlObject.Factory.parse(fpdFile);

            if (!doc.validate()) {
                throw new BuildException("Platform Surface Area file [" + fpdFile.getPath() + "] format is invalid!");
            }

            Map<String, XmlObject> map = new HashMap<String, XmlObject>();
            map.put("PlatformSurfaceArea", doc);
            saq = new SurfaceAreaQuery(map);

            //
            // Initialize
            //
            platformId = saq.getFpdHeader();
            platformId.setFpdFile(fpdFile);
            getProject().setProperty("PLATFORM", platformId.getName());
            getProject().setProperty("PLATFORM_FILE", platformId.getRelativeFpdFile().replaceAll("(\\\\)", "/"));
            getProject().setProperty("PLATFORM_DIR", platformId.getFpdFile().getParent().replaceAll("(\\\\)", "/"));
            getProject().setProperty("PLATFORM_RELATIVE_DIR", platformId.getPlatformRelativeDir().replaceAll("(\\\\)", "/"));
            
            if( !FrameworkBuildTask.multithread) {
                FrameworkBuildTask.originalProperties.put("PLATFORM", platformId.getName());
                FrameworkBuildTask.originalProperties.put("PLATFORM_FILE", platformId.getRelativeFpdFile().replaceAll("(\\\\)", "/"));
                FrameworkBuildTask.originalProperties.put("PLATFORM_DIR", platformId.getFpdFile().getParent().replaceAll("(\\\\)", "/"));
                FrameworkBuildTask.originalProperties.put("PLATFORM_RELATIVE_DIR", platformId.getPlatformRelativeDir().replaceAll("(\\\\)", "/"));
            }

            //
            // Build mode. User-defined output dir.
            //
            String buildMode = saq.getFpdIntermediateDirectories();
            String userDefinedOutputDir = saq.getFpdOutputDirectory();

            OutputManager.getInstance().setup(userDefinedOutputDir, buildMode);

            //
            // TBD. Deal PCD and BuildOption related Info
            //
            GlobalData.setFpdBuildOptions(saq.getFpdBuildOptions());

            GlobalData.setToolChainPlatformInfo(saq.getFpdToolChainInfo());

            //
            // Parse all list modules SA
            //
            parseModuleSAFiles();

            //
            // TBD. Deal PCD and BuildOption related Info
            //
            parseToolChainFamilyOptions();
            parseToolChainOptions();

            //
            // check if the tool chain is valid or not
            // 
            checkToolChain();

            saq.push(map);

            //
            // Pcd Collection. Call CollectPCDAction to collect pcd info.
            //
            if (!parsed) {
                PlatformPcdPreprocessActionForBuilding ca = new PlatformPcdPreprocessActionForBuilding();
                ca.perform(platformId.getFpdFile().getPath());
            }
        } catch (IOException ex) {
            BuildException buildException = new BuildException("Parsing of the FPD file [" + fpdFile.getPath() + "] failed!\n" + ex.getMessage());
            buildException.setStackTrace(ex.getStackTrace());
            throw buildException;
        } catch (XmlException ex) {
            BuildException buildException = new BuildException("Parsing of the FPD file [" + fpdFile.getPath() + "] failed!\n" + ex.getMessage());
            buildException.setStackTrace(ex.getStackTrace());
            throw buildException;
        } catch (EdkException ex) {
            BuildException buildException = new BuildException("Parsing of the FPD file [" + fpdFile.getPath() + "] failed!\n" + ex.getMessage());
            buildException.setStackTrace(ex.getStackTrace());
            throw buildException;
        }
        if (!parsed) {
            parsed = true;
        }
    }

    /**
      Parse all modules listed in FPD file.
    **/
    void parseModuleSAFiles() throws EdkException{
        Map<FpdModuleIdentification, Map<String, XmlObject>> moduleSAs = saq.getFpdModules();

        //
        // For every Module lists in FPD file.
        //
        Set<FpdModuleIdentification> keys = moduleSAs.keySet();
        Iterator iter = keys.iterator();
        while (iter.hasNext()) {
            FpdModuleIdentification fpdModuleId = (FpdModuleIdentification) iter.next();

            //
            // Judge if Module is existed?
            // TBD
            GlobalData.registerFpdModuleSA(fpdModuleId, moduleSAs.get(fpdModuleId));

            //
            // Put fpdModuleId to the corresponding FV
            //
            saq.push(GlobalData.getDoc(fpdModuleId));
            String fvBinding = saq.getModuleFvBindingKeyword();

            fpdModuleId.setFvBinding(fvBinding);
            updateFvs(fvBinding, fpdModuleId);

            //
            // Prepare for out put file name
            //
            ModuleIdentification moduleId = fpdModuleId.getModule();

            String baseName = saq.getModuleOutputFileBasename();
            
            if (baseName == null) {
                baseName = moduleId.getName();
            }
            outfiles.put(fpdModuleId, fpdModuleId.getArch() + File.separatorChar
                         + moduleId.getGuid() + "-" + baseName
                         + getSuffix(moduleId.getModuleType()));

            //
            // parse module build options, if any
            //
            GlobalData.addModuleToolChainOption(fpdModuleId, parseModuleBuildOptions(false));
            GlobalData.addModuleToolChainFamilyOption(fpdModuleId, parseModuleBuildOptions(true));
            
            //
            // parse MSA build options
            //
            GlobalData.addMsaBuildOption(moduleId, parseMsaBuildOptions(false));
            GlobalData.addMsaFamilyBuildOption(moduleId, parseMsaBuildOptions(true));
    
            ModuleIdentification[] libraryInstances = saq.getLibraryInstance(null);
            for (int i = 0; i < libraryInstances.length; i++) {
                saq.push(GlobalData.getDoc(libraryInstances[i], fpdModuleId.getArch()));
                GlobalData.addMsaBuildOption(libraryInstances[i], parseMsaBuildOptions(false));
                GlobalData.addMsaFamilyBuildOption(libraryInstances[i], parseMsaBuildOptions(true));
                saq.pop();
            }
            
            saq.pop();
        }
    }

    ToolChainMap parseModuleBuildOptions(boolean toolChainFamilyFlag) throws EdkException {
        String[][] options = saq.getModuleBuildOptions(toolChainFamilyFlag);
        if (options == null || options.length == 0) {
            return new ToolChainMap();
        }
        return parseOptions(options);
    }

    private ToolChainMap parsePlatformBuildOptions(boolean toolChainFamilyFlag) throws EdkException {
        String[][] options = saq.getPlatformBuildOptions(toolChainFamilyFlag);
        if (options == null || options.length == 0) {
            return new ToolChainMap();
        }
        return parseOptions(options);
    }

    ToolChainMap parseMsaBuildOptions(boolean toolChainFamilyFlag) throws EdkException {
        String[][] options = saq.getMsaBuildOptions(toolChainFamilyFlag);
        if (options == null || options.length == 0) {
            return new ToolChainMap();
        }
        return parseOptions(options);
    }
    
    private ToolChainMap parseOptions(String[][] options) throws EdkException {
        ToolChainMap map = new ToolChainMap();
        int flagIndex = ToolChainElement.ATTRIBUTE.value;

        for (int i = 0; i < options.length; ++i) {
            String flagString = options[i][flagIndex];
            if (flagString == null) {
                flagString = "";
            }
            options[i][flagIndex] = ToolDefinitions.TOOLS_DEF_ATTRIBUTE_FLAGS;
            map.put(options[i], flagString.trim());
        }

        return map;
    }

    private void parseToolChainFamilyOptions() throws EdkException {
        GlobalData.setPlatformToolChainFamilyOption(parsePlatformBuildOptions(true));
    }

    private void parseToolChainOptions() throws EdkException {
        GlobalData.setPlatformToolChainOption(parsePlatformBuildOptions(false));
    }

    /**
      Add the current module to corresponding FV.

      @param fvName current FV name
      @param moduleName current module identification
    **/
    void updateFvs(String fvName, FpdModuleIdentification fpdModuleId) {
        if (fvName == null || fvName.trim().length() == 0) {
            fvName = "NULL";
        }
        String[] fvNameArray = fvName.split("[, \t]+");
        for (int i = 0; i < fvNameArray.length; i++) {
            //
            // Put module to corresponding fvName
            //
            if (fvs.containsKey(fvNameArray[i])) {
                Set<FpdModuleIdentification> set = fvs.get(fvNameArray[i]);
                set.add(fpdModuleId);
            } else {
                Set<FpdModuleIdentification> set = new LinkedHashSet<FpdModuleIdentification>();
                set.add(fpdModuleId);
                fvs.put(fvNameArray[i], set);
            }
        }
    }

    /**
      Get the suffix based on module type. Current relationship are listed:

      <pre>
      <b>ModuleType</b>     <b>Suffix</b>
      BASE                 .FFS
      SEC                  .SEC
      PEI_CORE             .PEI
      PEIM                 .PEI
      DXE_CORE             .DXE
      DXE_DRIVER           .DXE
      DXE_RUNTIME_DRIVER   .DXE
      DXE_SAL_DRIVER       .DXE
      DXE_SMM_DRIVER       .DXE
      TOOL                 .FFS
      UEFI_DRIVER          .DXE
      UEFI_APPLICATION     .APP
      USER_DEFINED         .FFS
      </pre>

      @param moduleType module type
      @return
      @throws BuildException
      If module type is null
    **/
    public static String getSuffix(String moduleType) throws BuildException {
        if (moduleType == null) {
            throw new BuildException("Module type is not specified.");
        }

        String[][] suffix = EdkDefinitions.ModuleTypeExtensions;

        for (int i = 0; i < suffix.length; i++) {
            if (suffix[i][0].equalsIgnoreCase(moduleType)) {
                return suffix[i][1];
            }
        }
        //
        // Default is '.FFS'
        //
        return ".FFS";
    }
    /**
     Add a property.

     @param p property
     **/
    public void addProperty(Property p) {
        properties.addElement(p);
    }

    public void setFpdFile(File fpdFile) {
        this.fpdFile = fpdFile;
    }

    public void setType(String type) {
        this.type = type;
    }
    
    public String getAllArchForModule(ModuleIdentification moduleId) {
        String archs = "";
        Iterator<FpdModuleIdentification> iter = outfiles.keySet().iterator();
        while (iter.hasNext()) {
            FpdModuleIdentification fpdModuleId = iter.next();
            
            if (fpdModuleId.getModule().equals(moduleId)) {
                archs += fpdModuleId.getArch() + " ";
            }
        }
        
        return archs;
    }
    
    private void genAprioriFile(Set<FpdModuleIdentification> modules, File file) {
        try {
            FileWriter fw = new FileWriter(file);
            BufferedWriter bw = new BufferedWriter(fw);
            
            Iterator<FpdModuleIdentification> iter = modules.iterator();
            while(iter.hasNext()) {
                bw.write(iter.next().getModule().getGuid());
                bw.newLine();
            }
            
            bw.flush();
            bw.close();
            fw.close();
        }  catch (IOException ex) {
            BuildException buildException = new BuildException("Generation of the Apriori file [" + file.getPath() + "] failed!\n" + ex.getMessage());
            buildException.setStackTrace(ex.getStackTrace());
            throw buildException;
        }
    }
    
    private Set<FpdModuleIdentification> getPeiApriori(String fvName) throws EdkException {
        Node node = saq.getPeiApriori(fvName);
        Set<FpdModuleIdentification> result = new LinkedHashSet<FpdModuleIdentification>();
        if (node == null) {
            return null;
        }
        
        NodeList childNodes = node.getChildNodes();
        for (int i = 0; i < childNodes.getLength(); i++) {
            Node childItem = childNodes.item(i);
            if (childItem.getNodeType() == Node.ELEMENT_NODE) {
                //
                // Find child elements "IncludeModules"
                //
                if (childItem.getNodeName().compareTo("IncludeModules") == 0) {
                    //
                    // result will be updated
                    //
                    processNodes(childItem, result);
                } else if (childItem.getNodeName().compareTo("FvName") == 0) {
                    
                } else if (childItem.getNodeName().compareTo("InfFileName") == 0) {
                    
                } else {
                    //
                    // Report Warning
                    //
                    EdkLog.log(this, EdkLog.EDK_WARNING, "Unrecognised element " + childItem.getNodeName() + " under FPD.BuildOptions.UserExtensions[UserID='APRIORI' Identifier='0']");
                }
            }
        }
        
        return result;
    }

    private Set<FpdModuleIdentification> getDxeApriori(String fvName) throws EdkException {
        Node node = saq.getDxeApriori(fvName);
        Set<FpdModuleIdentification> result = new LinkedHashSet<FpdModuleIdentification>();
        if (node == null) {
            return null;
        }
        
        NodeList childNodes = node.getChildNodes();
        for (int i = 0; i < childNodes.getLength(); i++) {
            Node childItem = childNodes.item(i);
            if (childItem.getNodeType() == Node.ELEMENT_NODE) {
                //
                // Find child elements "IncludeModules"
                //
                if (childItem.getNodeName().compareTo("IncludeModules") == 0) {
                    //
                    // result will be updated
                    //
                    processNodes(childItem, result);
                } else if (childItem.getNodeName().compareTo("FvName") == 0) {
                    
                } else if (childItem.getNodeName().compareTo("InfFileName") == 0) {
                    
                } else {
                    //
                    // Report Warning
                    //
                    EdkLog.log(this, EdkLog.EDK_WARNING, "Unrecognised element " + childItem.getNodeName() + " under FPD.BuildOptions.UserExtensions[UserID='APRIORI' Identifier='1']");
                }
            }
        }
        
        return result;
    }
    
    private Set<FpdModuleIdentification> getModuleSequenceForFv(String fvName) throws EdkException {
        Node node = saq.getFpdModuleSequence(fvName);
        Set<FpdModuleIdentification> result = new LinkedHashSet<FpdModuleIdentification>();
        
        if ( node == null) {
            EdkLog.log(this, EdkLog.EDK_WARNING, "FV[" + fvName + "] does not specify module sequence in FPD. Assuming present sequence as default sequence in FV. ");
            return null;
        } else {
            NodeList childNodes = node.getChildNodes();
            for (int i = 0; i < childNodes.getLength(); i++) {
                Node childItem = childNodes.item(i);
                if (childItem.getNodeType() == Node.ELEMENT_NODE) {
                    //
                    // Find child elements "IncludeModules"
                    //
                    if (childItem.getNodeName().compareTo("IncludeModules") == 0) {
                        //
                        // result will be updated
                        //
                        processNodes(childItem, result);
                    } else if (childItem.getNodeName().compareTo("FvName") == 0) {
                        
                    } else if (childItem.getNodeName().compareTo("InfFileName") == 0) {
                        
                    } else {
                        //
                        // Report Warning
                        //
                        EdkLog.log(this, EdkLog.EDK_WARNING, "Unrecognised element " + childItem.getNodeName() + " under FPD.BuildOptions.UserExtensions[UserID='IMAGES' Identifier='1']");
                    }
                }
            }
        }
        
        return result;
    }
    
    private void processNodes(Node node, Set<FpdModuleIdentification> result) throws EdkException {
        //
        // Found out all elements "Module"
        //
        NodeList childNodes = node.getChildNodes();
        for (int j = 0; j < childNodes.getLength(); j++) {
            Node childItem = childNodes.item(j);
            if (childItem.getNodeType() == Node.ELEMENT_NODE) {
                if (childItem.getNodeName().compareTo("Module") == 0) {
                    String moduleGuid = null;
                    String moduleVersion = null;
                    String packageGuid = null;
                    String packageVersion = null;
                    String arch = null;
                    
                    NamedNodeMap attr = childItem.getAttributes();
                    for (int i = 0; i < attr.getLength(); i++) {
                        Node attrItem = attr.item(i);
                        if (attrItem.getNodeName().compareTo("ModuleGuid") == 0) {
                            moduleGuid = attrItem.getNodeValue();
                        } else if (attrItem.getNodeName().compareTo("ModuleVersion") == 0) {
                            moduleVersion = attrItem.getNodeValue();
                        } else if (attrItem.getNodeName().compareTo("PackageGuid") == 0) {
                            packageGuid = attrItem.getNodeValue();
                        } else if (attrItem.getNodeName().compareTo("PackageVersion") == 0) {
                            packageVersion = attrItem.getNodeValue();
                        } else if (attrItem.getNodeName().compareTo("Arch") == 0) {
                            arch = attrItem.getNodeValue();
                        } else {
                            //
                            // Report warning
                            //
                            EdkLog.log(this, EdkLog.EDK_WARNING, "Unrecognised attribute " + attrItem.getNodeName() + " under FPD.BuildOptions.UserExtensions[UserID='IMAGES' Identifier='1'].IncludeModules.Module");
                        }
                    }
                    
                    PackageIdentification packageId = new PackageIdentification(packageGuid, packageVersion);
                    GlobalData.refreshPackageIdentification(packageId);
                    
                    ModuleIdentification moduleId = new ModuleIdentification(moduleGuid, moduleVersion);
                    moduleId.setPackage(packageId);
                    GlobalData.refreshModuleIdentification(moduleId);
                    
                    if (arch == null) {
                        throw new EdkException("Attribute [Arch] is required for element FPD.BuildOptions.UserExtensions[UserID='IMAGES' Identifier='1'].IncludeModules.Module. ");
                    }
                    
                    result.add(new FpdModuleIdentification(moduleId, arch));
                } else {
                    //
                    // Report Warning
                    //
                    EdkLog.log(this, EdkLog.EDK_WARNING, "Unrecognised element " + childItem.getNodeName() + " under FPD.BuildOptions.UserExtensions[UserID='IMAGES' Identifier='1'].IncludeModules");
                }
            }
        }
    }


    private void checkToolChain() throws EdkException {
        ToolChainInfo toolChainInfo = GlobalData.getToolChainInfo();

        if (toolChainInfo.getTargets().length == 0) {
            throw new EdkException("No valid target found! "+
                                   "Please check the TARGET definition in Tools/Conf/target.txt, "+
                                   "or the <BuildTarget>, <BuildOptions> in the FPD file.");
        }

        if (toolChainInfo.getTagnames().length == 0) {
            throw new EdkException("No valid tool chain found! "+
                                   "Please check the TOOL_CHAIN_TAG definition in Tools/Conf/target.txt, "+
                                   "or the <BuildOptions> in the FPD file.");
        }

        if (toolChainInfo.getArchs().length == 0) {
            throw new EdkException("No valid architecture found! "+
                                   "Please check the TARGET_ARCH definition in Tools/Conf/target.txt, "+
                                   "or the <SupportedArchitectures>, <BuildOptions> in the FPD file.");
        }

        if (toolChainInfo.getCommands().length == 0) {
            throw new EdkException("No valid COMMAND found! Please check the tool chain definitions "+
                                   "in Tools/Conf/tools_def.txt.");
        }
    }
}
