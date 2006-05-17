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
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import org.apache.xmlbeans.XmlException;
import org.apache.xmlbeans.XmlObject;
import org.tianocore.FrameworkPlatformDescriptionDocument;
import org.tianocore.ModuleSADocument;
import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.PcdBuildDeclarationsDocument.PcdBuildDeclarations.PcdBuildData;
import org.tianocore.PcdDefinitionsDocument.PcdDefinitions;
import org.tianocore.build.autogen.CommonDefinition;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.pcd.action.ActionMessage;
import org.tianocore.build.pcd.entity.MemoryDatabaseManager;
import org.tianocore.build.pcd.entity.SkuInstance;
import org.tianocore.build.pcd.entity.Token;
import org.tianocore.build.pcd.entity.UsageInstance;
import org.tianocore.build.pcd.exception.EntityException;

/** This action class is to collect PCD information from MSA, SPD, FPD xml file.
    This class will be used for wizard and build tools, So it can *not* inherit
    from buildAction or UIAction.
**/
public class CollectPCDAction {
    /// memoryDatabase hold all PCD information collected from SPD, MSA, FPD.
    private MemoryDatabaseManager dbManager;

    /// Workspacepath hold the workspace information.
    private String                workspacePath;

    /// FPD file is the root file. 
    private String                fpdFilePath;

    /// Message level for CollectPCDAction.
    private int                   originalMessageLevel;

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
      1) Get all token's platform information from FPD, and create token object into memory database.
      2) Get all token's module information from MSA, and create usage instance for every module's PCD entry.
      3) Get all token's inherited information from MSA's library, and create usage instance 
         for module who consume this library and create usage instance for library for building.
      4) Collect token's package information from SPD, update these information for token in memory
         database.
      
      @throws  EntityException Exception indicate failed to execute this action.
      
    **/
    private void execute() throws EntityException {
        FrameworkPlatformDescriptionDocument fpdDoc               = null;
        Object[][]                           modulePCDArray       = null;
        Map<String, XmlObject>               docMap               = null;
        ModuleSADocument.ModuleSA[]          moduleSAs            = null;
        UsageInstance                        usageInstance        = null;
        String                               packageName          = null;
        String                               packageFullPath      = null;
        int                                  index                = 0;
        int                                  libraryIndex         = 0;
        int                                  pcdArrayIndex        = 0;
        List<String>                         listLibraryInstance  = null;
        String                               componentTypeStr     = null;

        //
        // Collect all PCD information defined in FPD file.
        // Evenry token defind in FPD will be created as an token into 
        // memory database.
        //
        fpdDoc = createTokenInDBFromFPD();

        //
        // Searching MSA and SPD document. 
        // The information of MSA will be used to create usage instance into database.
        // The information of SPD will be used to update the token information in database.
        //

        HashMap<String, XmlObject> map = new HashMap<String, XmlObject>();
        map.put("FrameworkPlatformDescription", fpdDoc);
        SurfaceAreaQuery.setDoc(map);    

        moduleSAs = SurfaceAreaQuery.getFpdModules();
        for(index = 0; index < moduleSAs.length; index ++) {
            //
            // Get module document and use SurfaceAreaQuery to get PCD information
            //
            docMap = GlobalData.getDoc(moduleSAs[index].getModuleName());
            SurfaceAreaQuery.setDoc(docMap);
            modulePCDArray    = SurfaceAreaQuery.getModulePCDTokenArray();
            componentTypeStr  = SurfaceAreaQuery.getComponentType();
            packageName       = 
                GlobalData.getPackageNameForModule(moduleSAs[index].getModuleName());
            packageFullPath   = this.workspacePath + File.separator    +
                                GlobalData.getPackagePath(packageName) +
                                packageName + ".spd";

            if(modulePCDArray != null) {
                //
                // If current MSA contains <PCDs> information, then create usage
                // instance for PCD information from MSA
                //
                for(pcdArrayIndex = 0; pcdArrayIndex < modulePCDArray.length; 
                     pcdArrayIndex ++) {
                    usageInstance = 
                        createUsageInstanceFromMSA(moduleSAs[index].getModuleName(),
                                                   modulePCDArray[pcdArrayIndex]);

                    if(usageInstance == null) {
                        continue;
                    }
                    //
                    // Get remaining PCD information from the package which this module belongs to
                    //
                    updateTokenBySPD(usageInstance, packageFullPath);
                }
            }

            //
            // Get inherit PCD information which inherit from library instance of this module.
            //
            listLibraryInstance = 
                SurfaceAreaQuery.getLibraryInstance(moduleSAs[index].getArch().toString(),
                                                    CommonDefinition.AlwaysConsumed);
            if(listLibraryInstance != null) {
                for(libraryIndex = 0; libraryIndex < listLibraryInstance.size(); 
                     libraryIndex ++) {
                    inheritPCDFromLibraryInstance(listLibraryInstance.get(libraryIndex),
                                                  moduleSAs[index].getModuleName(),
                                                  packageName,
                                                  componentTypeStr);
                }
            }
        }
    }

    /**
      This function will collect inherit PCD information from library for a module.
     
      This function will create two usage instance for inherited PCD token, one is 
      for module and another is for library.
      For module, if it inherited a PCD token from library, this PCD token's value 
      should be instanced in module level, and belongs to module.
      For library, it also need a usage instance for build.
      
      @param libraryName         The name of library instance.
      @param moduleName          The name of module.
      @param packageName         The name of package while module belongs to.
      @param parentcomponentType The component type of module.
      
      @throws EntityException  If the token does *not* exist in memory database.
      
    **/
    private void inheritPCDFromLibraryInstance(String libraryName,
                                               String moduleName,
                                               String packageName,
                                               String parentcomponentType) 
        throws EntityException {
        Map<String, XmlObject>  docMap            = null;
        String                  primaryKeyString  = null;
        Object[][]              libPcdDataArray   = null;
        UUID                    nullUUID          = new UUID(0,0);
        UUID                    platformUUID      = nullUUID;
        UUID                    tokenSpaceGuid    = null;
        int                     tokenIndex        = 0;
        Token                   token             = null;
        Token.PCD_TYPE          pcdType           = Token.PCD_TYPE.UNKNOWN;
        UsageInstance           usageInstance     = null;
        String                  packageFullPath   = null;

        //
        // Query PCD information from library's document.
        //
        docMap          = GlobalData.getDoc(libraryName);
        SurfaceAreaQuery.setDoc(docMap);
        libPcdDataArray = SurfaceAreaQuery.getModulePCDTokenArray();

        if(libPcdDataArray == null) {
            return;
        }

        for(tokenIndex = 0; tokenIndex < libPcdDataArray.length; tokenIndex ++) {
            tokenSpaceGuid =((UUID)libPcdDataArray[tokenIndex][2] == null) ? 
                             nullUUID :(UUID)libPcdDataArray[tokenIndex][2];

            //
            // Get token from memory database. The token must be created from FPD already.
            //
            primaryKeyString = Token.getPrimaryKeyString((String)libPcdDataArray[tokenIndex][0],
                                                         tokenSpaceGuid,
                                                         platformUUID
                                                         );

            if(dbManager.isTokenInDatabase(primaryKeyString)) {
                token = dbManager.getTokenByKey(primaryKeyString);
            } else {
                throw new EntityException("The PCD token " + primaryKeyString + 
                                          " defined in module " + moduleName + 
                                          " does not exist in FPD file!");
            }      

            //
            // Create usage instance for module.
            //
            pcdType = Token.getpcdTypeFromString((String)libPcdDataArray[tokenIndex][1]);
            usageInstance = new UsageInstance(token,
                                              Token.PCD_USAGE.ALWAYS_CONSUMED,
                                              pcdType,
                                              CommonDefinition.getComponentType(parentcomponentType),
                                              libPcdDataArray[tokenIndex][3],
                                              null,
                                             (String) libPcdDataArray[tokenIndex][5],
                                              "",
                                              moduleName,
                                              packageName,
                                              true);
            if(Token.PCD_USAGE.UNKNOWN == token.isUsageInstanceExist(moduleName)) {
                token.addUsageInstance(usageInstance);

                packageFullPath = this.workspacePath + File.separator    +
                                  GlobalData.getPackagePath(packageName) +
                                  packageName + ".spd";
                updateTokenBySPD(usageInstance, packageFullPath);
            }

            //
            // We need create second usage instance for inherited case, which
            // add library as an usage instance, because when build a module, and 
            // if module inherited from base library, then build process will build
            // library at first. 
            //
            if(Token.PCD_USAGE.UNKNOWN == token.isUsageInstanceExist(libraryName)) {
                packageName   = GlobalData.getPackageNameForModule(libraryName);
                usageInstance = new UsageInstance(token,
                                                  Token.PCD_USAGE.ALWAYS_CONSUMED,
                                                  pcdType,
                                                  CommonDefinition.ComponentTypeLibrary,
                                                  libPcdDataArray[tokenIndex][3],
                                                  null,
                                                 (String)libPcdDataArray[tokenIndex][5],
                                                  "",
                                                  libraryName,
                                                  packageName,
                                                  false);
                token.addUsageInstance(usageInstance);
            }
        }
    }

    /**
      Create usage instance for PCD token defined in MSA document

      A PCD token maybe used by many modules, and every module is one of usage
      instance of this token. For ALWAY_CONSUMED, SOMETIMES_CONSUMED, it is 
      consumer type usage instance of this token, and for ALWAYS_PRODUCED, 
      SOMETIMES_PRODUCED, it is produce type usage instance.
     
      @param moduleName      The name of module 
      @param tokenInfoInMsa  The PCD token information array retrieved from MSA.
      
      @return UsageInstance  The usage instance created in memroy database.
      
      @throws EntityException  If token did not exist in database yet.
      
    **/
    private UsageInstance createUsageInstanceFromMSA(String   moduleName,
                                                     Object[] tokenInfoInMsa) 
        throws EntityException {
        String          packageName         = null;
        UsageInstance   usageInstance       = null;
        UUID            tokenSpaceGuid      = null;
        UUID            nullUUID            = new UUID(0,0);
        String          primaryKeyString    = null;
        UUID            platformTokenSpace  = nullUUID;
        Token           token               = null;
        Token.PCD_TYPE  pcdType             = Token.PCD_TYPE.UNKNOWN;
        Token.PCD_USAGE pcdUsage            = Token.PCD_USAGE.UNKNOWN;

        tokenSpaceGuid =((UUID)tokenInfoInMsa[2] == null) ? nullUUID :(UUID)tokenInfoInMsa[2];

        primaryKeyString = Token.getPrimaryKeyString((String)tokenInfoInMsa[0],
                                                     tokenSpaceGuid,
                                                     platformTokenSpace);

        //
        // Get token object from memory database firstly.
        //
        if(dbManager.isTokenInDatabase(primaryKeyString)) {
            token = dbManager.getTokenByKey(primaryKeyString);
        } else {
            throw new EntityException("The PCD token " + primaryKeyString + " defined in module " + 
                                      moduleName + " does not exist in FPD file!" );
        }
        pcdType     = Token.getpcdTypeFromString((String)tokenInfoInMsa[1]);
        pcdUsage    = Token.getUsageFromString((String)tokenInfoInMsa[4]);

        packageName = GlobalData.getPackageNameForModule(moduleName);

        if(Token.PCD_USAGE.UNKNOWN != token.isUsageInstanceExist(moduleName)) {
            //
            // BUGBUG: It is legal that same base name exist in one FPD file. In furture
            //         we should use "Guid, Version, Package" and "Arch" to differ a module.
            //         So currently, warning should be disabled.
            //
            //ActionMessage.warning(this,
            //                      "In module " + moduleName + " exist more than one PCD token " + token.cName
            //                      );
            return null;
        }

        //
        // BUGBUG: following code could be enabled at current schema. Because 
        //         current schema does not provide usage information.
        // 
        // For FEATRURE_FLAG, FIXED_AT_BUILD, PATCH_IN_MODULE type PCD token, his 
        // usage is always ALWAYS_CONSUMED
        //
        //if((pcdType != Token.PCD_TYPE.DYNAMIC) &&
        //   (pcdType != Token.PCD_TYPE.DYNAMIC_EX)) {
        pcdUsage = Token.PCD_USAGE.ALWAYS_CONSUMED;
        //}

        usageInstance = new UsageInstance(token,
                                          pcdUsage,
                                          pcdType,
                                          CommonDefinition.getComponentType(SurfaceAreaQuery.getComponentType()),
                                          tokenInfoInMsa[3],
                                          null,
                                         (String) tokenInfoInMsa[5],
                                          "",
                                          moduleName,
                                          packageName,
                                          false);

        //
        // Use default value defined in MSA to update datum of token,
        // if datum of token does not defined in FPD file.
        //
        if((token.datum == null) &&(tokenInfoInMsa[3] != null)) {
            token.datum = tokenInfoInMsa[3];
        }

        token.addUsageInstance(usageInstance);

        return usageInstance;
    }

    /**
      Create token instance object into memory database, the token information
      comes for FPD file. Normally, FPD file will contain all token platform 
      informations.
     
      This fucntion should be executed at firsly before others collection work
      such as searching token information from MSA, SPD.
      
      @return FrameworkPlatformDescriptionDocument   The FPD document instance for furture usage.
      
      @throws EntityException                        Failed to parse FPD xml file.
      
    **/
    private FrameworkPlatformDescriptionDocument createTokenInDBFromFPD() 
        throws EntityException {
        XmlObject                            doc               = null;
        FrameworkPlatformDescriptionDocument fpdDoc            = null;
        int                                  index             = 0;
        List<PcdBuildData>                   pcdBuildDataArray = new ArrayList<PcdBuildData>();
        PcdBuildData                         pcdBuildData      = null;
        Token                                token             = null;
        UUID                                 nullUUID          = new UUID(0,0);
        UUID                                 platformTokenSpace= nullUUID;
        List                                 skuDataArray      = new ArrayList();
        SkuInstance                          skuInstance       = null;
        int                                  skuIndex          = 0;

        //
        // Get all tokens from FPD file and create token into database.
        // 

        try {
            doc = XmlObject.Factory.parse(new File(fpdFilePath));
        } catch(IOException ioE) {
            throw new EntityException("Can't find the FPD xml fle:" + fpdFilePath);
        } catch(XmlException xmlE) {
            throw new EntityException("Can't parse the FPD xml fle:" + fpdFilePath);
        }

        //
        // Get memoryDatabaseManager instance from GlobalData.
        //
        if((dbManager = GlobalData.getPCDMemoryDBManager()) == null) {
            throw new EntityException("The instance of PCD memory database manager is null");
        }

        dbManager = new MemoryDatabaseManager();

        if(!(doc instanceof FrameworkPlatformDescriptionDocument)) {
            throw new EntityException("File " + fpdFilePath + 
                                       " is not a FrameworkPlatformDescriptionDocument");
        }

        fpdDoc =(FrameworkPlatformDescriptionDocument)doc;

        //
        // Add all tokens in FPD into Memory Database.
        //
        pcdBuildDataArray = 
            fpdDoc.getFrameworkPlatformDescription().getPcdBuildDeclarations().getPcdBuildDataList();
        for(index = 0; 
             index < fpdDoc.getFrameworkPlatformDescription().getPcdBuildDeclarations().sizeOfPcdBuildDataArray(); 
             index ++) {
            pcdBuildData = pcdBuildDataArray.get(index);
            token        = new Token(pcdBuildData.getCName(), new UUID(0, 0), new UUID(0, 0));
            //
            // BUGBUG: in FPD, <defaultValue> should be defined as <Value>
            //
            token.datum        = pcdBuildData.getDefaultValue();
            token.tokenNumber  = Integer.decode(pcdBuildData.getToken().getStringValue());
            token.hiiEnabled   = pcdBuildData.getHiiEnable();
            token.variableGuid = Token.getGUIDFromSchemaObject(pcdBuildData.getVariableGuid());
            token.variableName = pcdBuildData.getVariableName();
            token.variableOffset = Integer.decode(pcdBuildData.getDataOffset());
            token.skuEnabled   = pcdBuildData.getSkuEnable();
            token.maxSkuCount  = Integer.decode(pcdBuildData.getMaxSku());
            token.skuId        = Integer.decode(pcdBuildData.getSkuId());
            token.skuDataArrayEnabled  = pcdBuildData.getSkuDataArrayEnable();
            token.assignedtokenNumber  = Integer.decode(pcdBuildData.getToken().getStringValue());
            skuDataArray               = pcdBuildData.getSkuDataArray1();
            token.datumType    = Token.getdatumTypeFromString(pcdBuildData.getDatumType().toString());

            if(skuDataArray != null) {
                for(skuIndex = 0; skuIndex < skuDataArray.size(); skuIndex ++) {
                    //
                    // BUGBUG: Now in current schema, The value is defined as String type, 
                    // it is not correct, the type should be same as the datumType
                    //
                    skuInstance = new SkuInstance(((PcdBuildData.SkuData)skuDataArray.get(skuIndex)).getId(),
                                                  ((PcdBuildData.SkuData)skuDataArray.get(skuIndex)).getValue());
                    token.skuData.add(skuInstance);
                }
            }

            if(dbManager.isTokenInDatabase(Token.getPrimaryKeyString(token.cName, 
                                                                      token.tokenSpaceName, 
                                                                      platformTokenSpace))) {
                //
                // If found duplicate token, Should tool be hold?
                //
                ActionMessage.warning(this, 
                                       "Token " + token.cName + " exists in token database");
                continue;
            }
            token.pcdType = Token.getpcdTypeFromString(pcdBuildData.getItemType().toString());
            dbManager.addTokenToDatabase(Token.getPrimaryKeyString(token.cName, 
                                                                   token.tokenSpaceName, 
                                                                   platformTokenSpace), 
                                         token);
        }

        return fpdDoc;
    }

    /**
      Update PCD token in memory database by help information in SPD.

      After create token from FPD and create usage instance from MSA, we should collect
      PCD package level information from SPD and update token information in memory 
      database.
      
      @param usageInstance   The usage instance defined in MSA and want to search in SPD.
      @param packageFullPath The SPD file path.
      
      @throws EntityException Failed to parse SPD xml file.
      
    **/
    private void updateTokenBySPD(UsageInstance  usageInstance,
                                  String         packageFullPath) 
        throws EntityException {
        PackageSurfaceAreaDocument      pkgDoc          = null;
        PcdDefinitions                  pcdDefinitions  = null;
        List<PcdDefinitions.PcdEntry>   pcdEntryArray   = new ArrayList<PcdDefinitions.PcdEntry>();
        int                             index           = 0;
        boolean                         isFoundInSpd    = false;
        Token.DATUM_TYPE                datumType       = Token.DATUM_TYPE.UNKNOWN;

        try {
            pkgDoc =(PackageSurfaceAreaDocument)XmlObject.Factory.parse(new File(packageFullPath));
        } catch(IOException ioE) {
            throw new EntityException("Can't find the FPD xml fle:" + packageFullPath);
        } catch(XmlException xmlE) {
            throw new EntityException("Can't parse the FPD xml fle:" + packageFullPath);
        }
        pcdDefinitions = pkgDoc.getPackageSurfaceArea().getPcdDefinitions();
        //
        // It is illege for SPD file does not contains any PCD information.
        //
        if (pcdDefinitions == null) {
            return;
        }

        pcdEntryArray = pcdDefinitions.getPcdEntryList();
        if (pcdEntryArray == null) {
            return;
        }
        for(index = 0; index < pcdEntryArray.size(); index ++) {
            if(pcdEntryArray.get(index).getCName().equalsIgnoreCase(
                usageInstance.parentToken.cName)) {
                isFoundInSpd = true;
                //
                // From SPD file , we can get following information.
                //  Token:        Token number defined in package level.
                //  PcdItemType:  This item does not single one. It means all supported item type.
                //  datumType:    UINT8, UNIT16, UNIT32, UINT64, VOID*, BOOLEAN 
                //  datumSize:    The size of default value or maxmine size.
                //  defaultValue: This value is defined in package level.
                //  HelpText:     The help text is provided in package level.
                //

                usageInstance.parentToken.tokenNumber = Integer.decode(pcdEntryArray.get(index).getToken());

                if(pcdEntryArray.get(index).getDatumType() != null) {
                    datumType = Token.getdatumTypeFromString(
                        pcdEntryArray.get(index).getDatumType().toString());
                    if(usageInstance.parentToken.datumType == Token.DATUM_TYPE.UNKNOWN) {
                        usageInstance.parentToken.datumType = datumType;
                    } else {
                        if(datumType != usageInstance.parentToken.datumType) {
                            throw new EntityException("Different datum types are defined for Token :" + 
                                                      usageInstance.parentToken.cName);
                        }
                    }

                } else {
                    throw new EntityException("The datum type for token " + usageInstance.parentToken.cName + 
                                              " is not defind in SPD file " + packageFullPath);
                }

                usageInstance.defaultValueInSPD = pcdEntryArray.get(index).getDefaultValue();
                usageInstance.helpTextInSPD     = "Help Text in SPD";

                //
                // If token's datum is not valid, it indicate that datum is not provided
                // in FPD and defaultValue is not provided in MSA, then use defaultValue
                // in SPD as the datum of token.
                //
                if(usageInstance.parentToken.datum == null) {
                    if(pcdEntryArray.get(index).getDefaultValue() != null) {
                        usageInstance.parentToken.datum = pcdEntryArray.get(index).getDefaultValue();
                    } else {
                        throw new EntityException("FPD does not provide datum for token " + usageInstance.parentToken.cName +
                                                  ", MSA and SPD also does not provide <defaultValue> for this token!");
                    }
                }
            }
        }
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
        ca.setWorkspacePath("G:/mdk");
        ca.setFPDFilePath("G:/mdk/EdkNt32Pkg/build/Nt32.fpd");
        ca.setActionMessageLevel(ActionMessage.MAX_MESSAGE_LEVEL);
        GlobalData.initInfo("Tools" + File.separator + "Conf" + File.separator + "FrameworkDatabase.db",
                            "G:/mdk");
        ca.execute();
    }
}
