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
package org.tianocore.pcd.entity;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.tianocore.pcd.entity.UsageIdentification;
import org.tianocore.pcd.exception.EntityException;

/**
  Database hold all PCD information comes from SPD, MSA, FPD file in memory.
**/
public class MemoryDatabaseManager {
    ///
    ///  Memory database. The string "cName + SpaceNameGuid" is primary key.
    ///  memory database is in global scope, and it will be used for others PCD tools.
    ///
    private static Map<String, Token>  memoryDatabase       = null;

    ///
    /// Before build a module, the used libary will be build firstly, the PCD of these
    /// library is inheritted by the module, so stored module's PCD information as PCD
    /// context of building libary.
    ///
    public static List<UsageInstance> UsageInstanceContext = null;

    ///
    /// Current module name, if now is buiding library, this value indicate this library
    /// is for building what module.
    ///
    public static String CurrentModuleName                 = null;

    ///
    /// String for PCD PEIM and DXE autogen file
    ///
    public static String PcdPeimHString                    = "";
    public static String PcdPeimCString                    = "";
    public static String PcdDxeHString                     = "";
    public static String PcdDxeCString                     = "";

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
        int         size        = 0;

        if (memoryDatabase == null) {
            return null;
        }
        dataArray  = memoryDatabase.entrySet().toArray();
        size       = memoryDatabase.size();
        tokenArray = new Token[memoryDatabase.size()];
        for (index = 0; index < size; index++) {
            entry =(Map.Entry) dataArray [index];
            tokenArray[index] =(Token) entry.getValue();
        }
        return tokenArray;
    }

    /**
       Get record array only contains DYNAMIC or DYNAMIC_EX type PCD.

       @return ArrayList the array list contains all dynamic type PCD.
    **/
    private ArrayList getDynamicRecordArray() {
        Token[]     tokenArray  =   getRecordArray();
        int         index       =   0;
        ArrayList<Token>   al   =   new ArrayList<Token>();

        for (index = 0; index < tokenArray.length; index++) {
            if (tokenArray[index].isDynamicPCD) {
                al.add(tokenArray[index]);
            }
        }

        return al;
    }


    /**
      Get the token record array contained all PCD token referenced by PEI phase.
      The output array is sorted based on descending order of the size of alignment for each feilds.

      @return the token record array contained all PCD token referenced in PEI phase.
      @throws EntityException
    **/
    public void getTwoPhaseDynamicRecordArray(ArrayList<Token> pei, ArrayList<Token> dxe)
        throws EntityException {
        int                     usageInstanceIndex  =   0;
        int                     index               =   0;
        ArrayList               tokenArrayList      =   getDynamicRecordArray();
        Object[]                usageInstanceArray  =   null;
        UsageInstance           usageInstance       =   null;
        int                     size                =   0;
        int                     consumerSize        =   0;

        size = tokenArrayList.size();
        for (index = 0; index < size; index++) {
            boolean found   =   false;
            Token       token = (Token) tokenArrayList.get(index);
            if (token.consumers != null) {
                usageInstanceArray = token.consumers.entrySet().toArray();
                consumerSize       = token.consumers.size();
                for (usageInstanceIndex = 0; usageInstanceIndex < consumerSize; usageInstanceIndex++) {
                    usageInstance =(UsageInstance) (((Map.Entry)usageInstanceArray[usageInstanceIndex]).getValue());
                    if (usageInstance.isPeiPhaseComponent()) {
                        pei.add(token);
                        found = true;
                        break;
                    }
                }
            }

            //
            // If no PEI components reference the PCD entry,
            // we check if it is referenced in DXE driver.
            //
            if (!found) {
                if (token.consumers != null) {
                    usageInstanceArray = token.consumers.entrySet().toArray();
                    consumerSize       = token.consumers.size();
                    for (usageInstanceIndex = 0; usageInstanceIndex < consumerSize; usageInstanceIndex++) {
                        usageInstance =(UsageInstance) (((Map.Entry)usageInstanceArray[usageInstanceIndex]).getValue());
                        if (usageInstance.isDxePhaseComponent()) {
                            dxe.add(token);
                            found = true;
                            break;
                        }
                    }
                }

                if (!found) {
                    if (token.isDynamicPCD && token.consumers.size() == 0) {
                        dxe.add(token);
                    } else {
                        //
                        // We only support Dynamice(EX) type for PEI and DXE phase.
                        // If it is not referenced in either PEI or DXE, throw exception now.
                        //
                        throw new EntityException("[PCD Tool Internal Error] Dynamic(EX) PCD Entries are referenced in a module that is not used in either PEI or DXE phases.");
                    }
                }
            }
        }
    }

    public void clearDatabase() {
        memoryDatabase.clear();
    }

    /**
       Get all PCD token for a usage instance according to primary key.

       @param primaryKey    the primary key of usage instance.

       @return List<UsageInstance>  the list contains all usage instances.
    **/
    public List<UsageInstance> getUsageInstanceArrayById(UsageIdentification usageId) {
        Token[]               tokenArray          = null;
        int                   recordIndex         = 0;
        UsageInstance         usageInstance       = null;
        List<UsageInstance>   returnArray         = new ArrayList<UsageInstance>();
        String                primaryKey          = usageId.toString();

        tokenArray = getRecordArray();
        
        //
        // Loop to find all PCD record related to current module
        //
        for (recordIndex = 0; recordIndex < getDBSize(); recordIndex++) {
            if (tokenArray[recordIndex].consumers.size() != 0) {
                usageInstance = tokenArray[recordIndex].consumers.get(primaryKey);
                if (usageInstance != null) {
                    returnArray.add(usageInstance);
                }
            }
        }

        return returnArray;
    }

    public List<Token> getPcdTokenListForModule(UsageIdentification usageId) {
        List<UsageInstance> usageList = getUsageInstanceArrayById(usageId);
        List<Token>         tokenList = new ArrayList<Token>();

        if (usageList == null) {
            return null;
        }

        for (int usageIndex = 0; usageIndex < usageList.size(); usageIndex++) {
            tokenList.add(usageList.get(usageIndex).parentToken);
        }

        return tokenList;
    }

    /**
      Get all modules name who contains PCD information

      @return Array for usage's identification
    **/
    public List<UsageIdentification> getAllModuleArray()
    {
        int                       tokenIndex    = 0;
        int                       usageIndex    = 0;
        int                       moduleIndex   = 0;
        Token[]                   tokenArray    = null;
        Object[]                  usageInstanceArray = null;
        List<UsageIdentification> usageArray    = new ArrayList<UsageIdentification>();
        UsageInstance             usageInstance = null;
        boolean                   bFound        = false;
        String                    primaryKey    = null;

        tokenArray = getRecordArray();
        //
        // Find all consumer usage instance for retrieving module's name
        //
        for (tokenIndex = 0; tokenIndex < getDBSize(); tokenIndex++) {
            usageInstanceArray = tokenArray[tokenIndex].consumers.entrySet().toArray();
            for (usageIndex = 0; usageIndex < tokenArray[tokenIndex].consumers.size(); usageIndex++) {
                usageInstance = (UsageInstance)((Map.Entry)usageInstanceArray[usageIndex]).getValue();
                primaryKey    = usageInstance.getPrimaryKey();
                bFound        = false;
                for (moduleIndex = 0; moduleIndex < usageArray.size(); moduleIndex++) {
                    if (usageArray.get(moduleIndex).toString().equalsIgnoreCase(primaryKey)) {
                        bFound = true;
                        break;
                    }
                }
                if (!bFound) {
                    usageArray.add(usageInstance.usageId);
                }
            }
        }
        return usageArray;
    }
}
