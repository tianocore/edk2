/** @file
 GuidChkTask class.

 GuidChkTask is to call GuidChk.exe to generate Section.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
**/
package org.tianocore.framework.tasks;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.io.File;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;

import org.tianocore.common.logger.EdkLog;

/**
  GuidChkTask
  
  GuidChkTask is to call GuidChk.exe to generate Section.
  
**/
public class GuidChkTask extends Task implements EfiDefine{
    /**
     *  GuidChk task class
     *  class member
     *    -exDir   : directory name of exclusion searching  
     *    -exFile  : file name of exclusion searching
     *    -exExt   : extension name of exclusion searching
     *    -exSubDir: extesnion name of sub dir which excluded searching
     *    -outFile : out put file wrote internal GUID+basename list
     *    -chkGui  : check for duplicate guids
     *    -chkSign : check for duplicate signatures
     *    -printGuiDef : if set will print guid+defined symbol name
     *    -printAllGuid: if set will print all GUIDS found
     *    -outPut  : redirection file name
     *    -fos     : out put redirect to this file 
     *    
     */
    //
    // Tool name
    // 
    private static String toolName = "GuidChk";
    //
    // Directory name of exclusion searching 
    //
    private FileArg exDir = new FileArg();
    //
    // File name of exclusion searching.
    //
    private FileArg exFile = new FileArg();
    //
    // Extension name of exclusion searching.
    //
    private FileArg exExt = new FileArg();
    //
    // Extesnion name of sub dir which excluded searching.
    //
    private FileArg exSubDir = new FileArg();
    //
    // Out put file wrote internal GUID+basename list
    //
    private FileArg outFile = new FileArg();
    //
    // Check for duplicate guids.
    //
    private ToolArg chkGui = new ToolArg();
    //
    // Check for duplicate signatures
    //
    private ToolArg chkSign = new ToolArg();
    //
    // If set will print guid+defined symbol name
    //
    private ToolArg printGuiDef = new ToolArg();
    //
    // If set will print all GUIDS found
    //
    private ToolArg printAllGuid = new ToolArg();
    //
    // redirection file name.
    //
    private String outPut = "";
    //
    // out put redirect to this file.
    //
    protected PrintWriter fos = null;
    
    //
    // overload class execute method
    //   
    public void execute() throws BuildException {
        Project project = this.getOwningTarget().getProject();
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH");
        String command;
        if (path == null) {
            command = toolName;
        } else {
            command = path + File.separatorChar + toolName;
        }
        String argument = "" + exDir +
                               exFile +
                               exExt +
                               exSubDir +
                               outFile +
                               chkGui +
                               chkSign +
                               printGuiDef + 
                               printAllGuid;     
        try {
            EdkLog.log(this, EdkLog.EDK_VERBOSE, command + " " + argument);
            //
            // execute command line 
            //
            Process proc = Runtime.getRuntime().exec(command + " " + argument);
            //
            // if set output, redirect out put to output file, else print output to screen
            //         
            if ( !this.outPut.equals("")) {
                fos = new PrintWriter(this.outPut);
                BufferedReader bin = new BufferedReader(new InputStreamReader(proc.getInputStream()));
                String line = bin.readLine();
                while (line != null ){                  
                    fos.println(line);
                    line = bin.readLine();
                }
                fos.close();
            }
            else {
                BufferedReader bin = new BufferedReader(new InputStreamReader(proc.getInputStream()));
                String line = bin.readLine();
                while (line != null ){
                    line = bin.readLine();
                }               
            }                      
            EdkLog.log(this, EdkLog.EDK_VERBOSE, toolName + " Succeeded!");
        } catch (Exception e) {
            throw new BuildException(toolName + " failed!");
        }
    }
    /**
      getChkGui
      
      This function is to get the string of flag of ChkGui
      
      @return                     string of flag of ChkGui
    **/
    public String getChkGui() {
        return chkGui.getValue();
    }
    
    /**
      setChkGui
      
      This function is to set chkGui
      
      @param chkGui               set class member of chkGui
    **/
    public void setChkGui(boolean chkGui) {
        if (chkGui) {
            this.chkGui.setArg(" -", "g");
        }        
    }
    
    /**
      getChkSign
      
      This function is to get chkSign
      
      @return                  chkSign
    **/
    public String getChkSign() {
        return chkSign.getValue();
    }
    
    /**
      setChkSign
      
      This function is to set class member of chkSign
     * @param chkSign
     */
    public void setChkSign(boolean chkSign) {
        if (chkSign){
            this.chkSign.setArg(" -", "s");
        }       
    }
    /**
      getExDir
      
      This function is to get class member of exDir
      
      @return                 exDir
    **/
    public String getExDir() {
        return exDir.getValue();
    }
    
    /**
      setExDir
      
      This function is to set class member of exDir
      
      @param                   exDir
    **/
    public void setExDir(String exDir) {
        this.exDir.setArg(" -d ", exDir);
    }
    
    /**
      getExExt
      
      This function is to get class member of exExt
      
      @return                    exExt
    **/
    public String getExExt() {
        return exExt.getValue();
    }
    
    /**
      setExExt
      
      This function is to set class member of exExt
      @param                      exExt
    **/
    public void setExExt(String exExt) {
        this.exExt.setArg(" -e ", exExt);
    }
    
    /**
      getExFile
      
      This function is to get class member of exFile
      @return                    exFile
    **/
    public String getExFile() {
        return exFile.getValue();
    }
    
    /**
      setExFile
      
      This function is to set class member of exFile.
      
     @param                       exFile
    **/
    public void setExFile(String exFile) {
        this.exFile.setArg(" -f ", exFile);
    }
    
    /**
      getExSubDir
      
      This function is to get class member of exSubDir
      
      @return                      exSubDir
    **/
    public String getExSubDir() {
        return exSubDir.getValue();
    }
    
    /**
      setExSubDir
      
      This function is to set class member of exSubDir.
     @param                         exSubDir
    **/
    public void setExSubDir(String exSubDir) {
        this.exSubDir.setArg(" -u ", exSubDir);
    }
    
    /**
      getOutFile
      
      This function is to get outFile
      
     @return                        outFile
    **/
    public String getOutFile() {
        return outFile.getValue();
    }
    /**
     * set class member of outFile
     * @param outFile
     */
    public void setOutFile(String outFile) {
        this.outFile.setArg(" -b ", outFile);
    }
    /**
      getPrintGuidDef
      
      This function is to get printGuidDef
      
      @return     flage of printing (guid+defined symbol name)
    **/
    public String getPrintGuiDef() {
        return printGuiDef.getValue();
    }
    
    
    /**
      setPrintGuidDef
      
      This function is to set class member of printGuiDef.
      @param       printGuiDef
    **/
    public void setPrintGuiDef(boolean printGuiDef) {
        if (printGuiDef){
            this.printGuiDef.setArg(" -", "x");
        }
        
    }
    
    /**
      getOutput
      
      This function is to get output
      
      @return       name of outPut file
    **/
    public String getOutPut() {
        return outPut;
    }
    
    /**
      setOutPut
      
      This function is to set class member of outPut.
      @param outPut
    **/
    public void setOutPut(String outPut) {
        this.outPut = outPut;
    }
    
    /**
      getPrintAllGuid
      
      This function is to get printAllGuid
      @return         printAllGuid
    **/
    public String getPrintAllGuid() {
        return printAllGuid.getValue();
    }
    
    /**
      setPrintAllGuid
      
      This function is to set class member of printAllGuid.
      @param          printAllGuid
    **/
    public void setPrintAllGuid(boolean printAllGuid) {
        if (printAllGuid) {
            this.printAllGuid.setArg(" -", "p");
        }       
    }
}

