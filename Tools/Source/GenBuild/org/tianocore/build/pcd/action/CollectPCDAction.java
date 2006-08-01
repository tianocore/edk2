/** @file
  CollectPCDAction class.

  This action class is to collect PCD information from MSA, SPD, FPD xml file.
  This class will be used for wizard and build tools, So it can *not* inherit
  from buildAction or wizardAction.
 
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
import java.io.IOException;
import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.UUID;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.xmlbeans.XmlException;
import org.apache.xmlbeans.XmlObject;
import org.tianocore.DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions;
import org.tianocore.FrameworkModulesDocument;
import org.tianocore.ModuleSADocument;
import org.tianocore.PcdBuildDefinitionDocument;
import org.tianocore.PcdBuildDefinitionDocument.PcdBuildDefinition;
import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.build.fpd.FpdParserTask;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.id.FpdModuleIdentification;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.pcd.action.ActionMessage;
import org.tianocore.pcd.entity.CommonDefinition;
import org.tianocore.pcd.entity.DynamicTokenValue;
import org.tianocore.pcd.entity.MemoryDatabaseManager;
import org.tianocore.pcd.entity.SkuInstance;
import org.tianocore.pcd.entity.Token;
import org.tianocore.pcd.entity.UsageIdentification;
import org.tianocore.pcd.entity.UsageInstance;
import org.tianocore.pcd.exception.EntityException;

/** Module Info class is the data structure to hold information got from GlobalData.
*/
class ModuleInfo {
    ///
    /// Module's ID for a <ModuleSA>
    /// 
    private FpdModuleIdentification                       moduleId;
    ///
    /// <PcdBuildDefinition> xmlobject in FPD file for a <ModuleSA>
    /// 
    private PcdBuildDefinitionDocument.PcdBuildDefinition pcdBuildDef;

    public ModuleInfo (FpdModuleIdentification moduleId, XmlObject pcdDef) {
        this.moduleId       = moduleId;
        this.pcdBuildDef    = ((PcdBuildDefinitionDocument)pcdDef).getPcdBuildDefinition();
    }

    public FpdModuleIdentification getModuleId (){
    	return moduleId;
    }

    public PcdBuildDefinitionDocument.PcdBuildDefinition getPcdBuildDef(){
    	return pcdBuildDef;
    }
}

/** This action class is to collect PCD information from MSA, SPD, FPD xml file.
    This class will be used for wizard and build tools, So it can *not* inherit
    from buildAction or UIAction.
**/
public class CollectPCDAction {
    ///
    /// memoryDatabase hold all PCD information collected from SPD, MSA, FPD.
    /// 
    private MemoryDatabaseManager dbManager;
    ///
    /// Workspacepath hold the workspace information.
    /// 
    private String                workspacePath;
    ///
    /// FPD file is the root file. 
    /// 
    private String                fpdFilePath;
    ///
    /// Message level for CollectPCDAction.
    /// 
    private int                   originalMessageLevel;
    ///
    /// Cache the fpd docment instance for private usage.
    /// 
    private PlatformSurfaceAreaDocument fpdDocInstance;
    ///
    /// xmlObject name
    /// 
    private static String xmlObjectName = "PcdBuildDefinition"; 
    	
    /**
      Set WorkspacePath parameter for this action class.

      @param workspacePath parameter for this action
    **/
    public void setWorkspacePath(String workspacePath) {
        this.workspacePath = workspacePath;
    }

    /**
      Set action message level for CollectPcdAction tool.

      The message should be restored when this action exit.

      @param actionMessageLevel parameter for this action
    **/
    public void setActionMessageLevel(int actionMessageLevel) {
        originalMessageLevel       = ActionMessage.messageLevel;
        ActionMessage.messageLevel = actionMessageLevel;
    }

    /**
      Set FPDFileName parameter for this action class.

      @param fpdFilePath    fpd file path
    **/
    public void setFPDFilePath(String fpdFilePath) {
        this.fpdFilePath = fpdFilePath;
    }

    /**
      Common function interface for outer.
      
      @param workspacePath The path of workspace of current build or analysis.
      @param fpdFilePath   The fpd file path of current build or analysis.
      @param messageLevel  The message level for this Action.
      
      @throws  Exception The exception of this function. Because it can *not* be predict
                         where the action class will be used. So only Exception can be throw.
      
    **/
    public void perform(String workspacePath, String fpdFilePath, 
                        int messageLevel) throws Exception {
        setWorkspacePath(workspacePath);
        setFPDFilePath(fpdFilePath);
        setActionMessageLevel(messageLevel);
        checkParameter();
        execute();
        ActionMessage.messageLevel = originalMessageLevel;
    }

    /**
      Core execution function for this action class.
     
      This function work flows will be:
      1) Collect and prepocess PCD information from FPD file, all PCD
      information will be stored into memory database.
      2) Generate 3 strings for
        a) All modules using Dynamic(Ex) PCD entry.(Token Number)
        b) PEI PCDDatabase (C Structure) for PCD Service PEIM.
        c) DXE PCD Database (C structure) for PCD Service DXE.
                                
      
      @throws  EntityException Exception indicate failed to execute this action.
      
    **/
    public void execute() throws EntityException {
        //
        // Get memoryDatabaseManager instance from GlobalData.
        // The memoryDatabaseManager should be initialized for whatever build
        // tools or wizard tools
        //
        if((dbManager = GlobalData.getPCDMemoryDBManager()) == null) {
            throw new EntityException("The instance of PCD memory database manager is null");
        }

        //
        // Collect all PCD information defined in FPD file.
        // Evenry token defind in FPD will be created as an token into 
        // memory database.
        //
        createTokenInDBFromFPD();
        
        //
        // Generate for PEI, DXE PCD DATABASE's definition and initialization.
        //
        genPcdDatabaseSourceCode ();
        
    }

    /**
      This function generates source code for PCD Database.
     
      @param void
      @throws EntityException  If the token does *not* exist in memory database.

    **/
    private void genPcdDatabaseSourceCode()
        throws EntityException {
        String PcdCommonHeaderString = PcdDatabase.getPcdDatabaseCommonDefinitions();

        ArrayList<Token> alPei = new ArrayList<Token> ();
        ArrayList<Token> alDxe = new ArrayList<Token> ();

        dbManager.getTwoPhaseDynamicRecordArray(alPei, alDxe);
        PcdDatabase pcdPeiDatabase = new PcdDatabase (alPei, "PEI", 0);
        pcdPeiDatabase.genCode();
        MemoryDatabaseManager.PcdPeimHString        = PcdCommonHeaderString + pcdPeiDatabase.getHString() + 
                                                      PcdDatabase.getPcdPeiDatabaseDefinitions();
        MemoryDatabaseManager.PcdPeimCString        = pcdPeiDatabase.getCString();

        PcdDatabase pcdDxeDatabase = new PcdDatabase(alDxe, "DXE", alPei.size());
        pcdDxeDatabase.genCode();
        MemoryDatabaseManager.PcdDxeHString   = MemoryDatabaseManager.PcdPeimHString + pcdDxeDatabase.getHString() + 
                                                PcdDatabase.getPcdDxeDatabaseDefinitions();
        MemoryDatabaseManager.PcdDxeCString   = pcdDxeDatabase.getCString();
    }

    /**
      Get component array from FPD.
      
      This function maybe provided by some Global class.
      
      @return List<ModuleInfo> the component array.
      
     */
    private List<ModuleInfo> getComponentsFromFPD() 
        throws EntityException {
        List<ModuleInfo>                            allModules          = new ArrayList<ModuleInfo>();
        FrameworkModulesDocument.FrameworkModules   fModules            = null;
        ModuleSADocument.ModuleSA[]                 modules             = null;
        Map<FpdModuleIdentification, XmlObject>     pcdBuildDefinitions = null;

        pcdBuildDefinitions = GlobalData.getFpdPcdBuildDefinitions();
        if (pcdBuildDefinitions == null) {
            return null;
        }

        //
        // Loop map to retrieve all PCD build definition and Module id 
        // 
        Iterator item = pcdBuildDefinitions.keySet().iterator();
        while (item.hasNext()){
            FpdModuleIdentification id = (FpdModuleIdentification) item.next();
            allModules.add(new ModuleInfo(id, pcdBuildDefinitions.get(id)));    
        }
        
        return allModules;
    }

    /**
      Create token instance object into memory database, the token information
      comes for FPD file. Normally, FPD file will contain all token platform 
      informations.
      
      @return FrameworkPlatformDescriptionDocument   The FPD document instance for furture usage.
      
      @throws EntityException                        Failed to parse FPD xml file.
      
    **/
    private void createTokenInDBFromFPD() 
        throws EntityException {
        int                                 index             = 0;
        int                                 index2            = 0;
        int                                 pcdIndex          = 0;
        List<PcdBuildDefinition.PcdData>    pcdBuildDataArray = new ArrayList<PcdBuildDefinition.PcdData>();
        PcdBuildDefinition.PcdData          pcdBuildData      = null;
        Token                               token             = null;
        List<ModuleInfo>                    modules           = null;
        String                              primaryKey        = null;
        String                              exceptionString   = null;
        UsageInstance                       usageInstance     = null;
        String                              primaryKey1       = null;
        String                              primaryKey2       = null;
        boolean                             isDuplicate       = false;
        Token.PCD_TYPE                      pcdType           = Token.PCD_TYPE.UNKNOWN;
        Token.DATUM_TYPE                    datumType         = Token.DATUM_TYPE.UNKNOWN;
        long                                tokenNumber       = 0;
        String                              moduleName        = null;
        String                              datum             = null;
        int                                 maxDatumSize      = 0;
        String[]                            tokenSpaceStrRet  = null;
        UsageIdentification                 usageId           = null;
        ModuleIdentification                moduleId          = null;

        //
        // ----------------------------------------------
        // 1), Get all <ModuleSA> from FPD file.
        // ----------------------------------------------
        // 
        modules = getComponentsFromFPD();

        if (modules == null) {
            throw new EntityException("[FPD file error] No modules in FPD file, Please check whether there are elements in <FrameworkModules> in FPD file!");
        }

        //
        // -------------------------------------------------------------------
        // 2), Loop all modules to process <PcdBuildDeclarations> for each module.
        // -------------------------------------------------------------------
        // 
        for (index = 0; index < modules.size(); index ++) {
    	    //
    	    // It is legal for a module does not contains ANY pcd build definitions.
    	    // 
    	    if (modules.get(index).getPcdBuildDef() == null) {
                continue;
    	    }
    
            pcdBuildDataArray = modules.get(index).getPcdBuildDef().getPcdDataList();

            moduleName = modules.get(index).getModuleId().getModule().getName();

            //
            // ----------------------------------------------------------------------
            // 2.1), Loop all Pcd entry for a module and add it into memory database.
            // ----------------------------------------------------------------------
            // 
            for (pcdIndex = 0; pcdIndex < pcdBuildDataArray.size(); pcdIndex ++) {
                pcdBuildData = pcdBuildDataArray.get(pcdIndex);
                
                try {
                    tokenSpaceStrRet = GlobalData.getGuidInfoFromCname(pcdBuildData.getTokenSpaceGuidCName());
                } catch ( Exception e ) {
                    throw new EntityException ("Faile get Guid for token " + pcdBuildData.getCName() + ":" + e.getMessage());
                }

                if (tokenSpaceStrRet == null) {
                    throw new EntityException ("Fail to get Token space guid for token" + pcdBuildData.getCName());
                } 

                primaryKey   = Token.getPrimaryKeyString(pcdBuildData.getCName(), tokenSpaceStrRet[1]);
                pcdType      = Token.getpcdTypeFromString(pcdBuildData.getItemType().toString());
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
                    exceptionString = String.format("[FPD file error] For PCD %s in module %s, the PCD type is FEATRUE_FLAG but "+
                                                    "datum type of this PCD entry is not BOOLEAN!",
                                                    pcdBuildData.getCName(),
                                                    moduleName);
                    throw new EntityException(exceptionString);
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
                         exceptionString = String.format("[FPD file error] There is no value for PCD entry %s in module %s!",
                                                         pcdBuildData.getCName(),
                                                         moduleName);
                         throw new EntityException(exceptionString);
                     }

                     //
                     // Check whether the datum size is matched datum type.
                     // 
                     if ((exceptionString = verifyDatum(pcdBuildData.getCName(), 
                                                        moduleName,
                                                        datum,
                                                        datumType,
                                                        maxDatumSize)) != null) {
                         throw new EntityException(exceptionString);
                     }
                }

                //
                // ---------------------------------------------------------------------------------
                // 2.1.2), Create token or update token information for current anaylized PCD data.
                // ---------------------------------------------------------------------------------
                // 
                if (dbManager.isTokenInDatabase(primaryKey)) {
                    //
                    // If the token is already exist in database, do some necessary checking
                    // and add a usage instance into this token in database
                    // 
                    token = dbManager.getTokenByKey(primaryKey);
    
                    //
                    // checking for DatumType, DatumType should be unique for one PCD used in different
                    // modules.
                    // 
                    if (token.datumType != datumType) {
                        exceptionString = String.format("[FPD file error] The datum type of PCD entry %s is %s, which is different with  %s defined in before!",
                                                        pcdBuildData.getCName(), 
                                                        pcdBuildData.getDatumType().toString(), 
                                                        Token.getStringOfdatumType(token.datumType));
                        throw new EntityException(exceptionString);
                    }

                    //
                    // Check token number is valid
                    // 
                    if (tokenNumber != token.tokenNumber) {
                        exceptionString = String.format("[FPD file error] The token number of PCD entry %s in module %s is different with same PCD entry in other modules!",
                                                        pcdBuildData.getCName(),
                                                        moduleName);
                        throw new EntityException(exceptionString);
                    }

                    //
                    // For same PCD used in different modules, the PCD type should all be dynamic or non-dynamic.
                    // 
                    if (token.isDynamicPCD != Token.isDynamic(pcdType)) {
                        exceptionString = String.format("[FPD file error] For PCD entry %s in module %s, you define dynamic or non-dynamic PCD type which"+
                                                        "is different with others module's",
                                                        token.cName,
                                                        moduleName);
                        throw new EntityException(exceptionString);
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
                                exceptionString = String.format("[FPD file error] For dynamic PCD %s in module %s, the datum in <ModuleSA> is "+
                                                                "not equal to the datum in <DynamicPcdBuildDefinitions>, it is "+
                                                                "illega! You could no set <Value> in <ModuleSA> for a dynamic PCD!",
                                                                token.cName,
                                                                moduleName);
                                throw new EntityException(exceptionString);
                            }
                        }

                        if ((maxDatumSize != 0) &&
                            (maxDatumSize != token.datumSize)){
                            exceptionString = String.format("[FPD file error] For dynamic PCD %s in module %s, the max datum size is %d which "+
                                                            "is different with <MaxDatumSize> %d defined in <DynamicPcdBuildDefinitions>!",
                                                            token.cName,
                                                            moduleName,
                                                            maxDatumSize,
                                                            token.datumSize);
                            throw new EntityException(exceptionString);
                        }
                    }
                    
                } else {
                    //
                    // If the token is not in database, create a new token instance and add
                    // a usage instance into this token in database.
                    // 
                    try {
                        tokenSpaceStrRet = GlobalData.getGuidInfoFromCname(pcdBuildData.getTokenSpaceGuidCName());
                    } catch (Exception e) {
                        throw new EntityException("Fail to get token space guid for token " + token.cName);
                    }

                    if (tokenSpaceStrRet == null) {
                        throw new EntityException("Fail to get token space guid for token " + token.cName);
                    }

                    token = new Token(pcdBuildData.getCName(), tokenSpaceStrRet[1]);
    
                    token.datumType     = datumType;
                    token.tokenNumber   = tokenNumber;
                    token.isDynamicPCD  = Token.isDynamic(pcdType);
                    token.datumSize     = maxDatumSize;
                    
                    if (token.isDynamicPCD) {
                        //
                        // For Dynamic and Dynamic Ex type, need find the dynamic information
                        // in <DynamicPcdBuildDefinition> section in FPD file.
                        // 
                        updateDynamicInformation(moduleName, 
                                                 token,
                                                 datum,
                                                 maxDatumSize);
                    }
    
                    dbManager.addTokenToDatabase(primaryKey, token);
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
                moduleId = modules.get(index).getModuleId().getModule();
                usageId = new UsageIdentification (moduleId.getName(), 
                                                   moduleId.getGuid(), 
                                                   moduleId.getPackage().getName(), 
                                                   moduleId.getPackage().getGuid(), 
                                                   modules.get(index).getModuleId().getArch(),
                                                   moduleId.getVersion(),
                                                   moduleId.getModuleType());
                usageInstance = new UsageInstance(token, 
                                                  usageId,
                                                  pcdType,
                                                  datum,
                                                  maxDatumSize);
                token.addUsageInstance(usageInstance);
            }
        }

        //
        // ------------------------------------------------
        // 3), Add unreference dynamic_Ex pcd token into Pcd database.
        // ------------------------------------------------
        // 
        List<Token> tokenArray = getUnreferencedDynamicPcd();
        if (tokenArray != null) {
            for (index = 0; index < tokenArray.size(); index ++) {
                dbManager.addTokenToDatabase(tokenArray.get(index).getPrimaryKeyString(), 
                                             tokenArray.get(index));
            }
        }
    }

    private List<Token> getUnreferencedDynamicPcd () throws EntityException {
        List<Token>                                   tokenArray                 = new ArrayList<Token>();
        Token                                         token                      = null;
        DynamicPcdBuildDefinitions                    dynamicPcdBuildDefinitions = null;
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
        String  tokenSpaceStrRet[];
        String  variableGuidString[];

        //
        // Open fpd document to get <DynamicPcdBuildDefinition> Section.
        // BUGBUG: the function should be move GlobalData in furture.
        // 
        if (fpdDocInstance == null) {
            try {
                fpdDocInstance = (PlatformSurfaceAreaDocument)XmlObject.Factory.parse(new File(fpdFilePath));
            } catch(IOException ioE) {
                throw new EntityException("File IO error for xml file:" + fpdFilePath + "\n" + ioE.getMessage());
            } catch(XmlException xmlE) {
                throw new EntityException("Can't parse the FPD xml fle:" + fpdFilePath + "\n" + xmlE.getMessage());
            }
        }

        dynamicPcdBuildDefinitions = fpdDocInstance.getPlatformSurfaceArea().getDynamicPcdBuildDefinitions();
        if (dynamicPcdBuildDefinitions == null) {
            return null;
        }

        dynamicPcdBuildDataArray = dynamicPcdBuildDefinitions.getPcdBuildDataList();
        for (index2 = 0; index2 < dynamicPcdBuildDataArray.size(); index2 ++) {
            pcdBuildData = dynamicPcdBuildDataArray.get(index2);
            try {
                tokenSpaceStrRet = GlobalData.getGuidInfoFromCname(pcdBuildData.getTokenSpaceGuidCName());
            } catch ( Exception e ) {
                throw new EntityException ("Faile get Guid for token " + pcdBuildData.getCName() + ":" + e.getMessage());
            }

            if (tokenSpaceStrRet == null) {
                throw new EntityException ("Fail to get Token space guid for token" + pcdBuildData.getCName());
            } 

            primaryKey = Token.getPrimaryKeyString(pcdBuildData.getCName(),
                                                   tokenSpaceStrRet[1]);

            if (dbManager.isTokenInDatabase(primaryKey)) {
                continue;
            }

            pcdType = Token.getpcdTypeFromString(pcdBuildData.getItemType().toString());
            if (pcdType != Token.PCD_TYPE.DYNAMIC_EX) {
                throw new EntityException (String.format("[FPD file error] It not allowed for DYNAMIC PCD %s who is no used by any module",
                                                         pcdBuildData.getCName()));
            }

            //
            // Create new token for unreference dynamic PCD token
            // 
            token           = new Token(pcdBuildData.getCName(), tokenSpaceStrRet[1]);
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
                throw new EntityException(exceptionString);
            }

            skuInfoList = pcdBuildData.getSkuInfoList();

            //
            // Loop all sku data 
            // 
            for (index = 0; index < skuInfoList.size(); index ++) {
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
                        throw new EntityException(exceptionString);
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
                        exceptionString = String.format("[FPD file error] For dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                        "file, who use HII, but there is no <VariableGuid> defined for Sku %d data!",
                                                        token.cName,
                                                        index);
                        if (exceptionString != null) {
                            throw new EntityException(exceptionString);
                        }                                                    
                    }

                    if (skuInfoList.get(index).getVariableOffset() == null) {
                        exceptionString = String.format("[FPD file error] For dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                        "file, who use HII, but there is no <VariableOffset> defined for Sku %d data!",
                                                        token.cName,
                                                        index);
                        if (exceptionString != null) {
                            throw new EntityException(exceptionString);
                        }
                    }

                    if (skuInfoList.get(index).getHiiDefaultValue() == null) {
                        exceptionString = String.format("[FPD file error] For dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                        "file, who use HII, but there is no <HiiDefaultValue> defined for Sku %d data!",
                                                        token.cName,
                                                        index);
                        if (exceptionString != null) {
                            throw new EntityException(exceptionString);
                        }
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
                        throw new EntityException(exceptionString);
                    }

                    offset = Integer.decode(skuInfoList.get(index).getVariableOffset());
                    if (offset > 0xFFFF) {
                        throw new EntityException(String.format("[FPD file error] For dynamic PCD %s ,  the variable offset defined in sku %d data "+
                                                                "exceed 64K, it is not allowed!",
                                                                token.cName,
                                                                index));
                    }

                    //
                    // Get variable guid string according to the name of guid which will be mapped into a GUID in SPD file.
                    // 
                    variableGuidString = GlobalData.getGuidInfoFromCname(skuInfoList.get(index).getVariableGuid().toString());
                    if (variableGuidString == null) {
                        throw new EntityException(String.format("[GUID Error] For dynamic PCD %s,  the variable guid %s can be found in all SPD file!",
                                                                token.cName, 
                                                                skuInfoList.get(index).getVariableGuid().toString()));
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
                                                 translateSchemaStringToUUID(variableGuidString[1]),
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

                exceptionString = String.format("[FPD file error] For dynamic PCD %s, the dynamic info must "+
                                                "be one of 'DefaultGroup', 'HIIGroup', 'VpdGroup'.",
                                                token.cName);
                throw new EntityException(exceptionString);
            }

            if (!hasSkuId0) {
                exceptionString = String.format("[FPD file error] For dynamic PCD %s in <DynamicPcdBuildDefinitions>, there are "+
                                                "no sku id = 0 data, which is required for every dynamic PCD",
                                                token.cName);
                throw new EntityException(exceptionString);
            }

            tokenArray.add(token);
        }

        return tokenArray;
    }

    /**
       Verify the datum value according its datum size and datum type, this
       function maybe moved to FPD verification tools in future.
       
       @param cName
       @param moduleName
       @param datum
       @param datumType
       @param maxDatumSize
       
       @return String
     */
    /***/
    public String verifyDatum(String            cName,
                              String            moduleName,
                              String            datum, 
                              Token.DATUM_TYPE  datumType, 
                              int               maxDatumSize) {
        String      exceptionString = null;
        int         value;
        BigInteger  value64;
        String      subStr;
        int         index;

        if (moduleName == null) {
            moduleName = "section <DynamicPcdBuildDefinitions>";
        } else {
            moduleName = "module " + moduleName;
        }

        if (maxDatumSize == 0) {
            exceptionString = String.format("[FPD file error] You maybe miss <MaxDatumSize> for PCD %s in %s",
                                            cName,
                                            moduleName);
            return exceptionString;
        }

        switch (datumType) {
        case UINT8:
            if (maxDatumSize != 1) {
                exceptionString = String.format("[FPD file error] The datum type of PCD data %s in %s "+
                                                "is UINT8, but datum size is %d, they are not matched!",
                                                 cName,
                                                 moduleName,
                                                 maxDatumSize);
                return exceptionString;
            }

            if (datum != null) {
                try {
                    value = Integer.decode(datum);
                } catch (NumberFormatException nfeExp) {
                    exceptionString = String.format("[FPD file error] The datum for PCD %s in %s is not valid "+
                                                    "digital format of UINT8",
                                                    cName,
                                                    moduleName);
                    return exceptionString;
                }
                if (value > 0xFF) {
                    exceptionString = String.format("[FPD file error] The datum for PCD %s in %s is %s exceed"+
                                                    " the max size of UINT8 - 0xFF",
                                                    cName, 
                                                    moduleName,
                                                    datum);
                    return exceptionString;
                }
            }
            break;
        case UINT16:
            if (maxDatumSize != 2) {
                exceptionString = String.format("[FPD file error] The datum type of PCD data %s in %s "+
                                                "is UINT16, but datum size is %d, they are not matched!",
                                                 cName,
                                                 moduleName,
                                                 maxDatumSize);
                return exceptionString;
            }
            if (datum != null) {
                try {
                    value = Integer.decode(datum);
                } catch (NumberFormatException nfeExp) {
                    exceptionString = String.format("[FPD file error] The datum for PCD %s in %s is "+
                                                    "not valid digital of UINT16",
                                                    cName,
                                                    moduleName);
                    return exceptionString;
                }
                if (value > 0xFFFF) {
                    exceptionString = String.format("[FPD file error] The datum for PCD %s in %s is %s "+
                                                    "which exceed the range of UINT16 - 0xFFFF",
                                                    cName, 
                                                    moduleName,
                                                    datum);
                    return exceptionString;
                }
            }
            break;
        case UINT32:
            if (maxDatumSize != 4) {
                exceptionString = String.format("[FPD file error] The datum type of PCD data %s in %s "+
                                                "is UINT32, but datum size is %d, they are not matched!",
                                                 cName,
                                                 moduleName,
                                                 maxDatumSize);
                return exceptionString;
            }

            if (datum != null) {
                try {
                    if (datum.length() > 2) {
                        if ((datum.charAt(0) == '0')        && 
                            ((datum.charAt(1) == 'x') || (datum.charAt(1) == 'X'))){
                            subStr = datum.substring(2, datum.length());
                            value64 = new BigInteger(subStr, 16);
                        } else {
                            value64 = new BigInteger(datum);
                        }
                    } else {
                        value64 = new BigInteger(datum);
                    }
                } catch (NumberFormatException nfeExp) {
                    exceptionString = String.format("[FPD file error] The datum for PCD %s in %s is not "+
                                                    "valid digital of UINT32",
                                                    cName,
                                                    moduleName);
                    return exceptionString;
                }

                if (value64.bitLength() > 32) {
                    exceptionString = String.format("[FPD file error] The datum for PCD %s in %s is %s which "+
                                                    "exceed the range of UINT32 - 0xFFFFFFFF",
                                                    cName, 
                                                    moduleName,
                                                    datum);
                    return exceptionString;
                }
            }
            break;
        case UINT64:
            if (maxDatumSize != 8) {
                exceptionString = String.format("[FPD file error] The datum type of PCD data %s in %s "+
                                                "is UINT64, but datum size is %d, they are not matched!",
                                                 cName,
                                                 moduleName,
                                                 maxDatumSize);
                return exceptionString;
            }

            if (datum != null) {
                try {
                    if (datum.length() > 2) {
                        if ((datum.charAt(0) == '0')        && 
                            ((datum.charAt(1) == 'x') || (datum.charAt(1) == 'X'))){
                            subStr = datum.substring(2, datum.length());
                            value64 = new BigInteger(subStr, 16);
                        } else {
                            value64 = new BigInteger(datum);
                        }
                    } else {
                        value64 = new BigInteger(datum);
                    }
                } catch (NumberFormatException nfeExp) {
                    exceptionString = String.format("[FPD file error] The datum for PCD %s in %s is not valid"+
                                                    " digital of UINT64",
                                                    cName,
                                                    moduleName);
                    return exceptionString;
                }

                if (value64.bitLength() > 64) {
                    exceptionString = String.format("[FPD file error] The datum for PCD %s in %s is %s "+
                                                    "exceed the range of UINT64 - 0xFFFFFFFFFFFFFFFF",
                                                    cName, 
                                                    moduleName,
                                                    datum);
                    return exceptionString;
                }
            }
            break;
        case BOOLEAN:
            if (maxDatumSize != 1) {
                exceptionString = String.format("[FPD file error] The datum type of PCD data %s in %s "+
                                                "is BOOLEAN, but datum size is %d, they are not matched!",
                                                 cName,
                                                 moduleName,
                                                 maxDatumSize);
                return exceptionString;
            }

            if (datum != null) {
                if (!(datum.equalsIgnoreCase("TRUE") ||
                     datum.equalsIgnoreCase("FALSE"))) {
                    exceptionString = String.format("[FPD file error] The datum type of PCD data %s in %s "+
                                                    "is BOOELAN, but value is not 'true'/'TRUE' or 'FALSE'/'false'",
                                                    cName,
                                                    moduleName);
                    return exceptionString;
                }

            }
            break;
        case POINTER:
            if (datum == null) {
                break;
            }

            char    ch     = datum.charAt(0);
            int     start, end;
            String  strValue;
            //
            // For void* type PCD, only three datum is support:
            // 1) Unicode: string with start char is "L"
            // 2) Ansci: String start char is ""
            // 3) byte array: String start char "{"
            // 
            if (ch == 'L') {
                start       = datum.indexOf('\"');
                end         = datum.lastIndexOf('\"');
                if ((start > end)           || 
                    (end   > datum.length())||
                    ((start == end) && (datum.length() > 0))) {
                    exceptionString = String.format("[FPD file error] The datum type of PCD %s in %s is VOID* and datum is "+
                                                    "a UNICODE string because start with L\", but format maybe"+
                                                    "is not right, correct UNICODE string is L\"...\"!",
                                                    cName,
                                                    moduleName);
                    return exceptionString;
                }

                strValue    = datum.substring(start + 1, end);
                if ((strValue.length() * 2) > maxDatumSize) {
                    exceptionString = String.format("[FPD file error] The datum type of PCD %s in %s is VOID*, and datum is "+
                                                    "a UNICODE string, but the datum size is %d exceed to <MaxDatumSize> : %d",
                                                    cName,
                                                    moduleName,
                                                    strValue.length() * 2, 
                                                    maxDatumSize);
                    return exceptionString;
                }
            } else if (ch == '\"'){
                start       = datum.indexOf('\"');
                end         = datum.lastIndexOf('\"');
                if ((start > end)           || 
                    (end   > datum.length())||
                    ((start == end) && (datum.length() > 0))) {
                    exceptionString = String.format("[FPD file error] The datum type of PCD %s in %s is VOID* and datum is "+
                                                    "a ANSCII string because start with \", but format maybe"+
                                                    "is not right, correct ANSIC string is \"...\"!",
                                                    cName,
                                                    moduleName);
                    return exceptionString;
                }
                strValue    = datum.substring(start + 1, end);
                if ((strValue.length()) > maxDatumSize) {
                    exceptionString = String.format("[FPD file error] The datum type of PCD %s in %s is VOID*, and datum is "+
                                                    "a ANSCI string, but the datum size is %d which exceed to <MaxDatumSize> : %d",
                                                    cName,
                                                    moduleName,
                                                    strValue.length(),
                                                    maxDatumSize);
                    return exceptionString;
                }
            } else if (ch =='{') {
                String[]  strValueArray;

                start           = datum.indexOf('{');
                end             = datum.lastIndexOf('}');
                strValue        = datum.substring(start + 1, end);
                strValue        = strValue.trim();
                if (strValue.length() == 0) {
                    exceptionString = String.format ("[FPD file error] The datum type of PCD %s in %s is VOID*, and "+
                                                     "it is byte array in fact, but '{}' is not valid for NULL datam but"+
                                                     " need use '{0}'",
                                                     cName,
                                                     moduleName);
                    return exceptionString;
                }
                strValueArray   = strValue.split(",");
                for (index = 0; index < strValueArray.length; index ++) {
                    try{
                        value = Integer.decode(strValueArray[index].trim());
                    } catch (NumberFormatException nfeEx) {
                        exceptionString = String.format("[FPD file error] The datum type of PCD %s in %s is VOID*, and "+
                                                         "it is byte array in fact. For every byte in array should be a valid"+
                                                         "byte digital, but element %s is not a valid byte digital!",
                                                         cName,
                                                         moduleName,
                                                         strValueArray[index]);
                        return exceptionString;
                    }
                    if (value > 0xFF) {
                        exceptionString = String.format("[FPD file error] The datum type of PCD %s in %s is VOID*, "+
                                                        "it is byte array in fact. But the element of %s exceed the byte range",
                                                        cName,
                                                        moduleName,
                                                        strValueArray[index]);
                        return exceptionString;
                    }
                }

                if (strValueArray.length > maxDatumSize) {
                    exceptionString = String.format("[FPD file error] The datum type of PCD %s in %s is VOID*, and datum is byte"+
                                                    "array, but the number of bytes is %d which exceed to <MaxDatumSzie> : %d!",
                                                    cName,
                                                    moduleName,
                                                    strValueArray.length,
                                                    maxDatumSize);
                    return exceptionString;
                }
            } else {
                exceptionString = String.format("[FPD file error] The datum type of PCD %s in %s is VOID*. For VOID* type, you have three format choise:\n "+
                                                "1) UNICODE string: like L\"xxxx\";\r\n"+
                                                "2) ANSIC string: like \"xxx\";\r\n"+
                                                "3) Byte array: like {0x2, 0x45, 0x23}\r\n"+
                                                "But the datum in seems does not following above format!",
                                                cName, 
                                                moduleName);
                return exceptionString;
            }
            break;
        default:
            exceptionString = String.format("[FPD file error] For PCD entry %s in %s, datum type is unknown, it should be one of "+
                                            "UINT8, UINT16, UINT32, UINT64, VOID*, BOOLEAN",
                                            cName,
                                            moduleName);
            return exceptionString;
        }
        return null;
    }

    /**
       Get dynamic information for a dynamic PCD from <DynamicPcdBuildDefinition> seciton in FPD file.
       
       This function should be implemented in GlobalData in future.
       
       @param token         The token instance which has hold module's PCD information
       @param moduleName    The name of module who will use this Dynamic PCD.
       
       @return DynamicPcdBuildDefinitions.PcdBuildData
     */
    /***/
    private DynamicPcdBuildDefinitions.PcdBuildData getDynamicInfoFromFPD(Token     token,
                                                                          String    moduleName)
        throws EntityException {
        int    index             = 0;
        String exceptionString   = null;
        String dynamicPrimaryKey = null;
        DynamicPcdBuildDefinitions                    dynamicPcdBuildDefinitions = null;
        List<DynamicPcdBuildDefinitions.PcdBuildData> dynamicPcdBuildDataArray   = null;
        String[]                                      tokenSpaceStrRet           = null;

        //
        // If FPD document is not be opened, open and initialize it.
        // BUGBUG: The code should be moved into GlobalData in future.
        // 
        if (fpdDocInstance == null) {
            try {
                fpdDocInstance = (PlatformSurfaceAreaDocument)XmlObject.Factory.parse(new File(fpdFilePath));
            } catch(IOException ioE) {
                throw new EntityException("File IO error for xml file:" + fpdFilePath + "\n" + ioE.getMessage());
            } catch(XmlException xmlE) {
                throw new EntityException("Can't parse the FPD xml fle:" + fpdFilePath + "\n" + xmlE.getMessage());
            }
        }
        
        dynamicPcdBuildDefinitions = fpdDocInstance.getPlatformSurfaceArea().getDynamicPcdBuildDefinitions();
        if (dynamicPcdBuildDefinitions == null) {
            exceptionString = String.format("[FPD file error] There are no <PcdDynamicBuildDescriptions> in FPD file but contains Dynamic type "+
                                            "PCD entry %s in module %s!",
                                            token.cName,
                                            moduleName);
            throw new EntityException(exceptionString);
        }

        dynamicPcdBuildDataArray = dynamicPcdBuildDefinitions.getPcdBuildDataList();
        for (index = 0; index < dynamicPcdBuildDataArray.size(); index ++) {
            //String tokenSpaceGuidString = GlobalData.getGuidInfoFromCname(dynamicPcdBuildDataArray.get(index).getTokenSpaceGuidCName())[1];
            String tokenSpaceGuidString = null;
            try {
                tokenSpaceStrRet = GlobalData.getGuidInfoFromCname(dynamicPcdBuildDataArray.get(index).getTokenSpaceGuidCName());
            } catch (Exception e) {
                throw new EntityException ("Fail to get token space guid for token " + dynamicPcdBuildDataArray.get(index).getCName());
            }
            
            if (tokenSpaceStrRet == null) {
                throw new EntityException ("Fail to get token space guid for token " + dynamicPcdBuildDataArray.get(index).getCName());
            }

            dynamicPrimaryKey = Token.getPrimaryKeyString(dynamicPcdBuildDataArray.get(index).getCName(),
                                                          tokenSpaceStrRet[1]);
            if (dynamicPrimaryKey.equalsIgnoreCase(token.getPrimaryKeyString())) {
                return dynamicPcdBuildDataArray.get(index);
            }
        }

        return null;
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
        throws EntityException {
        int                 index           = 0;
        int                 offset;
        String              exceptionString = null;
        DynamicTokenValue   dynamicValue;
        SkuInstance         skuInstance     = null;
        String              temp;
        boolean             hasSkuId0       = false;
        Token.PCD_TYPE      pcdType         = Token.PCD_TYPE.UNKNOWN;
        long                tokenNumber     = 0;
        String              hiiDefaultValue = null;
        String[]            variableGuidString = null;

        List<DynamicPcdBuildDefinitions.PcdBuildData.SkuInfo>   skuInfoList = null;
        DynamicPcdBuildDefinitions.PcdBuildData                 dynamicInfo = null;

        dynamicInfo = getDynamicInfoFromFPD(token, moduleName);
        if (dynamicInfo == null) {
            exceptionString = String.format("[FPD file error] For Dynamic PCD %s used by module %s, "+
                                            "there is no dynamic information in <DynamicPcdBuildDefinitions> "+
                                            "in FPD file, but it is required!",
                                            token.cName,
                                            moduleName);
            throw new EntityException(exceptionString);
        }

        token.datumSize = dynamicInfo.getMaxDatumSize();

        exceptionString = verifyDatum(token.cName, 
                                      moduleName,
                                      null, 
                                      token.datumType, 
                                      token.datumSize);
        if (exceptionString != null) {
            throw new EntityException(exceptionString);
        }

        if ((maxDatumSize != 0) && 
            (maxDatumSize != token.datumSize)) {
            exceptionString = String.format("FPD file error] For dynamic PCD %s, the datum size in module %s is %d, but "+
                                            "the datum size in <DynamicPcdBuildDefinitions> is %d, they are not match!",
                                            token.cName,
                                            moduleName, 
                                            maxDatumSize,
                                            dynamicInfo.getMaxDatumSize());
            throw new EntityException(exceptionString);
        }
        tokenNumber = Long.decode(dynamicInfo.getToken().toString());
        if (tokenNumber != token.tokenNumber) {
            exceptionString = String.format("[FPD file error] For dynamic PCD %s, the token number in module %s is 0x%x, but"+
                                            "in <DynamicPcdBuildDefinictions>, the token number is 0x%x, they are not match!",
                                            token.cName,
                                            moduleName,
                                            token.tokenNumber,
                                            tokenNumber);
            throw new EntityException(exceptionString);
        }

        pcdType = Token.getpcdTypeFromString(dynamicInfo.getItemType().toString());
        token.dynamicExTokenNumber = tokenNumber;

        skuInfoList = dynamicInfo.getSkuInfoList();

        //
        // Loop all sku data 
        // 
        for (index = 0; index < skuInfoList.size(); index ++) {
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
                    throw new EntityException(exceptionString);
                }

                token.skuData.add(skuInstance);

                //
                // Judege wether is same of datum between module's information
                // and dynamic information.
                // 
                if (datum != null) {
                    if ((skuInstance.id == 0)                                   &&
                        !datum.toString().equalsIgnoreCase(skuInfoList.get(index).getValue().toString())) {
                        exceptionString = "[FPD file error] For dynamic PCD " + token.cName + ", the value in module " + moduleName + " is " + datum.toString() + " but the "+
                                          "value of sku 0 data in <DynamicPcdBuildDefinition> is " + skuInstance.value.value + ". They are must be same!"+
                                          " or you could not define value for a dynamic PCD in every <ModuleSA>!"; 
                        throw new EntityException(exceptionString);
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
                    exceptionString = String.format("[FPD file error] For dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                    "file, who use HII, but there is no <VariableGuid> defined for Sku %d data!",
                                                    token.cName,
                                                    index);
                    if (exceptionString != null) {
                        throw new EntityException(exceptionString);
                    }                                                    
                }

                if (skuInfoList.get(index).getVariableOffset() == null) {
                    exceptionString = String.format("[FPD file error] For dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                    "file, who use HII, but there is no <VariableOffset> defined for Sku %d data!",
                                                    token.cName,
                                                    index);
                    if (exceptionString != null) {
                        throw new EntityException(exceptionString);
                    }
                }

                if (skuInfoList.get(index).getHiiDefaultValue() == null) {
                    exceptionString = String.format("[FPD file error] For dynamic PCD %s in <DynamicPcdBuildDefinitions> section in FPD "+
                                                    "file, who use HII, but there is no <HiiDefaultValue> defined for Sku %d data!",
                                                    token.cName,
                                                    index);
                    if (exceptionString != null) {
                        throw new EntityException(exceptionString);
                    }
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
                    throw new EntityException(exceptionString);
                }

                offset = Integer.decode(skuInfoList.get(index).getVariableOffset());
                if (offset > 0xFFFF) {
                    throw new EntityException(String.format("[FPD file error] For dynamic PCD %s ,  the variable offset defined in sku %d data "+
                                                            "exceed 64K, it is not allowed!",
                                                            token.cName,
                                                            index));
                }

                //
                // Get variable guid string according to the name of guid which will be mapped into a GUID in SPD file.
                // 
                variableGuidString = GlobalData.getGuidInfoFromCname(skuInfoList.get(index).getVariableGuid().toString());
                if (variableGuidString == null) {
                    throw new EntityException(String.format("[GUID Error] For dynamic PCD %s,  the variable guid %s can be found in all SPD file!",
                                                            token.cName, 
                                                            skuInfoList.get(index).getVariableGuid().toString()));
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
                                             translateSchemaStringToUUID(variableGuidString[1]),
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

            exceptionString = String.format("[FPD file error] For dynamic PCD %s, the dynamic info must "+
                                            "be one of 'DefaultGroup', 'HIIGroup', 'VpdGroup'.",
                                            token.cName);
            throw new EntityException(exceptionString);
        }

        if (!hasSkuId0) {
            exceptionString = String.format("[FPD file error] For dynamic PCD %s in <DynamicPcdBuildDefinitions>, there are "+
                                            "no sku id = 0 data, which is required for every dynamic PCD",
                                            token.cName);
            throw new EntityException(exceptionString);
        }

        return token;
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
        throws EntityException {
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
                throw new EntityException ("[FPD file error] Wrong format for UUID string: " + uuidString);
            }

            //
            // Remove blank space from these string and remove header string "0x"
            // 
            for (index = 0; index < 11; index ++) {
                splitStringArray[index] = splitStringArray[index].trim();
                splitStringArray[index] = splitStringArray[index].substring(2, splitStringArray[index].length());
            }

            //
            // Add heading '0' to normalize the string length
            // 
            for (index = 3; index < 11; index ++) {
                chLen = splitStringArray[index].length();
                for (chIndex = 0; chIndex < 2 - chLen; chIndex ++) {
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

    /**
      check parameter for this action.
      
      @throws EntityException  Bad parameter.
    **/
    private void checkParameter() throws EntityException {
        File file = null;

        if((fpdFilePath    == null) ||(workspacePath  == null)) {
            throw new EntityException("WorkspacePath and FPDFileName should be blank for CollectPCDAtion!");
        }

        if(fpdFilePath.length() == 0 || workspacePath.length() == 0) {
            throw new EntityException("WorkspacePath and FPDFileName should be blank for CollectPCDAtion!");
        }

        file = new File(workspacePath);
        if(!file.exists()) {
            throw new EntityException("WorkpacePath " + workspacePath + " does not exist!");
        }

        file = new File(fpdFilePath);

        if(!file.exists()) {
            throw new EntityException("FPD File " + fpdFilePath + " does not exist!");
        }
    }

    /**
      Test case function

      @param argv  parameter from command line
    **/
    public static void main(String argv[]) throws EntityException {
        CollectPCDAction ca = new CollectPCDAction();
        String projectDir = "x:/edk2";
        ca.setWorkspacePath(projectDir);
        ca.setFPDFilePath(projectDir + "/EdkNt32Pkg/Nt32.fpd");
        ca.setActionMessageLevel(ActionMessage.MAX_MESSAGE_LEVEL);
        GlobalData.initInfo("Tools" + File.separator + "Conf" + File.separator + "FrameworkDatabase.db",
                            projectDir,
                            "tools_def.txt");
        System.out.println("After initInfo!");
        FpdParserTask fpt = new FpdParserTask();
        fpt.parseFpdFile(new File(projectDir + "/EdkNt32Pkg/Nt32.fpd"));
        ca.execute();
    }
}
