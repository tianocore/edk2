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
import org.tianocore.ModuleTypeDef;
import org.tianocore.build.pcd.exception.EntityException;

/**
  This class indicate an usage instance for a PCD token. This instance maybe a module
  or platform setting. When a module produce or cosume a PCD token, then this module
  is an usage instance for this PCD token.
**/
public class UsageInstance {
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
    public ModuleTypeDef.Enum    moduleType;

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
                          ModuleTypeDef.Enum moduleType,
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
        if ((moduleType == ModuleTypeDef.PEI_CORE) ||
            (moduleType == ModuleTypeDef.PEIM)) {
            return true;
        }
        return false;
    }
  
  public boolean isDxePhaseComponent() {
      //
      // BugBug: May need confirmation on which type of module can
      //         make use of Dynamic(EX) PCD entry.
      //
      if ((moduleType == ModuleTypeDef.DXE_DRIVER) ||
          (moduleType == ModuleTypeDef.DXE_RUNTIME_DRIVER) ||
          (moduleType == ModuleTypeDef.DXE_SAL_DRIVER) ||
          (moduleType == ModuleTypeDef.DXE_SMM_DRIVER) ||
          (moduleType == ModuleTypeDef.UEFI_DRIVER) ||
          (moduleType == ModuleTypeDef.UEFI_APPLICATION)
          ) {
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
        String  guidStringCName  = null;
        boolean isByteArray      = false;
        String  printDatum       = null;

        hAutogenStr = "";
        cAutogenStr = "";

        if (this.modulePcdType == Token.PCD_TYPE.DYNAMIC_EX) {
            hAutogenStr += String.format("#define _PCD_TOKEN_%s   0x%016x\r\n", 
                                         parentToken.cName, parentToken.dynamicExTokenNumber);
        } else {
            hAutogenStr += String.format("#define _PCD_TOKEN_%s   0x%016x\r\n", 
                                         parentToken.cName, parentToken.tokenNumber);
        }

        if (!isBuildUsedLibrary && !parentToken.isDynamicPCD) {
            if (datum.trim().charAt(0) == '{') {
                isByteArray = true;
            }
        }

        if (parentToken.datumType == Token.DATUM_TYPE.UINT64) {
            printDatum = this.datum + "ULL";
        } else {
            printDatum = this.datum;
        }

        switch (modulePcdType) {
        case FEATURE_FLAG:
            hAutogenStr += String.format("extern const BOOLEAN _gPcd_FixedAtBuild_%s;\r\n", 
                                         parentToken.cName);
            hAutogenStr += String.format("#define _PCD_GET_MODE_%s_%s  _gPcd_FixedAtBuild_%s\r\n",
                                         parentToken.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                         parentToken.cName,
                                         parentToken.cName);
            hAutogenStr += String.format("//#define _PCD_SET_MODE_%s_%s ASSERT(FALSE) If is not allowed to set value for a FEATURE_FLAG PCD\r\n",
                                         parentToken.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                         parentToken.cName);

            if (!isBuildUsedLibrary) {
                hAutogenStr += String.format("#define _PCD_VALUE_%s   %s\r\n", 
                                             parentToken.cName, 
                                             printDatum);
                cAutogenStr += String.format("GLOBAL_REMOVE_IF_UNREFERENCED const BOOLEAN _gPcd_FixedAtBuild_%s = _PCD_VALUE_%s;\r\n",
                                             parentToken.cName,
                                             parentToken.cName);
            }
            break;
        case FIXED_AT_BUILD:
            if (isByteArray) {
                hAutogenStr += String.format("extern const UINT8 _gPcd_FixedAtBuild_%s[];\r\n",
                                             parentToken.cName);
                hAutogenStr += String.format("#define _PCD_GET_MODE_%s_%s  (VOID*)_gPcd_FixedAtBuild_%s\r\n", 
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);
            } else {
                hAutogenStr += String.format("extern const %s _gPcd_FixedAtBuild_%s;\r\n",
                                             Token.getAutogendatumTypeString(parentToken.datumType),
                                             parentToken.cName);
                hAutogenStr += String.format("#define _PCD_GET_MODE_%s_%s  _gPcd_FixedAtBuild_%s\r\n", 
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);
            }

            hAutogenStr += String.format("//#define _PCD_SET_MODE_%s_%s ASSERT(FALSE) // It is not allowed to set value for a FIXED_AT_BUILD PCD\r\n",
                                         parentToken.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                         parentToken.cName);
            if (!isBuildUsedLibrary) {
                hAutogenStr += String.format("#define _PCD_VALUE_%s   %s\r\n", 
                                             parentToken.cName, 
                                             printDatum);
                if (isByteArray) {
                    cAutogenStr += String.format("GLOBAL_REMOVE_IF_UNREFERENCED const UINT8 _gPcd_FixedAtBuild_%s[] = _PCD_VALUE_%s;\r\n",
                                                 parentToken.cName,
                                                 parentToken.cName);
                } else {
                    cAutogenStr += String.format("GLOBAL_REMOVE_IF_UNREFERENCED const %s _gPcd_FixedAtBuild_%s = _PCD_VALUE_%s;\r\n",
                                                 Token.getAutogendatumTypeString(parentToken.datumType),
                                                 parentToken.cName,
                                                 parentToken.cName);
                }
            }
            break;
        case PATCHABLE_IN_MODULE:
            if (isByteArray) {
                hAutogenStr += String.format("extern UINT8 _gPcd_BinaryPatch_%s[];\r\n",
                                             parentToken.cName);
                hAutogenStr += String.format("#define _PCD_GET_MODE_%s_%s  (VOID*)_gPcd_BinaryPatch_%s\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);  
            } else {
                hAutogenStr += String.format("extern %s _gPcd_BinaryPatch_%s;\r\n",
                                             Token.getAutogendatumTypeString(parentToken.datumType),
                                             parentToken.cName);
                hAutogenStr += String.format("#define _PCD_GET_MODE_%s_%s  _gPcd_BinaryPatch_%s\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);                
            }

            //
            // Generate _PCD_SET_MODE_xx macro for using set BinaryPatch value via PcdSet macro
            // 
            if (parentToken.datumType == Token.DATUM_TYPE.POINTER) {
                hAutogenStr += String.format("#define _PCD_SET_MODE_%s_%s(SizeOfBuffer, Buffer) CopyMem (_gPcd_BinaryPatch_%s, (Buffer), (SizeOfBuffer))\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);
            } else {
                hAutogenStr += String.format("#define _PCD_SET_MODE_%s_%s(Value) (_gPcd_BinaryPatch_%s = (Value))\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);
            }
            
            if (!isBuildUsedLibrary) {
                hAutogenStr += String.format("#define _PCD_VALUE_%s   %s\r\n", 
                                             parentToken.cName, 
                                             printDatum);
                if (isByteArray) {
                    cAutogenStr += String.format("GLOBAL_REMOVE_IF_UNREFERENCED UINT8 _gPcd_BinaryPatch_%s[] = _PCD_VALUE_%s;\r\n",
                                                 parentToken.cName,
                                                 parentToken.cName);
                } else {
                    cAutogenStr += String.format("GLOBAL_REMOVE_IF_UNREFERENCED %s _gPcd_BinaryPatch_%s = _PCD_VALUE_%s;\r\n",
                                                 Token.getAutogendatumTypeString(parentToken.datumType),
                                                 parentToken.cName,
                                                 parentToken.cName);
                }
            }

            break;
        case DYNAMIC:
            hAutogenStr += String.format("#define _PCD_GET_MODE_%s_%s  LibPcdGet%s(_PCD_TOKEN_%s)\r\n",
                                         Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                         parentToken.cName,
                                         Token.getAutogenLibrarydatumTypeString(parentToken.datumType),
                                         parentToken.cName);
            if (parentToken.datumType == Token.DATUM_TYPE.POINTER) {
                hAutogenStr += String.format("#define _PCD_SET_MODE_%s_%s(SizeOfBuffer, Buffer)  LibPcdSet%s(_PCD_TOKEN_%s, (SizeOfBuffer), (Buffer))\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             Token.getAutogenLibrarydatumTypeString(parentToken.datumType),
                                             parentToken.cName);
            } else {
                hAutogenStr += String.format("#define _PCD_SET_MODE_%s_%s(Value)  LibPcdSet%s(_PCD_TOKEN_%s, (Value))\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             Token.getAutogenLibrarydatumTypeString(parentToken.datumType),
                                             parentToken.cName);
            }
            break;
        case DYNAMIC_EX:
            guidStringCName = "_gPcd_TokenSpaceGuid_" +
                              parentToken.tokenSpaceName.toString().replaceAll("-", "_");

            hAutogenStr += String.format("#define _PCD_GET_MODE_%s_%s LibPcdGetEx%s(&%s, _PCD_TOKEN_%s)\r\n",
                                         Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                         parentToken.cName,
                                         Token.getAutogenLibrarydatumTypeString(parentToken.datumType),
                                         guidStringCName,
                                         parentToken.cName);

            if (parentToken.datumType == Token.DATUM_TYPE.POINTER) {
                hAutogenStr += String.format("#define _PCD_SET_MODE_%s_%s(SizeOfBuffer, Buffer) LibPcdSetEx%s(&%s, _PCD_TOKEN_%s, (SizeOfBuffer), (Buffer))\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             Token.getAutogenLibrarydatumTypeString(parentToken.datumType),
                                             guidStringCName,
                                             parentToken.cName);
            } else {
                hAutogenStr += String.format("#define _PCD_SET_MODE_%s_%s(Value) LibPcdSetEx%s(&%s, _PCD_TOKEN_%s, (Value))\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             Token.getAutogenLibrarydatumTypeString(parentToken.datumType),
                                             guidStringCName,
                                             parentToken.cName);

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

