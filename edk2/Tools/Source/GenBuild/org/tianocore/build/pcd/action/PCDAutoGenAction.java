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
    /// Wheter current module is PCD emulated driver. It is only for 
    /// emulated PCD driver and will be kept until PCD IMAGE tool ready.
    ///
    private boolean               isEmulatedPCDDriver;
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

    /**
      Set parameter isEmulatedPCDDriver
  
      @param isEmulatedPCDDriver  whether this module is PeiEmulatedPCD driver
    **/
    public void setIsEmulatedPCDDriver(boolean isEmulatedPCDDriver) {
        this.isEmulatedPCDDriver = isEmulatedPCDDriver;
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
    public PCDAutoGenAction(String moduleName, boolean isEmulatedPCDDriver) {
        dbManager = null;
        setIsEmulatedPCDDriver(isEmulatedPCDDriver);
        setModuleName(moduleName);
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

        hAutoGenString = "";
        cAutoGenString = "";

        if(isEmulatedPCDDriver) {
            generateAutogenForPCDEmulatedDriver();
        } else {
            generateAutogenForModule();
        }
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

        usageInstanceArray  = dbManager.getUsageInstanceArrayByModuleName(moduleName);

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
                usageInstanceArray.get(index).generateAutoGen();
                hAutoGenString += usageInstanceArray.get(index).getHAutogenStr() + "\r\n";
                cAutoGenString += usageInstanceArray.get(index).getCAutogenStr() + "\r\n";
            } catch(EntityException exp) {
                throw new BuildActionException(exp.getMessage());
            }
        }

        ActionMessage.debug(this,
                            "Module " + moduleName + "'s PCD header file:\r\n" + hAutoGenString + "\r\n"
                           );
        ActionMessage.debug(this,
                             "Module " + moduleName + "'s PCD C file:\r\n" + cAutoGenString + "\r\n"
                            );
    }

    /**
      Generate all PCD autogen string and the emulated PCD IMAGE array for emulated driver.
      
      Currently, we should generated all PCD information(maybe all dynamic) as array 
      in Pei emulated driver for simulating PCD runtime database. 
      
    **/
    private void generateAutogenForPCDEmulatedDriver() {
        int           index;
        Token[]       tokenArray;
        UsageInstance usageInstance;

        //
        // Add "#include 'PcdLib.h'" for Header file
        //
        hAutoGenString = "#include <MdePkg/Include/Library/PcdLib.h>\r\n";

        tokenArray = dbManager.getRecordArray();
        for(index = 0; index < tokenArray.length; index ++) {
            //
            // Get one consumer instance and generate autogen for this token.
            //
            if(tokenArray[index].consumers != null ) {
                if(tokenArray[index].consumers.size() == 0) {
                    continue;
                }

                usageInstance = tokenArray[index].consumers.get(0);
                try {
                    usageInstance.generateAutoGen();
                } catch(EntityException exp) {
                    throw new BuildActionException(exp.getMessage());
                }

                hAutoGenString += usageInstance.getHAutogenStr();
                cAutoGenString += usageInstance.getCAutogenStr();

                hAutoGenString += "\r\n";
                cAutoGenString += "\r\n";
            }
        }

        generatePCDEmulatedArray(tokenArray);

        ActionMessage.debug(this,
                             "PCD emulated driver's header: \r\n" + hAutoGenString + "\r\n"
                            );
        ActionMessage.debug(this,
                             "PCD emulated driver's C code: \r\n" + cAutoGenString + "\r\n"
                            );

    }

    /**
      Generate PCDEmulated array in PCDEmulated driver for emulated runtime database.
      
      @param tokenArray  All PCD token in memory database.
      
      @throws BuildActionException  Unknown PCD_TYPE
    **/
    private void generatePCDEmulatedArray(Token[] tokenArray)
        throws BuildActionException {
        int       index;
        Token     token;
        String[]  guidStrArray;
        String    value;

        //
        // The value of String type of PCD entry maybe use byte array but not string direcly
        // such as {0x1, 0x2, 0x3}, and define PCD1_STRING_Value as L"string define here"
        // For this case, we should generate a string array to C output and use the address
        // of generated string array.
        //
        for(index = 0; index < tokenArray.length; index ++) {
            token = tokenArray[index];

            if((token.producers.size() == 0) &&(token.consumers.size() == 0)) {
                //
                // If no one use this PCD token, it will not generated in emulated array.
                //
                continue;
            }
            value = token.datum.toString();
            if(token.datumType == Token.DATUM_TYPE.POINTER) {
                if(!((value.charAt(0) == 'L' && value.charAt(1) == '"') ||(value.charAt(0) == '"'))) {
                    cAutoGenString += String.format("UINT8 _mPcdArray%08x[] = %s;\r\n", 
                                                     index, 
                                                     value
                                                     );
                }
            }
        }

        //
        // Output emulated PCD entry array
        //
        cAutoGenString += "\r\nEMULATED_PCD_ENTRY gEmulatedPcdEntry[] = {\r\n";

        for(index = 0; index < tokenArray.length; index ++) {
            token = tokenArray[index];

            if((token.producers.size() == 0) &&(token.consumers.size() == 0)) {
                //
                // If no one use this PCD token, it will not generated in emulated array.
                //
                continue;
            }

            if(index != 0) {
                cAutoGenString += ",\r\n";
            }

            //
            // Print Start "{" for a Token item in array
            //
            cAutoGenString += "  {\r\n";

            //
            // Print Token Name
            //
            cAutoGenString += String.format("    _PCD_TOKEN_%s,\r\n", token.cName);

            //
            // Print Hii information
            //
            if(token.hiiEnabled) {
                cAutoGenString += String.format("    TRUE,\r\n");
            } else {
                cAutoGenString += String.format("    FALSE,\r\n");
            }

            //
            // Print sku information
            //
            if(token.skuEnabled) {
                cAutoGenString += String.format("    TRUE,\r\n");
            } else {
                cAutoGenString += String.format("    FALSE,\r\n");
            }

            //
            // Print maxSkuCount
            //
            cAutoGenString += String.format("    %d,\r\n", token.maxSkuCount);

            cAutoGenString += String.format("    %d,\r\n", token.skuId);

            if(token.variableGuid == null) {
                cAutoGenString += "    { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },\r\n";
            } else {
                guidStrArray =(token.variableGuid.toString()).split("-");

                cAutoGenString += String.format("    { 0x%s, 0x%s, 0x%s, { 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s, 0x%s } },\r\n",
                                                 guidStrArray[0],
                                                 guidStrArray[1],
                                                 guidStrArray[2],
                                                (guidStrArray[3].substring(0, 2)),
                                                (guidStrArray[3].substring(2, 4)),
                                                (guidStrArray[4].substring(0, 2)),
                                                (guidStrArray[4].substring(2, 4)),
                                                (guidStrArray[4].substring(4, 6)),
                                                (guidStrArray[4].substring(6, 8)),
                                                (guidStrArray[4].substring(8, 10)),
                                                (guidStrArray[4].substring(10, 12))
                                                );

            }

            value = token.datum.toString();
            if(token.datumType == Token.DATUM_TYPE.POINTER) {
                if((value.charAt(0) == 'L' && value.charAt(1) == '"') || value.charAt(0) == '"') {
                    cAutoGenString += String.format("    sizeof(_PCD_VALUE_%s),\r\n", token.cName);
                    cAutoGenString += String.format("    0, %s, %s,\r\n", token.variableName, value);
                } else {
                    cAutoGenString += String.format("    sizeof(_mPcdArray%08x),\r\n", index);
                    cAutoGenString += String.format("    0, &_mPcdArray%08x, %s,\r\n", index, token.variableName);
                }
            } else {
                switch(token.datumType) {
                case UINT8:
                    cAutoGenString += "    1,\r\n";
                    break;
                case UINT16:
                    cAutoGenString += "    2,\r\n";
                    break;
                case UINT32:
                    cAutoGenString += "    4,\r\n";
                    break;
                case UINT64:
                    cAutoGenString += "    8,\r\n";
                    break;
                case BOOLEAN:
                    cAutoGenString += "    1,\r\n";
                    break;
                default:
                    throw new BuildActionException("Unknown datum size");
                }
                cAutoGenString += String.format("    %s, %s, NULL,\r\n", value, token.variableName);
            }

            //
            // Print end "}" for a token item in array
            //
            cAutoGenString += "  }";
        }

        cAutoGenString += "\r\n};\r\n";
        cAutoGenString += "\r\n";
        cAutoGenString += "UINTN\r\n";
        cAutoGenString += "GetPcdDataBaseSize(\r\n";
        cAutoGenString += "  VOID\r\n";
        cAutoGenString += "  )\r\n";
        cAutoGenString += "{\r\n";
        cAutoGenString += "  return sizeof(gEmulatedPcdEntry);\r\n";
        cAutoGenString += "}\r\n";
    }

    /**
      Test case function

      @param argv  paramter from command line
    **/
    public static void main(String argv[]) {
        String logFilePath = "G:/mdk/EdkNt32Pkg/build/Nt32.fpd";

        //
        // At first, CollectPCDAction should be invoked to collect
        // all PCD information from SPD, MSA, FPD.
        //
        CollectPCDAction collectionAction = new CollectPCDAction();
        GlobalData.initInfo("Tools" + File.separator + "Conf" + File.separator + "FrameworkDatabase.db",
                            "G:/mdk");

        GlobalData.getPCDMemoryDBManager().setLogFileName(logFilePath + ".PCDMemroyDatabaseLog.txt");

        try {
            collectionAction.perform("G:/mdk", 
                                     logFilePath,
                                     ActionMessage.MAX_MESSAGE_LEVEL);
        } catch(Exception e) {
            e.printStackTrace();
        }

        //
        // Then execute the PCDAuotoGenAction to get generated Autogen.h and Autogen.c
        //
        PCDAutoGenAction autogenAction = new PCDAutoGenAction("HelloWorld",
                                                              true
                                                              );
        autogenAction.execute();
    }
}
