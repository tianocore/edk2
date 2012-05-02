## @file
# This file is used to define class objects of INF file [UserExtension] section. 
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
InfUserExtensionsObject
'''

from Logger import StringTable as ST
from Logger import ToolError
import Logger.Log as Logger
from Library import GlobalData 

from Library.Misc import Sdict

class InfUserExtensionItem():
    def __init__(self,
                 Content = '',
                 UserId = '',
                 IdString = ''):
        self.Content  = Content
        self.UserId   = UserId
        self.IdString = IdString
        self.SupArchList = []
    
    def SetContent(self, Content):
        self.Content = Content
    def GetContent(self):
        return self.Content
    
    def SetUserId(self, UserId):
        self.UserId = UserId
    def GetUserId(self):
        return self.UserId
    
    def SetIdString(self, IdString):
        self.IdString = IdString
    def GetIdString(self):
        return self.IdString
    
    def SetSupArchList(self, SupArchList):
        self.SupArchList = SupArchList
    def GetSupArchList(self):
        return self.SupArchList
    
##
#
#
#
class InfUserExtensionObject():
    def __init__(self):
        self.UserExtension = Sdict()
    
    def SetUserExtension(self, UserExtensionCont, IdContent=None, LineNo=None):
        if not UserExtensionCont or UserExtensionCont == '':
            return True
        #
        # IdContent is a list contain UserId and IdString 
        # For this call the general section header  parser, if no definition of
        # IdString/UserId, it will return 'COMMON'
        #
        for IdContentItem in IdContent:                              
            InfUserExtensionItemObj = InfUserExtensionItem()
            if IdContentItem[0] == 'COMMON':
                UserId = ''
            else:
                UserId = IdContentItem[0]
                
            if IdContentItem[1] == 'COMMON':
                IdString = ''
            else:
                IdString = IdContentItem[1]   
            
            #
            # Fill UserExtensionObj members.
            #     
            InfUserExtensionItemObj.SetUserId(UserId)
            InfUserExtensionItemObj.SetIdString(IdString)
            InfUserExtensionItemObj.SetContent(UserExtensionCont)
            InfUserExtensionItemObj.SetSupArchList(IdContentItem[2]) 
            
            for CheckItem in self.UserExtension:
                if IdContentItem[0] == CheckItem[0] and IdContentItem[1] == CheckItem[1]:
                    if IdContentItem[2].upper() == 'COMMON' or CheckItem[2].upper() == 'COMMON':
                        #
                        # For COMMON ARCH type, do special check.
                        #
                        Logger.Error('InfParser', 
                            ToolError.FORMAT_INVALID,
                            ST.ERR_INF_PARSER_UE_SECTION_DUPLICATE_ERROR%\
                            (IdContentItem[0] + '.' + IdContentItem[1] + '.' + IdContentItem[2]),
                            File=GlobalData.gINF_MODULE_NAME, 
                            Line=LineNo,
                            ExtraData=None)
            
            if self.UserExtension.has_key(IdContentItem):           
                #
                # Each UserExtensions section header must have a unique set 
                # of UserId, IdString and Arch values.
                # This means that the same UserId can be used in more than one 
                # section header, provided the IdString or Arch values are 
                # different. The same IdString values can be used in more than 
                # one section header if the UserId or Arch values are 
                # different. The same UserId and the same IdString can be used 
                # in a section header if the Arch values are different in each 
                # of the section headers.
                #
                Logger.Error('InfParser', 
                             ToolError.FORMAT_INVALID,
                             ST.ERR_INF_PARSER_UE_SECTION_DUPLICATE_ERROR%\
                             (IdContentItem[0] + '.' + IdContentItem[1] + '.' + IdContentItem[2]),
                             File=GlobalData.gINF_MODULE_NAME, 
                             Line=LineNo,
                             ExtraData=None)
            else:
                UserExtensionList = []
                UserExtensionList.append(InfUserExtensionItemObj)
                self.UserExtension[IdContentItem] = UserExtensionList
        
        return True
        
    def GetUserExtension(self):
        return self.UserExtension