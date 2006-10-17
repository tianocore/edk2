/** @file
This file is to define an ANT task which wraps VfrCompile.exe tool

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
import java.io.IOException;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;

import org.tianocore.common.logger.EdkLog;

/**
 VfrcompilerTask Task Class
  class member 
      -createListFile  : create an output IFR listing file.
      -outPutDir       : deposit all output files to directory OutputDir (default=cwd)
      -createIfrBinFile: create an IFR HII pack file
      -vfrFile         : name of the input VFR script file
      -processArg      : c processer argument
      -includepathList : add IncPath to the search path for VFR included files
 **/
public class VfrCompilerTask extends Task implements EfiDefine {
    private static String toolName = "VfrCompile";

    private ToolArg createListFile = new ToolArg();
    private ToolArg createIfrBinFile = new ToolArg();
    private ToolArg processerArg = new ToolArg();
    private FileArg vfrFile = new FileArg();
    private IncludePath includepathList = new IncludePath();
    private FileArg outPutDir = new FileArg(" -od ", ".");
    private String dllPath = "";

    /**
     get class member of createList file

     @returns file name of createList
     **/
    public boolean getCreateListFile() {
        return this.createListFile.getValue().length() > 0;
    }

    /**
     set class member of createList file

     @param     createListFile  if createList string equal "on" set '-l' flag
     **/
    public void setCreateListFile(boolean createListFile) {
        if (createListFile) {
            this.createListFile.setArg(" -", "l");
        }
    }

    /**
     get output dir 

     @returns name of output dir
     **/
    public String getOutPutDir() {
        return this.outPutDir.getValue();
    }

    /**
     set class member of outPutDir

     @param     outPutDir   The directory name for ouput file
     **/
    public void setOutPutDir(String outPutDir) {
        this.outPutDir.setArg(" -od ", outPutDir);
    }


    /**
     get class member of ifrBinFile

     @return file name of ifrBinFile
     **/
    public boolean getCreateIfrBinFile() {
        return this.createIfrBinFile.getValue().length() > 0;
    }

    /**
     set class member of ifrBinFile 

     @param     createIfrBinFile    The flag to specify if the IFR binary file should
                                    be generated or not
     */
    public void setCreateIfrBinFile(boolean createIfrBinFile) {
        if (createIfrBinFile) {
            this.createIfrBinFile.setArg(" -", "ibin");
        }
    }

    /**
     get class member of vfrFile

     @returns name of vfrFile
     **/
    public String getVfrFile() {
        return this.vfrFile.getValue();
    }

    /**
     set class member of vfrFile 

     @param     vfrFile The name of VFR file
     **/
    public void setVfrFile(String vfrFile) {
        this.vfrFile.setArg(" ", vfrFile);
    }

    /**
     add includePath in includepath List 

     @param     includepath The IncludePath object which represents include path
     **/
    public void addConfiguredIncludepath(IncludePath includepath){
        this.includepathList.insert(includepath);
    }

    /**
     get class member of processerArg

     @returns processer argument
     **/
    public String getProcesserArg() {
        return this.processerArg.getValue();
    }


    /**
     set class member of processerArg

     @param     processerArg    The processor argument
     */
    public void setProcesserArg(String processerArg) {
        this.processerArg.setArg(" -ppflag ", processerArg);
    }

    public void setDllPath(String dllPath) {
        this.dllPath = dllPath;
    }

    /**
     The standard execute method of ANT task.
     **/
    public void execute() throws BuildException {
        Project project = this.getProject();
        String  toolPath= project.getProperty("env.FRAMEWORK_TOOLS_PATH");
        String  command;
        if (toolPath == null) {
            command = toolName;
        } else {
            command = toolPath + File.separator + toolName;
        }

        String argument = "" + createIfrBinFile
                             + processerArg  
                             + includepathList
                             + outPutDir
                             + createListFile
                             + vfrFile;
        try {
            ///
            /// constructs the command-line
            ///
            Commandline commandLine = new Commandline();
            commandLine.setExecutable(command);
            commandLine.createArgument().setLine(argument);

            ///
            /// configures the Execute object
            ///
            LogStreamHandler streamHandler = new LogStreamHandler(this,
                                                                  Project.MSG_INFO,
                                                                  Project.MSG_WARN);

            Execute runner = new Execute(streamHandler,null);
            runner.setAntRun(project);            
            runner.setCommandline(commandLine.getCommandline());
            runner.setWorkingDirectory(new File(outPutDir.getValue())); 
            runner.setEnvironment(new String[]{"PATH", dllPath});
            
            EdkLog.log(this, EdkLog.EDK_VERBOSE, Commandline.toString(commandLine.getCommandline()));
            EdkLog.log(this, vfrFile.toFileList());

            int returnVal = runner.execute();
            if (EFI_SUCCESS == returnVal) {
                EdkLog.log(this, EdkLog.EDK_VERBOSE, "VfrCompile succeeded!");
            } else {
                EdkLog.log(this, "ERROR = " + Integer.toHexString(returnVal));
                throw new BuildException("VfrCompile failed!");
            }
        } catch (IOException e) {
            throw new BuildException(e.getMessage());
        }
    }
}
