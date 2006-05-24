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


import org.tianocore.build.pcd.exception.EntityException;
import org.tianocore.build.pcd.action.ActionMessage;

import org.tianocore.build.autogen.CommonDefinition;

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
  /// The usage of this token for platform or module.
  ///
  public Token.PCD_USAGE  usage;
  ///
  /// Whether this usage instance inherit from library
  ///
  public boolean          isInherit;
  ///
  /// The pcd type of this token for module.
  ///
  public Token.PCD_TYPE   modulePcdType;
  ///
  /// The name of the module who contains this PCD.
  ///
  public String           moduleName;
  ///
  /// The name of the package whose module contains this PCD.
  ///
  public String           packageName;
  ///
  /// The component type for this usage instance.
  ///
  public int              componentType;
  ///
  /// The default value defined in MSA has high prior than defined in SPD.
  ///
  public Object           defaultValueInMSA;
  ///
  /// The default value defined in SPD.
  ///
  public Object           defaultValueInSPD;
  ///
  /// Help text in MSA
  ///
  public String           helpTextInMSA;
  ///
  /// Help text in SPD
  ///
  public String           helpTextInSPD;
  ///
  /// Autogen string for header file.
  ///
  public String           hAutogenStr;
  /**
   * Auotgen string for C code file.
   */
  public String           cAutogenStr;

  /**
    Constructure function
    
    @param parentToken         Member variable.
    @param usage               Member variable.
    @param pcdType             Member variable.
    @param componentType       Member variable.
    @param defaultValueInMSA   Member variable.
    @param defaultValueInSPD   Member variable.
    @param helpTextInMSA       Member variable.
    @param helpTextInSPD       Member variable.
    @param moduleName          Member variable.
    @param packageName         Member variable.
    @param isInherit           Member variable.
  **/
  public UsageInstance(
    Token           parentToken,
    Token.PCD_USAGE usage,
    Token.PCD_TYPE  pcdType,
    int             componentType,
    Object          defaultValueInMSA,
    Object          defaultValueInSPD,
    String          helpTextInMSA,
    String          helpTextInSPD,
    String          moduleName,
    String          packageName,
    boolean         isInherit
    )
  {
    this.parentToken       = parentToken;
    this.usage             = usage;
    this.modulePcdType     = pcdType;
    this.componentType     = componentType;
    this.defaultValueInMSA = defaultValueInMSA;
    this.defaultValueInSPD = defaultValueInSPD;
    this.helpTextInMSA     = helpTextInMSA;
    this.helpTextInSPD     = helpTextInSPD;
    this.moduleName        = moduleName;
    this.packageName       = packageName;
    this.isInherit         = isInherit;
  }

  /**
    Generate autogen string for header file and C code file.
    
    @throws EntityException Fail to generate.
  **/
  public void generateAutoGen() throws EntityException {
    Object value        = null;
    int    tokenNumber  = 0;

    hAutogenStr = "";
    cAutogenStr = "";

    value = this.parentToken.datum;

    //
    // If this pcd token's PCD_TYPE is DYNAMIC_EX, use itself token space name 
    // otherwices use assgined token space name from tool automatically.
    //
    if(parentToken.pcdType == Token.PCD_TYPE.DYNAMIC_EX) {
      tokenNumber = parentToken.tokenNumber;
    } else {
      tokenNumber = parentToken.assignedtokenNumber;
    }

    hAutogenStr += String.format("#define _PCD_TOKEN_%s   0x%016x\r\n", 
                                 parentToken.cName, tokenNumber);

    switch(modulePcdType) {
    case FEATURE_FLAG:
      //
      // BUGBUG: The judegement of module PCD type and platform PCD type should not be 
      //         done here, but in wizard tools, But here is just following something 
      //         PcdEmulation driver. 
      //
      if(parentToken.pcdType.ordinal() > Token.PCD_TYPE.FEATURE_FLAG.ordinal()) {
        throw new EntityException(
          String.format(
            "%s:Platform PCD Type %d is not compatible with Module PCD Type %d\r\n",
            parentToken.cName,
            parentToken.pcdType.name(),
            modulePcdType.name()
            )
          );
      }

      if(CommonDefinition.isLibraryComponent(componentType)) {
          hAutogenStr += String.format(
                           "extern const BOOLEAN _gPcd_FixedAtBuild_%s;\r\n", 
                           parentToken.cName
                           );
          hAutogenStr += String.format(
                             "#define _PCD_MODE_%s_%s  _gPcd_FixedAtBuild_%s\r\n",
                             parentToken.GetAutogenDefinedatumTypeString(parentToken.datumType),
                             parentToken.cName,
                             parentToken.cName
                             );
      } else {
          hAutogenStr += String.format(
                           "#define _PCD_VALUE_%s   %s\r\n", 
                           parentToken.cName, 
                           value.toString()
                           );
          hAutogenStr += String.format(
                           "extern const BOOLEAN _gPcd_FixedAtBuild_%s;\r\n", 
                           parentToken.cName
                           );
          cAutogenStr += String.format(
                           "GLOBAL_REMOVE_IF_UNREFERENCED const BOOLEAN _gPcd_FixedAtBuild_%s = _PCD_VALUE_%s;\r\n",
                           parentToken.cName,
                           parentToken.cName
                           );
          hAutogenStr += String.format(
                           "#define _PCD_MODE_%s_%s  _PCD_VALUE_%s\r\n",
                           Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                           parentToken.cName,
                           parentToken.cName
                           );
      }
      break;
    case FIXED_AT_BUILD:
      //
      // BUGBUG: The judegement of module PCD type and platform PCD type should not be 
      //         done here, but in wizard tools, But here is just following something 
      //         PcdEmulation driver. 
      //
      if(parentToken.pcdType.ordinal() > Token.PCD_TYPE.FIXED_AT_BUILD.ordinal()) {
        throw new EntityException(
          String.format(
            "%s:Platform PCD Type %d is not compatible with Module PCD Type %d\r\n",
            parentToken.cName,
            parentToken.pcdType.name(),
            modulePcdType.name()
            )
          );
      }

      if(CommonDefinition.isLibraryComponent(componentType)) {
        hAutogenStr += String.format(
                         "extern const %s _gPcd_FixedAtBuild_%s;\r\n",
                         Token.getAutogendatumTypeString(parentToken.datumType),
                         parentToken.cName
                         );
        hAutogenStr += String.format(
                         "#define _PCD_MODE_%s_%s  _gPcd_FixedAtBuild_%s\r\n",
                         Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                         parentToken.cName,
                         parentToken.cName
                         );
      } else {
        hAutogenStr += String.format(
                         "#define _PCD_VALUE_%s   %s\r\n", 
                         parentToken.cName, 
                         value.toString()
                         );
        hAutogenStr += String.format(
                         "extern const %s _gPcd_FixedAtBuild_%s;\r\n",
                         Token.getAutogendatumTypeString(parentToken.datumType),
                         parentToken.cName
                         );
        cAutogenStr += String.format(
                         "GLOBAL_REMOVE_IF_UNREFERENCED const %s _gPcd_FixedAtBuild_%s = _PCD_VALUE_%s;\r\n",
                         Token.getAutogendatumTypeString(parentToken.datumType),
                         parentToken.cName,
                         parentToken.cName
                         );
        hAutogenStr += String.format(
                         "#define _PCD_MODE_%s_%s  _PCD_VALUE_%s\r\n",
                         Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                         parentToken.cName,
                         parentToken.cName
                         );
      }
      break;
    case PATCHABLE_IN_MODULE:
      //
      // BUGBUG: The judegement of module PCD type and platform PCD type should not be 
      //         done here, but in wizard tools, But here is just following something 
      //         PcdEmulation driver. 
      //
      if(parentToken.pcdType.ordinal() > Token.PCD_TYPE.PATCHABLE_IN_MODULE.ordinal()) {
        throw new EntityException(
          String.format(
            "%s:Platform PCD Type %d is not compatible with Module PCD Type %d\r\n",
            parentToken.cName,
            parentToken.pcdType.name(),
            modulePcdType.name()
            )
          );
      }

      if(CommonDefinition.isLibraryComponent(componentType)) {
        hAutogenStr += String.format(
                         "extern %s _gPcd_BinaryPatch_%s;\r\n",
                         Token.getAutogendatumTypeString(parentToken.datumType),
                         parentToken.cName
                         );
        hAutogenStr += String.format(
                         "#define _PCD_MODE_%s_%s  _gPcd_BinaryPatch_%s\r\n",
                         Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                         parentToken.cName,
                         parentToken.cName
                         );
      } else {
        hAutogenStr += String.format(
                         "#define _PCD_VALUE_%s   %s\r\n", 
                         parentToken.cName, 
                         value
                         );
        hAutogenStr += String.format(
                         "extern %s _gPcd_BinaryPatch_%s;\r\n",
                         Token.getAutogendatumTypeString(parentToken.datumType),
                         parentToken.cName
                         );
        cAutogenStr += String.format(
                         "GLOBAL_REMOVE_IF_UNREFERENCED %s _gPcd_BinaryPatch_%s = _PCD_VALUE_%s;\r\n",
                         Token.getAutogendatumTypeString(parentToken.datumType),
                         parentToken.cName,
                         parentToken.cName
                         );
        hAutogenStr += String.format(
                         "#define _PCD_MODE_%s_%s  _gPcd_BinaryPatch_%s\r\n",
                         Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                         parentToken.cName,
                         parentToken.cName
                         );
      }

      break;
    case DYNAMIC:
      //
      // BUGBUG: The judegement of module PCD type and platform PCD type should not be 
      //         done here, but in wizard tools, But here is just following something 
      //         PcdEmulation driver. 
      //
      if(parentToken.pcdType.ordinal() > Token.PCD_TYPE.DYNAMIC.ordinal()) {
        throw new EntityException(
          String.format(
            "%s:Platform PCD Type %d is not compatible with Module PCD Type %d\r\n",
            parentToken.cName,
            parentToken.pcdType.name(),
            modulePcdType.name()
            )
          );
      }

      switch(parentToken.pcdType) {
        case FEATURE_FLAG:
          if(CommonDefinition.isLibraryComponent(componentType)) {
            hAutogenStr += String.format(
                             "extern const BOOLEAN _gPcd_FixedAtBuild_%s;\r\n", 
                             parentToken.cName
                             );
            hAutogenStr += String.format(
                             "#define _PCD_MODE_%s_%s  _gPcd_FixedAtBuild_%s\r\n",
                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                             parentToken.cName,
                             parentToken.cName
                             );
          } else {
            hAutogenStr += String.format(
                             "#define _PCD_VALUE_%s   %s\r\n", 
                             parentToken.cName, 
                             value
                             );
            hAutogenStr += String.format(
                             "extern const BOOLEAN _gPcd_FixedAtBuild_%s;\r\n", 
                             parentToken.cName
                             );
            cAutogenStr += String.format(
                             "const BOOLEAN _gPcd_FixedAtBuild_%s = _PCD_VALUE_%s;\r\n",
                             parentToken.cName,
                             parentToken.cName
                             );
            hAutogenStr += String.format(
                             "#define _PCD_MODE_%s_%s  _PCD_VALUE_%s\r\n",
                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                             parentToken.cName,
                             parentToken.cName
                             );
          }
          break;
        case FIXED_AT_BUILD:
          if(CommonDefinition.isLibraryComponent(componentType)) {
            hAutogenStr += String.format(
                             "extern const %s _gPcd_FixedAtBuild_%s;\r\n",
                             Token.getAutogendatumTypeString(parentToken.datumType),
                             parentToken.cName
                             );
            hAutogenStr += String.format(
                             "#define _PCD_MODE_%s_%s  _gPcd_FixedAtBuild_%s\r\n",
                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                             parentToken.cName,
                             parentToken.cName
                             );

          } else {
            hAutogenStr += String.format(
                             "#define _PCD_VALUE_%s   %s\r\n", 
                             parentToken.cName, 
                             value
                             );
            hAutogenStr += String.format(
                             "extern const %s _gPcd_FixedAtBuild_%s\r\n",
                             Token.getAutogendatumTypeString(parentToken.datumType),
                             parentToken.cName
                             );
            cAutogenStr += String.format(
                             "const %s _gPcd_FixedAtBuild_%s = _PCD_VALUE_%s;\r\n",
                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                             parentToken.cName,
                             parentToken.cName
                             );
            hAutogenStr += String.format(
                             "#define _PCD_MODE_%s_%s  _PCD_VALUE_%s\r\n",
                             Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                             parentToken.cName,
                             parentToken.cName
                             );
          }
          break;
        case PATCHABLE_IN_MODULE:
          hAutogenStr += String.format(
                           "#define _PCD_VALUE_%s   %s\r\n", 
                           parentToken.cName, 
                           value
                           );
          hAutogenStr += String.format(
                           "extern %s _gPcd_BinaryPatch_%s;\r\n",
                           Token.getAutogendatumTypeString(parentToken.datumType),
                           parentToken.cName,
                           parentToken.cName
                           );
          cAutogenStr += String.format(
                           "%s _gPcd_BinaryPatch_%s = _PCD_VALUE_%s;",
                           Token.getAutogendatumTypeString(parentToken.datumType),
                           parentToken.cName,
                           parentToken.cName
                           );
          hAutogenStr += String.format(
                           "#define _PCD_MODE_%s_%s  _gPcd_BinaryPatch_%s\r\n",
                           Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                           parentToken.cName,
                           parentToken.cName
                           );
          break;
			case DYNAMIC:
					hAutogenStr += "\r\n";
          hAutogenStr += String.format(
                           "#define _PCD_MODE_%s_%s  LibPcdGet%s(_PCD_TOKEN_%s)\r\n",
                           Token.GetAutogenDefinedatumTypeString(parentToken.datumType),
                           parentToken.cName,
                           Token.getAutogenLibrarydatumTypeString(parentToken.datumType),
                           parentToken.cName
                           );
          break;
        default:
         ActionMessage.log(
           this, 
           "The PCD_TYPE setted by platform is unknown"
           );
      }
      break;
    case DYNAMIC_EX:
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

