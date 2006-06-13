/** @file
 GenSectionTask class.

 GenSectionTask is to call GenSection.exe to generate Section.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
**/

package org.tianocore.framework.tasks;

import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;

public class GenSectionTask extends Task implements EfiDefine {
    ///
    /// inputfile name
    ///
    private String inputFile = "";
    ///
    /// outputfile name
    ///
    private String outputFile = "";
    ///
    /// section type
    ///
    private String sectionType = "";
    ///
    /// version number
    ///
    private String versionNum = "";
    ///
    /// interface string
    ///
    private String interfaceString = "";

    /**
      execute
      
      GenSectionTaks execute is to assemble tool command line & execute tool
      command line.
      
      @throws BuildException
    **/
    public void execute() throws BuildException {
        String command;
        Project project = this.getOwningTarget().getProject();
        //
        // absolute path of efi tools
        //
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH");
        if (path == null) {
            command = "GenSection";
        } else {
            command = path + "/" + "GenSection";
        }
        //
        // argument of tools
        //
        String argument = inputFile + outputFile + sectionType + versionNum
                + interfaceString;
        //
        // return value of gensection execution
        //
        int revl = -1;

        try {
            Commandline cmdline = new Commandline();
            cmdline.setExecutable(command);
            cmdline.createArgument().setLine(argument);

            LogStreamHandler streamHandler = new LogStreamHandler(this,
                    Project.MSG_INFO, Project.MSG_WARN);
            Execute runner = new Execute(streamHandler, null);

            runner.setAntRun(project);
            runner.setCommandline(cmdline.getCommandline());
            System.out.println(Commandline.toString(cmdline.getCommandline()));

            revl = runner.execute();
            if (EFI_SUCCESS == revl) {
                //
                // command execution success
                //
                System.out.println("gensection succeeded!");
            } else {
                //
                // command execution fail
                //
                System.out.println("gensection failed. (error="
                        + Integer.toHexString(revl) + ")");
                throw new BuildException("gensection failed. (error="
                        + Integer.toHexString(revl) + ")");
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }
    }

    /**
      getInputFile
      
      This function is to get class member "inputFile".
      
      @return                    name of input file
    **/
    public String getInputFile() {
        return this.inputFile;
    }

    /**
      setInputFile
      
      This function is to set class member "inputFile".
      
      @param inputFile            name of input file
    **/
    public void setInputFile(String inputFile) {
        this.inputFile = " -i " + inputFile;
    }

    /**
      getOutputFile
      
      This function is to get class member "outputFile".
      
      @return                      name of output file
    **/
    public String getOutputFile() {
        return this.outputFile;
    }

    /**
      setOutputfile
      
      This function is to set class member "outputFile".
      @param  outputFile            name of output file
    **/
    public void setOutputfile(String outputFile) {
        this.outputFile = " -o " + outputFile;
    }

    /**
      getSectionType
      
      This function is to get class member "sectionType".
      
      @return                        sectoin type
    **/
    public String getSectionType() {
        return this.sectionType;
    }

    /**
      setSectionType
      
      This function is to set class member "sectionType".
      
      @param sectionType              section type
    **/
    public void setSectionType(String sectionType) {
        this.sectionType = " -s " + sectionType;
    }

    /**
      getVersionNum
      
      This function is to get class member "versionNum".
      @return                         version number
    **/
    public String getVersionNum() {
        return this.versionNum;
    }

    /**
      setVersionNume
      
      This function is to set class member "versionNum".
      @param versionNum               version number
    **/
    public void setVersionNum(String versionNum) {
        this.versionNum = " -v " + versionNum;
    }

    /**
      getInterfaceString
      
      This function is to get class member "interfaceString".
      @return                         interface string
    **/
    public String getInterfaceString() {
        return this.interfaceString;
    }

    /**
      setInterfaceString
      
      This funcion is to set class member "interfaceString".
      @param interfaceString            interface string
    **/
    public void setInterfaceString(String interfaceString) {
        this.interfaceString = " -a " + "\"" + interfaceString + "\"";
    }
}
