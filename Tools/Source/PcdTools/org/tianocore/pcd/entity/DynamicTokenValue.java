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

/** This class is to descript a value type of dynamic PCD.
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
    /// BUGBUG: Not use upcase charater is to facility for reading. It may be changed
    ///         in coding review.
    public enum        VALUE_TYPE {HII_TYPE, VPD_TYPE, DEFAULT_TYPE}

    public VALUE_TYPE  type;

    ///
    /// ---------------------------------------------------------------------
    /// Following member is for HII case.
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
    /// Following member is for VPD case.
    /// BUGBUG: Consider 64 bit integer by using java.math.BigInteger.
    /// 
    public String      vpdOffset;

    ///
    /// Following member is for default case.
    /// 
    public String      value;

    public DynamicTokenValue() {
        this.type               = VALUE_TYPE.DEFAULT_TYPE;
        this.variableName       = null;
        this.variableGuid       = null;
        this.variableOffset     = null;
        this.hiiDefaultValue    = null;

        this.vpdOffset          = null;

        this.value              = null;
    }

    /**
       Set the HII case data.
       
       @param variableName
       @param variableGuid
       @param variableOffset
       @param hiiDefaultValue
     */
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
       Get the string like L"xxx" for a variable Name.
       
       BUGBUG: In fact, it is not correctly, variable name should be
               treated as unicode UINT16 array.
       
       @return String
     */
    public String getStringOfVariableName() 
        throws EntityException {
        String str;
        int    index, num;
        char   ch;

        str = "";
        for (index = 0; index < variableName.size(); index ++) {
            num = Integer.decode(variableName.get(index).toString());
            if ((num > 127 ) || (num < 0)) {
                throw new EntityException(String.format("variable name contains >0x80 character, now is not support!"));
            }
            str += (char)num;
        }

        return str;
    }

    /**
       Set Vpd case data.
       
       @param vpdOffset
     */
    public void setVpdData(String vpdOffset) {
        this.type = VALUE_TYPE.VPD_TYPE;

        this.vpdOffset = vpdOffset;
    }

    /**
       Set default case data.
       
       @param value
     */
    public void setValue(String value) {
        this.type = VALUE_TYPE.DEFAULT_TYPE;

        this.value = value;
    }
}





