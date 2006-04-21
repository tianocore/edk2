/** @file
  File is FileProcess class which is used to generate ANT script to build 
  source files. 
  
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
import java.util.Set;

import org.apache.tools.ant.Project;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;

/**
  <p><code>FileProcess</code> is class to generate ANT script to build source
  files.</p>
  
  <p>If file does not specify file type, <code>FileProcess</code> will judge 
  by its extension. Following is the current supported extensions. </p>
  
  <pre>   
          .c         |      C_Code
          .asm       |      Assembly
          .s         |      IPF_Assembly_Code
          .h         |      Header
          .lib       |      Static_Library
          .i         |      IPF_PP_Code
          .vfr       |      Vfr
          .uni       |      Unicode
          .dxs       |      Dependency_File
          .bmp       |      Graphics
          .efi       |      EFI
  </pre>
  
  @since GenBuild 1.0
**/
public class FileProcess {
    ///
    ///  The mapping information about source suffix, result suffix, file type.
    ///
    public final String[][] fileTypes = { { ".c", ".obj", "C_Code" }, { ".asm", ".obj", "Assembly" },
                                         { ".s", ".obj", "IPF_Assembly_Code" }, { ".h", "", "Header" },
                                         { ".lib", "", "Static_Library" }, { ".src", ".c", "" },
                                         { ".i", ".obj", "IPF_PP_Code" }, { ".vfr", ".obj", "Vfr" },
                                         { ".uni", "", "Unicode" }, { ".dxs", "", "Dependency_File" },
                                         { ".bmp", "", "Graphics" }, { ".efi", "", "EFI" } };

    ///
    /// Current ANT context. 
    ///
    private Project project;

    ///
    /// Current module's include pathes
    ///
    private Set<String> includes;

    ///
    /// Current source files. 
    ///
    private Set<String> sourceFiles;
    
    ///
    /// Xml Document.
    ///
    private Document document;
    
    ///
    /// The flag to ensure all unicode files build before others. 
    ///
    private boolean unicodeFirst = true;
    
    ///
    /// The flag present whether current module contains Unicode files or not.
    ///
    private boolean unicodeExist = false;

    /**
      Initialize the project, includes, sourceFiles, document members.
      
      @param project ANT project
      @param includes Module include pathes
      @param sourceFiles Modules source files
      @param document XML document
    **/
    public void init(Project project, Set<String> includes, Set<String> sourceFiles, Document document) {
        this.document = document;
        this.includes = includes;
        this.project = project;
        this.sourceFiles = sourceFiles;
    }

    /**
      Parse file without file type. 
      
      @param filename Source file name
      @param root Root node
      @param unicodeFirst whether build Unicode file firstly or not
    **/
    public synchronized void parseFile(String filename, Node root, boolean unicodeFirst) {
        this.unicodeFirst = unicodeFirst;
        parseFile(filename, root);
    }
    
    /**
      Get whether current module contains Unicode files or not.
      
      @return Whether current module contains Unicode files or not
    **/
    public boolean isUnicodeExist() {
        return unicodeExist;
    }

    /**
      Parse file.
      
      @param filename Source file name
      @param filetype Source file type
      @param root Root node
      @param unicodeFirst whether build Unicode file firstly or not
    **/
    public synchronized void parseFile(String filename, String filetype, Node root, boolean unicodeFirst) {
        this.unicodeFirst = unicodeFirst;
        parseFile(filename, filetype, root);
    }
    
    /**
      Find out source file's type. 
      
      @param filename Source file name
      @param root Root node
    **/
    public synchronized void parseFile(String filename, Node root) {
        boolean flag = false;
        for (int i = 0; i < fileTypes.length; i++) {
            if (filename.toLowerCase().endsWith(fileTypes[i][0])) {
                flag = true;
                parseFile(filename, fileTypes[i][2], root);
            }
        }
        if (!flag) {
            System.out.println("Warning: File " + filename + " is not known from its suffix.");
        }
    }

    /**
      Parse file. If flag <code>unicodeFirst</code> is true, then build all
      unicode files firstly. 
      
      <p>Note that AutoGen.c is processed specially. It's output path is always
      <code>${DEST_DIR_OUTPUT}</code>, others are <code>${DEST_DIR_OUTPUT}</code>
      and relative to module path. </p>
      
      @param filename Source file name
      @param filetype Source file type
      @param root Root node
    **/
    public synchronized void parseFile(String filename, String filetype, Node root) {
        if (unicodeFirst) {
            if ( ! filetype.equalsIgnoreCase("Unicode")){
                return ;
            }
            unicodeExist= true;
        } else {
            if (filetype.equalsIgnoreCase("Unicode")){
                return ;
            }
        }
        sourceFiles.add(filename);
        if (filetype.equalsIgnoreCase("Header")) {
            return;
        }
        if (filetype.equalsIgnoreCase("IPF_PP_Code")) {
            return;
        }
        String module_path = project.getProperty("MODULE_DIR");
        File moduleFile = new File(module_path);
        File sourceFile = new File(filename);
        // If source file is AutoGen.c, then Filepath is .
        String sourceFilepath;
        String sourceFilename;
        if (sourceFile.getPath().endsWith("AutoGen.c")) {
            sourceFilepath = ".";
            sourceFilename = "AutoGen";
            filetype = "AUTOGEN";
        } else {
            // sourceFile.
            String str = sourceFile.getPath().substring(moduleFile.getPath().length() + 1);
            int index = str.lastIndexOf(File.separatorChar);
            sourceFilepath = ".";
            if (index > 0) {
                sourceFilepath = str.substring(0, index);
                str = str.substring(index + 1);
            }
            sourceFilename = str;
            index = str.lastIndexOf('.');
            if (index > 0) {
                sourceFilename = str.substring(0, index);
            }
        }
        // <Build_filetype FILEPATH="" FILENAME="" />
        Element ele = document.createElement("Build_" + filetype);
        ele.setAttribute("FILEPATH", sourceFilepath);
        ele.setAttribute("FILENAME", sourceFilename);
        String[] includePaths = includes.toArray(new String[includes.size()]);
        Element includesEle = document.createElement("EXTRA.INC");
        for (int i = 0; i < includePaths.length; i++) {
            Element includeEle = document.createElement("includepath");
            includeEle.setAttribute("path", includePaths[i]);
            includesEle.appendChild(includeEle);
        }
        ele.appendChild(includesEle);
        root.appendChild(ele);
    }
}