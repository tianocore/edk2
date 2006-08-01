/** @file
  PCDAutoGenAction class.

  This class is to manage how to generate the PCD information into Autogen.c and
  Autogen.h.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.build.pcd.action;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.tianocore.build.global.GlobalData;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.pcd.entity.MemoryDatabaseManager;
import org.tianocore.pcd.entity.Token;
import org.tianocore.pcd.entity.UsageInstance;
import org.tianocore.pcd.exception.BuildActionException;
import org.tianocore.pcd.entity.UsageIdentification;
import org.tianocore.pcd.action.BuildAction;
import org.tianocore.pcd.action.ActionMessage;

/** This class is to manage how to generate the PCD information into Autogen.c and
    Autogen.h.
**/
public class PCDAutoGenAction extends BuildAction {
    ///
    /// The reference of DBManager in GlobalData class.
    ///
    private MemoryDatabaseManager dbManager;

    ///
    /// The identification for a UsageInstance.
    ///
    private UsageIdentification   usageId;

    ///
    /// Whether current autogen is for building library used by current module.
    ///
    private boolean               isBuildUsedLibrary;

    ///
    /// The generated string for header file.
    ///
    private String                hAutoGenString;

    ///
    /// The generated string for C code file.
    ///
    private String                cAutoGenString;

    ///
    /// The name array of <PcdCoded> in a module.
    ///
    private String[]              pcdNameArrayInMsa;

    /**
      Set parameter moduleId

      @param moduleName   the module name parameter.
    **/
    public void setUsageId(UsageIdentification usageId) {
        this.usageId = usageId;
    }

    /**
       set isBuildUsedLibrary parameter.

       @param isBuildUsedLibrary
    **/
    public void setIsBuildUsedLibrary(boolean isBuildUsedLibrary) {
        this.isBuildUsedLibrary = isBuildUsedLibrary;
    }

    /**
       set pcdNameArrayInMsa parameter.

       @param pcdNameArrayInMsa
     */
    public void setPcdNameArrayInMsa(String[] pcdNameArrayInMsa) {
        this.pcdNameArrayInMsa = pcdNameArrayInMsa;
    }

    /**
      Get the output of generated string for header file.

      @return the string of header file for PCD
    **/
    public String OutputH() {
        return hAutoGenString;
    }

    /**
      Get the output of generated string for C Code file.

      @return the string of C code file for PCD
    **/
    public String OutputC() {
        return cAutoGenString;
    }


    /**
        Construct function

        This function mainly initialize some member variable.

        @param moduleId             the identification for module
        @param arch                 the architecture for module
        @param isBuildUsedLibary    Is the current module library.
        @param pcdNameArrayInMsa    the pcd name array got from MSA file.
    **/
    public PCDAutoGenAction(ModuleIdentification moduleId,
                            String               arch,
                            boolean              isBuildUsedLibrary,
                            String[]             pcdNameArrayInMsa) {
        dbManager       = null;
        hAutoGenString  = "";
        cAutoGenString  = "";

        setUsageId(new UsageIdentification(moduleId.getName(),
                                           moduleId.getGuid(),
                                           moduleId.getPackage().getName(),
                                           moduleId.getPackage().getGuid(),
                                           arch,
                                           moduleId.getVersion(),
                                           moduleId.getModuleType()));
        setIsBuildUsedLibrary(isBuildUsedLibrary);
        setPcdNameArrayInMsa(pcdNameArrayInMsa);
    }

    /**
      check the parameter for action class.

      @throws BuildActionException Bad parameter.
    **/
    public void checkParameter() {
    }

    /**
      Core execution function for this action class.

      All PCD information of this module comes from memory dabase. The collection
      work should be done before this action execution.
      Currently, we should generated all PCD information(maybe all dynamic) as array
      in Pei emulated driver for simulating PCD runtime database.

      @throws BuildActionException Failed to execute this aciton class.
    **/
    public void performAction() {
        ActionMessage.debug(this,
                            "Starting PCDAutoGenAction to generate autogen.h and autogen.c!...");
        //
        // Check the PCD memory database manager is valid.
        //
        if(GlobalData.getPCDMemoryDBManager() == null) {
            throw new BuildActionException("Memory database has not been initlizated!");
        }

        dbManager = GlobalData.getPCDMemoryDBManager();

        if(dbManager.getDBSize() == 0) {
            return;
        }

        ActionMessage.debug(this,
                            "PCD memory database contains " + dbManager.getDBSize() + " PCD tokens");

        generateAutogenForModule();
    }

    /**
      Generate the autogen string for a common module.

      All PCD information of this module comes from memory dabase. The collection
      work should be done before this action execution.
    **/
    private void generateAutogenForModule()
    {
        int                   index, index2;
        List<UsageInstance>   usageInstanceArray, usageContext;
        String[]              guidStringArray = null;
        String                guidStringCName = null;
        String                guidString      = null;
        String                moduleName      = usageId.moduleName;
        UsageInstance         usageInstance   = null;
        boolean               found           = false;

        usageInstanceArray = null;
        if (!isBuildUsedLibrary) {
            usageInstanceArray  = dbManager.getUsageInstanceArrayByModuleName(usageId);
            MemoryDatabaseManager.UsageInstanceContext = usageInstanceArray;
            MemoryDatabaseManager.CurrentModuleName    = moduleName;
        } else if ((pcdNameArrayInMsa != null) && (pcdNameArrayInMsa.length > 0)) {
            usageContext = MemoryDatabaseManager.UsageInstanceContext;
            //
            // For building library package, although all module are library, but PCD entries of
            // these library should be used to autogen.
            //
            if (usageContext == null) {
                usageInstanceArray  = dbManager.getUsageInstanceArrayByModuleName(usageId);
            } else {
                usageInstanceArray = new ArrayList<UsageInstance>();

                //
                // Try to find all PCD defined in library's PCD in all <PcdEntry> in module's
                // <ModuleSA> in FPD file.
                //
                for (index = 0; index < pcdNameArrayInMsa.length; index++) {
                    found = false;
                    for (index2 = 0; index2 < usageContext.size(); index2 ++) {
                        if (pcdNameArrayInMsa[index].equalsIgnoreCase(usageContext.get(index2).parentToken.cName)) {
                            usageInstanceArray.add(usageContext.get(index2));
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        //
                        // All library's PCD should instanted in module's <ModuleSA> who
                        // use this library instance. If not, give errors.
                        //
                        throw new BuildActionException (String.format("[PCD Autogen Error] Module %s use library instance %s, the PCD %s " +
                                                                      "is required by this library instance, but can not find " +
                                                                      "it in the %s's <ModuleSA> in FPD file!",
                                                                      MemoryDatabaseManager.CurrentModuleName,
                                                                      moduleName,
                                                                      pcdNameArrayInMsa[index],
                                                                      MemoryDatabaseManager.CurrentModuleName
                                                                      ));
                    }
                }
            }
        }

        if (usageInstanceArray == null) {
            return;
        }

        //
        // Generate all PCD entry for a module.
        //
        for(index = 0; index < usageInstanceArray.size(); index ++) {
            usageInstance = usageInstanceArray.get(index);
            //
            // Before generate any PCD information into autogen.h/autogen.c for a module,
            // generate TokenSpaceGuid array variable firstly. For every dynamicEx type
            // PCD in this module the token, they are all reference to TokenSpaceGuid
            // array.
            //
            if (usageInstanceArray.get(index).modulePcdType == Token.PCD_TYPE.DYNAMIC_EX) {
                guidStringArray = usageInstance.parentToken.tokenSpaceName.split("-");
                guidStringCName = "_gPcd_TokenSpaceGuid_" +
                                  usageInstance.parentToken.tokenSpaceName.replaceAll("-", "_");
                guidString      = String.format("{ 0x%s, 0x%s, 0x%s, {0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s}}",
                                                guidStringArray[0],
                                                guidStringArray[1],
                                                guidStringArray[2],
                                                (guidStringArray[3].substring(0, 2)),
                                                (guidStringArray[3].substring(2, 4)),
                                                (guidStringArray[4].substring(0, 2)),
                                                (guidStringArray[4].substring(2, 4)),
                                                (guidStringArray[4].substring(4, 6)),
                                                (guidStringArray[4].substring(6, 8)),
                                                (guidStringArray[4].substring(8, 10)),
                                                (guidStringArray[4].substring(10, 12)));

                Pattern pattern = Pattern.compile("(" + guidStringCName + ")+?");
                Matcher matcher = pattern.matcher(cAutoGenString + " ");
                //
                // Find whether this guid array variable has been generated into autogen.c
                // For different DyanmicEx pcd token who use same token space guid, the token space
                // guid array should be only generated once.
                //
                if (!matcher.find()) {
                    hAutoGenString += String.format("extern EFI_GUID %s;\r\n", guidStringCName);
                    if (!isBuildUsedLibrary) {
                        cAutoGenString += String.format("GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID %s = %s;\r\n",
                                                        guidStringCName,
                                                        guidString);
                    }
                }
            }

            usageInstance.generateAutoGen(isBuildUsedLibrary);
            //
            // For every PCD entry for this module(usage instance), autogen string would
            // be appand.
            //
            hAutoGenString += usageInstance.getHAutogenStr() + "\r\n";
            cAutoGenString += usageInstance.getCAutogenStr();
        }

        //
        // Work around code, In furture following code should be modified that get
        // these information from Uplevel Autogen tools.
        //
        if (moduleName.equalsIgnoreCase("PcdPeim")) {
            hAutoGenString += MemoryDatabaseManager.PcdPeimHString;
            cAutoGenString += MemoryDatabaseManager.PcdPeimCString;
        } else if (moduleName.equalsIgnoreCase("PcdDxe")) {
            hAutoGenString += MemoryDatabaseManager.PcdDxeHString;
            cAutoGenString += MemoryDatabaseManager.PcdDxeCString;
        }
    }

    /**
      Test case function

      @param argv  paramter from command line
    **/
    public static void main(String argv[]) {

        String WorkSpace = "X:/edk2";
        String logFilePath = WorkSpace  + "/EdkNt32Pkg/Nt32.fpd";

        //
        // At first, CollectPCDAction should be invoked to collect
        // all PCD information from SPD, MSA, FPD.
        //
        PlatformPcdPreprocessActionForBuilding collectionAction = new PlatformPcdPreprocessActionForBuilding();
        GlobalData.initInfo("Tools" + File.separator + "Conf" + File.separator + "FrameworkDatabase.db",
                            WorkSpace,null);

        try {
            collectionAction.perform(WorkSpace,
                                     logFilePath,
                                     ActionMessage.MAX_MESSAGE_LEVEL);
        } catch(Exception e) {
            e.printStackTrace();
        }
    }
}
