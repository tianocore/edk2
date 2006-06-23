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
import java.util.UUID;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.tianocore.build.global.GlobalData;
import org.tianocore.build.pcd.entity.MemoryDatabaseManager;
import org.tianocore.build.pcd.entity.Token;
import org.tianocore.build.pcd.entity.UsageInstance;
import org.tianocore.build.pcd.exception.BuildActionException;
import org.tianocore.build.pcd.exception.EntityException;

/** This class is to manage how to generate the PCD information into Autogen.c and
    Autogen.h.
**/
public class PCDAutoGenAction extends BuildAction {
    ///
    /// The reference of DBManager in GlobalData class.
    ///
    private MemoryDatabaseManager dbManager;
    ///
    /// The name of module which is analysised currently.
    ///
    private String                moduleName;
    ///
    /// The Guid of module which is analyzed currently.
    /// 
    private UUID                  moduleGuid;
    ///
    /// The name of package whose module is analysized currently.
    /// 
    private String                packageName;
    ///
    /// The Guid of package whose module is analyszed curretnly.
    /// 
    private UUID                  packageGuid;
    ///
    /// The arch of current module
    /// 
    private String                arch;
    ///
    /// The version of current module
    /// 
    private String                version;
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
    private String[]              pcdNameArray;
    /**
      Set parameter ModuleName
  
      @param moduleName   the module name parameter.
    **/
    public void setModuleName(String moduleName) {
        this.moduleName = moduleName;
    }

    /**
       set the moduleGuid parameter.
       
       @param moduleGuid
    **/
    public void setModuleGuid(UUID moduleGuid) {
        this.moduleGuid = moduleGuid;
    }

    /**
       set packageName parameter.
       
       @param packageName
    **/
    public void setPackageName(String packageName) {
        this.packageName = packageName;
    }

    /**
        set packageGuid parameter.
       
       @param packageGuid
    **/
    public void setPackageGuid(UUID packageGuid) {
        this.packageGuid = packageGuid;
    }

    /**
       set Arch parameter.
       
       @param arch
    **/
    public void setArch(String arch) {
        this.arch = arch;
    }

    /**
       set version parameter
       
       @param version
     */
    public void setVersion(String version) {
        this.version = version;
    }

    /**
       set isBuildUsedLibrary parameter.
       
       @param isBuildUsedLibrary
     */
    public void setIsBuildUsedLibrary(boolean isBuildUsedLibrary) {
        this.isBuildUsedLibrary = isBuildUsedLibrary;
    }
    /**
       set pcdNameArray parameter.
       
       @param pcdNameArray
     */
    public void setPcdNameArray(String[] pcdNameArray) {
        this.pcdNameArray = pcdNameArray;
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
     
      @param moduleName            Parameter of this action class.
      @param isEmulatedPCDDriver   Parameter of this action class.
    **/
    public PCDAutoGenAction(String   moduleName, 
                            UUID     moduleGuid, 
                            String   packageName,
                            UUID     packageGuid,
                            String   arch,
                            String   version,
                            boolean  isBuildUsedLibrary,
                            String[] pcdNameArray) {
        dbManager       = null;
        hAutoGenString  = "";
        cAutoGenString  = "";

        setModuleName(moduleName);
        setModuleGuid(moduleGuid);
        setPackageName(packageName);
        setPackageGuid(packageGuid);
        setPcdNameArray(pcdNameArray);
        setArch(arch);
        setVersion(version);
        setIsBuildUsedLibrary(isBuildUsedLibrary);
    }

    /**
      check the parameter for action class.
      
      @throws BuildActionException Bad parameter.
    **/
    void checkParameter() throws BuildActionException {
        
    }

    /**
      Core execution function for this action class.
     
      All PCD information of this module comes from memory dabase. The collection
      work should be done before this action execution.
      Currently, we should generated all PCD information(maybe all dynamic) as array 
      in Pei emulated driver for simulating PCD runtime database. 
      
      @throws BuildActionException Failed to execute this aciton class.
    **/
    void performAction() throws BuildActionException {
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
        UsageInstance         usageInstance   = null;

        if (!isBuildUsedLibrary) {
            usageInstanceArray  = dbManager.getUsageInstanceArrayByModuleName(moduleName,
                                                                              moduleGuid,
                                                                              packageName,
                                                                              packageGuid,
                                                                              arch,
                                                                              version);
            dbManager.UsageInstanceContext = usageInstanceArray;
            dbManager.CurrentModuleName    = moduleName; 
        } else {
            usageContext = dbManager.UsageInstanceContext;
            //
            // For building MDE package, although all module are library, but PCD entries of 
            // these library should be used to autogen.
            // 
            if (usageContext == null) {
                usageInstanceArray  = dbManager.getUsageInstanceArrayByModuleName(moduleName,
                                                                                  moduleGuid,
                                                                                  packageName,
                                                                                  packageGuid,
                                                                                  arch,
                                                                                  version);
            } else {
                usageInstanceArray = new ArrayList<UsageInstance>();
                //
                // Remove PCD entries which are not belong to this library.
                // 
                for (index = 0; index < usageContext.size(); index++) {
                    if ((pcdNameArray == null) || (pcdNameArray.length == 0)){
                        break;
                    }

                    for (index2 = 0; index2 < pcdNameArray.length; index2 ++) {
                        if (pcdNameArray[index2].equalsIgnoreCase(usageContext.get(index).parentToken.cName)) {
                            usageInstanceArray.add(usageContext.get(index));
                            break;
                        }
                    }
                }
            }
        }

        if(usageInstanceArray.size() != 0) {
            //
            // Add "#include 'PcdLib.h'" for Header file
            //
            hAutoGenString = "#include <MdePkg/Include/Library/PcdLib.h>\r\n";
        }

        //
        // Generate all PCD entry for a module.
        // 
        for(index = 0; index < usageInstanceArray.size(); index ++) {
            ActionMessage.debug(this,
                                "Module " + moduleName + "'s PCD [" + Integer.toHexString(index) + 
                                "]: " + usageInstanceArray.get(index).parentToken.cName);
            try {
                usageInstance = usageInstanceArray.get(index);
                //
                // Before generate any PCD information into autogen.h/autogen.c for a module,
                // generate TokenSpaceGuid array variable firstly. For every dynamicEx type
                // PCD in this module the token, they are all reference to TokenSpaceGuid 
                // array.
                // 
                if (usageInstanceArray.get(index).modulePcdType == Token.PCD_TYPE.DYNAMIC_EX) {
                    guidStringArray = usageInstance.parentToken.tokenSpaceName.toString().split("-");
                    guidStringCName = "_gPcd_TokenSpaceGuid_" + 
                                      usageInstance.parentToken.tokenSpaceName.toString().replaceAll("-", "_");
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
                        hAutoGenString += String.format("extern EFI_GUID %s;\r\n",
                                                        guidStringCName);
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

            } catch(EntityException exp) {
                throw new BuildActionException("[PCD Autogen Error]: " + exp.getMessage());
            }
        }

        //
        // Work around code, In furture following code should be modified that get 
        // these information from Uplevel Autogen tools.
        // 
        if (moduleName.equalsIgnoreCase("PcdPeim")) {
            hAutoGenString += dbManager.PcdPeimHString;
            cAutoGenString += dbManager.PcdPeimCString;
        } else if (moduleName.equalsIgnoreCase("PcdDxe")) {
            hAutoGenString += dbManager.PcdDxeHString;
            cAutoGenString += dbManager.PcdDxeCString;
        }

        ActionMessage.debug(this,
                            "Module " + moduleName + "'s PCD header file:\r\n" + hAutoGenString + "\r\n"
                           );
        ActionMessage.debug(this,
                             "Module " + moduleName + "'s PCD C file:\r\n" + cAutoGenString + "\r\n"
                            );
    }

    /**
      Test case function

      @param argv  paramter from command line
    **/
    public static void main(String argv[]) {

        String WorkSpace = "X:/edk2";
        String logFilePath = WorkSpace  + "/EdkNt32Pkg/Nt32.fpd";
        String[] nameArray = null;

        //
        // At first, CollectPCDAction should be invoked to collect
        // all PCD information from SPD, MSA, FPD.
        //
        CollectPCDAction collectionAction = new CollectPCDAction();
        GlobalData.initInfo("Tools" + File.separator + "Conf" + File.separator + "FrameworkDatabase.db",
                            WorkSpace);

        try {
            collectionAction.perform(WorkSpace, 
                                     logFilePath,
                                     ActionMessage.MAX_MESSAGE_LEVEL);
        } catch(Exception e) {
            e.printStackTrace();
        }

        //
        // Then execute the PCDAuotoGenAction to get generated Autogen.h and Autogen.c
        //
        PCDAutoGenAction autogenAction = new PCDAutoGenAction("PcdPeim",
                                                              null,
                                                              null,
                                                              null,
                                                              "IA32",
                                                              null,
                                                              true,
                                                              nameArray);
        autogenAction.execute();

        System.out.println(autogenAction.OutputH());
        System.out.println("WQWQWQWQWQ");
        System.out.println(autogenAction.OutputC());
    }
}
