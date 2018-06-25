# Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
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
from struct import pack, unpack
import collections
import copy
from Common.VariableAttributes import VariableAttributes
from Common.Misc import *
import collections
import Common.DataType as DataType

var_info = collections.namedtuple("uefi_var", "pcdindex,pcdname,defaultstoragename,skuname,var_name, var_guid, var_offset,var_attribute,pcd_default_value, default_value, data_type")
NvStorageHeaderSize = 28
VariableHeaderSize = 32

class VariableMgr(object):
    def __init__(self, DefaultStoreMap, SkuIdMap):
        self.VarInfo = []
        self.DefaultStoreMap = DefaultStoreMap
        self.SkuIdMap = SkuIdMap
        self.VpdRegionSize = 0
        self.VpdRegionOffset = 0
        self.NVHeaderBuff = None
        self.VarDefaultBuff = None
        self.VarDeltaBuff = None

    def append_variable(self, uefi_var):
        self.VarInfo.append(uefi_var)

    def SetVpdRegionMaxSize(self, maxsize):
        self.VpdRegionSize = maxsize

    def SetVpdRegionOffset(self, vpdoffset):
        self.VpdRegionOffset = vpdoffset

    def PatchNVStoreDefaultMaxSize(self, maxsize):
        if not self.NVHeaderBuff:
            return ""
        self.NVHeaderBuff = self.NVHeaderBuff[:8] + pack("=Q", maxsize)
        default_var_bin = VariableMgr.format_data(self.NVHeaderBuff + self.VarDefaultBuff + self.VarDeltaBuff)
        value_str = "{"
        default_var_bin_strip = [ data.strip("""'""") for data in default_var_bin]
        value_str += ",".join(default_var_bin_strip)
        value_str += "}"
        return value_str
    def combine_variable(self):
        indexedvarinfo = collections.OrderedDict()
        for item in self.VarInfo:
            if (item.skuname, item.defaultstoragename, item.var_name, item.var_guid) not in indexedvarinfo:
                indexedvarinfo[(item.skuname, item.defaultstoragename, item.var_name, item.var_guid) ] = []
            indexedvarinfo[(item.skuname, item.defaultstoragename, item.var_name, item.var_guid)].append(item)
        for key in indexedvarinfo:
            sku_var_info_offset_list = indexedvarinfo[key]
            if len(sku_var_info_offset_list) == 1:
                continue
            newvalue = {}
            for item in sku_var_info_offset_list:
                data_type = item.data_type
                value_list = item.default_value.strip("{").strip("}").split(",")
                if data_type in DataType.TAB_PCD_NUMERIC_TYPES:
                    data_flag = DataType.PACK_CODE_BY_SIZE[MAX_SIZE_TYPE[data_type]]
                    data = value_list[0]
                    value_list = []
                    for data_byte in pack(data_flag, int(data, 16) if data.upper().startswith('0X') else int(data)):
                        value_list.append(hex(unpack("B", data_byte)[0]))
                newvalue[int(item.var_offset, 16) if item.var_offset.upper().startswith("0X") else int(item.var_offset)] = value_list
            try:
                newvaluestr = "{" + ",".join(VariableMgr.assemble_variable(newvalue)) +"}"
            except:
                EdkLogger.error("build", AUTOGEN_ERROR, "Variable offset conflict in PCDs: %s \n" % (" and ".join(item.pcdname for item in sku_var_info_offset_list)))
            n = sku_var_info_offset_list[0]
            indexedvarinfo[key] =  [var_info(n.pcdindex, n.pcdname, n.defaultstoragename, n.skuname, n.var_name, n.var_guid, "0x00", n.var_attribute, newvaluestr, newvaluestr, DataType.TAB_VOID)]
        self.VarInfo = [item[0] for item in indexedvarinfo.values()]

    @staticmethod
    def assemble_variable(valuedict):
        ordered_valuedict_keys = sorted(valuedict.keys())
        var_value = []
        for current_valuedict_key in ordered_valuedict_keys:
            if current_valuedict_key < len(var_value):
                raise
            for _ in xrange(current_valuedict_key - len(var_value)):
                var_value.append('0x00')
            var_value += valuedict[current_valuedict_key]
        return var_value

    def process_variable_data(self):

        var_data = collections.defaultdict(collections.OrderedDict)

        indexedvarinfo = collections.OrderedDict()
        for item in self.VarInfo:
            if item.pcdindex not in indexedvarinfo:
                indexedvarinfo[item.pcdindex] = dict()
            indexedvarinfo[item.pcdindex][(item.skuname, item.defaultstoragename)] = item

        for index in indexedvarinfo:
            sku_var_info = indexedvarinfo[index]

            default_data_buffer = ""
            others_data_buffer = ""
            tail = None
            default_sku_default = indexedvarinfo[index].get((DataType.TAB_DEFAULT, DataType.TAB_DEFAULT_STORES_DEFAULT))

            if default_sku_default.data_type not in DataType.TAB_PCD_NUMERIC_TYPES:
                var_max_len = max(len(var_item.default_value.split(",")) for var_item in sku_var_info.values())
                if len(default_sku_default.default_value.split(",")) < var_max_len:
                    tail = ",".join("0x00" for i in range(var_max_len-len(default_sku_default.default_value.split(","))))

            default_data_buffer = VariableMgr.PACK_VARIABLES_DATA(default_sku_default.default_value, default_sku_default.data_type, tail)

            default_data_array = ()
            for item in default_data_buffer:
                default_data_array += unpack("B", item)

            var_data[(DataType.TAB_DEFAULT, DataType.TAB_DEFAULT_STORES_DEFAULT)][index] = (default_data_buffer, sku_var_info[(DataType.TAB_DEFAULT, DataType.TAB_DEFAULT_STORES_DEFAULT)])

            for (skuid, defaultstoragename) in indexedvarinfo[index]:
                tail = None
                if (skuid, defaultstoragename) == (DataType.TAB_DEFAULT, DataType.TAB_DEFAULT_STORES_DEFAULT):
                    continue
                other_sku_other = indexedvarinfo[index][(skuid, defaultstoragename)]

                if default_sku_default.data_type not in DataType.TAB_PCD_NUMERIC_TYPES:
                    if len(other_sku_other.default_value.split(",")) < var_max_len:
                        tail = ",".join("0x00" for i in range(var_max_len-len(other_sku_other.default_value.split(","))))

                others_data_buffer = VariableMgr.PACK_VARIABLES_DATA(other_sku_other.default_value, other_sku_other.data_type, tail)

                others_data_array = ()
                for item in others_data_buffer:
                    others_data_array += unpack("B", item)

                data_delta = VariableMgr.calculate_delta(default_data_array, others_data_array)

                var_data[(skuid, defaultstoragename)][index] = (data_delta, sku_var_info[(skuid, defaultstoragename)])
        return var_data

    def new_process_varinfo(self):
        self.combine_variable()

        var_data = self.process_variable_data()

        if not var_data:
            return []

        pcds_default_data = var_data.get((DataType.TAB_DEFAULT, DataType.TAB_DEFAULT_STORES_DEFAULT), {})
        NvStoreDataBuffer = ""
        var_data_offset = collections.OrderedDict()
        offset = NvStorageHeaderSize
        for default_data, default_info in pcds_default_data.values():
            var_name_buffer = VariableMgr.PACK_VARIABLE_NAME(default_info.var_name)

            vendorguid = default_info.var_guid.split('-')

            if default_info.var_attribute:
                var_attr_value, _ = VariableAttributes.GetVarAttributes(default_info.var_attribute)
            else:
                var_attr_value = 0x07

            DataBuffer = VariableMgr.AlignData(var_name_buffer + default_data)

            data_size = len(DataBuffer)
            offset += VariableHeaderSize + len(default_info.var_name.split(","))
            var_data_offset[default_info.pcdindex] = offset
            offset += data_size - len(default_info.var_name.split(","))

            var_header_buffer = VariableMgr.PACK_VARIABLE_HEADER(var_attr_value, len(default_info.var_name.split(",")), len (default_data), vendorguid)
            NvStoreDataBuffer += (var_header_buffer + DataBuffer)

        variable_storage_header_buffer = VariableMgr.PACK_VARIABLE_STORE_HEADER(len(NvStoreDataBuffer) + 28)

        nv_default_part = VariableMgr.AlignData(VariableMgr.PACK_DEFAULT_DATA(0, 0, VariableMgr.unpack_data(variable_storage_header_buffer+NvStoreDataBuffer)), 8)

        data_delta_structure_buffer = ""
        for skuname, defaultstore in var_data:
            if (skuname, defaultstore) == (DataType.TAB_DEFAULT, DataType.TAB_DEFAULT_STORES_DEFAULT):
                continue
            pcds_sku_data = var_data[(skuname, defaultstore)]
            delta_data_set = []
            for pcdindex in pcds_sku_data:
                offset = var_data_offset[pcdindex]
                delta_data, _ = pcds_sku_data[pcdindex]
                delta_data = [(item[0] + offset, item[1]) for item in delta_data]
                delta_data_set.extend(delta_data)

            data_delta_structure_buffer += VariableMgr.AlignData(self.PACK_DELTA_DATA(skuname, defaultstore, delta_data_set), 8)

        size = len(nv_default_part + data_delta_structure_buffer) + 16
        maxsize = self.VpdRegionSize if self.VpdRegionSize else size
        NV_Store_Default_Header = VariableMgr.PACK_NV_STORE_DEFAULT_HEADER(size, maxsize)

        self.NVHeaderBuff =  NV_Store_Default_Header
        self.VarDefaultBuff =nv_default_part
        self.VarDeltaBuff =  data_delta_structure_buffer
        return VariableMgr.format_data(NV_Store_Default_Header + nv_default_part + data_delta_structure_buffer)


    @staticmethod
    def format_data(data):
        return  [hex(item) for item in VariableMgr.unpack_data(data)]

    @staticmethod
    def unpack_data(data):
        final_data = ()
        for item in data:
            final_data += unpack("B", item)
        return final_data

    @staticmethod
    def calculate_delta(default, theother):
        if len(default) - len(theother) != 0:
            EdkLogger.error("build", FORMAT_INVALID, 'The variable data length is not the same for the same PCD.')
        data_delta = []
        for i in range(len(default)):
            if default[i] != theother[i]:
                data_delta.append((i, theother[i]))
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

    @staticmethod
    def PACK_VARIABLE_STORE_HEADER(size):
        #Signature: gEfiVariableGuid
        Guid = "{ 0xddcf3616, 0x3275, 0x4164, { 0x98, 0xb6, 0xfe, 0x85, 0x70, 0x7f, 0xfe, 0x7d }}"
        Guid = GuidStructureStringToGuidString(Guid)
        GuidBuffer = PackGUID(Guid.split('-'))

        SizeBuffer = pack('=L', size)
        FormatBuffer = pack('=B', 0x5A)
        StateBuffer = pack('=B', 0xFE)
        reservedBuffer = pack('=H', 0)
        reservedBuffer += pack('=L', 0)

        return GuidBuffer + SizeBuffer + FormatBuffer + StateBuffer + reservedBuffer

    @staticmethod
    def PACK_NV_STORE_DEFAULT_HEADER(size, maxsize):
        Signature = pack('=B', ord('N'))
        Signature += pack("=B", ord('S'))
        Signature += pack("=B", ord('D'))
        Signature += pack("=B", ord('B'))

        SizeBuffer = pack("=L", size)
        MaxSizeBuffer = pack("=Q", maxsize)

        return Signature + SizeBuffer + MaxSizeBuffer

    @staticmethod
    def PACK_VARIABLE_HEADER(attribute, namesize, datasize, vendorguid):

        Buffer = pack('=H', 0x55AA) # pack StartID
        Buffer += pack('=B', 0x3F)  # pack State
        Buffer += pack('=B', 0)     # pack reserved

        Buffer += pack('=L', attribute)
        Buffer += pack('=L', namesize)
        Buffer += pack('=L', datasize)

        Buffer += PackGUID(vendorguid)

        return Buffer

    @staticmethod
    def PACK_VARIABLES_DATA(var_value,data_type, tail = None):
        Buffer = ""
        data_len = 0
        if data_type == DataType.TAB_VOID:
            for value_char in var_value.strip("{").strip("}").split(","):
                Buffer += pack("=B", int(value_char, 16))
            data_len += len(var_value.split(","))
            if tail:
                for value_char in tail.split(","):
                    Buffer += pack("=B", int(value_char, 16))
                data_len += len(tail.split(","))
        elif data_type == "BOOLEAN":
            Buffer += pack("=B", True) if var_value.upper() == "TRUE" else pack("=B", False)
            data_len += 1
        elif data_type  == DataType.TAB_UINT8:
            Buffer += pack("=B", GetIntegerValue(var_value))
            data_len += 1
        elif data_type == DataType.TAB_UINT16:
            Buffer += pack("=H", GetIntegerValue(var_value))
            data_len += 2
        elif data_type == DataType.TAB_UINT32:
            Buffer += pack("=L", GetIntegerValue(var_value))
            data_len += 4
        elif data_type == DataType.TAB_UINT64:
            Buffer += pack("=Q", GetIntegerValue(var_value))
            data_len += 8

        return Buffer

    @staticmethod
    def PACK_DEFAULT_DATA(defaultstoragename, skuid, var_value):
        Buffer = ""
        Buffer += pack("=L", 4+8+8)
        Buffer += pack("=Q", int(skuid))
        Buffer += pack("=Q", int(defaultstoragename))

        for item in var_value:
            Buffer += pack("=B", item)

        Buffer = pack("=L", len(Buffer)+4) + Buffer

        return Buffer

    def GetSkuId(self, skuname):
        if skuname not in self.SkuIdMap:
            return None
        return self.SkuIdMap.get(skuname)[0]

    def GetDefaultStoreId(self, dname):
        if dname not in self.DefaultStoreMap:
            return None
        return self.DefaultStoreMap.get(dname)[0]

    def PACK_DELTA_DATA(self, skuname, defaultstoragename, delta_list):
        skuid = self.GetSkuId(skuname)
        defaultstorageid = self.GetDefaultStoreId(defaultstoragename)
        Buffer = ""
        Buffer += pack("=L", 4+8+8)
        Buffer += pack("=Q", int(skuid))
        Buffer += pack("=Q", int(defaultstorageid))
        for (delta_offset, value) in delta_list:
            Buffer += pack("=L", delta_offset)
            Buffer = Buffer[:-1] + pack("=B", value)

        Buffer = pack("=L", len(Buffer) + 4) + Buffer

        return Buffer

    @staticmethod
    def AlignData(data, align = 4):
        mybuffer = data
        if (len(data) % align) > 0:
            for i in range(align - (len(data) % align)):
                mybuffer += pack("=B", 0)

        return mybuffer

    @staticmethod
    def PACK_VARIABLE_NAME(var_name):
        Buffer = ""
        for name_char in var_name.strip("{").strip("}").split(","):
            Buffer += pack("=B", int(name_char, 16))

        return Buffer
