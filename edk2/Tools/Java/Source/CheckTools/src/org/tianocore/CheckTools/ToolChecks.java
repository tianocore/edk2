/** @file
 Verify the tool configuration file for location of the correct tools.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/

/**
 * This tool checks to see if specified tool chain paths exist.
 * It will check all specified paths, as indicated by the last field of the
 * tool property line equal to _PATH
 * 
 * no option = check 1) the file specified in target.txt or 2) tools_def.txt
 *              if neither is found, we check the tools_def.template file.
 * -i = INIT checks the tools_def.template file
 * 
 * -s = SCAN will check all standard locations for tool chains
 *  C:\Program Files\Microsoft Visual Studio *
 *  C:\WINDDK
 *  C:\Bin
 *  C:\ASL
 *  C:\MASM*
 *  /opt/tiano
 *  
 * -f = FILE check the tools in this file instead of tools_def.txt, or
 *      a file that was specified in target.txt
 */
package org.tianocore.CheckTools;

import java.io.*;
import java.util.*;

public class ToolChecks {
    private static int DEBUG = 0;

    private final int DEFAULT = 1;

    private final int TEST = 2;

    private final int SCAN = 4;

    private final int INTERACTIVE = 8;

    private final int PASS = 0;

    private final int FAIL = 1;
    
    private ArrayList<String> errLog = new ArrayList<String>();
    
    private ArrayList<String> goodLog = new ArrayList<String>();

    public int checkTools(String toolConfFile, int cmdCode, int VERBOSE) {

        int returnCode = FAIL;
        boolean interActive = false;

        if ((DEBUG > 0) || (VERBOSE > 0)) {
            System.out.println("Using Tool Configuration File: " + toolConfFile);
        }
        
        if (DEBUG > 2)
            System.out.println("The cmdCode: " + cmdCode);
        
        if ((cmdCode & INTERACTIVE) == INTERACTIVE) {
            interActive = true;
            System.out.println("***** WARNING ***** The Interactive function has not been implemented yet!");
        }

        if ((cmdCode & SCAN) == SCAN) {
            returnCode = scanFile(toolConfFile, interActive, VERBOSE);
        }

        if (((cmdCode & TEST) == TEST) || ((cmdCode & DEFAULT) == DEFAULT))
            returnCode = testFile(toolConfFile, interActive, VERBOSE);
        
        if (!errLog.isEmpty()) {
            System.out.println("Tool Configuration File: " + toolConfFile);
            for (int i = 0; i < goodLog.size(); i++)
                System.out.println("Tool Chain Tag Name: " + goodLog.get(i) + " is valid!");
            for (int i = 0; i < errLog.size(); i++)
                System.out.println(errLog.get(i));
            if (VERBOSE > 0) {
                System.out.println();
                System.out.println("You can remove these WARNING messages by editing the file:");
                System.out.println("  " + toolConfFile);
                System.out.println("and commenting out out or deleting the entries for the tool");
                System.out.println("chain tag names that do not apply to your system.");
            }
        }

        return returnCode;
    }

    private int scanFile(String testFile, boolean interActive, int VERBOSE) {
        if ((DEBUG > 0) || (VERBOSE > 0))
            System.out.println("Scanning the Normal Installation Locations ...");
        System.out.println("The Scan function has not been implemented yet!");        
        return FAIL;
    }
    private int testFile(String testFile, boolean interActive, int VERBOSE) {

        int retCode = PASS;
        String readLine = "";
        String fileLine[] = new String[2];
        try {
            FileReader toolConfFile = new FileReader(testFile);
            BufferedReader reader = new BufferedReader(toolConfFile);
            String path = "";
            String props[] = new String[5];
            String lastErrTag = "barf";
            String lastTag = "barf";
            while ((readLine = reader.readLine()) != null) {
                if ((!readLine.startsWith("#")) && (readLine.contains("_PATH"))) {
                    if (DEBUG > 2) {
                        System.out.println(" PATH LINE: " + readLine);
                    }
                    readLine = readLine.trim();
                    fileLine = readLine.split("=");
                    path = fileLine[1].trim();
                    props = fileLine[0].split("_");
                    File testPath = new File(path);
                    if (!testPath.exists()) {
                        if (!props[1].trim().contentEquals(lastErrTag))
                            errLog.add("  -- WARNING: Tool Chain Tag Name: " + props[1].trim() + " is NOT valid!");
                        if (VERBOSE > 1)
                            errLog.add("    Tool Code: [" + props[3].trim() + "] Path: " + path + " does not exist!");
                        retCode = 1;
                        lastErrTag = props[1].trim();
                    } else {
                        if ((DEBUG > 0) || (VERBOSE > 0)) {
                            if ((!props[1].trim().contentEquals(lastTag))
                                && (!props[1].trim().contentEquals(lastErrTag)))
                                System.out.println("Tool Chain: " + props[1].trim() + " is valid");
                        }
                        if (!props[1].trim().contentEquals(lastTag))
                            goodLog.add(props[1].trim());
                        lastTag = props[1].trim();                                                
                    }
                }
            }
        } catch (IOException e) {
            System.out.println(" [" + testFile + "] " + e);
            System.exit(1);
        }
        if (errLog.size() > 0)
            for (int i = 0; i < goodLog.size(); i++) {
                for (int j = 0; j < errLog.size(); j++) {
                    if (errLog.get(j).contains(goodLog.get(i).trim())) {
                        goodLog.remove(i);
                        break;
                    }
                }
            }
        return retCode;

    }

}
