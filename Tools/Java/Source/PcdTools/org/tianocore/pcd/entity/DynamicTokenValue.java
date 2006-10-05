/** @file
  DynamicTokenValue class.

  This module contains the value type of a dynamic token.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.pcd.entity;

import java.util.List;
import java.util.UUID;

import org.tianocore.pcd.exception.EntityException;

/**
   This class is to descript a value type of dynamic PCD.
   For a dynamic or dynamicEx type PCD data, the value type can be:
       1) Hii type: the value of dynamic or dynamicEx is stored into a variable.
       2) Vpd type: the value of dynamic or dynamicEx is stored into somewhere set
                    by OEM.
       3) Default type: the value of dynamic or dynamicEx is stored into PCD dynamic
                        database.
**/
public class DynamicTokenValue {
    ///
    /// Enumeration macro defintion for value type.
    ///
    public static enum        VALUE_TYPE {HII_TYPE, VPD_TYPE, DEFAULT_TYPE}

    ///
    /// The value type maybe:
    /// HII_TYPE: the value stored into variable area.
    /// VPD_TYPE: the value stored into OEM specific area.
    /// DEFAULT_TYPE: the value stored into PCD runtime database.
    ///
    public VALUE_TYPE  type;

    ///
    /// ---------------------------------------------------------------------
    /// Following member is for HII case. The value of HII case will be put
    /// into variable area in flash.
    /// ---------------------------------------------------------------------
    ///

    ///
    /// variableName is valid only when this token support Hii functionality. variableName
    /// indicates the value of token is associated with what variable.
    /// variableName is defined in FPD.
    public List        variableName;

    ///
    /// variableGuid is the GUID this token associated with.
    ///
    public UUID        variableGuid;

    ///
    /// Variable offset indicate the associated variable's offset in NV storage.
    ///
    public String      variableOffset;

    ///
    /// The default value for HII case.
    ///
    public String      hiiDefaultValue;

    ///
    /// ---------------------------------------------------------------------
    /// Following member is for VPD case. The value of VPD case will be put into
    /// some flash position pointed by OEM.
    /// ---------------------------------------------------------------------
    ///

    public String      vpdOffset;

    /// ---------------------------------------------------------------------
    /// Following member is for default case. The value of default type will
    /// be put into PCD runtime database.
    /// ---------------------------------------------------------------------

    ///
    /// The default value of this PCD in default case.
    ///
    public String      value;

    /**
       Constructor function for DynamicTokenValue class.

    **/
    public DynamicTokenValue() {
        type               = VALUE_TYPE.DEFAULT_TYPE;
        variableName       = null;
        variableGuid       = null;
        variableOffset     = null;
        hiiDefaultValue    = null;
        vpdOffset          = null;
        value              = null;
    }

    /**
       Set the HII case data.

       @param variableName      The variable name
       @param variableGuid      The variable guid
       @param variableOffset    The offset of value in this variable
       @param hiiDefaultValue   Default value for this PCD
    **/
    public void setHiiData(List        variableName,
                           UUID        variableGuid,
                           String      variableOffset,
                           String      hiiDefaultValue) {
        this.type = VALUE_TYPE.HII_TYPE;

        this.variableName    = variableName;
        this.variableGuid    = variableGuid;
        this.variableOffset  = variableOffset;
        this.hiiDefaultValue = hiiDefaultValue;
    }

    /**
       Get the variable Name.

       @return String
    **/
    public List getStringOfVariableName() {
        return variableName;
    }

    /**
       Set Vpd case data.

       @param vpdOffset the value offset the start address of OEM specific.
    **/
    public void setVpdData(String vpdOffset) {
        this.type = VALUE_TYPE.VPD_TYPE;

        this.vpdOffset = vpdOffset;
    }

    /**
       Set default case data.

       @param value
    **/
    public void setValue(String value) {
        this.type = VALUE_TYPE.DEFAULT_TYPE;

        this.value = value;
    }
}





