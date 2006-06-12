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
import java.util.List;
import java.util.UUID;

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
    /// Wheter current module is PCD emulated driver. It is only for 
    /// emulated PCD driver and will be kept until PCD IMAGE tool ready.
    ///
    private boolean               isEmulatedPCDDriver;
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

    /**
      Set parameter ModuleName
  
      @param moduleName   the module name parameter.
    **/
    public void setModuleName(String moduleName) {
        this.moduleName = moduleName;
    }

    public void setModuleGuid(UUID moduleGuid) {
        this.moduleGuid = moduleGuid;
    }

    public void setPackageName(String packageName) {
        this.packageName = packageName;
    }

    public void setPackageGuid(UUID packageGuid) {
        this.packageGuid = packageGuid;
    }

    public void setArch(String arch) {
        this.arch = arch;
    }

    public void setVersion(String version) {
        this.version = version;
    }

    /**
      Set parameter isEmulatedPCDDriver
  
      @param isEmulatedPCDDriver  whether this module is PeiEmulatedPCD driver
    **/
    public void setIsEmulatedPCDDriver(boolean isEmulatedPCDDriver) {
        this.isEmulatedPCDDriver = isEmulatedPCDDriver;
    }

    public void setIsBuildUsedLibrary(boolean isBuildUsedLibrary) {
        this.isBuildUsedLibrary = isBuildUsedLibrary;
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
    public PCDAutoGenAction(String  moduleName, 
                            UUID    moduleGuid, 
                            String  packageName,
                            UUID    packageGuid,
                            String  arch,
                            String  version,
                            boolean isEmulatedPCDDriver, 
                            boolean isBuildUsedLibrary) {
        dbManager       = null;
        hAutoGenString  = "";
        cAutoGenString  = "";

        setIsEmulatedPCDDriver(isEmulatedPCDDriver);
        setModuleName(moduleName);
        setModuleGuid(moduleGuid);
        setPackageName(packageName);
        setPackageGuid(packageGuid);
        setArch(arch);
        setVersion(version);
        setIsBuildUsedLibrary(isBuildUsedLibrary);
    }

    /**
      check the parameter for action class.
      
      @throws BuildActionException Bad parameter.
    **/
    void checkParameter() throws BuildActionException {
        if(!isEmulatedPCDDriver &&(moduleName == null)) {
            throw new BuildActionException("Wrong module name parameter for PCDAutoGenAction tool!");
        }

        if(!isEmulatedPCDDriver && moduleName.length() == 0) {
            throw new BuildActionException("Wrong module name parameter for PCDAutoGenAction tool!");
        }
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
        int                   index;
        List<UsageInstance>   usageInstanceArray;

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
            usageInstanceArray = dbManager.UsageInstanceContext;
            //
            // For building MDE package, although all module are library, but PCD entries of 
            // these library should be used to autogen.
            // 
            if (usageInstanceArray == null) {
                usageInstanceArray  = dbManager.getUsageInstanceArrayByModuleName(moduleName,
                                                                                  moduleGuid,
                                                                                  packageName,
                                                                                  packageGuid,
                                                                                  arch,
                                                                                  version);
            }
        }

        if(usageInstanceArray.size() != 0) {
            //
            // Add "#include 'PcdLib.h'" for Header file
            //
            hAutoGenString = "#include <MdePkg/Include/Library/PcdLib.h>\r\n";
        }

        for(index = 0; index < usageInstanceArray.size(); index ++) {
            ActionMessage.debug(this,
                                "Module " + moduleName + "'s PCD [" + Integer.toHexString(index) + 
                                "]: " + usageInstanceArray.get(index).parentToken.cName);
            try {
                usageInstanceArray.get(index).generateAutoGen(isBuildUsedLibrary);
                hAutoGenString += usageInstanceArray.get(index).getHAutogenStr() + "\r\n";
                cAutoGenString += usageInstanceArray.get(index).getCAutogenStr() + "\r\n";
            } catch(EntityException exp) {
                throw new BuildActionException(exp.getMessage());
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

        String WorkSpace = "M:/ForPcd/edk2";
        String logFilePath = WorkSpace  + "/MdePkg/MdePkg.fpd";

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
        PCDAutoGenAction autogenAction = new PCDAutoGenAction("BaseLib",
                                                              null,
                                                              null,
                                                              null,
                                                              null,
                                                              null,
                                                              false,
                                                              false
                                                              );
        autogenAction.execute();

        System.out.println(autogenAction.OutputH());
        System.out.println("WQWQWQWQWQ");
        System.out.println(autogenAction.OutputC());


        System.out.println (autogenAction.hAutoGenString);
        System.out.println (autogenAction.cAutoGenString);

    }
}
