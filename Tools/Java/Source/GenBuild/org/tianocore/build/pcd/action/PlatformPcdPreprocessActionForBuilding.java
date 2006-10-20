/** @file
  PlatformPcdPreprocessActionForBuilding class.

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
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import org.apache.xmlbeans.XmlException;
import org.apache.xmlbeans.XmlObject;
import org.tianocore.DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions;
import org.tianocore.PcdBuildDefinitionDocument;
import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.build.exception.PlatformPcdPreprocessBuildException;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.id.FpdModuleIdentification;
import org.tianocore.pcd.action.PlatformPcdPreprocessAction;
import org.tianocore.pcd.entity.MemoryDatabaseManager;
import org.tianocore.pcd.entity.ModulePcdInfoFromFpd;
import org.tianocore.pcd.entity.Token;
import org.tianocore.pcd.entity.UsageIdentification;
import org.tianocore.pcd.exception.EntityException;
import org.tianocore.pcd.exception.PlatformPcdPreprocessException;

/**
   This action class is to collect PCD information from MSA, SPD, FPD xml file.
   This class will be used for wizard and build tools, So it can *not* inherit
   from buildAction or UIAction.
**/
public class PlatformPcdPreprocessActionForBuilding extends PlatformPcdPreprocessAction {
    ///
    /// FPD file path.
    ///
    private String                      fpdFilePath;

    ///
    /// Cache the fpd docment instance for private usage.
    ///
    private PlatformSurfaceAreaDocument fpdDocInstance;

    /**
      Set FPDFileName parameter for this action class.

      @param fpdFilePath    fpd file path
    **/
    public void setFPDFilePath(String fpdFilePath) {
        this.fpdFilePath = fpdFilePath;
    }

    /**
      Common function interface for outer.

      @param fpdFilePath    The fpd file path of current build or processing.

      @throws  PlatformPreprocessBuildException 
                            The exception of this function. Because it can *not* be predict
                            where the action class will be used. So only Exception can be throw.

    **/
    public void perform(String fpdFilePath) 
        throws PlatformPcdPreprocessBuildException {
        this.fpdFilePath = fpdFilePath;
        checkParameter();
        execute();
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
    public void execute() throws PlatformPcdPreprocessBuildException {
        String errorMessageHeader   = "Failed to initialize the Pcd memory database because: ";
        String errorsForPreprocess  = null;

        //
        // Get memoryDatabaseManager instance from GlobalData.
        // The memoryDatabaseManager should be initialized as static variable
        // in some Pre-process class.
        //
        setPcdDbManager(GlobalData.getPCDMemoryDBManager());

        //
        // Collect all PCD information defined in FPD file.
        // Evenry token defind in FPD will be created as an token into
        // memory database.
        //
        try {
            initPcdMemoryDbWithPlatformInfo();
        } catch (PlatformPcdPreprocessException exp) {
            throw new PlatformPcdPreprocessBuildException(errorMessageHeader + exp.getMessage());
        }
        errorsForPreprocess = this.getErrorString();
        if (errorsForPreprocess != null) {
            throw new PlatformPcdPreprocessBuildException(errorMessageHeader + "\r\n" + errorsForPreprocess);
        }

        //
        // Generate for PEI, DXE PCD DATABASE's definition and initialization.
        //
        try {
            genPcdDatabaseSourceCode ();
        } catch (EntityException exp) {
            throw new PlatformPcdPreprocessBuildException(errorMessageHeader + "\r\n" + exp.getMessage());
        }
    }

    /**
      Override function: implementate the method of get Guid string information from SPD file.

      @param guidCName      Guid CName string.

      @return String        Guid information from SPD file.
    **/
    public String getGuidInfoFromSpd(String guidCName) {
        return GlobalData.getGuidInfoFromCname(guidCName);
    }

    /**
      This function generates source code for PCD Database.

      @throws EntityException  If the token does *not* exist in memory database.

    **/
    private void genPcdDatabaseSourceCode()
        throws EntityException {
        String PcdCommonHeaderString = PcdDatabase.getPcdDatabaseCommonDefinitions();

        ArrayList<Token> alPei = new ArrayList<Token> ();
        ArrayList<Token> alDxe = new ArrayList<Token> ();

        getPcdDbManager().getTwoPhaseDynamicRecordArray(alPei, alDxe);
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
      Override function: Get component array from FPD.

      This function maybe provided by some Global class.

      @return List<ModuleInfo>                  the component array.
      @throws PlatformPcdPreprocessException    get all modules in <ModuleSA> in FPD file.

    **/
    public List<ModulePcdInfoFromFpd> getComponentsFromFpd()
        throws PlatformPcdPreprocessException {
        List<ModulePcdInfoFromFpd>                  allModules          = new ArrayList<ModulePcdInfoFromFpd>();
        Map<FpdModuleIdentification, XmlObject>     pcdBuildDefinitions = null;
        UsageIdentification                         usageId             = null;

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
            usageId                    = new UsageIdentification(id.getModule().getName(),
                                                                 id.getModule().getGuid(),
                                                                 id.getModule().getPackage().getName(),
                                                                 id.getModule().getPackage().getGuid(),
                                                                 id.getArch(),
                                                                 id.getModule().getVersion(),
                                                                 id.getModule().getModuleType());
            allModules.add(
                new ModulePcdInfoFromFpd(
                    usageId, 
                    ((PcdBuildDefinitionDocument)pcdBuildDefinitions.get(id)).getPcdBuildDefinition()));
        }
        return allModules;
    }

    /**
       Override function: Verify the datum value according its datum size and datum type, this
       function maybe moved to FPD verification tools in future.

       @param cName         The token name
       @param moduleName    The module who use this PCD token
       @param datum         The PCD's datum
       @param datumType     The PCD's datum type
       @param maxDatumSize  The max size for PCD's Datum.

       @return String       exception strings.
     */
    public String verifyDatum(String            cName,
                              String            moduleName,
                              String            datum,
                              Token.DATUM_TYPE  datumType,
                              int               maxDatumSize) {
        //
        // In building system, datum should not be checked, the checking work
        // should be done by wizard tools or PCD verification tools.
        //                                
        return null;
    }

    /**
       Override function: Get dynamic information for a dynamic PCD from <DynamicPcdBuildDefinition> seciton in FPD file.

       This function should be implemented in GlobalData in future.

       @param token         The token instance which has hold module's PCD information
       @param moduleName    The name of module who will use this Dynamic PCD.

       @return DynamicPcdBuildDefinitions.PcdBuildData
    **/
    public DynamicPcdBuildDefinitions.PcdBuildData getDynamicInfoFromFpd(Token     token,
                                                                         String    moduleName)
        throws PlatformPcdPreprocessException {
        int    index             = 0;
        String exceptionString   = null;
        String dynamicPrimaryKey = null;
        DynamicPcdBuildDefinitions                    dynamicPcdBuildDefinitions = null;
        List<DynamicPcdBuildDefinitions.PcdBuildData> dynamicPcdBuildDataArray   = null;
        String                                        tokenSpaceStrRet           = null;

        //
        // If FPD document is not be opened, open and initialize it.
        // BUGBUG: The code should be moved into GlobalData in future.
        //
        if (fpdDocInstance == null) {
            try {
                fpdDocInstance = (PlatformSurfaceAreaDocument)XmlObject.Factory.parse(new File(fpdFilePath));
            } catch(IOException ioE) {
                throw new PlatformPcdPreprocessException("File IO error for xml file:" + fpdFilePath + "\n" + ioE.getMessage());
            } catch(XmlException xmlE) {
                throw new PlatformPcdPreprocessException("Can't parse the FPD xml fle:" + fpdFilePath + "\n" + xmlE.getMessage());
            }
        }

        dynamicPcdBuildDefinitions = fpdDocInstance.getPlatformSurfaceArea().getDynamicPcdBuildDefinitions();
        if (dynamicPcdBuildDefinitions == null) {
            exceptionString = String.format("[FPD file error] There are no <PcdDynamicBuildDescriptions> elements in FPD file but there are Dynamic type "+
                                            "PCD entries %s in module %s!",
                                            token.cName,
                                            moduleName);
            putError(exceptionString);
            return null;
        }

        dynamicPcdBuildDataArray = dynamicPcdBuildDefinitions.getPcdBuildDataList();
        for (index = 0; index < dynamicPcdBuildDataArray.size(); index ++) {
            tokenSpaceStrRet = getGuidInfoFromSpd(dynamicPcdBuildDataArray.get(index).getTokenSpaceGuidCName());

            if (tokenSpaceStrRet == null) {
                exceptionString = "Fail to get token space guid for token " + dynamicPcdBuildDataArray.get(index).getCName();
                putError(exceptionString);
                continue;
            }

            dynamicPrimaryKey = Token.getPrimaryKeyString(dynamicPcdBuildDataArray.get(index).getCName(),
                                                          tokenSpaceStrRet);
            if (dynamicPrimaryKey.equals(token.getPrimaryKeyString())) {
                return dynamicPcdBuildDataArray.get(index);
            }
        }

        return null;
    }

    /**
       Override function: get all <DynamicPcdBuildDefinition> from FPD file.

       @return List<DynamicPcdBuildDefinitions.PcdBuildData>    All DYNAMIC PCD list in <DynamicPcdBuildDefinitions> in FPD file.
       @throws PlatformPcdPreprocessBuildException              Failure to get dynamic information list.

    **/
    public List<DynamicPcdBuildDefinitions.PcdBuildData>
                                            getAllDynamicPcdInfoFromFpd()
        throws PlatformPcdPreprocessException {
        DynamicPcdBuildDefinitions dynamicPcdBuildDefinitions = null;

        //
        // Open fpd document to get <DynamicPcdBuildDefinition> Section.
        // BUGBUG: the function should be move GlobalData in furture.
        //
        if (fpdDocInstance == null) {
            try {
                fpdDocInstance = (PlatformSurfaceAreaDocument)XmlObject.Factory.parse(new File(fpdFilePath));
            } catch(IOException ioE) {
                throw new PlatformPcdPreprocessException("File IO error for xml file:" + fpdFilePath + "\n" + ioE.getMessage());
            } catch(XmlException xmlE) {
                throw new PlatformPcdPreprocessException("Can't parse the FPD xml fle:" + fpdFilePath + "\n" + xmlE.getMessage());
            }
        }

        dynamicPcdBuildDefinitions = fpdDocInstance.getPlatformSurfaceArea().getDynamicPcdBuildDefinitions();
        if (dynamicPcdBuildDefinitions == null) {
            return null;
        }

        return dynamicPcdBuildDefinitions.getPcdBuildDataList();
    }

    /**
      check parameter for this action.

      @throws PlatformPcdPreprocessBuildException  Bad parameter.
    **/
    private void checkParameter() throws PlatformPcdPreprocessBuildException {
        File file = null;

        if (fpdFilePath == null) {
            throw new PlatformPcdPreprocessBuildException("FPDFileName should be empty for CollectPCDAtion!");
        }

        if (fpdFilePath.length() == 0) {
            throw new PlatformPcdPreprocessBuildException("FPDFileName should be empty for CollectPCDAtion!");
        }

        file = new File(fpdFilePath);

        if(!file.exists()) {
            throw new PlatformPcdPreprocessBuildException("FPD File " + fpdFilePath + " does not exist!");
        }
    }
}
