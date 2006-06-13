/** @file
 GenCRC32SectionTask class.

 GenCRC32SectionTask is to call GenCRC32Section.exe to generate crc32 section.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/

package org.tianocore.framework.tasks;

import java.util.*;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.taskdefs.Execute;
import org.apache.tools.ant.taskdefs.LogStreamHandler;
import org.apache.tools.ant.types.Commandline;

/**
  GenCRC32SectionTask
  
  GenCRC32SectionTask is to call GenCRC32Section.exe to generate crc32 section. 
  
**/
public class GenCRC32SectionTask extends Task implements EfiDefine{
    ///
    /// output file
    ///
    private String outputFile;
    ///
    /// inputFile list
    ///
    private List<Object> inputFileList = new ArrayList<Object>();
    
    ///
    /// Project
    ///
    static private Project project;
    
    /**
      execute
      
      GenCRC32SectionTask execute is to assemble tool command line & execute
      tool command line
      
      @throws BuildException
    **/
    public void execute() throws BuildException {
        
        project = this.getOwningTarget().getProject(); 
        ///
        /// absolute path of efi tools
        ///
        String path = project.getProperty("env.FRAMEWORK_TOOLS_PATH"); 
        String command;
        if (path == null) {
            command = "GenCRC32Section";
        } else {
            command = path + "/" + "GenCRC32Section" ;
        }
        // 
        // string line of input files 
        // 
        String inputFiles = list2Str(inputFileList, ""); 
        // 
        // assemble argument 
        //
        String argument =  inputFiles + outputFile; 
        // 
        // return value of fwimage execution 
        //
        int revl = -1; 
        
        try {
            Commandline cmdline = new Commandline();
            cmdline.setExecutable(command);
            cmdline.createArgument().setLine(argument);
            
            LogStreamHandler streamHandler = new LogStreamHandler(this, Project.MSG_INFO, Project.MSG_WARN);
            Execute runner = new Execute(streamHandler, null);
            
            runner.setAntRun(project);
            runner.setCommandline(cmdline.getCommandline());
            System.out.println(Commandline.toString(cmdline.getCommandline()));
            
            revl = runner.execute();
            if (EFI_SUCCESS == revl){
                //
                //  command execution success 
                //
                System.out.println("gencrc32section succeeded!");
            }
            else
            {
                // 
                // command execution fail
                //
                System.out.println("gencrc32section failed. (error=" + 
                    Integer.toHexString(revl) + 
                    ")"
                    );
            }
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }
        
    }

    /**
      addInputFile
     
      This function is to add a inputFile element into list
      @param inputFile : inputFile element
    **/
    public void addInputfile(InputFile inputFile) {
        inputFileList.add(inputFile);
    }
    
    /**
     get class member "outputFile"
     * @return name of output file
     */
    public String getOutputFile() {
        return this.outputFile;
    }
    /**
     * set class member "outputFile"
     * @param outputFile : outputFile parameter 
     */
    public void setOutputFile(String outputFile) {
        this.outputFile = " -o " + outputFile;
    };
    
    /**
     * transfer List to String
     * @param list : nested element list
     * @param tag : interval tag of parameter
     * @return string line of parameters 
     */
    private String list2Str(List list, String tag) {
        /*
         * string line for return
         */
        String paraStr = " -i"; 
        /*
         * nested element in list
         */
        NestElement element; 
        /*
         * iterator of nested element list
         */
        Iterator elementIter = list.iterator(); 
        /*
         * string parameter list
         */
        List<Object> strList = new ArrayList<Object>();   
        
        while (elementIter.hasNext()) {
            element = (NestElement) elementIter.next();
            if (null != element.getFile()) {
                FileParser.loadFile(project, strList, element.getFile(), tag);
            } else {
                paraStr = paraStr + element.getName();
            }
        }
        /*
         * iterator of string parameter list
         */
        Iterator strIter = strList.iterator();  
        while (strIter.hasNext()) {
            paraStr = paraStr + " " + strIter.next();
        }
        return paraStr;
    }   
}
