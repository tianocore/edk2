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

package org.tianocore.Merge;

import java.io.*;
import java.util.*;
// import java.lang.*;
// import java.lang.ExceptionInInitializerError;

// import org.apache.xmlbeans.*;
import org.apache.xmlbeans.XmlCursor;
// import org.apache.xmlbeans.XmlObject;
import org.apache.xmlbeans.XmlOptions;
import org.apache.xmlbeans.XmlException;
import org.tianocore.*;

// import org.tianocore.*;
import org.tianocore.ModuleSurfaceAreaDocument.*;
import org.tianocore.MsaHeaderDocument.*;
import org.tianocore.LicenseDocument.*;
import org.tianocore.ModuleDefinitionsDocument.*;
import org.tianocore.LibraryClassDefinitionsDocument.*;
import org.tianocore.SourceFilesDocument.*;
import org.tianocore.PackageDependenciesDocument.*;
import org.tianocore.ProtocolsDocument.*;
import org.tianocore.EventsDocument.*;
import org.tianocore.HobsDocument.*;
import org.tianocore.PPIsDocument.*;
import org.tianocore.VariablesDocument.*;
import org.tianocore.BootModesDocument.*;
import org.tianocore.SystemTablesDocument.*;
import org.tianocore.DataHubsDocument.*;
import org.tianocore.HiiPackagesDocument.*;
import org.tianocore.GuidsDocument.*;
import org.tianocore.ExternsDocument.*;
import org.tianocore.PcdCodedDocument.*;
import org.tianocore.ModuleBuildOptionsDocument.*;
import org.tianocore.UserExtensionsDocument.*;

import org.tianocore.PackageSurfaceAreaDocument.*;

public class CombineMsa {

    private final int DEBUG = 0;

    private final int PASS = 0;

    private final int FAIL = 1;

    private final int FOUND = 0;

    private final int NOTFOUND = 1;

    private int result = PASS;

    private String licenseTxt = "";

    private ArrayList<String> aLicenses = new ArrayList<String>();

    private String Copyright = "";

    private ArrayList<String> aCopyright = new ArrayList<String>();

    private String Abstract = "\n      Merged Modules: \n";

    private String Description = "\n      Merging Modules: \n";

    private String sArchitectures = "";

    private MsaHeader header = null;

    private final String Specification = "FRAMEWORK_BUILD_PACKAGING_SPECIFICATION   0x00000052";

    private ModuleSurfaceArea mergeMsaFile = null;

    private ModuleSurfaceArea leafMsa = null;

    private ModuleDefinitions moduleDefs = null;

    private LibraryClassDefinitions libClassDefs = null;

    private ArrayList<String> aLibClassDefs = new ArrayList<String>();

    private int libraryClassIndex = 0;

    private SourceFiles mergeSourceFiles = null;

    private int sourceFileIndex = 0;

    private PackageDependencies mergePackageDependencies = null;

    private ArrayList<String> aPackageList = new ArrayList<String>();

    private int packageDependenciesIndex = 0;

    private Protocols mergeProtocols = null;

    private ArrayList<String> aProtocolList = new ArrayList<String>();

    private ArrayList<String> aProtocolNotifyList = new ArrayList<String>();

    private int protocolIndex = 0;

    private int protocolNotifyIndex = 0;

    private Events mergeEvents = null;

    private Events.CreateEvents mergeCreateEvents = null;

    private Events.SignalEvents mergeSignalEvents = null;

    private ArrayList<String> aCreateEventsList = new ArrayList<String>();

    private ArrayList<String> aSignalEventsList = new ArrayList<String>();

    private int createEventIndex = 0;

    private int signalEventIndex = 0;

    private Hobs mergeHobs = null;

    private ArrayList<String> aHobsList = new ArrayList<String>();

    private int hobsIndex = 0;

    private PPIs mergePpis = null;

    private ArrayList<String> aPpiList = new ArrayList<String>();

    private ArrayList<String> aPpiNotifyList = new ArrayList<String>();

    private int ppiIndex = 0;

    private int ppiNotifyIndex = 0;

    private Variables mergeVariables = null;

    private ArrayList<String> aVariablesList = new ArrayList<String>();

    private int variablesIndex = 0;

    private BootModes mergeBootModes = null;

    private ArrayList<String> aBootModesList = new ArrayList<String>();

    private int bootModesIndex = 0;

    private SystemTables mergeSystemTables = null;

    private ArrayList<String> aSystemTablesList = new ArrayList<String>();

    private int systemTableIndex = 0;

    private DataHubs mergeDataHubs = null;

    private ArrayList<String> aDataHubsList = new ArrayList<String>();

    private int dataHubsIndex = 0;

    private HiiPackages mergeHiiPackages = null;

    private ArrayList<String> aHiiPackagesList = new ArrayList<String>();

    private int hiiPackageIndex = 0;

    private Guids mergeGuids = null;

    private ArrayList<String> aGuidsList = new ArrayList<String>();

    private int guidsIndex = 0;

    private Externs mergeExterns = null;

    private ArrayList<String> aEntryPointList = new ArrayList<String>();

    private ArrayList<String> aUnloadImageList = new ArrayList<String>();

    private ArrayList<String> aDriverBindingList = new ArrayList<String>();

    private ArrayList<String> aConstructorList = new ArrayList<String>();

    private ArrayList<String> aDestructorList = new ArrayList<String>();

    private ArrayList<String> aVirtualAddressMapList = new ArrayList<String>();

    private ArrayList<String> aExitBootServicesList = new ArrayList<String>();

    private int externsIndex = 0;

    private ArrayList<String> aSpecArray = new ArrayList<String>();

    private int specIndex = 0;

    private PcdCoded mergePcdCoded = null;

    private ArrayList<String> aPcdCNameList = new ArrayList<String>();

    private ArrayList<String> aPcdItemTypeList = new ArrayList<String>();

    private int pcdIndex = 0;

    private ModuleBuildOptions mergeBuildOptions = null;

    private UserExtensions mergeUserExtensions = null;

    private String mergeUsage = "";

    private String leafUsage = "";

    private int VERBOSE = 0;

    // The combineMsaFiles routine is the primary routine for creating a 
    // Merged MSA file.

    public int combineMsaFiles(String msaFilename, ArrayList<String> msaFiles, String uiName, String spdFilename,
                               String baseName, int Flags) {
        // msaFile has been verified to either not exist, or, if it does exist, 
        // it will be over written.
        // All files in the msaFiles ArrayList have been verifed to exist.
        // If the uiName is not null, we will have a new UI Name for the merged 
        // module.
        // If the uiName is null, we will use the module name from the first 
        // Leaf module.
        // If the spdFile is not null, the Package (SPD) file has been verified 
        // to exist.
        // If the spdFile is null, don't attempt to add the new msa file.

        if (Flags > 0)
            VERBOSE = 1;
        if (mergeMsaFile == null) {
            //
            // create the data structure for the merged Module
            //
            mergeMsaFile = ModuleSurfaceArea.Factory.newInstance();

            System.out.println("Merging " + msaFiles.size() + " Modules");
            //
            // we always require a Header and a Module Definition section.
            // These will be added to the mergeMsaFile after we have completed 
            // all processing of the Leaf MSA files.
            //
            header = MsaHeaderDocument.Factory.newInstance().addNewMsaHeader();
            moduleDefs = ModuleDefinitionsDocument.Factory.newInstance().addNewModuleDefinitions();
            //
            // A merged module cannot be created from binary modules - we force 
            // the new module to be source here, however we will test every 
            // module to make sure that none are binary; exiting the program if
            // a module is binary
            //
            moduleDefs.setBinaryModule(false);

            for (int i = 0; i < msaFiles.size(); i++) {
                String leafFilename = msaFiles.get(i).toString();
                leafMsa = getLeafFile(leafFilename);
                if (leafMsa == null) {
                    System.out.println("Could not read Leaf MSA file: " + leafFilename);
                    System.exit(FAIL);
                }
                if (i == 0) {
                    //
                    // Special code for first file, since this file is used 
                    // to initialize some of the data in the mergeMsaFile.                        
                    // Set the Merge module's ModuleName to the name in the 
                    // first Leaf Module.  If a new module name is given, 
                    // over write it later, just before writing the Merge 
                    // Module MSA file.
                    header.setModuleName(leafMsa.getMsaHeader().getModuleName().toString());
                    //
                    // All modules must be of the same module type, we set it 
                    // here, and test the other Leaf modules' type later.
                    header.setModuleType(leafMsa.getMsaHeader().getModuleType());
                    //
                    // This is a new Module, so we need a new GUID
                    header.setGuidValue(UUID.randomUUID().toString());
                    //
                    // Use the version from the first Leaf module as the 
                    // Merge Module version number.
                    header.setVersion(leafMsa.getMsaHeader().getVersion().toString());
                    //
                    // There is no special requirement for processing the 
                    // following, so we just fall through on these elements 
                    // of the header.
                    // Abstract will be added after parsing all leaf MSA files.
                    // Description will be added after parsing all leaf MSA files.
                    // Copyright will be added after parsing all leaf MSA files.
                    // License will be added after parsing all leaf MSA files.
                    //
                    // Force the specification to match this tool's spec version.
                    header.setSpecification(Specification);
                    //
                    // Set the Merged Module's Output Basename to match the first 
                    // leaf module
                    String OutputFileName = leafMsa.getModuleDefinitions().getOutputFileBasename().toString();
                    //
                    // Just in case someone put a space character in the first
                    // leaf module's output filename, replace the spaces with
                    // an underscore.
                    OutputFileName.replace(" ", "_");
                    moduleDefs.setOutputFileBasename(OutputFileName);
                    //
                    // We start with the first module's list of supported 
                    // architectures.  As we process the additional leaf modules,
                    // we may have to remove some supported architectures from 
                    // the list, as we can only build for the "least common 
                    // denominator" subset of architectures.
                    sArchitectures = leafMsa.getModuleDefinitions().getSupportedArchitectures().toString();
                    if ((DEBUG > 5) || (VERBOSE > 5))
                        System.out.println("New Header:  \"" + header.getModuleName().toString() + "\"");
                }
                //
                // We test the license in each leaf module, and will only print
                // licenses that are may be different in wording (white spaces 
                // and line feeds are ignored in this test.)
                if (leafMsa.getMsaHeader().getLicense() != null)
                    licenseTxt += checkDuplicateStrings(leafMsa.getMsaHeader().getLicense().getStringValue().trim(),
                                                        aLicenses);
                if ((DEBUG > 10) || (VERBOSE > 10))
                    System.out.println("License:   " + licenseTxt);
                //
                // We test the copyright line from each leaf module, and will 
                // add additional copyright lines only if they are different 
                // in wording (white spaces and line feeds are ignored in this
                // test.)
                if (leafMsa.getMsaHeader().getCopyright() != null)
                    Copyright += checkDuplicateStrings(leafMsa.getMsaHeader().getCopyright().toString().trim(),
                                                       aCopyright);
                if ((DEBUG > 10) || (VERBOSE > 10))
                    System.out.println("Copyright: " + Copyright);
                //
                // ALL leaf modules must be of the same Module Type, if not, 
                // print an error and exit.
                if (header.getModuleType() != leafMsa.getMsaHeader().getModuleType()) {
                    System.out.println("ERROR: Module Types different!");
                    System.out.println("  Expected: " + header.getModuleType());
                    System.out.println("  " + leafFilename + " ModuleType: " + leafMsa.getMsaHeader().getModuleType());
                    System.out.println("Merge ABORTED!");
                    System.exit(FAIL);
                }
                //
                // Combine the Abstract and Descriptions into a single 
                // description entry, prefixing each with the Leaf MSA filename,
                // so you know which description is from which Leaf module.
                Description += "      -- " + leafFilename + " -- \n      Abstract:    "
                               + leafMsa.getMsaHeader().getAbstract().toString() + "\n      Description: "
                               + leafMsa.getMsaHeader().getDescription().toString() + "\n";
                //
                // Use the Abstract of the Merged Module to list the Leaf 
                // Module's MSA files.
                Abstract += "      -- " + leafFilename + " -- \n";
                //
                // Ignore ClonedFrom right now
                //

                // Process Supported Architectures
                // A merged module supports the lowest common set of 
                // architectures
                String testArch = "";
                if (leafMsa.getModuleDefinitions().getSupportedArchitectures() == null) {
                    System.out
                              .println("Module " + leafFilename + " does not have the Supported Architectures defined!");
                    System.out.println("Supported Architectures is a required element!");
                    System.out.println("Merge ABORTED!");
                    System.exit(FAIL);
                }
                testArch = leafMsa.getModuleDefinitions().getSupportedArchitectures().toString();
                String aArch[] = sArchitectures.split(" ");
                for (int ictr = 0; ictr < aArch.length; ictr++) {
                    if (!testArch.contains(aArch[ictr])) {
                        sArchitectures = sArchitectures.replace(aArch[ictr], "");
                    }
                }
                if (sArchitectures.length() < 2) {
                    System.out.println("ERROR: The Leaf Modules' Supported Architectures are mutually exclusive.");
                    System.out.println("At least one architecture must be common to all Leaf Modules!");
                    System.out.println("Merge Aborting!");
                    System.exit(FAIL);
                }
                //
                // Now start to process the rest of the Leaf Module's MSA files.
                // We will only test a Leaf module's section if it has data, skipping
                // empty sections
                // As part of this process, we will only create a Library Class Definition
                // if one was defined, and we will only do it one time.
                if (leafMsa.isSetLibraryClassDefinitions()) {
                    // 
                    //  If libClassDefs == null, create a new libClassDefs and add
                    //  this module's libClassDefs to the merge Module's
                    //  If libClassDefs != null, check that we have only unique LibraryClass entries
                    //  IF one Leaf usage is SOMETIMES_PRODUCED, and another Leaf usage is ALWAYS_PRODUCED,
                    //     Mark the LibraryClass as ALWAYS_PRODUCED
                    //  IF one Leaf usage is SOMETIMES_CONSUMED, and another Leaf usage is ALWAYS_CONSUMED,
                    //     Mark the LibraryClass as ALWAYS_CONSUMED
                    //  It is permissable to have one PRODUCED and one CONSUMED entry
                    //  TO_START and BY_START cannot be combined in any fashion, if one Leaf uses TO_START
                    //     and another Leaf uses BY_START, create two entries, one for each.
                    //
                    //  The RecommendedInstance attributes can be ignored!
                    //
                    //  First PASS of the TOOL 
                    //      SupArchList must be identical for all Leaf Modules!  Fail the Merge if not, reporting
                    //      this as an error!
                    //  Probable Enhancement
                    //      The SupArchList, if set, must be for the "Lowest Common Denominator"
                    //
                    //  First PASS of the TOOL
                    //      The SupModuleList must be identical for all Leaf Modules!
                    //  Probable Enhancement
                    //      The SupModuleList should be combined to include all possible supported module types.
                    //
                    //  First PASS of the TOOL
                    //      Ignore the FeatureFlag - we are not using it now.
                    //  Probable Enhancement
                    //      FeatureFlags must be identical for each LibraryClass, otherwize fail the Merge
                    //
                    // Create the working copy if one does not exist!
                    if (libClassDefs == null)
                        libClassDefs = LibraryClassDefinitionsDocument.Factory.newInstance()
                                                                              .addNewLibraryClassDefinitions();
                    //
                    // Get the Leaf LibraryClassDefinitions Section
                    LibraryClassDefinitions leafLibClassDef = leafMsa.getLibraryClassDefinitions();
                    //
                    // We only need to test there are entries in the Leaf LibraryClassDefinitions section!
                    if (leafLibClassDef.getLibraryClassList().size() > 0) {
                        for (int index = 0; index < leafLibClassDef.getLibraryClassList().size(); index++) {
                            //
                            // We can use the Keyword to search to see if the Leaf's Library Class was already
                            // added to the Merge Module.
                            String test = "";
                            String leafKeyword = leafLibClassDef.getLibraryClassList().get(index).getKeyword()
                                                                .toString().trim();

                            leafUsage = "";
                            if (leafLibClassDef.getLibraryClassList().get(index).getUsage() != null)
                                leafUsage = leafLibClassDef.getLibraryClassList().get(index).getUsage().toString()
                                                           .trim();

                            String leafSupArchList = "";
                            if (leafLibClassDef.getLibraryClassList().get(index).getSupArchList() != null)
                                leafSupArchList = leafLibClassDef.getLibraryClassList().get(index).getSupArchList()
                                                                 .toString().trim();

                            String leafSupModuleList = "";
                            if (leafLibClassDef.getLibraryClassList().get(index).getSupModuleList() != null)
                                leafSupModuleList = leafLibClassDef.getLibraryClassList().get(index).getSupModuleList()
                                                                   .toString().trim();

                            test = checkDuplicateStrings(leafKeyword, aLibClassDefs);
                            if (test.length() > 0) {
                                // The checkDuplicateStrings returns "" if a duplicate was found.
                                // Here, the Leaf LibraryClass gets entered because the Keyword was not found.
                                // No more testing is required, since this is the first instance of the LibraryClass
                                libClassDefs.addNewLibraryClass();
                                libClassDefs.setLibraryClassArray(libraryClassIndex++,
                                                                  leafLibClassDef.getLibraryClassList().get(index));
                            } else {
                                // The Merged Module has already specified the Library Class
                                // Check ATTRIBUTES, following the rules above.
                                // Since we cannot get the LibraryClass entry using the Keyword, we have to search
                                // all of the Merged Module's LibraryClass statements until we find a match.
                                // Also, we may have more than one LibraryClass with the same Keyword, but different
                                // Usage, SupArchList, FeatureFlag or SupModuleList
                                for (int nidx = 0; nidx < libraryClassIndex; nidx++) {
                                    String mergeKeyword = libClassDefs.getLibraryClassList().get(nidx).getKeyword()
                                                                      .trim();

                                    if (leafKeyword.contentEquals(mergeKeyword)) {
                                        // We have the FIRST match, let's check usage, remember, we can have more than one LibraryClass Keyword.
                                        mergeUsage = libClassDefs.getLibraryClassList().get(nidx).getUsage().toString()
                                                                 .trim();
                                        // If the usage is identical, check the SupArchList next
                                        if (!leafUsage.contentEquals(mergeUsage)) {
                                            if (checkUsage().trim().contains("DIFFERENT")) {
                                                //  See if there is another entry for PRODUCED or CONSUME
                                                int anotherLC = NOTFOUND;
                                                for (int iidx = nidx + 1; iidx < libraryClassIndex; iidx++) {
                                                    String innerTestKeyword = libClassDefs.getLibraryClassList()
                                                                                          .get(iidx).getKeyword()
                                                                                          .toString().trim();
                                                    if (leafKeyword.contentEquals(innerTestKeyword)) {
                                                        anotherLC = FOUND;
                                                        mergeUsage = libClassDefs.getLibraryClassList().get(iidx)
                                                                                 .getUsage().toString().trim();
                                                        if (checkProduced()) {
                                                            libClassDefs
                                                                        .getLibraryClassList()
                                                                        .get(iidx)
                                                                        .setUsage(
                                                                                  org.tianocore.UsageTypes.ALWAYS_PRODUCED);
                                                        }
                                                        // Both Usage types are CONSUMED
                                                        if (checkConsumed()) {
                                                            libClassDefs
                                                                        .getLibraryClassList()
                                                                        .get(iidx)
                                                                        .setUsage(
                                                                                  org.tianocore.UsageTypes.ALWAYS_CONSUMED);
                                                        }
                                                        if (((mergeUsage.contains("TO_START")) && (leafUsage
                                                                                                            .contains("TO_START"))))
                                                            anotherLC = FOUND;
                                                        if (((mergeUsage.contains("BY_START")) && (leafUsage
                                                                                                            .contains("BY_START"))))
                                                            anotherLC = FOUND;
                                                    }
                                                }
                                                if (anotherLC == NOTFOUND) {
                                                    // we need to add the leaf Library Class
                                                    libClassDefs.addNewLibraryClass();
                                                    libClassDefs
                                                                .setLibraryClassArray(
                                                                                      libraryClassIndex++,
                                                                                      leafLibClassDef
                                                                                                     .getLibraryClassList()
                                                                                                     .get(index));
                                                }
                                            }

                                            // Both Usage types are PRODUCED
                                            if (checkUsage().trim().contains("PRODUCED")) {
                                                libClassDefs.getLibraryClassList().get(nidx)
                                                            .setUsage(org.tianocore.UsageTypes.ALWAYS_PRODUCED);
                                            }
                                            // Both Usage types are CONSUMED
                                            if (checkUsage().trim().contains("CONSUMED")) {
                                                libClassDefs.getLibraryClassList().get(nidx)
                                                            .setUsage(org.tianocore.UsageTypes.ALWAYS_CONSUMED);
                                            }
                                        }
                                        // Usage testing completed
                                        // Check SupArchList
                                        String mergeSupArchList = "";
                                        if (libClassDefs.getLibraryClassList().get(nidx).getSupArchList() != null)
                                            mergeSupArchList = libClassDefs.getLibraryClassList().get(nidx)
                                                                           .getSupArchList().toString().trim();
                                        if (!mergeSupArchList.equalsIgnoreCase(leafSupArchList)) {
                                            System.out
                                                      .println("ERROR: Library Class, keyword: " + leafKeyword
                                                               + " defines a different set of supported architectures.");
                                            System.out
                                                      .println("Version 0.1 of the merge tool requires that they must be identical!");
                                            System.out.println("First instance of the Library used: "
                                                               + mergeSupArchList);
                                            System.out.println("While this module, " + leafFilename + " uses: "
                                                               + leafSupArchList);
                                            System.out.println("Merge ABORTED!");
                                            System.exit(FAIL);
                                        }
                                        // Architecture Testing completed
                                        // Check SupModuleType
                                        String mergeSupModuleList = "";
                                        if (libClassDefs.getLibraryClassList().get(nidx).getSupModuleList() != null)
                                            mergeSupModuleList = libClassDefs.getLibraryClassList().get(nidx)
                                                                             .getSupModuleList().toString().trim();
                                        if (!mergeSupModuleList.equalsIgnoreCase(leafSupModuleList)) {
                                            System.out.println("ERROR: Library Class, keyword: " + leafKeyword
                                                               + " defines a different set of supported modules.");
                                            System.out
                                                      .println("Version 0.1 of the merge tool requires that they must be identical!");
                                            System.out.println("First instance of the Library used: "
                                                               + mergeSupModuleList);
                                            System.out.println("While this module, " + leafFilename + " uses: "
                                                               + leafSupModuleList);
                                            System.out.println("Merge ABORTED!");
                                            System.exit(FAIL);
                                        }
                                        // Supported Module Testing completed
                                        // Check FeatureFlage
                                        //  Next version, not this one.
                                    }
                                } // end of processing of duplicate Library Class entries
                            } // end duplicate entry
                        } // end of test loop for duplicates   
                    } // endif Merge Module LibraryModuleDefinitions existed
                } // endif of LibraryModuleDefinition Tests

                if (leafMsa.isSetSourceFiles()) {
                    // TODO: test for NULL settings
                    // Add Sourcefiles to the Merge Module. NOTE: ONLY MODIFY THE Filename, prepending the path to the MSA file.
                    // First get the path portion of the leafMSA file, which will be prepended to the filename
                    // everything else stays intact.
                    if (mergeSourceFiles == null)
                        mergeSourceFiles = SourceFilesDocument.Factory.newInstance().addNewSourceFiles();

                    String pathToMsa = getPathPartOfLeafMsa(leafFilename);
                    if (DEBUG > 10)
                        System.out.println("PATH TO SOURCE FILES: " + pathToMsa);
                    if (leafMsa.getSourceFiles().getFilenameList() != null) {
                        List<FilenameDocument.Filename> leafSourceFiles = leafMsa.getSourceFiles().getFilenameList();
                        for (int index = 0; index < leafSourceFiles.size(); index++) {
                            String leafFile = leafSourceFiles.get(index).getStringValue().toString();
                            leafFile = pathToMsa + leafFile;
                            leafSourceFiles.get(index).setStringValue(leafFile);
                            mergeSourceFiles.addNewFilename();
                            mergeSourceFiles.setFilenameArray(sourceFileIndex++, leafSourceFiles.get(index));
                        }
                    }
                }

                if (leafMsa.isSetPackageDependencies()) {
                    //
                    //  If mergePackageDependencies == null, create a new mergePackageDependencies and
                    //  add the leaf module's Package Dependencies section to the merge Module's
                    //  If mergePackageDependencies != null, test the leaf Package entries against
                    //  what has already been added to the mergePackageDependencies data structure.
                    //
                    //  Add Unique Package entries.
                    //  For this Merge Tool a Package is defined as PackageGuid
                    //
                    //   ABORT THE MERGE WITH FAIL if the PACKAGE VERSION NUMBERS ARE DIFFERENT
                    //     between Leaf modules
                    //
                    //  Version 0.1 of the tool
                    //      SupArchList, if it exists, must be identical for all Leaf Modules
                    //  Probable Enhancement
                    //      Just specify the lowest common denominator
                    //
                    //  Create the working copy if one does not exist!
                    // TODO: CODE GOES HERE
                    if (mergePackageDependencies == null)
                        mergePackageDependencies = PackageDependenciesDocument.Factory.newInstance()
                                                                                      .addNewPackageDependencies();

                    PackageDependencies leafPackageDependencies = leafMsa.getPackageDependencies();
                    if (leafPackageDependencies.sizeOfPackageArray() > 0) {
                        for (int index = 0; index < leafPackageDependencies.sizeOfPackageArray(); index++) {
                            String packageGuid = leafPackageDependencies.getPackageArray(index).getPackageGuid();
                            String test = checkDuplicateStrings(packageGuid, aPackageList);
                            if (test.length() > 0) {
                                mergePackageDependencies.addNewPackage();
                                mergePackageDependencies
                                                        .setPackageArray(packageDependenciesIndex++,
                                                                         leafPackageDependencies.getPackageArray(index));
                            }
                        }
                    }
                } // endif PackageDependencies

                if (leafMsa.isSetProtocols()) {
                    // TODO: 
                    //  TEST FOR NULL SETTINGS so we don't get an error!
                    //  Add Usage Merging routines
                    //  
                    //  If mergeProtocols == null, create a new mergeProtocols and add
                    //  leaf module's Protocols section to the merge Module's
                    //
                    //  Keep ALL Protocol entries before ProtocolNotify entries!
                    //
                    //  If mergeProtocols != null, check that we have only unique Protocol and ProtocolNotify entries
                    //  IF one Leaf usage is SOMETIMES_PRODUCED, and another Leaf usage is ALWAYS_PRODUCED,
                    //     Mark the Protocol or ProtocolNotify as ALWAYS_PRODUCED
                    //  IF one Leaf usage is SOMETIMES_CONSUMED, and another Leaf usage is ALWAYS_CONSUMED,
                    //     Mark the Protocol or ProtocolNotify as ALWAYS_CONSUMED
                    //  It is permissable to have one PRODUCED and one CONSUMED entry
                    //  TO_START and BY_START cannot be combined in any fashion, if one Leaf uses TO_START
                    //     and another Leaf uses BY_START, create two entries, one for each.
                    //
                    //  First PASS of the TOOL 
                    //      SupArchList must be identical for all Leaf Modules!  Fail the Merge if not, reporting
                    //      this as an error!
                    //  Probable Enhancement
                    //      The SupArchList, if set, must be for the "Lowest Common Denominator"
                    //
                    //  First PASS of the TOOL
                    //      Ignore the FeatureFlag - we are not using it now.
                    //  Probable Enhancement
                    //      FeatureFlags must be identical for each unique Protocol or ProtocolNotify Entry, otherwise fail the Merge
                    //
                    //  HelpText RULE:
                    //  HelpText should be concatenated with a line -- leafFilename -- separator, where leafFilename
                    //  is the leaf Module's path and filename to the MSA file!
                    //
                    // Create the working copy if one does not exist!

                    if (mergeProtocols == null)
                        mergeProtocols = ProtocolsDocument.Factory.newInstance().addNewProtocols();

                    Protocols leafProtocols = leafMsa.getProtocols();
                    // Handle Protocol Entries First
                    if (leafProtocols.sizeOfProtocolArray() > 0) {
                        for (int index = 0; index < leafProtocols.sizeOfProtocolArray(); index++) {
                            String protocolCName = leafProtocols.getProtocolArray(index).getProtocolCName();
                            String test = checkDuplicateStrings(protocolCName, aProtocolList);
                            if (test.length() > 0) {
                                // new Protocol
                                mergeProtocols.addNewProtocol();
                                mergeProtocols.setProtocolArray(protocolIndex++, leafProtocols.getProtocolArray(index));
                            } else {
                                // Found an existing protocol
                                leafUsage = leafProtocols.getProtocolArray(index).getUsage().toString().trim();
                                for (int nidx = 0; nidx < protocolIndex; nidx++) {
                                    if (mergeProtocols.getProtocolArray(nidx).getProtocolCName()
                                                      .contains(protocolCName)) {
                                        // Found one entry that matches.
                                        mergeUsage = mergeProtocols.getProtocolArray(nidx).getUsage().toString().trim();
                                        if (!mergeUsage.contentEquals(leafUsage)) {
                                            // Usages are different
                                            if (checkUsage().trim().contains("DIFFERENT")) {
                                                // We need to check to see if there's another entry
                                                int anotherProtocol = NOTFOUND;
                                                for (int iidx = nidx + 1; iidx < protocolIndex; iidx++) {

                                                    if (mergeProtocols.getProtocolArray(iidx).getUsage().toString()
                                                                      .trim().contains(protocolCName)) {
                                                        anotherProtocol = FOUND;
                                                        mergeUsage = libClassDefs.getLibraryClassList().get(iidx)
                                                                                 .getUsage().toString().trim();
                                                        if (checkProduced()) {
                                                            mergeProtocols
                                                                          .getProtocolArray(nidx)
                                                                          .setUsage(
                                                                                    org.tianocore.UsageTypes.ALWAYS_PRODUCED);
                                                            anotherProtocol = FOUND;
                                                        }
                                                        // Both Usage types are CONSUMED
                                                        if (checkConsumed()) {
                                                            mergeProtocols
                                                                          .getProtocolArray(nidx)
                                                                          .setUsage(
                                                                                    org.tianocore.UsageTypes.ALWAYS_CONSUMED);
                                                            anotherProtocol = FOUND;
                                                        }
                                                        if (((mergeUsage.contains("TO_START")) && (leafUsage
                                                                                                            .contains("TO_START"))))
                                                            anotherProtocol = FOUND;
                                                        if (((mergeUsage.contains("BY_START")) && (leafUsage
                                                                                                            .contains("BY_START"))))
                                                            anotherProtocol = FOUND;
                                                    }
                                                }
                                                if (anotherProtocol == NOTFOUND) {
                                                    mergeProtocols.addNewProtocol();
                                                    mergeProtocols
                                                                  .setProtocolArray(
                                                                                    protocolIndex++,
                                                                                    leafProtocols
                                                                                                 .getProtocolArray(index));
                                                }
                                            } else {
                                                // usage types are either both PRODUCED or CONSUMED
                                                if (checkProduced())
                                                    mergeProtocols.getProtocolArray(nidx)
                                                                  .setUsage(org.tianocore.UsageTypes.ALWAYS_PRODUCED);
                                                if (checkConsumed())
                                                    mergeProtocols.getProtocolArray(nidx)
                                                                  .setUsage(org.tianocore.UsageTypes.ALWAYS_CONSUMED);
                                            }
                                        }
                                    }
                                } // end of Usage Test
                            }
                        }
                    }

                    // Handle ProtocolNotify Entries Second
                    if (leafProtocols.sizeOfProtocolNotifyArray() > 0) {
                        for (int index = 0; index < leafProtocols.sizeOfProtocolNotifyArray(); index++) {
                            String protocolNotifyCName = leafProtocols.getProtocolNotifyArray(index)
                                                                      .getProtocolNotifyCName();
                            String test = checkDuplicateStrings(protocolNotifyCName, aProtocolNotifyList);
                            if (test.length() > 0) {
                                mergeProtocols.addNewProtocolNotify();
                                mergeProtocols.setProtocolNotifyArray(protocolNotifyIndex++,
                                                                      leafProtocols.getProtocolNotifyArray(index));
                            } else {
                                // We have an existing ProtocolNotify Entry
                                leafUsage = leafProtocols.getProtocolNotifyArray(index).getUsage().toString().trim();
                                for (int nidx = 0; nidx < protocolIndex; nidx++) {
                                    if (mergeProtocols.getProtocolNotifyArray(nidx).getProtocolNotifyCName()
                                                      .contains(protocolNotifyCName)) {
                                        // Found one entry that matches.
                                        mergeUsage = mergeProtocols.getProtocolNotifyArray(nidx).getUsage().toString().trim();
                                        if (!mergeUsage.contentEquals(leafUsage)) {
                                            // Usages are different
                                            if (checkUsage().trim().contains("DIFFERENT")) {
                                                // We need to check to see if there's another entry
                                                int anotherProtocol = NOTFOUND;
                                                for (int iidx = nidx + 1; iidx < protocolIndex; iidx++) {

                                                    if (mergeProtocols.getProtocolNotifyArray(iidx).getUsage().toString()
                                                                      .trim().contains(protocolNotifyCName)) {
                                                        anotherProtocol = FOUND;
                                                        mergeUsage = libClassDefs.getLibraryClassList().get(iidx)
                                                                                 .getUsage().toString().trim();
                                                        if (checkProduced()) {
                                                            mergeProtocols
                                                                          .getProtocolNotifyArray(nidx)
                                                                          .setUsage(
                                                                                    org.tianocore.UsageTypes.ALWAYS_PRODUCED);
                                                            anotherProtocol = FOUND;
                                                        }
                                                        // Both Usage types are CONSUMED
                                                        if (checkConsumed()) {
                                                            mergeProtocols
                                                                          .getProtocolNotifyArray(nidx)
                                                                          .setUsage(
                                                                                    org.tianocore.UsageTypes.ALWAYS_CONSUMED);
                                                            anotherProtocol = FOUND;
                                                        }
                                                        if (((mergeUsage.contains("TO_START")) && (leafUsage
                                                                                                            .contains("TO_START"))))
                                                            anotherProtocol = FOUND;
                                                        if (((mergeUsage.contains("BY_START")) && (leafUsage
                                                                                                            .contains("BY_START"))))
                                                            anotherProtocol = FOUND;
                                                    }
                                                }
                                                if (anotherProtocol == NOTFOUND) {
                                                    mergeProtocols.addNewProtocolNotify();
                                                    mergeProtocols
                                                                  .setProtocolNotifyArray(
                                                                                    protocolNotifyIndex++,
                                                                                    leafProtocols
                                                                                                 .getProtocolNotifyArray(index));
                                                }
                                            } else {
                                                // usage types are either both PRODUCED or CONSUMED
                                                if (checkProduced())
                                                    mergeProtocols.getProtocolNotifyArray(nidx)
                                                                  .setUsage(org.tianocore.UsageTypes.ALWAYS_PRODUCED);
                                                if (checkConsumed())
                                                    mergeProtocols.getProtocolNotifyArray(nidx)
                                                                  .setUsage(org.tianocore.UsageTypes.ALWAYS_CONSUMED);
                                            }
                                        }
                                    }
                                } // end of Usage Test
                            } // end of Usage test
                        }
                    }

                } // endif Protocols

                if (leafMsa.isSetEvents()) {
                    // TODO: TEST FOR NULL SETTINGS so we don't get an error!
                    //
                    //  "Unique" Entries are based on EventsTypes:EventGuidCName attributes
                    //    NOTE: The EventGuidCName can appear once and only once in a CreateEvents Section
                    //          The SAME EventGuidCName can appear once and only once in the SignalEvents Section
                    //    Two EventGuidCName entries, one in CreateEvents the other in SignalEvents IS PERMITTED!
                    //
                    //  If mergeEvents == null, create a new mergeEvents and add
                    //  leaf module's Events section to the merge Module's
                    //
                    //  Keep ALL CreateEvents entries before SignalEvents entries!
                    //
                    //  If mergeEvents != null, check that we have only unique CreateEvents and SignalEvents entries
                    //  IF one Leaf usage is SOMETIMES_PRODUCED, and another Leaf usage is ALWAYS_PRODUCED,
                    //     Mark the CreateEvents.EventTypes or SignalEvents.EventTypes as ALWAYS_PRODUCED
                    //  IF one Leaf usage is SOMETIMES_CONSUMED, and another Leaf usage is ALWAYS_CONSUMED,
                    //     Mark the CreateEvents.EventTypes or SignalEvents.EventTypes as ALWAYS_CONSUMED
                    //  It is permissable to have one PRODUCED and one CONSUMED entry
                    //  TO_START and BY_START cannot be combined in any fashion, if one Leaf uses TO_START
                    //     and another Leaf uses BY_START, create two entries, one for each.
                    //
                    //  First PASS of the TOOL 
                    //      SupArchList must be identical for all Leaf Modules!  Fail the Merge if not, reporting
                    //      this as an error!
                    //  Probable Enhancement
                    //      The SupArchList, if set, must be for the "Lowest Common Denominator"
                    //
                    //  First PASS of the TOOL
                    //      Ignore the FeatureFlag - we are not using it now.
                    //  Probable Enhancement
                    //      FeatureFlags must be identical for each unique EventTypes, otherwise fail the Merge
                    //
                    //  The EventTypes.EventType elements must be identical for all instances of
                    //    the EventGuidCName  FAIL THE MERGE WITH ERROR indicating the Leaf file name that 
                    //    was different.
                    //
                    //
                    //  HelpText RULE:
                    //  HelpText should be concatenated with a line -- leafFilename -- separator, where leafFilename
                    //  is the leaf Module's path and filename to the MSA file!
                    //
                    // Create the working copy if one does not exist!
                    // TODO: Code goes here

                    if (mergeCreateEvents == null)
                        mergeCreateEvents = EventsDocument.Events.CreateEvents.Factory.newInstance();

                    Events leafEvents = leafMsa.getEvents();
                    if (leafEvents.getCreateEvents() != null) {
                        Events.CreateEvents leafCreateEvents = leafEvents.getCreateEvents();
                        if (leafCreateEvents.sizeOfEventTypesArray() > 0) {
                            for (int index = 0; index < leafCreateEvents.sizeOfEventTypesArray(); index++) {
                                String EventGuidCName = leafCreateEvents.getEventTypesArray(index).getEventGuidCName();
                                String test = checkDuplicateStrings(EventGuidCName, aCreateEventsList);
                                if (test.length() > 0) {
                                    mergeCreateEvents.addNewEventTypes();
                                    mergeCreateEvents.setEventTypesArray(createEventIndex++,
                                                                         leafEvents.getCreateEvents()
                                                                                   .getEventTypesArray(index));

                                }
                            }
                        }
                    }

                    if (mergeSignalEvents == null)
                        mergeSignalEvents = EventsDocument.Events.SignalEvents.Factory.newInstance();

                    if (leafEvents.getSignalEvents() != null) {
                        Events.SignalEvents leafSignalEvents = leafEvents.getSignalEvents();
                        if (leafSignalEvents.sizeOfEventTypesArray() > 0) {
                            for (int index = 0; index < leafSignalEvents.sizeOfEventTypesArray(); index++) {
                                String EventGuidCName = leafSignalEvents.getEventTypesArray(index).getEventGuidCName();
                                String test = checkDuplicateStrings(EventGuidCName, aSignalEventsList);
                                if (test.length() > 0) {
                                    mergeSignalEvents.addNewEventTypes();
                                    mergeSignalEvents.setEventTypesArray(signalEventIndex++,
                                                                         leafEvents.getSignalEvents()
                                                                                   .getEventTypesArray(index));
                                }
                            }
                        }
                    }
                } // endif Events                

                if (leafMsa.isSetHobs()) {
                    // TODO: TEST FOR NULL SETTINGS so we don't get an error!
                    //
                    //  "Unique" Entries are based on Hobs.HobTypes:HobGuidCName attribute
                    //
                    //  If mergeHobs == null, create a new mergeHobs and add
                    //  leaf module's Hobs section to the merge Module's
                    //
                    //  
                    //  If mergeHobs != null, check that we have only unique Hobs.HobTypes entries
                    //  IF one Leaf usage is SOMETIMES_PRODUCED, and another Leaf usage is ALWAYS_PRODUCED,
                    //     Mark the Hobs.HobTypes as ALWAYS_PRODUCED
                    //  IF one Leaf usage is SOMETIMES_CONSUMED, and another Leaf usage is ALWAYS_CONSUMED,
                    //     Mark the Hobs.HobTypes as ALWAYS_CONSUMED
                    //  It is permissable to have one PRODUCED and one CONSUMED entry
                    //  TO_START and BY_START cannot be combined in any fashion, if one Leaf uses TO_START
                    //     and another Leaf uses BY_START, create two entries, one for each.
                    //
                    //  First PASS of the TOOL 
                    //      SupArchList must be identical for all Leaf Modules!  Fail the Merge if not, reporting
                    //      this as an error!
                    //  Probable Enhancement
                    //      The SupArchList, if set, must be for the "Lowest Common Denominator"
                    //
                    //  First PASS of the TOOL
                    //      Ignore the FeatureFlag - we are not using it now.
                    //  Probable Enhancement
                    //      FeatureFlags must be identical for each unique HobTypes element, otherwise fail the Merge
                    //
                    //  The HobTypes.HobType elements must be identical for all instances of
                    //    the HobGuidCName  FAIL THE MERGE WITH ERROR indicating the Leaf file name that 
                    //    was different.
                    //
                    //
                    //  HelpText RULE:
                    //  HelpText should be concatenated with a line -- leafFilename -- separator, where leafFilename
                    //  is the leaf Module's path and filename to the MSA file!
                    //
                    // Create the working copy if one does not exist!
                    // TODO: Code goes here
                    if (mergeHobs == null)
                        mergeHobs = HobsDocument.Factory.newInstance().addNewHobs();

                    Hobs leafHobs = leafMsa.getHobs();
                    if (leafHobs.sizeOfHobTypesArray() > 0) {
                        for (int index = 0; index < leafHobs.sizeOfHobTypesArray(); index++) {
                            String hobGuidCName = leafHobs.getHobTypesArray(index).getHobType().toString();
                            String test = checkDuplicateStrings(hobGuidCName, aHobsList);
                            if (test.length() > 0) {
                                mergeHobs.addNewHobTypes();
                                mergeHobs.setHobTypesArray(hobsIndex++, leafHobs.getHobTypesArray(index));
                            }
                        }
                    }
                } // endif Hobs                

                if (leafMsa.isSetPPIs()) {
                    // TODO: TEST FOR NULL SETTINGS so we don't get an error!
                    //
                    //  Keep only Unique Ppi or PpiNotify elements, based on the PpiCName and PpiNotifyCName respectively.

                    //  If mergePpi == null, create a new mergePpi and add
                    //  leaf module's PPIs section to the merge Module's
                    //
                    //  Keep ALL Ppi entries before PpiNotify entries!
                    //
                    //  If mergePpi != null, check that we have only unique Ppi and PpiNotify entries
                    //  IF one Leaf usage is SOMETIMES_PRODUCED, and another Leaf usage is ALWAYS_PRODUCED,
                    //     Mark the Ppi or PpiNotify as ALWAYS_PRODUCED
                    //  IF one Leaf usage is SOMETIMES_CONSUMED, and another Leaf usage is ALWAYS_CONSUMED,
                    //     Mark the Ppi or PpiNotify as ALWAYS_CONSUMED
                    //  It is permissable to have one PRODUCED and one CONSUMED entry
                    //  TO_START and BY_START cannot be combined in any fashion, if one Leaf uses TO_START
                    //     and another Leaf uses BY_START, create two entries, one for each.
                    //
                    //  First PASS of the TOOL 
                    //      SupArchList must be identical for all Leaf Modules!  Fail the Merge if not, reporting
                    //      this as an error!
                    //  Probable Enhancement
                    //      The SupArchList, if set, must be for the "Lowest Common Denominator"
                    //
                    //  First PASS of the TOOL
                    //      Ignore the FeatureFlag - we are not using it now.
                    //  Probable Enhancement
                    //      FeatureFlags must be identical for each unique Ppi or PpiNotify Entry, otherwise fail the Merge
                    //
                    //  HelpText RULE:
                    //  HelpText should be concatenated with a line -- leafFilename -- separator, where leafFilename
                    //  is the leaf Module's path and filename to the MSA file!
                    //
                    // Create the working copy if one does not exist!
                    // TODO: Code Goes Here!
                    if (mergePpis == null)
                        mergePpis = PPIsDocument.Factory.newInstance().addNewPPIs();

                    PPIs leafPPIs = leafMsa.getPPIs();
                    // Handle the PPI Entries First
                    if (leafPPIs.sizeOfPpiArray() > 0) {
                        for (int index = 0; index < leafPPIs.sizeOfPpiArray(); index++) {
                            String ppiCName = leafPPIs.getPpiArray(index).getPpiCName();
                            String test = checkDuplicateStrings(ppiCName, aPpiList);
                            if (test.length() > 0) {
                                mergePpis.addNewPpi();
                                mergePpis.setPpiArray(ppiIndex++, leafPPIs.getPpiArray(index));
                            }
                        }
                    }

                    // Handle the PpiNotify Second
                    if (leafPPIs.sizeOfPpiNotifyArray() > 0) {
                        for (int index = 0; index < leafPPIs.sizeOfPpiNotifyArray(); index++) {
                            String ppiNotifyCName = leafPPIs.getPpiNotifyArray(index).getPpiNotifyCName();
                            String test = checkDuplicateStrings(ppiNotifyCName, aPpiNotifyList);
                            if (test.length() > 0) {
                                mergePpis.addNewPpiNotify();
                                mergePpis.setPpiNotifyArray(ppiNotifyIndex++, leafPPIs.getPpiNotifyArray(index));
                            }
                        }
                    }

                } // endif Ppis

                if (leafMsa.isSetVariables()) {
                    // TODO: TEST FOR NULL SETTINGS so we don't get an error!
                    //
                    //  Keep only Unique Variable elements, based on the VariableName element.
                    //
                    //  If mergeVariables == null, create a new mergeVariables and add
                    //  leaf module's Variables section to the merge Module's
                    //
                    //  If mergeVariables != null, check that we have only unique Variable entries
                    //  IF one Leaf usage is SOMETIMES_PRODUCED, and another Leaf usage is ALWAYS_PRODUCED,
                    //     Mark the Variable as ALWAYS_PRODUCED
                    //  IF one Leaf usage is SOMETIMES_CONSUMED, and another Leaf usage is ALWAYS_CONSUMED,
                    //     Mark the Variable as ALWAYS_CONSUMED
                    //  It is permissable to have one PRODUCED and one CONSUMED entry
                    //  TO_START and BY_START cannot be combined in any fashion, if one Leaf uses TO_START
                    //     and another Leaf uses BY_START, create two entries, one for each.
                    //
                    //  First PASS of the TOOL 
                    //      SupArchList must be identical for all Leaf Modules!  Fail the Merge if not, reporting
                    //      this as an error!
                    //  Probable Enhancement
                    //      The SupArchList, if set, must be for the "Lowest Common Denominator"
                    //
                    //  First PASS of the TOOL
                    //      Ignore the FeatureFlag - we are not using it now.
                    //  Probable Enhancement
                    //      FeatureFlags must be identical for each unique Variable entry, otherwise fail the Merge
                    //
                    //  HelpText RULE:
                    //  HelpText should be concatenated with a line -- leafFilename -- separator, where leafFilename
                    //  is the leaf Module's path and filename to the MSA file!
                    //
                    // Create the working copy if one does not exist!
                    // TODO: Code Goes Here!
                    if (mergeVariables == null)
                        mergeVariables = VariablesDocument.Factory.newInstance().addNewVariables();

                    Variables leafVariables = leafMsa.getVariables();
                    if (leafVariables.sizeOfVariableArray() > 0) {
                        for (int index = 0; index < leafVariables.sizeOfVariableArray(); index++) {
                            String variableGuidCName = leafVariables.getVariableArray(index).getGuidCName();
                            String test = checkDuplicateStrings(variableGuidCName, aVariablesList);
                            if (test.length() > 0) {
                                mergeVariables.addNewVariable();
                                mergeVariables
                                              .setVariableArray(variablesIndex++, leafVariables.getVariableArray(index));
                            }
                        }
                    }

                }// endif Variables

                if (leafMsa.isSetBootModes()) {
                    // TODO: TEST FOR NULL SETTINGS so we don't get an error!
                    //
                    //  Keep only Unique BootMode elements, based on the BootModeName Attribute.
                    //
                    //  If mergeBootModes == null, create a new mergeBootModes and add
                    //  leaf module's BootModes section to the merge Module's
                    //
                    //  If mergeBootModes != null, check that we have only unique BootMode entries
                    //  IF one Leaf usage is SOMETIMES_PRODUCED, and another Leaf usage is ALWAYS_PRODUCED,
                    //     Mark the BootMode as ALWAYS_PRODUCED
                    //  IF one Leaf usage is SOMETIMES_CONSUMED, and another Leaf usage is ALWAYS_CONSUMED,
                    //     Mark the BootMode as ALWAYS_CONSUMED
                    //  It is permissable to have one PRODUCED and one CONSUMED entry
                    //  TO_START and BY_START cannot be combined in any fashion, if one Leaf uses TO_START
                    //     and another Leaf uses BY_START, create two entries, one for each.
                    //
                    //  First PASS of the TOOL 
                    //      SupArchList must be identical for all Leaf Modules!  Fail the Merge if not, reporting
                    //      this as an error!
                    //  Probable Enhancement
                    //      The SupArchList, if set, must be for the "Lowest Common Denominator"
                    //
                    //  First PASS of the TOOL
                    //      Ignore the FeatureFlag - we are not using it now.
                    //  Probable Enhancement
                    //      FeatureFlags must be identical for each unique BootMode entry, otherwise fail the Merge
                    //
                    //  HelpText RULE:
                    //  HelpText should be concatenated with a line -- leafFilename -- separator, where leafFilename
                    //  is the leaf Module's path and filename to the MSA file!
                    //
                    // Create the working copy if one does not exist!
                    // TODO: Code Goes Here!
                    if (mergeBootModes == null)
                        mergeBootModes = BootModesDocument.Factory.newInstance().addNewBootModes();

                    BootModes leafBootModes = leafMsa.getBootModes();
                    if (leafBootModes.sizeOfBootModeArray() > 0) {
                        for (int index = 0; index < leafBootModes.sizeOfBootModeArray(); index++) {
                            String bootModeName = leafBootModes.getBootModeArray(index).getBootModeName().toString();
                            String test = checkDuplicateStrings(bootModeName, aBootModesList);
                            if (test.length() > 0) {
                                mergeBootModes.addNewBootMode();
                                mergeBootModes
                                              .setBootModeArray(bootModesIndex++, leafBootModes.getBootModeArray(index));
                            }
                        }
                    }

                }// endif BootMode

                if (leafMsa.isSetSystemTables()) {
                    // TODO: TEST FOR NULL SETTINGS so we don't get an error!
                    //
                    //  Keep only Unique SystemTableCNames elements, based on the SystemTableCName element.
                    //
                    //  If mergeSystemTables == null, create a new mergeSystemTables and add
                    //  leaf module's Variables section to the merge Module's
                    //
                    //  If mergeSystemTables != null, check that we have only unique SystemTableCNames entries
                    //  IF one Leaf usage is SOMETIMES_PRODUCED, and another Leaf usage is ALWAYS_PRODUCED,
                    //     Mark the SystemTableCName as ALWAYS_PRODUCED
                    //  IF one Leaf usage is SOMETIMES_CONSUMED, and another Leaf usage is ALWAYS_CONSUMED,
                    //     Mark the SystemTableCName as ALWAYS_CONSUMED
                    //  It is permissable to have one PRODUCED and one CONSUMED entry
                    //  TO_START and BY_START cannot be combined in any fashion, if one Leaf uses TO_START
                    //     and another Leaf uses BY_START, create two entries, one for each.
                    //
                    //  First PASS of the TOOL 
                    //      SupArchList must be identical for all Leaf Modules!  Fail the Merge if not, reporting
                    //      this as an error!
                    //  Probable Enhancement
                    //      The SupArchList, if set, must be for the "Lowest Common Denominator"
                    //
                    //  First PASS of the TOOL
                    //      Ignore the FeatureFlag - we are not using it now.
                    //  Probable Enhancement
                    //      FeatureFlags must be identical for each unique SystemTableCNames entry, otherwise fail the Merge
                    //
                    //  HelpText RULE:
                    //  HelpText should be concatenated with a line -- leafFilename -- separator, where leafFilename
                    //  is the leaf Module's path and filename to the MSA file!
                    //
                    // Create the working copy if one does not exist!
                    // TODO: Code Goes Here!
                    if (mergeSystemTables == null)
                        mergeSystemTables = SystemTablesDocument.Factory.newInstance().addNewSystemTables();

                    SystemTables leafSystemTables = leafMsa.getSystemTables();
                    if (leafSystemTables.sizeOfSystemTableCNamesArray() > 0) {
                        for (int index = 0; index < leafSystemTables.sizeOfSystemTableCNamesArray(); index++) {
                            String systemTableCName = leafSystemTables.getSystemTableCNamesArray(index)
                                                                      .getSystemTableCName();
                            String test = checkDuplicateStrings(systemTableCName, aSystemTablesList);
                            if (test.length() > 0) {
                                mergeSystemTables.addNewSystemTableCNames();
                                mergeSystemTables
                                                 .setSystemTableCNamesArray(
                                                                            systemTableIndex++,
                                                                            leafSystemTables
                                                                                            .getSystemTableCNamesArray(index));
                            }
                        }
                    }

                }// endif SystemTables

                if (leafMsa.isSetDataHubs()) {
                    // TODO: TEST FOR NULL SETTINGS so we don't get an error!
                    //
                    //  Keep only Unique DataHubs elements, based on the DataHubRecord.DataHubCName element.
                    //
                    //  If mergeDataHubs == null, create a new mergeDataHubs and add
                    //  leaf module's DataHubs section to the merge Module's
                    //
                    //  If mergeDataHubs != null, check that we have only unique DataHubRecord entries
                    //  IF one Leaf usage is SOMETIMES_PRODUCED, and another Leaf usage is ALWAYS_PRODUCED,
                    //     Mark the DataHubCName as ALWAYS_PRODUCED
                    //  IF one Leaf usage is SOMETIMES_CONSUMED, and another Leaf usage is ALWAYS_CONSUMED,
                    //     Mark the DataHubCName as ALWAYS_CONSUMED
                    //  It is permissable to have one PRODUCED and one CONSUMED entry
                    //  TO_START and BY_START cannot be combined in any fashion, if one Leaf uses TO_START
                    //     and another Leaf uses BY_START, create two entries, one for each.
                    //
                    //  First PASS of the TOOL 
                    //      SupArchList must be identical for all Leaf Modules!  Fail the Merge if not, reporting
                    //      this as an error!
                    //  Probable Enhancement
                    //      The SupArchList, if set, must be for the "Lowest Common Denominator"
                    //
                    //  First PASS of the TOOL
                    //      Ignore the FeatureFlag - we are not using it now.
                    //  Probable Enhancement
                    //      FeatureFlags must be identical for each unique DataHubRecord entry, otherwise fail the Merge
                    //
                    //  HelpText RULE:
                    //  HelpText should be concatenated with a line -- leafFilename -- separator, where leafFilename
                    //  is the leaf Module's path and filename to the MSA file!
                    //
                    // Create the working copy if one does not exist!
                    // TODO: Code Goes Here!
                    if (mergeDataHubs == null)
                        mergeDataHubs = DataHubsDocument.Factory.newInstance().addNewDataHubs();

                    DataHubs leafDataHubs = leafMsa.getDataHubs();
                    if (leafDataHubs.sizeOfDataHubRecordArray() > 0) {
                        for (int index = 0; index < leafDataHubs.sizeOfDataHubRecordArray(); index++) {
                            String dataHubCName = leafDataHubs.getDataHubRecordArray(index).getDataHubCName();
                            String test = checkDuplicateStrings(dataHubCName, aDataHubsList);
                            if (test.length() > 0) {
                                mergeDataHubs.addNewDataHubRecord();
                                mergeDataHubs.setDataHubRecordArray(dataHubsIndex++,
                                                                    leafDataHubs.getDataHubRecordArray(index));
                            }
                        }
                    }

                }// endif DataHubs

                if (leafMsa.isSetHiiPackages()) {
                    // TODO: TEST FOR NULL SETTINGS so we don't get an error!
                    //
                    //  Keep only Unique HiiPackage elements, based on the HiiPackage.HiiCName element.
                    //
                    //  If mergeHiiPackages == null, create a new mergeHiiPackages and add
                    //  leaf module's DataHubs section to the merge Module's
                    //
                    //  If mergeHiiPackages != null, check that we have only unique HiiPackage entries
                    //  IF one Leaf usage is SOMETIMES_PRODUCED, and another Leaf usage is ALWAYS_PRODUCED,
                    //     Mark the HiiPackage as ALWAYS_PRODUCED
                    //  IF one Leaf usage is SOMETIMES_CONSUMED, and another Leaf usage is ALWAYS_CONSUMED,
                    //     Mark the HiiPackage as ALWAYS_CONSUMED
                    //  It is permissable to have one PRODUCED and one CONSUMED entry
                    //  TO_START and BY_START cannot be combined in any fashion, if one Leaf uses TO_START
                    //     and another Leaf uses BY_START, create two entries, one for each.
                    //
                    //  First PASS of the TOOL 
                    //      SupArchList must be identical for all Leaf Modules!  Fail the Merge if not, reporting
                    //      this as an error!
                    //  Probable Enhancement
                    //      The SupArchList, if set, must be for the "Lowest Common Denominator"
                    //
                    //  First PASS of the TOOL
                    //      Ignore the FeatureFlag - we are not using it now.
                    //  Probable Enhancement
                    //      FeatureFlags must be identical for each unique HiiPackage entry, otherwise fail the Merge
                    //
                    //  HelpText RULE:
                    //  HelpText should be concatenated with a line -- leafFilename -- separator, where leafFilename
                    //  is the leaf Module's path and filename to the MSA file!
                    //
                    // Create the working copy if one does not exist!
                    // TODO: Code Goes Here!
                    if (mergeHiiPackages == null)
                        mergeHiiPackages = HiiPackagesDocument.Factory.newInstance().addNewHiiPackages();

                    HiiPackages leafHiiPackages = leafMsa.getHiiPackages();
                    if (leafHiiPackages.sizeOfHiiPackageArray() > 0) {
                        for (int index = 0; index < leafHiiPackages.sizeOfHiiPackageArray(); index++) {
                            String hiiCName = leafHiiPackages.getHiiPackageArray(index).getHiiCName();
                            String test = checkDuplicateStrings(hiiCName, aHiiPackagesList);
                            if (test.length() > 0) {
                                mergeHiiPackages.addNewHiiPackage();
                                mergeHiiPackages.setHiiPackageArray(hiiPackageIndex++,
                                                                    leafHiiPackages.getHiiPackageArray(index));
                            }
                        }
                    }

                }// endif HiiPackage

                if (leafMsa.isSetGuids()) {
                    // TODO: TEST FOR NULL SETTINGS so we don't get an error!
                    //
                    //  Keep only Unique Guids elements, based on the GuidCNames.GuidCName element.
                    //
                    //  If mergeGuids == null, create a new mergeGuids and add
                    //  leaf module's Guids section to the merge Module's
                    //
                    //  If mergeGuids != null, check that we have only unique GuidCNames entries
                    //  IF one Leaf usage is SOMETIMES_PRODUCED, and another Leaf usage is ALWAYS_PRODUCED,
                    //     Mark the GuidCNames as ALWAYS_PRODUCED
                    //  IF one Leaf usage is SOMETIMES_CONSUMED, and another Leaf usage is ALWAYS_CONSUMED,
                    //     Mark the GuidCNames as ALWAYS_CONSUMED
                    //  It is permissable to have one PRODUCED and one CONSUMED entry
                    //  TO_START and BY_START cannot be combined in any fashion, if one Leaf uses TO_START
                    //     and another Leaf uses BY_START, create two entries, one for each.
                    //
                    //  First PASS of the TOOL 
                    //      SupArchList must be identical for all Leaf Modules!  Fail the Merge if not, reporting
                    //      this as an error!
                    //  Probable Enhancement
                    //      The SupArchList, if set, must be for the "Lowest Common Denominator"
                    //
                    //  First PASS of the TOOL
                    //      Ignore the FeatureFlag - we are not using it now.
                    //  Probable Enhancement
                    //      FeatureFlags must be identical for each unique GuidCNames entry, otherwise fail the Merge
                    //
                    //  HelpText RULE:
                    //  HelpText should be concatenated with a line -- leafFilename -- separator, where leafFilename
                    //  is the leaf Module's path and filename to the MSA file!
                    //
                    // Create the working copy if one does not exist!
                    // TODO: Code Goes Here!
                    if (mergeGuids == null)
                        mergeGuids = GuidsDocument.Factory.newInstance().addNewGuids();

                    Guids leafGuids = leafMsa.getGuids();
                    if (leafGuids.sizeOfGuidCNamesArray() > 0) {
                        for (int index = 0; index < leafGuids.sizeOfGuidCNamesArray(); index++) {
                            String hiiCName = leafGuids.getGuidCNamesArray(index).getGuidCName();
                            String test = checkDuplicateStrings(hiiCName, aGuidsList);
                            if (test.length() > 0) {
                                mergeGuids.addNewGuidCNames();
                                mergeGuids.setGuidCNamesArray(guidsIndex++, leafGuids.getGuidCNamesArray(index));
                            }
                        }
                    }

                }// endif GuidCNames

                if (leafMsa.isSetExterns()) {
                    // TODO: TEST FOR NULL SETTINGS so we don't get an error!
                    //
                    // FAIL THE MERGE if Externs.PcdIsDriver is present
                    // FAIL THE MERGE if Externs.TianoR8FlashMap_h is TRUE
                    //
                    //  Keep only Unique Extern elements, based on the Extern.* elements.
                    //
                    //  If mergeExterns == null, create a new mergeExterns and add
                    //  leaf module's Extern section to the merge Module's
                    //
                    //  If mergeExterns != null, check that we have only unique Extern.* entries
                    //
                    //  After storing the initial LEAF MODULE'S SPECIFICATION SECTION
                    //  ALL other Leaf Modules must declare the exact same specifications
                    //  If they do not, FAIL the MERGE with an error message, printing the
                    //  name of the leaf MSA that did not match, along with
                    //  Expected: from the merge module's specification list
                    //  Got:      from the leaf file that fails!
                    //
                    //  For Each <Extern>
                    //    For each pair of <ModuleEntryPoint> and/or <ModuleUnloadImage>
                    //          The ModuleUnloadImage value must be identical for an identical pair of ModuleEntryPoint values
                    //          If not, FAIL THE MERGE, giving the current leaf MSA filename as the failure, along with the
                    //          additional error information as follows:
                    //      -- leafFilename --
                    //      ModuleEntryPoint: 
                    //      Expected ModuleUnloadImage:  fromMergeModule
                    //      Got ModuleUnloadImage:       fromLeaf
                    //      Merge Aborted!
                    //    More than one <Extern> Section with a pair of <ModuleEntryPoint><ModuleUnloadImage> is allowed
                    //
                    //    For each pair of one <Constructor> and/or one <Destructor> elements
                    //       The <Extern> section containing the <Constructor> <Destructor> pairs 
                    //        The Destructor value in all leaf modules must be identical for all Constructor elements that are identical.
                    //      More than one <Extern> Section with Constructor/Destructor pair is permitted.
                    //
                    //  For each Set four elements, DriverBinding, ComponentName, DriverConfig and DriverDiag,
                    //      1 DriverBinding and
                    //         0 or 1 ComponentName and/or
                    //         0 or 1 DriverConfig and/or
                    //         0 or 1 DriverDiag 
                    //        elements must appear in 1 <Extern> Section.
                    //
                    //      A ComponentName cannot be used without a DriverBinding element.
                    //      A DriverConfig element cannot appear without a DriverBinding element. 
                    //      A DriverDiag element cannot appear without a DriverBinding element
                    //      These elements are matched within a single <Extern> Section uniquely defined by the DriverBinding element.  
                    //    Multiple <Extern> sections of this type are permitted.
                    //
                    //  Each pair of SetVirtualAddressMapCallBack and ExitBootServiceCallBack elements MUST 
                    //  BE in one Extern Section.  ONE AND ONLY ONE of this section is permitted.
                    //
                    //  First PASS of the TOOL 
                    //      SupArchList must be identical for all Leaf Modules!  Fail the Merge if not, reporting
                    //      this as an error!
                    //  Probable Enhancement
                    //      The SupArchList, if set, must be for the "Lowest Common Denominator"
                    //
                    //  First PASS of the TOOL
                    //      Ignore the FeatureFlag - we are not using it now.
                    //  Probable Enhancement
                    //      FeatureFlags must be identical for each unique Extern entry, otherwise fail the Merge
                    //
                    // Create the working copy if one does not exist!
                    // TODO: Code Goes Here!
                    if (mergeExterns == null)
                        mergeExterns = ExternsDocument.Factory.newInstance().addNewExterns();

                    Externs leafExterns = leafMsa.getExterns();
                    // PCD IS DRIVER NOT ALLOWED IN A MERGED Module
                    if (leafExterns.isSetPcdIsDriver()) {
                        System.out.println("The Module: " + leafFilename + " is a PCD Driver and CANNOT BE MERGED!");
                        System.out.println("Merge Aborted");
                        System.err.flush();
                        System.exit(FAIL);
                    }

                    // TIANO R8 FLASHMAP.H NOT ALLOWED IN A MERGED Module
                    if (leafExterns.isSetTianoR8FlashMapH()) {
                        System.out.println("The Module: " + leafFilename
                                           + " set the Tiano R8 FlashMap.h Flag and CANNOT BE MERGED!");
                        System.out.println("Merge Aborted");
                        System.err.flush();
                        System.exit(FAIL);
                    }

                    // Add the Specification Array, one time only.
                    if (leafExterns.sizeOfSpecificationArray() > 0) {
                        for (int index = 0; index < leafExterns.sizeOfSpecificationArray(); index++) {
                            String spec = leafExterns.getSpecificationArray(index);
                            String test = checkSpecs(spec, leafFilename, aSpecArray);
                            if (test.length() > 0) {
                                mergeExterns.addNewSpecification();
                                mergeExterns.setSpecificationArray(specIndex++, test);
                            }
                        }
                    }

                    if (leafExterns.sizeOfExternArray() > 0) {
                        for (int index = 0; index < leafExterns.sizeOfExternArray(); index++) {
                            String test = "";
                            if (leafExterns.getExternArray(index).isSetModuleEntryPoint()) {
                                // ModuleEntryPoint, if an Unload Image is paired with
                                // the Module Entry point, it will piggy back on the
                                // Module Entry Point Extern
                                String moduleEntryPoint = leafExterns.getExternArray(index).getModuleEntryPoint();
                                test = checkDuplicateStrings(moduleEntryPoint, aEntryPointList);

                            } else if (leafExterns.getExternArray(index).isSetModuleUnloadImage()) {
                                // Module Unload Image is here in case there is no
                                // Entry Point - not very typical
                                String moduleUnloadImage = leafExterns.getExternArray(index).getModuleUnloadImage();
                                test = checkDuplicateStrings(moduleUnloadImage, aUnloadImageList);

                            } else if (leafExterns.getExternArray(index).isSetConstructor()) {
                                // Constructors must be unique, if a Destructor is
                                // paired with a constructor, it will pigback on
                                // the constructor
                                String constructor = leafExterns.getExternArray(index).getConstructor();
                                test = checkDuplicateStrings(constructor, aConstructorList);

                            } else if (leafExterns.getExternArray(index).isSetDestructor()) {
                                // Destructors must be unique
                                String destructor = leafExterns.getExternArray(index).getDestructor();
                                test = checkDuplicateStrings(destructor, aDestructorList);

                            } else if (leafExterns.getExternArray(index).isSetDriverBinding()) {
                                // Driver Bindings must be unique
                                // Fixed the MSA files - ComponentName, Driver Config and
                                // Driver Diag statments must be inside of an Extern that 
                                // has a Driver Binding
                                String driverBinding = leafExterns.getExternArray(index).getDriverBinding();
                                test = checkDuplicateStrings(driverBinding, aDriverBindingList);

                            } else if (leafExterns.getExternArray(index).isSetSetVirtualAddressMapCallBack()) {
                                // Handle Virtual Address Map and Exit Boot Services Call Backs
                                // in a single Extern if they are present
                                String virtualAddressMap = leafExterns.getExternArray(index)
                                                                      .getSetVirtualAddressMapCallBack();
                                test = checkDuplicateStrings(virtualAddressMap, aVirtualAddressMapList);

                            } else if (leafExterns.getExternArray(index).isSetExitBootServicesCallBack()) {
                                // Handle a stand alone Exit Boot Services Call Back
                                String exitBootServices = leafExterns.getExternArray(index)
                                                                     .getExitBootServicesCallBack();
                                test = checkDuplicateStrings(exitBootServices, aExitBootServicesList);
                            } else {
                                // Unknown Extern  FAIL  - May be an invalid Component Name in it's own Extern Statement
                                System.out.println("Unknown EXTERN defined in Module: " + leafFilename);
                                System.out.println("Value: " + leafExterns.getExternArray(index).toString());
                                System.out.println("Merge Aborted!");
                                System.err.flush();
                                System.exit(FAIL);
                            }

                            if (test.length() > 0) {
                                mergeExterns.addNewExtern();
                                mergeExterns.setExternArray(externsIndex++, leafExterns.getExternArray(index));
                            }
                        }
                    }
                }// endif mergeExterns

                if (leafMsa.isSetPcdCoded()) {
                    // TODO: TEST FOR NULL SETTINGS so we don't get an error!
                    //
                    //  Keep only Unique PcdCoded elements, based on the PcdCoded.PcdEntry.C_Name element.
                    //
                    //  If mergePcdCoded == null, create a new mergePcdCoded and add
                    //  leaf module's PcdCoded section to the merge Module's
                    //
                    //  If mergePcdCoded != null, check that we have only unique PcdEntry entries
                    //  IF one Leaf usage is SOMETIMES_PRODUCED, and another Leaf usage is ALWAYS_PRODUCED,
                    //     Mark the PcdEntry as ALWAYS_PRODUCED
                    //  IF one Leaf usage is SOMETIMES_CONSUMED, and another Leaf usage is ALWAYS_CONSUMED,
                    //     Mark the PcdEntry as ALWAYS_CONSUMED
                    //  It is permissable to have one PRODUCED and one CONSUMED entry
                    //  TO_START and BY_START cannot be combined in any fashion, if one Leaf uses TO_START
                    //     and another Leaf uses BY_START, create two entries, one for each.
                    //
                    //  First PASS of the TOOL 
                    //      SupArchList must be identical for all Leaf Modules!  Fail the Merge if not, reporting
                    //      this as an error!
                    //  Probable Enhancement
                    //      The SupArchList, if set, must be for the "Lowest Common Denominator"
                    //
                    //  First PASS of the TOOL
                    //      Ignore the FeatureFlag - we are not using it now.
                    //  Probable Enhancement
                    //      FeatureFlags must be identical for each unique HiiPackage entry, otherwise fail the Merge
                    //
                    //  First PASS of the TOOL
                    //      Ignore the PcdEntry:Usage Attribute
                    //  Probable Enhancment
                    //      Have Usage Combined like was done for the Library Class
                    //
                    //  First PASS of the TOOL
                    //      If Different PcdItemTypes, Abort The MERGE
                    //
                    //  Probably Enhancement
                    //      The PcdItemType Should be checked using the following rules
                    //          Feature Flag MUST ALWAYS BE A FEATURE FLAG
                    //          If different Item Types occur, mark the PCD as DYNAMIC
                    //
                    //  HelpText RULE:
                    //  HelpText should be concatenated with a line -- leafFilename -- separator, where leafFilename
                    //  is the leaf Module's path and filename to the MSA file!
                    //
                    // Create the working copy if one does not exist!
                    // TODO: Code Goes Here!
                    if (mergePcdCoded == null)
                        mergePcdCoded = PcdCodedDocument.Factory.newInstance().addNewPcdCoded();

                    PcdCoded leafPcdCoded = leafMsa.getPcdCoded();
                    if (leafPcdCoded.sizeOfPcdEntryArray() > 0) {
                        for (int index = 0; index < leafPcdCoded.sizeOfPcdEntryArray(); index++) {
                            String pcdCName = leafPcdCoded.getPcdEntryArray(index).getCName();
                            String pcdItemType = leafPcdCoded.getPcdEntryArray(index).getPcdItemType().toString();
                            String test = checkPcd(pcdCName, pcdItemType, leafFilename, aPcdCNameList);
                            if (test.length() > 0) {
                                mergePcdCoded.addNewPcdEntry();
                                mergePcdCoded.setPcdEntryArray(pcdIndex++, leafPcdCoded.getPcdEntryArray(index));
                            }
                        }
                    }

                }// endif PcdCoded

                if (leafMsa.isSetModuleBuildOptions()) {
                    // TODO: TEST FOR NULL SETTINGS so we don't get an error!
                    //
                    // Any element that appear within a leaf's ModuleBuildOptions should be appended to
                    // the Merge Module's BuildOptions section.
                    //
                    //  NO ATTEMPT IS MADE TO VERIFY UNIQUENESS ON ANYTHING WITHIN THIS SECTION!
                    //
                    // Create the working copy if one does not exist!
                    // if (mergeBuildOptions == null)
                    //    mergeBuildOptions = ModuleBuildOptionsDocument.Factory.newInstance().addNewModuleBuildOptions();

                    // ModuleBuildOptions leafModuleBuildOptions = leafMsa.getModuleBuildOptions();

                    // mergeBuildOptions.addNewModuleBuildOptions();
                    // mergeBuildOptions.setModuleBuildOptions(leafModuleBuildOptions);

                    //
                    // TODO: Code Goes Here!
                } // endif ModuleBuildOptions

                // Need to process any UserExtensions here too.
                if (leafMsa.getUserExtensionsList() != null) {

                    if (mergeUserExtensions == null)
                        mergeUserExtensions = UserExtensionsDocument.Factory.newInstance().addNewUserExtensions();

                    // for (int index = 0; index < leafMsa.getUserExtensionsList().size(); index++)

                }

            } // Completed parsing all leaf files.

            header.setAbstract(Abstract);
            header.setCopyright(Copyright);
            header.setDescription(Description);
            License mLicense = License.Factory.newInstance();
            mLicense.setStringValue(licenseTxt);
            header.setLicense(mLicense);
            if ((DEBUG > 0) || (VERBOSE > 0))
                System.out.println("Merged Module supports: " + sArchitectures + " architectures.");
            List<String> lArchitectures = new ArrayList<String>();
            String s[] = sArchitectures.replace("  ", " ").trim().split(" ");
            for (int idx = 0; idx < s.length; idx++) {
                lArchitectures.add(s[idx]);
                if (DEBUG > 7)
                    System.out.println("Adding architecture: " + s[idx]);
            }
            moduleDefs.setSupportedArchitectures(lArchitectures);

        } // endif mergeMsaFile == null

        if ((uiName != null) && (uiName.length() > 0) && (result == PASS)) {
            // TODO: Stub for replacing the msaFile UiName
            if ((DEBUG > 0) || (VERBOSE > 0))
                System.out.println("Updating the uiName: " + uiName);
            header.setModuleName(uiName);
        }

        if ((baseName != null) && (baseName.length() > 0) && (result == PASS)) {
            if ((DEBUG > 0) || (VERBOSE > 0))
                System.out.println("Setting the Output Filename:" + baseName);
            moduleDefs.setOutputFileBasename(baseName);
        }

        if (result == PASS) {
            // TODO: Stub to write out the new MSA file
            File outMsa = new File(msaFilename);
            try {
                if (DEBUG > 2)
                    System.out.println("SAVING new MSA FILE: " + msaFilename);

                mergeMsaFile.setMsaHeader(header);
                mergeMsaFile.setModuleDefinitions(moduleDefs);
                // ALL THE REST OF THE SECTIONS ARE OPTIONAL
                // SO check that they are not null before adding them to the merged MSA file!
                if (libClassDefs != null)
                    mergeMsaFile.setLibraryClassDefinitions(libClassDefs);
                if (mergeSourceFiles != null)
                    mergeMsaFile.setSourceFiles(mergeSourceFiles);
                if (mergePackageDependencies != null)
                    mergeMsaFile.setPackageDependencies(mergePackageDependencies);
                if (mergeProtocols != null)
                    mergeMsaFile.setProtocols(mergeProtocols);

                if ((mergeCreateEvents != null) || (mergeSignalEvents != null)) {
                    if (mergeEvents == null)
                        mergeEvents = EventsDocument.Factory.newInstance().addNewEvents();

                    if (mergeCreateEvents.getEventTypesList().size() > 0) {
                        mergeEvents.addNewCreateEvents();
                        mergeEvents.setCreateEvents(mergeCreateEvents);
                    }
                    if (mergeSignalEvents.getEventTypesList().size() > 0) {
                        mergeEvents.addNewSignalEvents();
                        mergeEvents.setSignalEvents(mergeSignalEvents);
                    }

                    mergeMsaFile.setEvents(mergeEvents);
                }

                if (mergeHobs != null)
                    mergeMsaFile.setHobs(mergeHobs);

                if (mergePpis != null)
                    mergeMsaFile.setPPIs(mergePpis);

                if (mergeVariables != null)
                    mergeMsaFile.setVariables(mergeVariables);

                if (mergeBootModes != null)
                    mergeMsaFile.setBootModes(mergeBootModes);

                if (mergeSystemTables != null)
                    mergeMsaFile.setSystemTables(mergeSystemTables);

                if (mergeDataHubs != null)
                    mergeMsaFile.setDataHubs(mergeDataHubs);

                if (mergeHiiPackages != null)
                    mergeMsaFile.setHiiPackages(mergeHiiPackages);

                if (mergeGuids != null)
                    mergeMsaFile.setGuids(mergeGuids);

                if (mergeExterns != null)
                    mergeMsaFile.setExterns(mergeExterns);

                if (mergePcdCoded != null)
                    mergeMsaFile.setPcdCoded(mergePcdCoded);

                XmlCursor cursor = XmlConfig.setupXmlCursor(mergeMsaFile.newCursor());
                XmlOptions options = XmlConfig.setupXmlOptions();
                ModuleSurfaceAreaDocument msaDoc = ModuleSurfaceAreaDocument.Factory.newInstance();
                msaDoc.addNewModuleSurfaceArea();
                msaDoc.setModuleSurfaceArea(mergeMsaFile);
                msaDoc.save(outMsa, options);
                System.out.println("The Merged MSA file: " + msaFilename + ", has been created!");
            } catch (IOException e) {
                System.out.println("Problem writing the output file: " + msaFilename + " " + e);
                result = FAIL;
            }
        }

        if ((spdFilename != null) && (result == PASS)) {
            // TODO: Stub for adding the msaFile to the <MsaFiles><Filename> in the spdFile
            String msaLine = getPathFromSpd(spdFilename, msaFilename);
            System.out.println("Updating the SPD file (" + spdFilename + ") with: " + msaLine);
            try {
                File spdFile = new File(spdFilename);
                PackageSurfaceAreaDocument spdDoc = PackageSurfaceAreaDocument.Factory.parse(spdFile);
                PackageSurfaceArea spd = spdDoc.getPackageSurfaceArea();

                List<String> msaFilenames = spd.getMsaFiles().getFilenameList();
                msaFilenames.add(msaLine);
                XmlCursor cursor = XmlConfig.setupXmlCursor(spd.newCursor());
                XmlOptions options = XmlConfig.setupXmlOptions();
                spdDoc.save(spdFile, options);
            } catch (IOException e) {
                System.out.println("I/O Exception on spd file: " + spdFilename + " " + e);
            } catch (XmlException x) {
                System.out.println("XML Exception on SPD file: " + spdFilename + " " + x);
            }
        } else if ((spdFilename == null) && (result == PASS)) {

            System.out.println("The file: " + msaFilename + ", must be added to a package file before it can be used!");
        }
        return result;
    }

    public ModuleSurfaceArea getLeafFile(String filename) {
        File leafMsaFile = new File(filename);
        if ((DEBUG > 1) || (VERBOSE > 1))
            System.out.println("Processing MSA File: " + filename);
        try {
            leafMsa = ModuleSurfaceAreaDocument.Factory.parse(leafMsaFile).getModuleSurfaceArea();
            if ((DEBUG > 4) || (VERBOSE > 4))
                System.out.println("Binary: " + leafMsa.getModuleDefinitions().getBinaryModule());
            if (leafMsa.getModuleDefinitions().getBinaryModule()) {
                System.out.println("ERROR: Binary Module was specified in MSA: " + filename);
                System.out.println("Merge Aborted!");
                System.err.flush();
                return null;
            }
        } catch (IOException e) {
            System.out.println("I/O Exception on filename: " + filename + " " + e);
            System.out.println("Merge Aborted!");
            System.err.flush();
            System.exit(FAIL);
        } catch (XmlException x) {
            System.out.println("XML Exception reading file: " + filename + " " + x);
            System.out.println("Merge Aborted!");
            System.err.flush();
            System.exit(FAIL);
        }
        return leafMsa;
    }

    private String getPathFromSpd(String spdFn, String msaFn) {
        String path2Msa = null;

        spdFn = spdFn.replace("\\", "/").trim();
        String s[] = spdFn.split("/");
        String justSpdFilename = s[s.length - 1];

        String Cwd = System.getProperty("user.dir");
        Cwd = Cwd.replace("\\", "/").trim();
        if ((DEBUG > 10) || (VERBOSE > 10)) {
            System.out.println("Current directory = " + Cwd);
        }
        String sp[] = Cwd.split("/");
        int theDirectory = sp.length - (s.length - 1);
        if (DEBUG > 10)
            System.out.println("The Directory length: " + theDirectory + " s.length: " + s.length + " sp.length: "
                               + (sp.length - 1));

        String path2Spd = "";
        for (int ictr = 0; ictr < theDirectory; ictr++) {
            path2Spd += sp[ictr] + "/";
            if (DEBUG > 10)
                System.out.println("Creating path to SPD file: " + path2Spd);
        }

        String testPath2Spd = path2Spd + justSpdFilename;

        File tFile = new File(testPath2Spd);
        if (!tFile.exists()) {
            System.out.println("The specified SPD file, " + spdFn + " does not exist at: " + testPath2Spd);
            System.out.println("Please use the FrameworkWizard to add this MSA file to the package.");
            System.exit(FAIL);
        }
		if (path2Spd.equals(Cwd + "/")) 
		{
			//  .msa file and .spd in the same directory
			path2Msa = msaFn;
		} 
		else 
		{
			path2Msa = Cwd.replace(path2Spd, "");
			path2Msa = path2Msa + "/" + msaFn;
		}

        return path2Msa;
    }

    private String checkDuplicateStrings(String aString, ArrayList<String> aList) {

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

    private String checkSpecs(String specName, String filename, ArrayList<String> aList) {
        // Check Specifications
        // Skip of Specs are identical
        String spec[] = new String[2];
        spec = specName.replace("  ", " ").trim().split(" ");
        String specInMem[] = new String[2];
        if ((DEBUG > 10) || (VERBOSE > 10))
            System.out.println("Specification: " + specName);

        for (int lctr = 0; lctr < aList.size(); lctr++) {
            if (DEBUG > 8)
                System.out.println("Comparing: \n" + specName.replace(" ", "").replace("\n", "") + "\nTo: \n"
                                   + aList.get(lctr).replace(" ", "").replace("\n", "").toString().trim());
            if (specName.replace(" ", "").replace("\n", "").contains(
                                                                     aList.get(lctr).replace(" ", "").replace("\n", "")
                                                                          .toString().trim())) {
                if ((DEBUG > 3) || (VERBOSE > 3))
                    System.out.println("Found a duplicate String, skipping!");
                return "";
            }
            specInMem = aList.get(lctr).replace("  ", " ").trim().split(" ");
            if (spec[0].contentEquals(specInMem[0])) {
                if (!spec[1].contains(specInMem[1])) {
                    System.out.println("Module: " + filename + " is coded to " + specName);
                    System.out.println("Merge needs to be coded to: " + aList.get(lctr));
                    System.out.println("Merge Aborted!");
                    System.err.flush();
                    System.exit(FAIL);
                }
            }

        }
        if ((DEBUG > 3) || (VERBOSE > 3))
            System.out.println("Returning Specification: " + specName);
        aList.add(specName);
        return specName;
    }

    private String checkPcd(String pcdName, String itemType, String filename, ArrayList<String> aList) {

        for (int lctr = 0; lctr < aList.size(); lctr++) {
            if (DEBUG > 8)
                System.out.println("Comparing: \n" + pcdName.replace(" ", "").replace("\n", "") + "\nTo: \n"
                                   + aList.get(lctr).replace(" ", "").replace("\n", "").toString().trim());
            if (pcdName.replace(" ", "").replace("\n", "").contains(
                                                                    aList.get(lctr).replace(" ", "").replace("\n", "")
                                                                         .toString().trim())) {
                if (!aPcdItemTypeList.get(lctr).contains(itemType)) {
                    System.out
                              .println("The Pcd Item Type for " + pcdName + " in file: " + filename + "does not match!");
                    System.out.println("Expected:   " + aPcdItemTypeList.get(lctr));
                    System.out.println("Was set to: " + itemType);
                    System.out.println("Merge Aborted!");
                    System.err.flush();
                    System.exit(FAIL);
                }
                if ((DEBUG > 3) || (VERBOSE > 3))
                    System.out.println("Found a duplicate String, skipping!");
                return "";
            }
        }
        if ((DEBUG > 3) || (VERBOSE > 3))
            System.out.println("Returning UNIQUE String!\n " + pcdName);
        aPcdItemTypeList.add(itemType);
        aList.add(pcdName);
        return pcdName;
    }

    private String checkUsage() {
        String result = "";
        // Usage types are different
        if (((mergeUsage.contains("CONSUMED")) && (leafUsage.contains("PRODUCED")))
            || ((mergeUsage.contains("PRODUCED")) && (leafUsage.contains("CONSUMED")))
            || ((mergeUsage.contains("TO_START")) && (leafUsage.contains("BY_START")))
            || ((mergeUsage.contains("BY_START")) && (leafUsage.contains("TO_START")))) {
            result = "DIFFERENT";
        }
        // Both Usage types are PRODUCED
        if (((mergeUsage.contains("ALWAYS_PRODUCED")) && (leafUsage.contains("SOMETIMES_PRODUCED")))
            || ((mergeUsage.contains("SOMETIMES_PRODUCED")) && (leafUsage.contains("ALWAYS_PRODUCED")))) {
            result = "PRODUCED";
        }

        // Both Usage types are CONSUMED
        if (((mergeUsage.contains("ALWAYS_CONSUMED")) && (leafUsage.contains("SOMETIMES_CONSUMED")))
            || ((mergeUsage.contains("SOMETIMES_CONSUMED")) && (leafUsage.contains("ALWAYS_CONSUMED")))) {
            result = "CONSUMED";
        }
        return result;
    }

    private boolean checkProduced() {
        boolean result = false;

        if (((mergeUsage.contains("ALWAYS_PRODUCED")) && (leafUsage.contains("SOMETIMES_PRODUCED")))
            || ((mergeUsage.contains("SOMETIMES_PRODUCED")) && (leafUsage.contains("ALWAYS_PRODUCED")))) {
            result = true;
        }
        return result;
    }

    private boolean checkConsumed() {
        boolean result = false;

        if (((mergeUsage.contains("ALWAYS_CONSUMED")) && (leafUsage.contains("SOMETIMES_CONSUMED")))
            || ((mergeUsage.contains("SOMETIMES_CONSUMED")) && (leafUsage.contains("ALWAYS_CONSUMED")))) {
            result = true;
        }
        return result;
    }

    private String getPathPartOfLeafMsa(String sFilename) {
        String pathName = "";
        String s[] = sFilename.replace("\\", "/").trim().split("/");
        for (int j = 0; j < (s.length - 1); j++) {
            pathName += s[j] + "/";
        }
        return pathName;
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
