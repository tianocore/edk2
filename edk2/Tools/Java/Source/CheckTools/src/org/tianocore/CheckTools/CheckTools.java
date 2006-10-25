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
 * -i = INTERACTIVE checks the "active" tools_def.txt file and lets the 
 *              user modify invalid entries.
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
 *
 * -t = TEST can be used with -f or -s, not with -i.
 * 
 * -d = DUMP can be used with -f, not with -i
 * 
 * -q = QUIET - turns off all System.out.print statements, the return code
 *              is the only thing that will determine PASS or FAIL
 *
 */
package org.tianocore.CheckTools;

import java.io.*;

public class CheckTools {
    private static int DEBUG = 0;

    private static final String copyright = "Copyright (c) 2006, Intel Corporation      All rights reserved.";

    private static final String version = "Version 0.1";

    private int VERBOSE = 0;

    private boolean QUIET = false;

    // private String argv[];

    private final int DEFAULT = 1;

    private final int TEST = 2;

    private final int SCAN = 4;

    private final int DUMP = 8;

    private final int INTERACTIVE = 16;

    private boolean USERFILE = false;

    private String inFile = "";

    private final int PASS = 0;

    private final int FAIL = 1;

    private String SEP = System.getProperty("file.separator");

    public static void main(String[] argv) {
        CheckTools cts = new CheckTools();
        int exitCode = cts.checkTool(argv);
        if (DEBUG > 10)
            System.out.println("Exit Code: " + exitCode);
        if (exitCode == -1) {
            new CheckTools().usage();
            System.exit(1);
        }
        System.exit(exitCode);
    }

    private int checkTool(String[] arguments) {
        if ((DEBUG > 1) && (arguments.length > 0))
            System.out.println("Arguments: ");
        int cmdCode = DEFAULT;
        if (arguments.length > 0) {
            cmdCode = DEFAULT;
            for (int i = 0; i < arguments.length; i++)
                if (arguments[i].toLowerCase().startsWith("-q"))
                    QUIET = true;
            for (int i = 0; i < arguments.length; i++) {
                String arg = arguments[i];
                if (DEBUG > 1)
                    System.out.println(" [" + i + "] " + arg);
                if (!(arg.toLowerCase().startsWith("-t") || arg.toLowerCase().startsWith("-s")
                      || arg.toLowerCase().startsWith("-d") || arg.toLowerCase().startsWith("-i")
                      || arg.toLowerCase().startsWith("-v") || arg.toLowerCase().startsWith("-q")
                      || arg.toLowerCase().startsWith("-h") || arg.toLowerCase().startsWith("-f"))) {
                    // Only allow valid option flags
                    if (QUIET == false) {
                        System.out.println("Invalid argument: " + arg);
                        usage();
                    }
                    System.exit(FAIL);
                }

                if (arg.toLowerCase().startsWith("-d")) {
                    if (cmdCode == DEFAULT) {
                        cmdCode = DUMP;
                    } else {
                        if (QUIET == false)
                            System.out.println("Invalid Options");
                        usage();
                        System.exit(FAIL);
                    }
                }
                if (arg.toLowerCase().startsWith("-f")) {
                    i++;
                    inFile = arguments[i];
                    USERFILE = true;
                }
                if (arg.toLowerCase().startsWith("-h")) {
                    usage();
                    System.exit(PASS);
                }
                if (arg.toLowerCase().startsWith("-i")) {
                    // Interactive can be specified with any
                    // other option - it turns on the query
                    // on fail mode.
                    cmdCode = cmdCode | INTERACTIVE;
                }
                if (arg.toLowerCase().startsWith("-q")) {
                    QUIET = true;
                }
                if (arg.toLowerCase().startsWith("-s")) {
                    if (cmdCode == DEFAULT) {
                        cmdCode = SCAN;
                    } else {
                        if (!QUIET) {
                            System.out.println("Invalid Options");
                            usage();
                        }
                        System.exit(FAIL);
                    }
                }
                if (arg.toLowerCase().startsWith("-t")) {
                    if (cmdCode == DEFAULT) {
                        cmdCode = TEST;
                    } else {
                        if (!QUIET) {
                            System.out.println("Invalid Options");
                            usage();
                        }
                        System.exit(FAIL);
                    }
                }
                if (arg.startsWith("-v")) {
                    // Verbose level can be increased to print
                    // more INFO messages.
                    VERBOSE += 1;
                }
                if (arg.startsWith("-V")) {
                    if (!QUIET) {
                        System.out.println(copyright);
                        System.out.println("CheckTools, " + version);
                    }
                    System.exit(PASS);
                }
            }
        }
        if (QUIET)
            VERBOSE = 0;

        String WORKSPACE = System.getenv("WORKSPACE");
        if ((DEBUG > 0) || (VERBOSE > 0))
            System.out.println("Verifying Tool Chains for WORKSPACE: " + WORKSPACE);
        int returnCode = 0;

        if (WORKSPACE == null) {
            if (QUIET == false)
                System.out.println("Please set the environment variable, WORKSPACE and run again.");
            System.exit(FAIL);
        }
        String targetTxt = WORKSPACE + SEP + "Tools" + SEP + "Conf" + SEP + "target.txt";


        if (inFile.length() < 1) {
            //
            // Check the target.txt file for a  Tool Configuration File.
            // If not set, we use tools_def.txt, unless we are running with the
            // INTERACTIVE flag - where we check the template file before copying over to the
            // tools_def.txt file.
            //
            inFile = "tools_def.txt";
            File target = new File(targetTxt);
            String readLine = null;
            String fileLine[] = new String[2];
            if (target.exists()) {
                try {
                    FileReader fileReader = new FileReader(targetTxt);
                    BufferedReader bufReader = new BufferedReader(fileReader);
                    while ((readLine = bufReader.readLine()) != null) {
                        if (readLine.startsWith("TOOL_CHAIN_CONF")) {
                            fileLine = readLine.trim().split("=");
                            if (fileLine[1].trim().length() > 0) {
                                if (fileLine[1].trim().contains("Tools/Conf/"))
                                    inFile = fileLine[1].replace("Tools/Conf/", "").trim();
                                else
                                    inFile = fileLine[1].trim();
                            }
                        }
                    }
                    bufReader.close();
                } catch (IOException e) {
                    if (QUIET == false)
                        System.out.println(" [target.txt] Read Error: " + e);
                    System.exit(FAIL);
                }
            }
        }

        // OK, now check the infile of we had one.
        String toolsDef = WORKSPACE + SEP + "Tools" + SEP + "Conf" + SEP + inFile;
        File toolsFile = new File(toolsDef);
        if (!toolsFile.exists()) {
            // use the template file
            if (USERFILE) {
                if (QUIET == false) {
                    System.out.println("Could not locate the specified file: " + inFile);
                    System.out.println(" It must be located in the WORKSPACE" + SEP + "Tools" + SEP + "Conf directory");
                }
                System.exit(FAIL);
            }
            toolsDef = WORKSPACE + SEP + "Tools" + SEP + "Conf" + SEP + "tools_def.template";
            File toolsTemplate = new File(toolsDef);
            if (!toolsTemplate.exists()) {
                if (QUIET == false)
                    System.out.println("Your WORKSPACE is not properly configured!");
                System.exit(FAIL);
            } else {
                if (QUIET == false)
                    System.out.println("**** WARNING: No Tool Configuration File was found, using the template file, "
                                       + toolsDef);
            }
        }

        //
        // at this point the file, toolsDef points to a tool configuration file of some sort.
        //
        // check tool configuration file
        if (DEBUG > 2)
            System.out.println("Calling checkTools(" + toolsDef + ", " + cmdCode + ", " + VERBOSE + ")");
        ToolChecks tc = new ToolChecks();
        returnCode = tc.checkTools(toolsDef, cmdCode, VERBOSE, QUIET);

        if (VERBOSE > 10)
            System.out.println(" checkTools returned: " + returnCode);
        return returnCode;
    }

    private void usage() {
        if (QUIET)
            return;
        System.out
                  .println("Usage: checkTools [-h] [-d] [-i] [-v] [-s | -scan] [-t | -test] [-q | -quiet] [[-f | -filename] filename.txt]");
        System.out.println("  Where");
        System.out.println("     -h           Help - display this screen.");
        System.out.println("     -d           Dump - display the tool defintion file in user readable format");
        System.out.println("     -i           Interactive query - not yet implemented!");
        System.out.println("     -v           Verbose - add up to 3 -v options to increase info messages.");
        System.out.println("     -s           Scan - search the usual places on your system for tools.");
        System.out.println("                    The Scan feature not yet implemented!.");
        System.out.println("     -t           Test - checks that PATH entries in the tool configuration file exist.");
        System.out
                  .println("     -q           Quiet - no messages get printed, the return value determines pass or fail.");
        System.out.println("     -f filename  Use filename instead of the file specified in target.txt or");
        System.out.println("                    tools_def.txt or tools_def.template.");
        System.out.println("                    By Rule, all tool configuration files must reside in the");
        System.out.println("                    WORKSPACE" + SEP + "Tools" + SEP + "Conf directory.");
    }
}
