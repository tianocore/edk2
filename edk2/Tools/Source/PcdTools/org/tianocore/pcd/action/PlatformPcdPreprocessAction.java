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

       @return List<ModuleInfo>                  the component array.
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
    public abstract String                  getGuidInfoFromSpd(String guidCName)
                                            throws PlatformPcdPreprocessException;

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
    public abstract String                  verifyDatum(String            cName,
                                                        String            moduleName,
                                                        String            datum,
                                                        Token.DATUM_TYPE  datumType,
                                                        int               maxDatumSize);

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
        int                                 index             = 0;
        int                                 pcdIndex          = 0;
        List<PcdBuildDefinition.PcdData>    pcdBuildDataArray = new ArrayList<PcdBuildDefinition.PcdData>();
        PcdBuildDefinition.PcdData          pcdBuildData      = null;
        Token                               token             = null;
        List<ModulePcdInfoFromFpd>          modules           = null;
        String                              primaryKey        = null;
        String                              exceptionString   = null;
        UsageInstance                       usageInstance     = null;
        Token.PCD_TYPE                      pcdType           = Token.PCD_TYPE.UNKNOWN;
        Token.DATUM_TYPE                    datumType         = Token.DATUM_TYPE.UNKNOWN;
        long                                tokenNumber       = 0;
        String                              moduleName        = null;
        String                              datum             = null;
        int                                 maxDatumSize      = 0;
        String                              tokenSpaceStrRet  = null;

        //
        // ----------------------------------------------
        // 1), Get all <ModuleSA> from FPD file.
        // ----------------------------------------------
        //
        modules = getComponentsFromFpd();

        if (modules == null) {
            throw new PlatformPcdPreprocessException(
                "No modules in FPD file, Please check whether there are elements in <FrameworkModules> in FPD file!");
        }

        //
        // -------------------------------------------------------------------
        // 2), Loop all modules to process <PcdBuildDeclarations> for each module.
        // -------------------------------------------------------------------
        //
        for (index = 0; index < modules.size(); index++) {
    	    //
    	    // It is legal for a module does not contains ANY pcd build definitions.
    	    //
    	    if (modules.get(index).pcdBuildDefinition == null) {
                continue;
    	    }

            pcdBuildDataArray = modules.get(index).pcdBuildDefinition.getPcdDataList();

            moduleName = modules.get(index).usageId.moduleName;

            //
            // ----------------------------------------------------------------------
            // 2.1), Loop all Pcd entry for a module and add it into memory database.
            // ----------------------------------------------------------------------
            //
            for (pcdIndex = 0; pcdIndex < pcdBuildDataArray.size(); pcdIndex++) {
                pcdBuildData = pcdBuildDataArray.get(pcdIndex);

                tokenSpaceStrRet = getGuidInfoFromSpd(pcdBuildData.getTokenSpaceGuidCName());

                if (tokenSpaceStrRet == null) {
                    putError("Fail to get Token space guid for token" + pcdBuildData.getCName() +
                             " from all SPD files. You must have an <GuidDeclaration> for this token space Guid");
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
                    exceptionString = String.format("In FPD file, for PCD %s in module %s, the PCD type is FEATRUE_FLAG but "+
                                                    "datum type of this PCD entry is not BOOLEAN!",
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
                         exceptionString = String.format("In FPD file, there is no value for PCD entry %s in module %s!",
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
                        exceptionString = String.format("In FPD file, the datum type of PCD entry %s is %s, which is different with  %s defined in before!",
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
                        exceptionString = String.format("In FPD file, the token number of PCD entry %s in module %s is different with same PCD entry in other modules!",
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
                        exceptionString = String.format("In FPD file, for PCD entry %s in module %s, you define dynamic or non-dynamic PCD type which"+
                                                        " is different with others module's",
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
                        //
                        // Check datum is equal the datum in dynamic information.
                        // For dynamic PCD, you can do not write <Value> in sperated every <PcdBuildDefinition> in different <ModuleSA>,
                        // But if you write, the <Value> must be same as the value in <DynamicPcdBuildDefinitions>.
                        //
                        if (!token.isSkuEnable() &&
                            (token.getDefaultSku().type == DynamicTokenValue.VALUE_TYPE.DEFAULT_TYPE) &&
                            (datum != null)) {
                            if (!datum.equalsIgnoreCase(token.getDefaultSku().value)) {
                                exceptionString = String.format("In FPD file, for dynamic PCD %s in module %s, the datum in <ModuleSA> is "+
                                                                "not equal to the datum in <DynamicPcdBuildDefinitions>, it is "+
                                                                "illega! You could no set <Value> in <ModuleSA> for a dynamic PCD!",
                                                                token.cName,
                                                                moduleName);
                                putError(exceptionString);
                                //
                                // Do not break preprocess, continues to analysis.
                                // All errors will be summary to be shown.
                                // 
                                continue;
                            }
                        }

                        if ((maxDatumSize != 0) &&
                            (maxDatumSize != token.datumSize)){
                            exceptionString = String.format("In FPD file, for dynamic PCD %s in module %s, the max datum size is %d which "+
                                                            "is different with <MaxDatumSize> %d defined in <DynamicPcdBuildDefinitions>!",
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
                        putError("Fail to get Token space guid for token" + token.cName +
                                 " from all SPD files. You must have an <GuidDeclaration> for this token space Guid");
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
                                                  modules.get(index).usageId,
                                                  pcdType,
                                                  datum,
                                                  maxDatumSize);
                if (!token.addUsageInstance(usageInstance)) {
                    putError(String.format("PCD %s for module %s(%s) has already exist in database, Please check all PCD build entries "+
                                           "in modules %s in <ModuleSA> to make sure no duplicated definitions in FPD file!",
                                           token.cName,
                                           modules.get(index).usageId.moduleGuid,
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
            exceptionString = String.format("In FPD file, for Dynamic PCD %s used by module %s, "+
                                            "there is no dynamic information in <DynamicPcdBuildDefinitions> "+
                                            "in FPD file, but it is required!",
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
            exceptionString = String.format("In FPD file, for dynamic PCD %s, the datum size in module %s is %d, but "+
                                            "the datum size in <DynamicPcdBuildDefinitions> is %d, they are not match!",
                                            token.cName,
                                            moduleName,
                                            maxDatumSize,
                                            dynamicInfo.getMaxDatumSize());
            putError(exceptionString);
            return null;
        }
        tokenNumber = Long.decode(dynamicInfo.getToken().toString());
        if (tokenNumber != token.tokenNumber) {
            exceptionString = String.format("In FPD file, for dynamic PCD %s, the token number in module %s is 0x%x, but"+
                                            "in <DynamicPcdBuildDefinictions>, the token number is 0x%x, they are not match!",
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

                //
                // Judege wether is same of datum between module's information
                // and dynamic information.
                //
                if (datum != null) {
                    if ((skuInstance.id == 0)                                   &&
                        !datum.toString().equalsIgnoreCase(skuInfoList.get(index).getValue().toString())) {
                        exceptionString = "In FPD file, for dynamic PCD " + token.cName + ", the value in module " + moduleName + " is " + datum.toString() + " but the "+
                                          "value of sku 0 data in <DynamicPcdBuildDefinition> is " + skuInstance.value.value + ". They are must be same!"+
                                          " or you could not define value for a dynamic PCD in every <ModuleSA>!";
                        putError(exceptionString);
                        return null;
                    }
                }
                continue;
            }

            //
            // Judge whether is HII group case.
            //
            if (skuInfoList.get(index).getVariableName() != null) {
                exceptionString = null;
                if (skuInfoList.get(index).getVariableGuid() == null) {
                    exceptionString = String.format("In FPD file, for dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                    "file, who use HII, but there is no <VariableGuid> defined for Sku %d data!",
                                                    token.cName,
                                                    index);
                    putError(exceptionString);
                    return null;
                }

                if (skuInfoList.get(index).getVariableOffset() == null) {
                    exceptionString = String.format("In FPD file, for dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                    "file, who use HII, but there is no <VariableOffset> defined for Sku %d data!",
                                                    token.cName,
                                                    index);
                    putError(exceptionString);
                    return null;
                }

                if (skuInfoList.get(index).getHiiDefaultValue() == null) {
                    exceptionString = String.format("In FPD file, for dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                    "file, who use HII, but there is no <HiiDefaultValue> defined for Sku %d data!",
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
                    putError(String.format("In FPD file, for dynamic PCD %s ,  the variable offset defined in sku %d data "+
                                           "exceed 64K, it is not allowed!",
                                           token.cName,
                                           index));
                    return null;
                }

                //
                // Get variable guid string according to the name of guid which will be mapped into a GUID in SPD file.
                //
                variableGuidString = getGuidInfoFromSpd(skuInfoList.get(index).getVariableGuid().toString());
                if (variableGuidString == null) {
                    putError(String.format("In FPD file, for dynamic PCD %s,  the variable guid %s can be found in all SPD file!",
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

            exceptionString = String.format("In FPD file, for dynamic PCD %s, the dynamic info must "+
                                            "be one of 'DefaultGroup', 'HIIGroup', 'VpdGroup'.",
                                            token.cName);
            putError(exceptionString);
            return null;
        }

        if (!hasSkuId0) {
            exceptionString = String.format("In FPD file, for dynamic PCD %s in <DynamicPcdBuildDefinitions>, there are "+
                                            "no sku id = 0 data, which is required for every dynamic PCD",
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
                putError("Fail to get Token space guid for token" + pcdBuildData.getCName());
                continue;
            }

            primaryKey = Token.getPrimaryKeyString(pcdBuildData.getCName(),
                                                   tokenSpaceStrRet);

            if (pcdDbManager.isTokenInDatabase(primaryKey)) {
                continue;
            }

            pcdType = Token.getPcdTypeFromString(pcdBuildData.getItemType().toString());
            if (pcdType != Token.PCD_TYPE.DYNAMIC_EX) {
                putError(String.format("In FPD file, it not allowed for DYNAMIC PCD %s who is no used by any module",
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
                        exceptionString = String.format("In FPD file, for dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                        "file, who use HII, but there is no <VariableGuid> defined for Sku %d data!",
                                                        token.cName,
                                                        index);
                        putError(exceptionString);
                        continue;
                    }

                    if (skuInfoList.get(index).getVariableOffset() == null) {
                        exceptionString = String.format("In FPD file, for dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                        "file, who use HII, but there is no <VariableOffset> defined for Sku %d data!",
                                                        token.cName,
                                                        index);
                        putError(exceptionString);
                        continue;
                    }

                    if (skuInfoList.get(index).getHiiDefaultValue() == null) {
                        exceptionString = String.format("In FPD file, for dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                        "file, who use HII, but there is no <HiiDefaultValue> defined for Sku %d data!",
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
                        exceptionString = String.format("In FPD file, for dynamic PCD %s ,  the variable offset defined in sku %d data "+
                                          "exceed 64K, it is not allowed!",
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
                        exceptionString = String.format("In FPD file, for dynamic PCD %s,  the variable guid %s can be found in all SPD file!",
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

                exceptionString = String.format("In FPD file, for dynamic PCD %s, the dynamic info must "+
                                                "be one of 'DefaultGroup', 'HIIGroup', 'VpdGroup'.",
                                                token.cName);
                putError(exceptionString);
            }

            if (!hasSkuId0) {
                exceptionString = String.format("In FPD file, for dynamic PCD %s in <DynamicPcdBuildDefinitions>, there are "+
                                                "no sku id = 0 data, which is required for every dynamic PCD",
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
                throw new PlatformPcdPreprocessException ("Wrong format for UUID string: " + uuidString);
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

