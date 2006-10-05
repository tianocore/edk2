/** @file
   PlatformPcdPreprocessAction class.

   The abstract parent class PlatformPcdPreprocessAction, This class is to collect platform's
   pcd build information from fpd file.
   This class will be extended by building tools and wizard tools.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.pcd.action;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.tianocore.DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions;
import org.tianocore.PcdBuildDefinitionDocument.PcdBuildDefinition;
import org.tianocore.pcd.entity.*;
import org.tianocore.pcd.entity.Token;
import org.tianocore.pcd.entity.MemoryDatabaseManager;
import org.tianocore.pcd.exception.PlatformPcdPreprocessException;

/**
   The abstract parent class PlatformPcdPreprocessAction, This class is to collect platform's
   pcd build information from fpd file.
   This class will be extended by building tools and wizard tools.

**/
public abstract class PlatformPcdPreprocessAction {
    ///
    /// PCD memory database
    ///
    private MemoryDatabaseManager pcdDbManager;

    ///
    /// Errors string when perform preprocess 
    /// 
    private String                errorString;

    ///
    /// the count of errors when perform preprocess
    /// 
    private int                   errorCount;

    /**
       Default contructor function  
    **/
    public PlatformPcdPreprocessAction() {
        pcdDbManager = null;
        errorString  = null;
	errorCount   = 0;
    }

    /**
       Set parameter pcdDbManager

       @param pcdDbManager
    **/
    public void setPcdDbManager(MemoryDatabaseManager pcdDbManager) {
        this.pcdDbManager = pcdDbManager;
    }

    /**
       Get parameter pcdDbManager

       @return MemoryDatabaseManager
    **/
    public MemoryDatabaseManager getPcdDbManager() {
        return pcdDbManager;
    }

    /**
       Abstract function: retrieve module information from FPD file.

       In building environement, this function will be implementated by FpdParserTask.

       @return List<ModulePcdInfoFromFpd>        the component array.
       @throws PlatformPcdPreprocessException    get all modules in <ModuleSA> in FPD file.

    **/
    public abstract List<ModulePcdInfoFromFpd> getComponentsFromFpd()
                                               throws PlatformPcdPreprocessException;

    /**
       Abstract function to get GUID string from SPD file.

       In building evnironment, this function will be implementated by GlobaData.

       @param guidCName the CName of GUID

       @return String        Guid information from SPD file.
       @throws PlatformPcdPreprocessException
                            Fail to get Guid information from SPD file.
    **/
    public abstract String getGuidInfoFromSpd(String guidCName) throws PlatformPcdPreprocessException;

    /**
       Abstract function: Verification the PCD data.

       In different environment, such as building environment and wizard environment,
       it has different implementation according to optimization.

       @param cName         The token name
       @param moduleName    The module who use this PCD token
       @param datum         The PCD's datum
       @param datumType     The PCD's datum type
       @param maxDatumSize  The max size for PCD's Datum.

       @return String       exception strings.

    **/
    public abstract String verifyDatum(String cName, String moduleName, String datum,
                                       Token.DATUM_TYPE  datumType, int maxDatumSize);

    /**
       Abstract function: Get dynamic information for a token

       @param token
       @param moduleName

       @return DynamicPcdBuildDefinitions.PcdBuildData
    **/
    public abstract DynamicPcdBuildDefinitions.PcdBuildData
                                            getDynamicInfoFromFpd(Token     token,
                                                                  String    moduleName)
                                            throws PlatformPcdPreprocessException;

    /**
       Abstract function: Get all dynamic PCD information from FPD file.

       @return List<DynamicPcdBuildDefinitions.PcdBuildData>    All DYNAMIC PCD list in <DynamicPcdBuildDefinitions> in FPD file.
       @throws PlatformPcdPreprocessBuildException              Failure to get dynamic information list.

    **/
    public abstract List<DynamicPcdBuildDefinitions.PcdBuildData>
                                            getAllDynamicPcdInfoFromFpd()
                                            throws PlatformPcdPreprocessException;

    /**
       Return the error string after preprocess 

       @return String error string
    **/
    public String getErrorString() {
        return errorString;
    }

    public void putError(String error) {
        if (errorString == null) {
            errorString = "### ERROR[" + errorCount + "] ###\r\n" + error + "\r\n";
        } else {
            errorString += "### ERROR[" + errorCount + "] ###\r\n" + error + "\r\n";
        }

        errorCount++;
    }

    /**
      Collect all PCD information from FPD file into PCD memory database.

    **/
    public void initPcdMemoryDbWithPlatformInfo()
        throws PlatformPcdPreprocessException {
        int                                 index;
        int                                 pcdIndex;
        List<PcdBuildDefinition.PcdData>    pcdBuildDataArray = new ArrayList<PcdBuildDefinition.PcdData>();
        PcdBuildDefinition.PcdData          pcdBuildData;
        Token                               token             = null;
        List<ModulePcdInfoFromFpd>          modules;
        String                              primaryKey;
        String                              exceptionString;
        UsageInstance                       usageInstance;
        Token.PCD_TYPE                      pcdType           = Token.PCD_TYPE.UNKNOWN;
        Token.DATUM_TYPE                    datumType         = Token.DATUM_TYPE.UNKNOWN;
        long                                tokenNumber;
        String                              moduleName;
        String                              datum;
        int                                 maxDatumSize;
        String                              tokenSpaceStrRet;
        ModulePcdInfoFromFpd                curModule;

        //
        // ----------------------------------------------
        // 1), Get all <ModuleSA> from FPD file.
        // ----------------------------------------------
        //
        modules = getComponentsFromFpd();

        if (modules == null) {
            throw new PlatformPcdPreprocessException(
                "No modules found in the FPD file.\nPlease check whether there are elements in <FrameworkModules> in the FPD file!");
        }

        //
        // -------------------------------------------------------------------
        // 2), Loop all modules to process <PcdBuildDeclarations> for each module.
        // -------------------------------------------------------------------
        //
        for (index = 0; index < modules.size(); index++) {
            curModule = modules.get(index);

            //
    	    // It is legal for a module does not contains ANY pcd build definitions.
    	    //
    	    if (curModule.pcdBuildDefinition == null) {
                continue;
    	    }

            pcdBuildDataArray = curModule.pcdBuildDefinition.getPcdDataList();
            moduleName        = curModule.usageId.moduleName;

            //
            // ----------------------------------------------------------------------
            // 2.1), Loop all Pcd entry for a module and add it into memory database.
            // ----------------------------------------------------------------------
            //
            for (pcdIndex = 0; pcdIndex < pcdBuildDataArray.size(); pcdIndex++) {
                pcdBuildData = pcdBuildDataArray.get(pcdIndex);

                tokenSpaceStrRet = getGuidInfoFromSpd(pcdBuildData.getTokenSpaceGuidCName());

                if (tokenSpaceStrRet == null) {
                    putError("Failed to get Token Space Guid for token " + pcdBuildData.getCName() +
                             " from any SPD file. You must have an <GuidDeclaration> for this Token Space Guid!");
                    //
                    // Do not break preprocess, continues to analysis.
                    // All errors will be summary to be shown.
                    // 
                    continue;
                }

                primaryKey   = Token.getPrimaryKeyString(pcdBuildData.getCName(), tokenSpaceStrRet);
                pcdType      = Token.getPcdTypeFromString(pcdBuildData.getItemType().toString());
                datumType    = Token.getdatumTypeFromString(pcdBuildData.getDatumType().toString());
                tokenNumber  = Long.decode(pcdBuildData.getToken().toString());
                if (pcdBuildData.getValue() != null) {
                    datum = pcdBuildData.getValue().toString();
                } else {
                    datum = null;
                }
                maxDatumSize = pcdBuildData.getMaxDatumSize();

                if ((pcdType    == Token.PCD_TYPE.FEATURE_FLAG) &&
                    (datumType  != Token.DATUM_TYPE.BOOLEAN)){
                    exceptionString = String.format("In FPD file, for PCD %s in module %s, the PCD type is FEATURE_FLAG but "+
                                                    "datum type for this PCD entry is not BOOLEAN!",
                                                    pcdBuildData.getCName(),
                                                    moduleName);
                    putError(exceptionString);
                    //
                    // Do not break preprocess, continues to analysis.
                    // All errors will be summary to be shown.
                    // 
                    continue;
                }

                //
                // -------------------------------------------------------------------------------------------
                // 2.1.1), Do some necessary checking work for FixedAtBuild, FeatureFlag and PatchableInModule
                // -------------------------------------------------------------------------------------------
                //
                if (!Token.isDynamic(pcdType)) {
                     //
                     // Value is required.
                     //
                     if (datum == null) {
                         exceptionString = String.format("In the FPD file, there is no value for PCD entry %s in module %s!",
                                                         pcdBuildData.getCName(),
                                                         moduleName);
                         putError(exceptionString);
                         //
                         // Do not break preprocess, continues to analysis.
                         // All errors will be summary to be shown.
                         // 
                         continue;
                     }

                     //
                     // Check whether the datum size is matched datum type.
                     //
                     if ((exceptionString = verifyDatum(pcdBuildData.getCName(),
                                                        moduleName,
                                                        datum,
                                                        datumType,
                                                        maxDatumSize)) != null) {
                         putError(exceptionString);
                         //
                         // Do not break preprocess, continues to analysis.
                         // All errors will be summary to be shown.
                         // 
                         continue;
                     }
                }

                //
                // ---------------------------------------------------------------------------------
                // 2.1.2), Create token or update token information for current anaylized PCD data.
                // ---------------------------------------------------------------------------------
                //
                if (pcdDbManager.isTokenInDatabase(primaryKey)) {
                    //
                    // If the token is already exist in database, do some necessary checking
                    // and add a usage instance into this token in database
                    //
                    token = pcdDbManager.getTokenByKey(primaryKey);

                    //
                    // checking for DatumType, DatumType should be unique for one PCD used in different
                    // modules.
                    //
                    if (token.datumType != datumType) {
                        exceptionString = String.format("In the FPD file, the datum type of the PCD entry %s is %s, which is different from %s which was previously defined!",
                                                        pcdBuildData.getCName(),
                                                        pcdBuildData.getDatumType().toString(),
                                                        Token.getStringOfdatumType(token.datumType));
                        putError(exceptionString);
                        //
                        // Do not break preprocess, continues to analysis.
                        // All errors will be summary to be shown.
                        // 
                        continue;
                    }

                    //
                    // Check token number is valid
                    //
                    if (tokenNumber != token.tokenNumber) {
                        exceptionString = String.format("In the FPD file, the token number of PCD entry %s in module %s is different from the same PCD entry in other modules!",
                                                        pcdBuildData.getCName(),
                                                        moduleName);
                        putError(exceptionString);
                        //
                        // Do not break preprocess, continues to analysis.
                        // All errors will be summary to be shown.
                        // 
                        continue;
                    }

                    //
                    // For same PCD used in different modules, the PCD type should all be dynamic or non-dynamic.
                    //
                    if (token.isDynamicPCD != Token.isDynamic(pcdType)) {
                        exceptionString = String.format("In the FPD file, for PCD entry %s in module %s, you defined dynamic or non-dynamic PCD type which"+
                                                        " is different from other module's definition.",
                                                        token.cName,
                                                        moduleName);
                        putError(exceptionString);
                        //
                        // Do not break preprocess, continues to analysis.
                        // All errors will be summary to be shown.
                        // 
                        continue;
                    }

                    if (token.isDynamicPCD) {
                        if ((maxDatumSize != 0) &&
                            (maxDatumSize != token.datumSize)){
                            exceptionString = String.format("In the FPD file, for dynamic PCD %s in module %s, the max datum size is %d which "+
                                                            "is different than <MaxDatumSize> %d defined in <DynamicPcdBuildDefinitions>!",
                                                            token.cName,
                                                            moduleName,
                                                            maxDatumSize,
                                                            token.datumSize);
                            putError(exceptionString);
                            //
                            // Do not break preprocess, continues to analysis.
                            // All errors will be summary to be shown.
                            // 
                            continue;
                        }
                    }

                } else {
                    //
                    // If the token is not in database, create a new token instance and add
                    // a usage instance into this token in database.
                    //
                    tokenSpaceStrRet = getGuidInfoFromSpd(pcdBuildData.getTokenSpaceGuidCName());

                    if (tokenSpaceStrRet == null) {
                        putError("Failed to get the Token Space Guid for token" + token.cName +
                                 " from any SPD file. You must have a <GuidDeclaration> for this Token Space Guid!");
                        //
                        // Do not break preprocess, continues to analysis.
                        // All errors will be summary to be shown.
                        // 
                        continue;
                    }

                    token = new Token(pcdBuildData.getCName(), tokenSpaceStrRet);

                    token.datumType     = datumType;
                    token.tokenNumber   = tokenNumber;
                    token.isDynamicPCD  = Token.isDynamic(pcdType);
                    token.datumSize     = maxDatumSize;

                    if (token.isDynamicPCD) {
                        //
                        // For Dynamic and Dynamic Ex type, need find the dynamic information
                        // in <DynamicPcdBuildDefinition> section in FPD file.
                        //
                        if (null == updateDynamicInformation(moduleName,
                                                             token,
                                                             datum,
                                                             maxDatumSize)) {
                            continue;
                        }
                    }

                    pcdDbManager.addTokenToDatabase(primaryKey, token);
                }

                //
                // -----------------------------------------------------------------------------------
                // 2.1.3), Add the PcdType in current module into this Pcd token's supported PCD type.
                // -----------------------------------------------------------------------------------
                //
                token.updateSupportPcdType(pcdType);

                //
                // ------------------------------------------------
                // 2.1.4), Create an usage instance for this token.
                // ------------------------------------------------
                //
                usageInstance = new UsageInstance(token,
                                                  curModule.usageId,
                                                  pcdType,
                                                  datum,
                                                  maxDatumSize);
                if (!token.addUsageInstance(usageInstance)) {
                    putError(String.format("PCD %s for module %s(%s) already exists in the database.\nPlease check all PCD build entries "+
                                           "in the %s module's <ModuleSA> section to make sure there are no duplicated definitions in the FPD file!",
                                           token.cName,
                                           curModule.usageId.moduleGuid,
                                           moduleName,
                                           moduleName));
                    continue;
                }
            }
        }

        //
        // ------------------------------------------------
        // 3), Add unreference dynamic_Ex pcd token into Pcd database.
        // ------------------------------------------------
        //
        List<Token> tokenArray = getUnreferencedDynamicPcd();
        if (tokenArray != null) {
            for (index = 0; index < tokenArray.size(); index++) {
                pcdDbManager.addTokenToDatabase(tokenArray.get(index).getPrimaryKeyString(),
                                             tokenArray.get(index));
            }
        }
    }

    /**
       Update dynamic information for PCD entry.

       Dynamic information is retrieved from <PcdDynamicBuildDeclarations> in
       FPD file.

       @param moduleName        The name of the module who use this PCD
       @param token             The token instance
       @param datum             The <datum> in module's PCD information
       @param maxDatumSize      The <maxDatumSize> in module's PCD information

       @return Token
     */
    private Token updateDynamicInformation(String   moduleName,
                                           Token    token,
                                           String   datum,
                                           int      maxDatumSize)
        throws PlatformPcdPreprocessException {
        int                 index           = 0;
        int                 offset;
        String              exceptionString = null;
        SkuInstance         skuInstance     = null;
        String              temp;
        boolean             hasSkuId0       = false;
        long                tokenNumber     = 0;
        String              hiiDefaultValue = null;
        String              variableGuidString = null;

        List<DynamicPcdBuildDefinitions.PcdBuildData.SkuInfo>   skuInfoList = null;
        DynamicPcdBuildDefinitions.PcdBuildData                 dynamicInfo = null;

        dynamicInfo = getDynamicInfoFromFpd(token, moduleName);
        if (dynamicInfo == null) {
            exceptionString = String.format("In the FPD file, the Dynamic PCD %s is used by module %s.\n" +
                                            "However, there is no dynamic information in the <DynamicPcdBuildDefinitions> " +
                                            "section of the FPD file.  This section is required!",
                                            token.cName,
                                            moduleName);
            putError(exceptionString);
            return null;
        }

        token.datumSize = dynamicInfo.getMaxDatumSize();

        exceptionString = verifyDatum(token.cName,
                                      moduleName,
                                      null,
                                      token.datumType,
                                      token.datumSize);
        if (exceptionString != null) {
            throw new PlatformPcdPreprocessException(exceptionString);
        }

        if ((maxDatumSize != 0) &&
            (maxDatumSize != token.datumSize)) {
            exceptionString = String.format("In the FPD file, for dynamic PCD %s, the datum size in module %s is %d, but "+
                                            "the datum size in <DynamicPcdBuildDefinitions> is %d, they do not match!",
                                            token.cName,
                                            moduleName,
                                            maxDatumSize,
                                            dynamicInfo.getMaxDatumSize());
            putError(exceptionString);
            return null;
        }
        tokenNumber = Long.decode(dynamicInfo.getToken().toString());
        if (tokenNumber != token.tokenNumber) {
            exceptionString = String.format("In the FPD file, for dynamic PCD %s, the token number in module %s is 0x%x, but "+
                                            "in the <DynamicPcdBuildDefinictions> section, the token number is 0x%x, they do not match!",
                                            token.cName,
                                            moduleName,
                                            token.tokenNumber,
                                            tokenNumber);
            putError(exceptionString);
            return null;
        }

        token.dynamicExTokenNumber = tokenNumber;

        skuInfoList = dynamicInfo.getSkuInfoList();

        //
        // Loop all sku data
        //
        for (index = 0; index < skuInfoList.size(); index++) {
            skuInstance = new SkuInstance();
            //
            // Although SkuId in schema is BigInteger, but in fact, sku id is 32 bit value.
            //
            temp = skuInfoList.get(index).getSkuId().toString();
            skuInstance.id = Integer.decode(temp);
            if (skuInstance.id == 0) {
                hasSkuId0 = true;
            }
            //
            // Judge whether is DefaultGroup at first, because most case is DefautlGroup.
            //
            if (skuInfoList.get(index).getValue() != null) {
                skuInstance.value.setValue(skuInfoList.get(index).getValue().toString());
                if ((exceptionString = verifyDatum(token.cName,
                                                   null,
                                                   skuInfoList.get(index).getValue().toString(),
                                                   token.datumType,
                                                   token.datumSize)) != null) {
                    putError(exceptionString);
                    return null;
                }

                token.skuData.add(skuInstance);

                continue;
            }

            //
            // Judge whether is HII group case.
            //
            if (skuInfoList.get(index).getVariableName() != null) {
                exceptionString = null;
                if (skuInfoList.get(index).getVariableGuid() == null) {
                    exceptionString = String.format("In the FPD file, for dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                    "file, use of HII was defined, but there is no <VariableGuid> defined for SKU %d data!",
                                                    token.cName,
                                                    index);
                    putError(exceptionString);
                    return null;
                }

                if (skuInfoList.get(index).getVariableOffset() == null) {
                    exceptionString = String.format("In the FPD file, for dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                    "file, use of HII was defined, but there is no <VariableOffset> defined for SKU %d data!",
                                                    token.cName,
                                                    index);
                    putError(exceptionString);
                    return null;
                }

                if (skuInfoList.get(index).getHiiDefaultValue() == null) {
                    exceptionString = String.format("In the FPD file, for dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                    "file, use of HII was defined, but there is no <HiiDefaultValue> defined for SKU %d data!",
                                                    token.cName,
                                                    index);
                    putError(exceptionString);
                    return null;
                }

                if (skuInfoList.get(index).getHiiDefaultValue() != null) {
                    hiiDefaultValue = skuInfoList.get(index).getHiiDefaultValue().toString();
                } else {
                    hiiDefaultValue = null;
                }

                if ((exceptionString = verifyDatum(token.cName,
                                                   null,
                                                   hiiDefaultValue,
                                                   token.datumType,
                                                   token.datumSize)) != null) {
                    throw new PlatformPcdPreprocessException(exceptionString);
                }

                offset = Integer.decode(skuInfoList.get(index).getVariableOffset());
                if (offset > 0xFFFF) {
                    putError(String.format("In the FPD file, for dynamic PCD %s, the variable offset defined in SKU %d data "+
                                           "exceeds 64K, which is not allowed!",
                                           token.cName,
                                           index));
                    return null;
                }

                //
                // Get variable guid string according to the name of guid which will be mapped into a GUID in SPD file.
                //
                variableGuidString = getGuidInfoFromSpd(skuInfoList.get(index).getVariableGuid().toString());
                if (variableGuidString == null) {
                    putError(String.format("In the FPD file, for dynamic PCD %s, the variable guid: %s cannot be found in any SPD file!",
                                           token.cName,
                                           skuInfoList.get(index).getVariableGuid().toString()));
                    return null;
                }
                String variableStr = skuInfoList.get(index).getVariableName();
                Pattern pattern = Pattern.compile("0x([a-fA-F0-9]){4}");
                Matcher matcher = pattern.matcher(variableStr);
                List<String> varNameList = new ArrayList<String>();
                while (matcher.find()){
                    String str = variableStr.substring(matcher.start(),matcher.end());
                    varNameList.add(str);
                }

                skuInstance.value.setHiiData(varNameList,
                                             translateSchemaStringToUUID(variableGuidString),
                                             skuInfoList.get(index).getVariableOffset(),
                                             skuInfoList.get(index).getHiiDefaultValue().toString());
                token.skuData.add(skuInstance);
                continue;
            }

            if (skuInfoList.get(index).getVpdOffset() != null) {
                skuInstance.value.setVpdData(skuInfoList.get(index).getVpdOffset());
                token.skuData.add(skuInstance);
                continue;
            }

            exceptionString = String.format("In the FPD file, for dynamic PCD %s, the dynamic info must "+
                                            "be one of: 'DefaultGroup', 'HIIGroup', 'VpdGroup'.",
                                            token.cName);
            putError(exceptionString);
            return null;
        }

        if (!hasSkuId0) {
            exceptionString = String.format("In the FPD file, for dynamic PCD %s in <DynamicPcdBuildDefinitions>, there is "+
                                            "no SKU ID = 0 data, which is required for every dynamic PCD",
                                            token.cName);
            putError(exceptionString);
            return null;
        }

        return token;
    }

    /**
       Get all dynamic PCD defined in <DynamicPcdBuildDefinitions> which unreferenced by
       any <ModuleSA> in FPD file.

       @return List<Token>  Return PCD token
    **/
    private List<Token> getUnreferencedDynamicPcd () throws PlatformPcdPreprocessException {
        List<Token>                                   tokenArray                 = new ArrayList<Token>();
        Token                                         token                      = null;
        List<DynamicPcdBuildDefinitions.PcdBuildData> dynamicPcdBuildDataArray   = null;
        DynamicPcdBuildDefinitions.PcdBuildData       pcdBuildData               = null;
        List<DynamicPcdBuildDefinitions.PcdBuildData.SkuInfo>   skuInfoList      = null;
        Token.PCD_TYPE                                pcdType;
        SkuInstance                                   skuInstance                = null;
        String  primaryKey = null;
        boolean hasSkuId0  = false;
        int     index, offset, index2;
        String  temp;
        String  exceptionString;
        String  hiiDefaultValue;
        String  tokenSpaceStrRet;
        String  variableGuidString;

        dynamicPcdBuildDataArray = getAllDynamicPcdInfoFromFpd();
        if (dynamicPcdBuildDataArray == null) {
            return null;
        }

        for (index2 = 0; index2 < dynamicPcdBuildDataArray.size(); index2++) {
            pcdBuildData = dynamicPcdBuildDataArray.get(index2);
            tokenSpaceStrRet = getGuidInfoFromSpd(pcdBuildData.getTokenSpaceGuidCName());

            if (tokenSpaceStrRet == null) {
                putError("Failed to get the Token Space Guid for token" + pcdBuildData.getCName());
                continue;
            }

            primaryKey = Token.getPrimaryKeyString(pcdBuildData.getCName(),
                                                   tokenSpaceStrRet);

            if (pcdDbManager.isTokenInDatabase(primaryKey)) {
                continue;
            }

            pcdType = Token.getPcdTypeFromString(pcdBuildData.getItemType().toString());
            if (pcdType != Token.PCD_TYPE.DYNAMIC_EX) {
                putError(String.format("In the FPD file, it not allowed to define DYNAMIC PCD %s that is not used by any module",
                                       pcdBuildData.getCName()));
                continue;
            }

            //
            // Create new token for unreference dynamic PCD token
            //
            token           = new Token(pcdBuildData.getCName(), tokenSpaceStrRet);
            token.datumSize = pcdBuildData.getMaxDatumSize();


            token.datumType     = Token.getdatumTypeFromString(pcdBuildData.getDatumType().toString());
            token.tokenNumber   = Long.decode(pcdBuildData.getToken().toString());
            token.dynamicExTokenNumber = token.tokenNumber;
            token.isDynamicPCD  = true;
            token.updateSupportPcdType(pcdType);

            exceptionString = verifyDatum(token.cName,
                                          null,
                                          null,
                                          token.datumType,
                                          token.datumSize);
            if (exceptionString != null) {
                putError(exceptionString);
                continue;
            }

            skuInfoList = pcdBuildData.getSkuInfoList();

            //
            // Loop all sku data
            //
            for (index = 0; index < skuInfoList.size(); index++) {
                skuInstance = new SkuInstance();
                //
                // Although SkuId in schema is BigInteger, but in fact, sku id is 32 bit value.
                //
                temp = skuInfoList.get(index).getSkuId().toString();
                skuInstance.id = Integer.decode(temp);
                if (skuInstance.id == 0) {
                    hasSkuId0 = true;
                }
                //
                // Judge whether is DefaultGroup at first, because most case is DefautlGroup.
                //
                if (skuInfoList.get(index).getValue() != null) {
                    skuInstance.value.setValue(skuInfoList.get(index).getValue().toString());
                    if ((exceptionString = verifyDatum(token.cName,
                                                       null,
                                                       skuInfoList.get(index).getValue().toString(),
                                                       token.datumType,
                                                       token.datumSize)) != null) {
                        putError(exceptionString);
                        continue;
                    }

                    token.skuData.add(skuInstance);

                    continue;
                }

                //
                // Judge whether is HII group case.
                //
                if (skuInfoList.get(index).getVariableName() != null) {
                    exceptionString = null;
                    if (skuInfoList.get(index).getVariableGuid() == null) {
                        exceptionString = String.format("In the FPD file, for dynamic PCD %s in the <DynamicPcdBuildDefinitions> section of the FPD "+
                                                        "file, use of HII is defined, but there is no <VariableGuid> defined for SKU %d data!",
                                                        token.cName,
                                                        index);
                        putError(exceptionString);
                        continue;
                    }

                    if (skuInfoList.get(index).getVariableOffset() == null) {
                        exceptionString = String.format("In the FPD file, for dynamic PCD %s in the <DynamicPcdBuildDefinitions> section of the FPD "+
                                                        "file, use of HII is defined, but there is no <VariableOffset> defined for SKU %d data!",
                                                        token.cName,
                                                        index);
                        putError(exceptionString);
                        continue;
                    }

                    if (skuInfoList.get(index).getHiiDefaultValue() == null) {
                        exceptionString = String.format("In the FPD file, for dynamic PCD %s in the <DynamicPcdBuildDefinitions> section of the FPD "+
                                                        "file, use of HII is defined, but there is no <HiiDefaultValue> defined for SKU %d data!",
                                                        token.cName,
                                                        index);
                        putError(exceptionString);
                        continue;
                    }

                    if (skuInfoList.get(index).getHiiDefaultValue() != null) {
                        hiiDefaultValue = skuInfoList.get(index).getHiiDefaultValue().toString();
                    } else {
                        hiiDefaultValue = null;
                    }

                    if ((exceptionString = verifyDatum(token.cName,
                                                       null,
                                                       hiiDefaultValue,
                                                       token.datumType,
                                                       token.datumSize)) != null) {
                        putError(exceptionString);
                        continue;
                    }

                    offset = Integer.decode(skuInfoList.get(index).getVariableOffset());
                    if (offset > 0xFFFF) {
                        exceptionString = String.format("In the FPD file, for dynamic PCD %s, the variable offset defined in SKU %d data "+
                                          "exceeds 64K, which is not allowed!",
                                          token.cName,
                                          index);
                        putError(exceptionString);
                        continue;
                    }

                    //
                    // Get variable guid string according to the name of guid which will be mapped into a GUID in SPD file.
                    //
                    variableGuidString = getGuidInfoFromSpd(skuInfoList.get(index).getVariableGuid().toString());
                    if (variableGuidString == null) {
                        exceptionString = String.format("In the FPD file, for dynamic PCD %s, the variable guid %s cannot be found in any SPD file!",
                                                        token.cName,
                                                        skuInfoList.get(index).getVariableGuid().toString());
                        putError(exceptionString);
                        continue;
                    }
                    String variableStr = skuInfoList.get(index).getVariableName();
                    Pattern pattern = Pattern.compile("0x([a-fA-F0-9]){4}");
                    Matcher matcher = pattern.matcher(variableStr);
                    List<String> varNameList = new ArrayList<String>();
                    while (matcher.find()){
                        String str = variableStr.substring(matcher.start(),matcher.end());
                        varNameList.add(str);
                    }

                    skuInstance.value.setHiiData(varNameList,
                                                 translateSchemaStringToUUID(variableGuidString),
                                                 skuInfoList.get(index).getVariableOffset(),
                                                 skuInfoList.get(index).getHiiDefaultValue().toString());
                    token.skuData.add(skuInstance);
                    continue;
                }

                if (skuInfoList.get(index).getVpdOffset() != null) {
                    skuInstance.value.setVpdData(skuInfoList.get(index).getVpdOffset());
                    token.skuData.add(skuInstance);
                    continue;
                }

                exceptionString = String.format("In the FPD file, for dynamic PCD %s, the dynamic info must "+
                                                "be one of 'DefaultGroup', 'HIIGroup', 'VpdGroup'.",
                                                token.cName);
                putError(exceptionString);
            }

            if (!hasSkuId0) {
                exceptionString = String.format("In the FPD file, for dynamic PCD %s in <DynamicPcdBuildDefinitions>, there is "+
                                                "no SKU ID = 0 data, which is required for every dynamic PCD",
                                                token.cName);
                putError(exceptionString);
                continue;
            }

            tokenArray.add(token);
        }

        return tokenArray;
    }

    /**
       Translate the schema string to UUID instance.

       In schema, the string of UUID is defined as following two types string:
        1) GuidArrayType: pattern = 0x[a-fA-F0-9]{1,8},( )*0x[a-fA-F0-9]{1,4},(
        )*0x[a-fA-F0-9]{1,4}(,( )*\{)?(,?( )*0x[a-fA-F0-9]{1,2}){8}( )*(\})?

        2) GuidNamingConvention: pattern =
        [a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}

       This function will convert string and create uuid instance.

       @param uuidString    UUID string in XML file

       @return UUID         UUID instance
    **/
    private UUID translateSchemaStringToUUID(String uuidString)
        throws PlatformPcdPreprocessException {
        String      temp;
        String[]    splitStringArray;
        int         index;
        int         chIndex;
        int         chLen;

        if (uuidString == null) {
            return null;
        }

        if (uuidString.length() == 0) {
            return null;
        }

        if (uuidString.equals("0") ||
            uuidString.equalsIgnoreCase("0x0")) {
            return new UUID(0, 0);
        }

        uuidString = uuidString.replaceAll("\\{", "");
        uuidString = uuidString.replaceAll("\\}", "");

        //
        // If the UUID schema string is GuidArrayType type then need translate
        // to GuidNamingConvention type at first.
        //
        if ((uuidString.charAt(0) == '0') && ((uuidString.charAt(1) == 'x') || (uuidString.charAt(1) == 'X'))) {
            splitStringArray = uuidString.split("," );
            if (splitStringArray.length != 11) {
                throw new PlatformPcdPreprocessException ("Wrong format for GUID string: " + uuidString);
            }

            //
            // Remove blank space from these string and remove header string "0x"
            //
            for (index = 0; index < 11; index++) {
                splitStringArray[index] = splitStringArray[index].trim();
                splitStringArray[index] = splitStringArray[index].substring(2, splitStringArray[index].length());
            }

            //
            // Add heading '0' to normalize the string length
            //
            for (index = 3; index < 11; index++) {
                chLen = splitStringArray[index].length();
                for (chIndex = 0; chIndex < 2 - chLen; chIndex++) {
                    splitStringArray[index] = "0" + splitStringArray[index];
                }
            }

            //
            // construct the final GuidNamingConvention string
            //
            temp = String.format("%s-%s-%s-%s%s-%s%s%s%s%s%s",
                                 splitStringArray[0],
                                 splitStringArray[1],
                                 splitStringArray[2],
                                 splitStringArray[3],
                                 splitStringArray[4],
                                 splitStringArray[5],
                                 splitStringArray[6],
                                 splitStringArray[7],
                                 splitStringArray[8],
                                 splitStringArray[9],
                                 splitStringArray[10]);
            uuidString = temp;
        }

        return UUID.fromString(uuidString);
    }
}

