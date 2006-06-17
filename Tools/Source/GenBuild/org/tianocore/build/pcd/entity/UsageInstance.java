/** @file
  UsageInstance class.

  This class indicate an usage instance for a PCD token. This instance maybe a module
  or platform setting. When a module produce or cosume a PCD token, then this module
  is an usage instance for this PCD token.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
 
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/  
package org.tianocore.build.pcd.entity;


import java.util.UUID;

import org.tianocore.build.autogen.CommonDefinition;
import org.tianocore.build.pcd.action.ActionMessage;
import org.tianocore.build.pcd.exception.EntityException;

/**
  This class indicate an usage instance for a PCD token. This instance maybe a module
  or platform setting. When a module produce or cosume a PCD token, then this module
  is an usage instance for this PCD token.
**/
public class UsageInstance {
    ///
    /// The module type of usage instance.
    /// 
    public enum MODULE_TYPE {SEC, PEI_CORE, PEIM, DXE_CORE, DXE_DRIVERS, OTHER_COMPONENTS}

    ///
    /// This parent that this usage instance belongs to.
    ///
    public Token            parentToken;

    ///
    /// The name of the module who contains this PCD.
    ///
    public String           moduleName;

    ///
    /// The GUID of the module who contains this PCD. 
    /// 
    public UUID             moduleGUID;

    ///
    /// The name of the package whose module contains this PCD.
    ///
    public String           packageName;

    ///
    /// The GUID of the package whose module contains this PCD.
    /// 
    public UUID             packageGUID;

    ///
    /// The PCD type defined for module 
    /// 
    public Token.PCD_TYPE   modulePcdType;

    ///
    /// The arch string of module contains this PCD
    ///
    public String           arch;

    ///
    /// The version of module contains this PCD
    /// 
    public String           version;

    ///
    /// The module type for this usage instance.
    ///
    public MODULE_TYPE      moduleType;

    ///
    /// The value of the PCD in this usage instance. 
    /// 
    public String           datum;

    ///
    /// The maxDatumSize could be different for same PCD in different module
    /// But this case is allow for FeatureFlag, FixedAtBuild, PatchableInModule
    /// type.
    /// 
    public int              maxDatumSize;

    ///
    /// Autogen string for header file.
    ///
    public String           hAutogenStr;

    ///
    /// Auotgen string for C code file.
    /// 
    public String           cAutogenStr;

    /**
       Constructure function
       
       @param parentToken         Member variable.
       @param moduleName          Member variable.
       @param moduleGUID          Member variable.
       @param packageName         Member variable.
       @param packageGUID         Member variable.
       @param moduleType          Member variable.
       @param modulePcdType       Member variable.
       @param arch                Member variable.
       @param version             Member variable.
       @param value               Member variable.
       @param maxDatumSize        Member variable.
     */
    public UsageInstance (Token             parentToken,
                          String            moduleName,
                          UUID              moduleGUID,
                          String            packageName,
                          UUID              packageGUID,
                          MODULE_TYPE       moduleType,
                          Token.PCD_TYPE    modulePcdType,
                          String            arch,
                          String            version,
                          String            value,
                          int               maxDatumSize) {
        this.parentToken      = parentToken;
        this.moduleName       = moduleName;
        this.moduleGUID       = moduleGUID;
        this.packageName      = packageName;
        this.packageGUID      = packageGUID;
        this.moduleType       = moduleType;
        this.modulePcdType    = modulePcdType;
        this.arch             = arch;
        this.version          = version;
        this.datum            = value;
        this.maxDatumSize     = maxDatumSize;
    }

    /**
       Get the primary key for usage instance array for every token.
       
       @param moduleName      the name of module
       @param moduleGUID      the GUID name of module
       @param packageName     the name of package who contains this module
       @param packageGUID     the GUID name of package
       @param arch            the archtecture string
       @param version         the version of this module
       
       @return String         primary key
     */
    public static String getPrimaryKey(String moduleName,  
                                       UUID   moduleGUID,  
                                       String packageName,  
                                       UUID   packageGUID,
                                       String arch,
                                       String version) {
        //
        // Because currently transition schema not require write moduleGuid, package Name, Packge GUID in
        // <ModuleSA> section, So currently no expect all paramter must be valid.
        return(moduleName                                                              + "_" +
               ((moduleGUID  != null) ? moduleGUID.toString() : "NullModuleGuid")      + "_" +
               ((packageName != null) ? packageName : "NullPackageName")               + "_" +
               ((packageGUID != null) ? packageGUID.toString() : "NullPackageGuid")    + "_" +
               ((arch        != null) ? arch : "NullArch")                             + "_" +
               ((version     != null) ? version : "NullVersion"));
    }

    /**
       Get primary key string for this usage instance
       
       @return String primary key string
    **/
    public String getPrimaryKey() {
        return UsageInstance.getPrimaryKey(moduleName, moduleGUID, packageName, packageGUID, arch, version);
    }

    /**
       Judget whether current module is PEI driver
       
       @return boolean
     */
    public boolean isPeiPhaseComponent() {
        if ((moduleType == MODULE_TYPE.PEI_CORE) ||
            (moduleType == MODULE_TYPE.PEIM)) {
            return true;
        }
        return false;
    }

    /**
       Generate autogen string for header file and C code file.
       
       @throws EntityException Fail to generate.
       
       @param isBuildUsedLibrary  whether the autogen is for library.
     */
    public void generateAutoGen(boolean isBuildUsedLibrary) 
    throws EntityException {
        String guidStringArray[] = null;
        String guidString        = null;

        hAutogenStr = "";
        cAutogenStr = "";

        if (this.modulePcdType == Token.PCD_TYPE.DYNAMIC_EX) {
            hAutogenStr += String.format("#define _PCD_LOCAL_TOKEN_%s   0x%016x\r\n", 
                                         parentToken.cName, parentToken.tokenNumber);
            hAutogenStr += String.format("#define _PCD_TOKEN_%s   0x%016x\r\n", 
                                         parentToken.cName, parentToken.dynamicExTokenNumber);
        } else {
            hAutogenStr += String.format("#define _PCD_TOKEN_%s   0x%016x\r\n", 
                                         parentToken.cName, parentToken.tokenNumber);
        }

        switch (modulePcdType) {
        case FEATURE_FLAG:
            if (isBuildUsedLibrary) {
                hAutogenStr += String.format("extern const BOOLEAN _gPcd_FixedAtBuild_%s;\r\n", 
                                             parentToken.cName);
                hAutogenStr += String.format("#define _PCD_MODE_%s_%s  _gPcd_FixedAtBuild_%s\r\n",
                                             parentToken.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);
            } else {
                hAutogenStr += String.format("#define _PCD_VALUE_%s   %s\r\n", 
                                             parentToken.cName, 
                                             datum.toString());
                hAutogenStr += String.format("extern const BOOLEAN _gPcd_FixedAtBuild_%s;\r\n", 
                                             parentToken.cName);
                cAutogenStr += String.format("GLOBAL_REMOVE_IF_UNREFERENCED const BOOLEAN _gPcd_FixedAtBuild_%s = _PCD_VALUE_%s;\r\n",
                                             parentToken.cName,
                                             parentToken.cName);
                hAutogenStr += String.format("#define _PCD_MODE_%s_%s  _PCD_VALUE_%s\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);
            }
            break;
        case FIXED_AT_BUILD:
            if (isBuildUsedLibrary) {
                hAutogenStr += String.format("extern const %s _gPcd_FixedAtBuild_%s;\r\n",
                                             Token.getAutogendatumTypeString(parentToken.datumType),
                                             parentToken.cName);
                hAutogenStr += String.format("#define _PCD_MODE_%s_%s  _gPcd_FixedAtBuild_%s\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);
            } else {
                hAutogenStr += String.format("#define _PCD_VALUE_%s   %s\r\n", 
                                             parentToken.cName, 
                                             datum.toString());
                hAutogenStr += String.format("extern const %s _gPcd_FixedAtBuild_%s;\r\n",
                                             Token.getAutogendatumTypeString(parentToken.datumType),
                                             parentToken.cName);
                cAutogenStr += String.format("GLOBAL_REMOVE_IF_UNREFERENCED const %s _gPcd_FixedAtBuild_%s = _PCD_VALUE_%s;\r\n",
                                             Token.getAutogendatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);
                hAutogenStr += String.format("#define _PCD_MODE_%s_%s  _PCD_VALUE_%s\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);
            }
            break;
        case PATCHABLE_IN_MODULE:
            if (isBuildUsedLibrary) {
                hAutogenStr += String.format("extern %s _gPcd_BinaryPatch_%s;\r\n",
                                             Token.getAutogendatumTypeString(parentToken.datumType),
                                             parentToken.cName);
                hAutogenStr += String.format("#define _PCD_MODE_%s_%s  _gPcd_BinaryPatch_%s\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);
            } else {
                hAutogenStr += String.format("#define _PCD_VALUE_%s   %s\r\n", 
                                             parentToken.cName, 
                                             datum.toString());
                hAutogenStr += String.format("extern %s _gPcd_BinaryPatch_%s;\r\n",
                                             Token.getAutogendatumTypeString(parentToken.datumType),
                                             parentToken.cName);
                cAutogenStr += String.format("GLOBAL_REMOVE_IF_UNREFERENCED %s _gPcd_BinaryPatch_%s = _PCD_VALUE_%s;\r\n",
                                             Token.getAutogendatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);
                hAutogenStr += String.format("#define _PCD_MODE_%s_%s  _gPcd_BinaryPatch_%s\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);
            }

            break;
        case DYNAMIC:
            hAutogenStr += String.format("#define _PCD_MODE_%s_%s  LibPcdGet%s(_PCD_TOKEN_%s)\r\n",
                                         Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                         parentToken.cName,
                                         Token.getAutogenLibrarydatumTypeString(parentToken.datumType),
                                         parentToken.cName);
            break;
        case DYNAMIC_EX:
            guidStringArray = parentToken.tokenSpaceName.toString().split("-");
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
                                            
            hAutogenStr += String.format("extern EFI_GUID _gPcd_DynamicEx_TokenSpaceGuid_%s;\r\n",
                                         parentToken.cName);
            hAutogenStr += String.format("#define _PCD_MODE_%s_%s LibPcdGet%s(_PCD_LOCAL_TOKEN_%s)\r\n",
                                         Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                         parentToken.cName,
                                         Token.getAutogenLibrarydatumTypeString(parentToken.datumType),
                                         parentToken.cName,
                                         parentToken.cName);

            if (!isBuildUsedLibrary) {
                cAutogenStr += String.format("GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID _gPcd_DynamicEx_TokenSpaceGuid_%s = %s;\r\n",
                                             parentToken.cName,
                                             guidString);
            }
            break;
        }
    }

    /**
      Get the autogen string for header file.
      
      @return The string of header file.
    **/
    public String getHAutogenStr() {
        return hAutogenStr;
    }

    /**
      Get the autogen string for C code file.
      
      @return The string of C Code file.
    **/
    public String getCAutogenStr() {
        return cAutogenStr;
    }
}

