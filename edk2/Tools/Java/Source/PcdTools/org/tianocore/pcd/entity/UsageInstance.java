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
package org.tianocore.pcd.entity;

import org.tianocore.pcd.entity.CommonDefinition;
import org.tianocore.pcd.entity.UsageIdentification;

/**
  This class indicate an usage instance for a PCD token. This instance maybe a module
  or platform setting. When a module produce or cosume a PCD token, then this module
  is an usage instance for this PCD token.
**/
public class UsageInstance {
    ///
    /// This parent that this usage instance belongs to.
    ///
    public Token                parentToken;

    ///
    /// ModuleIdentification for Usage Instance
    ///
    public UsageIdentification  usageId;

    ///
    /// Arch also is a key for a UsageInstance
    ///
    public String               arch;

    ///
    /// The PCD type defined for module
    ///
    public Token.PCD_TYPE       modulePcdType;

    ///
    /// The value of the PCD in this usage instance.
    ///
    public String               datum;

    ///
    /// The maxDatumSize could be different for same PCD in different module
    /// But this case is allow for FeatureFlag, FixedAtBuild, PatchableInModule
    /// type.
    ///
    public int                  maxDatumSize;

    ///
    /// Autogen string for header file.
    ///
    public String               hAutogenStr;

    ///
    /// Auotgen string for C code file.
    ///
    public String               cAutogenStr;

    /**
       Constructure function for UsageInstance

       @param parentToken         The token instance for this usgaInstance
       @param usageId             The identification for usage instance
       @param modulePcdType       The PCD type for this usage instance
       @param value               The value of this PCD in this usage instance
       @param maxDatumSize        The max datum size of this PCD in this usage
                                  instance.
    **/
    public UsageInstance(Token                 parentToken,
                         UsageIdentification   usageId,
                         Token.PCD_TYPE        modulePcdType,
                         String                value,
                         int                   maxDatumSize) {
        this.parentToken      = parentToken;
        this.usageId          = usageId;
        this.modulePcdType    = modulePcdType;
        this.datum            = value;
        this.maxDatumSize     = maxDatumSize;
    }

    /**
       Get the primary key for usage instance array for every token.

       @param   usageId       The identification of UsageInstance

       @retval  String        The primary key for this usage instance
    **/
    public static String getPrimaryKey(UsageIdentification usageId) {
        return usageId.toString();
    }

    /**
       Get primary key string for this usage instance

       @return String primary key string
    **/
    public String getPrimaryKey() {
        return UsageInstance.getPrimaryKey(usageId);
    }

    /**
       Judget whether current module is PEI driver

       @return boolean whether current module is PEI driver
    **/
    public boolean isPeiPhaseComponent() {
        int moduleType = CommonDefinition.getModuleType(usageId.moduleType);

        if ((moduleType == CommonDefinition.ModuleTypePeiCore) ||
            (moduleType == CommonDefinition.ModuleTypePeim)) {
            return true;
        }
        return false;
    }

    /**
       Judge whether current module is DXE driver.

       @return boolean whether current module is DXE driver
    **/
    public boolean isDxePhaseComponent() {
        int moduleType = CommonDefinition.getModuleType(usageId.moduleType);

        if ((moduleType == CommonDefinition.ModuleTypeDxeDriver)        ||
            (moduleType == CommonDefinition.ModuleTypeDxeRuntimeDriver) ||
            (moduleType == CommonDefinition.ModuleTypeDxeSalDriver)     ||
            (moduleType == CommonDefinition.ModuleTypeDxeSmmDriver)     ||
            (moduleType == CommonDefinition.ModuleTypeUefiDriver)       ||
            (moduleType == CommonDefinition.ModuleTypeUefiApplication)
            ) {
            return true;
        }
        return false;
    }

    /**
       Generate autogen string for header file and C code file.

       @param isBuildUsedLibrary  whether the autogen is for library.
    **/
    public void generateAutoGen(boolean isBuildUsedLibrary) {
        String  guidStringCName     = null;
        boolean isByteArray         = false;
        String  printDatum          = null;
        String  tokenNumberString   = null;

        hAutogenStr = "";
        cAutogenStr = "";

        if (this.modulePcdType == Token.PCD_TYPE.DYNAMIC_EX) {
            //
            // For DYNAMIC_EX type PCD, use original token number in SPD or FPD to generate autogen
            //
            tokenNumberString =  Long.toString(parentToken.dynamicExTokenNumber, 16);
        } else {
            //
            // For Others type PCD, use autogenerated token number to generate autogen
            //
            tokenNumberString = Long.toString(parentToken.tokenNumber, 16);
        }

        hAutogenStr += String.format("#define _PCD_TOKEN_%s  0x%s\r\n", parentToken.cName, tokenNumberString);

        //
        // Judge the value of this PCD is byte array type
        //
        if (!isBuildUsedLibrary && !parentToken.isDynamicPCD) {
            if (datum.trim().charAt(0) == '{') {
                isByteArray = true;
            }
        }

        //
        // "ULL" should be added to value's tail for UINT64 value
        //
        if (parentToken.datumType == Token.DATUM_TYPE.UINT64) {
            printDatum = this.datum + "ULL";
        } else {
            printDatum = this.datum;
        }

        switch (modulePcdType) {
        case FEATURE_FLAG:
            //
            // Example autogen string for following generation:
            // "extern const BOOLEAN _gPcd_FixedAtBuild_PcdSampleToken";
            // 
            hAutogenStr += String.format("extern const BOOLEAN _gPcd_FixedAtBuild_%s;\r\n",
                                         parentToken.cName);
            //
            // Example autogen string for following generation:
            // "#define _PCD_GET_MODE_8_PcdSampleToken _gPcd_FixedAtBuild_PcdSampleToken";
            // 
            hAutogenStr += String.format("#define _PCD_GET_MODE_%s_%s  _gPcd_FixedAtBuild_%s\r\n",
                                         Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                         parentToken.cName,
                                         parentToken.cName);
            //
            // Example autogen string for following generation:
            // "//#define _PCD_SET_MODE_8_PcdSampleToken ASSERT(FALSE) If is not allowed to set value...";
            // 
            hAutogenStr += String.format("//#define _PCD_SET_MODE_%s_%s ASSERT(FALSE) If is not allowed to set value for a FEATURE_FLAG PCD\r\n",
                                         Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                         parentToken.cName);

            if (!isBuildUsedLibrary) {
                //
                // Example autogen string for following generation:
                // "#define _PCD_VALUE_PcdSampleToken 0x1000"
                // 
                hAutogenStr += String.format("#define _PCD_VALUE_%s   %s\r\n",
                                             parentToken.cName,
                                             printDatum);
                //
                // Example autogen string for following generation:
                // "GLOBAL_REMOVE_IF_UNREFERENCED const BOOLEAN _gPcd_FixedAtBuild_PcdSampleToken = _PCD_VALUE_PcdSampleToken;"
                // 
                cAutogenStr += String.format("GLOBAL_REMOVE_IF_UNREFERENCED const BOOLEAN _gPcd_FixedAtBuild_%s = _PCD_VALUE_%s;\r\n",
                                             parentToken.cName,
                                             parentToken.cName);
            }
            break;
        case FIXED_AT_BUILD:
            if (isByteArray) {
                //
                // Example autogen string for following generation:
                // "extern const BOOLEAN _gPcd_FixedAtBuild_PcdSampleToken";
                // 
                hAutogenStr += String.format("extern const UINT8 _gPcd_FixedAtBuild_%s[];\r\n",
                                             parentToken.cName);
                //
                // Example autogen string for following generation:
                // "#define _PCD_GET_MODE_8_PcdSampleToken (VOID*)_gPcd_FixedAtBuild_PcdSampleToken";
                // 
                hAutogenStr += String.format("#define _PCD_GET_MODE_%s_%s  (VOID*)_gPcd_FixedAtBuild_%s\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);
            } else {
                //
                // Example autogen string for following generation:
                // "extern const UINT8 _gPcd_FixedAtBuild_PcdSampleToken";
                // 
                hAutogenStr += String.format("extern const %s _gPcd_FixedAtBuild_%s;\r\n",
                                             Token.getAutogendatumTypeString(parentToken.datumType),
                                             parentToken.cName);
                //
                // Example autogen string for following generation:
                // "#define _PCD_GET_MODE_8_PcdSampleToken _gPcd_FixedAtBuild_PcdSampleToken";
                // 
                hAutogenStr += String.format("#define _PCD_GET_MODE_%s_%s  _gPcd_FixedAtBuild_%s\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);
            }

            //
            // Example autogen string for following generation:
            // "//#define _PCD_SET_MODE_8_PcdSampleToken ASSERT(FALSE) If is not allowed to set value...";
            // 
            hAutogenStr += String.format("//#define _PCD_SET_MODE_%s_%s ASSERT(FALSE) // It is not allowed to set value for a FIXED_AT_BUILD PCD\r\n",
                                         Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                         parentToken.cName);
            if (!isBuildUsedLibrary) {
                if (parentToken.datumType == Token.DATUM_TYPE.POINTER) {
                    if (isByteArray) {
                        //
                        // Example autogen string for following generation:
                        // "#define _PCD_VALUE_PcdSampleToken (VOID*)_gPcd_FixedAtBuild_PcdSampleToken"
                        // 
                        hAutogenStr += String.format("#define _PCD_VALUE_%s   (VOID*)_gPcd_FixedAtBuild_%s\r\n",
                                                     parentToken.cName,
                                                     parentToken.cName);
                        //
                        // Example autogen string for following generation:
                        // "GLOBAL_REMOVE_IF_UNREFERENCED const UINT8 _gPcd_FixedAtBuild_PcdSampleToken[] = 'dfdf';"
                        // 
                        cAutogenStr += String.format("GLOBAL_REMOVE_IF_UNREFERENCED const UINT8 _gPcd_FixedAtBuild_%s[] = %s;\r\n",
                                                     parentToken.cName,
                                                     printDatum);
                    } else {
                        //
                        // Example autogen string for following generation:
                        // "#define _PCD_VALUE_PcdSampleToken 0x222"
                        // 
                        hAutogenStr += String.format("#define _PCD_VALUE_%s   %s\r\n",
                                                     parentToken.cName,
                                                     printDatum);
                        //
                        // Example autogen string for following generation:
                        // "GLOBAL_REMOVE_IF_UNREFERENCED const UINT8 _gPcd_FixedAtBuild_PcdSampleToken[] = _PCD_VALUE_PcdSampleToken;"
                        // 
                        cAutogenStr += String.format("GLOBAL_REMOVE_IF_UNREFERENCED const %s _gPcd_FixedAtBuild_%s = _PCD_VALUE_%s;\r\n",
                                                     Token.getAutogendatumTypeString(parentToken.datumType),
                                                     parentToken.cName,
                                                     parentToken.cName);
                    }
                } else {
                    //
                    // Example autogen string for following generation:
                    // "#define _PCD_VALUE_PcdSampleToken 0x222"
                    // 
                    hAutogenStr += String.format("#define _PCD_VALUE_%s   %s\r\n",
                                                 parentToken.cName,
                                                 printDatum);
                    //
                    // Example autogen string for following generation:
                    // "GLOBAL_REMOVE_IF_UNREFERENCED const UINT8 _gPcd_FixedAtBuild_PcdSampleToken[] = _PCD_VALUE_PcdSampleToken;"
                    // 
                    cAutogenStr += String.format("GLOBAL_REMOVE_IF_UNREFERENCED const %s _gPcd_FixedAtBuild_%s = _PCD_VALUE_%s;\r\n",
                                                 Token.getAutogendatumTypeString(parentToken.datumType),
                                                 parentToken.cName,
                                                 parentToken.cName);
                }
            }
            break;
        case PATCHABLE_IN_MODULE:
            if (parentToken.datumType == Token.DATUM_TYPE.POINTER) {
                //
                // Example autogen string for following generation:
                // "extern UINT8 _gPcd_BinaryPatch_PcdSampleToken[];"
                // 
                hAutogenStr += String.format("extern UINT8 _gPcd_BinaryPatch_%s[];\r\n",
                                             parentToken.cName);
                //
                // Example autogen string for following generation:
                // "#define _PCD_GET_MODE_8_PcdSampleToken  (VOID*)_gPcd_BinaryPatch_PcdSampleToken"
                // 
                hAutogenStr += String.format("#define _PCD_GET_MODE_%s_%s  (VOID*)_gPcd_BinaryPatch_%s\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);
                //
                // Example autogen string for following generation:
                // "#define _PCD_SET_MODE_8_PcdSampleToken(SizeOfBuffer, Buffer) CopyMem (_gPcd_BinaryPatch_PcdSampleToken, (Buffer), (SizeOfBuffer))"
                // 
                hAutogenStr += String.format("#define _PCD_PATCHABLE_%s_SIZE %d\r\n",
                                             parentToken.cName,
                                             parentToken.datumSize);
                hAutogenStr += String.format("#define _PCD_SET_MODE_%s_%s(SizeOfBuffer, Buffer) "+
                                             "LibPatchPcdSetPtr (_gPcd_BinaryPatch_%s, (UINTN)_PCD_PATCHABLE_%s_SIZE, "+
                                             "(SizeOfBuffer), (Buffer))\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName,
                                             parentToken.cName);

            } else {
                //
                // Example autogen string for following generation:
                // "extern UINT8 _gPcd_BinaryPatch_PcdSampleToken;"
                // 
                hAutogenStr += String.format("extern %s _gPcd_BinaryPatch_%s;\r\n",
                                             Token.getAutogendatumTypeString(parentToken.datumType),
                                             parentToken.cName);
                //
                // Example autogen string for following generation:
                // "#define _PCD_GET_MODE_8_PcdSampleToken  _gPcd_BinaryPatch_PcdSampleToken"
                // 
                hAutogenStr += String.format("#define _PCD_GET_MODE_%s_%s  _gPcd_BinaryPatch_%s\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);
                //
                // Example autogen string for following generation:
                // "#define _PCD_SET_MODE_8_PcdSampleToken(Value) (_gPcd_BinaryPatch_PcdSampleToken = (Value))"
                // 
                hAutogenStr += String.format("#define _PCD_SET_MODE_%s_%s(Value) (_gPcd_BinaryPatch_%s = (Value))\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             parentToken.cName);
            }

            if (!isBuildUsedLibrary) {
                if (parentToken.datumType == Token.DATUM_TYPE.POINTER) {
                    printDatum = parentToken.getByteArrayForPointerDatum(printDatum);
                }
                //
                // Example autogen string for following generation:
                // "#define _PCD_VALUE_PcdSampleToken   0x111"
                // 
                hAutogenStr += String.format("#define _PCD_VALUE_%s   %s\r\n",
                                             parentToken.cName,
                                             printDatum);
                if (parentToken.datumType == Token.DATUM_TYPE.POINTER) {
                    //
                    // Example autogen string for following generation:
                    // "GLOBAL_REMOVE_IF_UNREFERENCED UINT8 _gPcd_BinaryPatch_PcdSampleToken[] = _PCD_VALUE_PcdSampleToken;"
                    // 
                    cAutogenStr += String.format("GLOBAL_REMOVE_IF_UNREFERENCED UINT8 _gPcd_BinaryPatch_%s[%d] = _PCD_VALUE_%s;\r\n",
                                                 parentToken.cName,
                                                 parentToken.datumSize,
                                                 parentToken.cName);
                } else {
                    //
                    // Example autogen string for following generation:
                    // "GLOBAL_REMOVE_IF_UNREFERENCED UINT8 _gPcd_BinaryPatch_PcdSampleToken[] = _PCD_VALUE_PcdSampleToken;"
                    // 
                    cAutogenStr += String.format("GLOBAL_REMOVE_IF_UNREFERENCED %s _gPcd_BinaryPatch_%s = _PCD_VALUE_%s;\r\n",
                                                 Token.getAutogendatumTypeString(parentToken.datumType),
                                                 parentToken.cName,
                                                 parentToken.cName);
                }
            }

            break;
        case DYNAMIC:
            //
            // Example autogen string for following generation:
            // "#define _PCD_GET_MODE_8_PcdSampleToken  LibPcdGet%s(_PCD_TOKEN_PcdSampleToken)"
            // 
            hAutogenStr += String.format("#define _PCD_GET_MODE_%s_%s  LibPcdGet%s(_PCD_TOKEN_%s)\r\n",
                                         Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                         parentToken.cName,
                                         Token.getAutogenLibrarydatumTypeString(parentToken.datumType),
                                         parentToken.cName);
            if (parentToken.datumType == Token.DATUM_TYPE.POINTER) {
                //
                // Example autogen string for following generation:
                // "#define _PCD_SET_MODE_8_PcdSampleToken(SizeOfBuffer, Buffer)  LibPcdSet%s(_PCD_TOKEN_PcdSampleToken, (SizeOfBuffer), (Buffer))"
                // 
                hAutogenStr += String.format("#define _PCD_SET_MODE_%s_%s(SizeOfBuffer, Buffer)  LibPcdSet%s(_PCD_TOKEN_%s, (SizeOfBuffer), (Buffer))\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             Token.getAutogenLibrarydatumTypeString(parentToken.datumType),
                                             parentToken.cName);
            } else {
                //
                // Example autogen string for following generation:
                // "#define _PCD_SET_MODE_8_PcdSampleToken(Value)  LibPcdSet%s(_PCD_TOKEN_PcdSampleToken, (Value))"
                // 
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

            //
            // Example autogen string for following generation:
            // "#define _PCD_GET_MODE_8_PcdSampleToken LibPcdGetEx%s(&_gPcd_TokenSpaceGuid_00_00_00, _PCD_TOKEN_PcdSampleToken)"
            // 
            hAutogenStr += String.format("#define _PCD_GET_MODE_%s_%s LibPcdGetEx%s(&%s, _PCD_TOKEN_%s)\r\n",
                                         Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                         parentToken.cName,
                                         Token.getAutogenLibrarydatumTypeString(parentToken.datumType),
                                         guidStringCName,
                                         parentToken.cName);

            if (parentToken.datumType == Token.DATUM_TYPE.POINTER) {
                //
                // Example autogen string for following generation:
                // "#define _PCD_SET_MODE_8_PcdSampleToken(SizeOfBuffer, Buffer) LibPcdSetEx%s(&_gPcd_TokenSpaceGuid_00_00_00, _PCD_TOKEN_PcdSampleToken, (SizeOfBuffer), (Buffer))"
                // 
                hAutogenStr += String.format("#define _PCD_SET_MODE_%s_%s(SizeOfBuffer, Buffer) LibPcdSetEx%s(&%s, _PCD_TOKEN_%s, (SizeOfBuffer), (Buffer))\r\n",
                                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                                             parentToken.cName,
                                             Token.getAutogenLibrarydatumTypeString(parentToken.datumType),
                                             guidStringCName,
                                             parentToken.cName);
            } else {
                //
                // Example autogen string for following generation:
                // "#define _PCD_SET_MODE_8_PcdSampleToken(Value) LibPcdSetEx%s(&_gPcd_TokenSpaceGuid_00_00_00, _PCD_TOKEN_PcdSampleToken, (Value))"
                //
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

