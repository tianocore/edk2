## @file
# This file is used to define class objects of INF file header. 
# It will consumed by InfParser. 
#
# Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available 
# under the terms and conditions of the BSD License which accompanies this 
# distribution. The full text of the license may be found at 
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
        if not (FileName == '' or FileName == None):
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
        if not (Abstract == '' or Abstract == None):
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
        if not (Description == '' or Description == None):
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
        if not (Copyright == '' or Copyright == None):
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
        if not (License == '' or License == None):
            self.License = License
            return True
        else:
            return False

    ## GetLicense
    #          
    def GetLicense(self):
        return self.License