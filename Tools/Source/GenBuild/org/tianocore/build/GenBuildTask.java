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
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.Vector;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.taskdefs.Ant;
import org.apache.xmlbeans.XmlObject;
import org.w3c.dom.Comment;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;

import org.tianocore.build.autogen.AutoGen;
import org.tianocore.build.autogen.CommonDefinition;
import org.tianocore.build.fpd.FpdParserTask;
import org.tianocore.build.global.GenBuildLogger;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.OutputManager;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.toolchain.ToolChainFactory;
import org.tianocore.logger.EdkLog;
import org.tianocore.FilenameDocument;
import org.tianocore.MsaHeaderDocument;
import org.tianocore.MsaLibHeaderDocument;

/**
  <p>
  <code>GenBuildTask</code> is an ANT task that can be used in ANT build
  system. The main function of this task is to parse module's surface area,
  then generate the corresponding <em>BaseName_build.xml</em> (the real ANT
  build script) and call this to build the module.
  </p>
  
  <p>
  The usage is (take module <em>HelloWorld</em> for example):
  </p>
  
  <pre>
   &lt;GenBuild baseName=&quot;HelloWorld&quot; 
             mbdFilename=&quot;${MODULE_DIR}/HelloWorld.mbd&quot; 
             msaFilename=&quot;${MODULE_DIR}/HelloWorld.msa&quot;/&gt;
  </pre>
  
  <p>
  This task calls <code>AutoGen</code> to generate <em>AutoGen.c</em> and
  <em>AutoGen.h</em>. The task also parses the development environment
  configuration files, such as collecting package information, setting compiler
  flags and so on.
  </p>
  
  
  @since GenBuild 1.0
**/
public class GenBuildTask extends Task {

    ///
    /// Module surface area file.
    ///
    File msaFilename;

    ///
    /// Module build description file.
    ///
    File mbdFilename;

    ///
    /// Module surface area information after overrided.
    ///
    public Map<String, XmlObject> map = new HashMap<String, XmlObject>();

    ///
    /// Module's base name.
    ///
    private String baseName;

    ///
    /// Current build Arch, such as IA32, X64, IPF and so on.
    ///
    private String arch;

    ///
    /// Module's GUID (Globally Unique Identifier).
    ///
    private String guid;

    ///
    /// Module's component type, such as SEC, LIBRARY, BS_DRIVER and so on.
    ///
    private String componentType;

    ///
    /// This value is used in build time. Override module's component type. When
    /// search FFS (Sections information) in common file, buildtype instead of
    /// component type.
    ///
    private String buildType;

    ///
    /// List all required includes for current build module.
    ///
    public Set<String> includes = new LinkedHashSet<String>();

    ///
    /// List all libraries for current build module.
    ///
    public Set<String> libraries = new LinkedHashSet<String>();

    ///
    /// List all source files for current build module.
    ///
    public Set<String> sourceFiles = new LinkedHashSet<String>();

    ///
    /// Flag to identify what surface area files are specified. Current value is
    /// <em>NO_SA</em>, <em>ONLY_MSA</em>, <em>ONLY_LIBMSA</em>,
    /// <em>MSA_AND_MBD</em> or <em>LIBMSA_AND_LIBMBD</em>.
    /// 
    /// @see org.tianocore.build.global.GlobaData
    ///
    private int flag = GlobalData.NO_SA;

    ///
    /// The information at the header of <em>build.xml</em>.
    ///
    private String info = "====================================================================\n"
                        + "DO NOT EDIT \n"
                        + "File auto-generated by build utility\n"
                        + "\n"
                        + "Abstract:\n"
                        + "Auto-generated ANT build file for building of EFI Modules/Platforms\n"
                        + "=====================================================================";

    /**
      Public construct method. It is necessary for ANT task.
    **/
    public GenBuildTask() {
    }

    /**
      ANT task's entry point, will be called after init(). The main steps is described
      as following: 
      <ul>
      <li> Judge current build mode (MODULE | PACKAGE | PLATFORM). This step will execute
      only once in whole build process; </li>
      <li> Initialize global information (Framework DB, SPD files and all MSA files 
      listed in SPD). This step will execute only once in whole build process; </li>
      <li> Restore some important ANT property. If current build is single module 
      build, here will set many default values; </li>
      <li> Get the current module's overridded surface area information from 
      global data; </li> 
      <li> Set up the output directories, including BIN_DIR, DEST_DIR_OUTPUT and
      DEST_DIR_DEBUG; </li>
      <li> Get module dependent library instances and include pathes; </li>
      <li> Judge whether current module is built. If yes, skip it; </li>
      <li> Call AutoGen and PCD to generate AutoGen.c & AutoGen.h </li>
      <li> Set up the compile flags; </li>
      <li> Generate BaseName_build.xml; </li>
      <li> Call to BaseName_build.xml, and build the current module. </li>
      </ul>
      
      <p>Build is dependent on BuildMacro.xml which define many macro. </p> 
      
      @throws BuildException
              From module build, exception from module surface area invalid.
    **/
    public void execute() throws BuildException {
        System.out.println("Module [" + baseName + "] start.");
        //
        // Inital GenBuild log  method 
        //
        GenBuildLogger logger = new GenBuildLogger(getProject());
        EdkLog.setLogger(logger);
        EdkLog.setLogLevel(1);
        
        OutputManager.update(getProject());
        GlobalData.initInfo("Tools" + File.separatorChar + "Conf" + File.separatorChar + "FrameworkDatabase.db",
                            getProject().getProperty("WORKSPACE_DIR"));
        recallFixedProperties();
        arch = getProject().getProperty("ARCH");
        arch = arch.toUpperCase();
        map = GlobalData.getDoc(baseName);
        //
        // Initialize SurfaceAreaQuery
        //
        SurfaceAreaQuery.setDoc(map);
        //
        // Setup Output Management
        //
        String[] outdir = SurfaceAreaQuery.getOutputDirectory();
        OutputManager.update(getProject(), outdir[1], outdir[0]);

        updateIncludesAndLibraries();

        if (GlobalData.isModuleBuilt(baseName, arch)) {
            return;
        } else {
            GlobalData.registerBuiltModule(baseName, arch);
        }
        //
        // Call AutoGen
        //
        AutoGen autogen = new AutoGen(getProject().getProperty("DEST_DIR_DEBUG"), baseName, arch);
        autogen.genAutogen();
        //
        // Update parameters
        //
        updateParameters();
        //
        // Update flags like CC_FLAGS, LIB_FLAGS etc.
        //
        flagsSetup();
        GlobalData.addLibrary(baseName, arch, getProject().getProperty("BIN_DIR") + File.separatorChar + baseName + ".lib");
        GlobalData.addModuleLibrary(baseName, arch, libraries);
        //
        // If ComponentType is USER_DEFINED,
        // then call the exist BaseName_build.xml directly.
        //
        if (buildType.equalsIgnoreCase("CUSTOM_BUILD")) {
            System.out.println("Call user-defined " + baseName + "_build.xml");
            Ant ant = new Ant();
            ant.setProject(getProject());
            ant.setAntfile(getProject().getProperty("MODULE_DIR") + File.separatorChar + baseName + "_build.xml");
            ant.setInheritAll(true);
            ant.init();
            ant.execute();
            return;
        }
        //
        // Generate ${BASE_NAME}_build.xml file
        //
        System.out.println("Generate " + baseName + "_build.xml");
        genBuildFile();
        System.out.println("Call the " + baseName + "_build.xml");
        Ant ant = new Ant();
        ant.setProject(getProject());
        ant.setAntfile(getProject().getProperty("DEST_DIR_OUTPUT") + File.separatorChar + baseName + "_build.xml");
        ant.setInheritAll(true);
        ant.init();
        ant.execute();
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

    /**
      Get the dependent library instances and include package name from 
      surface area, and initialize module include pathes. 
     
    **/
    private void updateIncludesAndLibraries() {
        List<String> rawIncludes = SurfaceAreaQuery.getIncludePackageName(arch);
        if (rawIncludes != null) {
            Iterator iter = rawIncludes.iterator();
            while (iter.hasNext()) {
                String packageName = (String) iter.next();
                includes.add("${WORKSPACE_DIR}" + File.separatorChar + GlobalData.getPackagePath(packageName)
                             + File.separatorChar + "Include");
                includes.add("${WORKSPACE_DIR}" + File.separatorChar + GlobalData.getPackagePath(packageName)
                             + File.separatorChar + "Include" + File.separatorChar + archDir(arch));
            }
        }
        includes.add("${DEST_DIR_DEBUG}");
        List<String> rawLibraries = SurfaceAreaQuery.getLibraryInstance(this.arch, CommonDefinition.AlwaysConsumed);
        if (rawLibraries != null) {
            Iterator iter = rawLibraries.iterator();
            while (iter.hasNext()) {
                libraries.add((String) iter.next());
            }
        }
        normalize();
    }

    /**
      Normalize all dependent library instance and include pathes' format. 
     
    **/
    private void normalize() {
        String[] includesArray = includes.toArray(new String[includes.size()]);
        includes.clear();
        for (int i = 0; i < includesArray.length; i++) {
            includes.add((new File(includesArray[i])).getPath());
        }
        String[] librariesArray = libraries.toArray(new String[libraries.size()]);
        libraries.clear();
        for (int i = 0; i < librariesArray.length; i++) {
            libraries.add((new File(librariesArray[i])).getPath());
        }
    }

    /**
      Restore some important ANT property. If current build is single module 
      build, here will set many default values.
      
      <p> If current build is single module build, then the default <code>ARCH</code>
      is <code>IA32</code>. Also set up the properties <code>PACKAGE</code>, 
      <code>PACKAGE_DIR</code>, <code>TARGET</code> and <code>MODULE_DIR</code></p>
      
      <p> Note that for package build, package name is stored in <code>PLATFORM</code>
      and package directory is stored in <code>PLATFORM_DIR</code>. </p> 
     
      @see org.tianocore.build.global.OutputManager
    **/
    private void recallFixedProperties() {
        //
        // If build is for module build
        //
        if (getProject().getProperty("PACKAGE_DIR") == null) {
            ToolChainFactory toolChainFactory = new ToolChainFactory(getProject());
            toolChainFactory.setupToolChain();
            //
            // PACKAGE PACKAGE_DIR ARCH (Default) COMMON_FILE BUILD_MACRO
            //
            if (getProject().getProperty("ARCH") == null) {
                getProject().setProperty("ARCH", "IA32");
            }
            String packageName = GlobalData.getPackageNameForModule(baseName);
            getProject().setProperty("PACKAGE", packageName);
            
            String packageDir = GlobalData.getPackagePath(packageName);
            getProject().setProperty("PACKAGE_DIR",
                                     getProject().getProperty("WORKSPACE_DIR") + File.separatorChar + packageDir);
            
            getProject().setProperty("TARGET", toolChainFactory.getCurrentTarget());
            
            getProject().setProperty("MODULE_DIR",
                                     getProject().replaceProperties(getProject().getProperty("MODULE_DIR")));
        }
        if (OutputManager.PLATFORM != null) {
            getProject().setProperty("PLATFORM", OutputManager.PLATFORM);
        }
        if (OutputManager.PLATFORM_DIR != null) {
            getProject().setProperty("PLATFORM_DIR", OutputManager.PLATFORM_DIR);
        }
    }

    /**
      The whole BaseName_build.xml is composed of seven part. 
      <ul>
      <li> ANT properties; </li>
      <li> Dependent module (dependent library instances in most case); </li>
      <li> Source files; </li>
      <li> Sections if module is not library; </li>
      <li> Output (different for library module and driver module); </li>
      <li> Clean; </li>
      <li> Clean all. </li>
      </ul>
      
      @throws BuildException
              Error throws during BaseName_build.xml generating. 
    **/
    private void genBuildFile() throws BuildException {
        FfsProcess fp = new FfsProcess();
        DocumentBuilderFactory domfac = DocumentBuilderFactory.newInstance();
        try {
            DocumentBuilder dombuilder = domfac.newDocumentBuilder();
            Document document = dombuilder.newDocument();
            Comment rootComment = document.createComment(info);
            //
            // create root element and its attributes
            //
            Element root = document.createElement("project");
            //
            // root.setAttribute("name", base_name);
            //
            root.setAttribute("default", "main");
            root.setAttribute("basedir", ".");
            //
            // element for External ANT tasks
            //
            root.appendChild(document.createComment("Apply external ANT tasks"));
            Element ele = document.createElement("taskdef");
            ele.setAttribute("resource", "frameworktasks.tasks");
            root.appendChild(ele);
            ele = document.createElement("taskdef");
            ele.setAttribute("resource", "cpptasks.tasks");
            root.appendChild(ele);
            ele = document.createElement("typedef");
            ele.setAttribute("resource", "cpptasks.types");
            root.appendChild(ele);
            ele = document.createElement("taskdef");
            ele.setAttribute("resource", "net/sf/antcontrib/antlib.xml");
            root.appendChild(ele);
            //
            // elements for Properties
            //
            root.appendChild(document.createComment("All Properties"));
            ele = document.createElement("property");
            ele.setAttribute("name", "BASE_NAME");
            ele.setAttribute("value", baseName);
            root.appendChild(ele);
            //
            // Generate the default target,
            // which depends on init, sections and output target
            //
            root.appendChild(document.createComment("Default target"));
            ele = document.createElement("target");
            ele.setAttribute("name", "main");
            ele.setAttribute("depends", "libraries, sourcefiles, sections, output");
            root.appendChild(ele);
            //
            // compile all source files
            //
            root.appendChild(document.createComment("Compile all dependency Library instances."));
            ele = document.createElement("target");
            ele.setAttribute("name", "libraries");
            //
            // Parse all sourfiles but files specified in sections
            //
            applyLibraryInstance(document, ele);
            root.appendChild(ele);
            //
            // compile all source files
            //
            root.appendChild(document.createComment("sourcefiles target"));
            ele = document.createElement("target");
            ele.setAttribute("name", "sourcefiles");
            //
            // Parse all sourfiles but files specified in sections
            //
            applyCompileElement(document, ele);
            root.appendChild(ele);
            //
            // generate the init target
            // main purpose is create all nessary pathes
            // generate the sections target
            //
            root.appendChild(document.createComment("sections target"));
            ele = document.createElement("target");
            ele.setAttribute("name", "sections");
            applySectionsElement(document, ele, fp);
            root.appendChild(ele);
            //
            // generate the output target
            //
            root.appendChild(document.createComment("output target"));
            ele = document.createElement("target");
            ele.setAttribute("name", "output");
            applyOutputElement(document, ele, fp);
            root.appendChild(ele);
            //
            // generate the clean target
            //
            root.appendChild(document.createComment("clean target"));
            ele = document.createElement("target");
            ele.setAttribute("name", "clean");
            applyCleanElement(document, ele);
            root.appendChild(ele);
            //
            // generate the Clean All target
            //
            root.appendChild(document.createComment("Clean All target"));
            ele = document.createElement("target");
            ele.setAttribute("name", "cleanall");
            applyDeepcleanElement(document, ele);
            root.appendChild(ele);
            //
            // add the root element to the document
            //
            document.appendChild(rootComment);
            document.appendChild(root);
            //
            // Prepare the DOM document for writing
            //
            Source source = new DOMSource(document);
            //
            // Prepare the output file
            //
            File file = new File(getProject().getProperty("DEST_DIR_OUTPUT") + File.separatorChar + baseName
                                 + "_build.xml");
            //
            // generate all directory path
            //
            (new File(file.getParent())).mkdirs();
            Result result = new StreamResult(file);
            //
            // Write the DOM document to the file
            //
            Transformer xformer = TransformerFactory.newInstance().newTransformer();
            xformer.setOutputProperty("{http://xml.apache.org/xslt}indent-amount", "2");
            xformer.setOutputProperty(OutputKeys.INDENT, "yes");
            xformer.transform(source, result);
        } catch (Exception ex) {
            throw new BuildException("Module [" + baseName + "] generating build file failed.\n" + ex.getMessage());
        }
    }

    /**
      Generate the clean elements for BaseName_build.xml. 
      
      @param document current BaseName_build.xml XML document
      @param root Root element for current
    **/
    private void applyCleanElement(Document document, Node root) {
        String[] libinstances = libraries.toArray(new String[libraries.size()]);
        for (int i = 0; i < libinstances.length; i++) {
            File file = new File(GlobalData.getModulePath(libinstances[i]) + File.separatorChar + "build.xml");

            Element ifEle = document.createElement("if");
            Element availableEle = document.createElement("available");
            availableEle.setAttribute("file", file.getPath());
            ifEle.appendChild(availableEle);
            Element elseEle = document.createElement("then");

            Element ele = document.createElement("ant");
            ele.setAttribute("antfile", file.getPath());
            ele.setAttribute("inheritAll", "false");
            ele.setAttribute("target", libinstances[i] + "_clean");
            //
            // Workspace_DIR
            //
            Element property = document.createElement("property");
            property.setAttribute("name", "WORKSPACE_DIR");
            property.setAttribute("value", "${WORKSPACE_DIR}");
            ele.appendChild(property);
            //
            // Package Dir
            //
            property = document.createElement("property");
            property.setAttribute("name", "PACKAGE_DIR");
            property.setAttribute("value", "${WORKSPACE_DIR}" + File.separatorChar
                                           + GlobalData.getPackagePathForModule(libinstances[i]));
            ele.appendChild(property);
            //
            // ARCH
            //
            property = document.createElement("property");
            property.setAttribute("name", "ARCH");
            property.setAttribute("value", "${ARCH}");
            ele.appendChild(property);
            //
            // TARGET
            //
            property = document.createElement("property");
            property.setAttribute("name", "TARGET");
            property.setAttribute("value", "${TARGET}");
            ele.appendChild(property);
            //
            // PACKAGE
            //
            property = document.createElement("property");
            property.setAttribute("name", "PACKAGE");
            property.setAttribute("value", GlobalData.getPackageNameForModule(libinstances[i]));
            ele.appendChild(property);

            elseEle.appendChild(ele);
            ifEle.appendChild(elseEle);
            root.appendChild(ifEle);
        }
    }

    /**
      Generate the cleanall elements for BaseName_build.xml. 
      
      @param document current BaseName_build.xml XML document
      @param root Root element for current
    **/
    private void applyDeepcleanElement(Document document, Node root) {
        String[] libinstances = libraries.toArray(new String[libraries.size()]);
        for (int i = 0; i < libinstances.length; i++) {
            File file = new File(GlobalData.getModulePath(libinstances[i]) + File.separatorChar + "build.xml");

            Element ifEle = document.createElement("if");
            Element availableEle = document.createElement("available");
            availableEle.setAttribute("file", file.getPath());
            ifEle.appendChild(availableEle);
            Element elseEle = document.createElement("then");

            Element ele = document.createElement("ant");
            ele.setAttribute("antfile", file.getPath());
            ele.setAttribute("inheritAll", "false");
            ele.setAttribute("target", libinstances[i] + "_cleanall");
            //
            // Workspace_DIR
            //
            Element property = document.createElement("property");
            property.setAttribute("name", "WORKSPACE_DIR");
            property.setAttribute("value", "${WORKSPACE_DIR}");
            ele.appendChild(property);
            //
            // Package Dir
            //
            property = document.createElement("property");
            property.setAttribute("name", "PACKAGE_DIR");
            property.setAttribute("value", "${WORKSPACE_DIR}" + File.separatorChar
                                           + GlobalData.getPackagePathForModule(libinstances[i]));
            ele.appendChild(property);
            //
            // ARCH
            //
            property = document.createElement("property");
            property.setAttribute("name", "ARCH");
            property.setAttribute("value", "${ARCH}");
            ele.appendChild(property);
            //
            // TARGET
            //
            property = document.createElement("property");
            property.setAttribute("name", "TARGET");
            property.setAttribute("value", "${TARGET}");
            ele.appendChild(property);
            //
            // PACKAGE
            //
            property = document.createElement("property");
            property.setAttribute("name", "PACKAGE");
            property.setAttribute("value", GlobalData.getPackageNameForModule(libinstances[i]));
            ele.appendChild(property);

            elseEle.appendChild(ele);
            ifEle.appendChild(elseEle);
            root.appendChild(ifEle);
        }
    }

    /**
      Generate the dependent library instances elements for BaseName_build.xml. 
      
      @param document current BaseName_build.xml XML document
      @param root Root element for current
    **/
    private void applyLibraryInstance(Document document, Node root) {
        String[] libinstances = libraries.toArray(new String[libraries.size()]);
        for (int i = 0; i < libinstances.length; i++) {
            Element ele = document.createElement("ant");
            File file = new File(GlobalData.getModulePath(libinstances[i]) + File.separatorChar + "build.xml");
            ele.setAttribute("antfile", file.getPath());
            ele.setAttribute("inheritAll", "false");
            ele.setAttribute("target", libinstances[i]);
            //
            // Workspace_DIR
            //
            Element property = document.createElement("property");
            property.setAttribute("name", "WORKSPACE_DIR");
            property.setAttribute("value", "${WORKSPACE_DIR}");
            ele.appendChild(property);
            //
            // Package Dir
            //
            property = document.createElement("property");
            property.setAttribute("name", "PACKAGE_DIR");
            property.setAttribute("value", "${WORKSPACE_DIR}" + File.separatorChar
                                           + GlobalData.getPackagePathForModule(libinstances[i]));
            ele.appendChild(property);
            //
            // ARCH
            //
            property = document.createElement("property");
            property.setAttribute("name", "ARCH");
            property.setAttribute("value", "${ARCH}");
            ele.appendChild(property);
            //
            // TARGET
            //
            property = document.createElement("property");
            property.setAttribute("name", "TARGET");
            property.setAttribute("value", "${TARGET}");
            ele.appendChild(property);
            //
            // PACKAGE
            //
            property = document.createElement("property");
            property.setAttribute("name", "PACKAGE");
            property.setAttribute("value", GlobalData.getPackageNameForModule(libinstances[i]));
            ele.appendChild(property);
            root.appendChild(ele);
        }
        Element expand = document.createElement("Expand");
        root.appendChild(expand);
    }
    
    /**
      Generate the build source files elements for BaseName_build.xml. 
      
      @param document current BaseName_build.xml XML document
      @param root Root element for current
    **/
    private void applyCompileElement(Document document, Node root) {
        FileProcess fileProcess = new FileProcess();
        fileProcess.init(getProject(), includes, sourceFiles, document);
        Node[] files = this.getSourceFiles();
        //
        // Parse all unicode files
        //
        for (int i = 0; i < files.length; i++) {
            String filetype = getFiletype(files[i]);
            if (filetype != null) {
                fileProcess.parseFile(getFilename(files[i]), filetype, root, true);
            } else {
                fileProcess.parseFile(getFilename(files[i]), root, true);
            }
        }
        if (fileProcess.isUnicodeExist()) {
            Element ele = document.createElement("Build_Unicode_Database");
            ele.setAttribute("FILEPATH", ".");
            ele.setAttribute("FILENAME", "${BASE_NAME}");
            root.appendChild(ele);
        }

        //
        // Parse AutoGen.c & AutoGen.h
        //
        if (!baseName.equalsIgnoreCase("Shell")) {
            fileProcess.parseFile(getProject().getProperty("DEST_DIR_DEBUG") + File.separatorChar + "AutoGen.c", root,
                                  false);
        }
        //
        // Parse all source files
        //
        for (int i = 0; i < files.length; i++) {
            String filetype = getFiletype(files[i]);
            if (filetype != null) {
                fileProcess.parseFile(getFilename(files[i]), filetype, root, false);
            } else {
                fileProcess.parseFile(getFilename(files[i]), root, false);
            }
        }
        //
        // root.appendChild(parallelEle);
        //
        Iterator iter = sourceFiles.iterator();
        String str = "";
        while (iter.hasNext()) {
            str += " " + (String) iter.next();
        }
        getProject().setProperty("SOURCE_FILES", str);
    }

    /**
      Generate the section elements for BaseName_build.xml. Library module will
      skip this process.  
      
      @param document current BaseName_build.xml XML document
      @param root Root element for current
    **/
    private void applySectionsElement(Document document, Node root, FfsProcess fp) {
        if (fp.initSections(buildType, getProject())) {
            String targetFilename = guid + "-" + baseName + FpdParserTask.getSuffix(componentType);
            String[] list = fp.getGenSectionElements(document, baseName, guid, targetFilename);

            for (int i = 0; i < list.length; i++) {
                Element ele = document.createElement(list[i]);
                ele.setAttribute("FILEPATH", ".");
                ele.setAttribute("FILENAME", "${BASE_NAME}");
                root.appendChild(ele);
            }
        }
    }

    /**
      Generate the output elements for BaseName_build.xml. If module is library,
      call the <em>LIB</em> command, else call the <em>GenFfs</em> command. 
      
      @param document current BaseName_build.xml XML document
      @param root Root element for current
    **/
    private void applyOutputElement(Document document, Node root, FfsProcess fp) {
        if (flag == GlobalData.ONLY_LIBMSA || flag == GlobalData.LIBMSA_AND_LIBMBD) {
            //
            // call Lib command
            //
            Element cc = document.createElement("Build_Library");
            cc.setAttribute("FILENAME", baseName);
            root.appendChild(cc);
        }
        //
        // if it is a module but library
        //
        else {
            if (fp.getFfsNode() != null) {
                root.appendChild(fp.getFfsNode());
            }
        }
    }

    /**
      Get file name from node. If some wrong, return string with zero length. 
      
       @param node Filename node of MSA/MBD or specified in each Section
       @return File name
    **/
    private String getFilename(Node node) {
        String path = null;
        String filename = "${MODULE_DIR}" + File.separatorChar;
        String str = "";
        try {
            FilenameDocument file = (FilenameDocument) XmlObject.Factory.parse(node);
            str = file.getFilename().getStringValue().trim();
            path = file.getFilename().getPath();
        } catch (Exception e) {
            str = "";
        }
        if (path != null) {
            filename += path + File.separatorChar + str;
        } else {
            filename += str;
        }
        return getProject().replaceProperties(filename);
    }

    /**
      Get file type from node. If some wrong or not specified, return 
      <code>null</code>.  
      
      @param node Filename node of MSA/MBD or specified in each Section
      @return File type
    **/
    private String getFiletype(Node node) {
        String str = null;
        try {
            FilenameDocument file = (FilenameDocument) XmlObject.Factory.parse(node);
            str = file.getFilename().getFileType();
        } catch (Exception e) {
            str = null;
        }
        return str;
    }

    /**
      Return all source files but AutoGen.c.
      
      @return source files Node array
    **/
    public Node[] getSourceFiles() {
        XmlObject[] files = SurfaceAreaQuery.getSourceFiles(arch);
        if (files == null) {
            return new Node[0];
        }
        Vector<Node> vector = new Vector<Node>();
        for (int i = 0; i < files.length; i++) {
            vector.addElement(files[i].getDomNode());
        }
        //
        // To be consider sourcefiles from Sections
        //
        return vector.toArray(new Node[vector.size()]);
    }

    /**
      Get current module's base name. 
      
      @return base name
    **/
    public String getBaseName() {
        return baseName;
    }

    /**
      Set MBD surface area file. For ANT use.
      
      @param mbdFilename Surface Area file
    **/
    public void setMbdFilename(File mbdFilename) {
        this.mbdFilename = mbdFilename;
    }

    /**
      Set MSA surface area file. For ANT use.
      
      @param msaFilename Surface Area file
    **/
    public void setMsaFilename(File msaFilename) {
        this.msaFilename = msaFilename;
    }

    /**
      Compile flags setup. 
      
      <p> Take command <code>CC</code> and arch <code>IA32</code> for example, 
      Those flags are from <code>ToolChainFactory</code>: </p>
      <ul>
      <li> IA32_CC </li>
      <li> IA32_CC_STD_FLAGS </li>
      <li> IA32_CC_GLOBAL_FLAGS </li>
      <li> IA32_CC_GLOBAL_ADD_FLAGS </li>
      <li> IA32_CC_GLOBAL_SUB_FLAGS </li>
      </ul>
      Those flags can user-define: 
      <ul>
      <li> IA32_CC_PROJ_FLAGS </li>
      <li> IA32_CC_PROJ_ADD_FLAGS </li>
      <li> IA32_CC_PROJ_SUB_FLAGS </li>
      <li> CC_PROJ_FLAGS </li>
      <li> CC_PROJ_ADD_FLAGS </li>
      <li> CC_PROJ_SUB_FLAGS </li>
      <li> CC_FLAGS </li>
      <li> IA32_CC_FLAGS </li>
      </ul>
      
      <p> The final flags is composed of STD, GLOBAL and PROJ. If CC_FLAGS or
      IA32_CC_FLAGS is specified, STD, GLOBAL and PROJ will not affect. </p>
      
      Note that the <code>ToolChainFactory</code> executes only once 
      during whole build process. 
    **/
    private void flagsSetup() {
        Project project = getProject();
        //
        // If ToolChain has been set up before, do nothing.
        //
        ToolChainFactory toolChainFactory = new ToolChainFactory(project);
        toolChainFactory.setupToolChain();

        String[] cmd = ToolChainFactory.commandType;
        Set<String> addSet = new HashSet<String>(40);
        Set<String> subSet = new HashSet<String>(40);
        for (int i = 0; i < cmd.length; i++) {
            String str = ToolChainFactory.getValue(arch + "_" + cmd[i]);
            //
            // Command line path+command name
            //
            if (str != null) {
                project.setProperty(cmd[i], str);
            }
            //
            // ARCH_CMD_STD_FLAGS
            //
            str = ToolChainFactory.getValue(arch + "_" + cmd[i] + "_STD_FLAGS");
            if (str != null) {
                putFlagsToSet(addSet, str);
                project.setProperty(cmd[i] + "_STD_FLAGS", str);
            }
            //
            // ARCH_CMD_GLOBAL_FLAGS
            //
            str = ToolChainFactory.getValue(arch + "_" + cmd[i] + "_GLOBAL_FLAGS");
            if (str != null) {
                putFlagsToSet(addSet, str);
            }
            //
            // ARCH_CMD_GLOBAL_ADD_FLAGS
            //
            str = ToolChainFactory.getValue(arch + "_" + cmd[i] + "_GLOBAL_ADD_FLAGS");
            if (str != null) {
                putFlagsToSet(addSet, str);
            }
            //
            // ARCH_CMD_GLOBAL_SUB_FLAGS
            //
            str = ToolChainFactory.getValue(arch + "_" + cmd[i] + "_GLOBAL_SUB_FLAGS");
            if (str != null) {
                putFlagsToSet(subSet, str);
            }
            //
            // ARCH_CMD_PROJ_FLAGS
            //
            str = project.getProperty(arch + "_" + cmd[i] + "_PROJ_FLAGS");
            if (str != null) {
                putFlagsToSet(addSet, str);
            }
            //
            // ARCH_CMD_PROG_FLAGS
            //
            str = project.getProperty(arch + "_" + cmd[i] + "_PROJ_ADD_FLAGS");
            if (str != null) {
                putFlagsToSet(addSet, str);
            }
            //
            // ARCH_CMD_PROG_FLAGS
            //
            str = project.getProperty(arch + "_" + cmd[i] + "_PROJ_SUB_FLAGS");
            if (str != null) {
                putFlagsToSet(subSet, str);
            }
            //
            // CMD_PROJ_FLAGS
            //
            str = project.getProperty(cmd[i] + "_PROJ_FLAGS");
            if (str != null) {
                putFlagsToSet(addSet, str);
            }
            //
            // CMD_PROG_FLAGS
            //
            str = project.getProperty(cmd[i] + "_PROJ_ADD_FLAGS");
            if (str != null) {
                putFlagsToSet(addSet, str);
            }
            //
            // CMD_PROG_FLAGS
            //
            str = project.getProperty(cmd[i] + "_PROJ_SUB_FLAGS");
            if (str != null) {
                putFlagsToSet(subSet, str);
            }
            //
            // If IA32_CC_FLAGS or IA32_LIB_FLAGS .. has defined in BuildOptions
            //
            if ((str = project.getProperty(arch + "_" + cmd[i] + "_FLAGS")) != null) {
                project.setProperty(cmd[i] + "_FLAGS", getRawFlags(addSet, subSet));
                addSet.clear();
                subSet.clear();
                putFlagsToSet(addSet, project.replaceProperties(str));
                project.setProperty(cmd[i] + "_FLAGS", project.replaceProperties(getFlags(addSet, subSet)));
                addSet.clear();
                subSet.clear();
            }
            //
            // If CC_FLAGS or LIB_FLAGS .. has defined in BuildOptions
            //
            else if ((str = project.getProperty(cmd[i] + "_FLAGS")) != null) {
                project.setProperty(cmd[i] + "_FLAGS", getRawFlags(addSet, subSet));
                addSet.clear();
                subSet.clear();
                putFlagsToSet(addSet, project.replaceProperties(str));
                project.setProperty(cmd[i] + "_FLAGS", project.replaceProperties(getFlags(addSet, subSet)));
                addSet.clear();
                subSet.clear();
            } else {
                project.setProperty(cmd[i] + "_FLAGS", getFlags(addSet, subSet));
                addSet.clear();
                subSet.clear();
            }
        }
        project.setProperty("C_FLAGS", project.getProperty("CC_FLAGS"));
    }

    /**
      Initialize some properties will be used in current module build, including
      user-defined option from <em>Option</em> of <em>BuildOptions</em> in 
      surface area. 
    **/
    private void updateParameters() {
        getProject().setProperty("OBJECTS", "");
        getProject().setProperty("SDB_FILES", "");
        getProject().setProperty("BASE_NAME", baseName);
        if (map.get("MsaHeader") != null) {
            guid = SurfaceAreaQuery.getModuleGuid();//header.getGuid().getStringValue();
            componentType = SurfaceAreaQuery.getComponentType();//header.getComponentType().toString();
            if (!componentType.equalsIgnoreCase("LIBRARY")) {
                flag = GlobalData.MSA_AND_MBD;
            } else {
                flag = GlobalData.LIBMSA_AND_LIBMBD;
            }
        } 
        
        else if (map.get("MsaLibHeader") != null) {
            flag = GlobalData.LIBMSA_AND_LIBMBD;
            MsaLibHeaderDocument.MsaLibHeader header = ((MsaLibHeaderDocument) map.get("MsaLibHeader"))
                                                                                                       .getMsaLibHeader();
            guid = header.getGuid().getStringValue();
            componentType = header.getComponentType().toString();
        }
        
        if (componentType != null) {
            getProject().setProperty("COMPONENT_TYPE", componentType);
        }

        if (guid != null) {
            getProject().setProperty("FILE_GUID", guid);
        }
        //
        // Get all options and set to properties
        //
        String[][] options = SurfaceAreaQuery.getOptions(arch);
        for (int i = 0; i < options.length; i++) {
            if (options[i][0] != null && options[i][1] != null) {
                getProject().setProperty(options[i][0], getProject().replaceProperties(options[i][1]));
            }
        }

        buildType = getProject().getProperty("BUILD_TYPE");
        if (buildType == null) {
            buildType = componentType;
        }

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
        Pattern myPattern = Pattern.compile("[^\\\\]?(\".*?[^\\\\]\")[ \t,]+");
        Matcher matcher = myPattern.matcher(str + " ");
        while (matcher.find()) {
            String item = str.substring(matcher.start(1), matcher.end(1));
            if (!set.contains(item)) {
                set.add(item);
            }
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
            String str = getProject().replaceProperties((String) iter.next());
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
        String result = "";
        add.removeAll(sub);
        Iterator iter = add.iterator();
        while (iter.hasNext()) {
            String str = getProject().replaceProperties((String) iter.next());
            result += "\"" + str.substring(1, str.length() - 1) + "\", ";
        }
        return result;
    }

    /**
      Set base name. For ANT use.
      
      @param baseName Base name
    **/
    public void setBaseName(String baseName) {
        this.baseName = baseName;
    }

}
