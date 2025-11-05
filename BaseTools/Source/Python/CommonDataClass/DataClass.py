## @file
# This file is used to define class for data structure used in ECC
#
# Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent

##
# Import Modules
#
import Common.EdkLogger as EdkLogger

##
# Static values for data models
#
MODEL_UNKNOWN = 0

MODEL_FILE_C = 1001
MODEL_FILE_H = 1002
MODEL_FILE_ASM = 1003
MODEL_FILE_INF = 1011
MODEL_FILE_DEC = 1012
MODEL_FILE_DSC = 1013
MODEL_FILE_FDF = 1014
MODEL_FILE_INC = 1015
MODEL_FILE_CIF = 1016
MODEL_FILE_UNI = 1017
MODEL_FILE_OTHERS = 1099

MODEL_IDENTIFIER_FILE_HEADER = 2001
MODEL_IDENTIFIER_FUNCTION_HEADER = 2002
MODEL_IDENTIFIER_COMMENT = 2003
MODEL_IDENTIFIER_PARAMETER = 2004
MODEL_IDENTIFIER_STRUCTURE = 2005
MODEL_IDENTIFIER_VARIABLE = 2006
MODEL_IDENTIFIER_INCLUDE = 2007
MODEL_IDENTIFIER_PREDICATE_EXPRESSION = 2008
MODEL_IDENTIFIER_ENUMERATE = 2009
MODEL_IDENTIFIER_PCD = 2010
MODEL_IDENTIFIER_UNION = 2011
MODEL_IDENTIFIER_MACRO_IFDEF = 2012
MODEL_IDENTIFIER_MACRO_IFNDEF = 2013
MODEL_IDENTIFIER_MACRO_DEFINE = 2014
MODEL_IDENTIFIER_MACRO_ENDIF = 2015
MODEL_IDENTIFIER_MACRO_PROGMA = 2016
MODEL_IDENTIFIER_FUNCTION_CALLING = 2018
MODEL_IDENTIFIER_TYPEDEF = 2017
MODEL_IDENTIFIER_FUNCTION_DECLARATION = 2019
MODEL_IDENTIFIER_ASSIGNMENT_EXPRESSION = 2020

MODEL_EFI_PROTOCOL = 3001
MODEL_EFI_PPI = 3002
MODEL_EFI_GUID = 3003
MODEL_EFI_LIBRARY_CLASS = 3004
MODEL_EFI_LIBRARY_INSTANCE = 3005
MODEL_EFI_PCD = 3006
MODEL_EFI_SOURCE_FILE = 3007
MODEL_EFI_BINARY_FILE = 3008
MODEL_EFI_SKU_ID = 3009
MODEL_EFI_INCLUDE = 3010
MODEL_EFI_DEPEX = 3011
MODEL_EFI_DEFAULT_STORES = 3012

MODEL_PCD = 4000
MODEL_PCD_FIXED_AT_BUILD = 4001
MODEL_PCD_PATCHABLE_IN_MODULE = 4002
MODEL_PCD_FEATURE_FLAG = 4003
MODEL_PCD_DYNAMIC_EX = 4004
MODEL_PCD_DYNAMIC_EX_DEFAULT = 4005
MODEL_PCD_DYNAMIC_EX_VPD = 4006
MODEL_PCD_DYNAMIC_EX_HII = 4007
MODEL_PCD_DYNAMIC = 4008
MODEL_PCD_DYNAMIC_DEFAULT = 4009
MODEL_PCD_DYNAMIC_VPD = 4010
MODEL_PCD_DYNAMIC_HII = 4011
MODEL_PCD_TYPE_LIST = [MODEL_PCD_FIXED_AT_BUILD,
                        MODEL_PCD_PATCHABLE_IN_MODULE,
                        MODEL_PCD_FEATURE_FLAG,
                        MODEL_PCD_DYNAMIC_DEFAULT,
                        MODEL_PCD_DYNAMIC_HII,
                        MODEL_PCD_DYNAMIC_VPD,
                        MODEL_PCD_DYNAMIC_EX_DEFAULT,
                        MODEL_PCD_DYNAMIC_EX_HII,
                        MODEL_PCD_DYNAMIC_EX_VPD
                       ]

MODEL_META_DATA_HEADER_COMMENT = 5000
MODEL_META_DATA_HEADER = 5001
MODEL_META_DATA_INCLUDE = 5002
MODEL_META_DATA_DEFINE = 5003
MODEL_META_DATA_CONDITIONAL_STATEMENT_IF = 5004
MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSE = 5005
MODEL_META_DATA_CONDITIONAL_STATEMENT_IFDEF = 5006
MODEL_META_DATA_CONDITIONAL_STATEMENT_IFNDEF = 5007
MODEL_META_DATA_CONDITIONAL_STATEMENT_ERROR = 5400
MODEL_META_DATA_BUILD_OPTION = 5008
MODEL_META_DATA_COMPONENT = 5009
MODEL_META_DATA_USER_EXTENSION = 5010
MODEL_META_DATA_PACKAGE = 5011
MODEL_META_DATA_NMAKE = 5012
MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSEIF = 5013
MODEL_META_DATA_CONDITIONAL_STATEMENT_ENDIF = 5014
MODEL_META_DATA_COMMENT = 5016
MODEL_META_DATA_GLOBAL_DEFINE = 5017
MODEL_META_DATA_SECTION_HEADER = 5100
MODEL_META_DATA_SUBSECTION_HEADER = 5200
MODEL_META_DATA_TAIL_COMMENT = 5300

MODEL_EXTERNAL_DEPENDENCY = 10000

MODEL_LIST = [('MODEL_UNKNOWN', MODEL_UNKNOWN),
              ('MODEL_FILE_C', MODEL_FILE_C),
              ('MODEL_FILE_H', MODEL_FILE_H),
              ('MODEL_FILE_ASM', MODEL_FILE_ASM),
              ('MODEL_FILE_INF', MODEL_FILE_INF),
              ('MODEL_FILE_DEC', MODEL_FILE_DEC),
              ('MODEL_FILE_DSC', MODEL_FILE_DSC),
              ('MODEL_FILE_FDF', MODEL_FILE_FDF),
              ('MODEL_FILE_INC', MODEL_FILE_INC),
              ('MODEL_FILE_CIF', MODEL_FILE_CIF),
              ('MODEL_FILE_OTHERS', MODEL_FILE_OTHERS),
              ('MODEL_IDENTIFIER_FILE_HEADER', MODEL_IDENTIFIER_FILE_HEADER),
              ('MODEL_IDENTIFIER_FUNCTION_HEADER', MODEL_IDENTIFIER_FUNCTION_HEADER),
              ('MODEL_IDENTIFIER_COMMENT', MODEL_IDENTIFIER_COMMENT),
              ('MODEL_IDENTIFIER_PARAMETER', MODEL_IDENTIFIER_PARAMETER),
              ('MODEL_IDENTIFIER_STRUCTURE', MODEL_IDENTIFIER_STRUCTURE),
              ('MODEL_IDENTIFIER_VARIABLE', MODEL_IDENTIFIER_VARIABLE),
              ('MODEL_IDENTIFIER_INCLUDE', MODEL_IDENTIFIER_INCLUDE),
              ('MODEL_IDENTIFIER_PREDICATE_EXPRESSION', MODEL_IDENTIFIER_PREDICATE_EXPRESSION),
              ('MODEL_IDENTIFIER_ENUMERATE', MODEL_IDENTIFIER_ENUMERATE),
              ('MODEL_IDENTIFIER_PCD', MODEL_IDENTIFIER_PCD),
              ('MODEL_IDENTIFIER_UNION', MODEL_IDENTIFIER_UNION),
              ('MODEL_IDENTIFIER_MACRO_IFDEF', MODEL_IDENTIFIER_MACRO_IFDEF),
              ('MODEL_IDENTIFIER_MACRO_IFNDEF', MODEL_IDENTIFIER_MACRO_IFNDEF),
              ('MODEL_IDENTIFIER_MACRO_DEFINE', MODEL_IDENTIFIER_MACRO_DEFINE),
              ('MODEL_IDENTIFIER_MACRO_ENDIF', MODEL_IDENTIFIER_MACRO_ENDIF),
              ('MODEL_IDENTIFIER_MACRO_PROGMA', MODEL_IDENTIFIER_MACRO_PROGMA),
              ('MODEL_IDENTIFIER_FUNCTION_CALLING', MODEL_IDENTIFIER_FUNCTION_CALLING),
              ('MODEL_IDENTIFIER_TYPEDEF', MODEL_IDENTIFIER_TYPEDEF),
              ('MODEL_IDENTIFIER_FUNCTION_DECLARATION', MODEL_IDENTIFIER_FUNCTION_DECLARATION),
              ('MODEL_IDENTIFIER_ASSIGNMENT_EXPRESSION', MODEL_IDENTIFIER_ASSIGNMENT_EXPRESSION),
              ('MODEL_EFI_PROTOCOL', MODEL_EFI_PROTOCOL),
              ('MODEL_EFI_PPI', MODEL_EFI_PPI),
              ('MODEL_EFI_GUID', MODEL_EFI_GUID),
              ('MODEL_EFI_LIBRARY_CLASS', MODEL_EFI_LIBRARY_CLASS),
              ('MODEL_EFI_LIBRARY_INSTANCE', MODEL_EFI_LIBRARY_INSTANCE),
              ('MODEL_EFI_PCD', MODEL_EFI_PCD),
              ('MODEL_EFI_SKU_ID', MODEL_EFI_SKU_ID),
              ('MODEL_EFI_INCLUDE', MODEL_EFI_INCLUDE),
              ('MODEL_EFI_DEPEX', MODEL_EFI_DEPEX),
              ('MODEL_IDENTIFIER_UNION', MODEL_IDENTIFIER_UNION),
              ('MODEL_EFI_SOURCE_FILE', MODEL_EFI_SOURCE_FILE),
              ('MODEL_EFI_BINARY_FILE', MODEL_EFI_BINARY_FILE),
              ('MODEL_PCD', MODEL_PCD),
              ('MODEL_PCD_FIXED_AT_BUILD', MODEL_PCD_FIXED_AT_BUILD),
              ('MODEL_PCD_PATCHABLE_IN_MODULE', MODEL_PCD_PATCHABLE_IN_MODULE),
              ('MODEL_PCD_FEATURE_FLAG', MODEL_PCD_FEATURE_FLAG),
              ('MODEL_PCD_DYNAMIC_EX', MODEL_PCD_DYNAMIC_EX),
              ('MODEL_PCD_DYNAMIC_EX_DEFAULT', MODEL_PCD_DYNAMIC_EX_DEFAULT),
              ('MODEL_PCD_DYNAMIC_EX_VPD', MODEL_PCD_DYNAMIC_EX_VPD),
              ('MODEL_PCD_DYNAMIC_EX_HII', MODEL_PCD_DYNAMIC_EX_HII),
              ('MODEL_PCD_DYNAMIC', MODEL_PCD_DYNAMIC),
              ('MODEL_PCD_DYNAMIC_DEFAULT', MODEL_PCD_DYNAMIC_DEFAULT),
              ('MODEL_PCD_DYNAMIC_VPD', MODEL_PCD_DYNAMIC_VPD),
              ('MODEL_PCD_DYNAMIC_HII', MODEL_PCD_DYNAMIC_HII),
              ("MODEL_META_DATA_HEADER", MODEL_META_DATA_HEADER),
              ("MODEL_META_DATA_INCLUDE", MODEL_META_DATA_INCLUDE),
              ("MODEL_META_DATA_DEFINE", MODEL_META_DATA_DEFINE),
              ("MODEL_META_DATA_CONDITIONAL_STATEMENT_IF", MODEL_META_DATA_CONDITIONAL_STATEMENT_IF),
              ("MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSE", MODEL_META_DATA_CONDITIONAL_STATEMENT_ELSE),
              ("MODEL_META_DATA_CONDITIONAL_STATEMENT_IFDEF", MODEL_META_DATA_CONDITIONAL_STATEMENT_IFDEF),
              ("MODEL_META_DATA_CONDITIONAL_STATEMENT_IFNDEF", MODEL_META_DATA_CONDITIONAL_STATEMENT_IFNDEF),
              ("MODEL_META_DATA_CONDITIONAL_STATEMENT_ERROR", MODEL_META_DATA_CONDITIONAL_STATEMENT_ERROR),
              ("MODEL_META_DATA_BUILD_OPTION", MODEL_META_DATA_BUILD_OPTION),
              ("MODEL_META_DATA_COMPONENT", MODEL_META_DATA_COMPONENT),
              ('MODEL_META_DATA_USER_EXTENSION', MODEL_META_DATA_USER_EXTENSION),
              ('MODEL_META_DATA_PACKAGE', MODEL_META_DATA_PACKAGE),
              ('MODEL_META_DATA_NMAKE', MODEL_META_DATA_NMAKE),
              ('MODEL_META_DATA_COMMENT', MODEL_META_DATA_COMMENT)
             ]

## FunctionClass
#
# This class defines a structure of a function
#
# @param ID:               ID of a Function
# @param Header:           Header of a Function
# @param Modifier:         Modifier of a Function
# @param Name:             Name of a Function
# @param ReturnStatement:  ReturnStatement of a Function
# @param StartLine:        StartLine of a Function
# @param StartColumn:      StartColumn of a Function
# @param EndLine:          EndLine of a Function
# @param EndColumn:        EndColumn of a Function
# @param BodyStartLine:    BodyStartLine of a Function Body
# @param BodyStartColumn:  BodyStartColumn of a Function Body
# @param BelongsToFile:    The Function belongs to which file
# @param IdentifierList:   IdentifierList of a File
# @param PcdList:          PcdList of a File
#
# @var ID:                 ID of a Function
# @var Header:             Header of a Function
# @var Modifier:           Modifier of a Function
# @var Name:               Name of a Function
# @var ReturnStatement:    ReturnStatement of a Function
# @var StartLine:          StartLine of a Function
# @var StartColumn:        StartColumn of a Function
# @var EndLine:            EndLine of a Function
# @var EndColumn:          EndColumn of a Function
# @var BodyStartLine:      StartLine of a Function Body
# @var BodyStartColumn:    StartColumn of a Function Body
# @var BelongsToFile:      The Function belongs to which file
# @var IdentifierList:     IdentifierList of a File
# @var PcdList:            PcdList of a File
#
class FunctionClass(object):
    def __init__(self, ID = -1, Header = '', Modifier = '', Name = '', ReturnStatement = '', \
                 StartLine = -1, StartColumn = -1, EndLine = -1, EndColumn = -1, \
                 BodyStartLine = -1, BodyStartColumn = -1, BelongsToFile = -1, \
                 IdentifierList = [], PcdList = [], \
                 FunNameStartLine = -1, FunNameStartColumn = -1):
        self.ID = ID
        self.Header = Header
        self.Modifier = Modifier
        self.Name = Name
        self.ReturnStatement = ReturnStatement
        self.StartLine = StartLine
        self.StartColumn = StartColumn
        self.EndLine = EndLine
        self.EndColumn = EndColumn
        self.BodyStartLine = BodyStartLine
        self.BodyStartColumn = BodyStartColumn
        self.BelongsToFile = BelongsToFile
        self.FunNameStartLine = FunNameStartLine
        self.FunNameStartColumn = FunNameStartColumn

        self.IdentifierList = IdentifierList
        self.PcdList = PcdList

## IdentifierClass
#
# This class defines a structure of a variable
#
# @param ID:                 ID of a Identifier
# @param Modifier:           Modifier of a Identifier
# @param Type:               Type of a Identifier
# @param Name:               Name of a Identifier
# @param Value:              Value of a Identifier
# @param Model:              Model of a Identifier
# @param BelongsToFile:      The Identifier belongs to which file
# @param BelongsToFunction:  The Identifier belongs to which function
# @param StartLine:          StartLine of a Identifier
# @param StartColumn:        StartColumn of a Identifier
# @param EndLine:            EndLine of a Identifier
# @param EndColumn:          EndColumn of a Identifier
#
# @var ID:                   ID of a Identifier
# @var Modifier:             Modifier of a Identifier
# @var Type:                 Type of a Identifier
# @var Name:                 Name of a Identifier
# @var Value:                Value of a Identifier
# @var Model:                Model of a Identifier
# @var BelongsToFile:        The Identifier belongs to which file
# @var BelongsToFunction:    The Identifier belongs to which function
# @var StartLine:            StartLine of a Identifier
# @var StartColumn:          StartColumn of a Identifier
# @var EndLine:              EndLine of a Identifier
# @var EndColumn:            EndColumn of a Identifier
#
class IdentifierClass(object):
    def __init__(self, ID = -1, Modifier = '', Type = '', Name = '', Value = '', Model = MODEL_UNKNOWN, \
                 BelongsToFile = -1, BelongsToFunction = -1, StartLine = -1, StartColumn = -1, EndLine = -1, EndColumn = -1):
        self.ID = ID
        self.Modifier = Modifier
        self.Type = Type
        self.Name = Name
        self.Value = Value
        self.Model = Model
        self.BelongsToFile = BelongsToFile
        self.BelongsToFunction = BelongsToFunction
        self.StartLine = StartLine
        self.StartColumn = StartColumn
        self.EndLine = EndLine
        self.EndColumn = EndColumn

## PcdClass
#
# This class defines a structure of a Pcd
#
# @param ID:                   ID of a Pcd
# @param CName:                CName of a Pcd
# @param TokenSpaceGuidCName:  TokenSpaceGuidCName of a Pcd
# @param Token:                Token of a Pcd
# @param DatumType:            DatumType of a Pcd
# @param Model:                Model of a Pcd
# @param BelongsToFile:        The Pcd belongs to which file
# @param BelongsToFunction:    The Pcd belongs to which function
# @param StartLine:            StartLine of a Pcd
# @param StartColumn:          StartColumn of a Pcd
# @param EndLine:              EndLine of a Pcd
# @param EndColumn:            EndColumn of a Pcd
#
# @var ID:                     ID of a Pcd
# @var CName:                  CName of a Pcd
# @var TokenSpaceGuidCName:    TokenSpaceGuidCName of a Pcd
# @var Token:                  Token of a Pcd
# @var DatumType:              DatumType of a Pcd
# @var Model:                  Model of a Pcd
# @var BelongsToFile:          The Pcd belongs to which file
# @var BelongsToFunction:      The Pcd belongs to which function
# @var StartLine:              StartLine of a Pcd
# @var StartColumn:            StartColumn of a Pcd
# @var EndLine:                EndLine of a Pcd
# @var EndColumn:              EndColumn of a Pcd
#
class PcdDataClass(object):
    def __init__(self, ID = -1, CName = '', TokenSpaceGuidCName = '', Token = '', DatumType = '', Model = MODEL_UNKNOWN, \
                 BelongsToFile = -1, BelongsToFunction = -1, StartLine = -1, StartColumn = -1, EndLine = -1, EndColumn = -1):
        self.ID = ID
        self.CName = CName
        self.TokenSpaceGuidCName = TokenSpaceGuidCName
        self.Token = Token
        self.DatumType = DatumType
        self.BelongsToFile = BelongsToFile
        self.BelongsToFunction = BelongsToFunction
        self.StartLine = StartLine
        self.StartColumn = StartColumn
        self.EndLine = EndLine
        self.EndColumn = EndColumn

## FileClass
#
# This class defines a structure of a file
#
# @param ID:              ID of a File
# @param Name:            Name of a File
# @param ExtName:         ExtName of a File
# @param Path:            Path of a File
# @param FullPath:        FullPath of a File
# @param Model:           Model of a File
# @param TimeStamp:       TimeStamp of a File
# @param FunctionList:    FunctionList of a File
# @param IdentifierList:  IdentifierList of a File
# @param PcdList:         PcdList of a File
#
# @var ID:                ID of a File
# @var Name:              Name of a File
# @var ExtName:           ExtName of a File
# @var Path:              Path of a File
# @var FullPath:          FullPath of a File
# @var Model:             Model of a File
# @var TimeStamp:         TimeStamp of a File
# @var FunctionList:      FunctionList of a File
# @var IdentifierList:    IdentifierList of a File
# @var PcdList:           PcdList of a File
#
class FileClass(object):
    def __init__(self, ID = -1, Name = '', ExtName = '', Path = '', FullPath = '', Model = MODEL_UNKNOWN, TimeStamp = '', \
                 FunctionList = [], IdentifierList = [], PcdList = []):
        self.ID = ID
        self.Name = Name
        self.ExtName = ExtName
        self.Path = Path
        self.FullPath = FullPath
        self.Model = Model
        self.TimeStamp = TimeStamp

        self.FunctionList = FunctionList
        self.IdentifierList = IdentifierList
        self.PcdList = PcdList
