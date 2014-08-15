## @file
# 
# This package manage the VPD PCD information file which will be generated
# by build tool's autogen.
# The VPD PCD information file will be input for third-party BPDG tool which
# is pointed by *_*_*_VPD_TOOL_GUID in conf/tools_def.txt 
#
#
# Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
import Common.LongFilePathOs as os
import re
import Common.EdkLogger as EdkLogger
import Common.BuildToolError as BuildToolError
import subprocess
from Common.LongFilePathSupport import OpenLongFilePath as open

FILE_COMMENT_TEMPLATE = \
"""
## @file
#
#  THIS IS AUTO-GENERATED FILE BY BUILD TOOLS AND PLEASE DO NOT MAKE MODIFICATION.
#
#  This file lists all VPD informations for a platform collected by build.exe.
# 
# Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

"""

## The class manage VpdInfoFile.
#
#  This file contains an ordered (based on position in the DSC file) list of the PCDs specified in the platform description file (DSC). The Value field that will be assigned to the PCD comes from the DSC file, INF file (if not defined in the DSC file) or the DEC file (if not defined in the INF file). This file is used as an input to the BPDG tool.
#  Format for this file (using EBNF notation) is:
#  <File>            :: = [<CommentBlock>]
#                         [<PcdEntry>]*
#  <CommentBlock>    ::=  ["#" <String> <EOL>]*
#  <PcdEntry>        ::=  <PcdName> "|" <Offset> "|" <Size> "|" <Value> <EOL>
#  <PcdName>         ::=  <TokenSpaceCName> "." <PcdCName>
#  <TokenSpaceCName> ::=  C Variable Name of the Token Space GUID
#  <PcdCName>        ::=  C Variable Name of the PCD
#  <Offset>          ::=  {"*"} {<HexNumber>}
#  <HexNumber>       ::=  "0x" (a-fA-F0-9){1,8}
#  <Size>            ::=  <HexNumber>
#  <Value>           ::=  {<HexNumber>} {<NonNegativeInt>} {<QString>} {<Array>}
#  <NonNegativeInt>  ::=  (0-9)+
#  <QString>         ::=  ["L"] <DblQuote> <String> <DblQuote>
#  <DblQuote>        ::=  0x22
#  <Array>           ::=  {<CArray>} {<NList>}
#  <CArray>          ::=  "{" <HexNumber> ["," <HexNumber>]* "}"
#  <NList>           ::=  <HexNumber> ["," <HexNumber>]*
#
class VpdInfoFile:
    
    ## The mapping dictionary from datum type to size string.
    _MAX_SIZE_TYPE = {"BOOLEAN":"1", "UINT8":"1", "UINT16":"2", "UINT32":"4", "UINT64":"8"}
    _rVpdPcdLine = None 
    ## Constructor
    def __init__(self):
        ## Dictionary for VPD in following format
        #
        #  Key    : PcdClassObject instance. 
        #           @see BuildClassObject.PcdClassObject
        #  Value  : offset in different SKU such as [sku1_offset, sku2_offset]
        self._VpdArray = {}
    
    ## Add a VPD PCD collected from platform's autogen when building.
    #
    #  @param vpds  The list of VPD PCD collected for a platform.
    #               @see BuildClassObject.PcdClassObject
    #
    #  @param offset integer value for VPD's offset in specific SKU.
    #
    def Add(self, Vpd, Offset):
        if (Vpd == None):
            EdkLogger.error("VpdInfoFile", BuildToolError.ATTRIBUTE_UNKNOWN_ERROR, "Invalid VPD PCD entry.")
        
        if not (Offset >= 0 or Offset == "*"):
            EdkLogger.error("VpdInfoFile", BuildToolError.PARAMETER_INVALID, "Invalid offset parameter: %s." % Offset)
        
        if Vpd.DatumType == "VOID*":
            if Vpd.MaxDatumSize <= 0:
                EdkLogger.error("VpdInfoFile", BuildToolError.PARAMETER_INVALID, 
                                "Invalid max datum size for VPD PCD %s.%s" % (Vpd.TokenSpaceGuidCName, Vpd.TokenCName))
        elif Vpd.DatumType in ["BOOLEAN", "UINT8", "UINT16", "UINT32", "UINT64"]: 
            if Vpd.MaxDatumSize == None or Vpd.MaxDatumSize == "":
                Vpd.MaxDatumSize = VpdInfoFile._MAX_SIZE_TYPE[Vpd.DatumType]
        else:
            EdkLogger.error("VpdInfoFile", BuildToolError.PARAMETER_INVALID,  
                            "Invalid DatumType %s for VPD PCD %s.%s" % (Vpd.DatumType, Vpd.TokenSpaceGuidCName, Vpd.TokenCName))
            
        if Vpd not in self._VpdArray.keys():
            #
            # If there is no Vpd instance in dict, that imply this offset for a given SKU is a new one 
            #
            self._VpdArray[Vpd] = [Offset]
        else:
            #
            # If there is an offset for a specific SKU in dict, then append this offset for other sku to array.
            #
            self._VpdArray[Vpd].append(Offset)
            
        
    ## Generate VPD PCD information into a text file
    #  
    #  If parameter FilePath is invalid, then assert.
    #  If 
    #  @param FilePath        The given file path which would hold VPD information
    def Write(self, FilePath):
        if not (FilePath != None or len(FilePath) != 0):
            EdkLogger.error("VpdInfoFile", BuildToolError.PARAMETER_INVALID,  
                            "Invalid parameter FilePath: %s." % FilePath)        
        try:
            fd = open(FilePath, "w")
        except:
            EdkLogger.error("VpdInfoFile", 
                            BuildToolError.FILE_OPEN_FAILURE, 
                            "Fail to open file %s for written." % FilePath)
        
        try:
            # write file header
            fd.write(FILE_COMMENT_TEMPLATE)

            # write each of PCD in VPD type
            Pcds = self._VpdArray.keys()
            Pcds.sort()
            for Pcd in Pcds:
                i = 0
                for Offset in self._VpdArray[Pcd]:
                    PcdValue = str(Pcd.SkuInfoList[Pcd.SkuInfoList.keys()[i]].DefaultValue).strip()
                    if PcdValue == "" :
                        PcdValue  = Pcd.DefaultValue
                        
                    fd.write("%s.%s|%s|%s|%s|%s  \n" % (Pcd.TokenSpaceGuidCName, Pcd.TokenCName, str(Pcd.SkuInfoList.keys()[i]),str(Offset).strip(), str(Pcd.MaxDatumSize).strip(),PcdValue))
                    i += 1
        except:
            EdkLogger.error("VpdInfoFile",
                            BuildToolError.FILE_WRITE_FAILURE,
                            "Fail to write file %s" % FilePath) 
        fd.close()

    ## Read an existing VPD PCD info file.
    #
    #  This routine will read VPD PCD information from existing file and construct
    #  internal PcdClassObject array.
    #  This routine could be used by third-party tool to parse VPD info file content.
    #
    #  @param FilePath The full path string for existing VPD PCD info file.
    def Read(self, FilePath):
        try:
            fd = open(FilePath, "r")
        except:
            EdkLogger.error("VpdInfoFile", 
                            BuildToolError.FILE_OPEN_FAILURE, 
                            "Fail to open file %s for written." % FilePath)
        Lines = fd.readlines()
        for Line in Lines:
            Line = Line.strip()
            if len(Line) == 0 or Line.startswith("#"):
                continue
            
            #
            # the line must follow output format defined in BPDG spec.
            #
            try:
                PcdName, SkuId,Offset, Size, Value = Line.split("#")[0].split("|")
                PcdName, SkuId,Offset, Size, Value = PcdName.strip(), SkuId.strip(),Offset.strip(), Size.strip(), Value.strip()
                TokenSpaceName, PcdTokenName = PcdName.split(".")
            except:
                EdkLogger.error("BPDG", BuildToolError.PARSER_ERROR, "Fail to parse VPD information file %s" % FilePath)
            
            Found = False
            
            for VpdObject in self._VpdArray.keys():
                for sku in VpdObject.SkuInfoList.keys(): 
                    if VpdObject.TokenSpaceGuidCName == TokenSpaceName and VpdObject.TokenCName == PcdTokenName.strip() and sku == SkuId:
                        if self._VpdArray[VpdObject][VpdObject.SkuInfoList.keys().index(sku)] == "*":
                            if Offset == "*":
                                EdkLogger.error("BPDG", BuildToolError.FORMAT_INVALID, "The offset of %s has not been fixed up by third-party BPDG tool." % PcdName)                              
                            self._VpdArray[VpdObject][VpdObject.SkuInfoList.keys().index(sku)] = Offset
                        Found = True
            if not Found:
                EdkLogger.error("BPDG", BuildToolError.PARSER_ERROR, "Can not find PCD defined in VPD guid file.")
                
    ## Get count of VPD PCD collected from platform's autogen when building.
    #
    #  @return The integer count value 
    def GetCount(self):
        Count = 0
        for OffsetList in self._VpdArray.values():
            Count += len(OffsetList)
            
        return Count
    
    ## Get an offset value for a given VPD PCD
    #
    #  Because BPDG only support one Sku, so only return offset for SKU default.   
    #
    #  @param vpd    A given VPD PCD 
    def GetOffset(self, vpd):
        if not self._VpdArray.has_key(vpd):
            return None
        
        if len(self._VpdArray[vpd]) == 0:
            return None
        
        return self._VpdArray[vpd]
    
## Call external BPDG tool to process VPD file
#    
#  @param ToolPath      The string path name for BPDG tool
#  @param VpdFileName   The string path name for VPD information guid.txt
# 
def CallExtenalBPDGTool(ToolPath, VpdFileName):
    assert ToolPath != None, "Invalid parameter ToolPath"
    assert VpdFileName != None and os.path.exists(VpdFileName), "Invalid parameter VpdFileName"
    
    OutputDir = os.path.dirname(VpdFileName)
    FileName = os.path.basename(VpdFileName)
    BaseName, ext = os.path.splitext(FileName)
    OutputMapFileName = os.path.join(OutputDir, "%s.map" % BaseName)
    OutputBinFileName = os.path.join(OutputDir, "%s.bin" % BaseName)
          
    try:
        PopenObject = subprocess.Popen([ToolPath,
                                        '-o', OutputBinFileName, 
                                        '-m', OutputMapFileName,
                                        '-q',
                                        '-f',
                                        VpdFileName],
                                        stdout=subprocess.PIPE, 
                                        stderr= subprocess.PIPE)
    except Exception, X:
        EdkLogger.error("BPDG", BuildToolError.COMMAND_FAILURE, ExtraData="%s" % (str(X)))
    (out, error) = PopenObject.communicate()
    print out
    while PopenObject.returncode == None :
        PopenObject.wait()
    
    if PopenObject.returncode != 0:
        if PopenObject.returncode != 0:
            EdkLogger.debug(EdkLogger.DEBUG_1, "Fail to call BPDG tool", str(error))
            EdkLogger.error("BPDG", BuildToolError.COMMAND_FAILURE, "Fail to execute BPDG tool with exit code: %d, the error message is: \n %s" % \
                            (PopenObject.returncode, str(error)))
        
    return PopenObject.returncode
