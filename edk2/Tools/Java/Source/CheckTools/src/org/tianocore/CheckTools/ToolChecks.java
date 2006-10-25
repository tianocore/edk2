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

    private final int DUMP = 8;

    private final int INTERACTIVE = 16;

    private final int PASS = 0;

    private final int FAIL = 1;

    private ArrayList<String> errLog = new ArrayList<String>();

    private ArrayList<String> goodLog = new ArrayList<String>();

    private ArrayList<String> warnLog = new ArrayList<String>();

    public int checkTools(String toolConfFile, int cmdCode, int VERBOSE, boolean QUIET) {

        int returnCode = FAIL;
        boolean interActive = false;

        if ((DEBUG > 0) || (VERBOSE > 0)) {
            if ((cmdCode & DUMP) == DUMP)
                System.out.print("Using Tool Configuration File: " + toolConfFile + "  ");
            else
                System.out.println("Using Tool Configuration File: " + toolConfFile);
        }

        if (DEBUG > 2)
            System.out.println("The cmdCode: " + cmdCode);

        if ((cmdCode & INTERACTIVE) == INTERACTIVE) {
            interActive = true;
            if (QUIET == false)
                System.out.println("***** WARNING ***** The Interactive function has not been implemented yet!");
        }

        if ((cmdCode & DUMP) == DUMP) {
            returnCode = dumpFile(toolConfFile, VERBOSE, QUIET);
            if (DEBUG > 10)
                System.out.println("dumpFile returned: " + returnCode);
        }

        if ((cmdCode & SCAN) == SCAN) {
            returnCode = scanFile(toolConfFile, interActive, VERBOSE, QUIET);
        }

        if (((cmdCode & TEST) == TEST) || ((cmdCode & DEFAULT) == DEFAULT))
            returnCode = testFile(toolConfFile, interActive, VERBOSE, QUIET);

        if ((errLog.isEmpty() == false) && (QUIET == false)) {
            // We had ERRORS, not just Warnings.
            // System.out.println("Tool Configuration File: " + toolConfFile);
            System.out.println(" goodLog has: " + goodLog.size() + " entries");
            if ((goodLog.isEmpty() == false) && (VERBOSE > 1))
                for (int i = 0; i < goodLog.size(); i++)
                    System.out.println("Tool Chain Tag Name: " + goodLog.get(i) + " is valid!");
            for (int i = 0; i < errLog.size(); i++)
                System.out.println(errLog.get(i));
            if (warnLog.isEmpty() == false)
                for (int i = 0; i < warnLog.size(); i++)
                    System.out.println("  " + warnLog.get(i));
            if (VERBOSE > 0) {
                System.out.println();
                System.out.println("You can remove these WARNING messages by editing the file:");
                System.out.println("  " + toolConfFile);
                System.out.println("and commenting out out or deleting the entries for the tool");
                System.out.println("chain tag names that do not apply to your system.");
            }
        } else {
            if (QUIET == false) {
                if ((cmdCode & DUMP) == DUMP)
                    System.out.println("");
                if (VERBOSE > 0) {
                    System.out.print("Valid Tag Names:");
                    for (int i = 0; i < goodLog.size(); i++)
                        System.out.print(" " + goodLog.get(i));
                    System.out.println("");
                }
                if (warnLog.isEmpty() == false)
                    for (int i = 0; i < warnLog.size(); i++)
                        System.out.println("  " + warnLog.get(i));
                if (returnCode == 0)
                    if (warnLog.isEmpty())
                        System.out.println("  Tool Configuration File: " + toolConfFile + " is valid!");
                    else
                        System.out.println("  Tool Configuration File: " + toolConfFile
                                           + " is valid!  However, there are WARNINGS!");
                else
                    System.out.println("  Tool Configuration File: " + toolConfFile
                                       + " contains INVALID tool tag names!");
            }
        }

        return returnCode;
    }

    private int scanFile(String testFile, boolean interActive, int VERBOSE, boolean QUIET) {
        if ((DEBUG > 0) || (VERBOSE > 0))
            System.out.println("Scanning the Normal Installation Locations ...");
        System.out.println("The Scan function has not been implemented yet!");
        return FAIL;
    }

    private int dumpFile(String testFile, int VERBOSE, boolean QUIET) {
        int retCode = PASS;
        try {
            //
            // initialize local variables
            //
            ArrayList<ToolInfo> info = new ArrayList<ToolInfo>();
            String readLine = "";
            String fileLine[] = new String[2];
            String property[] = new String[5];
            ToolInfo toolInf = null;
            String lastTagName = "arf";
            String lastFamily = "arf";
            String lastArch = "IA32";
            String lastCmdCode = "arf";
            //            String lastFlags = "arf";
            boolean found = false;
            String tFamily = "arf";
            String tCmdName = "arf";
            String acpiPath = "";
            String cmdPath = "";
            String asmPath = "";
            int ctr = 0;
            //
            // Setup the reader
            //
            FileReader toolConfFile = new FileReader(testFile);
            BufferedReader bufReader = new BufferedReader(toolConfFile);

            while ((readLine = bufReader.readLine()) != null) {
                if (!readLine.startsWith("#")) {
                    if (readLine.contains("IDENTIFIER")) {
                        readLine = readLine.trim();
                        fileLine = readLine.split("=");
                        if (QUIET == false)
                            System.out.println("IDENTIFIER: " + fileLine[1].trim());
                        toolInf = new ToolInfo();
                        toolInf.setValid();
                    } else if (readLine.contains("_")) {
                        /**
                         * This section should complete array values
                         * String TagName
                         * String Family
                         * boolean Valid
                         * String Arch
                         * ArrayList<String> Targets
                         * ArrayList<String> CmdCode
                         * ArrayList<String> Path
                         * ArrayList<String> Arguments
                         */
                        if (DEBUG > 10)
                            System.out.println("Processing: " + readLine.trim());
                        readLine = readLine.trim();
                        readLine = readLine.replaceFirst("=", "_SPLIT_HERE_");
                        fileLine = readLine.split("_SPLIT_HERE_");
                        fileLine[0] = fileLine[0].trim();
                        fileLine[1] = fileLine[1].trim();
                        property = fileLine[0].split("_");

                        // Covert to simple string names
                        String tTarget = property[0].trim();
                        if (tTarget.contentEquals("*"))
                            tTarget = "All Targets";
                        String tTag = property[1].trim();
                        String tArch = property[2].trim();
                        String tCmdCode = property[3].trim();
                        String tArg = property[4].trim();
                        if (tArg.contentEquals("FAMILY"))
                            tFamily = fileLine[1].trim();
                        if (tArg.contentEquals("NAME")) {
                            tCmdName = fileLine[1].trim();
                            tCmdCode = tCmdCode + " (" + tCmdName + ")";
                        }

                        String tVal = fileLine[1].trim();

                        // Process the TagName
                        if ((!tTag.contentEquals(lastTagName))
                            || ((!tArch.contentEquals(lastArch)) && (!tArch.contentEquals("*")))
                            || (!tFamily.contentEquals(lastFamily))) {
                            if (DEBUG > 10) {
                                System.out.println("  LAST Tag: " + lastTagName + " Arch: " + lastArch + " Family: "
                                                   + lastFamily);
                                System.out.println("  NEXT Tag: " + tTag + " Arch: " + tArch + " Family: " + tFamily);
                            }
                            if ((!lastTagName.equals("arf")) && (!tTag.equals("*"))) {
                                toolInf.setTagName(lastTagName);
                                toolInf.setFamily(lastFamily);
                                toolInf.setArch(lastArch);
                                if (toolInf.getCmdCode().size() < 1)
                                    toolInf.addCmdCode(lastCmdCode);
                                info.add(toolInf);
                                toolInf = new ToolInfo();
                                toolInf.setValid();
                                if (DEBUG > 3)
                                    System.out.println("  ADDED " + ctr + " Tag: " + lastTagName + " Arch: " + lastArch
                                                       + " Family: " + lastFamily + " CmdCode: " + lastCmdCode);
                                ctr++;

                            }

                            if ((!tTag.contentEquals("*")) && (!tTag.contentEquals(lastTagName)))
                                lastTagName = tTag;

                            if ((!tArch.contentEquals(lastArch)) && (!tArch.contentEquals("*")))
                                lastArch = tArch;
                            if ((!tArch.contentEquals(lastArch)) && (tArch.contentEquals("*")))
                                lastArch = "IA32";

                            if (!tFamily.contentEquals(lastFamily))
                                lastFamily = tFamily;
                            if (DEBUG > 10)
                                System.out.println("    Setting Tag: " + lastTagName + " Arch: " + lastArch
                                                   + " Family: " + lastFamily);
                        }

                        // Set the Arch
                        if ((!lastArch.contentEquals(tArch)) && (!tArch.contentEquals("*"))) {
                            toolInf.setArch(tArch);
                            lastArch = tArch;
                            if (DEBUG > 10)
                                System.out.println("    Setting Arch: " + tArch);
                        }

                        // Process Target field - making sure we add only unique values.
                        if (!tTarget.contains("*")) {
                            found = false;
                            for (int k = 0; k < toolInf.getTargetName().size(); k++)
                                if (toolInf.getTargetName(k).contentEquals(tTarget))
                                    found = true;
                            if (!found) {
                                toolInf.addTargetName(tTarget);
                                if (DEBUG > 10)
                                    System.out.println("    Adding Target: " + tTarget);
                            }
                        }

                        // Process Command Code Field - making sure we add only unique values.
                        if (!tCmdCode.contentEquals("*")) {
                            found = false;
                            for (int k = 0; k < toolInf.getCmdCode().size(); k++)
                                if (toolInf.getCmdCode(k).startsWith(tCmdCode))
                                    found = true;
                            if (!found) {
                                if (!tCmdCode.contains(" (")) {
                                    boolean nf = true;
                                    for (int m = toolInf.size(); m >= 0; --m) {
                                        if (nf) {
                                            ArrayList<String> lastCmdEntry = info.get(m).getCmdCode();
                                            for (int l = 0; l < lastCmdEntry.size(); l++) {
                                                if (lastCmdEntry.get(l).startsWith(tCmdCode)) {
                                                    tCmdCode = lastCmdEntry.get(l).trim();
                                                    if (DEBUG > 20)
                                                        System.out.println("found tCmdCode here: " + tCmdCode);
                                                    nf = false;
                                                }
                                            }
                                        }
                                    }
                                    if (nf == false) {
                                        toolInf.addCmdCode(tCmdCode);
                                        if (DEBUG > 10)
                                            System.out.println("    Adding previous CmdCode: " + tCmdCode);
                                    }
                                } else {
                                    toolInf.addCmdCode(tCmdCode);
                                    lastCmdCode = tCmdCode;
                                    if (DEBUG > 10)
                                        System.out.println("    Adding first CmdCode: " + tCmdCode);
                                }
                            }
                        }

                        // Process Path Field - making sure we add only unique values.
                        if (tArg.contentEquals("PATH")) {
                            String prefix = "PATH_";
                            if (tCmdCode.contentEquals("ASM"))
                                prefix = "ASMPATH_";
                            if (tCmdCode.contentEquals("ASMLINK"))
                                prefix = "ALPATH_";
                            if (tCmdCode.contentEquals("ASL"))
                                prefix = "ASLPATH_";
                            File path = new File(tVal);
                            found = false;
                            if (path.exists()) {
                                for (int k = 0; k < toolInf.getPath().size(); k++)
                                    if (toolInf.getPath(k).startsWith(prefix))
                                        found = true;
                                if (found == false) {
                                    toolInf.addPath(prefix + tVal);
                                    if (DEBUG > 10)
                                        System.out.println("    Adding valid path: " + tVal);
                                }
                                if (prefix.contentEquals("ASLPATH_"))
                                    acpiPath = tVal;
                                if (prefix.contentEquals("PATH_"))
                                    cmdPath = tVal;
                                if (prefix.contentEquals("ASMPATH_"))
                                    asmPath = tVal;
                            } else {
                                toolInf.setInvalid();
                                for (int k = 0; k < toolInf.getBadPath().size(); k++)
                                    if (toolInf.getBadPath(k).startsWith(prefix))
                                        found = true;
                                if (!found) {
                                    toolInf.addBadPath(prefix + tVal);
                                    if (DEBUG > 10)
                                        System.out.println("    Adding NOT valid Path: " + tVal);
                                    retCode = FAIL;
                                }
                            }
                        }

                        if (tArg.contentEquals("DPATH")) {
                            found = false;
                            File path = new File(tVal);
                            if (path.exists()) {
                                for (int k = 0; k < toolInf.getPath().size(); k++)
                                    if (toolInf.getPath(k).startsWith("DPATH_"))
                                        found = true;
                                if (!found) {
                                    toolInf.addPath("DPATH_" + tVal);
                                    if (DEBUG > 10)
                                        System.out.println("    Adding valid DPath: " + tVal);
                                }
                            } else {
                                toolInf.setInvalid();
                                for (int k = 0; k < toolInf.getBadPath().size(); k++)
                                    if (toolInf.getBadPath(k).contentEquals("DPATH_" + tVal))
                                        found = true;
                                if (!found) {
                                    toolInf.addBadPath("DPATH_" + tVal);
                                    if (DEBUG > 10)
                                        System.out.println("    Adding NOT valid DPath: " + tVal);
                                    retCode = FAIL;
                                }
                            }
                        }

                        if (tArg.contentEquals("SPATH")) {
                            found = false;
                            File path = new File(tVal);
                            if (path.exists()) {
                                for (int k = 0; k < toolInf.getPath().size(); k++)
                                    if (toolInf.getPath(k).contentEquals("SPATH_" + tVal))
                                        found = true;
                                if (!found) {
                                    toolInf.addPath("SPATH_" + tVal);
                                    if (DEBUG > 10)
                                        System.out.println("    Adding valid SPath: " + tVal);
                                }
                            } else {
                                toolInf.setInvalid();
                                for (int k = 0; k < toolInf.getBadPath().size(); k++)
                                    if (toolInf.getBadPath(k).contentEquals("SPATH_" + tVal))
                                        found = true;
                                if (!found) {
                                    toolInf.addBadPath("SPATH_" + tVal);
                                    if (DEBUG > 10)
                                        System.out.println("    Adding NOT valid SPath: " + tVal);
                                    retCode = FAIL;
                                }
                            }
                        }

                        if (tArg.equals("INCLUDEPATH")) {
                            found = false;
                            File path = new File(tVal);
                            if (path.exists()) {
                                for (int k = 0; k < toolInf.getPath().size(); k++)
                                    if (toolInf.getPath(k).contentEquals("INCLUDEPATH_" + tVal))
                                        found = true;
                                if (!found) {
                                    toolInf.addPath("INCLUDEPATH_" + tVal);
                                    if (DEBUG > 10)
                                        System.out.println("    Adding valid IPath: " + tVal);
                                }
                            } else {
                                toolInf.setInvalid();
                                for (int k = 0; k < toolInf.getBadPath().size(); k++)
                                    if (toolInf.getBadPath(k).contentEquals("INCLUDE_" + tVal))
                                        found = true;
                                if (!found) {
                                    toolInf.addBadPath("INCLUDE_" + tVal);
                                    if (DEBUG > 10)
                                        System.out.println("    Adding NOT valid IPath: " + tVal);
                                    retCode = FAIL;
                                }
                            }
                        }

                        if (tArg.equals("LIBPATH")) {
                            found = false;
                            File path = new File(tVal);
                            if (path.exists()) {
                                for (int k = 0; k < toolInf.getPath().size(); k++)
                                    if (toolInf.getPath(k).contentEquals("LIB_" + tVal))
                                        found = true;
                                if (!found) {
                                    toolInf.addPath("LIB_" + tVal);
                                    if (DEBUG > 10)
                                        System.out.println("    Adding valid LPath: " + tVal);
                                }
                            } else {
                                toolInf.setInvalid();
                                for (int k = 0; k < toolInf.getBadPath().size(); k++)
                                    if (toolInf.getBadPath(k).contentEquals("LIB_" + tVal))
                                        found = true;
                                if (!found) {
                                    toolInf.addBadPath("LIB_" + tVal);
                                    if (DEBUG > 10)
                                        System.out.println("    Adding NOT valid LPath: " + tVal);
                                    retCode = FAIL;
                                }
                            }
                        }

                        if (tArg.equals("FLAGS")) {
                            String flags = fileLine[1].trim();
                            String fLine = tTarget + "_" + tCmdCode + "_FLAGS_" + flags;
                            toolInf.addArguments(fLine);
                            if (DEBUG > 10)
                                System.out.println("    Adding flag line: " + fLine);
                        }
                    }
                }
            }
            toolInf.setArch(lastArch);
            toolInf.setFamily(lastFamily);
            toolInf.setTagName(lastTagName);
            info.add(toolInf);
            if (DEBUG > 2)
                System.out.println("  ADDED " + ctr + " LAST Tag: " + lastTagName + " Arch: " + lastArch + " Family: "
                                   + lastFamily + " Last CmdCode: " + lastCmdCode);
            //
            // Finished collecting data, print it out in Human Readable Format
            if (info.size() > 0) {
                if (QUIET == false)
                    System.out.println(" **** TOOL CHAINS ****");
                if ((DEBUG > 0) || (VERBOSE > 1))
                    System.out.println("There are " + info.size()
                                       + " different combinations of Tag Names and Architectures.");
                lastTagName = "arf";

                boolean asmExists = false;
                boolean asmPathDefined = false;

                boolean cmdPathDefined = false;
                boolean dPathDefined = false;

                boolean acpiPathDefined = false;
                boolean acpiDeclared = false;
                ArrayList<String> warnMsgs = null;

                for (int i = 0; i < info.size(); i++) {
                    if (!lastTagName.contentEquals(info.get(i).getTagName())) {
                        if ((acpiDeclared == false) && (!lastTagName.contentEquals("arf"))) {
                            if (QUIET == false) {
                                System.out
                                          .println("  ----WARNING: No ACPI assembler has been defined for "
                                                   + lastTagName
                                                   + ", you will not be able to successfully create a complete firmware image.");
                                System.out.println("");
                            }
                        }
                        if ((asmExists == false) && (!lastTagName.contentEquals("arf"))) {
                            if (QUIET == false) {
                                System.out.println("  ----WARNING: No Assembler has been defined for " + lastTagName
                                                   + ", you will not be able to successfully compile all platforms.");
                                System.out.println("");
                            }
                        }
                        asmPath = "";
                        asmPathDefined = false;
                        cmdPath = "";
                        cmdPathDefined = false;
                        dPathDefined = false;
                        acpiPath = "";
                        acpiPathDefined = false;
                        acpiDeclared = false;
                        warnMsgs = new ArrayList<String>();
                        lastTagName = info.get(i).getTagName();
                    }

                    if ((DEBUG > 0) || (VERBOSE > 1))
                        System.out.print("Tool Chain " + i + " Tag Name: " + info.get(i).getTagName() + ", Family: "
                                         + info.get(i).getFamily());
                    else if (QUIET == false)
                        System.out.print("Tool Chain Tag Name: " + info.get(i).getTagName() + ", Family: "
                                         + info.get(i).getFamily());

                    if (info.get(i).isValid()) {
                        ToolInfo vTool = info.get(i);
                        if (QUIET == false)
                            System.out.println(" is Valid for Architecture: " + info.get(i).getArch());
                        // OK, let's print out the command w/flags

                        for (int m = 0; m < vTool.getPath().size(); m++) {
                            String pathName[] = vTool.getPath(m).split("_");
                            String prefix = "  Tool Path: ";
                            if (pathName[0].startsWith("DPATH")) {
                                prefix = "  Dynamic Linker Path: ";
                                dPathDefined = true;
                            }
                            if (pathName[0].startsWith("PATH")) {
                                cmdPathDefined = true;
                                cmdPath = pathName[1].trim();
                            }
                            if (pathName[0].startsWith("APATH")) {
                                asmPathDefined = true;
                                asmPath = pathName[1].trim();
                            }
                            if (pathName[0].startsWith("ASLPATH")) {
                                acpiPathDefined = true;
                                acpiPath = pathName[1].trim();
                            }
                            if (VERBOSE > 0)
                                System.out.println(prefix + pathName[1].trim());
                        }
                        if (acpiPathDefined == false) {
                            if (VERBOSE > 0)
                                warnMsgs.add("  ---- WARNING: " + vTool.getTagName() + "ACPI Path is not defined!");
                        }
                        if ((VERBOSE == 0) && (QUIET == false))
                            System.out.print("Defined Targets: ");
                        for (int k = 0; k < vTool.getTargetName().size(); k++) {
                            String tTarget = vTool.getTargetName(k);
                            if (tTarget.contentEquals("*"))
                                break;

                            if (VERBOSE > 0)
                                System.out.println("+++ " + tTarget + " +++");
                            else if (QUIET == false)
                                System.out.print(" " + tTarget);

                            for (int j = 0; j < vTool.getCmdCode().size(); j++) {
                                String tCommand = vTool.getCmdCode(j);
                                if (tCommand.contains("ASL ")) {
                                    acpiDeclared = true;
                                    if (acpiPathDefined) {
                                        // TODO: Check the acpi tool path
                                    }
                                }
                                if (vTool.getArguments().size() > 1) {

                                    if (!cmdPathDefined) {
                                        warnMsgs.add("  ---- ERROR: Command Path is not defined!");
                                    }
                                    if ((vTool.getFamily().contentEquals("MSFT")) && (!dPathDefined)) {
                                        warnMsgs
                                                .add("  ---- ERROR: Microsoft Tool Chains require a Path (DPATH) to the mspdbXX.dll file");
                                    }

                                    // Process Flag lines!
                                    for (int n = 0; n < vTool.getArguments().size(); n++) {
                                        String tFlags = vTool.getArguments(n);
                                        String aFlags[] = new String[2];
                                        aFlags = tFlags.split("FLAGS_");
                                        String tsCmd[] = tCommand.split(" ");
                                        tsCmd[0] = tsCmd[0].trim();
                                        if (DEBUG > 10)
                                            System.out.println(" tCmd: " + tsCmd[0] + ", aFlags[0]: " + aFlags[0]
                                                               + ", tTarget: " + tTarget);
                                        if (aFlags[0].contains(tsCmd[0])) {
                                            if ((tFlags.contains("All")) || (aFlags[0].contains(tTarget))) {
                                                String flagLine = aFlags[1].trim();
                                                if (info.get(i).getFamily().contentEquals("MSFT")) {
                                                    if (tCommand.startsWith("PP")) {
                                                        if (QUIET == false)
                                                            if ((!flagLine.contains("/P "))
                                                                && (!flagLine.contains("/E "))) {
                                                                System.out
                                                                          .println("  **** ERROR ****  Pre-Processor Flags are NOT VALID!");
                                                            }
                                                    }
                                                }
                                                String cmdExe = "";

                                                if (tCommand.contains("ASM ")) {
                                                    cmdExe = tCommand.replace("ASM (", "");
                                                    cmdExe = cmdExe.replace(")", "");
                                                    if (asmPathDefined)
                                                        cmdExe = asmPath.trim() + File.separator + cmdExe.trim();
                                                    else
                                                        cmdExe = cmdPath.trim() + File.separator + cmdExe.trim();
                                                    File testCmd = new File(cmdExe);
                                                    // System.out.println("Check: " + cmdExe);
                                                    if (!testCmd.exists()) {
                                                        String errMsg = "  **** ERROR ****  Assembler Command Defined, but does not exist! "
                                                                        + cmdExe;
                                                        if (VERBOSE > 0) {
                                                            System.out.println(errMsg);
                                                        }
                                                        boolean nf = true;
                                                        for (int r = 0; r < warnMsgs.size(); r++)
                                                            if (warnMsgs.get(r).contentEquals(errMsg))
                                                                nf = false;
                                                        if (nf)
                                                            warnMsgs.add(errMsg);
                                                    } else
                                                        asmExists = true;

                                                }

                                                if ((tCommand.contains("ASM ")) && (asmPathDefined == false)
                                                    && (VERBOSE > 0))
                                                    System.out
                                                              .println("  --- WARNING: Assembler - "
                                                                       + tCommand
                                                                       + " defined, but a Separate Path to the Assembler was not defined, using: "
                                                                       + cmdExe);
                                                if ((asmPathDefined) && (tCommand.contains("ASM "))
                                                    && (asmPathDefined == false) && (VERBOSE == 0)) {
                                                    String msgIs = "  --- WARNING: "
                                                                   + info.get(i).getTagName()
                                                                   + " Assembler "
                                                                   + info.get(i).getArch()
                                                                   + " - "
                                                                   + tCommand
                                                                   + " defined, but a Separate Path to the Assembler was not defined, using: "
                                                                   + cmdExe;
                                                    boolean nf = true;
                                                    for (int r = 0; r < warnMsgs.size(); r++)
                                                        if (warnMsgs.get(r).contentEquals(msgIs))
                                                            nf = false;
                                                    if (nf)
                                                        warnMsgs.add(msgIs);
                                                }
                                                if (VERBOSE > 0)
                                                    System.out.println("  " + tCommand + " " + flagLine);
                                            }
                                        }
                                    }
                                } else {
                                    if (VERBOSE > 0)
                                        System.out.println("  " + tCommand + " has no arguments defined.");
                                }
                            }
                        }

                    } else {
                        if (QUIET == false)
                            System.out.println(" is NOT VALID for Architecture: " + info.get(i).getArch());
                        retCode = FAIL;
                        for (int k = 0; k < info.get(i).getBadPath().size(); k++) {
                            String failureString = "";
                            String tempString = info.get(i).getBadPath(k);
                            if (tempString.startsWith("PATH_"))
                                failureString = "Common Command Path:  " + tempString.replace("PATH_", "")
                                                + " is NOT Valid!";
                            if (tempString.startsWith("SPATH"))
                                failureString = "Static Linker Path:   " + tempString.replace("SPATH_", "")
                                                + " is NOT valid!";
                            if (tempString.startsWith("DPATH"))
                                failureString = "Dynamic Linker Path:  " + tempString.replace("DPATH_", "")
                                                + " is NOT valid!";
                            if (tempString.startsWith("LIB_"))
                                failureString = "System Lib Path:      " + tempString.replace("LIB_", "")
                                                + " is NOT valid!";
                            if (tempString.startsWith("INCLUDEPATH_"))
                                failureString = "System include Path:  " + tempString.replace("INCLUDEPATH_", "")
                                                + " is NOT valid!";
                            if (tempString.startsWith("APATH"))
                                failureString = "Assembler Path:       " + tempString.replace("APATH_", "")
                                                + " is NOT valid!";
                            if (tempString.startsWith("ALPATH"))
                                failureString = "Assembler Link Path:  " + tempString.replace("ALPATH_", "")
                                                + " is NOT valid!";
                            if (tempString.startsWith("ASLPATH"))
                                failureString = "ACPI Path:            " + tempString.replace("ASLPATH_", "")
                                                + " is NOT valid!";
                            if (QUIET == false)
                                System.out.println("    --- " + failureString);
                        }

                    }

                    if ((VERBOSE == 0) && (QUIET == false)) {
                        System.out.println("");
                        for (int p = 0; p < warnMsgs.size(); p++)
                            System.out.println(warnMsgs.get(p));
                    }
                    if (QUIET == false)
                        System.out.println("");
                }

            }

        } catch (IOException e) {
            if (QUIET == false)
                System.out.println("I/O Failure: " + e);
            retCode = FAIL;
        }

        return retCode;
    }

    private int testFile(String testFile, boolean interActive, int VERBOSE, boolean QUIET) {

        int retCode = PASS;
        String readLine = "";
        String fileLine[] = new String[2];
        boolean definedAsl = false;

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
                    String tagName = props[1].trim();
                    // System.out.println(" Testing TagName: " + tagName);
                    String cmdTag = props[3].trim();
                    if (!lastTag.contentEquals(tagName)) {
                        // we have a new tagName
                        // first past lastTag = barf, lastErrTag = barf, tagName = something other than BARF
                        if (DEBUG > 0)
                            System.out.println(" lastTag: " + lastTag + " this Tag: " + tagName + " lastErrTag: "
                                               + lastErrTag);

                        if ((definedAsl == false) && (lastTag.contentEquals("barf") == false))
                            warnLog.add("  -- WARNING: Tool Chain Tag Name: " + lastTag
                                        + " does not have an ACPI assembler defined!");
                        definedAsl = false;

                        if ((lastErrTag.contentEquals("barf")) || (!tagName.contentEquals(lastErrTag))) {
                            if ((DEBUG > 2) || (VERBOSE > 3))
                                System.out.println("Adding tag: " + tagName + " to the goodLog");
                            goodLog.add(tagName);
                        }

                        lastTag = tagName;

                    } // end of adding the new tag name to the goodLog and 
                    // checking if we had an ACPI compiler defined for the TagName
                    File testPath = new File(path);
                    if (cmdTag.contentEquals("ASL"))
                        definedAsl = true;
                    if (!testPath.exists()) {
                        if (VERBOSE > 0)
                            System.out.println(" Invalid path: " + path + ", path does not exist!");
                        if (!tagName.contentEquals(lastErrTag)) {
                            errLog.add("  -- WARNING: Tool Chain Tag Name: " + tagName + " is NOT valid!");
                            errLog.add("         Tool Code: [" + cmdTag + "] Path: " + path + " does not exist!");
                        }
                        retCode = 1;
                        lastErrTag = tagName;

                    } else {

                        if ((DEBUG > 0) || (VERBOSE > 0)) {
                            if ((!tagName.contentEquals(lastTag)) && (!tagName.contentEquals(lastErrTag))
                                && (!lastTag.contentEquals("barf"))) {
                                String goodMsg = "Tool Chain: " + tagName + " is valid";
                                System.out.println(goodMsg);
                            }
                        }
                    } // end of path existence check
                } // end of parsing the PATH line
            } // end of reading file
            // Check the LAST Tag to see of ACPI has been defined.
            if (definedAsl == false)
                warnLog.add("  -- WARNING: Tool Chain Tag Name: " + lastTag
                            + " does not have an ACPI assembler defined!");
            // CLEAN UP
            toolConfFile.close();
        } catch (IOException e) {
            if (QUIET == false)
                System.out.println(" [" + testFile + "] " + e);
            System.exit(FAIL);
        } // end of TRY-CATCH for reading file
        
        if ((errLog.isEmpty() == false) && (QUIET == false))
            for (int i = 0; i < goodLog.size(); i++) {
                for (int j = 0; j < errLog.size(); j++) {
                    System.out.println(" Error Log [" + j + "] is " + errLog.get(j));
                    if (errLog.get(j).contains(goodLog.get(i).trim())) {
                        if (VERBOSE > 1)
                            System.out.println(" Removing: " + goodLog.get(i).trim());
                        goodLog.remove(i);
                        break;
                    }
                }
            }
        if (DEBUG > 0)
            System.out.println("Returning with goodLog having: " + goodLog.size());
        return retCode;

    }

}
