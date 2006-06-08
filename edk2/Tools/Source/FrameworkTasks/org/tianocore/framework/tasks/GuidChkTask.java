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
    ///
    /// Directory name of exclusion searching 
    ///
    private String exDir = "";
    ///
    /// File name of exclusion searching.
    ///
    private String exFile = "";
    ///
    /// Extension name of exclusion searching.
    ///
    private String exExt = "";
    ///
    /// Extesnion name of sub dir which excluded searching.
    ///
    private String exSubDir = "";
    ///
    /// Out put file wrote internal GUID+basename list
    ///
    private String outFile = "";
    ///
    /// Check for duplicate guids.
    ///
    private String chkGui = "";
    ///
    /// Check for duplicate signatures
    ///
    private String chkSign = "";
    ///
    /// If set will print guid+defined symbol name
    ///
    private String printGuiDef = "";
    ///
    /// If set will print all GUIDS found
    ///
    private String printAllGuid = "";
    ///
    /// redirection file name.
    ///
    private String outPut = "";
    ///
    /// out put redirect to this file.
    ///
    protected PrintWriter fos = null;
    
    //
    // overload class execute method
    //   
    public void execute() throws BuildException {
        Project project = this.getOwningTarget().getProject();
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH");
        String command;
        if (path == null) {
            command = "GuidChk";
        } else {
            command = path + File.separatorChar + "GuidChk";
        }
        String argument = exDir +
                          exFile +
                          exExt +
                          exSubDir +
                          outFile +
                          chkGui +
                          chkSign +
                          printGuiDef + 
                          printAllGuid;     
        try {
            System.out.println(command + " " + argument);
            //
            // execute command line 
            //
            Process proc = Runtime.getRuntime().exec(command + "" + argument);
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
                System.out.println(line);
                while (line != null ){
                    System.out.print(line);
                    line = bin.readLine();
                }               
            }                      
            System.out.println("GuidChkTask Success!");
        } catch (Exception e) {
            System.out.println("GuidChkTask failed!");
            System.out.println(e.getMessage());
            
        }
    }
    /**
      getChkGui
      
      This function is to get the string of flag of ChkGui
      
      @return                     string of flag of ChkGui
    **/
    public String getChkGui() {
        return chkGui;
    }
    
    /**
      setChkGui
      
      This function is to set chkGui
      
      @param chkGui               set class member of chkGui
    **/
    public void setChkGui(String chkGui) {
        if (chkGui.equals("on")||(chkGui.equals("ON"))){
            this.chkGui = " -g ";
        }
        
    }
    
    /**
      getChkSign
      
      This function is to get chkSign
      
      @return                  chkSign
    **/
    public String getChkSign() {
        return chkSign;
    }
    
    /**
      setChkSign
      
      This function is to set class member of chkSign
     * @param chkSign
     */
    public void setChkSign(String chkSign) {
        if (chkSign.equals("on")|| chkSign.equals("ON")){
            this.chkSign = " -s ";
        }       
    }
    /**
      getExDir
      
      This function is to get class member of exDir
      
      @return                 exDir
    **/
    public String getExDir() {
        return exDir;
    }
    
    /**
      setExDir
      
      This function is to set class member of exDir
      
      @param                   exDir
    **/
    public void setExDir(String exDir) {
        this.exDir = " -d " + exDir;
    }
    
    /**
      getExExt
      
      This function is to get class member of exExt
      
      @return                    exExt
    **/
    public String getExExt() {
        return exExt;
    }
    
    /**
      setExExt
      
      This function is to set class member of exExt
      @param                      exExt
    **/
    public void setExExt(String exExt) {
        this.exExt = " -e " + exExt;
    }
    
    /**
      getExFile
      
      This function is to get class member of exFile
      @return                    exFile
    **/
    public String getExFile() {
        return exFile;
    }
    
    /**
      setExFile
      
      This function is to set class member of exFile.
      
     @param                       exFile
    **/
    public void setExFile(String exFile) {
        this.exFile = " -f " + exFile;
    }
    
    /**
      getExSubDir
      
      This function is to get class member of exSubDir
      
      @return                      exSubDir
    **/
    public String getExSubDir() {
        return exSubDir;
    }
    
    /**
      setExSubDir
      
      This function is to set class member of exSubDir.
     @param                         exSubDir
    **/
    public void setExSubDir(String exSubDir) {
        this.exSubDir = " -u " + exSubDir;
    }
    
    /**
      getOutFile
      
      This function is to get outFile
      
     @return                        outFile
    **/
    public String getOutFile() {
        return outFile;
    }
    /**
     * set class member of outFile
     * @param outFile
     */
    public void setOutFile(String outFile) {
        this.outFile = " -b " + outFile;
    }
    /**
      getPrintGuidDef
      
      This function is to get printGuidDef
      
      @return     flage of printing (guid+defined symbol name)
    **/
    public String getPrintGuiDef() {
        return printGuiDef;
    }
    
    
    /**
      setPrintGuidDef
      
      This function is to set class member of printGuiDef.
      @param       printGuiDef
    **/
    public void setPrintGuiDef(String printGuiDef) {
        if (printGuiDef.equals("on")|| printGuiDef.equals("ON")){
            this.printGuiDef = " -x ";
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
        return printAllGuid;
    }
    
    /**
      setPrintAllGuid
      
      This function is to set class member of printAllGuid.
      @param          printAllGuid
    **/
    public void setPrintAllGuid(String printAllGuid) {
        if (printAllGuid.equals("on")||printAllGuid.equals("ON")) {
            this.printAllGuid = " -p ";
        }       
    }
}

