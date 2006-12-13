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

import org.apache.tools.ant.Project;

import org.tianocore.build.exception.GenBuildException;

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

    /**
      Parse specified tool chain definition file.
    
      @param    filename    The config file name with full path

      @return String[][]    The definition array
    **/
    public static synchronized String[][] parse(Project prj, String filename) throws GenBuildException {
        return parse(prj, new File(filename));
    }

    /**
      Get all definitions in config file. the config file format is flat
      with "A=B". If line started with '#' looks as comments. 
    
      @param    configFile      The config file

      @return   String[][]      The variables defined in the config file

      @throws   GenBuildException
                Config file's format is not valid
    **/
    public static synchronized String[][] parse(Project prj, File configFile) throws GenBuildException {
        List<String> keyList = new ArrayList<String>(256);
        List<String> valueList = new ArrayList<String>(256);
        int lines = 0;

        try {
            FileReader reader = new FileReader(configFile);
            BufferedReader in = new BufferedReader(reader);
            String str;

            while ((str = in.readLine()) != null) {
                ++lines;
                str = str.trim();
                //
                // skip empty line, comment (start with '#') 
                //
                if (str.length() == 0 || str.startsWith("#")) {
                    continue;
                }

                //
                // stop if the definition line is not in "name=value" form
                // 
                int index;
                if ((index = str.indexOf('=')) <= 0) {
                    throw new GenBuildException("ERROR Processing file [" 
                        + configFile.getAbsolutePath() 
                        + "] (line " + lines + ").\n");
                }

                //
                // look as line "A = B"
                //
                keyList.add(str.substring(0, index).trim());
                if (prj != null) {
                    valueList.add(prj.replaceProperties(str.substring(index + 1).trim()));
                } else {
                    valueList.add(str.substring(index + 1).trim());
                }
            }
        } catch (Exception ex) {
            GenBuildException e = new GenBuildException("ERROR Processing file [" 
                + configFile.getAbsolutePath() 
                + "] (line " + lines + ").\n" + ex.getMessage());
            e.setStackTrace(ex.getStackTrace());
            throw e;
        }

        String[][] definitions = new String[2][keyList.size()];
        definitions[0] = (String[])keyList.toArray(definitions[0]);
        definitions[1] = (String[])valueList.toArray(definitions[1]);

        return definitions;
    }
}


