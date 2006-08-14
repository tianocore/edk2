/** @file
 FileParser class.

 FileParser class is to parse file which contains the list of file name and 
 add those files to list.
  
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.framework.tasks;
import java.io.*;
import java.util.List;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;

/**
  FileParser class.

  FileParser class is to parse file which contains the list of file name and 
  add those files to list.
**/
public class  FileParser {
    /**
      loadFile
     
      This function is to add files to array from input file which contains the 
      files list.
      @param  project  The current project.
      @param  list     File array.
      @param  file     File which contains the file list.
      @param  tag      Target of architecture.
      @throws BuildException
    **/
    public static synchronized void loadFile(Project project, List<Object> list, File file, String tag) throws BuildException{
        FileReader fileReader;
        BufferedReader in;
        String str;
        
        if (!file.exists()) {
            throw new BuildException("The file, " + file + " does not exist!");           
        } 
        try {
            fileReader = new FileReader(file);
            in = new BufferedReader(fileReader);
            while((str=in.readLine())!= null){
                if (str.trim()==""){
                    continue;
                }
                str = project.replaceProperties(str);
                if (str.trim().substring(0,2).equalsIgnoreCase(tag)) {
                    list.add(str.trim());
                } else {
                    list.add(tag + " " + str.trim());
                }
                
            }
        } catch (Exception e){
            System.out.println(e.getMessage());
            
        }
    }
    
}
