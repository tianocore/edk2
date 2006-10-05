/** @file
  OutputManager class.
  
  OutputManager class set output directories for every module by BUILD_MODE.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.build.global;

import org.apache.tools.ant.Project;
import java.io.File;

/**
  OutputManager class is used to setup output directories (BIN_DIR, DEST_DIR_OUTPUT, 
  DEST_DIR_DEBUG). 
  
  @since GenBuild 1.0
**/
public class OutputManager {

    ///
    /// means intermediate files will put under Module's dir
    ///
    private String MODULE = "MODULE";
    
    ///
    /// mean intermediate files will put under a unify dir
    ///
    private String UNIFIED = "UNIFIED";
    
    
    private String userdir;
    
    private String type;
    ///
    /// Singleton Design Pattern
    ///
    private static OutputManager object;
    
    public synchronized static OutputManager getInstance() {
        if ( object == null ) {
            object = new OutputManager();
        }
        return object;
    }
    
    public void setup(String userdir, String type) {
        this.userdir = userdir;
        this.type = type;
    }
    
    /**
      Setup BIN_DIR, DEST_DIR_OUTPUT and DEST_DIR_OUTPUT, following are the rules:
      
      <p>Divide all output files into two types: one is final files, such as FFS 
      file for driver module while LIB file for library module; another is 
      intermediate files, such AutoGen.c, OBJ files, Section files and so on. 
      
      <p>In FPD, OutputDirectory element is used to specify where to put the output 
      files to. There are two mode (MODULE | UNIFIED). MODULE mode means that all 
      output files will put to the module directory while UNIFIED mode means that 
      all output files will put together. Default is UNIFIED mode. 
      
      <p>BUILD_DIR is the base directory for current module build. By default, 
      BUILD_DIR is PLATFORM_DIR/Build in UNIFIED mode while is MODULE_DIR/Build 
      in MODULE mode. Of course, user can customize BUILD_DIR. If user-defined 
      BUILD_DIR is relative path, then look as related to WORKSPACE_DIR. 
      
      <p>Then, BIN_DIR is BUILD_DIR/TARGET/TOOLCHAIN/ARCH;
      
      <p>FV_DIR is BUILD_DIR/TARGET/TOOLCHAIN/FV;
      
      <p>DEST_DIR_DEBUG | DEST_DIR_OUTPUT is: 
      BIN_DIR/PACKAGE_RELATIVE_DIR/MODULE_RELATIVE_DIR/DEBUG | OUTPUT

      
      @param project current ANT build Project
      @param userdir user-defined directory
      @param type the module build type (MODULE or UNIFIED)
    **/
    public void update(Project project) {
        //
        // Default mode is UNIFIED. 
        //
        if (type != null && type.equalsIgnoreCase(MODULE)) {
            type = MODULE;
        }
        else {
            type = UNIFIED;
        }
        
        //
        // default BUILD_DIR value
        //
        String buildDir;
        if(type.equals(MODULE)){
            buildDir = project.getProperty("MODULE_DIR") + File.separatorChar + "Build";
        }
        else {
            buildDir = project.getProperty("PLATFORM_DIR") + File.separatorChar + "Build";
        }
        
        //
        // If user define BUILD_DIR
        //
        if (userdir != null && ! userdir.equals("")) {
            File buildFile = new File(userdir);
            if (buildFile.isAbsolute()){
                buildDir = userdir;
            }
            //
            // If path is not absolute, then look as related to WORKSPACE_DIR
            //
            else {
                buildDir = GlobalData.getWorkspacePath() + File.separatorChar + userdir;
            }
        }
        
        //
        // Define TARGET_DIR
        //
        String targetDir = buildDir + File.separatorChar + project.getProperty("TARGET")
                                    + "_" + project.getProperty("TOOLCHAIN");
        
        //
        // Define BIN_DIR and FV_DIR
        //
        String binDir = targetDir + File.separatorChar + project.getProperty("ARCH") ;
        
        String fvDir = targetDir + File.separatorChar + "FV";
        
        //
        // Define DEST_DIR_OUTPUT and DEST_DIR_DEBUG
        //
        String destDir = binDir + File.separatorChar + project.getProperty("PACKAGE_RELATIVE_DIR")
                                + File.separatorChar + project.getProperty("MODULE_RELATIVE_DIR");
        
        //
        // Set properties
        //
        project.setProperty("BUILD_DIR", buildDir.replaceAll("(\\\\)", "/"));
        project.setProperty("TARGET_DIR", targetDir.replaceAll("(\\\\)", "/"));
        project.setProperty("FV_DIR", fvDir.replaceAll("(\\\\)", "/"));
        project.setProperty("BIN_DIR", binDir.replaceAll("(\\\\)", "/"));
        project.setProperty("DEST_DIR_DEBUG", (destDir + File.separatorChar + "DEBUG").replaceAll("(\\\\)", "/"));
        project.setProperty("DEST_DIR_OUTPUT", (destDir + File.separatorChar + "OUTPUT").replaceAll("(\\\\)", "/"));
        
        //
        // Create all directory if necessary
        //
        (new File(buildDir)).mkdirs();
        (new File(fvDir)).mkdirs();
        (new File(binDir)).mkdirs();
        (new File(destDir + File.separatorChar + "DEBUG")).mkdirs();
        (new File(destDir + File.separatorChar + "OUTPUT")).mkdirs();
    }
    
    public boolean prepareBuildDir(Project project){
        boolean isUnified = true;
        
        if (type.equalsIgnoreCase("MODULE")) {
            isUnified = false;
        }
        
        String buildDir = project.getProperty("PLATFORM_DIR") + File.separatorChar + "Build";
        //
        // If user define BUILD_DIR
        //
        if (userdir != null && ! userdir.equals("")) {
            File buildFile = new File(userdir);
            if (buildFile.isAbsolute()){
                buildDir = userdir;
            }
            //
            // If path is not absolute, then look as related to WORKSPACE_DIR
            //
            else {
                buildDir = GlobalData.getWorkspacePath() + File.separatorChar + userdir;
            }
        }
        
        //
        // Set to property
        //
        project.setProperty("BUILD_DIR", buildDir.replaceAll("(\\\\)", "/"));
        
        //
        // Create all directory if necessary
        //
        (new File(buildDir)).mkdirs();
        return isUnified;
    }

}