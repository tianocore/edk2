## @file
# Module that encodes and decodes a capsule dependency.
#
# Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
import struct
import json
import sys
import uuid
import re

'''
CapsuleDependency
'''

class OpConvert (object):
    def __init__ (self):
        # Opcode: (OperandSize, PackSize, PackFmt, EncodeConvert, DecodeConvert)
        self._DepexOperations = {0x00:    (16, 16, 's', self.Str2Guid, self.Guid2Str),
                                 0x01:    (4,  1,  'I', self.Str2Uint, self.Uint2Str),
                                 0x02:    (1,  0,  's', self.Str2Utf8, self.Byte2Str),
                                 }

    def Str2Uint (self, Data):
        try:
            Value = int (Data, 16)
        except:
            Message = '{Data} is not a valid integer value.'.format (Data = Data)
            raise ValueError (Message)
        if Value < 0 or Value > 0xFFFFFFFF:
            Message = '{Data} is not an UINT32.'.format (Data = Data)
            raise ValueError (Message)
        return Value

    def Uint2Str (self, Data):
        if Data < 0 or Data > 0xFFFFFFFF:
            Message = '{Data} is not an UINT32.'.format (Data = Data)
            raise ValueError (Message)
        return "0x{Data:08x}".format (Data = Data)

    def Str2Guid (self, Data):
        try:
            Guid = uuid.UUID (Data)
        except:
            Message = '{Data} is not a valid registry format GUID value.'.format (Data = Data)
            raise ValueError (Message)
        return Guid.bytes_le

    def Guid2Str (self, Data):
        try:
            Guid = uuid.UUID (bytes_le = Data)
        except:
            Message = '{Data} is not a valid binary format GUID value.'.format (Data = Data)
            raise ValueError (Message)
        return str (Guid).upper ()

    def Str2Utf8 (self, Data):
        if isinstance (Data, str):
            return Data.encode ('utf-8')
        else:
            Message = '{Data} is not a valid string.'.format (Data = Data)
            raise ValueError (Message)

    def Byte2Str (self, Data):
        if isinstance (Data, bytes):
            if Data[-1:] == b'\x00':
                return str (Data[:-1], 'utf-8')
            else:
                return str (Data, 'utf-8')
        else:
            Message = '{Data} is not a valid binary string.'.format (Data = Data)
            raise ValueError (Message)

    def OpEncode (self, Opcode, Operand = None):
        BinTemp = struct.pack ('<b', Opcode)
        if Opcode <= 0x02 and Operand != None:
            OperandSize, PackSize, PackFmt, EncodeConvert, DecodeConvert = self._DepexOperations[Opcode]
            Value = EncodeConvert (Operand)
            if Opcode == 0x02:
                PackSize = len (Value) + 1
            BinTemp += struct.pack ('<{PackSize}{PackFmt}'.format (PackSize = PackSize, PackFmt = PackFmt), Value)
        return BinTemp

    def OpDecode (self, Buffer):
        Opcode = struct.unpack ('<b', Buffer[0:1])[0]
        if Opcode <= 0x02:
            OperandSize, PackSize, PackFmt, EncodeConvert, DecodeConvert = self._DepexOperations[Opcode]
            if Opcode == 0x02:
                try:
                    PackSize = Buffer[1:].index (b'\x00') + 1
                    OperandSize = PackSize
                except:
                    Message = 'CapsuleDependency: OpConvert: error: decode failed with wrong opcode/string.'
                    raise ValueError (Message)
            try:
                Operand = DecodeConvert (struct.unpack ('<{PackSize}{PackFmt}'.format (PackSize = PackSize, PackFmt = PackFmt), Buffer[1:1+OperandSize])[0])
            except:
                Message = 'CapsuleDependency: OpConvert: error: decode failed with unpack failure.'
                raise ValueError (Message)
        else:
            Operand = None
            OperandSize = 0
        return (Opcode, Operand, OperandSize)

class CapsuleDependencyClass (object):
    # //**************************************************************
    # // Image Attribute - Dependency
    # //**************************************************************
    # typedef struct {
    #   UINT8 Dependencies[];
    # } EFI_FIRMWARE_IMAGE_DEP

    # {expression operator : [precedence, opcode, type (1:unary/2:binocular)]}
    _opReference = {'&&':  [2, 0x03, 2],
                    '||':  [1, 0x04, 2],
                    '~':   [5, 0x05, 1],
                    '==':  [3, 0x08, 2],
                    '>':   [4, 0x09, 2],
                    '>=':  [4, 0x0A, 2],
                    '<':   [4, 0x0B, 2],
                    '<=':  [4, 0x0C, 2],
                    }

    def __init__ (self):
        self.Payload              = b''
        self._DepexExp            = None
        self._DepexList           = []
        self._DepexDump           = []
        self.Depex                = b''
        self._Valid               = False
        self._DepexSize           = 0
        self._opReferenceReverse  = {v[1] : k for k, v in self._opReference.items ()}
        self.OpConverter          = OpConvert ()

    @property
    def DepexExp (self):
        return self._DepexExp

    @DepexExp.setter
    def DepexExp (self, DepexExp = ''):
        if isinstance (DepexExp, str):
            DepexExp = re.sub (r'\n',r' ',DepexExp)
            DepexExp = re.sub (r'\(',r' ( ',DepexExp)
            DepexExp = re.sub (r'\)',r' ) ',DepexExp)
            DepexExp = re.sub (r'~',r' ~ ',DepexExp)
            self._DepexList = re.findall(r"[^\s\"\']+|\"[^\"]*\"|\'[^\']*\'",DepexExp)
            self._DepexExp  = " ".join(self._DepexList)

        else:
            Msg = 'Input Depex Expression is not valid string.'
            raise ValueError (Msg)

    def IsValidOperator (self, op):
        return op in self._opReference.keys ()

    def IsValidUnaryOperator (self, op):
        return op in self._opReference.keys () and self._opReference[op][2] == 1

    def IsValidBinocularOperator (self, op):
        return op in self._opReference.keys () and self._opReference[op][2] == 2

    def IsValidGuid (self, operand):
        try:
            uuid.UUID (operand)
        except:
            return False
        return True

    def IsValidVersion (self, operand):
        try:
            Value = int (operand, 16)
            if Value < 0 or Value > 0xFFFFFFFF:
                return False
        except:
            return False
        return True

    def IsValidBoolean (self, operand):
        try:
            return operand.upper () in ['TRUE', 'FALSE']
        except:
            return False

    def IsValidOperand (self, operand):
        return self.IsValidVersion (operand) or self.IsValidGuid (operand) or self.IsValidBoolean (operand)

    def IsValidString (self, operand):
        return operand[0] == "\"" and operand[-1] == "\"" and len(operand) >= 2

    # Check if priority of current operater is greater than pervious op
    def PriorityNotGreater (self, prevOp, currOp):
        return self._opReference[currOp][0] <= self._opReference[prevOp][0]

    def ValidateDepex (self):
        OpList = self._DepexList

        i = 0
        while i < len (OpList):
            Op = OpList[i]

            if Op == 'DECLARE':
                i += 1
                if i >= len (OpList):
                    Msg = 'No more Operand after {Op}.'.format (Op = OpList[i-1])
                    raise IndexError (Msg)
                # Check valid string
                if not self.IsValidString(OpList[i]):
                    Msg = '{Operand} after {Op} is not a valid expression input.'.format (Operand = OpList[i], Op = OpList[i-1])
                    raise ValueError (Msg)

            elif Op == '(':
                # Expression cannot end with (
                if i == len (OpList) - 1:
                    Msg = 'Expression cannot end with \'(\''
                    raise ValueError (Msg)
                # The previous op after '(' cannot be a binocular operator
                if self.IsValidBinocularOperator (OpList[i+1]) :
                    Msg = '{Op} after \'(\' is not a valid expression input.'.format (Op = OpList[i+1])
                    raise ValueError (Msg)

            elif Op == ')':
                # Expression cannot start with )
                if i == 0:
                    Msg = 'Expression cannot start with \')\''
                    raise ValueError (Msg)
                # The previous op before ')' cannot be an operator
                if self.IsValidOperator (OpList[i-1]):
                    Msg = '{Op} before \')\' is not a valid expression input.'.format (Op = OpList[i-1])
                    raise ValueError (Msg)
                # The next op after ')' cannot be operand or unary operator
                if (i + 1) < len (OpList) and (self.IsValidOperand (OpList[i+1]) or self.IsValidUnaryOperator (OpList[i+1])):
                    Msg = '{Op} after \')\' is not a valid expression input.'.format (Op = OpList[i+1])
                    raise ValueError (Msg)

            elif self.IsValidOperand (Op):
                # The next expression of operand cannot be operand or unary operator
                if (i + 1) < len (OpList) and (self.IsValidOperand (OpList[i+1]) or self.IsValidUnaryOperator (OpList[i+1])):
                    Msg = '{Op} after {PrevOp} is not a valid expression input.'.format (Op = OpList[i+1], PrevOp = Op)
                    raise ValueError (Msg)

            elif self.IsValidOperator (Op):
                # The next op of operator cannot binocular operator
                if (i + 1) < len (OpList) and self.IsValidBinocularOperator (OpList[i+1]):
                    Msg = '{Op} after {PrevOp} is not a valid expression input.'.format (Op = OpList[i+1], PrevOp = Op)
                    raise ValueError (Msg)
                # The first op can not be binocular operator
                if i == 0 and self.IsValidBinocularOperator (Op):
                    Msg = 'Expression cannot start with an operator {Op}.'.format (Op = Op)
                    raise ValueError (Msg)
                # The last op can not be operator
                if i == len (OpList) - 1:
                    Msg = 'Expression cannot ended with an operator {Op}.'.format (Op = Op)
                    raise ValueError (Msg)
                # The next op of unary operator cannot be guid / version
                if self.IsValidUnaryOperator (Op) and (self.IsValidGuid (OpList[i+1]) or self.IsValidVersion (OpList[i+1])):
                    Msg = '{Op} after {PrevOp} is not a valid expression input.'.format (Op = OpList[i+1], PrevOp = Op)
                    raise ValueError (Msg)

            else:
                Msg = '{Op} is not a valid expression input.'.format (Op = Op)
                raise ValueError (Msg)
            i += 1

    def Encode (self):
        # initialize
        self.Depex = b''
        self._DepexDump = []
        OperandStack = []
        OpeartorStack = []
        OpList = self._DepexList

        self.ValidateDepex ()

        # convert
        i = 0
        while i < len (OpList):
            Op = OpList[i]
            if Op == 'DECLARE':
                # This declare next expression value is a VERSION_STRING
                i += 1
                self.Depex += self.OpConverter.OpEncode (0x02, OpList[i][1:-1])

            elif Op == '(':
                OpeartorStack.append (Op)

            elif Op == ')':
                while (OpeartorStack and OpeartorStack[-1] != '('):
                    Operator = OpeartorStack.pop ()
                    self.Depex += self.OpConverter.OpEncode (self._opReference[Operator][1])
                try:
                    OpeartorStack.pop () # pop out '('
                except:
                    Msg = 'Pop out \'(\' failed, too many \')\''
                    raise ValueError (Msg)

            elif self.IsValidGuid (Op):
                if not OperandStack:
                    OperandStack.append (self.OpConverter.OpEncode (0x00, Op))
                else:
                    # accroding to uefi spec 2.8, the guid/version operands is a reversed order in firmware comparison.
                    self.Depex += self.OpConverter.OpEncode (0x00, Op)
                    self.Depex += OperandStack.pop ()

            elif self.IsValidVersion (Op):
                if not OperandStack:
                    OperandStack.append (self.OpConverter.OpEncode (0x01, Op))
                else:
                    # accroding to uefi spec 2.8, the guid/version operands is a reversed order in firmware comparison.
                    self.Depex += self.OpConverter.OpEncode (0x01, Op)
                    self.Depex += OperandStack.pop ()

            elif self.IsValidBoolean (Op):
                if Op.upper () == 'FALSE':
                    self.Depex += self.OpConverter.OpEncode (0x07)
                elif Op.upper () == 'TRUE':
                    self.Depex += self.OpConverter.OpEncode (0x06)

            elif self.IsValidOperator (Op):
                while (OpeartorStack and OpeartorStack[-1] != '(' and self.PriorityNotGreater (OpeartorStack[-1], Op)):
                    Operator = OpeartorStack.pop ()
                    self.Depex += self.OpConverter.OpEncode (self._opReference[Operator][1])
                OpeartorStack.append (Op)

            i += 1

        while OpeartorStack:
            Operator = OpeartorStack.pop ()
            if Operator == '(':
                Msg = 'Too many \'(\'.'
                raise ValueError (Msg)
            self.Depex += self.OpConverter.OpEncode (self._opReference[Operator][1])
        self.Depex += self.OpConverter.OpEncode (0x0D)

        self._Valid = True
        self._DepexSize = len (self.Depex)
        return self.Depex + self.Payload

    def Decode (self, Buffer):
        # initialize
        self.Depex = Buffer
        OperandStack = []
        DepexLen = 0

        while True:
            Opcode, Operand, OperandSize = self.OpConverter.OpDecode (Buffer[DepexLen:])
            DepexLen += OperandSize + 1

            if Opcode == 0x0D:
                break

            elif Opcode == 0x02:
                if not OperandStack:
                    OperandStack.append ('DECLARE \"{String}\"'.format (String = Operand))
                else:
                    PrevOperand = OperandStack.pop ()
                    OperandStack.append ('{Operand} DECLARE \"{String}\"'.format (Operand = PrevOperand, String = Operand))

            elif Opcode in [0x00, 0x01]:
                OperandStack.append (Operand)

            elif Opcode == 0x06:
                OperandStack.append ('TRUE')

            elif Opcode == 0x07:
                OperandStack.append ('FALSE')

            elif self.IsValidOperator (self._opReferenceReverse[Opcode]):
                Operator = self._opReferenceReverse[Opcode]
                if self.IsValidUnaryOperator (self._opReferenceReverse[Opcode]) and len (OperandStack) >= 1:
                    Oprand = OperandStack.pop ()
                    OperandStack.append (' ( {Operator} {Oprand} )'.format (Operator = Operator, Oprand = Oprand))
                elif self.IsValidBinocularOperator (self._opReferenceReverse[Opcode]) and len (OperandStack) >= 2:
                    Oprand1 = OperandStack.pop ()
                    Oprand2 = OperandStack.pop ()
                    OperandStack.append (' ( {Oprand1} {Operator} {Oprand2} )'.format (Operator = Operator, Oprand1 = Oprand1, Oprand2 = Oprand2))
                else:
                    Msg = 'No enough Operands for {Opcode:02X}.'.format (Opcode = Opcode)
                    raise ValueError (Msg)

            else:
                Msg = '{Opcode:02X} is not a valid OpCode.'.format (Opcode = Opcode)
                raise ValueError (Msg)

        self.DepexExp = OperandStack[0].strip (' ')
        self.Payload = Buffer[DepexLen:]
        self._Valid = True
        self._DepexSize = DepexLen
        return self.Payload


    def DumpInfo (self):
        DepexLen = 0
        Opcode = None
        Buffer = self.Depex

        if self._Valid == True:
            print ('EFI_FIRMWARE_IMAGE_DEP.Dependencies = {')
            while Opcode != 0x0D:
                Opcode, Operand, OperandSize = self.OpConverter.OpDecode (Buffer[DepexLen:])
                DepexLen += OperandSize + 1
                if Operand:
                    print ('    {Opcode:02X}, {Operand},'.format (Opcode = Opcode, Operand = Operand))
                else:
                    print ('    {Opcode:02X},'.format (Opcode = Opcode))
            print ('}')

            print ('sizeof (EFI_FIRMWARE_IMAGE_DEP.Dependencies)    = {Size:08X}'.format (Size = self._DepexSize))
            print ('sizeof (Payload)                                = {Size:08X}'.format (Size = len (self.Payload)))
