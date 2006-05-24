/** @file
  MemoryDatabaseManager class.

  Database hold all PCD information comes from SPD, MSA, FPD file in memory.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
 
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.build.pcd.entity;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.List;
import java.util.Map;

import org.tianocore.build.autogen.CommonDefinition;
import org.tianocore.build.pcd.action.ActionMessage;

/** Database hold all PCD information comes from SPD, MSA, FPD file in memory.
**/
public class MemoryDatabaseManager {
    ///
    ///  Memory database. The string "cName + SpaceNameGuid" is primary key.
    ///  memory database is in global scope, and it will be used for others PCD tools.
    ///
    private static Map<String, Token>  memoryDatabase = null;
    ///
    /// The log file name for dumping memory database.
    ///
    private static String              logFileName    = null;

    public static String PcdPeimHString       = "";
		public static String PcdPeimCString				= "";
    public static String PcdDxeHString  			= "";
    public static String PcdDxeCString  			= "";

    /**
      Constructure function
    **/
    public MemoryDatabaseManager() {
        //
        // Allocate memory for new database in global scope.
        //
        if (memoryDatabase == null) {
            memoryDatabase = new HashMap<String, Token>();
        }
    }

    /**
      Get the log file name.
    **/
    public String getLogFileName() {
        return logFileName;
    }

    /**
      Set parameter log file name.
  
      @param fileName log file name parameter.
    **/
    public void setLogFileName(String fileName) {
        logFileName = fileName;
    }

    /**
      Judege whether token exists in memory database
      
      @param primaryKey    the primaryKey for searching token
      
      @retval  TRUE  - token already exist in database.
      @retval  FALSE - token does not exist in database.
    **/
    public boolean isTokenInDatabase(String primaryKey) {
        return (memoryDatabase.get(primaryKey) != null);
    }

    /**
      Add a pcd token into memory database.
      
      @param primaryKey   the primary key for searching token
      @param token        token instance
    **/
    public void addTokenToDatabase(String primaryKey, Token token) {
        memoryDatabase.put(primaryKey, token);
    }

    /**
      Get a token instance from memory database with primary key.
  
      @param primaryKey   the primary key for searching token
    
      @return token instance.
    **/
    public Token getTokenByKey(String primaryKey) {
        return memoryDatabase.get(primaryKey);
    }

    /**
      Get the number of PCD token record in memory database.
      
      @return the number of PCD token record in memory database.
    **/
    public int getDBSize() {
        return memoryDatabase.size();
    }

    /**
      Get the token record array contained all PCD token in memory database.
      
      @return the token record array contained all PCD token in memory database.
    **/
    public Token[] getRecordArray() {
        Token[]     tokenArray  = null;
        Object[]    dataArray   = null;
        Map.Entry   entry       = null;
        int         index       = 0;

        if (memoryDatabase == null) {
            return null;
        }

        dataArray  = memoryDatabase.entrySet().toArray();
        tokenArray = new Token[memoryDatabase.size()];
        for (index = 0; index < memoryDatabase.size(); index ++) {
            entry =(Map.Entry) dataArray [index];
            tokenArray[index] =(Token) entry.getValue();
        }

        return tokenArray;
    }


    private ArrayList getDynamicRecordArray() {
        Token[]     tokenArray  =   getRecordArray();
        int         index       =   0;
        int         count       =   0;
        ArrayList   al          =   new ArrayList();

        for (index = 0; index < tokenArray.length; index++) {
            if (tokenArray[index].pcdType == Token.PCD_TYPE.DYNAMIC ||
                tokenArray[index].pcdType == Token.PCD_TYPE.DYNAMIC_EX) {
                al.add(tokenArray[index]);
            }
        }

        return al;
    }


    /**
      Get the token record array contained all PCD token referenced by PEI phase.
          The output array is sorted based on descending order of the size of alignment for each feilds.

      @return the token record array contained all PCD token referenced in PEI phase.
    **/
    public void getTwoPhaseDynamicRecordArray(ArrayList<Token> pei, ArrayList<Token> dxe) {
        int                     usageInstanceIndex  =   0;
        int                     index               =   0;
        ArrayList               tokenArrayList      =   getDynamicRecordArray();
        List<UsageInstance>     usageInstanceArray  =   null;
        UsageInstance           usageInstance       =   null;

				//pei = new ArrayList<Token>();
				//dxe = new ArrayList<Token>();

        for (index = 0; index < tokenArrayList.size(); index++) {
            boolean found   =   false;
            Token       token = (Token) tokenArrayList.get(index);
            if (token.producers != null) {
                usageInstanceArray = token.producers;
                for (usageInstanceIndex = 0; usageInstanceIndex < usageInstanceArray.size(); usageInstanceIndex++) {
                    usageInstance = (UsageInstance) usageInstanceArray.get(usageInstanceIndex);
                    if (CommonDefinition.isPeiPhaseComponent(usageInstance.componentType)) {
                        pei.add(token);
                        found = true;
                        break;
                    }
                }

            }
            if (!found) {
                if (token.consumers != null) {
                    usageInstanceArray = token.consumers;
                    for (usageInstanceIndex = 0; usageInstanceIndex < usageInstanceArray.size(); usageInstanceIndex ++) {
                        usageInstance =(UsageInstance) usageInstanceArray.get(usageInstanceIndex);
                        if (CommonDefinition.isPeiPhaseComponent(usageInstance.componentType)) {
                            pei.add(token);
														found = true;
                            break;
                        }
                    }
                }
            }

						//
						// If no PEI components reference the PCD entry, we insert it to DXE list
						//
						if (!found) {
								dxe.add(token);
						}
        }

				return;
    }

		/**
      Get all PCD record for a module according to module's name.
     
      @param moduleName  the name of module.
      
      @return  all usage instance for this module in memory database.
    **/
    public List<UsageInstance> getUsageInstanceArrayByModuleName(String moduleName) {
        Token[]               tokenArray          = null;
        int                   recordIndex         = 0; 
        int                   usageInstanceIndex  = 0;
        List<UsageInstance>   usageInstanceArray  = null;
        UsageInstance         usageInstance       = null;
        List<UsageInstance>   returnArray         = new ArrayList<UsageInstance>();

        tokenArray = getRecordArray();

        //
        // Loop to find all PCD record related to current module
        //
        for (recordIndex = 0; recordIndex < getDBSize(); recordIndex ++) {
            if (tokenArray[recordIndex].producers != null) {
                usageInstanceArray = tokenArray[recordIndex].producers;
                for (usageInstanceIndex = 0; usageInstanceIndex < usageInstanceArray.size(); usageInstanceIndex ++) {
                    usageInstance =(UsageInstance) usageInstanceArray.get(usageInstanceIndex);
                    if (usageInstance.moduleName.equalsIgnoreCase(moduleName)) {
                        returnArray.add(usageInstance);
                    }
                }
            }

            if (tokenArray[recordIndex].consumers != null) {
                usageInstanceArray = tokenArray[recordIndex].consumers;
                for (usageInstanceIndex = 0; usageInstanceIndex < usageInstanceArray.size(); usageInstanceIndex ++) {
                    usageInstance =(UsageInstance) usageInstanceArray.get(usageInstanceIndex);
                    if (usageInstance.moduleName.equalsIgnoreCase(moduleName)) {
                        returnArray.add(usageInstance);
                    }
                }
            }
        }

        if (returnArray.size() == 0) {
            ActionMessage.warning(this, "Can *not* find any usage instance for " + moduleName + " !");
        }

        return returnArray;
    }

    /**
      Get all modules name who contains PCD information
     
      @return Array for module name
    **/
    public List<String> getAllModuleArray()
    {
        int                       indexToken    = 0;
        int                       usageIndex    = 0;
        int                       moduleIndex   = 0;
        Token[]                   tokenArray    = null;
        List<String>              moduleNames   = new ArrayList<String>();
        UsageInstance             usageInstance = null;
        boolean                   bFound        = false;

        tokenArray = this.getRecordArray();
        //
        // Find all producer usage instance for retrieving module's name
        //
        for (indexToken = 0; indexToken < getDBSize(); indexToken ++) {
            for (usageIndex = 0; usageIndex < tokenArray[indexToken].producers.size(); usageIndex ++) {
                usageInstance = tokenArray[indexToken].producers.get(usageIndex);
                bFound        = false;
                for (moduleIndex = 0; moduleIndex < moduleNames.size(); moduleIndex ++) {
                    if (moduleNames.get(moduleIndex).equalsIgnoreCase(usageInstance.moduleName)) {
                        bFound = true;
                        break;
                    }
                }
                if (!bFound) {
                    moduleNames.add(usageInstance.moduleName);
                }
            }
        }

        //
        // Find all consumer usage instance for retrieving module's name
        //
        for (indexToken = 0; indexToken < getDBSize(); indexToken ++) {
            for (usageIndex = 0; usageIndex < tokenArray[indexToken].consumers.size(); usageIndex ++) {
                usageInstance = tokenArray[indexToken].consumers.get(usageIndex);
                bFound        = false;
                for (moduleIndex = 0; moduleIndex < moduleNames.size(); moduleIndex ++) {
                    if (moduleNames.get(moduleIndex).equalsIgnoreCase(usageInstance.moduleName)) {
                        bFound = true;
                        break;
                    }
                }
                if (!bFound) {
                    moduleNames.add(usageInstance.moduleName);
                }
            }
        }
        return moduleNames;
    }

    /**
      Dump all PCD record into file for reviewing.
    **/
    public void DumpAllRecords() {
        BufferedWriter    bWriter           = null;
        Object[]          tokenArray        = null;
        Map.Entry         entry             = null;
        Token             token             = null;
        int               index             = 0;
        int               usageIndex        = 0;
        UsageInstance     usageInstance     = null;
        String            inheritString     = null;
        String            componentTypeName = null;

        try {
            bWriter = new BufferedWriter(new FileWriter(new File(logFileName)));
            tokenArray = memoryDatabase.entrySet().toArray();
            for (index = 0; index < memoryDatabase.size(); index ++) {
                entry =(Map.Entry) tokenArray [index];
                token =(Token) entry.getValue();
                bWriter.write("****** token [" + Integer.toString(index) + "] ******\r\n");
                bWriter.write(" cName:" + token.cName + "\r\n");
                for (usageIndex = 0; usageIndex < token.producers.size(); usageIndex ++) {
                    usageInstance     =(UsageInstance)token.producers.get(usageIndex);
                    componentTypeName = CommonDefinition.getComponentTypeString(usageInstance.componentType);

                    if (usageInstance.isInherit) {
                        inheritString = "Inherit";
                    } else {
                        inheritString = "";
                    }
                    bWriter.write(String.format("   (Producer)#%d: %s:%s  Package:%s  %s\r\n",
                                                usageIndex,
                                                componentTypeName,
                                                usageInstance.moduleName,
                                                usageInstance.packageName,
                                                inheritString
                                               )
                                 );
                }
                for (usageIndex = 0; usageIndex < token.consumers.size(); usageIndex ++) {
                    usageInstance     =(UsageInstance)token.consumers.get(usageIndex);
                    componentTypeName = CommonDefinition.getComponentTypeString(usageInstance.componentType);
                    if (usageInstance.isInherit) {
                        inheritString = "Inherit";
                    } else {
                        inheritString = "";
                    }
                    bWriter.write(String.format("   (Consumer)#%d: %s:%s  Package:%s  %s\r\n",
                                                usageIndex,
                                                componentTypeName,
                                                usageInstance.moduleName,
                                                usageInstance.packageName,
                                                inheritString
                                               )
                                 );
                }
            }
            bWriter.close();
        } catch (IOException exp) {
            ActionMessage.warning(this, "Failed to open database log file: " + logFileName);
        }
    }
}
