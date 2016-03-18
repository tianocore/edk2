## @file
# This file is used to define common class objects for INF file. 
# It will consumed by InfParser
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
InfCommonObject
'''

## InfLineCommentObject
#  
#  Comment Object for any line in the INF file 
#   
#  #
#  # HeaderComment
#  #
#  Line # TailComment
#
class InfLineCommentObject():
    def __init__(self):
        self.HeaderComments = ''
        self.TailComments = ''
    
    def SetHeaderComments(self, HeaderComments):
        self.HeaderComments = HeaderComments
    
    def GetHeaderComments(self):
        return self.HeaderComments
    
    def SetTailComments(self, TailComments):
        self.TailComments = TailComments

    def GetTailComments(self):
        return self.TailComments   
       
## CurrentLine
#    
class CurrentLine():
    def __init__(self):
        self.LineNo = ''
        self.LineString = ''
        self.FileName = ''

    ## SetLineNo
    #     
    # @param LineNo: LineNo 
    #     
    def SetLineNo(self, LineNo):
        self.LineNo = LineNo
        
    ## GetLineNo
    #         
    def GetLineNo(self):
        return self.LineNo

    ## SetLineString
    #     
    # @param LineString: Line String content 
    #     
    def SetLineString(self, LineString):
        self.LineString = LineString
        
    ## GetLineString
    #         
    def GetLineString(self):
        return self.LineString

    ## SetFileName
    #     
    # @param FileName: File Name
    #       
    def SetFileName(self, FileName):
        self.FileName = FileName
        
    ## GetFileName
    #  
    def GetFileName(self):
        return self.FileName
    
## 
# Inf Section common data
#
class InfSectionCommonDef():
    def __init__(self):
        #
        # # 
        # # HeaderComments at here
        # #
        # [xxSection] TailComments at here
        # data
        #
        self.HeaderComments = ''
        self.TailComments   = ''
        #
        # The support arch list of this section
        #
        self.SupArchList  = []
        
        #
        # Store all section content
        # Key is supported Arch
        #
        self.AllContent   = {}

    ## SetHeaderComments
    #     
    # @param HeaderComments: HeaderComments
    #        
    def SetHeaderComments(self, HeaderComments):
        self.HeaderComments = HeaderComments

    ## GetHeaderComments
    #        
    def GetHeaderComments(self):
        return self.HeaderComments

    ## SetTailComments
    #     
    # @param TailComments: TailComments
    #            
    def SetTailComments(self, TailComments):
        self.TailComments = TailComments

    ## GetTailComments
    #   
    def GetTailComments(self):
        return self.TailComments

    ## SetSupArchList
    #     
    # @param Arch: Arch
    #         
    def SetSupArchList(self, Arch):
        if Arch not in self.SupArchList:
            self.SupArchList.append(Arch)

    ## GetSupArchList
    #      
    def GetSupArchList(self):
        return self.SupArchList

    ## SetAllContent
    #     
    # @param ArchList: ArchList
    # @param Content: Content
    #     
    def SetAllContent(self, Content):
        self.AllContent = Content
        
    ## GetAllContent
    #     
    def GetAllContent(self):
        return self.AllContent
