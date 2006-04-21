/** @file
  ToolChainTask class.
  
  ToolChainTask class's main fucntion is read all tool chain related config files. 

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  

**/
package org.tianocore.build.toolchain;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Task;

/**
  This class is an ANT task. The main function is to read all tool chain related
  config files. 
  
  @since GenBuild 1.0
**/
public class ToolChainTask extends Task{

    private String confPath = ".";
    
    /**
      Public construct method. It is necessary for ANT task.
    **/
    public ToolChainTask(){
    }
    
    /**
      ANT task's entry point, will be called after init(). Using
      <code>ToolChainFactory</code> to parse all config files, and
      set TARGET property. 
    
      @throws BuildException
              Config files are invalid.
    **/
    public void execute() throws BuildException {
        String toolChain = getProject().getProperty("env.TOOL_CHAIN");
        ToolChainFactory toolchain = new ToolChainFactory(confPath, toolChain);
        toolchain.setupToolChain();
        getProject().setProperty("TARGET", toolchain.getCurrentTarget());
    }

    /**
      Set the path of config files.
      
      @param confPath the path of config files
    **/
    public void setConfPath(String confPath) {
        this.confPath = confPath;
    }
}
