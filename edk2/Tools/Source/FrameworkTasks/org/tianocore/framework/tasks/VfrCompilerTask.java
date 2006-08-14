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
import java.util.ArrayList;
import java.util.List;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;

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
    private String createListFile = "";
    private String outPutDir = "";
    private String createIfrBinFile = "";
    private String processerArg ="";
    private String vfrFile = "";
    private String vfrFileName = "";

    private List<Object> includepathList = new ArrayList<Object>();

    /**
     get class member of createList file

     @returns file name of createList
     **/
    public String getCreateListFile() {
        return createListFile;
    }

    /**
     set class member of createList file

     @param     createListFile  if createList string equal "on" set '-l' flag
     **/
    public void setCreateListFile(String createListFile) {
        if (createListFile.equals("ON")||createListFile.equals("on"))
            this.createListFile = " -l";
    }

    /**
     get output dir 

     @returns name of output dir
     **/
    public String getOutPutDir() {
        return outPutDir;
    }

    /**
     set class member of outPutDir

     @param     outPutDir   The directory name for ouput file
     **/
    public void setOutPutDir(String outPutDir) {
        this.outPutDir = " -od " + outPutDir;
    }


    /**
     get class member of ifrBinFile

     @return file name of ifrBinFile
     **/
    public String getCreateIfrBinFile() {
        return createIfrBinFile;
    }

    /**
     set class member of ifrBinFile 

     @param     createIfrBinFile    The flag to specify if the IFR binary file should
                                    be generated or not
     */
    public void setCreateIfrBinFile(String createIfrBinFile) {
        if (createIfrBinFile.equals("ON") || createIfrBinFile.equals("on"));
        this.createIfrBinFile = " -ibin";
    }

    /**
     get class member of vfrFile

     @returns name of vfrFile
     **/
    public String getVfrFile() {
        return vfrFile;
    }

    /**
     set class member of vfrFile 

     @param     vfrFile The name of VFR file
     **/
    public void setVfrFile(String vfrFile) {
        this.vfrFileName = (new File(vfrFile)).getName();
        this.vfrFile = " " + vfrFile;
    }

    /**
     add includePath in includepath List 

     @param     includepath The IncludePath object which represents include path
     **/
    public void addIncludepath(IncludePath includepath){
        includepathList.add(includepath);
    }


    /**
     get class member of processerArg

     @returns processer argument
     **/
    public String getProcesserArg() {
        return processerArg;
    }


    /**
     set class member of processerArg

     @param     processerArg    The processor argument
     */
    public void setProcesserArg(String processerArg) {
        this.processerArg = " -ppflag " + processerArg;
    }

    /**
     The standard execute method of ANT task.
     **/
    public void execute() throws BuildException {
        Project project = this.getProject();
        String  toolPath= project.getProperty("env.FRAMEWORK_TOOLS_PATH");
        String  command;
        if (toolPath == null) {
            command = "VfrCompile";
        } else {
            command = toolPath + "/" + "VfrCompile";
        }
        List<Object> includePath = new ArrayList<Object>();     
        String incPath = "";        

        int  count = includepathList.size();    
        IncludePath path;
        for (int i = 0; i < count; i++) {
            path = (IncludePath) includepathList.get(i);
            if (path.getFile() != null) {
                FileParser.loadFile( project,includePath,path.getFile(), "-I");             
            }
        }
        for (int i = 0; i < count; i++) {
            incPath = incPath + " " + includepathList.get(i);
        }
        count =  includePath.size();
        for (int i = 0; i < count; i++) {
            incPath = incPath + " " + includePath.get(i);
        }
        String argument = this.createIfrBinFile +
                          this.processerArg + 
                          incPath +
                          this.outPutDir + 
                          this.createListFile +
                          this.vfrFile ;
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

            log(Commandline.toString(commandLine.getCommandline()), Project.MSG_VERBOSE);
            log(vfrFileName);
            int returnVal = runner.execute();
            if (EFI_SUCCESS == returnVal) {
                log("VfrCompile succeeded!", Project.MSG_VERBOSE);
            } else {
                log("ERROR = " + Integer.toHexString(returnVal));
                throw new BuildException("VfrCompile failed!");
            }
        } catch (IOException e) {
            throw new BuildException(e.getMessage());
        }
    }
}
