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

import org.tianocore.exception.EdkException;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.List;

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
    public static synchronized String[][] parse(String filename) throws EdkException {
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
    public static synchronized String[][] parse(String confPath, String filename) throws EdkException {
        //Map<String, String> map = new TreeMap<String,String>(comparator);
        List<String> keyList = new ArrayList<String>(256);
        List<String> valueList = new ArrayList<String>(256);

        try {
            File file = new File(confPath + File.separatorChar + filename);
            FileReader reader = new FileReader(file);
            BufferedReader in = new BufferedReader(reader);
            String str;

            while ((str = in.readLine()) != null) {
                str = str.trim();
                //
                // if str is empty line, comments (start with '#'),
                // without '=', or start with '='
                //
                int index;
                if (str.length() == 0 || str.startsWith("#") || 
                    (index = str.indexOf('=')) <= 0) {
                    continue;
                }
                //
                // look as line "A = B"
                //
                keyList.add(str.substring(0, index).trim());
                valueList.add(str.substring(index + 1).trim());
            }
        } catch (Exception e) {
            throw new EdkException("ERROR Processing file [" + filename + "].\n" + e.getMessage());
        }

        String[][] definitions = new String[2][keyList.size()];
        definitions[0] = (String[])keyList.toArray(definitions[0]);
        definitions[1] = (String[])valueList.toArray(definitions[1]);

        return definitions;
    }

    public static synchronized ToolChainMap parseToolChainConfig(File ConfigFile) throws EdkException {
        ToolChainMap map = new ToolChainMap();
    
        try {
            FileReader reader = new FileReader(ConfigFile);
            BufferedReader in = new BufferedReader(reader);
            String str;

            while ((str = in.readLine()) != null) {
                str = str.trim();
                //
                // if str is empty line, comments (start with '#'),
                // without '=', or start with '='
                //
                int index;
                if (str.length() == 0 || str.startsWith("#") || 
                    (index = str.indexOf('=')) <= 0) {
                    continue;
                }
                //
                // look as line "A = B"
                //
                String key = str.substring(0, index).trim().toUpperCase();
                String value = str.substring(index + 1).trim();
                map.put(key, value);
            }
        } catch (Exception e) {
            throw new EdkException("ERROR Processing file [" + ConfigFile.getAbsolutePath() + "].\n" + e.getMessage());
        }

        return map;
    }
}


