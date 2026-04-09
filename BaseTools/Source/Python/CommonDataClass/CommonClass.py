## @file
# This file is used to define common items of class object
#
# Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent


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
    #  Organize to a single line format string
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

    def __deepcopy__(self,memo):
        new_sku = SkuInfoClass()
        new_sku.SkuIdName = self.SkuIdName
        new_sku.SkuId = self.SkuId
        new_sku.VariableName = self.VariableName
        new_sku.VariableGuid = self.VariableGuid
        new_sku.VariableGuidValue = self.VariableGuidValue
        new_sku.VariableOffset = self.VariableOffset
        new_sku.HiiDefaultValue = self.HiiDefaultValue
        new_sku.VariableAttribute = self.VariableAttribute
        new_sku.DefaultStoreDict = {key:value for key,value in self.DefaultStoreDict.items()}
        new_sku.VpdOffset = self.VpdOffset
        new_sku.DefaultValue = self.DefaultValue
        return new_sku
