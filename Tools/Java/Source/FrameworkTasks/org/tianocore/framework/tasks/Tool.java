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
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Random;

import org.apache.tools.ant.BuildException;
import org.tianocore.common.logger.EdkLog;

/**
 Class Tool is to define an external tool to be used for genffsfile
 **/
public class Tool implements EfiDefine, Section {

    private int alignment = 0;
    private String toolName     = "";
    private ToolArg toolArgList = new ToolArg();
    private Input inputFiles = new Input();
    private Input tempInputFile = new Input();
    private String outputPath;
    private String outputFileName ;
    private static Random ran = new Random(9999); 
    private List<Section>  gensectList = new ArrayList<Section>();
    /**
     Call extern tool

     @param     buffer  The buffer to put the result with alignment
     **/
    public void toBuffer (DataOutputStream buffer){
        ///
        /// call extern tool
        ///
        try {
            executeTool ();
        } catch (Exception e) {
            throw new BuildException("Call to executeTool failed!\n" + e.getMessage());
        }

        ///
        /// check if file exist
        ///
        File outputFile = new File (this.outputFileName);
        if (!outputFile.exists()) {
            throw new BuildException("The file " + outputFile.getPath() + " does not exist!\n");
        }

        ///
        /// Read output file and write it's cotains to buffer
        ///
        FileInputStream fs = null;
        DataInputStream in = null;
        try {
            fs  = new FileInputStream (outputFile);
            in  = new DataInputStream (fs);


            int fileLen = (int)outputFile.length();
            byte[] data  = new byte[fileLen];
            in.read(data);
            buffer.write(data, 0, fileLen);

            ///
            /// 4 byte alignment
            ///
            while ((fileLen & 0x03) != 0) {
                fileLen++;
                buffer.writeByte(0);
            }
        } catch (Exception e) {
            EdkLog.log(e.getMessage());
            throw new BuildException("Tool call, toBuffer failed!\n");
        } finally {
            try {
                if (in != null) {
                    in.close();
                }
                if (fs != null) {
                    fs.close();
                }
                outputFile.delete(); 
            } catch (Exception e) {
                EdkLog.log("WARNING: Cannot close " + outputFile.getPath());
            }
        }
    }

    ///
    /// execute external tool for genffsfile
    ///
    private void executeTool () {
        String command   = "";
        String argument  = "";
        command          = toolName;
        
        //
        //  Get each section which under the compress {};
        //  And add it is contains to File;
        //
        Section sect;
        try{
            Iterator SectionIter = this.gensectList.iterator();
            while (SectionIter.hasNext()){
                sect = (Section)SectionIter.next();
                //
                // Parse <genSection> element
                //
                File outputFile = File.createTempFile("temp", "sec1", new File(outputPath));
                FileOutputStream bo = new FileOutputStream(outputFile);
                DataOutputStream Do = new DataOutputStream (bo);
                //
                //  Call each section class's toBuffer function.
                //
                try {
                    sect.toBuffer(Do);
                }
                catch (BuildException e) {
                    EdkLog.log(e.getMessage());
                    throw new BuildException ("GenSection failed at Tool!");
                } finally {
                    if (Do != null){
                        Do.close();    
                    }
                    
                } 
                this.tempInputFile.insFile(outputFile.getPath());
            }        
        } catch (IOException e){
            throw new BuildException ("Gensection failed at tool!");
        } 

        try {
            this.outputFileName = "Temp" + getRand();
            argument   = toolArgList + inputFiles.toStringWithSinglepPrefix(" -i ") 
                         + tempInputFile.toString(" ")+ " -o " + outputFileName;
            EdkLog.log(this, EdkLog.EDK_VERBOSE, command + " " + argument);
            ///
            /// execute command line
            ///
            Process process = Runtime.getRuntime().exec(command + " " + argument);
            process.waitFor();
            Iterator tempFile = tempInputFile.getNameList().iterator();
            while (tempFile.hasNext()){
                File file = new File((String)tempFile.next());
                if (file.exists()) {
                    file.delete();
                }
            }
        } catch (Exception e) {
            EdkLog.log(e.getMessage());
            throw new BuildException("Execution of externalTool task failed!\n");
        }
    }

    /**
     Add method of ANT task/datatype for nested ToolArg type of element

     @param     toolArg     The ToolArg object containing arguments for the tool
     **/
    public void addConfiguredToolArg (ToolArg toolArg) {
        toolArgList.insert(toolArg);
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
    public void addConfiguredInput(Input file) {
        inputFiles.insert(file);
    }
    
//    /**
//      addTool
//      
//      This function is to add instance of Tool to list.
//      
//      @param tool             instance of Tool.
//    **/
//    public void addTool(Tool tool){
//        this.toolList.add(tool);
//    }
    
    public void addGenSection(GenSectionTask genSect){
        this.gensectList.add(genSect);
    }

    /**
     Get random number.

     @returns   The random integer.
     **/
    public synchronized int getRand() {
        return ran.nextInt();
    }

    public int getAlignment() {
        return alignment;
    }

    public void setAlignment(int alignment) {
        if (alignment > 7) {
            this.alignment = 7;
        } else {
            this.alignment = alignment;
        }
    }

}


