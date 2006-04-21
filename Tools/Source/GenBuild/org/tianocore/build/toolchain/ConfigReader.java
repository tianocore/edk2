/** @file
  ConfigReader class.
  
  ConfigReader is used to read tool chain config file with flat format. 
  
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.build.toolchain;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.Vector;

import org.apache.tools.ant.BuildException;

/**
  
  ConfigReader is used to read tool chain config file with flat format. Comments
  is line starting with character '#'.
  
  @since GenBuild 1.0
**/
public class ConfigReader {

    private static String confPath = ".";

    /**
      Public construct method. 
    **/
    public ConfigReader () {
    }

    /**
      Default filepath is ".".
    
      @param filename the config file name like "target.txt"
      @return the variables defined in file
    **/
    public static synchronized String[][] parse(String filename) {
        return parse(confPath, filename);
    }

    /**
      Get all variables defined in config file. the config file format is flat
      with "A=B". If line started with '#' looks as comments. 
    
      @param confPath the path of config file
      @param filename the file name of the config file
      @return the variables defined in the config file
      @throws BuildException
              Config file's format is not valid
    **/
    public static synchronized String[][] parse(String confPath, String filename) throws BuildException {
        try {
            Map<String, String> map = new HashMap<String, String>(20);
            File file = new File(confPath + File.separatorChar + filename);
            FileReader reader = new FileReader(file);
            BufferedReader in = new BufferedReader(reader);
            String str;
            while ((str = in.readLine()) != null) {
                str = str.trim();
                //
                // if str is empty line or comments (start with '#')
                //
                if (str.equalsIgnoreCase("") || str.startsWith("#")) {
                    continue;
                }
                //
                // if str without '=' or start with '='
                //
                if (str.indexOf('=') <= 0) {
                    continue;
                }
                //
                // look as line "A = B"
                //
                int index = str.indexOf('=');
                String key = str.substring(0, index).trim();
                String value = str.substring(index + 1).trim();
                //
                // if key is existed, then update
                //
                if (map.containsKey(key)) {
                    map.remove(key);
                }
                map.put(key, value);
            }
            Set keyset = map.keySet();
            Iterator iter = keyset.iterator();
            String[][] result = new String[map.size()][2];
            int i = 0;
            while (iter.hasNext()) {
                String key = (String) iter.next();
                result[i][0] = key;
                result[i++][1] = (String) map.get(key);
            }
            return result;
        } catch (Exception e) {
            throw new BuildException("Processor file [" + filename + "] error. \n" + e.getMessage());
        }
    }

    /**
      Parse global flags table. The format is like such(global flag name, value, 
      vendor_arch_cmd, [add flags], [sub flags]): 
      
      <pre>
        # EFI_DEBUG
        EFI_DEBUG YES MSFT_IA32_ASM    ADD.["/Zi", "/DEBUG"]
        EFI_DEBUG YES MSFT_IA32_CC     ADD.["/Zi", "/Gm", "/D EFI_DEBUG"] SUB.["/nologo", "/WX"]
        EFI_DEBUG YES MSFT_IA32_LINK   ADD.["/DEBUG"]
        EFI_DEBUG YES MSFT_NT32_CC     ADD.["/DEBUG"]
      </pre>
     
      @param confPath the file path of config file
      @param filename the file name of config file
      @return the value list
      @throws BuildException
              Config file is not valid
    **/
    public static synchronized String[][] parseTable(String confPath,
                    String filename) throws BuildException {
        try {
            Vector<String[]> vector = new Vector<String[]>(20);
            File file = new File(confPath + File.separatorChar + filename);
            FileReader reader = new FileReader(file);
            BufferedReader in = new BufferedReader(reader);
            String str;
            while ((str = in.readLine()) != null) {
                str = str.trim();
                //
                // if str is empty line or comments (start with '#')
                //
                if (str.equalsIgnoreCase("") || str.startsWith("#")) {
                    continue;
                }
                String[] item = new String[5];
                for(int i=0; i < item.length; i++){
                    item[i] = "";
                }
                //
                // EFI_DEBUG YES MSFT_IA32_ASM    ADD.["/Zi", "/DEBUG"]
                // FLAGS: EFI_DEBUG
                //
                int index = str.indexOf(" ");
                item[0] = str.substring(0, index);
                str = str.substring(index + 1).trim();
                //
                // Setting: YES
                //
                index = str.indexOf(" ");
                item[1] = str.substring(0, index);
                str = str.substring(index + 1).trim();
                //
                // Vendor_Arch_Commandtype: MSFT_IA32_ASM
                //
                index = str.indexOf(" ");
                item[2] = str.substring(0, index);
                str = str.substring(index + 1).trim();
                //
                // Add or/and Sub
                //
                if (str.startsWith("ADD.")) {
                    index = str.indexOf("]");
                    if ( index > 0){
                        item[3] = str.substring(5, index);
                        str = str.substring(index + 1).trim();
                    }
                }
                else if(str.startsWith("SUB.")){
                    index = str.indexOf("]");
                    if ( index > 0){
                        item[4] = str.substring(5, index);
                        str = str.substring(index + 1).trim();
                    }
                }
                else {
                    throw new BuildException("File [" + filename + "] never conform to Global Flags Table format.");
                }
                
                if (str.startsWith("ADD.")) {
                    index = str.indexOf("]");
                    if ( index > 0){
                        item[3] = str.substring(5, index);
                        str = str.substring(index + 1).trim();
                    }
                }
                else if(str.startsWith("SUB.")){
                    index = str.indexOf("]");
                    if ( index > 0){
                        item[4] = str.substring(5, index);
                        str = str.substring(index + 1).trim();
                    }
                }
                vector.addElement(item);
            }
            String[][] result = new String[vector.size()][5];
            for(int i=0; i < vector.size(); i++){
                result[i] = (String[])vector.get(i);
            }
            return result;
        } catch (Exception e) {
            throw new BuildException("Processor file [" + filename + "] error. \n" + e.getMessage());
        }
    }
}
