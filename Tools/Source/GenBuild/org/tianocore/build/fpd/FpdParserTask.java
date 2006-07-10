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
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;
import java.util.Vector;
import java.util.TreeMap;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.taskdefs.Ant;
import org.apache.tools.ant.taskdefs.Property;
import org.apache.xmlbeans.XmlObject;

import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.OutputManager;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.id.FpdModuleIdentification;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.build.id.PlatformIdentification;
import org.tianocore.build.pcd.action.ActionMessage;
import org.tianocore.build.pcd.action.CollectPCDAction;
import org.tianocore.build.pcd.exception.EntityException;
import org.tianocore.build.toolchain.ToolChainAttribute;
import org.tianocore.build.toolchain.ToolChainElement;
import org.tianocore.build.toolchain.ToolChainMap;
import org.tianocore.exception.EdkException;

/**
  <code>FpdParserTask</code> is an ANT task. The main function is parsing Framework
  Platform Descritpion (FPD) XML file and generating its ANT build script for 
  corresponding platform.  

  <p>The task sets global properties PLATFORM, PLATFORM_DIR, PLATFORM_RELATIVE_DIR
  and BUILD_DIR. </p>
  
  <p>The task generates ${PLATFORM}_build.xml file which will be called by top level
  build.xml. The task also generate Fv.inf files (File is for Tool GenFvImage) 
  and flash definition file (File is for Tool FlashMap) if necessary. </p>
  
  <p>FpdParserTask task stores all FPD information to GlobalData. And parse
  tools definition file to set up compiler options for different Target and
  different ToolChainTag. </p>
  
  <p>The method parseFpdFile is also prepared for single module build. </p>
  
  <p>The usage is (take NT32 Platform for example):</p>

  <pre>
  &lt;FPDParser platformName="Nt32" /&gt;
  </pre>

  <p>The task will initialize all information through parsing Framework Database, 
  SPD, Tool chain configuration files. </p>

  @since GenBuild 1.0
**/
public class FpdParserTask extends Task {
    
    private String platformName;

    private File fpdFile = null;
    
    private PlatformIdentification platformId;
    
    ///
    /// 
    ///
    private String type;
    
    ///
    /// Mapping from modules identification to out put file name
    ///
    private Map<FpdModuleIdentification, String> outfiles = new LinkedHashMap<FpdModuleIdentification, String>();

    ///
    /// Mapping from FV name to its modules
    ///
    private Map<String, Set<FpdModuleIdentification>> fvs = new HashMap<String, Set<FpdModuleIdentification>>();

    ///
    /// Mapping from sequence number to FV names
    ///
    private Map<String, Set<String>> sequences = new TreeMap<String, Set<String>>();

    ///
    /// FpdParserTask can specify some ANT properties. 
    ///
    private Vector<Property> properties = new Vector<Property>();
    
    private boolean isUnified = true;


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
        // Remove !!
        if ( fpdFile == null) {
            if (platformName == null) {
                throw new BuildException("FpdParserTask parameter error. Please specify platform name or FPD file. ");
            }
            platformId = GlobalData.getPlatformByName(platformName);
            fpdFile = platformId.getFpdFile();
        }
        
        //
        // Parse FPD file
        //
        parseFpdFile();
        
        //
        // Prepare BUILD_DIR
        //
        isUnified = OutputManager.getInstance().prepareBuildDir(getProject());
        
        //
        // Generate FDF (Flash Definition File) file
        //

        //
        // For every Target and ToolChain
        //
        String[] targetList = GlobalData.getToolChainInfo().getTargets();
        for (int i = 0; i < targetList.length; i++){
            String[] toolchainList = GlobalData.getToolChainInfo().getTagnames();
            for(int j = 0; j < toolchainList.length; j++){
                //
                // Prepare FV_DIR
                //
                String ffsCommonDir = getProject().getProperty("BUILD_DIR") + File.separatorChar 
                                + targetList[i] + File.separatorChar 
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
        PlatformBuildFileGenerator fileGenerator = new PlatformBuildFileGenerator(getProject(), outfiles, fvs, sequences, isUnified);
        fileGenerator.genBuildFile();
        
        //
        // Ant call ${PLATFORM}_build.xml
        //
        
        Ant ant = new Ant();
        ant.setProject(getProject());
        ant.setAntfile(platformId.getFpdFile().getParent() + File.separatorChar + platformId.getName() + "_build.xml");
        ant.setTarget(type);
        ant.setInheritAll(true);
        ant.init();
        ant.execute();
        
//        GlobalData.log.info("Fpd build end. ");
    }

    /**
      Generate Fv.inf files. The Fv.inf file is composed with four 
      parts: Options, Attributes, Components and Files. The Fv.inf files 
      will be under FV_DIR.
     
      @throws BuildException
                  File write FV.inf files error. 
    **/
    private void genFvInfFiles(String ffsCommonDir) throws BuildException {
        String[] validFv = SurfaceAreaQuery.getFpdValidImageNames();
        for (int i = 0; i < validFv.length; i++) {
            //
            // Get all global variables from FPD and set them to properties
            //
            String[][] globalVariables = SurfaceAreaQuery.getFpdGlobalVariable();
            for (int j = 0; j < globalVariables.length; j++) {
                getProject().setProperty(globalVariables[j][0], globalVariables[j][1]);
            }

            getProject().setProperty("FV_FILENAME", validFv[i].toUpperCase());
            
            File fvFile = new File(getProject().replaceProperties( getProject().getProperty("FV_DIR") + File.separatorChar + validFv[i].toUpperCase() + ".inf"));
            fvFile.getParentFile().mkdirs();

            try {
                FileWriter fw = new FileWriter(fvFile);
                BufferedWriter bw = new BufferedWriter(fw);
                
                //
                // Options
                //
                String[][] options = SurfaceAreaQuery.getFpdOptions(validFv[i]);
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
                String[][] attributes = SurfaceAreaQuery.getFpdAttributes(validFv[i]);
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
                String[][] components = SurfaceAreaQuery.getFpdComponents(validFv[i]);
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
                Set<FpdModuleIdentification> filesSet = fvs.get(validFv[i].toUpperCase());
                if (filesSet != null) {
                    FpdModuleIdentification[] files = filesSet.toArray(new FpdModuleIdentification[filesSet.size()]);
                    bw.write("[files]");
                    bw.newLine();
                    for (int j = 0; j < files.length; j++) {
                        String str = ffsCommonDir + File.separatorChar + outfiles.get(files[j]);
                        bw.write(getProject().replaceProperties("EFI_FILE_NAME = " + str));
                        bw.newLine();
                    }
                }
                bw.flush();
                bw.close();
                fw.close();
            } catch (Exception e) {
                throw new BuildException("Generate FV file [" + fvFile.getPath() + "] failed. \n" + e.getMessage());
            }
        }
    }
    /**
      This method is used for Single Module Build.
      
      
      @throws BuildException
                  FPD file is not valid. 
    **/
    public void parseFpdFile(File fpdFile) throws BuildException {
        this.fpdFile = fpdFile;
        parseFpdFile();
    }

    /**
      Parse FPD file. 
     
      @throws BuildException
                  FPD file is not valid. 
     **/
    private void parseFpdFile() throws BuildException {
        try {
            XmlObject doc = XmlObject.Factory.parse(fpdFile);
            
            if (!doc.validate()) {
                throw new BuildException("Platform Surface Area file [" + fpdFile.getPath() + "] is invalid.");
            }
            
            Map<String, XmlObject> map = new HashMap<String, XmlObject>();
            map.put("PlatformSurfaceArea", doc);
            SurfaceAreaQuery.setDoc(map);

            //
            // Initialize
            //
            platformId = SurfaceAreaQuery.getFpdHeader();
            platformId.setFpdFile(fpdFile);
            getProject().setProperty("PLATFORM", platformId.getName());
            getProject().setProperty("PLATFORM_FILE", platformId.getRelativeFpdFile().replaceAll("(\\\\)", "/"));
            getProject().setProperty("PLATFORM_DIR", platformId.getFpdFile().getParent().replaceAll("(\\\\)", "/"));
            getProject().setProperty("PLATFORM_RELATIVE_DIR", platformId.getPlatformRelativeDir().replaceAll("(\\\\)", "/"));

            //
            // Build mode. User-defined output dir. 
            //
            String buildMode = SurfaceAreaQuery.getFpdIntermediateDirectories();
            String userDefinedOutputDir = SurfaceAreaQuery.getFpdOutputDirectory();

            OutputManager.getInstance().setup(userDefinedOutputDir, buildMode);

            //
            // TBD. Deal PCD and BuildOption related Info
            //
            GlobalData.setFpdBuildOptions(SurfaceAreaQuery.getFpdBuildOptions());
            
            GlobalData.setToolChainPlatformInfo(SurfaceAreaQuery.getFpdToolChainInfo());
            
            //
            // Parse all list modules SA
            //
            parseModuleSAFiles();

            //
            // TBD. Deal PCD and BuildOption related Info
            //
            parseToolChainFamilyOptions();
            parseToolChainOptions();

            SurfaceAreaQuery.setDoc(map);
            
            //
            // Pcd Collection. Call CollectPCDAction to collect pcd info.
            //
            try {
                CollectPCDAction ca = new CollectPCDAction();
                ca.perform(GlobalData.getWorkspacePath(),platformId.getFpdFile().getPath(),ActionMessage.NULL_MESSAGE_LEVEL);
            } catch (Exception e){
                throw new BuildException(e.getMessage());
            }
        } catch (Exception e) {
            throw new BuildException("Load FPD file [" + fpdFile.getPath() + "] error. \n" + e.getMessage());
        }
    }


    
    /**
      Parse all modules listed in FPD file. 
    **/
    private void parseModuleSAFiles() throws EdkException{
        Map<FpdModuleIdentification, Map<String, XmlObject>> moduleSAs = SurfaceAreaQuery.getFpdModules();

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
            SurfaceAreaQuery.push(GlobalData.getDoc(fpdModuleId));
            String fvBinding = SurfaceAreaQuery.getModuleFvBindingKeyword();
            SurfaceAreaQuery.pop();

            fpdModuleId.setFvBinding(fvBinding);
            String fvSequence = fpdModuleId.getSequence();
            updateFvs(fvSequence, fvBinding, fpdModuleId);
            
            //
            // Prepare for out put file name
            //
            ModuleIdentification moduleId = fpdModuleId.getModule();
            SurfaceAreaQuery.push(GlobalData.getDoc(fpdModuleId));
            String baseName = SurfaceAreaQuery.getModuleOutputFileBasename();
            SurfaceAreaQuery.pop();
            if (baseName == null) {
                baseName = moduleId.getName();
            }
            outfiles.put(fpdModuleId, fpdModuleId.getArch() + File.separatorChar 
                         + moduleId.getGuid() + "-" + baseName 
                         + getSuffix(moduleId.getModuleType()));

            //
            // parse module build options, if any
            // 
            SurfaceAreaQuery.push(GlobalData.getDoc(fpdModuleId));
            GlobalData.addModuleToolChainOption(fpdModuleId, parseModuleBuildOptions(false));
            GlobalData.addModuleToolChainFamilyOption(fpdModuleId, parseModuleBuildOptions(true));
            SurfaceAreaQuery.pop();
        }
    }

    private ToolChainMap parseModuleBuildOptions(boolean toolChainFamilyFlag) throws EdkException {
        String[][] options = SurfaceAreaQuery.getModuleBuildOptions(toolChainFamilyFlag);
        if (options == null || options.length == 0) {
            return null;
        }
        return parseOptions(options);
    }
    
    private ToolChainMap parsePlatformBuildOptions(boolean toolChainFamilyFlag) throws EdkException {
        String[][] options = SurfaceAreaQuery.getPlatformBuildOptions(toolChainFamilyFlag);
        if (options == null || options.length == 0) {
            return null;
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
            options[i][flagIndex] = ToolChainAttribute.FLAGS + "";
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
    private void updateFvs(String fvSequence, String fvName, FpdModuleIdentification fpdModuleId) {
        String upcaseFvName = fvName.toUpperCase();
        String[] fvNameArray = upcaseFvName.split("[, \t]+");
        for (int i = 0; i < fvNameArray.length; i++) {
            //
            // Put module to corresponding fvName
            //
            if (fvs.containsKey(fvNameArray[i])) {
                Set<FpdModuleIdentification> set = fvs.get(fvNameArray[i]);
                set.add(fpdModuleId);
            }
            else {
                Set<FpdModuleIdentification> set = new LinkedHashSet<FpdModuleIdentification>();
                set.add(fpdModuleId);
                fvs.put(fvNameArray[i], set);
            }
            
            //
            // Put fvName to corresponding fvSequence
            //
            if (sequences.containsKey(fvSequence)) {
                Set<String> set = sequences.get(fvSequence);
                set.add(fvNameArray[i]);
            }
            else {
                Set<String> set = new LinkedHashSet<String>();
                set.add(fvNameArray[i]);
                sequences.put(fvSequence, set);
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

        String[][] suffix = { { "BASE", ".FFS"},
                              { "SEC", ".SEC" }, { "PEI_CORE", ".PEI" }, 
                              { "PEIM", ".PEI" }, { "DXE_CORE", ".DXE" },
                              { "DXE_DRIVER", ".DXE" }, { "DXE_RUNTIME_DRIVER", ".DXE" }, 
                              { "DXE_SAL_DRIVER", ".DXE" }, { "DXE_SMM_DRIVER", ".DXE" }, 
                              { "TOOL", ".FFS" }, { "UEFI_DRIVER", ".DXE" },
                              { "UEFI_APPLICATION", ".APP" }, { "USER_DEFINED", ".FFS" } };
        
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

    public void setPlatformName(String platformName) {
        this.platformName = platformName;
    }

    public void setFpdFile(File fpdFile) {
        this.fpdFile = fpdFile;
    }

    public void setType(String type) {
        this.type = type;
    }
    

}
