/** @file
This file is to define nested element which is meant for specifying a tool

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.framework.tasks;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.apache.tools.ant.BuildException;

/**
 Class Tool is to define an external tool to be used for genffsfile
 **/
public class Tool implements EfiDefine, Section {

    String toolName     = "";
    List<Object>   toolArgList  = new ArrayList<Object>();
    String outputPath;
    String outPutFileName ;
    List<Input>    inputFiles = new ArrayList<Input>();

    /**
     Call extern tool

     @param     buffer  The buffer to put the result with alignment
     **/
    public void toBuffer (DataOutputStream buffer){
        File           OutputFile;
        byte           data;

        ///
        /// call extern tool
        ///
        try {
            executeTool ();
        } catch (Exception e) {
            throw new BuildException("Call tools failed!\n");
        }

        ///
        /// check if file exist
        ///
        OutputFile = new File (this.outPutFileName);
        long fileLen = OutputFile.length();
        if (!OutputFile.exists()) {
            throw new BuildException("The file " + outPutFileName + " is not exist!\n");
        }

        ///
        /// Read output file and write it's cotains to buffer
        ///
        try {
            FileInputStream fs  = new FileInputStream (this.outPutFileName);
            DataInputStream In  = new DataInputStream (fs);

            int i = 0;
            while (i < fileLen) {
                data = In.readByte();
                buffer.writeByte(data);
                i ++;
            }

            ///
            /// 4 byte alignment
            ///
            while ((fileLen & 0x03) != 0) {
                fileLen++;
                buffer.writeByte(0);
            }
            In.close();

        } catch (Exception e) {
            System.out.print(e.getMessage());
            throw new BuildException("Call tool2buffer failed!\n");
        }
    }

    ///
    /// execute external tool for genffsfile
    ///
    private void executeTool () {
        String command   = "";
        String argument  = "";
        command          = toolName;
        Iterator argIter = toolArgList.iterator();
        Iterator inputIter = inputFiles.iterator();
        ToolArg toolArg;
        Input file = null;

        ///
        /// argument of tools
        ///
        while (argIter.hasNext()) {
            toolArg = (ToolArg)argIter.next();
            argument = argument + toolArg.getLine() + " ";

        }

        ///
        /// input files for tools
        ///
        argument = argument + "-i ";
        while (inputIter.hasNext()) {
            file = (Input)inputIter.next();
            argument = argument + file.getFile() + " ";
        }

        outPutFileName = outputPath + File.separatorChar + (new File(file.getFile())).getName() + ".crc";
        argument       = argument + " -o " + outPutFileName; 

        try {

            ///
            /// execute command line
            ///
            Process crcProcess = Runtime.getRuntime().exec(command + " " + argument);
            crcProcess.waitFor();
        } catch (Exception e) {
            System.out.print (e.getMessage());
            throw new BuildException("Execute tools fails!\n");
        }
    }

    /**
     Add method of ANT task/datatype for nested ToolArg type of element

     @param     toolArg     The ToolArg object containing arguments for the tool
     **/
    public void addToolArg (ToolArg toolArg) {
        toolArgList.add (toolArg);
    }

    /**
     Get method of ANT task/datatype for attribute "OutputPath"

     @returns   The name of output path
     **/
    public String getOutputPath() {
        return outputPath;
    }

    /**
     Set method of ANT task/datatype for attribute "OutputPath"

     @param     outputPath  The name of output path
     **/
    public void setOutputPath(String outPutPath) {
        this.outputPath = outPutPath;
    }

    /**
     Get method of ANT task/datatype for attribute "ToolName"

     @returns   The name of the tool.
     **/
    public String getToolName() {
        return toolName;
    }

    /**
     Set method of ANT task/datatype for attribute "ToolName"

     @param     toolName    The name of the tool
     **/
    public void setToolName(String toolName) {
        this.toolName = toolName;
    }

    /**
     Add method of ANT task/datatype for nested Input type of element

     @param     file    The Input objec which represents a file
     **/
    public void addInput(Input file) {
        inputFiles.add(file);
    }
}


