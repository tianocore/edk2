## @file
# This file is used to define class objects of INF file header.
# It will consumed by InfParser.
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent

'''
InfHeaderObject
'''

## INF file header object
#
# A sample file header
#
# ## @file xxx.inf FileName
# # Abstract
# #
# # Description
# #
# # Copyright
# #
# # License
# #
#
class InfHeaderObject():
    def __init__(self):
        self.FileName    = ''
        self.Abstract    = ''
        self.Description = ''
        self.Copyright   = ''
        self.License     = ''

    ## SetFileName
    #
    # @param FileName: File Name
    #
    def SetFileName(self, FileName):
        if not (FileName == '' or FileName is None):
            self.FileName = FileName
            return True
        else:
            return False

    ## GetFileName
    #
    def GetFileName(self):
        return self.FileName

    ## SetAbstract
    #
    # @param Abstract: Abstract
    #
    def SetAbstract(self, Abstract):
        if not (Abstract == '' or Abstract is None):
            self.Abstract = Abstract
            return True
        else:
            return False

    ## GetAbstract
    #
    def GetAbstract(self):
        return self.Abstract

    ## SetDescription
    #
    # @param Description: Description content
    #
    def SetDescription(self, Description):
        if not (Description == '' or Description is None):
            self.Description = Description
            return True
        else:
            return False

    ## GetAbstract
    #
    def GetDescription(self):
        return self.Description

    ## SetCopyright
    #
    # @param Copyright: Copyright content
    #
    def SetCopyright(self, Copyright):
        if not (Copyright == '' or Copyright is None):
            self.Copyright = Copyright
            return True
        else:
            return False

    ## GetCopyright
    #
    def GetCopyright(self):
        return self.Copyright

    ## SetCopyright
    #
    # @param License: License content
    #
    def SetLicense(self, License):
        if not (License == '' or License is None):
            self.License = License
            return True
        else:
            return False

    ## GetLicense
    #
    def GetLicense(self):
        return self.License
