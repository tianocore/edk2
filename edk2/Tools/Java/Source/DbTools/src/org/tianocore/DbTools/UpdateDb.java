// @file
//   This Class processes multiple MSA files and merges them into a single, 
//   merged MSA file. It will optionally add the merged MSA file into a package.
//
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
//

package org.tianocore.DbTools;

import java.io.*;
import java.util.*;
// import java.sql.Time;

// import java.lang.*;
// import java.lang.ExceptionInInitializerError;

// import org.apache.xmlbeans.*;
import org.apache.xmlbeans.XmlCursor;
// import org.apache.xmlbeans.XmlObject;
import org.apache.xmlbeans.XmlOptions;
// import org.apache.xmlbeans.XmlException;
import org.tianocore.*;

// import org.tianocore.*;
// FrameworkDatabase.db Schema Elements
import org.tianocore.FrameworkDatabaseDocument.*;
import org.tianocore.FdbHeaderDocument.*;
import org.tianocore.PackageListDocument.*;
import org.tianocore.PlatformListDocument.*;
import org.tianocore.FarListDocument.*;

// FAR Schema Elements
// import org.tianocore.FrameworkArchiveManifestDocument.*;
// import org.tianocore.FarHeaderDocument.*;
// import org.tianocore.FarPackageDocument.*;
// import org.tianocore.FarPlatformDocument.*;

// FPD Schema Elements
import org.tianocore.PlatformSurfaceAreaDocument.*;
import org.tianocore.PlatformHeaderDocument.*;

// import org.tianocore.FrameworkModulesDocument.*;
// import org.tianocore.DynamicPcdBuildDefinitionsDocument.*;

// SPD Schema Elements
// import org.tianocore.PackageSurfaceAreaDocument.*;
// import org.tianocore.SpdHeaderDocument.*;
// import org.tianocore.LibraryClassDeclarationsDocument.*;
// import org.tianocore.GuidDeclarationsDocument.*;
// import org.tianocore.ProtocolDeclarationsDocument.*;
// import org.tianocore.PpiDeclarationsDocument.*;
// import org.tianocore.PcdDeclarationsDocument.*;

// MSA Schema Elements
// import org.tianocore.ModuleSurfaceAreaDocument.*;
// import org.tianocore.MsaHeaderDocument.*;
// import org.tianocore.LicenseDocument.*;
// import org.tianocore.LibraryClassDefinitionsDocument.*;
// import org.tianocore.PackageDependenciesDocument.*;
// import org.tianocore.ProtocolsDocument.*;
// import org.tianocore.PPIsDocument.*;
// import org.tianocore.PcdCodedDocument.*;

public class UpdateDb {

    private final int DEBUG = 0;

    private final int PASS = 0;

    private final int FAIL = 1;

    private int dCtr = 0;

    private int lCtr = 0;

    // Future implementations
    //    private XmlCursor cursor = null;

    // FrameworkDatabase Component Elements
    //    private FrameworkDatabaseDocument fdbDoc = null;

    //    private FrameworkDatabase fdbInstance = null;

    //    private FdbHeader fdbHeader = null;

    //    private PackageList fdbPackageList = null;

    //    private ArrayList<String> packageList = new ArrayList<String>();

    //    private PlatformList fdbPlatformList = null;

    //    private ArrayList<String> platformList = new ArrayList<String>();

    //    private FarList fdbFarList = null;

    //    private ArrayList<String> farList = new ArrayList<String>();

    private ArrayList<String> spdList = new ArrayList<String>();

    private ArrayList<String> fpdList = new ArrayList<String>();

    // Platform Components
    //    private PlatformSurfaceAreaDocument fpdDoc = null;

    //    private PlatformSurfaceArea fpd = null;

    //    private PlatformHeader fpdHeader = null;

    private boolean wasModified = false;

    private int VERBOSE = 0;

    // The combineMsaFiles routine is the primary routine for creating a 
    // Merged MSA file.

    public UpdateDb() {
        init();
    }

    private void init() {

    }

    public int getAllEntries(String dbFilename, int VERBOSE_FLAG) {
        System.out.println("Function not yet implemented!");
        return FAIL;
    }

    public int getCsvEntries(String dbFilename, int VERBOSE_FLAG) {
        System.out.println("Function not yet implemented!");
        return FAIL;
    }

    public int fixDatabase(String workspace, int VERBOSE_FLAG, boolean INTERACTIVE, boolean TEST, boolean QUIET) {
        int result = PASS;
        String dbFile = workspace + File.separator + "Tools" + File.separator + "Conf" + File.separator
                        + "FrameworkDatabase.db";
        String dbBak = dbFile + ".bak";

        try {
            // Make a backup file for frameworkDatabase
            if (VERBOSE_FLAG > 3)
                System.out.println("Creating Backup file: " + dbBak);
            copyFile(dbFile, dbBak);
        } catch (IOException e) {
            System.out.println("Error: Cannot make backup file " + dbBak);
            return FAIL;
        }

        // Datebase update for SPD and FPD files
        result = fixDB(workspace, dbFile, VERBOSE_FLAG, INTERACTIVE, TEST, QUIET);
        if ((result == 0) && (TEST) && (QUIET == false))
            System.out.println("Workspace is consistent with current database!");
        return PASS;
    }

    public int addItem(String dbFilename, String item, int VERBOSE_FLAG, boolean INTERACTIVE) {
        System.out.println("Function not yet implemented!");
        return FAIL;
    }

    public int delItem(String workspace, String item, int VERBOSE_FLAG, boolean INTERACTIVE) {
        System.out.println("Function not yet implemented!");
        return FAIL;
    }

    public int findItem(String workspace, int VERBOSE_FLAG, String findWhat, String item) {
        System.out.println("Function not yet implemented!");
        return FAIL;
    }

    public int findFars(String workspace, int VERBOSE_FLAG) {
        System.out.println("Function not yet implemented!");
        return FAIL;
    }

    public int findSpds(String workspace, int VERBOSE_FLAG) {
        System.out.println("The following Package (SPD) files are in the workspace" + workspace);            
        File wsDir = new File(workspace);
        findSPDFiles(wsDir, workspace, VERBOSE);
        for (int i = 0; i < spdList.size(); i++) {
            String Filename = workspace + File.separator + spdList.get(i).trim();
            File spdFilename = new File(Filename);
            try {
                FileReader fileReader = new FileReader(spdFilename);
                BufferedReader reader = new BufferedReader(fileReader);
                String PackageName = null;
                String rLine = null;
                while ((rLine = reader.readLine()) != null) {
                    if (rLine.contains("<PackageName>")) {
                        PackageName = rLine.replace("<PackageName>", "").trim();
                        PackageName = PackageName.replace("</PackageName>", "").trim();
                        System.out.printf("  %25s - %s\n",PackageName, spdList.get(i).trim());
                        break;
                    }
                }
                reader.close();
            } catch (IOException e) {
                System.out.println("ERROR Reading File: " + Filename + e);
                System.exit(FAIL);
            }
            
        }
        return PASS;
    }

    public int findFpds(String workspace, int VERBOSE_FLAG) {
        System.out.println("The following Platform (FPD) files are in the workspace: " + workspace);
        File wsDir = new File(workspace);
        findFPDFiles(wsDir, workspace, VERBOSE);
        for (int i = 0; i < fpdList.size(); i++) {
            String Filename = workspace + File.separator + fpdList.get(i).trim();
            File fpdFilename = new File(Filename);
            try {
                FileReader fileReader = new FileReader(fpdFilename);
                BufferedReader reader = new BufferedReader(fileReader);
                String PlatformName = null;
                String rLine = null;
                while ((rLine = reader.readLine()) != null) {
                    if (rLine.contains("<PlatformName>")) {
                        PlatformName = rLine.replace("<PlatformName>", "").trim();
                        PlatformName = PlatformName.replace("</PlatformName>", "").trim();
                        System.out.printf("  %25s - %s\n",PlatformName, fpdList.get(i).trim());
                        break;
                    }
                }
                reader.close();
            } catch (IOException e) {
                System.out.println("ERROR Reading File: " + Filename + e);
                System.exit(FAIL);
            }
        }
        return PASS;
    }

    // copy from source file to destination file
    private int copyFile(String src, String dst) throws IOException {
        try {
            File srcFile = new File(src);
            FileReader fileReader = new FileReader(srcFile);
            BufferedReader reader = new BufferedReader(fileReader);

            File dstFile = new File(dst);
            FileWriter fileWriter = new FileWriter(dstFile);
            BufferedWriter writer = new BufferedWriter(fileWriter);

            String line = null;
            while ((line = reader.readLine()) != null) {
                writer.write(line);
                writer.newLine();
            }

            reader.close();
            writer.close();
        } catch (IOException e) {
            System.out.println("I/O Exception during file copy: " + e);
        }

        return PASS;
    }

    private int fixDB(String workspace, String dbFile, int VERBOSE, boolean INTERACTIVE, boolean TEST, boolean QUIET) {
        File wsDir = new File(workspace);
        int retValue = PASS;
        // Find all .spd and .fpd files in workspace and put them in spdList and fpdList
        if (VERBOSE > 0)
            System.out.println("SPD File Search ");
        findSPDFiles(wsDir, workspace, VERBOSE);
        dCtr = 0;
        lCtr = 0;
        if (VERBOSE > 0) {
            System.out.println(" ");
            System.out.println("FPD File Search ");
        }
        findFPDFiles(wsDir, workspace, VERBOSE);
        if (VERBOSE > 0)
            System.out.println(" ");

        try {
            // check database file for possible update
            retValue = checkDBForUpdate(workspace, dbFile, VERBOSE, INTERACTIVE, TEST, QUIET);
        } catch (IOException e) {
            if (QUIET == false)
                System.out.println("Error: Updating " + dbFile + " file.");
            return FAIL;
        }

        if ((VERBOSE > 0) && (TEST) && (wasModified)) {
            System.out.println("FRAMEWORK Database does NOT match the contents of the WORKSPACE");
            retValue = FAIL;
        }
        if ((VERBOSE > 0) && (wasModified == false) && (QUIET == false))
            System.out.println("FRAMEWORK Database matches the contents of the WORKSPACE");

        return retValue;
    }

    private void findSPDFiles(File dir, String workspace, int VERBOSE) {
        String str;

        if (dir.isDirectory()) {
            dCtr++;
            String[] subdir = dir.list();
            if (dCtr >= 10) {
                if (VERBOSE > 2)
                    System.out.print(".");
                dCtr = 0;
                lCtr++;
            }
            if (lCtr > 79) {
                if (VERBOSE > 2)
                    System.out.println(" ");
                lCtr = 0;
                dCtr = 0;
            }

            for (int i = 0; i < subdir.length; i++) {
                findSPDFiles(new File(dir, subdir[i]), workspace, VERBOSE);
            }
        } else {
            if (dir.toString().toLowerCase().endsWith(".spd")) {
                str = dir.getPath().replace('\\', '/');
                workspace = workspace.replace('\\', '/');
                str = strStrip(str, workspace + "/");
                spdList.add(str.toString());
                if (VERBOSE == 2)
                    System.out.println("  " + str);
                if (VERBOSE > 2)
                    System.out.print("+");
                lCtr++;
            }
        }

    }

    private void findFPDFiles(File dir, String workspace, int VERBOSE) {
        String str;

        if (dir.isDirectory()) {
            String[] subdir = dir.list();
            if (dCtr >= 10) {
                if (VERBOSE > 2)
                    System.out.print(".");
                dCtr = 0;
                lCtr++;
            }
            if (lCtr > 79) {
                if (VERBOSE > 2)
                    System.out.println(" ");
                lCtr = 0;
                dCtr = 0;
            }

            for (int i = 0; i < subdir.length; i++) {
                findFPDFiles(new File(dir, subdir[i]), workspace, VERBOSE);
            }
        } else {
            if (dir.toString().toLowerCase().endsWith(".fpd")) {
                str = dir.getPath().replace('\\', '/');
                workspace = workspace.replace('\\', '/');
                str = strStrip(str, workspace + "/");
                fpdList.add(str.toString());
                if (VERBOSE == 2)
                    System.out.println("  " + str);
                if (VERBOSE > 2)
                    System.out.print("+");
                lCtr++;
            }
        }
    }

    private int checkDBForUpdate(String workspace, String dbFileName, int VERBOSE, boolean INTERACTIVE, boolean TEST, boolean QUIET)
                                                                                                               throws IOException {
        int SpdFlag = 0;
        int FpdFlag = 0;
        String SpdFile = null;
        String SpdFullFile = null;
        String FpdFile = null;
        String FpdFullFile = null;

        String tmpDbFile = dbFileName + ".tmp";
        String newSpd = null;
        String newFpd = null;

        FileReader fileReader = new FileReader(dbFileName);
        BufferedReader reader = new BufferedReader(fileReader);

        FileWriter fileWriter = new FileWriter(tmpDbFile);
        BufferedWriter writer = new BufferedWriter(fileWriter);

        String line = null;
        try {
            while ((line = reader.readLine()) != null) {
                if (line.indexOf("Added the following") >= 0) {
                    wasModified = true;
                    continue;
                }
                //
                // Process for .spd files
                //
                if (line.indexOf("<PackageList") >= 0) {
                    SpdFlag = 1;
                } else {
                    if (line.indexOf("</PackageList") >= 0) {
                        SpdFlag = 2;
                    }
                }
                if (SpdFlag == 1 && line.indexOf("Filename") >= 0) {
                    SpdFile = strStrip(line, "<Filename>");
                    SpdFile = strStrip(SpdFile, "</Filename>");
                    SpdFile = SpdFile.trim();
                    SpdFullFile = workspace + File.separator + SpdFile;
                    if (!(new File(SpdFullFile)).exists()) {
                        if (VERBOSE > 0)
                            System.out.println("WARNING: Removing SPD file: " + SpdFile
                                               + "from the DB, as it does not exist!");
                        wasModified = true;
                        continue;
                    }
                    // Don't add files that are already in the database
                    spdList.remove(SpdFile);
                }
                if (SpdFlag == 2) {
                    int cflag = 0;
                    for (int i = 0; i < spdList.size(); i++) {
                        newSpd = spdList.get(i);
                        newSpd = newSpd.trim();
                        if (newSpd.length() > 0) {
                            if (cflag == 0) {
                                Calendar c = Calendar.getInstance();
                                if (TEST == false)
                                    writer.write("    <!-- Adding the following SPD files " + c.getTime() + " -->\n");
                            }
                            if (VERBOSE > 0)
                                System.out.println("  Adding SPD file: " + newSpd);
                            if (TEST == false)
                                writer.write("    <Filename>" + newSpd + "</Filename>\n");
                            cflag++;
                            wasModified = true;
                        }
                    }
                    SpdFlag++;
                }
                //
                // Process for .fpd files
                // 
                if (line.indexOf("<PlatformList") >= 0) {
                    FpdFlag = 1;
                } else {
                    if (line.indexOf("</PlatformList") >= 0) {
                        FpdFlag = 2;
                    }
                }
                if (FpdFlag == 1 && line.indexOf("Filename") >= 0) {
                    FpdFile = strStrip(line, "<Filename>");
                    FpdFile = strStrip(FpdFile, "</Filename>");
                    FpdFile = FpdFile.trim();
                    FpdFullFile = workspace + File.separator + FpdFile;
                    if (!(new File(FpdFullFile)).exists()) {
                        if (VERBOSE > 0)
                            System.out.println("WARNING: Removing FPD file: " + FpdFile
                                               + " from the DB, as it does not exist!");
                        wasModified = true;
                        continue;
                    }
                    // Don't add files that are already in the database
                    fpdList.remove(FpdFile);
                }
                if (FpdFlag == 2) {
                    int cflag = 0;
                    for (int i = 0; i < fpdList.size(); i++) {
                        newFpd = fpdList.get(i);
                        newFpd = newFpd.trim();
                        if (newFpd.length() > 0) {
                            if (cflag == 0) {
                                Calendar c = Calendar.getInstance();
                                if (TEST == false)
                                    writer.write("    <!-- Adding the following FPD files " + c.getTime() + " -->\n");
                            }
                            if (VERBOSE > 0)
                                System.out.println("  Adding FPD file: " + newFpd);
                            if (TEST == false)
                                writer.write("    <Filename>" + newFpd + "</Filename>\n");
                            cflag++;
                            wasModified = true;
                        }
                    }
                    FpdFlag++;
                }

                if (DEBUG > 2) {
                    System.out.println(line);
                }
                if (TEST == false) {
                    writer.write(line);
                    writer.newLine();
                }
            }
            reader.close();
            writer.close();

        } catch (IOException e) {
            System.out.println("ERROR I/O Exception occured! " + e);
            System.exit(FAIL);
        }

        if (wasModified) {
            if ((VERBOSE > 0) && (QUIET == false))
                System.out.println("FrameworkDatabase has been UPDATED for this Workspace!\n");
            if (TEST == false)
                copyFile(tmpDbFile, dbFileName);
        } else {
            if ((VERBOSE > 0) && (QUIET == false))
                System.out.println("FrameworkDatabase correct for this Workspace!\n");
        }

        if (TEST == false) {
            File tmpFile = new File(tmpDbFile);
            if (tmpFile.exists()) {
                tmpFile.delete();
            }
        }

        return PASS;
    }

    static String strStrip(String str, String pattern) {
        int ps = 0;
        int pe = 0;
        StringBuffer result = new StringBuffer();

        while ((pe = str.indexOf(pattern, ps)) >= 0) {
            result.append(str.substring(ps, pe));
            ps = pe + pattern.length();
        }
        result.append(str.substring(ps));
        return result.toString();
    }

    private String checkDuplicateStrings(String aString, ArrayList<String> aList) {
        // This routine checks a string against an array.
        // If the string is found, it will return an empty string.
        // If the string is not found, it adds the string to the array, and
        // returns the string to the caller.
        for (int lctr = 0; lctr < aList.size(); lctr++) {
            if (DEBUG > 8)
                System.out.println("Comparing: \n" + aString.replace(" ", "").replace("\n", "") + "\nTo: \n"
                                   + aList.get(lctr).replace(" ", "").replace("\n", "").toString().trim());
            if (aString.replace(" ", "").replace("\n", "").contains(
                                                                    aList.get(lctr).replace(" ", "").replace("\n", "")
                                                                         .toString().trim())) {
                if ((DEBUG > 3) || (VERBOSE > 3))
                    System.out.println("Found a duplicate String, skipping!");
                return "";
            }
        }
        if ((DEBUG > 3) || (VERBOSE > 3))
            System.out.println("Returning UNIQUE String!\n " + aString);
        aList.add(aString);
        return aString;
    }

    private static class XmlConfig {
        public static XmlCursor setupXmlCursor(XmlCursor cursor) {
            String uri = "http://www.TianoCore.org/2006/Edk2.0";
            cursor.push();
            cursor.toNextToken();
            cursor.insertNamespace("", uri);
            cursor.insertNamespace("xsi", "http://www.w3.org/2001/XMLSchema-instance");
            cursor.pop();
            return cursor;

        }

        public static XmlOptions setupXmlOptions() {
            XmlOptions options = new XmlOptions();
            options.setCharacterEncoding("UTF-8");
            options.setSavePrettyPrint();
            options.setSavePrettyPrintIndent(2);
            return options;
        }

    }
}
