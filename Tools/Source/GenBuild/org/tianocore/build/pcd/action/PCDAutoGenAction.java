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

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.tianocore.build.FrameworkBuildTask;
import org.tianocore.build.autogen.CommonDefinition;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.common.logger.EdkLog;
import org.tianocore.pcd.action.BuildAction;
import org.tianocore.pcd.entity.MemoryDatabaseManager;
import org.tianocore.pcd.entity.Token;
import org.tianocore.pcd.entity.UsageIdentification;
import org.tianocore.pcd.entity.UsageInstance;
import org.tianocore.pcd.exception.BuildActionException;

/**
    This class is to manage how to generate the PCD information into Autogen.c
    and Autogen.h.
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
    /// One of PEI_PCD_DRIVER, DXE_PCD_DRIVER, NOT_PCD_DRIVER 
    /// 
    private CommonDefinition.PCD_DRIVER_TYPE pcdDriverType;

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

    private UsageIdentification parentId = null;
    /**
      Set parameter moduleId

      @param moduleName   the module name parameter.
    **/
    public void setUsageId(UsageIdentification usageId) {
        this.usageId = usageId;
    }

    /**
       Set paramter pcdDriverType
       
       @param pcdDriverType the driver type for PCD
    **/
    public void setPcdDriverType(CommonDefinition.PCD_DRIVER_TYPE pcdDriverType) {
        this.pcdDriverType = pcdDriverType;
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
    public String getHAutoGenString() {
        return hAutoGenString;
    }

    /**
      Get the output of generated string for C Code file.

      @return the string of C code file for PCD
    **/
    public String getCAutoGenString() {
        return cAutoGenString;
    }


    /**
        Construct function

        This function mainly initialize some member variable.

        @param moduleId             the identification for module
        @param arch                 the architecture for module
        @param isBuildUsedLibary    Is the current module library.
        @param pcdNameArrayInMsa    the pcd name array got from MSA file.
        @param pcdDriverType        one of PEI_PCD_DRIVER, DXE_PCD_DRIVER,
                                    NOT_PCD_DRIVER
    **/
    public PCDAutoGenAction(ModuleIdentification moduleId,
                            String               arch,
                            boolean              isBuildUsedLibrary,
                            String[]             pcdNameArrayInMsa,
                            CommonDefinition.PCD_DRIVER_TYPE pcdDriverType,
                            ModuleIdentification parentId) {
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
        if (parentId != null) {
            this.parentId = new UsageIdentification(parentId.getName(),
                        parentId.getGuid(),
                        parentId.getPackage().getName(),
                        parentId.getPackage().getGuid(),
                        arch,
                        parentId.getVersion(),
                        parentId.getModuleType());
        }
        setIsBuildUsedLibrary(isBuildUsedLibrary);
        setPcdNameArrayInMsa(pcdNameArrayInMsa);
        setPcdDriverType(pcdDriverType);
    }

    /**
      Override function: check the parameter for action class.

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
        EdkLog.log(EdkLog.EDK_DEBUG, "Starting PCDAutoGenAction to generate autogen.h and autogen.c!...");

        dbManager = GlobalData.getPCDMemoryDBManager();

        if(dbManager.getDBSize() == 0) {
            return;
        }

        EdkLog.log(EdkLog.EDK_DEBUG, "PCD memory database contains " + dbManager.getDBSize() + " PCD tokens.");

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
        
        if (FrameworkBuildTask.multithread) {
            if (parentId == null) {
                usageInstanceArray  = dbManager.getUsageInstanceArrayById(usageId);
            } else if ((pcdNameArrayInMsa != null) && (pcdNameArrayInMsa.length > 0)) {
                usageContext = dbManager.getUsageInstanceArrayById(parentId);
                //
                // For building library package, although all module are library, but PCD entries of
                // these library should be used to autogen.
                //
                if (usageContext == null) {
                    usageInstanceArray  = dbManager.getUsageInstanceArrayById(usageId);
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
                            throw new BuildActionException (String.format("Module %s using library instance %s; the PCD %s " +
                                                                          "is required by this library instance, but can not be found " +
                                                                          "in the %s's <ModuleSA> in the FPD file!",
                                                                          MemoryDatabaseManager.CurrentModuleName,
                                                                          moduleName,
                                                                          pcdNameArrayInMsa[index],
                                                                          MemoryDatabaseManager.CurrentModuleName
                                                                          ));
                        }
                    }
                }
            }
        } else {
            if (!isBuildUsedLibrary) {
                usageInstanceArray  = dbManager.getUsageInstanceArrayById(usageId);
                MemoryDatabaseManager.UsageInstanceContext = usageInstanceArray;
                MemoryDatabaseManager.CurrentModuleName    = moduleName;
            } else if ((pcdNameArrayInMsa != null) && (pcdNameArrayInMsa.length > 0)) {
                usageContext = MemoryDatabaseManager.UsageInstanceContext;
                //
                // For building library package, although all module are library, but PCD entries of
                // these library should be used to autogen.
                //
                if (usageContext == null) {
                    usageInstanceArray  = dbManager.getUsageInstanceArrayById(usageId);
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
                            throw new BuildActionException (String.format("Module %s using library instance %s; the PCD %s " +
                                                                          "is required by this library instance, but can not be found " +
                                                                          "in the %s's <ModuleSA> in the FPD file!",
                                                                          MemoryDatabaseManager.CurrentModuleName,
                                                                          moduleName,
                                                                          pcdNameArrayInMsa[index],
                                                                          MemoryDatabaseManager.CurrentModuleName
                                                                          ));
                        }
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

        if (pcdDriverType == CommonDefinition.PCD_DRIVER_TYPE.PEI_PCD_DRIVER) {
            hAutoGenString += MemoryDatabaseManager.PcdPeimHString;
            cAutoGenString += MemoryDatabaseManager.PcdPeimCString;
        } else if (pcdDriverType == CommonDefinition.PCD_DRIVER_TYPE.DXE_PCD_DRIVER) {
            hAutoGenString += MemoryDatabaseManager.PcdDxeHString;
            cAutoGenString += MemoryDatabaseManager.PcdDxeCString;
        }
    }
}
