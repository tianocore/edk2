# # @file
# 
# This file is used to handle the variable attributes and property information
#
#
# Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
   
class VariableAttributes(object):
    EFI_VARIABLE_NON_VOLATILE = 0x00000001
    EFI_VARIABLE_BOOTSERVICE_ACCESS = 0x00000002
    EFI_VARIABLE_RUNTIME_ACCESS = 0x00000004
    VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY = 0x00000001
    VarAttributesMap = {
                     "NV":EFI_VARIABLE_NON_VOLATILE,
                     "BS":EFI_VARIABLE_BOOTSERVICE_ACCESS,
                     "RT":EFI_VARIABLE_RUNTIME_ACCESS,
                     "RO":VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY
                     }
    
    def __init__(self):
        pass
    
    @staticmethod
    def GetVarAttributes(var_attr_str):
        VarAttr = 0x00000000
        VarProp = 0x00000000
        
        attr_list = var_attr_str.split(",")
        for attr in attr_list:
            attr = attr.strip()
            if attr == 'RO':
                VarProp = VariableAttributes.VAR_CHECK_VARIABLE_PROPERTY_READ_ONLY
            else:
                VarAttr = VarAttr | VariableAttributes.VarAttributesMap.get(attr, 0x00000000)   
        return VarAttr, VarProp
    @staticmethod
    def ValidateVarAttributes(var_attr_str):
        if not var_attr_str:
            return True, ""
        attr_list = var_attr_str.split(",")
        attr_temp = []
        for attr in attr_list:
            attr = attr.strip()
            attr_temp.append(attr)
            if attr not in VariableAttributes.VarAttributesMap:
                return False, "The variable attribute %s is not support to be specified in dsc file. Supported variable attribute are ['BS','NV','RT','RO'] "
        if 'RT' in attr_temp and 'BS' not in attr_temp:
            return False, "the RT attribute need the BS attribute to be present"
        return True, ""
