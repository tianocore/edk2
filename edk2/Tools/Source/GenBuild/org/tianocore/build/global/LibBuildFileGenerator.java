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
package org.tianocore.build.global;

import java.io.File;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.Vector;

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
import org.apache.xmlbeans.XmlObject;
import org.tianocore.MsaHeaderDocument.MsaHeader;
import org.tianocore.MsaLibHeaderDocument.MsaLibHeader;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

/**
  This class <code>LibBuildFileGenerator</code> is an ANT task to generate 
  build.xml for each module. Here are two usages. 
  
  <ul>
    <li>
      For one module (<b>bf</b> is LibBuildFileGenerator task name):
      <pre>
        &lt;bf buildFile="Application\HelloWorld\HelloWorld.msa" /&gt;
      </pre>
    </li>
    <li>
      For one package:
      <pre>
        &lt;bf recursive="true" /&gt;
      </pre>
    </li>
  </ul>
  
  @since GenBuild 1.0
**/
public class LibBuildFileGenerator extends Task {

    private File buildFile;

    private boolean recursive = false;
    
    private String license = " Copyright (c) 2006, Intel Corporation \n"
                    + "All rights reserved. This program and the accompanying materials \n"
                    + "are licensed and made available under the terms and conditions of the BSD License \n"
                    + "which accompanies this distribution.  The full text of the license may be found at  \n"
                    + "http://opensource.org/licenses/bsd-license.php \n"
                    + "\n"
                    + "THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN \"AS IS\" BASIS, \n"
                    + "WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.";

    private String base_name;
    
    private String module_relative_path;
    
    private File base_file = new File(".");
    
    /**
      Public construct method. It is necessary for ANT task.
    **/
    public LibBuildFileGenerator () {
    }

    /**
      ANT task's entry point, will be called after init(). 
      
      @throws BuildException
              buildFile do not specify while recursive set to false
    **/
    public void execute() throws BuildException {
        if(recursive){
            searchMsa(new File("."));
        }
        else {
            Map<String,File> map = new HashMap<String,File>();
            String basename = buildFile.getName();
            int k = basename.lastIndexOf('.');
            base_name = basename.substring(0, k);
            map.put(base_name, buildFile);
            genBuildFile(map);
        }
    }
    
    /**
      Recursivly find all MSA files and record all modules. 
      
      @param path Package path
    **/
    private void searchMsa(File path){
        File[] files = path.listFiles();
        Vector<File> vec = new Vector<File>();
        for(int i=0; i < files.length; i ++){
            if (files[i].isFile()){
                if(files[i].getName().endsWith(".msa")){
                    System.out.println("#" + files[i].getPath());
                    vec.add(files[i]);
                }
            }
        }
        Map<String,File> mapBasename = new HashMap<String,File>();
        if (vec.size() > 0){
            base_name = null;
            for ( int j = 0 ; j < vec.size(); j++){
                if ( vec.size() > 1){
                  System.out.println("##" + vec.get(0));
                }
                File f = (File)vec.get(j);
                SurfaceAreaParser surfaceAreaParser = new SurfaceAreaParser();
                Map<String, XmlObject> map = surfaceAreaParser.parseFile(f);
                String baseName = "";
                XmlObject header = null;
                if ( (header = map.get("MsaHeader")) != null ){
                    baseName = ((MsaHeader)header).getBaseName().getStringValue();
                }
                else if ( (header = map.get("MsaLibHeader")) != null){
                    baseName = ((MsaLibHeader)header).getBaseName().getStringValue();
                } else {
                    continue ;
                }
                if ( base_name == null || base_name.length() > baseName.length()){
                    base_name = baseName;
                    buildFile = f;
                    try {
                    module_relative_path = buildFile.getParent().substring(base_file.getPath().length() + 1);
                    }
                    catch(Exception e){
                        module_relative_path = ".";
                    }
                }
                mapBasename.put(baseName, f);
            }
            genBuildFile(mapBasename);
        }

        for(int i=0; i < files.length; i ++){
            if (files[i].isDirectory()){
                searchMsa(files[i]);
            }
        }
    }
    
    /**
      Generate build.xml.
      
      @param map All base name under one module directory
    **/
    private void genBuildFile(Map map) {
        DocumentBuilderFactory domfac = DocumentBuilderFactory.newInstance();
        try {
            DocumentBuilder dombuilder = domfac.newDocumentBuilder();
            Document document = dombuilder.newDocument();
            //
            // create root element and its attributes
            //
            document.appendChild(document.createComment(license));
            Element root = document.createElement("project");
            root.setAttribute("default", base_name);
            root.setAttribute("basedir", ".");
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
            
            ele = document.createElement("import");
            ele.setAttribute("file", "${WORKSPACE_DIR}"+File.separatorChar+"Tools"+File.separatorChar+"Conf"+File.separatorChar+"BuildMacro.xml");
            root.appendChild(ele);
            
            root.appendChild(document.createComment("MODULE_RELATIVE PATH is relative to PACKAGE_DIR"));
            ele = document.createElement("property");
            ele.setAttribute("name", "MODULE_RELATIVE_PATH");
            ele.setAttribute("value", module_relative_path);
            root.appendChild(ele);

            ele = document.createElement("property");
            ele.setAttribute("name", "MODULE_DIR");
            ele.setAttribute("value", "${PACKAGE_DIR}" + File.separatorChar + "${MODULE_RELATIVE_PATH}");
            root.appendChild(ele);

            ele = document.createElement("property");
            ele.setAttribute("name", "COMMON_FILE");
            ele.setAttribute("value", "${WORKSPACE_DIR}" + File.separatorChar + "Tools"
                            + File.separatorChar + "Conf" + File.separatorChar + "Common.xml");
            root.appendChild(ele);
            
            //
            // generate the buildfmd target
            //
            Set set = map.keySet();
            Iterator iter = set.iterator();
            while (iter.hasNext()){
                String bName = (String)iter.next();
                File msaFile = (File)map.get(bName);
                String msaFilename = "${MODULE_DIR}" + File.separatorChar + msaFile.getName();
                String mbdFilename = msaFilename.substring(0 , msaFilename.length() - 4) + ".mbd";
                ele = document.createElement("target");
                ele.setAttribute("name", bName);
                Element target = document.createElement("GenBuild");
                target.setAttribute("msaFilename", msaFilename);
                target.setAttribute("mbdFilename", mbdFilename);
                target.setAttribute("baseName", bName);
                ele.appendChild(target);
                root.appendChild(ele);
            }

            root.appendChild(ele);
            //
            // Default clean
            //
            ele = document.createElement("target");
            ele.setAttribute("name", "clean");
            ele.setAttribute("depends", base_name + "_clean");
            root.appendChild(ele);
            //
            // Default Clean ALl
            //
            ele = document.createElement("target");
            ele.setAttribute("name", "cleanall");
            ele.setAttribute("depends", base_name + "_cleanall");
            root.appendChild(ele);
            //
            // Every clean target for each BaseName
            //
            set = map.keySet();
            iter = set.iterator();
            while (iter.hasNext()){
                String bName = (String)iter.next();
                File msaFile = (File)map.get(bName);
                String msaFilename = "${MODULE_DIR}" + File.separatorChar + msaFile.getName();
                String mbdFilename = msaFilename.substring(0 , msaFilename.length() - 4) + ".mbd";
                
                ele = document.createElement("target");
                ele.setAttribute("name", bName + "_clean");
                //
                // Output Dir
                //
                Element target = document.createElement("OutputDirSetup");
                target.setAttribute("msaFilename", msaFilename);
                target.setAttribute("mbdFilename", mbdFilename);
                target.setAttribute("baseName", bName);
                ele.appendChild(target);
                //
                // Call BaseName_build.xml clean
                //
                Element ifEle = document.createElement("if");
                Element availableEle = document.createElement("available");
                availableEle.setAttribute("file", "${DEST_DIR_OUTPUT}" + File.separatorChar + bName + "_build.xml");
                ifEle.appendChild(availableEle);
                Element elseEle = document.createElement("then");
                
                Element moduleEle = document.createElement("ant");
                moduleEle.setAttribute("antfile", "${DEST_DIR_OUTPUT}" + File.separatorChar + bName + "_build.xml");
                moduleEle.setAttribute("target", "clean");
                
                elseEle.appendChild(moduleEle);
                ifEle.appendChild(elseEle);
                ele.appendChild(ifEle);
                //
                // just delete
                //
                Element clean = document.createElement("delete");
                clean.setAttribute("dir", "${DEST_DIR_OUTPUT}");
                clean.setAttribute("excludes", "*.xml");
                ele.appendChild(clean);
                
                root.appendChild(ele);
            }
            //
            // Every Clean ALl target for each BaseName
            //
            set = map.keySet();
            iter = set.iterator();
            while (iter.hasNext()){
                String bName = (String)iter.next();
                File msaFile = (File)map.get(bName);
                String msaFilename = "${MODULE_DIR}" + File.separatorChar + msaFile.getName();
                String mbdFilename = msaFilename.substring(0 , msaFilename.length() - 4) + ".mbd";
                
                ele = document.createElement("target");
                ele.setAttribute("name", bName + "_cleanall");
                //
                // Output Dir
                //
                Element target = document.createElement("OutputDirSetup");
                target.setAttribute("msaFilename", msaFilename);
                target.setAttribute("mbdFilename", mbdFilename);
                target.setAttribute("baseName", bName);
                ele.appendChild(target);
                //
                // Call BaseName_build.xml clean
                //
                Element ifEle = document.createElement("if");
                Element availableEle = document.createElement("available");
                availableEle.setAttribute("file", "${DEST_DIR_OUTPUT}" + File.separatorChar + bName + "_build.xml");
                ifEle.appendChild(availableEle);
                Element elseEle = document.createElement("then");
                
                Element moduleEle = document.createElement("ant");
                moduleEle.setAttribute("antfile", "${DEST_DIR_OUTPUT}" + File.separatorChar + bName + "_build.xml");
                moduleEle.setAttribute("target", "cleanall");
                
                elseEle.appendChild(moduleEle);
                ifEle.appendChild(elseEle);
                ele.appendChild(ifEle);
                //
                // just delete
                //
                Element clean = document.createElement("delete");
                clean.setAttribute("dir", "${DEST_DIR_OUTPUT}");
                ele.appendChild(clean);
                
                clean = document.createElement("delete");
                clean.setAttribute("dir", "${DEST_DIR_DEBUG}");
                ele.appendChild(clean);
                
                clean = document.createElement("delete");
                Element fileset = document.createElement("fileset");
                fileset.setAttribute("dir", "${BIN_DIR}");
                fileset.setAttribute("includes", "**" + bName + "*");
                clean.appendChild(fileset);
                ele.appendChild(clean);
                
                root.appendChild(ele);
            }
            document.appendChild(root);
            //
            // Prepare the DOM document for writing
            //
            Source source = new DOMSource(document);
            //
            // Prepare the output file
            //
            String filename = buildFile.getParent() + File.separatorChar + "build.xml";
            File file = new File(getProject().replaceProperties(filename));
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
    
    
    public File getBuildFile() {
        return buildFile;
    }

    public void setBuildFile(File buildFile) {
        this.buildFile = buildFile;
    }

    public boolean isRecursive() {
        return recursive;
    }

    public void setRecursive(boolean recursive) {
        this.recursive = recursive;
    }
}
