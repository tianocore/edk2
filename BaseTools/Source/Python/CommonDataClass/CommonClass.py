## @file
# This file is used to define common items of class object
#
# Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.    The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


## SkuInfoClass
#
# This class defined SkuInfo item used in Module/Platform/Package files
# 
# @param object:           Inherited from object class
# @param SkuIdName:        Input value for SkuIdName, default is ''
# @param SkuId:            Input value for SkuId, default is ''
# @param VariableName:     Input value for VariableName, default is ''
# @param VariableGuid:     Input value for VariableGuid, default is ''
# @param VariableOffset:   Input value for VariableOffset, default is ''
# @param HiiDefaultValue:  Input value for HiiDefaultValue, default is ''
# @param VpdOffset:        Input value for VpdOffset, default is ''
# @param DefaultValue:     Input value for DefaultValue, default is ''
#
# @var SkuIdName:          To store value for SkuIdName
# @var SkuId:              To store value for SkuId
# @var VariableName:       To store value for VariableName
# @var VariableGuid:       To store value for VariableGuid
# @var VariableOffset:     To store value for VariableOffset
# @var HiiDefaultValue:    To store value for HiiDefaultValue
# @var VpdOffset:          To store value for VpdOffset
# @var DefaultValue:       To store value for DefaultValue
#
class SkuInfoClass(object):
    def __init__(self, SkuIdName = '', SkuId = '', VariableName = '', VariableGuid = '', VariableOffset = '', 
                 HiiDefaultValue = '', VpdOffset = '', DefaultValue = '', VariableGuidValue = '', VariableAttribute = '', DefaultStore = None):
        self.SkuIdName = SkuIdName
        self.SkuId = SkuId
        
        #
        # Used by Hii
        #
        if DefaultStore is None:
            DefaultStore = {}
        self.VariableName = VariableName
        self.VariableGuid = VariableGuid
        self.VariableGuidValue = VariableGuidValue
        self.VariableOffset = VariableOffset
        self.HiiDefaultValue = HiiDefaultValue
        self.VariableAttribute = VariableAttribute
        self.DefaultStoreDict = DefaultStore
        
        #
        # Used by Vpd
        #
        self.VpdOffset = VpdOffset
        
        #
        # Used by Default
        #
        self.DefaultValue = DefaultValue
        
    ## Convert the class to a string
    #
    #  Convert each member of the class to string
    #  Organize to a signle line format string
    #
    #  @retval Rtn Formatted String
    #
    def __str__(self):
        Rtn = 'SkuId = ' + str(self.SkuId) + "," + \
                    'SkuIdName = ' + str(self.SkuIdName) + "," + \
                    'VariableName = ' + str(self.VariableName) + "," + \
                    'VariableGuid = ' + str(self.VariableGuid) + "," + \
                    'VariableOffset = ' + str(self.VariableOffset) + "," + \
                    'HiiDefaultValue = ' + str(self.HiiDefaultValue) + "," + \
                    'VpdOffset = ' + str(self.VpdOffset) + "," + \
                    'DefaultValue = ' + str(self.DefaultValue) + ","
        return Rtn
