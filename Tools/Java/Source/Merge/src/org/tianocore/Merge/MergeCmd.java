// @file
//  MergeCmd command-line interface to the classes that combine
//      multiple MSA files into a single MSA file.
//
//  Copyright (c) 2006, Intel Corporation    All rights reserved.
//
//  This program and the accompanying materials are licensed and made
//  available under the terms and conditions of the BSD License which
//  accompanies this distribution.  The full text of the license may 
//  be found at  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//    This program is the command line interface to the CombineMsa class, which
//    will take the following arguments:
//
//  Input:
//      -t Target   The MSA file that will be created
//      -u UiName   The UiName for the merged MSA   OPTIONAL
//                  If not provided, the UiName will come from the
//                  first leaf MSA file
//      -p Package  The SPD file that the new MSA file will be added to.  OPTIONAL
//      leaf.msa    The path and filename of the MSA files to be merged into the Target.
//
//  Output:
//      Target.msa
//
//  Modifies - OPTIONAL
//      Package.spd
//

package org.tianocore.Merge;

import java.io.*;
import java.util.*;

// import org.tianocore.Merge.CombineMsa;

public class MergeCmd {

    private static int DEBUG = 1;
    
    private static final String copyright  = "Copyright (c) 2006, Intel Corporation      All rights reserved.";
    
    private static final String version = "Version 0.1";
    
    private int VERBOSE = 0;

    private String targetFile = null;

    private ArrayList<String> leafFiles = new ArrayList<String>();

    private String spdFile = null;

    private String uiName = null;
    
    private String fileBasename = null;

    private final int ESUCCESS = 0;

    private final int EFAILURE = 1;

    private final static int FOUND = 1;

    private final static int NOTFOUND = 0;

    private int result = ESUCCESS;

    public int MergeCmdLine(String[] args) {
        result = parseCmdLine(args);
        if (result == ESUCCESS) {
            if ((DEBUG > 7) || (VERBOSE > 5)) {
                System.out.println("Parse Succeeded!");
                System.out.println("CWD: " + System.getProperty("user.dir"));
                System.out.println("Merge Module Name:   " + targetFile);
                System.out.println("Found Leaf Module:   " + leafFiles.size());
                if (spdFile != null)
                    System.out.println("Package Name:        " + spdFile);
                if (uiName != null)
                    System.out.println("User Interface Name: " + uiName);
            }
            CombineMsa newMsa = new CombineMsa();
            result = newMsa.combineMsaFiles(targetFile, leafFiles, uiName, spdFile, fileBasename, VERBOSE);
        }
        return result;
    }

    private int parseCmdLine(String[] args) {

        if (args.length == NOTFOUND) {
            outputUsage();
            System.exit(EFAILURE);
        }

        for (int i = 0; i < args.length; i++) {
            if (args[i].toLowerCase().contains("-t")) {
                i++;
                targetFile = args[i];
                targetFile.replace(" ", "_");
                if (!targetFile.toLowerCase().contains(".msa"))
                    targetFile = targetFile + ".msa";


            } else if (args[i].toLowerCase().contains("-p")) {
                i++;
                spdFile = args[i];
                if (!spdFile.toLowerCase().contains(".spd"))
                    spdFile = spdFile + ".spd";
                spdFile = spdFile.replace("\\", "/").trim();
                if (testFile(spdFile) == NOTFOUND) {
                    System.out.println("WARNING: The Package file: " + spdFile + " does NOT exist!");
                    System.out.print("Do you want to continue anyway [y|N]? ");
                    String inputLine = null;
                    try {
                        BufferedReader inputString = new BufferedReader(new InputStreamReader(System.in));
                        inputLine = inputString.readLine();
                        if ((inputLine.length() == 0) || (!inputLine.toLowerCase().contains("y"))) {
                            System.out.println("Merge Aborted at user request!");
                            System.exit(EFAILURE);
                        } else {
                            spdFile = null;
                            System.out
                                      .println("Continuing with the Merge.  Don't forget to add the new MSA file to a Package.");
                        }
                    } catch (IOException e) {
                        System.out.println("IOException: " + e);
                    }
                }
            } else if (args[i].toLowerCase().contains("-u")) {
                i++;
                uiName = args[i];
            } else if (args[i].toLowerCase().contains("-o")) {
                i++;
                fileBasename = args[i];
            } else if (args[i].toLowerCase().contains("-v")) {
                VERBOSE++;
            } else if ((args[i].toLowerCase().contains("-h")) || (args[i].toLowerCase().contains("-?"))
                       || (args[i].toLowerCase().contains("/h")) || (args[i].toLowerCase().contains("--help"))) {
                outputUsage();
                System.exit(EFAILURE);
            } else {
                if (args[i].startsWith("-")) {
                    System.out.println("Invalid Argument: " + args[i]);
                    outputUsage();
                    System.out.println("Merge Aborted!");
                    System.exit(EFAILURE);
                }
                String leafFile = args[i];
                if (!leafFile.toLowerCase().contains(".msa"))
                    leafFile = leafFile + ".msa";

                if (testFile(leafFile) == NOTFOUND) {
                    System.out.println("ERROR: The Leaf MSA File: " + leafFile + " was NOT FOUND!");
                    System.out.println("Merge Aborted!");
                    System.exit(EFAILURE);
                } else {
                    if (DEBUG > 9)
                        System.out.println("Found Leaf Module:   " + leafFile);
                    leafFiles.add(leafFile);
                }
            }
        }
        if (testFile(targetFile) == FOUND) {
            System.out.println("WARNING: The targetfile: " + targetFile + "Already Exists!");
            System.out.print("Do you want to over write it [y|N]? ");
            String inputLine = null;
            try {
                BufferedReader inputString = new BufferedReader(new InputStreamReader(System.in));
                inputLine = inputString.readLine();
                if ((inputLine.length() == 0) || (!inputLine.toLowerCase().contains("y"))) {
                    System.out.println("Please correct the options, then try again.");
                    System.out.println("Merge Aborted at user request!");
                    System.exit(EFAILURE);
                }
            } catch (IOException e) {
                System.out.println("IOException: " + e);
            }

        }
        return ESUCCESS;
    }

    private static int testFile(String Filename) {
        File tFile = new File(Filename);
        if (DEBUG > 8)
            System.out.println("File is located: " + tFile.getPath());
        if (tFile.exists())
            return FOUND;
        else
            return NOTFOUND;
    }

    private static void outputUsage() {
        
        
        System.out.println("Merge, " + version);
        System.out.println(copyright);
        System.out.println("Usage:");
        System.out.println("  merge [-v] -t target [-u UiName] [-p PackageFile] dir1" + File.separator + "leaf1 ... dirN" + File.separator + "leafN [-h | -? | --help]");
        System.out.println("    where:");
        System.out.println("      -h | -? | --help            OPTIONAL - This Help Text");
        System.out.println("      -t Target                   REQUIRED - The Name of the new Merge Module MSA file");
        System.out.println("      -p Package                  OPTIONAL - The Name of the Package (SPD) file to add the target");
        System.out.println("      -u UiName                   OPTIONAL - The User Interface Name for the Target Module");
        System.out.println("      -v                          OPTIONAL - Verbose, print information messages.");
        System.out.println("      -o OutputFileBasename       OPTIONAL - Set the Output Filename for this module to Basename");
        System.out.println("      dir1" + File.separator + "leaf1 ... dirN" + File.separator + "leafN   REQUIRED The path to two or more MSA files that will be merged");
        System.out.println("");
    }
}
