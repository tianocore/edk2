from ast import Pass, Return
from asyncio.windows_events import NULL
from ctypes.wintypes import SIZEL
from msilib.schema import Error
from tkinter import N
from turtle import goto
from typing import List
from unittest.mock import NonCallableMagicMock
from xmlrpc.client import boolean
from VfrError import *
from CommonCtypes import *
from abc import ABCMeta, abstractmethod
import ctypes

import sys

VFR_PACK_SHOW = 0x02
VFR_PACK_ASSIGN = 0x01
VFR_PACK_PUSH = 0x04
VFR_PACK_POP = 0x08
DEFAULT_PACK_ALIGN = 0x8
DEFAULT_ALIGN = 1
MAX_NAME_LEN = 64
MAX_BIT_WIDTH = 32
EFI_VARSTORE_ID_MAX = 0xFFFF
EFI_BITS_SHIFT_PER_UINT32 = 0x5
EFI_BITS_PER_UINT32 = (1 << EFI_BITS_SHIFT_PER_UINT32)
EFI_FREE_VARSTORE_ID_BITMAP_SIZE = int(
    (EFI_VARSTORE_ID_MAX + 1) / EFI_BITS_PER_UINT32)


class SVfrPackStackNode(object):

    def __init__(self, Identifier, Number):
        self.Identifier = Identifier
        self.Number = Number
        self.Next = None

    def Match(self, Identifier):
        if Identifier == None:
            return True
        elif self.Identifier == None:
            return False
        elif self.Identifier == Identifier:
            return True
        else:
            return False


class SVfrDataType(object):

    def __init__(self, TypeName=''):
        self.TypeName = TypeName
        self.Type = 0
        self.Align = 1
        self.TotalSize = 0
        self.HasBitField = False
        self.Members = None
        self.Next = None


class SVfrDataField(object):

    def __init__(self, FieldName=None):
        self.FieldName = FieldName
        self.FieldType = None
        self.Offset = 0
        self.ArrayNum = 0
        self.IsBitField = False
        self.BitWidth = 0
        self.BitOffset = 0
        self.Next = None

class InternalTypes():
    def __init__(self, TypeName, Type, Size, Align):
        self.TypeName = TypeName
        self.Type = Type
        self.Size = Size
        self.Align = Align

gInternalTypesTable = [
    InternalTypes("UINT64", EFI_IFR_TYPE_NUM_SIZE_64,
                  sizeof(ctypes.c_ulonglong), sizeof(ctypes.c_ulonglong)),
    InternalTypes("UINT32", EFI_IFR_TYPE_NUM_SIZE_32, sizeof(ctypes.c_ulong),
                  sizeof(ctypes.c_ulong)),
    InternalTypes("UINT16", EFI_IFR_TYPE_NUM_SIZE_16, sizeof(ctypes.c_ushort),
                  sizeof(ctypes.c_ushort)),
    InternalTypes("UINT8", EFI_IFR_TYPE_NUM_SIZE_8, sizeof(ctypes.c_ubyte),
                  sizeof(ctypes.c_ubyte)),
    InternalTypes("BOOLEAN", EFI_IFR_TYPE_BOOLEAN, sizeof(ctypes.c_ubyte),
                  sizeof(ctypes.c_ubyte)),
    InternalTypes("EFI_GUID", EFI_IFR_TYPE_OTHER, sizeof(EFI_GUID),
                  sizeof(c_ubyte * 8)),
    InternalTypes("EFI_HII_DATE", EFI_IFR_TYPE_DATE, sizeof(EFI_HII_DATE),
                  sizeof(ctypes.c_ushort)),
    InternalTypes("EFI_STRING_ID", EFI_IFR_TYPE_STRING,
                  sizeof(ctypes.c_ushort), sizeof(ctypes.c_ushort)),
    InternalTypes("EFI_HII_TIME", EFI_IFR_TYPE_TIME, sizeof(EFI_HII_TIME),
                  sizeof(ctypes.c_ubyte)),
    InternalTypes("EFI_HII_REF", EFI_IFR_TYPE_REF, sizeof(EFI_HII_REF),
                  sizeof(EFI_GUID)),
]

class CVfrVarDataTypeDB(object):

    def __init__(self):
        self.__PackAlign = DEFAULT_PACK_ALIGN
        self.__PackStack = None
        self.__DataTypeList = None
        self.__NewDataType = None
        self.__CurrDataType = None
        self.__CurrDataField = None
        self.__FirstNewDataTypeName = None
        self.InternalTypesListInit()


    def InternalTypesListInit(self):
        for i in range(0, len(gInternalTypesTable)):
            pNewType = SVfrDataType()
            pNewType.TypeName = gInternalTypesTable[i].TypeName
            pNewType.Type = gInternalTypesTable[i].Type
            pNewType.Align = gInternalTypesTable[i].Align
            pNewType.TotalSize = gInternalTypesTable[i].Size

            if gInternalTypesTable[i].TypeName == 'EFI_HII_DATE':
                pYearField  = SVfrDataField()
                pMonthField  = SVfrDataField()
                pDayField  = SVfrDataField()

                pYearField.FieldName = 'Year'
                pYearField.FieldType, _ = self.GetDataType('UINT16')
                pYearField.Offset = 0
                pYearField.Next = pMonthField
                pYearField.ArrayNum = 0
                pYearField.IsBitField = False

                pMonthField.FieldName = 'Month'
                pMonthField.FieldType, _ = self.GetDataType('UINT8')
                pMonthField.Offset = 2
                pMonthField.Next = pDayField
                pMonthField.ArrayNum = 0
                pMonthField.IsBitField = False

                pDayField.FieldName = 'Day'
                pDayField.FieldType, _ = self.GetDataType('UINT8')
                pDayField.Offset = 3
                pDayField.Next = None
                pDayField.ArrayNum = 0
                pDayField.IsBitField = False

                pNewType.Members = pYearField

            elif gInternalTypesTable[i].TypeName == 'EFI_HII_TIME':
                pHoursField  = SVfrDataField()
                pMinutesField  = SVfrDataField()
                pSecondsField  = SVfrDataField()

                pHoursField.FieldName = 'Hours'
                pHoursField.FieldType, _ = self.GetDataType('UINT8')
                pHoursField.Offset = 0
                pHoursField.Next = pMinutesField
                pHoursField.ArrayNum = 0
                pHoursField.IsBitField = False

                pMinutesField.FieldName = 'Minutes'
                pMinutesField.FieldType, _ = self.GetDataType('UINT8')
                pMinutesField.Offset = 1
                pMinutesField.Next = pSecondsField
                pMinutesField.ArrayNum = 0
                pMinutesField.IsBitField = False

                pSecondsField.FieldName = 'Seconds'
                pSecondsField.FieldType, _ = self.GetDataType('UINT8')
                pSecondsField.Offset = 2
                pSecondsField.Next = None
                pSecondsField.ArrayNum = 0
                pSecondsField.IsBitField = False

                pNewType.Members = pHoursField

            elif gInternalTypesTable[i].TypeName == 'EFI_HII_REF':
                pQuestionIdField  = SVfrDataField()
                pFormIdField  = SVfrDataField()
                pFormSetGuidField  = SVfrDataField()
                pDevicePathField = SVfrDataField()

                pQuestionIdField.FieldName = 'QuestionId'
                pQuestionIdField.FieldType, _ = self.GetDataType('UINT16')
                pQuestionIdField.Offset = 0
                pQuestionIdField.Next = pFormIdField
                pQuestionIdField.ArrayNum = 0
                pQuestionIdField.IsBitField = False

                pFormIdField.FieldName = 'FormId'
                pFormIdField.FieldType, _ = self.GetDataType('UINT16')
                pFormIdField.Offset = 2
                pFormIdField.Next = pFormSetGuidField
                pFormIdField.ArrayNum = 0
                pFormIdField.IsBitField = False

                pFormSetGuidField.FieldName = 'FormSetGuid'
                pFormSetGuidField.FieldType, _ = self.GetDataType('EFI_GUID')
                pFormSetGuidField.Offset = 4
                pFormSetGuidField.Next = pDevicePathField
                pFormSetGuidField.ArrayNum = 0
                pFormSetGuidField.IsBitField = False

                pDevicePathField.FieldName = 'DevicePath'
                pDevicePathField.FieldType, _ = self.GetDataType('EFI_STRING_ID')
                pDevicePathField.Offset = 20
                pDevicePathField.Next = None
                pDevicePathField.ArrayNum = 0
                pDevicePathField.IsBitField = False

                pNewType.Members = pQuestionIdField

            pNewType.Next = None
            self.__RegisterNewType(pNewType)
            pNewType = None

    def GetDataTypeList(self):
        return self.__DataTypeList

    def Pack(self,
             LineNum,
             Action,
             Identifier=None,
             Number=DEFAULT_PACK_ALIGN):

        if Action & VFR_PACK_SHOW:
            Msg = str.format('value of pragma pack(show) == %d' %
                             (self.__PackAlign))
            gCVfrErrorHandle.PrintMsg(LineNum, 'Warning', Msg)

        if Action & VFR_PACK_PUSH:
            pNew = SVfrPackStackNode(Identifier, self.__PackAlign)
            if pNew == None:
                return VfrReturnCode.VFR_RETURN_FATAL_ERROR
            pNew.Next = self.__PackStack
            self.__PackStack = pNew

        if Action & VFR_PACK_POP:
            pNode = None
            if self.__PackStack == None:
                gCVfrErrorHandle.PrintMsg(LineNum, 'Error', '#pragma pack(pop...) : more pops than pushes')

            pNode = self.__PackStack
            while pNode != None:
                if pNode.Match(Identifier) == True:
                    self.__PackAlign = pNode.Number
                    self.__PackStack = pNode.Next
                pNode = pNode.Next

        if Action & VFR_PACK_ASSIGN:
            PackAlign = (Number + Number % 2) if (Number > 1) else Number
            if PackAlign == 0 or PackAlign > 16:
                gCVfrErrorHandle.PrintMsg(LineNum, 'Error', "expected pragma parameter to be '1', '2', '4', '8', or '16'")
            else:
                self.__PackAlign = PackAlign

        return VfrReturnCode.VFR_RETURN_SUCCESS

    def DeclareDataTypeBegin(self):
        pNewType = SVfrDataType()

        pNewType.TypeName = ''
        pNewType.Type = EFI_IFR_TYPE_OTHER
        pNewType.Align = DEFAULT_ALIGN
        pNewType.TotalSize = 0
        pNewType.HasBitField = False
        pNewType.Members = None
        pNewType.Next = None
        self.__NewDataType = pNewType

    def SetNewTypeType(self, Type):
        if self.__NewDataType == None:
            return VfrReturnCode.VFR_RETURN_ERROR_SKIPED

        if Type == None:
            return VfrReturnCode.VFR_RETURN_FATAL_ERROR

        self.__NewDataType.Type = Type  # need to limit the value of the type

        return VfrReturnCode.VFR_RETURN_SUCCESS

    def SetNewTypeTotalSize(self, Size):
        self.__NewDataType.TotalSize = Size

    def SetNewTypeAlign(self, Align):
        self.__NewDataType.Align = Align

    def SetNewTypeName(self, TypeName):
        if self.__NewDataType == None:
            return VfrReturnCode.VFR_RETURN_ERROR_SKIPED

        if TypeName == None:
            return VfrReturnCode.VFR_RETURN_FATAL_ERROR

        if len(TypeName) >= MAX_NAME_LEN:
            return VfrReturnCode.VFR_RETURN_INVALID_PARAMETER

        pType = self.__DataTypeList
        while pType != None:
            if pType.TypeName == TypeName:
                return VfrReturnCode.VFR_RETURN_REDEFINED
            pType = pType.Next

        self.__NewDataType.TypeName = TypeName
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def __AlignStuff(self, Size, Align):
        return Align - (Size) % (Align)

    def DeclareDataTypeEnd(self):
        if self.__NewDataType.TypeName == '':
            return

        if self.__NewDataType.TotalSize % self.__NewDataType.Align != 0:
            self.__NewDataType.TotalSize += self.__AlignStuff(
                self.__NewDataType.TotalSize, self.__NewDataType.Align)

        self.__RegisterNewType(self.__NewDataType)
        if self.__FirstNewDataTypeName == None:
            self.__FirstNewDataTypeName = self.__NewDataType.TypeName

        self.__NewDataType = None

    # two definitions
    def GetDataTypeSizeByTypeName(self, TypeName):
        Size = 0
        pDataType = self.__DataTypeList
        while pDataType != None:
            if pDataType.TypeName == TypeName:
                Size = pDataType.TotalSize
                return Size, VfrReturnCode.VFR_RETURN_SUCCESS
            pDataType = pDataType.Next

        return Size, VfrReturnCode.VFR_RETURN_UNDEFINED

    def GetDataTypeSizeByDataType(self, DataType):
        Size = 0
        DataType = DataType & 0x0F
        # For user defined data type, the size can't be got by this function.
        if DataType == EFI_IFR_TYPE_OTHER:
            return Size, VfrReturnCode.VFR_RETURN_SUCCESS
        pDataType = self.__DataTypeList
        while pDataType != None:
            if DataType == pDataType.Type:
                Size = pDataType.TotalSize
                return Size, VfrReturnCode.VFR_RETURN_SUCCESS
            pDataType = pDataType.Next

        return Size, VfrReturnCode.VFR_RETURN_UNDEFINED

    def __ExtractStructTypeName(self, VarStr):
        try:
            index = VarStr.index('.')
        except ValueError:
            return VarStr, len(VarStr)
        else:
            return VarStr[0:index], index + 1

    def __ExtractFieldNameAndArrary(self, VarStr, s):

        ArrayIdx = INVALID_ARRAY_INDEX
        s_copy = s
        while (s < len(VarStr) and VarStr[s] != '.' and VarStr[s] != '['
               and VarStr[s] != ']'):
            s += 1

        FName = VarStr[s_copy:s]

        if s == len(VarStr):
            return ArrayIdx, s, FName, VfrReturnCode.VFR_RETURN_SUCCESS

        elif VarStr[s] == '.':
            s += 1
            return ArrayIdx, s, FName, VfrReturnCode.VFR_RETURN_SUCCESS

        elif VarStr[s] == '[':
            s += 1
            try:
                e = s + VarStr[s:].index(']')

            except ValueError:
                return None, None, None, VfrReturnCode.VFR_RETURN_DATA_STRING_ERROR
            else:
                ArrayStr = VarStr[s:e]
                ArrayIdx = int(ArrayStr)
                if VarStr[e] == ']':
                    e += 1
                if e < len(VarStr) and (VarStr[e] == '.'):
                    e += 1
                return ArrayIdx, e, FName, VfrReturnCode.VFR_RETURN_SUCCESS

        elif VarStr[s] == ']':
            return None, None, None, VfrReturnCode.VFR_RETURN_DATA_STRING_ERROR

        return ArrayIdx, s, FName, VfrReturnCode.VFR_RETURN_SUCCESS

    def GetTypeField(self, FName, Type):
        if FName == None or Type == None:
            return None, VfrReturnCode.VFR_RETURN_FATAL_ERROR

        pField = Type.Members
        while (pField != None):

            if Type.Type == EFI_IFR_TYPE_TIME:
                if FName == 'Hour':
                    FName = 'Hours'
                elif FName == 'Minute':
                    FName == 'Minutes'
                elif FName == 'Second':
                    FName = 'Seconds'
            if pField.FieldName == FName:
                Field = pField
                return Field, VfrReturnCode.VFR_RETURN_SUCCESS

            pField = pField.Next

        return None, VfrReturnCode.VFR_RETURN_UNDEFINED

    def IsThisBitField(self, VarStrName):

        TName, i = self.__ExtractStructTypeName(VarStrName)
        pType, ReturnCode = self.GetDataType(TName)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return None, ReturnCode
        pField = None
        while (i < len(VarStrName)):
            # i start from field
            _, i, FName, ReturnCode = self.__ExtractFieldNameAndArrary(
                VarStrName, i)
            if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                return None, ReturnCode
            pField, ReturnCode = self.GetTypeField(FName, pType)
            if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                return None, ReturnCode
            pType = pField.FieldType

        if pField != None and pField.IsBitField:
            return True, ReturnCode
        else:
            return False, ReturnCode

    def GetFieldOffset(self, Field, ArrayIdx, IsBitField):

        if Field == None:
            return None, VfrReturnCode.VFR_RETURN_FATAL_ERROR

        if (ArrayIdx != INVALID_ARRAY_INDEX) and (Field.ArrayNum == 0 or
                                                  Field.ArrayNum <= ArrayIdx):
            return None, VfrReturnCode.VFR_RETURN_ERROR_ARRARY_NUM

        Idx = 0 if ArrayIdx == INVALID_ARRAY_INDEX else ArrayIdx
        if IsBitField:
            Offset = Field.BitOffset + Field.FieldType.TotalSize * Idx * 8
        else:
            Offset = Field.Offset + Field.FieldType.TotalSize * Idx

        return Offset, VfrReturnCode.VFR_RETURN_SUCCESS

    def __GetFieldWidth(self, Field):
        if Field == None:
            return 0
        return Field.FieldType.Type

    def __GetFieldSize(self, Field, ArrayIdx, BitField):
        if Field == None:
            return VfrReturnCode.VFR_RETURN_FATAL_ERROR

        if (ArrayIdx == INVALID_ARRAY_INDEX) and (Field.ArrayNum != 0):
            return Field.FieldType.TotalSize * Field.ArrayNum
        else:

            if BitField:
                return Field.BitWidth
            else:
                return Field.FieldType.TotalSize

    def GetDataFieldInfo(self, VarStr):

        # VarStr -> Type.Field
        Offset = 0
        Type = EFI_IFR_TYPE_OTHER
        Size = 0
        BitField = False

        TName, i = self.__ExtractStructTypeName(VarStr)

        pType, ReturnCode = self.GetDataType(TName)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return Offset, Type, Size, BitField, ReturnCode

        BitField, ReturnCode = self.IsThisBitField(VarStr)

        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return Offset, Type, Size, BitField, ReturnCode

        #　if it is not struct data type
        Type = pType.Type
        Size = pType.TotalSize

        while (i < len(VarStr)):
            ArrayIdx, i, FName, ReturnCode = self.__ExtractFieldNameAndArrary(
                VarStr, i)
            if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                return Offset, Type, Size, BitField, ReturnCode
            pField, ReturnCode = self.GetTypeField(FName, pType)
            if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                return Offset, Type, Size, BitField, ReturnCode
            pType = pField.FieldType
            Tmp, ReturnCode = self.GetFieldOffset(pField, ArrayIdx,
                                                  pField.IsBitField)
            if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
                return Offset, Type, Size, BitField, ReturnCode

            if BitField and pField.IsBitField == False:
                Offset = int(Offset + Tmp * 8)
            else:
                Offset = int(Offset + Tmp)

            Type = self.__GetFieldWidth(pField)
            Size = self.__GetFieldSize(pField, ArrayIdx, BitField)

        return Offset, Type, Size, BitField, VfrReturnCode.VFR_RETURN_SUCCESS

    def __RegisterNewType(self, New):
        New.Next = self.__DataTypeList
        self.__DataTypeList = New
        return

    def DataTypeAddField(self, FieldName, TypeName, ArrayNum, FieldInUnion):

        pFieldType, ReturnCode = self.GetDataType(TypeName)

        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode

        MaxDataTypeSize = self.__NewDataType.TotalSize


        if len(FieldName) >= MAX_NAME_LEN:
            return VfrReturnCode.VFR_RETURN_INVALID_PARAMETER

        pTmp = self.__NewDataType.Members
        while pTmp != None:
            if pTmp.FieldName == FieldName:
                return VfrReturnCode.VFR_RETURN_REDEFINED
            pTmp = pTmp.Next

        Align = min(self.__PackAlign, pFieldType.Align)
        pNewField = SVfrDataField()
        if pNewField == None:
            return VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES
        pNewField.FieldName = FieldName
        pNewField.FieldType = pFieldType
        pNewField.ArrayNum = ArrayNum
        pNewField.IsBitField = False

        if self.__NewDataType.TotalSize % Align == 0:
            pNewField.Offset = self.__NewDataType.TotalSize
        else:
            pNewField.Offset = self.__NewDataType.TotalSize + self.__AlignStuff(
                self.__NewDataType.TotalSize, Align)

        if self.__NewDataType.Members == None:
            self.__NewDataType.Members = pNewField
            pNewField.Next = None
        else:
            pTmp = self.__NewDataType.Members
            while pTmp.Next != None:
                pTmp = pTmp.Next
            pTmp.Next = pNewField
            pNewField.Next = None

        self.__NewDataType.Align = min(
            self.__PackAlign, max(pFieldType.Align, self.__NewDataType.Align))

        if FieldInUnion:
            if MaxDataTypeSize < pNewField.FieldType.TotalSize:
                self.__NewDataType.TotalSize = pNewField.FieldType.TotalSize
            pNewField.Offset = 0
        else:
            Num = ArrayNum if ArrayNum != 0 else 1
            self.__NewDataType.TotalSize = pNewField.Offset + (
                pNewField.FieldType.TotalSize) * Num

        return VfrReturnCode.VFR_RETURN_SUCCESS

    def DataTypeAddBitField(self, FieldName, TypeName, Width, FieldInUnion):

        pFieldType, ReturnCode = self.GetDataType(TypeName)
        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode

        if Width > MAX_BIT_WIDTH:
            return VfrReturnCode.VFR_RETURN_BIT_WIDTH_ERROR

        if Width > (pFieldType.TotalSize) * 8:
            return VfrReturnCode.VFR_RETURN_BIT_WIDTH_ERROR

        if (FieldName != None) and (len(FieldName) >= MAX_NAME_LEN):
            return VfrReturnCode.VFR_RETURN_INVALID_PARAMETER

        if Width == 0 and FieldName != None:
            return VfrReturnCode.VFR_RETURN_INVALID_PARAMETER

        pTmp = self.__NewDataType.Members
        while pTmp != None:
            if FieldName != None and pTmp.FieldName == FieldName:
                return VfrReturnCode.VFR_RETURN_REDEFINED
            pTmp = pTmp.Next

        Align = min(self.__PackAlign, pFieldType.Align)
        UpdateTotalSize = False

        pNewField = SVfrDataField()
        if pNewField == None:
            return VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        MaxDataTypeSize = self.__NewDataType.TotalSize

        pNewField.FieldName = FieldName
        pNewField.FieldType = pFieldType
        pNewField.IsBitField = True
        pNewField.BitWidth = Width
        pNewField.ArrayNum = 0
        pNewField.BitOffset = 0
        pNewField.Offset = 0

        if self.__NewDataType.Members == None:
            self.__NewDataType.Members = pNewField
            pNewField.Next = None
        else:
            pTmp = self.__NewDataType.Members
            while pTmp.Next != None:
                pTmp = pTmp.Next
            pTmp.Next = pNewField
            pNewField.Next = None

        if FieldInUnion:
            pNewField.Offset = 0
            if MaxDataTypeSize < pNewField.FieldType.TotalSize:
                self.__NewDataType.TotalSize = pNewField.FieldType.TotalSize
        else:
            # Check whether the bit fields can be contained within one FieldType.
            cond1 = (pTmp != None) and (pTmp.IsBitField) and (
                pTmp.FieldType.TypeName == pNewField.FieldType.TypeName)
            cond2 = (pTmp != None) and (pTmp.BitOffset - pTmp.Offset * 8 +
                                        pTmp.BitWidth + pNewField.BitWidth <=
                                        pNewField.FieldType.TotalSize * 8)
            if cond1 and cond2:
                pNewField.BitOffset = pTmp.BitOffset + pTmp.BitWidth
                pNewField.Offset = pTmp.Offset

                if pNewField.BitWidth == 0:
                    pNewField.BitWidth = pNewField.FieldType.TotalSize * 8 - (
                        pNewField.BitOffset - pTmp.Offset * 8)
            else:
                pNewField.BitOffset = self.__NewDataType.TotalSize * 8
                UpdateTotalSize = True

        if UpdateTotalSize:
            if self.__NewDataType.TotalSize % Align == 0:
                pNewField.Offset = self.__NewDataType.TotalSize
            else:
                pNewField.Offset = self.__NewDataType.TotalSize + self.__AlignStuff(
                    self.__NewDataType.TotalSize, Align)
            self.__NewDataType.TotalSize = pNewField.Offset + pNewField.FieldType.TotalSize

        self.__NewDataType.Align = min(
            self.__PackAlign, max(pFieldType.Align, self.__NewDataType.Align))
        self.__NewDataType.HasBitField = True
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def GetDataType(self, TypeName):
        if TypeName == None:
            return None, VfrReturnCode.VFR_RETURN_ERROR_SKIPED

        DataType = self.__DataTypeList
        while DataType != None:
            if DataType.TypeName == TypeName:
                return DataType, VfrReturnCode.VFR_RETURN_SUCCESS
            DataType = DataType.Next
        return None, VfrReturnCode.VFR_RETURN_UNDEFINED

    def DataTypeHasBitField(self, TypeName):
        if TypeName == None:
            return False

        pType = self.__DataTypeList
        while pType != None:
            if pType.TypeName == TypeName:
                break
            pType = pType.Next

        if pType == None:
            return False

        pTmp = pType.Members
        while pTmp != None:
            if pTmp.IsBitField:
                return True
            pTmp = pTmp.Next
        return False

    def Dump(self, FileName):
        try:
            with open(FileName, 'w') as f:
                f.write("PackAlign = " + str(self.__PackAlign) + '\n')
                pNode = self.__DataTypeList
                while pNode != None:
                    f.write('struct {} : Align : {}  TotalSize : '.format(str(pNode.TypeName), str(pNode.Align)))
                    f.write('%#x\n'%(pNode.TotalSize))
                    # f.write(" struct " + str(pNode.TypeName) + " : " + " Align " + str(pNode.Align)) + " TotalSize " + str('%#x'%pNode.TotalSize))
                    f.write('struct {} \n'.format(str(pNode.TypeName)))
                    FNode = pNode.Members
                    while(FNode != None):
                        if FNode.ArrayNum > 0:
                            f.write('FieldName : {} , Offset : {}, ArrayNum : {} , FieldTypeName : {} , IsBitField : {} \n '.format(str(FNode.FieldName), str(FNode.Offset), str(FNode.ArrayNum), str(FNode.FieldType.TypeName), str(FNode.IsBitField)))
                        else:
                            f.write('FieldName : {} , Offset : {}, FieldTypeName : {} ,  IsBitField : {} \n '.format(str(FNode.FieldName), str(FNode.Offset), str(FNode.FieldType.TypeName), str(FNode.IsBitField)))
                        FNode = FNode.Next
                    f.write('\n')
                    pNode = pNode.Next
            f.close()
        except IOError as e:
            print("error")
            pass

class SVfrDefaultStoreNode(object):

    def __init__(self,
                 ObjAddr=None,
                 RefName='',
                 DefaultStoreNameId=0,
                 DefaultId=0):
        self.ObjAddr = ObjAddr
        self.RefName = RefName
        self.DefaultStoreNameId = DefaultStoreNameId
        self.DefaultId = DefaultId
        self.Next = None


class CVfrDefaultStore(object):

    def __init__(self):
        self.__DefaultStoreList = None

    def RegisterDefaultStore(self, ObjAddr: EFI_IFR_DEFAULTSTORE, RefName, DefaultStoreNameId, DefaultId):
        if RefName == '' or RefName == None:
            return VfrReturnCode.VFR_RETURN_FATAL_ERROR

        pNode = self.__DefaultStoreList
        while pNode != None:
            if pNode.RefName == RefName:
                return VfrReturnCode.VFR_RETURN_REDEFINED
            pNode = pNode.Next

        pNode = SVfrDefaultStoreNode(ObjAddr, RefName, DefaultStoreNameId, DefaultId)

        if pNode == None:
            return VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        pNode.Next = self.__DefaultStoreList
        self.__DefaultStoreList = pNode

        return VfrReturnCode.VFR_RETURN_SUCCESS

    def DefaultIdRegistered(self, DefaultId):
        pNode = self.__DefaultStoreList
        while pNode != None:
            if pNode.DefaultId == DefaultId:
                return True
            pNode = pNode.Next

        return False

    def ReRegisterDefaultStoreById(self, DefaultId, RefName, DefaultStoreNameId):

        pNode = self.__DefaultStoreList
        while pNode != None:
            if pNode.DefaultId == DefaultId:
                break
            pNode = pNode.Next

        if pNode == None:
            return None, VfrReturnCode.VFR_RETURN_UNDEFINED
        else:
            if pNode.DefaultStoreNameId == EFI_STRING_ID_INVALID:
                pNode.DefaultStoreNameId == DefaultStoreNameId
                pNode.RefName = RefName
                if pNode.ObjAddr != None:
                    pNode.ObjAddr.DefaultName = DefaultStoreNameId
            else:
                return None, VfrReturnCode.VFR_RETURN_REDEFINED

        return pNode, VfrReturnCode.VFR_RETURN_SUCCESS

    def GetDefaultId(self, RefName):
        pTmp = self.__DefaultStoreList
        while(pTmp != None):
            if pTmp.RefName == RefName:
                DefaultId = pTmp.DefaultId
                return DefaultId, VfrReturnCode.VFR_RETURN_SUCCESS
            pTmp = pTmp.Next
        return None, VfrReturnCode.VFR_RETURN_UNDEFINED

    def BufferVarStoreAltConfigAdd(self, DefaultId, BaseInfo, VarStoreName,
                                   VarStoreGuid, Type, Value):
        if VarStoreName == None:
            return VfrReturnCode.VFR_RETURN_FATAL_ERROR
        pNode = self.__DefaultStoreList
        while pNode != None:
            if pNode.DefaultId == DefaultId:
                break
            pNode = pNode.Next
        if pNode == None:
            return VfrReturnCode.VFR_RETURN_UNDEFINED
        # pNode.DefaultId sprintf (NewAltCfg, "%04x", pNode->mDefaultId)
        gCVfrBufferConfig.Open()
        if gCVfrBufferConfig.Select(VarStoreName, VarStoreGuid) == 0:
            Returnvalue = gCVfrBufferConfig.Write('a', VarStoreName,
                                                  VarStoreGuid,
                                                  pNode.DefaultId, Type,
                                                  BaseInfo.Info.VarOffset,
                                                  BaseInfo.VarTotalSize, Value)
            if Returnvalue != 0:
                gCVfrBufferConfig.Close()
                return VfrReturnCode(Returnvalue)
        gCVfrBufferConfig.Close()
        return VfrReturnCode.VFR_RETURN_SUCCESS



class EFI_VFR_VARSTORE_TYPE(Enum):
    EFI_VFR_VARSTORE_INVALID = 0
    EFI_VFR_VARSTORE_BUFFER = 1
    EFI_VFR_VARSTORE_EFI = 2
    EFI_VFR_VARSTORE_NAME = 3
    EFI_VFR_VARSTORE_BUFFER_BITS = 4


class EfiVar():

    def __init__(self, VarName=0, VarSize=0):
        self.EfiVarName = VarName
        self.EfiVarSize = VarSize


DEFAULT_NAME_TABLE_ITEMS = 1024


class SVfrVarStorageNode():

    def __init__(self,
                 VarStoreName='',
                 VarStoreId=0,
                 Guid=None,
                 Attributes=0,
                 Flag=True,
                 EfiValue=None,
                 DataType=None,
                 BitsVarstore=False):

        self.Guid = Guid
        self.VarStoreName = VarStoreName
        self.VarStoreId = VarStoreId
        self.AssignedFlag = Flag
        self.Attributes = Attributes
        self.Next = None
        self.EfiVar = EfiValue
        self.DataType = DataType
        self.NameSpace = []

        if EfiValue != None:
            self.VarstoreType = EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_EFI
        elif DataType != None:
            if BitsVarstore:
                self.VarstoreType = EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER_BITS
            else:
                self.VarstoreType = EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER
        else:
            self.VarstoreType = EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_NAME

class SConfigItem():

    def __init__(self,
                 Name=None,
                 Guid=None,
                 Id=None,
                 Type=None,
                 Offset=None,
                 Width=None,
                 Value=None):
        self.Name = Name  # varstore name
        self.Guid = Guid  # varstore guid, varstore name + guid deside one varstore
        self.Id = Id  # default ID
        if Type != None:
            # list of Offset/Value in the varstore
            self.InfoStrList = SConfigInfo(Type, Offset, Width, Value)
        else:
            self.InfoStrList = None
        self.Next = None


class SConfigInfo():

    def __init__(self, Type, Offset, Width, Value: EFI_IFR_TYPE_VALUE):
        self.Type = Type
        self.Offset = Offset
        self.Width = Width
        self.Next = None
        self.Value = Value

class CVfrBufferConfig(object):
    __metaclass__ = ABCMeta

    def __init__(self):
        self.__ItemListHead = None  # SConfigItem
        self.__ItemListTail = None
        self.__ItemListPos = None

    def GetVarItemList(self):
        return self.__ItemListHead

    @abstractmethod
    def Open(self):
        self.__ItemListPos = self.__ItemListHead

    @abstractmethod
    def Close(self):
        self.__ItemListPos = None

    @abstractmethod
    def Select(self, Name, Guid, Id=None):
        if Name == None or Guid == None:
            self.__ItemListPos = self.__ItemListHead
            return 0
        else:
            p = self.__ItemListHead
            while p != None:
                if p.Name != Name or p.Guid.__cmp__(Guid) == False:
                    p = p.Next
                    continue
                if Id != None:
                    if p.Id == None or p.Id != Id:
                        p = p.Next
                        continue
                elif p.Id != None:
                    p = p.Next
                    continue
                self.__ItemListPos = p
                return 0
        return 1

    @abstractmethod
    def Register(self, Name, Guid, Id=None):
        if self.Select(Name, Guid) == 0:
            return 1
        pNew = SConfigItem(Name, Guid, Id)
        if pNew == None:
            return 2
        if self.__ItemListHead == None:
            self.__ItemListHead = pNew
            self.__ItemListTail = pNew
        else:
            self.__ItemListTail.Next = pNew
            self.__ItemListTail = pNew
        self.__ItemListPos = pNew
        return 0

    @abstractmethod
    def Write(self, Mode, Name, Guid, Id, Type, Offset, Width,
              Value: EFI_IFR_TYPE_VALUE):
        Ret = self.Select(Name, Guid)
        if Ret != 0:
            return Ret

        if Mode == 'a':  # add
            if self.Select(Name, Guid, Id) != 0:
                pItem = SConfigItem(Name, Guid, Id, Type, Offset, Width, Value)
                if pItem == None:
                    return 2

                if self.__ItemListHead == None:
                    self.__ItemListHead = pItem
                    self.__ItemListTail = pItem
                else:
                    self.__ItemListTail.Next = pItem
                    self.__ItemListTail = pItem

                self.__ItemListPos = pItem

            else:
                # tranverse the list to find out if there's already the value for the same offset
                pInfo = self.__ItemListPos.InfoStrList
                while pInfo != None:
                    if pInfo.Offset == Offset:
                        return 0
                    pInfo = pInfo.Next

                pInfo = SConfigInfo(Type, Offset, Width, Value)
                if pInfo == None:
                    return 2

                pInfo.Next = self.__ItemListPos.InfoStrList
                self.__ItemListPos.InfoStrList = pInfo

        elif Mode == 'd':  # delete
            if self.__ItemListHead == self.__ItemListPos:
                self.__ItemListHead = self.__ItemListPos.Next

            pItem = self.__ItemListHead
            while pItem.Next != self.__ItemListPos:
                pItem = pItem.Next
            pItem.Next = self.__ItemListPos.Next

            if self.__ItemListTail == self.__ItemListPos:
                self.__ItemListTail = pItem

            self.__ItemListPos = pItem.Next

        elif Mode == 'i':  # set info
            if Id != None:
                self.__ItemListPos.Id = Id
        else:
            return 1
        return 0

gCVfrBufferConfig = CVfrBufferConfig()

class EFI_VARSTORE_INFO(Structure):
    _pack_ = 1
    _fields_ = [
        ('VarStoreId', c_uint16),
        ('Info', VarStoreInfoNode),
        ('VarType', c_uint8),
        ('VarTotalSize', c_uint32),
        ('IsBitVar', c_bool),
    ]

    def __init__(self,
                 VarStoreId=EFI_VARSTORE_ID_INVALID,
                 VarName=EFI_STRING_ID_INVALID,
                 VarOffset=EFI_VAROFFSET_INVALID,
                 VarType=EFI_IFR_TYPE_OTHER,
                 VarTotalSize=0,
                 IsBitVar=False):

        self.VarStoreId = VarStoreId
        self.Info.VarName = VarName
        self.Info.VarOffset = VarOffset
        self.VarTotalSize = VarTotalSize
        self.IsBitVar = IsBitVar
        self.VarType = VarType


class BufferVarStoreFieldInfoNode():

    def __init__(self, BaseInfo: EFI_VARSTORE_INFO):

        self.VarStoreInfo = EFI_VARSTORE_INFO()
        self.VarStoreInfo.VarType = BaseInfo.VarType
        self.VarStoreInfo.VarTotalSize = BaseInfo.VarTotalSize
        self.VarStoreInfo.Info.VarOffset = BaseInfo.Info.VarOffset
        self.VarStoreInfo.VarStoreId = BaseInfo.VarStoreId
        self.Next = None


class CVfrDataStorage(object):

    def __init__(self):
        self.__BufferVarStoreList = None  # SVfrVarStorageNode
        self.__EfiVarStoreList = None
        self.__NameVarStoreList = None
        self.__CurrVarStorageNode = None
        self.__NewVarStorageNode = None
        self.__BufferFieldInfoListHead = None
        self.__mBufferFieldInfoListTail = None
        self.__FreeVarStoreIdBitMap = []
        for i in range(0, EFI_FREE_VARSTORE_ID_BITMAP_SIZE):
            self.__FreeVarStoreIdBitMap.append(0)
        #Question ID0 is reserved
        self.__FreeVarStoreIdBitMap[0] = 0x80000000

    def GetBufferVarStoreList(self):
        return self.__BufferVarStoreList

    def __CheckGuidField(self, pNode, StoreGuid, HasFoundOne, ReturnCode):
        if StoreGuid != None:
            #　If has guid info, compare the guid filed.
            if pNode.Guid.__cmp__(StoreGuid):
                self.__CurrVarStorageNode = pNode
                ReturnCode = VfrReturnCode.VFR_RETURN_SUCCESS
                return True, ReturnCode, HasFoundOne
        else:
            #　not has Guid field, check whether this name is the only one.
            if HasFoundOne:
                #  The name has conflict, return name redefined.
                ReturnCode = VfrReturnCode.VFR_RETURN_VARSTORE_NAME_REDEFINED_ERROR
                return True, ReturnCode, HasFoundOne

            self.__CurrVarStorageNode = pNode
            HasFoundOne = True
        return False, ReturnCode, HasFoundOne

    def __GetVarStoreByDataType(self, DataTypeName, VarGuid):
        MatchNode = None
        pNode = self.__BufferVarStoreList
        while pNode != None:
            if pNode.DataType.TypeName != DataTypeName:
                pNode = pNode.Next
                continue
            if VarGuid != None:
                if pNode.Guid.__cmp__(VarGuid):
                    return pNode, VfrReturnCode.VFR_RETURN_SUCCESS
            else:
                if MatchNode == None:
                    MatchNode = pNode
                else:
                    # More than one varstores referred the same data structures
                    return None, VfrReturnCode.VFR_RETURN_VARSTORE_DATATYPE_REDEFINED_ERROR
            pNode = pNode.Next

        if MatchNode == None:
            return MatchNode, VfrReturnCode.VFR_RETURN_UNDEFINED

        return MatchNode, VfrReturnCode.VFR_RETURN_SUCCESS

    """
       Base on the input store name and guid to find the varstore id.
       If both name and guid are inputed, base on the name and guid to
       found the varstore. If only name inputed, base on the name to
       found the varstore and go on to check whether more than one varstore
       has the same name. If only has found one varstore, return this
       varstore; if more than one varstore has same name, return varstore
       name redefined error. If no varstore found by varstore name, call
       function GetVarStoreByDataType and use inputed varstore name as
       data type name to search.
       """

    def GetVarStoreId(self, StoreName, StoreGuid=None):

        ReturnCode = None
        HasFoundOne = False
        self.__CurrVarStorageNode = None

        pNode = self.__BufferVarStoreList
        while pNode != None:
            if pNode.VarStoreName == StoreName:
                Result, ReturnCode, HasFoundOne = self.__CheckGuidField(
                    pNode, StoreGuid, HasFoundOne, ReturnCode)
                if Result:
                    VarStoreId = self.__CurrVarStorageNode.VarStoreId
                    return VarStoreId, ReturnCode
            pNode = pNode.Next

        pNode = self.__EfiVarStoreList
        while pNode != None:
            if pNode.VarStoreName == StoreName:
                Result, ReturnCode, HasFoundOne = self.__CheckGuidField(
                    pNode, StoreGuid, HasFoundOne, ReturnCode)
                if Result:
                    VarStoreId = self.__CurrVarStorageNode.VarStoreId
                    return VarStoreId, ReturnCode
            pNode = pNode.Next

        pNode = self.__NameVarStoreList
        while pNode != None:
            if pNode.VarStoreName == StoreName:
                Result, ReturnCode, HasFoundOne = self.__CheckGuidField(
                    pNode, StoreGuid, HasFoundOne, ReturnCode)
                if Result:
                    VarStoreId = self.__CurrVarStorageNode.VarStoreId
                    return VarStoreId, ReturnCode
            pNode = pNode.Next

        if HasFoundOne:
            VarStoreId = self.__CurrVarStorageNode.VarStoreId
            return VarStoreId, VfrReturnCode.VFR_RETURN_SUCCESS

        VarStoreId = EFI_VARSTORE_ID_INVALID
        pNode, ReturnCode = self.__GetVarStoreByDataType(StoreName,
                                                         StoreGuid)  #
        if pNode != None:
            self.__CurrVarStorageNode = pNode
            VarStoreId = pNode.VarStoreId

        return VarStoreId, ReturnCode

    def __GetFreeVarStoreId(self, VarType):

        Index = 0
        for i in range(0, EFI_FREE_VARSTORE_ID_BITMAP_SIZE):
            if self.__FreeVarStoreIdBitMap[i] != 0xFFFFFFFF:
                Index = i
                break
        if Index == EFI_FREE_VARSTORE_ID_BITMAP_SIZE:
            return EFI_VARSTORE_ID_INVALID

        Offset = 0
        Mask = 0x80000000
        while Mask != 0:
            if (self.__FreeVarStoreIdBitMap[Index] & Mask) == 0:
                self.__FreeVarStoreIdBitMap[Index] |= Mask
                return (Index << EFI_BITS_SHIFT_PER_UINT32) + Offset
            Mask >>= 1
            Offset += 1
        return EFI_VARSTORE_ID_INVALID

    def __CheckVarStoreIdFree(self, VarStoreId):

        Index = int(VarStoreId / EFI_BITS_PER_UINT32)
        Offset = VarStoreId % EFI_BITS_PER_UINT32
        return (self.__FreeVarStoreIdBitMap[Index] &
                (0x80000000 >> Offset)) == 0

    def __MarkVarStoreIdUsed(self, VarStoreId):

        Index = int(VarStoreId / EFI_BITS_PER_UINT32)
        Offset = VarStoreId % EFI_BITS_PER_UINT32
        self.__FreeVarStoreIdBitMap[Index] |= (0x80000000 >> Offset)

    def DeclareBufferVarStore(self,
                              StoreName,
                              Guid,
                              DataTypeDB,
                              TypeName,
                              VarStoreId,
                              IsBitVarStore,
                              Attr=0,
                              Flag=True):

        if StoreName == None or Guid == None or DataTypeDB == None:
            return VfrReturnCode.VFR_RETURN_FATAL_ERROR
        _, ReturnCode = self.GetVarStoreId(StoreName, Guid)

        if ReturnCode == VfrReturnCode.VFR_RETURN_SUCCESS:
            return VfrReturnCode.VFR_RETURN_REDEFINED

        DataType, ReturnCode = DataTypeDB.GetDataType(TypeName)

        if ReturnCode != VfrReturnCode.VFR_RETURN_SUCCESS:
            return ReturnCode

        if VarStoreId == EFI_VARSTORE_ID_INVALID:
            VarStoreId = self.__GetFreeVarStoreId(
                EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_BUFFER)
        else:
            if self.__CheckVarStoreIdFree(VarStoreId) == False:
                return VfrReturnCode.VFR_RETURN_VARSTOREID_REDEFINED
            self.__MarkVarStoreIdUsed(VarStoreId)
        pNew = SVfrVarStorageNode(StoreName, VarStoreId, Guid, Attr, Flag, None,
                                  DataType, IsBitVarStore)

        if pNew == None:
            return VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        pNew.Next = self.__BufferVarStoreList
        self.__BufferVarStoreList = pNew

        if gCVfrBufferConfig.Register(StoreName, Guid) != 0:
            return VfrReturnCode.VFR_RETURN_FATAL_ERROR

        return VfrReturnCode.VFR_RETURN_SUCCESS

    def DeclareNameVarStoreBegin(self, StoreName, VarStoreId):
        if StoreName == None:
            return VfrReturnCode.VFR_RETURN_FATAL_ERROR

        _, ReturnCode = self.GetVarStoreId(StoreName)
        if ReturnCode == VfrReturnCode.VFR_RETURN_SUCCESS:
            return VfrReturnCode.VFR_RETURN_REDEFINED

        if VarStoreId == EFI_VARSTORE_ID_INVALID:
            VarStoreId = self.__GetFreeVarStoreId(
                EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_NAME)
        else:
            if self.__CheckVarStoreIdFree(VarStoreId) == False:
                return VfrReturnCode.VFR_RETURN_VARSTOREID_REDEFINED
            self.__MarkVarStoreIdUsed(VarStoreId)

        pNode = SVfrVarStorageNode(StoreName, VarStoreId)

        if pNode == None:
            return VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        self.__NewVarStorageNode = pNode
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def DeclareNameVarStoreEnd(self, Guid):
        self.__NewVarStorageNode.Guid = Guid
        self.__NewVarStorageNode.Next = self.__NameVarStoreList
        self.__NameVarStoreList = self.__NewVarStorageNode
        self.__NewVarStorageNode = None

    def NameTableAddItem(self, Item):
        self.__NewVarStorageNode.NameSpace.append(Item)
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def GetNameVarStoreInfo(self, BaseInfo, Index):
        if BaseInfo == None:
            return VfrReturnCode.VFR_RETURN_FATAL_ERROR

        if self.__CurrVarStorageNode == None:
            return VfrReturnCode.VFR_RETURN_GET_NVVARSTORE_ERROR

        BaseInfo.Info.VarName = self.__CurrVarStorageNode.NameSpace[Index]

        return VfrReturnCode.VFR_RETURN_SUCCESS

    def GetVarStoreType(self, VarStoreId):

        VarStoreType = EFI_VFR_VARSTORE_TYPE.EFI_VFR_VARSTORE_INVALID

        if VarStoreId == EFI_VARSTORE_ID_INVALID:
            return VarStoreType

        pNode = self.__BufferVarStoreList
        while pNode != None:
            if pNode.VarStoreId == VarStoreId:
                VarStoreType = pNode.VarstoreType
                return VarStoreType
            pNode = pNode.Next

        pNode = self.__EfiVarStoreList
        while pNode != None:
            if pNode.VarStoreId == VarStoreId:
                VarStoreType = pNode.VarstoreType
                return VarStoreType
            pNode = pNode.Next

        pNode = self.__NameVarStoreList
        while pNode != None:
            if pNode.VarStoreId == VarStoreId:
                VarStoreType = pNode.VarstoreType
                return VarStoreType
            pNode = pNode.Next

        return VarStoreType

    def GetVarStoreName(self, VarStoreId):

        pNode = self.__BufferVarStoreList
        while pNode != None:
            if pNode.VarStoreId == VarStoreId:
                return pNode.VarStoreName, VfrReturnCode.VFR_RETURN_SUCCESS
            pNode = pNode.Next

        pNode = self.__EfiVarStoreList
        while pNode != None:
            if pNode.VarStoreId == VarStoreId:
                return pNode.VarStoreName, VfrReturnCode.VFR_RETURN_SUCCESS
            pNode = pNode.Next

        pNode = self.__NameVarStoreList
        while pNode != None:
            if pNode.VarStoreId == VarStoreId:
                return pNode.VarStoreName, VfrReturnCode.VFR_RETURN_SUCCESS
            pNode = pNode.Next

        return None, VfrReturnCode.VFR_RETURN_UNDEFINED


    def GetBufferVarStoreDataTypeName(self, VarStoreId):

        DataTypeName = None
        if VarStoreId == EFI_VARSTORE_ID_INVALID:
            return DataTypeName, VfrReturnCode.VFR_RETURN_FATAL_ERROR

        pNode = self.__BufferVarStoreList
        while pNode != None:
            if pNode.VarStoreId == VarStoreId:
                DataTypeName = pNode.DataType.TypeName
                return DataTypeName, VfrReturnCode.VFR_RETURN_SUCCESS
            pNode = pNode.Next

        return DataTypeName, VfrReturnCode.VFR_RETURN_UNDEFINED

    def GetEfiVarStoreInfo(self, BaseInfo: EFI_VARSTORE_INFO):

        if BaseInfo == None:
            return VfrReturnCode.VFR_RETURN_FATAL_ERROR

        if self.__CurrVarStorageNode == None:
            return VfrReturnCode.VFR_RETURN_GET_EFIVARSTORE_ERROR

        BaseInfo.Info.VarName = self.__CurrVarStorageNode.EfiVar.EfiVarName
        BaseInfo.VarTotalSize = self.__CurrVarStorageNode.EfiVar.EfiVarSize

        if BaseInfo.VarTotalSize == 1:
            BaseInfo.VarType = EFI_IFR_TYPE_NUM_SIZE_8
        elif BaseInfo.VarTotalSize == 2:
            BaseInfo.VarType = EFI_IFR_TYPE_NUM_SIZE_16
        elif BaseInfo.VarTotalSize == 4:
            BaseInfo.VarType = EFI_IFR_TYPE_NUM_SIZE_32
        elif BaseInfo.VarTotalSize == 8:
            BaseInfo.VarType = EFI_IFR_TYPE_NUM_SIZE_64
        else:
            return VfrReturnCode.VFR_RETURN_FATAL_ERROR

        return VfrReturnCode.VFR_RETURN_SUCCESS

    def GetVarStoreGuid(self, VarStoreId):

        VarGuid = None
        if VarStoreId == EFI_VARSTORE_ID_INVALID:
            return VarGuid

        pNode = self.__BufferVarStoreList
        while pNode != None:
            if pNode.VarStoreId == VarStoreId:
                VarGuid = pNode.Guid
                return VarGuid
            pNode = pNode.Next

        pNode = self.__EfiVarStoreList
        while pNode != None:
            if pNode.VarStoreId == VarStoreId:
                VarGuid = pNode.Guid
                return VarGuid
            pNode = pNode.Next

        pNode = self.__NameVarStoreList
        while pNode != None:
            if pNode.VarStoreId == VarStoreId:
                VarGuid = pNode.Guid
                return VarGuid
            pNode = pNode.Next

        return VarGuid

    def AddBufferVarStoreFieldInfo(self, BaseInfo: EFI_VARSTORE_INFO):

        pNew = BufferVarStoreFieldInfoNode(BaseInfo)
        if pNew == None:
            return VfrReturnCode.VFR_RETURN_FATAL_ERROR

        if self.__BufferFieldInfoListHead == None:
            self.__BufferFieldInfoListHead = pNew
            self.__mBufferFieldInfoListTail = pNew
        else:
            self.__mBufferFieldInfoListTail.Next = pNew
            self.__mBufferFieldInfoListTail = pNew

        return VfrReturnCode.VFR_RETURN_SUCCESS


class CVfrStringDB(object):

    def __init__(self):
        self.__StringFileName = ''

    def GetVarStoreNameFromStringId(self, StringId):
        if self.__StringFileName == '':
            return None
        try:
            f = open(self.__StringFileName)
            StringPtr = f.read()
            f.close()
        except IOError:
            print('Error')

gCVfrStringDB = CVfrStringDB()

EFI_RULE_ID_START = 0x01
EFI_RULE_ID_INVALID = 0x00


class SVfrRuleNode():

    def __init__(self, RuleName=None, RuleId=0):
        self.RuleId = RuleId
        self.RuleName = RuleName
        self.Next = None


class CVfrRulesDB(object):

    def __init__(self):
        self.__RuleList = None
        self.__FreeRuleId = EFI_VARSTORE_ID_START

    def RegisterRule(self, RuleName):
        if RuleName == None:
            return

        pNew = SVfrRuleNode(RuleName, self.__FreeRuleId)
        if pNew == None: return
        self.__FreeRuleId += 1
        pNew.Next = self.__RuleList
        self.__RuleList = pNew

    def GetRuleId(self, RuleName):
        if RuleName == None:
            return

        pNode = self.__RuleList
        while pNode != None:
            if pNode.RuleName == RuleName:
                return pNode.RuleId
            pNode = pNode.Next

        return EFI_RULE_ID_INVALID


EFI_QUESTION_ID_MAX = 0xFFFF
EFI_FREE_QUESTION_ID_BITMAP_SIZE = int(
    (EFI_QUESTION_ID_MAX + 1) / EFI_BITS_PER_UINT32)
EFI_QUESTION_ID_INVALID = 0x0


class EFI_QUESION_TYPE(Enum):
    QUESTION_NORMAL = 0
    QUESTION_DATE = 1
    QUESTION_TIME = 2
    QUESTION_REF = 3


class SVfrQuestionNode():

    def __init__(self, Name=None, VarIdStr=None, BitMask=0):  #
        self.Name = Name
        self.VarIdStr = VarIdStr
        self.QuestionId = EFI_QUESTION_ID_INVALID
        self.BitMask = BitMask
        self.Next = None
        self.QType = EFI_QUESION_TYPE.QUESTION_NORMAL


DATE_YEAR_BITMASK = 0x0000FFFF
DATE_MONTH_BITMASK = 0x00FF0000
DATE_DAY_BITMASK = 0xFF000000
TIME_HOUR_BITMASK = 0x000000FF
TIME_MINUTE_BITMASK = 0x0000FF00
TIME_SECOND_BITMASK = 0x00FF0000


class CVfrQuestionDB(object):

    def __init__(self):
        self.__FreeQIdBitMap = []
        for i in range(0, EFI_FREE_QUESTION_ID_BITMAP_SIZE):
            self.__FreeQIdBitMap.append(0)

        # Question ID 0 is reserved.
        self.__FreeQIdBitMap[0] = 0x80000000

        self.__QuestionList = None

    def FindQuestionByName(self, Name):
        if Name == None:
            return VfrReturnCode.VFR_RETURN_FATAL_ERROR

        pNode = self.__QuestionList
        while pNode != None:
            if pNode.Name == Name:
                return VfrReturnCode.VFR_RETURN_SUCCESS
            pNode = pNode.Next

        return VfrReturnCode.VFR_RETURN_UNDEFINED

    def FindQuestionById(self, QuestionId):
        if QuestionId == EFI_QUESTION_ID_INVALID:
            return VfrReturnCode.VFR_RETURN_INVALID_PARAMETER

        pNode = self.__QuestionList
        while pNode != None:
            if pNode.QuestionId == QuestionId:
                return VfrReturnCode.VFR_RETURN_SUCCESS
            pNode = pNode.Next

        return VfrReturnCode.VFR_RETURN_UNDEFINED

    def __GetFreeQuestionId(self):

        Index = 0
        for i in range(0, EFI_FREE_QUESTION_ID_BITMAP_SIZE):
            if self.__FreeQIdBitMap[i] != 0xFFFFFFFF:
                Index = i
                break
        if Index == EFI_FREE_QUESTION_ID_BITMAP_SIZE:
            return EFI_QUESTION_ID_INVALID

        Offset = 0
        Mask = 0x80000000
        while Mask != 0:
            if (self.__FreeQIdBitMap[Index] & Mask) == 0:
                self.__FreeQIdBitMap[Index] |= Mask
                return (Index << EFI_BITS_SHIFT_PER_UINT32) + Offset
            Mask >>= 1
            Offset += 1

        return EFI_QUESTION_ID_INVALID

    def __CheckQuestionIdFree(self, QId):
        Index = int(QId / EFI_BITS_PER_UINT32)
        Offset = QId % EFI_BITS_PER_UINT32
        return (self.__FreeQIdBitMap[Index] & (0x80000000 >> Offset)) == 0

    def __MarkQuestionIdUsed(self, QId):

        Index = int(QId / EFI_BITS_PER_UINT32)
        Offset = QId % EFI_BITS_PER_UINT32
        self.__FreeQIdBitMap[Index] |= (0x80000000 >> Offset)

    def __MarkQuestionIdUnused(self, QId):
        Index = int(QId / EFI_BITS_PER_UINT32)
        Offset = QId % EFI_BITS_PER_UINT32
        self.__FreeQIdBitMap[Index] &= ~(0x80000000 >> Offset)

    def RegisterQuestion(self, Name, VarIdStr, QuestionId):

        if (Name != None) and (self.FindQuestionByName(Name) == VfrReturnCode.VFR_RETURN_SUCCESS):
            return QuestionId, VfrReturnCode.VFR_RETURN_REDEFINED

        pNode = SVfrQuestionNode(Name, VarIdStr)
        if pNode == None:
            return QuestionId, VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        if QuestionId == EFI_QUESTION_ID_INVALID:
            QuestionId = self.__GetFreeQuestionId()
        else:
            if self.__CheckQuestionIdFree(QuestionId) == False:
                return QuestionId, VfrReturnCode.VFR_RETURN_QUESTIONID_REDEFINED
            self.__MarkQuestionIdUsed(QuestionId)

        pNode.QuestionId = QuestionId
        pNode.Next = self.__QuestionList
        self.__QuestionList = pNode

        # gCFormPkg.DoPendingAssign

        return QuestionId, VfrReturnCode.VFR_RETURN_SUCCESS

    def UpdateQuestionId(self, QId, NewQId):

        if QId == NewQId:
            # don't update
            return VfrReturnCode.VFR_RETURN_SUCCESS

        if self.__CheckQuestionIdFree(NewQId) == False:
            return VfrReturnCode.VFR_RETURN_REDEFINED

        pNode = self.__QuestionList
        TempList = []
        while pNode != None:
            if pNode.QuestionId == QId:
                TempList.append(pNode)
            pNode = pNode.Next

        if len(TempList) == 0:
            return VfrReturnCode.VFR_RETURN_UNDEFINED

        self.__MarkQuestionIdUnused(QId)

        for pNode in TempList:
            pNode.QuestionId = NewQId

        self.__MarkQuestionIdUsed(NewQId)

        # gCFormPkg.DoPendingAssign
        return VfrReturnCode.VFR_RETURN_SUCCESS

    def GetQuestionId(self, Name, VarIdStr=None, QType=None):

        QuestionId = EFI_QUESTION_ID_INVALID
        BitMask = 0x00000000
        if QType != None:
            QType = EFI_QUESION_TYPE.QUESTION_NORMAL

        if Name == None and VarIdStr == None:
            return QuestionId, BitMask, QType

        pNode = self.__QuestionList
        while pNode != None:

            if Name != None:
                if pNode.Name != Name:
                    pNode = pNode.Next
                    continue

            if VarIdStr != None:
                if pNode.VarIdStr != VarIdStr:
                    pNode = pNode.Next
                    continue

            QuestionId = pNode.QuestionId
            BitMask = pNode.BitMask
            if QType != None:
                QType = pNode.QType
            break

        return QuestionId, BitMask, QType

    def RegisterNewDateQuestion(self, Name, BaseVarId, QuestionId):

        if BaseVarId == '' and Name == None:
            if QuestionId == EFI_QUESTION_ID_INVALID:
                QuestionId = self.__GetFreeQuestionId()
            else:
                if self.__CheckQuestionIdFree(QuestionId) == False:
                    return QuestionId, VfrReturnCode.VFR_RETURN_REDEFINED
                self.__MarkQuestionIdUsed(QuestionId)
            return QuestionId, VfrReturnCode.VFR_RETURN_SUCCESS

        VarIdStrList = []
        if BaseVarId != '':
            VarIdStrList.append(BaseVarId + '.Year')
            VarIdStrList.append(BaseVarId + '.Month')
            VarIdStrList.append(BaseVarId + '.Day')

        else:
            VarIdStrList.append(Name + '.Year')
            VarIdStrList.append(Name + '.Month')
            VarIdStrList.append(Name + '.Day')

        pNodeList = []
        pNode = SVfrQuestionNode(Name, VarIdStrList[0], DATE_YEAR_BITMASK)
        if pNode != None:
            pNodeList.append(pNode)
        else:
            return QuestionId, VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        pNode = SVfrQuestionNode(Name, VarIdStrList[1], DATE_MONTH_BITMASK)
        if pNode != None:
            pNodeList.append(pNode)
        else:
            return QuestionId, VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        pNode = SVfrQuestionNode(Name, VarIdStrList[2], DATE_DAY_BITMASK)
        if pNode != None:
            pNodeList.append(pNode)
        else:
            return QuestionId, VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        if QuestionId == EFI_QUESTION_ID_INVALID:
            QuestionId = self.__GetFreeQuestionId()
        else:
            if self.__CheckQuestionIdFree(QuestionId) == False:
                return QuestionId, VfrReturnCode.VFR_RETURN_REDEFINED
            self.__MarkQuestionIdUsed(QuestionId)

        pNodeList[0].QuestionId = QuestionId
        pNodeList[1].QuestionId = QuestionId
        pNodeList[2].QuestionId = QuestionId
        pNodeList[0].QType = EFI_QUESION_TYPE.QUESTION_DATE
        pNodeList[1].QType = EFI_QUESION_TYPE.QUESTION_DATE
        pNodeList[2].QType = EFI_QUESION_TYPE.QUESTION_DATE
        pNodeList[0].Next = pNodeList[1]
        pNodeList[1].Next = pNodeList[2]
        pNodeList[2].Next = self.__QuestionList
        self.__QuestionList = pNodeList[0]

        # DoPendingAssign
        return QuestionId, VfrReturnCode.VFR_RETURN_SUCCESS

    def RegisterNewTimeQuestion(self, Name, BaseVarId, QuestionId):
        if BaseVarId == '' and Name == None:
            if QuestionId == EFI_QUESTION_ID_INVALID:
                QuestionId = self.__GetFreeQuestionId()
            else:
                if self.__CheckQuestionIdFree(QuestionId) == False:
                    return QuestionId, VfrReturnCode.VFR_RETURN_REDEFINED
                self.__MarkQuestionIdUsed(QuestionId)
            return QuestionId, VfrReturnCode.VFR_RETURN_SUCCESS

        VarIdStrList = []
        if BaseVarId != '':
            VarIdStrList.append(BaseVarId + '.Hour')
            VarIdStrList.append(BaseVarId + '.Minute')
            VarIdStrList.append(BaseVarId + '.Second')

        else:
            VarIdStrList.append(Name + '.Hour')
            VarIdStrList.append(Name + '.Minute')
            VarIdStrList.append(Name + '.Second')

        pNodeList = []
        pNode = SVfrQuestionNode(Name, VarIdStrList[0], TIME_HOUR_BITMASK)
        if pNode != None:
            pNodeList.append(pNode)
        else:
            return QuestionId, VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        pNode = SVfrQuestionNode(Name, VarIdStrList[1], TIME_MINUTE_BITMASK)
        if pNode != None:
            pNodeList.append(pNode)
        else:
            return QuestionId, VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        pNode = SVfrQuestionNode(Name, VarIdStrList[2], TIME_SECOND_BITMASK)
        if pNode != None:
            pNodeList.append(pNode)
        else:
            return QuestionId, VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        if QuestionId == EFI_QUESTION_ID_INVALID:
            QuestionId = self.__GetFreeQuestionId()
        else:
            if self.__CheckQuestionIdFree(QuestionId) == False:
                return QuestionId, VfrReturnCode.VFR_RETURN_REDEFINED
            self.__MarkQuestionIdUsed(QuestionId)

        pNodeList[0].QuestionId = QuestionId
        pNodeList[1].QuestionId = QuestionId
        pNodeList[2].QuestionId = QuestionId
        pNodeList[0].QType = EFI_QUESION_TYPE.QUESTION_TIME
        pNodeList[1].QType = EFI_QUESION_TYPE.QUESTION_TIME
        pNodeList[2].QType = EFI_QUESION_TYPE.QUESTION_TIME
        pNodeList[0].Next = pNodeList[1]
        pNodeList[1].Next = pNodeList[2]
        pNodeList[2].Next = self.__QuestionList
        self.__QuestionList = pNodeList[0]

        # DoPendingAssign
        return QuestionId, VfrReturnCode.VFR_RETURN_SUCCESS

    def RegisterRefQuestion(self, Name, BaseVarId, QuestionId):

        if BaseVarId == '' and Name == None:
            return QuestionId, VfrReturnCode.VFR_RETURN_FATAL_ERROR

        VarIdStrList = []
        if BaseVarId != '':
            VarIdStrList.append(BaseVarId + '.QuestionId')
            VarIdStrList.append(BaseVarId + '.FormId')
            VarIdStrList.append(BaseVarId + '.FormSetGuid')
            VarIdStrList.append(BaseVarId + '.DevicePath')

        else:
            VarIdStrList.append(BaseVarId + '.QuestionId')
            VarIdStrList.append(BaseVarId + '.FormId')
            VarIdStrList.append(BaseVarId + '.FormSetGuid')
            VarIdStrList.append(BaseVarId + '.DevicePath')

        pNodeList = []
        pNode = SVfrQuestionNode(Name, VarIdStrList[0])
        if pNode != None:
            pNodeList.append(pNode)
        else:
            return QuestionId, VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        pNode = SVfrQuestionNode(Name, VarIdStrList[1])
        if pNode != None:
            pNodeList.append(pNode)
        else:
            return QuestionId, VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        pNode = SVfrQuestionNode(Name, VarIdStrList[2])
        if pNode != None:
            pNodeList.append(pNode)
        else:
            return QuestionId, VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        pNode = SVfrQuestionNode(Name, VarIdStrList[3])
        if pNode != None:
            pNodeList.append(pNode)
        else:
            return QuestionId, VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        if QuestionId == EFI_QUESTION_ID_INVALID:
            QuestionId = self.__GetFreeQuestionId()
        else:
            if self.__CheckQuestionIdFree(QuestionId) == False:
                return QuestionId, VfrReturnCode.VFR_RETURN_REDEFINED
            self.__MarkQuestionIdUsed(QuestionId)

        pNodeList[0].QuestionId = QuestionId
        pNodeList[1].QuestionId = QuestionId
        pNodeList[2].QuestionId = QuestionId
        pNodeList[3].QuestionId = QuestionId

        pNodeList[0].QType = EFI_QUESION_TYPE.QUESTION_REF
        pNodeList[1].QType = EFI_QUESION_TYPE.QUESTION_REF
        pNodeList[2].QType = EFI_QUESION_TYPE.QUESTION_REF
        pNodeList[3].QType = EFI_QUESION_TYPE.QUESTION_REF

        pNodeList[0].Next = pNodeList[1]
        pNodeList[1].Next = pNodeList[2]
        pNodeList[2].Next = pNodeList[3]
        pNodeList[3].Next = self.__QuestionList
        self.__QuestionList = pNodeList[0]
        x = self.__QuestionList

        # DoPendingAssign

        return QuestionId, VfrReturnCode.VFR_RETURN_SUCCESS

    def RegisterOldDateQuestion(self, YearVarId, MonthVarId, DayVarId, QuestionId):
        pNodeList = []
        if YearVarId == '' or MonthVarId == '' or DayVarId == '' or YearVarId == None or MonthVarId == None or DayVarId == None:
            return QuestionId, VfrReturnCode.VFR_RETURN_ERROR_SKIPED

        pNode = SVfrQuestionNode(None, YearVarId, DATE_YEAR_BITMASK)
        if pNode != None:
            pNodeList.append(pNode)
        else:
            return EFI_QUESTION_ID_INVALID, VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        pNode = SVfrQuestionNode(None, MonthVarId, DATE_MONTH_BITMASK)
        if pNode != None:
            pNodeList.append(pNode)
        else:
            return EFI_QUESTION_ID_INVALID, VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        pNode = SVfrQuestionNode(None, DayVarId, DATE_DAY_BITMASK)
        if pNode != None:
            pNodeList.append(pNode)
        else:
            return EFI_QUESTION_ID_INVALID, VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        if QuestionId == EFI_QUESTION_ID_INVALID:
            QuestionId = self.__GetFreeQuestionId()
        else:
            if self.__CheckQuestionIdFree(QuestionId) == False:
                return EFI_QUESTION_ID_INVALID, VfrReturnCode.VFR_RETURN_REDEFINED
            self.__MarkQuestionIdUsed(QuestionId)

        pNodeList[0].QuestionId = QuestionId
        pNodeList[1].QuestionId = QuestionId
        pNodeList[2].QuestionId = QuestionId
        pNodeList[0].QType = EFI_QUESION_TYPE.QUESTION_DATE
        pNodeList[1].QType = EFI_QUESION_TYPE.QUESTION_DATE
        pNodeList[2].QType = EFI_QUESION_TYPE.QUESTION_DATE
        pNodeList[0].Next = pNodeList[1]
        pNodeList[1].Next = pNodeList[2]
        pNodeList[2].Next = self.__QuestionList
        self.__QuestionList = pNodeList[0]

        # DoPendingAssign
        return QuestionId, VfrReturnCode.VFR_RETURN_SUCCESS

    def RegisterOldTimeQuestion(self, HourVarId, MinuteVarId, SecondVarId, QuestionId):
        pNodeList = []
        if HourVarId == '' or MinuteVarId == '' or SecondVarId == '' or HourVarId == None or MinuteVarId == None or SecondVarId == None:
            return QuestionId, VfrReturnCode.VFR_RETURN_ERROR_SKIPED

        pNode = SVfrQuestionNode(None, HourVarId, TIME_HOUR_BITMASK)
        if pNode != None:
            pNodeList.append(pNode)
        else:
            return EFI_QUESTION_ID_INVALID, VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        pNode = SVfrQuestionNode(None, MinuteVarId, TIME_MINUTE_BITMASK)
        if pNode != None:
            pNodeList.append(pNode)
        else:
            return EFI_QUESTION_ID_INVALID, VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        pNode = SVfrQuestionNode(None, SecondVarId, TIME_SECOND_BITMASK)
        if pNode != None:
            pNodeList.append(pNode)
        else:
            return EFI_QUESTION_ID_INVALID, VfrReturnCode.VFR_RETURN_OUT_FOR_RESOURCES

        if QuestionId == EFI_QUESTION_ID_INVALID:
            QuestionId = self.__GetFreeQuestionId()
        else:
            if self.__CheckQuestionIdFree(QuestionId) == False:
                return EFI_QUESTION_ID_INVALID, VfrReturnCode.VFR_RETURN_REDEFINED
            self.__MarkQuestionIdUsed(QuestionId)

        pNodeList[0].QuestionId = QuestionId
        pNodeList[1].QuestionId = QuestionId
        pNodeList[2].QuestionId = QuestionId
        pNodeList[0].QType = EFI_QUESION_TYPE.QUESTION_TIME
        pNodeList[1].QType = EFI_QUESION_TYPE.QUESTION_TIME
        pNodeList[2].QType = EFI_QUESION_TYPE.QUESTION_TIME
        pNodeList[0].Next = pNodeList[1]
        pNodeList[1].Next = pNodeList[2]
        pNodeList[2].Next = self.__QuestionList
        self.__QuestionList = pNodeList[0]

        # DoPendingAssign
        return QuestionId, VfrReturnCode.VFR_RETURN_SUCCESS

    def PrintAllQuestion(self, FileName):

        with open(FileName, 'w') as f:
            pNode = self.__QuestionList
            while(pNode != None):

                f.write('Question VarId is {} and QuestionId is '.format(pNode.VarIdStr))
                f.write('%d\n'%(pNode.QuestionId))
                # f.write('%#x\n'%(pNode.QuestionId))
                pNode = pNode.Next

        f.close()