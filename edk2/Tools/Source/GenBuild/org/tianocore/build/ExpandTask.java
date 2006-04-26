/** @file
  This file is ANT task Expand. 
  
  Expand task is used to prepare ANT properties for further build. 

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.build;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

import org.tianocore.build.global.GlobalData;

/**
  Expand task is used to prepare ANT properties for further build. 
  <p>Current, prepare the dependent Library instance list for <code>LIBS</code></p>
 
  @since GenBuild 1.0
**/
public class ExpandTask extends Task {
    
    /**
      Public construct method. It is necessary for ANT task.
    **/
    public ExpandTask () {
    }
    
    /**
      ANT task's entry point, will be called after init().
      
      Set <code>LIBS</code> for further build usage. 
    **/
    public void execute() throws BuildException {
        String basename = getProject().getProperty("BASE_NAME");
        String arch = getProject().getProperty("ARCH");
        arch = arch.toUpperCase();
        String[] libraries = GlobalData.getModuleLibrary(basename, arch);
        String str = "";
        for (int i = 0; i < libraries.length; i ++){
            str += " " + GlobalData.getLibrary(libraries[i], arch);
        }
        getProject().setProperty("LIBS", str);
       
    }
}
