// @file
//  DbCmds command-line interface to the classes that 
//  update the FrameworkDatabase.db file based on WORKSPACE Contents
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
//      -t, --test  Test the Workspace against the FrameworkDatabase.db file
//                    Returns 0 if valid, returns 1 if failed
//
//      -f, --fix   Fix the FrameworkDatabase.db file, so that it matches the
//                    the contents of the WORKSPACE
//                    Returns 0 if valid, returns 1 if failed
//
//      -v [-v]     Verbose Flag - sum of these will be used to set different
//                    levels of verbosity
//
//      -i          Interactive, when used with -f or --fix, will query the user
//                    regarding adds and deletes of packages and platforms from
//                    the database.
//
//      -a, --add   Add an SPD or FPD file to the FrameworkDatabase.db file.  The
//                    SPD/FPD file must exist, or the command will fail.
//                    Returns 0 if valid, returns 1 if failed
//
//      -r, --del   Remove an SPD or FPD file from the FrameworkDatabase.db file.
//                    If the SPD/FPD file exists, the user will be queried to
//                    remove it from the directory tree.  For SPD files, the user
//                    will also be presented with a list of Modules in the SPD
//                    file, with the query to remove the modules as well as the
//                    SPD file.
//                    Returns 0 if valid, returns 1 if failed
//
//      -u          Display the UiName for all Packages and Platforms currently in
//                    the FrameworkDatabase.db file.
//                    Returns 0 if valid, returns 1 if failed
//
//      -c          Display a CSV listing of Type (SPD|FPD) UiName and Filename of
//                    every entry in the FrameworkDatabase.db file.
//                    Returns 0 if valid, returns 1 if failed
//
//    No Options    Display a list of Type (Package|Platfrom) and Filename of every
//                    entry in the FrameworkDatabase.db file.
//                    Returns 0 if valid, returns 1 if failed
//
//    -h, -?, --help  Displays this usage and exits.
//                    Returns 0 if valid, returns 1 if failed
//
//
//  Output:
//      Displayed information
//
//  Modifies - OPTIONAL
//      FrameworkDatabase.db
//

package org.tianocore.DbTools;

import java.io.*;

public class DbCmds {

    protected enum Cmd {
        SHOW_CSV, SHOW_PLATFORMS, SHOW_PACKAGES, FIX_DB, TEST_DB, FIND_SOMETHING, SHOW_FAR, ADD_SOMETHING,
        DELETE_SOMETHING
    }

    private int DEBUG = 0;

    private static final String copyright = "Copyright (c) 2006, Intel Corporation     All rights reserved.";

    private static final String version = "Version 0.1";

    private static final String Specification = "FRAMEWORK_BUILD_PACKAGING_SPECIFICATION   0x00000052";

    private int VERBOSE = 0;

    public boolean INTERACTIVE = false;

    private String workspace = System.getenv("WORKSPACE");

    private final String frameworkDatabase = workspace + File.separator + "Tools" + File.separator + "Conf"
                                             + File.separator + "FrameworkDatabase.db";

    private final int ESUCCESS = 0;

    private final int EFAILURE = 1;

    private final int MIN_VERBOSE = 0;

    private final int MED_VERBOSE = 1;

    private final int MAX_VERBOSE = 2;

    private final static int FOUND = 1;

    private final static int NOTFOUND = 0;

    private boolean TEST = false;

    private int result = ESUCCESS;

    private Cmd commandToCall;

    private boolean isPlatform = false;

    private boolean isPackage = false;

    private boolean isFar = false;

    private boolean QUIET = false;

    private String commandArgs = "";

    private String whatToFind = "";

    public int DbUpdateCmdLine(String[] args) {

        if (testFile(frameworkDatabase) == NOTFOUND) {
            System.out.println("ERROR: E000  Invalid Workspace!");
            System.out.println("The environment variable, WORKSPACE, does not point to a valid workspace");
            System.out.println("DbUpdate Aborted!");
            System.err.flush();
            System.exit(EFAILURE);
        }
        result = parseCmdLine(args);
        if (result == ESUCCESS) {
            if (DEBUG > 2)
                System.out.println("Parse Succeeded!");
            if (VERBOSE > MAX_VERBOSE)
                System.out.println("WORKSPACE: " + workspace);
            if (DEBUG > 1)
                System.out.println("Command to call: " + commandToCall.toString());
            result = processCmdLine(commandToCall);
        } else {
            if (QUIET == false) {
                System.out.println("Invalid Arguments");
                outputUsage();
                result = EFAILURE;
            }
        }
        if (DEBUG > 2)
            System.out.println(" Result: " + result);

        return result;
    }

    private int processCmdLine(Cmd cmdCode) {
        UpdateDb dbUp = new UpdateDb();
        int res = ESUCCESS;
        switch (cmdCode) {
        case SHOW_CSV:
            // display current Database contents in CSV format
            // Modifies: NOTHING
            if ((DEBUG > 3) || (VERBOSE > MED_VERBOSE))
                System.out.println("Display contents of the FrameworkDatabase.db file in CSV format.");
            result = dbUp.getCsvEntries(frameworkDatabase, VERBOSE);
            break;
        case FIX_DB:
            // Automatically make the database match the contents of the workspace
            // Modifies: FrameworkDatabase.db
            if ((VERBOSE > MIN_VERBOSE) && (QUIET == false)) {
                if (TEST == false)
                    System.out.println("Adjusting the FrameworkDatabase to match the contents of the WORKSPACE: "
                                       + workspace);
                if (TEST)
                    System.out.println("Verify the FrameworkDatabase matches the contents of the WORKSPACE: "
                                       + workspace);
            }
            if ((DEBUG > 3) || (VERBOSE > MED_VERBOSE))
                System.out.println("Scan the Workspace and update the FrameworkDatabase.db file.");
            result = dbUp.fixDatabase(workspace, VERBOSE, INTERACTIVE, TEST, QUIET);

            break;
        case ADD_SOMETHING:
            // Add a Platform, package or FAR to the workspace.
            // Modifies: FrameworkDatabase.db
            if (((DEBUG > 3) || (VERBOSE > MED_VERBOSE)) && (isPlatform))
                System.out.println("Add Platform " + commandArgs + " to the FrameworkDatabase.db file");
            else if (((DEBUG > 3) || (VERBOSE > MED_VERBOSE)) && (isPackage))
                System.out.println("Add Package " + commandArgs + " to the FrameworkDatabase.db file");
            else if (((DEBUG > 3) || (VERBOSE > MED_VERBOSE)) && (isFar))
                System.out.println("Add Framework Archive " + commandArgs + " to the FrameworkDatabase.db file");
            result = dbUp.addItem(frameworkDatabase, commandArgs, VERBOSE, INTERACTIVE);
            break;
        case DELETE_SOMETHING:
            // Remove a platform, package or FAR from the workspace
            // Modifies: FrameworkDatabase.db, AND Filesystem
            if (((DEBUG > 3) || (VERBOSE > MED_VERBOSE)) && (isPlatform))
                System.out.println("Removing Platform " + commandArgs + " from the FrameworkDatabase.db file");
            else if (((DEBUG > 3) || (VERBOSE > MED_VERBOSE)) && (isPackage))
                System.out.println("Removing Package " + commandArgs + " from the FrameworkDatabase.db file");
            else if (((DEBUG > 3) || (VERBOSE > MED_VERBOSE)) && (isFar))
                System.out.println("Removing Framework Archive " + commandArgs + " from the FrameworkDatabase.db file");
            result = dbUp.addItem(workspace, commandArgs, VERBOSE, INTERACTIVE);
            break;
        case FIND_SOMETHING:
            // Find something in the workspace
            // Modifies: NOTHING
            if ((DEBUG > 3) || (VERBOSE > MED_VERBOSE))
                System.out.println("Finding " + whatToFind + " " + commandArgs + " in the Workspace");
            result = dbUp.findItem(workspace, VERBOSE, whatToFind, commandArgs);
            break;
        case SHOW_FAR:
            // Display FAR information for all SPDs in the workspace
            // Modifies: NOTHING
            if ((DEBUG > 3) || (VERBOSE > MED_VERBOSE))
                System.out.println("Display Framework Archives in the Workspace");
            result = dbUp.findFars(workspace, VERBOSE);
            break;
        case SHOW_PACKAGES:
            // Display SPD information for all SPDs in the workspace
            // Modifies: NOTHING
            if ((DEBUG > 3) || (VERBOSE > MED_VERBOSE))
                System.out.println("Display Packages in the Workspace");
            result = dbUp.findSpds(workspace, VERBOSE);
            break;
        case SHOW_PLATFORMS:
            // Display FPD information for all SPDs in the workspace
            // Modifies: NOTHING
            if ((DEBUG > 3) || (VERBOSE > MED_VERBOSE))
                System.out.println("Display Platforms in the Workspace");
            result = dbUp.findFpds(workspace, VERBOSE);
            break;
        default:
            // ERROR IF WE GET HERE!
            if ((DEBUG > 3) || (VERBOSE > MAX_VERBOSE))
                System.out.println("We could not process the following: " + commandToCall.toString());
            else if (QUIET == false)
                outputUsage();
            result = EFAILURE;
            break;
        }
        return res;
    }

    private int parseCmdLine(String[] args) {

        // Default is to fix the database.
        commandToCall = Cmd.FIX_DB;
        if (args.length == NOTFOUND) {
            if ((DEBUG > 3) || (VERBOSE > MAX_VERBOSE))
                System.out.println("NO ARGUMENTS! " + commandToCall.toString());
            return (ESUCCESS);
        }

        for (int i = 0; i < args.length; i++)
            if (args[i].toLowerCase().contentEquals("-q"))
                QUIET = true;

        for (int i = 0; i < args.length; i++) {
            String arg = args[i].trim();
            // This is the list of valid options
            if (!((arg.toLowerCase().startsWith("-t")) || (arg.toLowerCase().startsWith("--t"))
                  || (arg.toLowerCase().startsWith("-q")) || (arg.toLowerCase().startsWith("-i"))
                  || (arg.toLowerCase().startsWith("-f")) || (arg.toLowerCase().startsWith("--f"))
                  || (arg.toLowerCase().startsWith("-c")) || (arg.toLowerCase().startsWith("--c"))
                  || (arg.toLowerCase().startsWith("-a")) || (arg.toLowerCase().startsWith("--a"))
                  || (arg.toLowerCase().startsWith("-r")) || (arg.toLowerCase().startsWith("--d"))
                  || (arg.toLowerCase().startsWith("-v")) || (arg.toLowerCase().startsWith("--c"))
                  || (arg.toLowerCase().startsWith("--p")) || (arg.toLowerCase().startsWith("-h"))
                  || (arg.toLowerCase().startsWith("--h")) || (arg.toLowerCase().startsWith("/h")) || (arg
                                                                                                          .toLowerCase()
                                                                                                                        .startsWith("-?")))) {
                // This is the fall through, we got something we did not know how to
                // process!
                if (args[i].startsWith("-")) {
                    System.out.println("ERROR: E1004  Unknown Option: " + arg);
                    System.out.println("Try running with -h or --help");
                } else {
                    System.out.println("ERROR: E1005  Unknown Argument: " + arg);
                    System.out.println("Try running with -h or --help");
                }
                System.out.println("Program Aborted!");
                System.err.flush();
                System.exit(EFAILURE);
            }
            if ((arg.toLowerCase().contentEquals("-t")) || (arg.toLowerCase().contains("--test"))) {
                // Test Workspace, do not fix.
                TEST = true;
            }
            if (arg.toLowerCase().contentEquals("-q")) {
                QUIET = true;
            }
            if (arg.toLowerCase().contentEquals("-i")) {
                INTERACTIVE = true;
            }
            if ((arg.toLowerCase().contentEquals("-f")) || (arg.toLowerCase().contains("--fix"))) {
                // Non-interactive fix of the database
                commandToCall = Cmd.FIX_DB;
            }
            if ((arg.toLowerCase().contentEquals("-c")) || (arg.toLowerCase().contains("--csv"))) {
                // Dump database in CSV format
                commandToCall = Cmd.SHOW_CSV;
            }
            if ((arg.toLowerCase().trim().contentEquals("-a")) || (arg.toLowerCase().contains("--add"))) {
                i++;
                if (args[i].startsWith("-")) {
                    System.out.println("ERROR: E002 Missing Argument!");
                    System.out
                              .println("The add function requires an argument, either a package name or a platform name!");
                    System.out.println("DbUpdate Aborted!");
                    System.err.flush();
                    System.exit(EFAILURE);
                }
                commandToCall = Cmd.ADD_SOMETHING;
                commandArgs = args[i];
                setArgType(commandArgs);
            }
            if ((arg.toLowerCase().trim().contentEquals("-r")) || (arg.toLowerCase().contains("--del"))) {
                i++;
                if (args[i].startsWith("-")) {
                    System.out.println("ERROR: E002 Missing Argument!");
                    System.out
                              .println("The remove function requires an argument, either a package name or a platform name!");
                    System.out.println("DbUpdate Aborted!");
                    System.err.flush();
                    System.exit(EFAILURE);
                }
                commandToCall = Cmd.DELETE_SOMETHING;
                commandArgs = args[i];
                setArgType(commandArgs);
            }
            if (arg.toLowerCase().contains("--find")) {
                commandToCall = Cmd.FIND_SOMETHING;
                i++;
                if (args[i].toLowerCase().contains("--lib"))
                    whatToFind = "LIBRARY";
                else if (args[i].toLowerCase().contains("--guid"))
                    whatToFind = "GUID";
                else if (args[i].toLowerCase().contains("--ppi"))
                    whatToFind = "PPI";
                else if (args[i].toLowerCase().contains("--prot"))
                    whatToFind = "PROTOCOL";
                else if (args[i].toLowerCase().contains("--pcd"))
                    whatToFind = "PCD";
                else if (args[i].startsWith("-")) {
                    System.out.println("ERROR: E001 Invalid Argument");
                    System.out.println("The find function takes either a qualifier of --guid, --ppi,");
                    System.out.println("  --proto, --pcd, or the string to search for.");
                    System.err.flush();
                    System.exit(EFAILURE);
                } else
                    commandArgs = args[i];

                if (!whatToFind.contentEquals("")) {
                    i++;
                    if (args[i].startsWith("-")) {
                        System.out.println("ERROR: E001 Invalid Argument");
                        System.out.println("The find function qualifier (--guid, --ppi, --proto, --pcd)");
                        System.out.println("  must be followed by the string to search for.");
                        System.err.flush();
                        System.exit(EFAILURE);
                    } else
                        commandArgs = args[i];
                }

            }
            if (arg.trim().contentEquals("-v")) {
                VERBOSE++;
            }
            if (arg.toLowerCase().contains("--lib")) {
                whatToFind = "LIBRARY";
            }
            if (arg.toLowerCase().contains("--guid")) {
                whatToFind = "GUID";
            }
            if (arg.toLowerCase().contains("--ppi")) {
                whatToFind = "PPI";
            }
            if (arg.toLowerCase().contains("--prot")) {
                whatToFind = "PROTOCOL";
            }
            if (arg.toLowerCase().contains("--pcd")) {
                whatToFind = "PCD";
            }
            if (arg.toLowerCase().contentEquals("-d") || arg.trim().toLowerCase().contains("--debug")) {
                if ((i + 1 == args.length) || (args[i + 1].trim().startsWith("-"))) {
                    DEBUG = 1;
                } else if (i + 1 < args.length) {
                    String pattern = "^\\d+";
                    if (args[i + 1].trim().matches(pattern)) {
                        i++;
                        DEBUG = Integer.parseInt(args[i]);
                    } else
                        DEBUG = 1;
                }
            }
            if (arg.trim().contentEquals("-V")) {
                System.out.println("DbTools, " + version);
                System.out.println(copyright);
                System.out.println(Specification);
                System.err.flush();
                System.exit(ESUCCESS);
            }
            if (arg.toLowerCase().trim().startsWith("--pack")) {
                commandToCall = Cmd.SHOW_PACKAGES;
            }
            if (arg.toLowerCase().trim().startsWith("--far")) {
                commandToCall = Cmd.SHOW_FAR;
            }
            if (arg.toLowerCase().trim().startsWith("--plat")) {
                commandToCall = Cmd.SHOW_PLATFORMS;
            }
            if ((arg.toLowerCase().contentEquals("-h")) || (arg.toLowerCase().contentEquals("-?"))
                || (arg.toLowerCase().startsWith("/h")) || (arg.toLowerCase().contentEquals("--help"))) {
                outputUsage();
                System.exit(EFAILURE);
            }

        }
        return ESUCCESS;
    }

    private int testFile(String Filename) {
        File tFile = new File(Filename);
        if (DEBUG > 4)
            System.out.println("File is located: " + tFile.getPath());
        if (tFile.exists())
            return FOUND;
        else
            return NOTFOUND;
    }

    private void setArgType(String argv) {
        if (argv.trim().toLowerCase().contains(".spd"))
            isPackage = true;
        if (argv.trim().toLowerCase().contains(".fpd"))
            isPlatform = true;
        if (argv.trim().toLowerCase().contains(".far"))
            isFar = true;
    }

    public String getArgType() {
        String argt = "UNKNOWN";
        if (isPackage)
            argt = "SPD";
        if (isFar)
            argt = "FAR";
        if (isPlatform)
            argt = "FPD";
        return argt;
    }

    private static void outputUsage() {

        System.out.println("DbTool, " + version);
        System.out.println(copyright);
        System.out.println("Usage:");
        System.out.println("  DbTool [-v] [-t] [-q] [-V] [--package] [--platform] [-h | -? | --help]");
        System.out.println("    where:");
        System.out.println("      -h | -? | --help          OPTIONAL - This Help Text");
        System.out
                  .println("      -t | --test               OPTIONAL - Test the FrameworkDatabase Contents against the WORKSPACE");
        System.out
                  .println("      -q                        OPTIONAL - Quiet mode - pass or fail only on return value, nothing is printed");
        System.out
                  .println("      -f | --fix                OPTIONAL - Automatically fix (non-interactive) the Database file to match the WORKSPACE.");
        System.out
                  .println("      -v                        OPTIONAL - Verbose, print information messages.  Adding more -v arguments increases verbosity.");
        System.out.println("      --package                 OPTIONAL - Show all Packages installed in the WORKSPACE.");
        System.out.println("      --platforms               OPTIONAL - Show all Platforms installed in the WORKSPACE.");
        System.out.println("      -V                        OPTIONAL - Display Version information and exit.");
        //
        // TODO: Implement the following options.
        //
        /**
        System.out.println("");
        System.out.println(" =================== Options below this line have not been implemented in this release ===================");
        System.out
                  .println("      -i                        OPTIONAL - Force interactive on commands that modify the WORKSPACE");
        System.out
                  .println("      --far                     OPTIONAL - Show all Framework Archives installed in the WORKSPACE.");
        System.out
                  .println("      --find [qualifier] value  OPTIONAL - Search the WORKSPACE for value, with one and only one optional Qualifier.");
        System.out.println("         qualifiers: --guid  Find a GUID by Guid C Name");
        System.out.println("                     --prot  Find a Protocol or ProtocolNotify by C Name");
        System.out.println("                     --ppi   Find a PPI or PpiNotify by C Name");
        System.out.println("                     --pcd   Find a PCD entry by C Name");
        System.out.println("                     --lib   Find information about a Library Class");
        System.out
                  .println("      -c                        OPTIONAL - Print a comma separated value listing of TYPE,UiName,Filename");
        System.out
                  .println("      -a, --add value           OPTIONAL - Add a value (package, platform or far) to the database");
        System.out
                  .println("      -r, --del value           OPTIONAL - Remove a value (package, platform or far) from the database");
        System.out.println("");
        */
    }
}
