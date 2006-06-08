/** @file
 GenFvImageTask class.

 GenFvImageTask is to call GenFvImage.exe to generate FvImage.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.framework.tasks;
import java.io.File;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;

/**
  GenFvImageTask
  
  GenFvImageTask is to call GenFvImage.exe to generate the FvImage.
  
**/
public class GenFvImageTask extends Task implements EfiDefine{
    ///
    /// The name of input inf file
    ///
    private String infFile="";
    ///
    /// The target architecture.
    ///
    private String arch="";
    
    /**
      execute
      
      GenFvImageTask execute is to assemble tool command line & execute tool
      command line.
    **/
    public void execute() throws BuildException  {
        Project project = this.getOwningTarget().getProject();
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH");
        String command = "";
        
        if (path == null){
            path = "";
        }else {
            path = path + File.separatorChar;
        }
        
        if (arch.equalsIgnoreCase("")){
            command = path + "GenFvImage";
        }
        if (arch.equalsIgnoreCase("ia32")){
            command = path + "GenFvImage_IA32";
        }   
        if (arch.equalsIgnoreCase("x64")){
            command = path + "GenFvImage_X64";
        }
        if (arch.equalsIgnoreCase("ipf")){
            command = path + "GenFvImage_IPF";
        }
        String argument = infFile;
        
        try {
            
            Commandline commandLine = new Commandline();
            commandLine.setExecutable(command);
            commandLine.createArgument().setLine(argument);
            
            LogStreamHandler streamHandler = new LogStreamHandler(this,
                                                   Project.MSG_INFO,
                                                   Project.MSG_WARN);
            //
            // create a execute object and set it's commandline
            //
            Execute runner = new Execute(streamHandler,null);
            runner.setAntRun(project);
            runner.setCommandline(commandLine.getCommandline());            
            System.out.println(Commandline.toString(commandLine.getCommandline()));
            
            int revl = -1;
            //
            //  user execute class call external programs - GenFvImage
            //
            revl = runner.execute();
            // 
            // execute command line success!
            //
            if (EFI_SUCCESS == revl){
                System.out.println("GenFvImage succeeded!");
            } else {
                
            // 
            // execute command line failed! 
            //
                throw new BuildException("GenFvImage failed !(error =" + 
                    Integer.toHexString(revl) + ")");
            }
            
        } catch (Exception e) {
            System.out.println(e.getMessage());
        }       
    }
    /**
      getInfFile
      
      This function is to get class member of infFile
      @return String    name of infFile
    **/
    public String getInfFile() {
        return infFile;
    }
    
    /**
      setInfFile
      
      This function is to set class member of infFile.
      
      @param infFile  name of infFile
    **/
    public void setInfFile(String infFile) {
        this.infFile = "-I " + infFile;
    }
    
    /**
      getArch
      
      This function is to get class member of arch.
      @return           The target architecture.
    **/
    public String getArch() {
        return arch;
    }
    
    /**
      setArch
      
      This function is to set class member of arch. 
      
      @param arch       The target architecture.
    **/
    public void setArch(String arch) {
        this.arch = arch;
    }   
}