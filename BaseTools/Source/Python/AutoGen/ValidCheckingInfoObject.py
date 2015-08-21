# Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

#
# This file is used to collect the Variable checking information
#

# #
# Import Modules
#
import os
from Common.RangeExpression import RangeExpression
from Common.Misc import *
from StringIO import StringIO
from struct import pack

class VAR_CHECK_PCD_VARIABLE_TAB_CONTAINER(object):
    def __init__(self):
        self.var_check_info = []
        
    def push_back(self, var_check_tab):
        for tab in self.var_check_info:
            if tab.equal(var_check_tab):
                tab.merge(var_check_tab)
                break
        else:
            self.var_check_info.append(var_check_tab)
    
    def dump(self, dest, Phase):
        
        FormatMap = {}
        FormatMap[1] = "=B"
        FormatMap[2] = "=H"
        FormatMap[4] = "=L"
        FormatMap[8] = "=Q"
        
        if not os.path.isabs(dest):
            return
        if not os.path.exists(dest):
            os.mkdir(dest)
        BinFileName = "PcdVarCheck.bin"
        BinFilePath = os.path.join(dest, BinFileName)
        Buffer = ''
        index = 0
        for var_check_tab in self.var_check_info:
            index += 1
            realLength = 0
            realLength += 32
            Name = var_check_tab.Name[1:-1]
            NameChars = Name.split(",")
            realLength += len(NameChars)
            if (index < len(self.var_check_info) and realLength % 4) or (index == len(self.var_check_info) and len(var_check_tab.validtab) > 0 and realLength % 4):
                realLength += (4 - (realLength % 4))
            itemIndex = 0
            for item in var_check_tab.validtab:
                itemIndex += 1
                realLength += 5
                for v_data in item.data:
                    if type(v_data) in (int, long):
                        realLength += item.StorageWidth
                    else:
                        realLength += item.StorageWidth
                        realLength += item.StorageWidth
                if (index == len(self.var_check_info)) :
                    if (itemIndex < len(var_check_tab.validtab)) and realLength % 4:
                        realLength += (4 - (realLength % 4))
                else:
                    if realLength % 4:
                        realLength += (4 - (realLength % 4))
            var_check_tab.Length = realLength
        realLength = 0
        index = 0
        for var_check_tab in self.var_check_info:
            index += 1

            b = pack("=H", var_check_tab.Revision)
            Buffer += b
            realLength += 2

            b = pack("=H", var_check_tab.HeaderLength)
            Buffer += b
            realLength += 2

            b = pack("=L", var_check_tab.Length)
            Buffer += b
            realLength += 4

            b = pack("=B", var_check_tab.Type)
            Buffer += b
            realLength += 1

            for i in range(0, 3):
                b = pack("=B", var_check_tab.Reserved)
                Buffer += b
                realLength += 1

            b = pack("=L", var_check_tab.Attributes)
            Buffer += b
            realLength += 4

            Guid = var_check_tab.Guid
            b = pack('=LHHBBBBBBBB',
                Guid[0],
                Guid[1],
                Guid[2],
                Guid[3],
                Guid[4],
                Guid[5],
                Guid[6],
                Guid[7],
                Guid[8],
                Guid[9],
                Guid[10],
                )
            Buffer += b
            realLength += 16

            Name = var_check_tab.Name[1:-1]
            NameChars = Name.split(",")
            for NameChar in NameChars:
                NameCharNum = int(NameChar, 16)
                b = pack("=B", NameCharNum)
                Buffer += b
                realLength += 1

            if (index < len(self.var_check_info) and realLength % 4) or (index == len(self.var_check_info) and len(var_check_tab.validtab) > 0 and realLength % 4):
                for i in range(4 - (realLength % 4)):
                    b = pack("=B", var_check_tab.pad)
                    Buffer += b
                    realLength += 1
            itemIndex = 0
            for item in var_check_tab.validtab:
                itemIndex += 1

                b = pack("=B", item.Type)
                Buffer += b
                realLength += 1

                b = pack("=B", item.Length)
                Buffer += b
                realLength += 1

                b = pack("=H", int(item.VarOffset, 16))
                Buffer += b
                realLength += 2

                b = pack("=B", item.StorageWidth)
                Buffer += b
                realLength += 1
                for v_data in item.data:
                    if type(v_data) in (int, long):
                        b = pack(FormatMap[item.StorageWidth], v_data)
                        Buffer += b
                        realLength += item.StorageWidth
                    else:
                        b = pack(FormatMap[item.StorageWidth], v_data[0])
                        Buffer += b
                        realLength += item.StorageWidth
                        b = pack(FormatMap[item.StorageWidth], v_data[1])
                        Buffer += b
                        realLength += item.StorageWidth

                if (index == len(self.var_check_info)) :
                    if (itemIndex < len(var_check_tab.validtab)) and realLength % 4:
                        for i in range(4 - (realLength % 4)):
                            b = pack("=B", var_check_tab.pad)
                            Buffer += b
                            realLength += 1
                else:
                    if realLength % 4:
                        for i in range(4 - (realLength % 4)):
                            b = pack("=B", var_check_tab.pad)
                            Buffer += b
                            realLength += 1
        
        DbFile = StringIO()
        if Phase == 'DXE' and os.path.exists(BinFilePath):
            BinFile = open(BinFilePath, "rb")
            BinBuffer = BinFile.read()
            BinFile.close()
            BinBufferSize = len(BinBuffer)
            if (BinBufferSize % 4):
                for i in range(4 - (BinBufferSize % 4)):
                    b = pack("=B", VAR_CHECK_PCD_VARIABLE_TAB.pad)
                    BinBuffer += b
            Buffer = BinBuffer + Buffer
        DbFile.write(Buffer)
        SaveFileOnChange(BinFilePath, DbFile.getvalue(), True)
    

class VAR_CHECK_PCD_VARIABLE_TAB(object):
    pad = 0xDA
    def __init__(self, TokenSpaceGuid, PcdCName):
        self.Revision = 0x0001
        self.HeaderLength = 0
        self.Length = 0  # Length include this header
        self.Type = 0
        self.Reserved = 0
        self.Attributes = 0x00000000
        self.Guid = eval("[" + TokenSpaceGuid.replace("{", "").replace("}", "") + "]")
        self.Name = PcdCName
        self.validtab = []

    def UpdateSize(self):
        self.HeaderLength = 32 + len(self.Name.split(","))
        self.Length = 32 + len(self.Name.split(",")) + self.GetValidTabLen()
    
    def GetValidTabLen(self):
        validtablen = 0
        for item in self.validtab:
            validtablen += item.Length  
        return validtablen 
    
    def SetAttributes(self, attributes):
        self.Attributes = attributes
            
    def push_back(self, valid_obj):
        if valid_obj is not None:
            self.validtab.append(valid_obj)
        
    def equal(self, varchecktab):
        if self.Guid == varchecktab.Guid and self.Name == varchecktab.Name:
            return True
        else:
            return False
        
    def merge(self, varchecktab):
        for validobj in varchecktab.validtab:
            if validobj in self.validtab:
                continue
            self.validtab.append(validobj)
        self.UpdateSize()


class VAR_CHECK_PCD_VALID_OBJ(object):
    def __init__(self, VarOffset, data, PcdDataType):
        self.Type = 1
        self.Length = 0  # Length include this header
        self.VarOffset = VarOffset
        self.StorageWidth = 0
        self.PcdDataType = PcdDataType.strip()
        self.rawdata = data
        self.data = set()
        self.ValidData = True
        self.updateStorageWidth()
    def updateStorageWidth(self):
        if self.PcdDataType == "UINT8" or self.PcdDataType == "BOOLEAN":
            self.StorageWidth = 1
        elif self.PcdDataType == "UINT16":
            self.StorageWidth = 2
        elif self.PcdDataType == "UINT32":
            self.StorageWidth = 4
        elif self.PcdDataType == "UINT64":
            self.StorageWidth = 8
        else:
            self.StorageWidth = 0
            self.ValidData = False
            
    def __eq__(self, validObj):       
        if self.VarOffset == validObj.VarOffset:
            return True
        else:
            return False
         
class VAR_CHECK_PCD_VALID_LIST(VAR_CHECK_PCD_VALID_OBJ):
    def __init__(self, VarOffset, validlist, PcdDataType):
        super(VAR_CHECK_PCD_VALID_LIST, self).__init__(VarOffset, validlist, PcdDataType)
        self.Type = 1
        self.update_data()
        self.update_size()
    def update_data(self):
        valid_num_list = []
        data_list = []
        for item in self.rawdata:
            valid_num_list.extend(item.split(','))
        
        for valid_num in valid_num_list:
            valid_num = valid_num.strip()

            if valid_num.startswith('0x') or valid_num.startswith('0X'):
                data_list.append(int(valid_num, 16))
            else:
                data_list.append(int(valid_num))

                
        self.data = set(data_list)
        
    def update_size(self):
        self.Length = 5 + len(self.data) * self.StorageWidth
        
           
class VAR_CHECK_PCD_VALID_RANGE(VAR_CHECK_PCD_VALID_OBJ):
    def __init__(self, VarOffset, validrange, PcdDataType):
        super(VAR_CHECK_PCD_VALID_RANGE, self).__init__(VarOffset, validrange, PcdDataType)
        self.Type = 2
        self.update_data()
        self.update_size()
    def update_data(self):
        RangeExpr = ""
        data_list = []
        i = 0
        for item in self.rawdata:
            if i == 0:
                RangeExpr = "( " + item + " )"
            else:
                RangeExpr = RangeExpr + "OR ( " + item + " )"
        range_result = RangeExpression(RangeExpr, self.PcdDataType)(True)
        for rangelist in range_result:
            for obj in rangelist.pop():
                data_list.append((obj.start, obj.end))
        self.data = set(data_list)
    
    def update_size(self):
        self.Length = 5 + len(self.data) * 2 * self.StorageWidth
        

class VAR_VALID_OBJECT_FACTORY(object):
    def __init__(self):
        pass
    @staticmethod
    def Get_valid_object(PcdClass, VarOffset):
        if PcdClass.validateranges:
            return VAR_CHECK_PCD_VALID_RANGE(VarOffset, PcdClass.validateranges, PcdClass.DatumType)
        if PcdClass.validlists:
            return VAR_CHECK_PCD_VALID_LIST(VarOffset, PcdClass.validlists, PcdClass.DatumType)
        else:
            return None

if __name__ == "__main__":
    class TestObj(object):
        def __init__(self, number1):
            self.number_1 = number1
        def __eq__(self, testobj):
            if self.number_1 == testobj.number_1:
                return True
            else:
                return False
    test1 = TestObj(1)
    test2 = TestObj(2)
    
    testarr = [test1, test2]
    print TestObj(2) in testarr
    print TestObj(2) == test2
    
