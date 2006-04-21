/** @file
  
  This file is an ANT task OutputDirSetupTask. 
  
  This task main purpose is to setup some necessary properties for Package,
  Platform or Module clean. 
 
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
import java.util.Map;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;
import org.apache.xmlbeans.XmlObject;

import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.OutputManager;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.toolchain.ToolChainFactory;

/**
  <code>OutputDirSetupTask</code> is an ANT task that can be used in ANT build
  system. The main function of this task is to initialize some basic information
  for Package|Platform|Module clean or cleanall usage. 
  
  <p>Here is an example: </p> 
  <pre>
     &lt;OutputDirSetup baseName="HelloWorld" 
                     mbdFilename="${MODULE_DIR}\HelloWorld.mbd" 
                     msaFilename="${MODULE_DIR}\HelloWorld.msa" /&gt;
  </pre>
  
  <p>Note that all this task doing is part of GenBuildTask. </p>
  
  @since GenBuild 1.0
  @see org.tianocore.build.GenBuildTask
**/
public class OutputDirSetupTask extends Task {
    
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
    
    /**
      Public construct method. It is necessary for ANT task.
    **/
    public OutputDirSetupTask () {
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
      </ul>
      
      @throws BuildException
              From module build, exception from module surface area invalid.
    **/
    public void execute() throws BuildException {
        System.out.println("Deleting module [" + baseName + "] start.");
        OutputManager.update(getProject());
        GlobalData.initInfo("Tools" + File.separatorChar + "Conf" + File.separatorChar + "FrameworkDatabase.db", getProject()
                        .getProperty("WORKSPACE_DIR"));
        recallFixedProperties();
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
    }
    
    /**
      Get current module's base name. 
      
      @return base name
    **/
    public String getBaseName() {
        return baseName;
    }

    /**
      Set base name. For ANT use.
      
      @param baseName Base name
    **/
    public void setBaseName(String baseName) {
        this.baseName = baseName;
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
      Restore some important ANT property. If current build is single module 
      build, here will set many default values.
      
      <p> If current build is single module build, then the default <code>ARCH</code>
      is <code>IA32</code>. Also set up the properties <code>PACKAGE</code>, 
      <code>PACKAGE_DIR</code>, <code>TARGET</code> and <code>MODULE_DIR</code></p>
      
      <p> Note that for package build, package name is stored in <code>PLATFORM</code>
      and package directory is stored in <code>PLATFORM_DIR</code>. </p> 
     
      @see org.tianocore.build.global.OutputManager
    **/
    private void recallFixedProperties(){
        //
        // If build is for module build
        //
        if (getProject().getProperty("PACKAGE_DIR") == null) {
            ToolChainFactory toolChainFactory = new ToolChainFactory(getProject());
            toolChainFactory.setupToolChain();
            //
            // PACKAGE PACKAGE_DIR ARCH (Default) COMMON_FILE BUILD_MACRO
            //
            if (getProject().getProperty("ARCH") == null){
                getProject().setProperty("ARCH", "IA32");
            }
            String packageName = GlobalData.getPackageNameForModule(baseName);
            getProject().setProperty("PACKAGE", packageName);
            String packageDir = GlobalData.getPackagePath(packageName);
            getProject().setProperty("PACKAGE_DIR", getProject().getProperty("WORKSPACE_DIR") + File.separatorChar + packageDir);
            getProject().setProperty("TARGET", toolChainFactory.getCurrentTarget());
            getProject().setProperty("MODULE_DIR", getProject().replaceProperties(getProject().getProperty("MODULE_DIR")));
        }
        if (OutputManager.PLATFORM != null) {
            getProject().setProperty("PLATFORM", OutputManager.PLATFORM);
        }
        if (OutputManager.PLATFORM_DIR != null) {
            getProject().setProperty("PLATFORM_DIR", OutputManager.PLATFORM_DIR);
        }
    }
}
