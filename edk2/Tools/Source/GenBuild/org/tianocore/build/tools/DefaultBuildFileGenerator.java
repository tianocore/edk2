/** @file
  This file is an ANT task.
  
  LibBuildFileGenerator task is used to generate module's build.xml file.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.build.tools;

import java.io.File;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.Set;

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
import org.apache.tools.ant.Task;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.build.id.PackageIdentification;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

public class DefaultBuildFileGenerator extends Task {

    private Set<ModuleIdentification> modules = new LinkedHashSet<ModuleIdentification>();
    
    private Set<PackageIdentification> packages = new LinkedHashSet<PackageIdentification>();
    
    //
    //  <DefaultBuildFileGenerator mode="WORKSPACE | PACKAGE | MODULE">
    //    <PackageItem packageName="" packageGuid="" packageVersion="" />
    //    <ModuleItem  moduleName="HelloWorld" moduleGuid="" moduleVersion="" packageName="" packageGuid="" packageVersion="" />
    //  </DefaultBuildFileGenerator>
    //
    private String mode = "MODULE";
    
    private String license = " Copyright (c) 2006, Intel Corporation \n"
                    + "All rights reserved. This program and the accompanying materials \n"
                    + "are licensed and made available under the terms and conditions of the BSD License \n"
                    + "which accompanies this distribution.  The full text of the license may be found at  \n"
                    + "http://opensource.org/licenses/bsd-license.php \n"
                    + "\n"
                    + "THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN \"AS IS\" BASIS, \n"
                    + "WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.";
    
    /**
      Public construct method. It is necessary for ANT task.
    **/
    public DefaultBuildFileGenerator () {
    }

    public void execute() throws BuildException {
        //
        // Global Data initialization
        //
        GlobalData.initInfo("Tools" + File.separatorChar + "Conf" + File.separatorChar + "FrameworkDatabase.db",
                            getProject().getProperty("WORKSPACE_DIR"), "tools_def.txt");
        
        if (mode.equalsIgnoreCase("WORKSPACE")) {
            modules.clear();
            packages = GlobalData.getPackageList();
        } 
        else if (mode.equalsIgnoreCase("PACKAGE")) {
            modules.clear();
        }
        if (mode.equalsIgnoreCase("WORKSPACE") || mode.equalsIgnoreCase("PACKAGE")) {
            Iterator iter = packages.iterator();
            while (iter.hasNext()) {
                PackageIdentification packageId = (PackageIdentification)iter.next();
                modules.addAll(GlobalData.getModules(packageId));
            }
        }
        
        Iterator iter = modules.iterator();
        while (iter.hasNext()) {
            ModuleIdentification moduleId = (ModuleIdentification)iter.next();
            genBuildFile (moduleId);
        }
    }
    
    private void genBuildFile(ModuleIdentification moduleId) {
        DocumentBuilderFactory domfac = DocumentBuilderFactory.newInstance();
        try {
            DocumentBuilder dombuilder = domfac.newDocumentBuilder();
            Document document = dombuilder.newDocument();
            //
            // create root element and its attributes
            //
            document.appendChild(document.createComment(license));
            Element root = document.createElement("project");
            root.setAttribute("default", "all");
            root.setAttribute("basedir", ".");
            root.setAttribute("name", moduleId.getName());
            
            //
            // element for External ANT tasks
            //
            root.appendChild(document.createComment("Apply external ANT tasks"));
            Element ele = document.createElement("taskdef");
            ele.setAttribute("resource", "GenBuild.tasks");
            root.appendChild(ele);
            
            //
            // <taskdef resource="net/sf/antcontrib/antlib.xml" />
            //
            ele = document.createElement("taskdef");
            ele.setAttribute("resource", "net/sf/antcontrib/antlib.xml");
            root.appendChild(ele);
            
            ele = document.createElement("property");
            ele.setAttribute("environment", "env");
            root.appendChild(ele);

            ele = document.createElement("property");
            ele.setAttribute("name", "WORKSPACE_DIR");
            ele.setAttribute("value", "${env.WORKSPACE}");
            root.appendChild(ele);
            
            ele = document.createElement("property");
            ele.setAttribute("name", "MSA_FILENAME");
            ele.setAttribute("value", GlobalData.getMsaFile(moduleId).getName());
            root.appendChild(ele);
            
            ele = document.createElement("property");
            ele.setAttribute("name", "BASE_NAME");
            ele.setAttribute("value", moduleId.getName());
            root.appendChild(ele);
            
            //
            // Don't change it!!
            //
            ele = document.createElement("import");
            ele.setAttribute("file", "${WORKSPACE_DIR}/Tools/Conf/BuildMacro.xml");
            root.appendChild(ele);
            
            //
            // <target name="all">
            //   <GenBuild msaFile="HelloWorld.msa"/>
            // </target>
            //
            Element targetEle = document.createElement("target");
            targetEle.setAttribute("name", "all");
            
            ele = document.createElement("GenBuild");
            ele.setAttribute("msaFile", "${MSA_FILENAME}");
            targetEle.appendChild(ele);
            
            root.appendChild(targetEle);
            
            //
            //  <target name="clean">
            //    <OutputDirSetup msaFile="HelloWorld.msa"/>
            //    <if>
            //      <available file="${DEST_DIR_OUTPUT}/HelloWorld_build.xml"/>
            //      <then>
            //        <ant antfile="${DEST_DIR_OUTPUT}/HelloWorld_build.xml" target="clean"/>
            //      </then>
            //    </if>
            //    <delete dir="${DEST_DIR_OUTPUT}" excludes="*.xml"/>
            //  </target>
            //
            targetEle = document.createElement("target");
            targetEle.setAttribute("name", "clean");
            
            ele = document.createElement("OutputDirSetup");
            ele.setAttribute("msaFile", "${MSA_FILENAME}");
            targetEle.appendChild(ele);
            
            ele = document.createElement("if");
            
            Element availableEle = document.createElement("available");
            availableEle.setAttribute("file", "${DEST_DIR_OUTPUT}/${BASE_NAME}_build.xml");
            ele.appendChild(availableEle);
            
            Element thenEle = document.createElement("then");
            Element antEle = document.createElement("ant");
            antEle.setAttribute("antfile", "${DEST_DIR_OUTPUT}/${BASE_NAME}_build.xml");
            antEle.setAttribute("target", "clean");
            thenEle.appendChild(antEle);
            ele.appendChild(thenEle);
            targetEle.appendChild(ele);
            
            ele = document.createElement("delete");
            ele.setAttribute("dir", "${DEST_DIR_OUTPUT}");
            ele.setAttribute("excludes", "*.xml");
            targetEle.appendChild(ele);
            
            root.appendChild(targetEle);
            
            //
            //  <target name="cleanall">
            //    <OutputDirSetup msaFile="HelloWorld.msa"/>
            //    <if>
            //      <available file="${DEST_DIR_OUTPUT}/HelloWorld_build.xml"/>
            //      <then>
            //        <ant antfile="${DEST_DIR_OUTPUT}/HelloWorld_build.xml" target="cleanall"/>
            //      </then>
            //    </if>
            //    <delete dir="${DEST_DIR_OUTPUT}"/>
            //    <delete dir="${DEST_DIR_DEBUG}"/>
            //    <delete>
            //      <fileset dir="${BIN_DIR}" includes="**HelloWorld*"/>
            //    </delete>
            //  </target>
            //
            targetEle = document.createElement("target");
            targetEle.setAttribute("name", "cleanall");
            
            ele = document.createElement("OutputDirSetup");
            ele.setAttribute("msaFile", "${MSA_FILENAME}");
            targetEle.appendChild(ele);
            
            ele = document.createElement("if");
            
            availableEle = document.createElement("available");
            availableEle.setAttribute("file", "${DEST_DIR_OUTPUT}/${BASE_NAME}_build.xml");
            ele.appendChild(availableEle);
            
            thenEle = document.createElement("then");
            antEle = document.createElement("ant");
            antEle.setAttribute("antfile", "${DEST_DIR_OUTPUT}/${BASE_NAME}_build.xml");
            antEle.setAttribute("target", "cleanall");
            thenEle.appendChild(antEle);
            ele.appendChild(thenEle);
            targetEle.appendChild(ele);
            
            ele = document.createElement("delete");
            ele.setAttribute("dir", "${DEST_DIR_OUTPUT}");
            targetEle.appendChild(ele);
            
            ele = document.createElement("delete");
            ele.setAttribute("dir", "${DEST_DIR_DEBUG}");
            targetEle.appendChild(ele);
            
            ele = document.createElement("delete");
            
            Element filesetEle = document.createElement("fileset");
            filesetEle.setAttribute("dir", "${BIN_DIR}");
            filesetEle.setAttribute("includes", "**${BASE_NAME}*");
            ele.appendChild(filesetEle);
            
            targetEle.appendChild(ele);
            
            root.appendChild(targetEle);

            
            document.appendChild(root);
            
            //
            // Prepare the DOM document for writing
            //
            Source source = new DOMSource(document);
            
            //
            // Prepare the output file
            //
            String filename = GlobalData.getMsaFile(moduleId).getParent() + File.separatorChar + "build.xml";
            File file = new File(getProject().replaceProperties(filename));
            
            GlobalData.log.info("File generating - " + filename);
            //
            // generate all directory path
            //
            Result result = new StreamResult(file);
            
            //
            // Write the DOM document to the file
            //
            Transformer xformer = TransformerFactory.newInstance()
                            .newTransformer();
            xformer.setOutputProperty("{http://xml.apache.org/xslt}indent-amount", "2");
            xformer.setOutputProperty(OutputKeys.INDENT, "yes");
            xformer.transform(source, result);
        } catch (Exception ex) {
            System.out.println("##" + ex);
        }
        
    }
    
    public void addConfiguredModuleItem(ModuleItem moduleItem) {
        PackageIdentification packageId = new PackageIdentification(moduleItem.getPackageName(), moduleItem.getPackageGuid(), moduleItem.getPackageVersion());
        ModuleIdentification moduleId = new ModuleIdentification(moduleItem.getModuleName(), moduleItem.getModuleGuid(), moduleItem.getModuleVersion());
        moduleId.setPackage(packageId);
        modules.add(moduleId);
    }
    
    public void addConfiguredPackageItem(PackageItem packageItem) {
        PackageIdentification packageId = new PackageIdentification(packageItem.getPackageName(), packageItem.getPackageGuid(), packageItem.getPackageVersion());
        packages.add(packageId);
    }

    public void setMode(String mode) {
        this.mode = mode;
    }
}
