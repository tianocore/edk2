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

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.lang.ProcessBuilder;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

/**
  GenFvImageTask
  
  GenFvImageTask is to call GenFvImage.exe to generate the FvImage.
  
**/
public class GenFvImageTask extends Task implements EfiDefine{
    ///
    /// tool name
    ///
    static final private String toolName = "GenFvImage";
    ///
    /// The name of input inf file
    ///
    private String infFile="";
    ///
    /// Output directory
    ///
    private String outputDir = ".";
    ///
    /// argument list
    ///
    LinkedList<String> argList = new LinkedList<String>();

    /**
      execute
      
      GenFvImageTask execute is to assemble tool command line & execute tool
      command line.
    **/
    public void execute() throws BuildException  {
        Project project = this.getOwningTarget().getProject();
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH");

        if (path == null) {
            path = "";
        } else {
            path += File.separatorChar;
        }
        argList.addFirst(path + toolName);

        /// lauch the program
        ///
        ProcessBuilder pb = new ProcessBuilder(argList);
        pb.directory(new File(outputDir));
        int exitCode = 0;
        try {
            Process cmdProc = pb.start();
            InputStreamReader cmdOut = new InputStreamReader(cmdProc.getInputStream());
            char[] buf = new char[1024];

            exitCode = cmdProc.waitFor();
            if (exitCode != 0) {
                int len = cmdOut.read(buf, 0, 1024);
                log(new String(buf, 0, len), Project.MSG_ERR);
            } else {
                log("GenFvImage - DONE!", Project.MSG_VERBOSE);
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        } finally {
            if (exitCode != 0) {
                throw new BuildException("GenFvImage: failed to generate FV file!");
            }
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
        this.infFile = infFile;
        argList.add("-I");
        argList.add(infFile);
    }
    
    /**
      getOutputDir
      
      This function is to get output directory.
      
      @return                Path of output directory.
    **/
    public String getOutputDir() {
        return outputDir;
    }

    /**
      setOutputDir
      
      This function is to set output directory.
      
      @param outputDir        The output direcotry.
    **/
    public void setOutputDir(String outputDir) {
        this.outputDir = outputDir;
    }
}
