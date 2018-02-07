# Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
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
from struct import pack,unpack
import collections
import copy
from Common.VariableAttributes import VariableAttributes
from Common.Misc import *
import collections

var_info = collections.namedtuple("uefi_var", "pcdindex,pcdname,defaultstoragename,skuname,var_name, var_guid, var_offset,var_attribute,pcd_default_value, default_value, data_type")
NvStorageHeaderSize = 28
VariableHeaderSize = 32

def StringArrayToList(StringArray):
    StringArray = StringArray[1:-1]
    StringArray = '[' + StringArray + ']'
    return eval(StringArray)

def PackGUID(Guid):
    GuidBuffer = pack('=LHHBBBBBBBB',
                int(Guid[0], 16),
                int(Guid[1], 16),
                int(Guid[2], 16),
                int(Guid[3][-4:-2], 16),
                int(Guid[3][-2:], 16),
                int(Guid[4][-12:-10], 16),
                int(Guid[4][-10:-8], 16),
                int(Guid[4][-8:-6], 16),
                int(Guid[4][-6:-4], 16),
                int(Guid[4][-4:-2], 16),
                int(Guid[4][-2:], 16)
                )
    return GuidBuffer

class VariableMgr(object):
    def __init__(self, DefaultStoreMap,SkuIdMap):
        self.VarInfo = []
        self.DefaultStoreMap = DefaultStoreMap
        self.SkuIdMap = SkuIdMap
        self.VpdRegionSize = 0
        self.VpdRegionOffset = 0
        self.NVHeaderBuff = None
        self.VarDefaultBuff = None
        self.VarDeltaBuff = None

    def append_variable(self,uefi_var):
        self.VarInfo.append(uefi_var)

    def SetVpdRegionMaxSize(self,maxsize):
        self.VpdRegionSize = maxsize

    def SetVpdRegionOffset(self,vpdoffset):
        self.VpdRegionOffset = vpdoffset

    def PatchNVStoreDefaultMaxSize(self,maxsize):
        if not self.NVHeaderBuff:
            return ""
        self.NVHeaderBuff = self.NVHeaderBuff[:8] + pack("=Q",maxsize)
        default_var_bin = self.format_data(self.NVHeaderBuff + self.VarDefaultBuff + self.VarDeltaBuff)
        value_str = "{"
        default_var_bin_strip = [ data.strip("""'""") for data in default_var_bin]
        value_str += ",".join(default_var_bin_strip)
        value_str += "}"
        return value_str
    def combine_variable(self):
        indexedvarinfo = collections.OrderedDict()
        for item in self.VarInfo:
            if (item.skuname,item.defaultstoragename, item.var_name,item.var_guid) not in indexedvarinfo:
                indexedvarinfo[(item.skuname,item.defaultstoragename, item.var_name,item.var_guid) ] = []
            indexedvarinfo[(item.skuname,item.defaultstoragename, item.var_name,item.var_guid)].append(item)
        for key in indexedvarinfo:
            sku_var_info_offset_list = indexedvarinfo[key]
            if len(sku_var_info_offset_list) == 1:
                continue
            newvalue = {}
            for item in sku_var_info_offset_list:
                data_type = item.data_type
                value_list = item.default_value.strip("{").strip("}").split(",")
                if data_type in ["BOOLEAN","UINT8","UINT16","UINT32","UINT64"]:
                    if data_type == ["BOOLEAN","UINT8"]:
                        data_flag = "=B"
                    elif data_type == "UINT16":
                        data_flag = "=H"
                    elif data_type == "UINT32":
                        data_flag = "=L"
                    elif data_type == "UINT64":
                        data_flag = "=Q"
                    data = value_list[0]
                    value_list = []
                    for data_byte in pack(data_flag,int(data,16) if data.upper().startswith('0X') else int(data)):
                        value_list += [hex(unpack("B",data_byte)[0])]
                newvalue[int(item.var_offset,16) if item.var_offset.upper().startswith("0X") else int(item.var_offset)] = value_list
            try:
                newvaluestr = "{" + ",".join(self.assemble_variable(newvalue)) +"}"
            except:
                EdkLogger.error("build", AUTOGEN_ERROR, "Variable offset conflict in PCDs: %s \n" % (" and ".join([item.pcdname for item in sku_var_info_offset_list])))
            n = sku_var_info_offset_list[0]
            indexedvarinfo[key] =  [var_info(n.pcdindex,n.pcdname,n.defaultstoragename,n.skuname,n.var_name, n.var_guid, "0x00",n.var_attribute,newvaluestr  , newvaluestr , "VOID*")]
        self.VarInfo = [item[0] for item in indexedvarinfo.values()]

    def assemble_variable(self, valuelist):
        ordered_value = [valuelist[k] for k in sorted(valuelist.keys())]
        ordered_offset = sorted(valuelist.keys())
        var_value = []
        num = 0
        for offset in ordered_offset:
            if offset < len(var_value):
                raise
            for _ in xrange(offset - len(var_value)):
                var_value.append('0x00')
            var_value += ordered_value[num]
            num +=1
        return var_value
    def process_variable_data(self):

        var_data = dict()

        indexedvarinfo = collections.OrderedDict()
        for item in self.VarInfo:
            if item.pcdindex not in indexedvarinfo:
                indexedvarinfo[item.pcdindex] = dict()
            indexedvarinfo[item.pcdindex][(item.skuname,item.defaultstoragename)] = item

        for index in indexedvarinfo:
            sku_var_info = indexedvarinfo[index]

            default_data_buffer = ""
            others_data_buffer = ""
            tail = None
            default_sku_default = indexedvarinfo.get(index).get(("DEFAULT","STANDARD"))

            if default_sku_default.data_type not in ["UINT8","UINT16","UINT32","UINT64","BOOLEAN"]:
                var_max_len = max([len(var_item.default_value.split(",")) for var_item in sku_var_info.values()])
                if len(default_sku_default.default_value.split(",")) < var_max_len:
                    tail = ",".join([ "0x00" for i in range(var_max_len-len(default_sku_default.default_value.split(",")))])

            default_data_buffer = self.PACK_VARIABLES_DATA(default_sku_default.default_value,default_sku_default.data_type,tail)

            default_data_array = ()
            for item in default_data_buffer:
                default_data_array += unpack("B",item)

            if ("DEFAULT","STANDARD") not in var_data:
                var_data[("DEFAULT","STANDARD")] = collections.OrderedDict()
            var_data[("DEFAULT","STANDARD")][index] = (default_data_buffer,sku_var_info[("DEFAULT","STANDARD")])

            for (skuid,defaultstoragename) in indexedvarinfo.get(index):
                tail = None
                if (skuid,defaultstoragename) == ("DEFAULT","STANDARD"):
                    continue
                other_sku_other = indexedvarinfo.get(index).get((skuid,defaultstoragename))

                if default_sku_default.data_type not in ["UINT8","UINT16","UINT32","UINT64","BOOLEAN"]:
                    if len(other_sku_other.default_value.split(",")) < var_max_len:
                        tail = ",".join([ "0x00" for i in range(var_max_len-len(other_sku_other.default_value.split(",")))])

                others_data_buffer = self.PACK_VARIABLES_DATA(other_sku_other.default_value,other_sku_other.data_type,tail)

                others_data_array = ()
                for item in others_data_buffer:
                    others_data_array += unpack("B",item)

                data_delta = self.calculate_delta(default_data_array, others_data_array)

                if (skuid,defaultstoragename) not in var_data:
                    var_data[(skuid,defaultstoragename)] = collections.OrderedDict()
                var_data[(skuid,defaultstoragename)][index] = (data_delta,sku_var_info[(skuid,defaultstoragename)])
        return var_data

    def new_process_varinfo(self):
        self.combine_variable()

        var_data = self.process_variable_data()

        if not var_data:
            return []

        pcds_default_data = var_data.get(("DEFAULT","STANDARD"),{})
        NvStoreDataBuffer = ""
        var_data_offset = collections.OrderedDict()
        offset = NvStorageHeaderSize
        for default_data,default_info in pcds_default_data.values():
            var_name_buffer = self.PACK_VARIABLE_NAME(default_info.var_name)

            vendorguid = default_info.var_guid.split('-')

            if default_info.var_attribute:
                var_attr_value,_ = VariableAttributes.GetVarAttributes(default_info.var_attribute)
            else:
                var_attr_value = 0x07

            DataBuffer = self.AlignData(var_name_buffer + default_data)

            data_size = len(DataBuffer)
            offset += VariableHeaderSize + len(default_info.var_name.split(","))
            var_data_offset[default_info.pcdindex] = offset
            offset += data_size - len(default_info.var_name.split(","))

            var_header_buffer = self.PACK_VARIABLE_HEADER(var_attr_value, len(default_info.var_name.split(",")), len (default_data), vendorguid)
            NvStoreDataBuffer += (var_header_buffer + DataBuffer)

        variable_storage_header_buffer = self.PACK_VARIABLE_STORE_HEADER(len(NvStoreDataBuffer) + 28)

        nv_default_part = self.AlignData(self.PACK_DEFAULT_DATA(0, 0, self.unpack_data(variable_storage_header_buffer+NvStoreDataBuffer)), 8)

        data_delta_structure_buffer = ""
        for skuname,defaultstore in var_data:
            if (skuname,defaultstore) == ("DEFAULT","STANDARD"):
                continue
            pcds_sku_data = var_data.get((skuname,defaultstore))
            delta_data_set = []
            for pcdindex in pcds_sku_data:
                offset = var_data_offset[pcdindex]
                delta_data,_ = pcds_sku_data[pcdindex]
                delta_data = [(item[0] + offset, item[1]) for item in delta_data]
                delta_data_set.extend(delta_data)

            data_delta_structure_buffer += self.AlignData(self.PACK_DELTA_DATA(skuname,defaultstore,delta_data_set), 8)

        size = len(nv_default_part + data_delta_structure_buffer) + 16
        maxsize = self.VpdRegionSize if self.VpdRegionSize else size
        NV_Store_Default_Header = self.PACK_NV_STORE_DEFAULT_HEADER(size,maxsize)

        self.NVHeaderBuff =  NV_Store_Default_Header
        self.VarDefaultBuff =nv_default_part
        self.VarDeltaBuff =  data_delta_structure_buffer
        return self.format_data(NV_Store_Default_Header + nv_default_part + data_delta_structure_buffer)


    def format_data(self,data):

        return  [hex(item) for item in self.unpack_data(data)]

    def unpack_data(self,data):
        final_data = ()
        for item in data:
            final_data += unpack("B",item)
        return final_data

    def calculate_delta(self, default, theother):
        if len(default) - len(theother) != 0:
            EdkLogger.error("build", FORMAT_INVALID, 'The variable data length is not the same for the same PCD.')
        data_delta = []
        for i in range(len(default)):
            if default[i] != theother[i]:
                data_delta.append((i,theother[i]))
        return data_delta

    def dump(self):

        default_var_bin = self.new_process_varinfo()
        if default_var_bin:
            value_str = "{"
            default_var_bin_strip = [ data.strip("""'""") for data in default_var_bin]
            value_str += ",".join(default_var_bin_strip)
            value_str += "}"
            return value_str
        return ""

    def PACK_VARIABLE_STORE_HEADER(self,size):
        #Signature: gEfiVariableGuid
        Guid = "{ 0xddcf3616, 0x3275, 0x4164, { 0x98, 0xb6, 0xfe, 0x85, 0x70, 0x7f, 0xfe, 0x7d }}"
        Guid = GuidStructureStringToGuidString(Guid)
        GuidBuffer = PackGUID(Guid.split('-'))

        SizeBuffer = pack('=L',size)
        FormatBuffer = pack('=B',0x5A)
        StateBuffer = pack('=B',0xFE)
        reservedBuffer = pack('=H',0)
        reservedBuffer += pack('=L',0)

        return GuidBuffer + SizeBuffer + FormatBuffer + StateBuffer + reservedBuffer

    def PACK_NV_STORE_DEFAULT_HEADER(self,size,maxsize):
        Signature = pack('=B',ord('N'))
        Signature += pack("=B",ord('S'))
        Signature += pack("=B",ord('D'))
        Signature += pack("=B",ord('B'))

        SizeBuffer = pack("=L",size)
        MaxSizeBuffer = pack("=Q",maxsize)

        return Signature + SizeBuffer + MaxSizeBuffer

    def PACK_VARIABLE_HEADER(self,attribute,namesize,datasize,vendorguid):

        Buffer = pack('=H',0x55AA) # pack StartID
        Buffer += pack('=B',0x3F)  # pack State
        Buffer += pack('=B',0)     # pack reserved

        Buffer += pack('=L',attribute)
        Buffer += pack('=L',namesize)
        Buffer += pack('=L',datasize)

        Buffer += PackGUID(vendorguid)

        return Buffer

    def PACK_VARIABLES_DATA(self, var_value,data_type, tail = None):
        Buffer = ""
        data_len = 0
        if data_type == "VOID*":
            for value_char in var_value.strip("{").strip("}").split(","):
                Buffer += pack("=B",int(value_char,16))
            data_len += len(var_value.split(","))
            if tail:
                for value_char in tail.split(","):
                    Buffer += pack("=B",int(value_char,16))
                data_len += len(tail.split(","))
        elif data_type == "BOOLEAN":
            Buffer += pack("=B",True) if var_value.upper() == "TRUE" else pack("=B",False)
            data_len += 1
        elif data_type  == "UINT8":
            Buffer += pack("=B",GetIntegerValue(var_value))
            data_len += 1
        elif data_type == "UINT16":
            Buffer += pack("=H",GetIntegerValue(var_value))
            data_len += 2
        elif data_type == "UINT32":
            Buffer += pack("=L",GetIntegerValue(var_value))
            data_len += 4
        elif data_type == "UINT64":
            Buffer += pack("=Q",GetIntegerValue(var_value))
            data_len += 8

        return Buffer

    def PACK_DEFAULT_DATA(self, defaultstoragename,skuid,var_value):
        Buffer = ""
        Buffer += pack("=L",4+8+8)
        Buffer += pack("=Q",int(skuid))
        Buffer += pack("=Q",int(defaultstoragename))

        for item in var_value:
            Buffer += pack("=B",item)

        Buffer = pack("=L",len(Buffer)+4) + Buffer

        return Buffer

    def GetSkuId(self,skuname):
        if skuname not in self.SkuIdMap:
            return None
        return self.SkuIdMap.get(skuname)[0]
    def GetDefaultStoreId(self,dname):
        if dname not in self.DefaultStoreMap:
            return None
        return self.DefaultStoreMap.get(dname)[0]
    def PACK_DELTA_DATA(self,skuname,defaultstoragename,delta_list):
        skuid = self.GetSkuId(skuname)
        defaultstorageid = self.GetDefaultStoreId(defaultstoragename)
        Buffer = ""
        Buffer += pack("=L",4+8+8)
        Buffer += pack("=Q",int(skuid))
        Buffer += pack("=Q",int(defaultstorageid))
        for (delta_offset,value) in delta_list:
            Buffer += pack("=L",delta_offset)
            Buffer = Buffer[:-1] + pack("=B",value)

        Buffer = pack("=L",len(Buffer) + 4) + Buffer

        return Buffer

    def AlignData(self,data, align = 4):
        mybuffer = data
        if (len(data) % align) > 0:
            for i in range(align - (len(data) % align)):
                mybuffer += pack("=B",0)

        return mybuffer

    def PACK_VARIABLE_NAME(self, var_name):
        Buffer = ""
        for name_char in var_name.strip("{").strip("}").split(","):
            Buffer += pack("=B",int(name_char,16))

        return Buffer
