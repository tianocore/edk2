from ctypes import *
from re import A, X
from telnetlib import X3PAD
from tkinter import YView
import uuid
from VfrError import *
from struct import *


EFI_STRING_ID_INVALID = 0x0
EFI_HII_DEFAULT_CLASS_STANDARD = 0x0000
EFI_HII_DEFAULT_CLASS_MANUFACTURING = 0x0001
EFI_IFR_MAX_LENGTH = 0xFF
EFI_VARSTORE_ID_INVALID = 0
EFI_VARSTORE_ID_START = 0x20
EFI_VAROFFSET_INVALID = 0xFFFF
EFI_IMAGE_ID_INVALID = 0xFFFF

EFI_NON_DEVICE_CLASS = 0x00
EFI_DISK_DEVICE_CLASS = 0x01
EFI_VIDEO_DEVICE_CLASS = 0x02
EFI_NETWORK_DEVICE_CLASS = 0x04
EFI_INPUT_DEVICE_CLASS = 0x08
EFI_ON_BOARD_DEVICE_CLASS = 0x10
EFI_OTHER_DEVICE_CLASS = 0x20

EFI_SETUP_APPLICATION_SUBCLASS = 0x00
EFI_GENERAL_APPLICATION_SUBCLASS = 0x01
EFI_FRONT_PAGE_SUBCLASS = 0x02
EFI_SINGLE_USE_SUBCLASS = 0x03

# EFI_HII_PACKAGE_TYPE_x.

EFI_HII_PACKAGE_TYPE_ALL = 0x00
EFI_HII_PACKAGE_TYPE_GUID = 0x01
EFI_HII_PACKAGE_FORM = 0x02
EFI_HII_PACKAGE_KEYBOARD_LAYOUT = 0x03
EFI_HII_PACKAGE_STRINGS = 0x04
EFI_HII_PACKAGE_FONTS = 0x05
EFI_HII_PACKAGE_IMAGES = 0x06
EFI_HII_PACKAGE_SIMPLE_FONTS = 0x07
EFI_HII_PACKAGE_DEVICE_PATH = 0x08
EFI_HII_PACKAGE_END = 0xDF
EFI_HII_PACKAGE_TYPE_SYSTEM_BEGIN = 0xE0
EFI_HII_PACKAGE_TYPE_SYSTEM_END = 0xFF

EFI_IFR_EXTEND_OP_LABEL = 0x0
EFI_IFR_EXTEND_OP_BANNER = 0x1
EFI_IFR_EXTEND_OP_TIMEOUT = 0x2
EFI_IFR_EXTEND_OP_CLASS = 0x3
EFI_IFR_EXTEND_OP_SUBCLASS = 0x4

MAX_IFR_EXPRESSION_DEPTH = 0x9

INVALID_ARRAY_INDEX = 0xFFFFFFFF

EFI_IFR_TYPE_NUM_SIZE_8 = 0x00
EFI_IFR_TYPE_NUM_SIZE_16 = 0x01
EFI_IFR_TYPE_NUM_SIZE_32 = 0x02
EFI_IFR_TYPE_NUM_SIZE_64 = 0x03
EFI_IFR_TYPE_BOOLEAN = 0x04
EFI_IFR_TYPE_TIME = 0x05
EFI_IFR_TYPE_DATE = 0x06
EFI_IFR_TYPE_STRING = 0x07
EFI_IFR_TYPE_OTHER = 0x08
EFI_IFR_TYPE_UNDEFINED = 0x09
EFI_IFR_TYPE_ACTION = 0x0A
EFI_IFR_TYPE_BUFFER = 0x0B
EFI_IFR_TYPE_REF = 0x0C


EFI_IFR_FLAGS_HORIZONTAL = 0x01

EFI_IFR_FLAG_READ_ONLY = 0x01
EFI_IFR_FLAG_CALLBACK = 0x04
EFI_IFR_FLAG_RESET_REQUIRED = 0x10
EFI_IFR_FLAG_REST_STYLE = 0x20
EFI_IFR_FLAG_RECONNECT_REQUIRED = 0x40
EFI_IFR_FLAG_OPTIONS_ONLY = 0x80

EFI_IFR_STRING_MULTI_LINE = 0x01

EDKII_IFR_DISPLAY_BIT = 0xC0
EDKII_IFR_DISPLAY_INT_DEC_BIT = 0x00
EDKII_IFR_DISPLAY_UINT_DEC_BIT = 0x40
EDKII_IFR_DISPLAY_UINT_HEX_BIT = 0x80

EDKII_IFR_NUMERIC_SIZE_BIT = 0x3F

EFI_IFR_MAX_DEFAULT_TYPE = 0x10

EFI_HII_SIBT_END = 0x00
EFI_HII_SIBT_STRING_SCSU = 0x10
EFI_HII_SIBT_STRING_SCSU_FONT = 0x11
EFI_HII_SIBT_STRINGS_SCSU = 0x12
EFI_HII_SIBT_STRINGS_SCSU_FONT = 0x13
EFI_HII_SIBT_STRING_UCS2 = 0x14
EFI_HII_SIBT_STRING_UCS2_FONT = 0x15
EFI_HII_SIBT_STRINGS_UCS2 = 0x16
EFI_HII_SIBT_STRINGS_UCS2_FONT = 0x17
EFI_HII_SIBT_DUPLICATE = 0x20
EFI_HII_SIBT_SKIP2 = 0x21
EFI_HII_SIBT_SKIP1 = 0x22
EFI_HII_SIBT_EXT1 = 0x30
EFI_HII_SIBT_EXT2 = 0x31
EFI_HII_SIBT_EXT4 = 0x32
EFI_HII_SIBT_FONT = 0x40

BasicTypes = [EFI_IFR_TYPE_NUM_SIZE_8, EFI_IFR_TYPE_NUM_SIZE_16, EFI_IFR_TYPE_NUM_SIZE_32, EFI_IFR_TYPE_NUM_SIZE_64]
class EFI_GUID(Structure):
    _pack_ = 1
    _fields_ = [
        ('Data1', c_uint32),
        ('Data2', c_uint16),
        ('Data3', c_uint16),
        ('Data4', c_ubyte * 8),
    ]

    def from_list(self, listformat: list) -> None:
        self.Data1 = listformat[0]
        self.Data2 = listformat[1]
        self.Data3 = listformat[2]
        for i in range(8):
            self.Data4[i] = listformat[i + 3]

    def to_string(self) -> str:
        GuidStr = '{' + '{}, {}, {}, '.format('0x%x'%(self.Data1),'0x%x'%(self.Data2), '0x%x'%(self.Data3)) \
                        + '{' +  '{}, {}, {}, {}, {}, {}, {}, {}'.format('0x%x'%(self.Data4[0]), '0x%x'%(self.Data4[1]), '0x%x'%(self.Data4[2]), '0x%x'%(self.Data4[3]), \
                        '0x%x'%(self.Data4[4]), '0x%x'%(self.Data4[5]), '0x%x'%(self.Data4[6]), '0x%x'%(self.Data4[7])) + '}}'
        return GuidStr

    def __cmp__(self, otherguid) -> bool:
        if not isinstance(otherguid, EFI_GUID):
            return 'Input is not the GUID instance!'
        rt = False
        if self.Data1 == otherguid.Data1 and self.Data2 == otherguid.Data2 and self.Data3 == otherguid.Data3:
            rt = True
            for i in range(8):
                rt = rt & (self.Data4[i] == otherguid.Data4[i])
        return rt


GuidArray = c_ubyte * 8
EFI_HII_PLATFORM_SETUP_FORMSET_GUID = EFI_GUID(
    0x93039971, 0x8545, 0x4b04,
    GuidArray(0xb4, 0x5e, 0x32, 0xeb, 0x83, 0x26, 0x4, 0xe))

EFI_IFR_TIANO_GUID = EFI_GUID(
    0xf0b1735, 0x87a0, 0x4193,
    GuidArray(0xb2, 0x66, 0x53, 0x8c, 0x38, 0xaf, 0x48, 0xce))

EDKII_IFR_BIT_VARSTORE_GUID = (0x82DDD68B, 0x9163, 0x4187,
                               GuidArray(0x9B, 0x27, 0x20, 0xA8, 0xFD, 0x60,
                                         0xA7, 0x1D))

EFI_IFR_FRAMEWORK_GUID = (0x31ca5d1a, 0xd511, 0x4931,
                          GuidArray(0xb7, 0x82, 0xae, 0x6b, 0x2b, 0x17, 0x8c,
                                    0xd7))

class EFI_IFR_OP_HEADER(Structure):
    _pack_ = 1
    _fields_ = [
        ('OpCode', c_ubyte),
        ('Length', c_ubyte, 7),
        ('Scope', c_ubyte, 1),
    ]


class EFI_IFR_FORM_SET(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Guid', EFI_GUID),
        ('FormSetTitle', c_uint16),
        ('Help', c_uint16),
        ('Flags', c_ubyte),
    ]


class EFI_IFR_GUID_CLASS(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Guid', EFI_GUID),
        ('ExtendOpCode', c_ubyte),
        ('Class', c_uint16),
    ]


class EFI_IFR_GUID_SUBCLASS(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Guid', EFI_GUID),
        ('ExtendOpCode', c_ubyte),
        ('SubClass', c_uint16),
    ]


class EFI_IFR_DEFAULTSTORE(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('DefaultName', c_uint16),
        ('DefaultId', c_uint16),
    ]

class EFI_IFR_VARSTORE(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Guid', EFI_GUID),
        ('VarStoreId', c_uint16),
        ('Size', c_uint16),
        ('Name', ARRAY(c_ubyte, 2)),
    ]

def Refine_EFI_IFR_VARSTORE(Nums):
    class EFI_IFR_VARSTORE(Structure):
        _pack_ = 1
        _fields_ = [
            ('Header', EFI_IFR_OP_HEADER),
            ('Guid', EFI_GUID),
            ('VarStoreId', c_uint16),
            ('Size', c_uint16),
            ('Name', ARRAY(c_ubyte, Nums)),
        ]
        def __init__(self) -> None:
            self.Header = EFI_IFR_OP_HEADER()

    return EFI_IFR_VARSTORE()

class EFI_IFR_VARSTORE_EFI(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('VarStoreId', c_uint16),
        ('Guid', EFI_GUID),
        ('Attributes', c_uint32),
        ('Size', c_uint16),
        ('Name', ARRAY(c_ubyte, 2)),
    ]


def Refine_EFI_IFR_VARSTORE_EFI(Nums):
    class EFI_IFR_VARSTORE_EFI(Structure):
        _pack_ = 1
        _fields_ = [
            ('Header', EFI_IFR_OP_HEADER),
            ('VarStoreId', c_uint16),
            ('Guid', EFI_GUID),
            ('Attributes', c_uint32),
            ('Size', c_uint16),
            ('Name', ARRAY(c_ubyte, Nums)),
        ]
        def __init__(self) -> None:
            self.Header = EFI_IFR_OP_HEADER()

    return EFI_IFR_VARSTORE_EFI()


class EFI_IFR_GUID(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Guid', EFI_GUID),
    ]

def Refine_EFI_IFR_BUFFER(Nums):
    class EFI_IFR_BUFFER(Structure):
        _pack_ = 1
        _fields_ = [
            ('Data', ARRAY(c_ubyte, Nums)),
        ]
        def SetBuffer(self, Buffer):
            self.Buffer = Buffer

    return EFI_IFR_BUFFER()

def Refine_EFI_IFR_BIT_BUFFER(Nums):
    class EFI_IFR_BIT_BUFFER(Structure):
        _pack_ = 1
        _fields_ = [
            ('Data', ARRAY(c_ubyte, Nums)),
        ]
        def SetBuffer(self, Buffer):
            self.Buffer = Buffer

    return EFI_IFR_BIT_BUFFER

def Refine_EFI_IFR_BIT(Type, PreBits, Size, Data):

    if PreBits == 0:
        class Refine_EFI_IFR_BIT(Structure):
            _pack_ = 1
            _fields_ = [
                ('Data', Type, Size), # the really needed data
                ('Postfix', Type, sizeof(Type) * 8 - Size)
            ]
            def __init__(self, Data):
                self.Data = Data

    elif sizeof(Type) * 8 - Size - PreBits == 0:
        class Refine_EFI_IFR_BIT(Structure):
            _pack_ = 1
            _fields_ = [
                ('Prefix', Type, PreBits),
                ('Data', Type, Size), # the really needed data
            ]
            def __init__(self, Data):
                self.Data = Data
    else:
        class Refine_EFI_IFR_BIT(Structure):
            _pack_ = 1
            _fields_ = [
                ('Prefix', Type, PreBits),
                ('Data', Type, Size), # the really needed data
                ('Postfix', Type, sizeof(Type) * 8 - Size - PreBits)
            ]
            def __init__(self, Data):
                self.Data = Data

    return Refine_EFI_IFR_BIT(Data)

class EFI_IFR_GUID_VAREQNAME(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Guid', EFI_GUID),
        ('ExtendOpCode', c_uint8),
        ('QuestionId', c_uint16),
        ('NameId', c_uint16),

    ]


class EFI_HII_PACKAGE_HEADER(Structure):
    _pack_ = 1
    _fields_ = [
        ('Length', c_uint32, 24),
        ('Type', c_uint32, 8),
    ]


class EFI_HII_STRING_PACKAGE_HDR(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_HII_PACKAGE_HEADER),
        ('HdrSize', c_uint32),
        ('StringInfoOffset', c_uint32),
        ('LanguageWindow', c_ushort * 16),
        ('LanguageName', c_uint16),
        ('Language', c_char * 2),
    ]


class EFI_IFR_VARSTORE_NAME_VALUE(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('VarStoreId', c_uint16),
        ('Guid', EFI_GUID),
    ]


class EFI_IFR_FORM(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('FormId', c_uint16),
        ('FormTitle', c_uint16),
    ]


class EFI_IFR_GUID_BANNER(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Guid', EFI_GUID),
        ('ExtendOpCode', c_ubyte),
        ('Title', c_uint16),
        ('LineNumber', c_uint16),
        ('Alignment', c_ubyte),
    ]


class EFI_IFR_GUID_TIMEOUT(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Guid', EFI_GUID),
        ('ExtendOpCode', c_ubyte),
        ('TimeOut', c_uint16),
    ]


class EFI_IFR_GUID_LABEL(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Guid', EFI_GUID),
        ('ExtendOpCode', c_ubyte),
        ('Number', c_uint16),
    ]


class EFI_IFR_RULE(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('RuleId', c_ubyte),
    ]


class EFI_IFR_STATEMENT_HEADER(Structure):
    _pack_ = 1
    _fields_ = [
        ('Prompt', c_uint16),
        ('Help', c_uint16),
    ]


class EFI_IFR_SUBTITLE(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Statement', EFI_IFR_STATEMENT_HEADER),
        ('Flags', c_ubyte),
    ]


class EFI_IFR_TEXT(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Statement', EFI_IFR_STATEMENT_HEADER),
        ('TextTwo', c_uint16),
    ]


class EFI_IFR_IMAGE(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Id', c_uint16),
    ]


class VarStoreInfoNode(Union):
    _pack_ = 1
    _fields_ = [
        ('VarName', c_uint16),
        ('VarOffset', c_uint16),
    ]


class EFI_IFR_QUESTION_HEADER(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_STATEMENT_HEADER),
        ('QuestionId', c_uint16),
        ('VarStoreId', c_uint16),
        ('VarStoreInfo', VarStoreInfoNode),
        ('Flags', c_ubyte),
    ]


class u8Node(Structure):
    _pack_ = 1
    _fields_ = [
        ('MinValue', c_ubyte),
        ('MaxValue', c_ubyte),
        ('Step', c_ubyte),
    ]


class u16Node(Structure):
    _pack_ = 1
    _fields_ = [
        ('MinValue', c_uint16),
        ('MaxValue', c_uint16),
        ('Step', c_uint16),
    ]


class u32Node(Structure):
    _pack_ = 1
    _fields_ = [
        ('MinValue', c_uint32),
        ('MaxValue', c_uint32),
        ('Step', c_uint32),
    ]


class u64Node(Structure):
    _pack_ = 1
    _fields_ = [
        ('MinValue', c_uint64),
        ('MaxValue', c_uint64),
        ('Step', c_uint64),
    ]


class MINMAXSTEP_DATA(Union):
    _pack_ = 1
    _fields_ = [
        ('u8', u8Node),
        ('u16', u16Node),
        ('u32', u32Node),
        ('u64', u64Node),
    ]


EFI_IFR_NUMERIC_SIZE = 0x03
EFI_IFR_NUMERIC_SIZE_1 = 0x00
EFI_IFR_NUMERIC_SIZE_2 = 0x01
EFI_IFR_NUMERIC_SIZE_4 = 0x02
EFI_IFR_NUMERIC_SIZE_8 = 0x03

EFI_IFR_DISPLAY = 0x30
EFI_IFR_DISPLAY_INT_DEC = 0x00
EFI_IFR_DISPLAY_UINT_DEC = 0x10
EFI_IFR_DISPLAY_UINT_HEX = 0x20

class EFI_IFR_ONE_OF(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Question', EFI_IFR_QUESTION_HEADER),
        ('Flags', c_ubyte),
        ('Data', MINMAXSTEP_DATA),
    ]


def Refine_EFI_IFR_ONE_OF(Type):

    if Type == EFI_IFR_TYPE_NUM_SIZE_8:
        DataType = u8Node
    elif Type == EFI_IFR_TYPE_NUM_SIZE_16:
        DataType = u16Node
    elif Type == EFI_IFR_TYPE_NUM_SIZE_32:
        DataType = u32Node
    elif Type == EFI_IFR_TYPE_NUM_SIZE_64:
        DataType = u64Node
    else:
        DataType = u32Node
    class EFI_IFR_ONE_OF(Structure):
        _pack_ = 1
        _fields_ = [
            ('Header', EFI_IFR_OP_HEADER),
            ('Question', EFI_IFR_QUESTION_HEADER),
            ('Flags', c_ubyte),
            ('Data', DataType),
        ]
    return EFI_IFR_ONE_OF()

EFI_IFR_CHECKBOX_DEFAULT = 0x01
EFI_IFR_CHECKBOX_DEFAULT_MFG = 0x02


class EFI_IFR_CHECKBOX(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Question', EFI_IFR_QUESTION_HEADER),
        ('Flags', c_ubyte),
    ]

class EFI_IFR_NUMERIC(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Question', EFI_IFR_QUESTION_HEADER),
        ('Flags', c_ubyte),
        ('Data', MINMAXSTEP_DATA),
    ]

def Refine_EFI_IFR_NUMERIC(Type):

    if Type == EFI_IFR_TYPE_NUM_SIZE_8:
        DataType = u8Node
    elif Type == EFI_IFR_TYPE_NUM_SIZE_16:
        DataType = u16Node
    elif Type == EFI_IFR_TYPE_NUM_SIZE_32:
        DataType = u32Node
    elif Type == EFI_IFR_TYPE_NUM_SIZE_64:
        DataType = u64Node
    else:
        DataType = u32Node
    class EFI_IFR_NUMERIC(Structure):
        _pack_ = 1
        _fields_ = [
            ('Header', EFI_IFR_OP_HEADER),
            ('Question', EFI_IFR_QUESTION_HEADER),
            ('Flags', c_ubyte),
            ('Data', DataType),
        ]
    return EFI_IFR_NUMERIC()


class EFI_IFR_PASSWORD(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Question', EFI_IFR_QUESTION_HEADER),
        ('MinSize', c_uint16),
        ('MaxSize', c_uint16),
    ]


class EFI_HII_TIME(Structure):
    _pack_ = 1
    _fields_ = [
        ('Hour', c_ubyte),
        ('Minute', c_ubyte),
        ('Second', c_ubyte),
    ]


class EFI_HII_DATE(Structure):
    _pack_ = 1
    _fields_ = [
        ('Year', c_uint16),
        ('Month', c_ubyte),
        ('Day', c_ubyte),
    ]


class EFI_HII_REF(Structure):
    _pack_ = 1
    _fields_ = [
        ('QuestionId', c_uint16),
        ('FormId', c_uint16),
        ('FormSetGuid', EFI_GUID),
        ('DevicePath', c_uint16),
    ]


class EFI_IFR_TYPE_VALUE(Union):
    _pack_ = 1
    _fields_ = [
        ('u8', c_ubyte),
        ('u16', c_uint16),
        ('u32', c_uint32),
        ('u64', c_uint64),
        ('b', c_ubyte), #
        ('time', EFI_HII_TIME),
        ('date', EFI_HII_DATE),
        ('string', c_uint16),
        ('ref', EFI_HII_REF),
    ]


class EFI_IFR_ONE_OF_OPTION(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Option', c_uint16),
        ('Flags', c_ubyte),
        ('Type', c_ubyte),
        ('Value', EFI_IFR_TYPE_VALUE),
    ]


TypeDict = {
    EFI_IFR_TYPE_NUM_SIZE_8: c_ubyte,
    EFI_IFR_TYPE_NUM_SIZE_16: c_ushort,
    EFI_IFR_TYPE_NUM_SIZE_32: c_ulong,
    EFI_IFR_TYPE_NUM_SIZE_64: c_ulonglong,
    EFI_IFR_TYPE_BOOLEAN: c_ubyte,
    EFI_IFR_TYPE_TIME: EFI_HII_TIME,
    EFI_IFR_TYPE_DATE: EFI_HII_DATE,
    EFI_IFR_TYPE_STRING: c_uint16,
    EFI_IFR_TYPE_REF: EFI_HII_REF
}

TypeSizeDict = {
    EFI_IFR_TYPE_NUM_SIZE_8: 1,
    EFI_IFR_TYPE_NUM_SIZE_16: 2,
    EFI_IFR_TYPE_NUM_SIZE_32: 4,
    EFI_IFR_TYPE_NUM_SIZE_64: 8,
    EFI_IFR_TYPE_BOOLEAN: 1,
    EFI_IFR_TYPE_STRING: 2
}

SizeTypeDict = {
    1: c_uint8,
    2: c_uint16,
    4: c_uint32,
    8: c_uint64,
}

def Refine_EFI_IFR_ONE_OF_OPTION(Type, Nums):

    ValueType = TypeDict[Type]
    class EFI_IFR_ONE_OF_OPTION(Structure):
        _pack_ = 1
        _fields_ = [
            ('Header', EFI_IFR_OP_HEADER),
            ('Option', c_uint16),
            ('Flags', c_ubyte),
            ('Type', c_ubyte),
            ('Value', ValueType * Nums),
        ]
        def __init__(self) -> None:
            self.Header = EFI_IFR_OP_HEADER()

    return EFI_IFR_ONE_OF_OPTION()


EFI_IFR_OPTION_DEFAULT = 0x10
EFI_IFR_OPTION_DEFAULT_MFG = 0x20


class EFI_IFR_SUPPRESS_IF(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_LOCKED(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_ACTION(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Question', EFI_IFR_QUESTION_HEADER),
        ('QuestionConfig', c_uint16),
    ]


class EFI_IFR_REF(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Question', EFI_IFR_QUESTION_HEADER),
        ('FormId', c_uint16),
    ]


class EFI_IFR_REF2(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Question', EFI_IFR_QUESTION_HEADER),
        ('FormId', c_uint16),
        ('QuestionId', c_uint16),
    ]


class EFI_IFR_REF3(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Question', EFI_IFR_QUESTION_HEADER),
        ('FormId', c_uint16),
        ('QuestionId', c_uint16),
        ('FormSetId', EFI_GUID),
    ]


class EFI_IFR_REF4(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Question', EFI_IFR_QUESTION_HEADER),
        ('FormId', c_uint16),
        ('QuestionId', c_uint16),
        ('FormSetId', EFI_GUID),
        ('DevicePath', c_uint16),
    ]


class EFI_IFR_REF5(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Question', EFI_IFR_QUESTION_HEADER),
    ]


class EFI_IFR_RESET_BUTTON(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Statement', EFI_IFR_STATEMENT_HEADER),
        ('DefaultId', c_uint16),
    ]


class EFI_IFR_NO_SUBMIT_IF(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Error', c_uint16),
    ]


class EFI_IFR_INCONSISTENT_IF(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Error', c_uint16),
    ]


class EFI_IFR_EQ_ID_VAL(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('QuestionId', c_uint16),
        ('Value', c_uint16),
    ]


class EFI_IFR_EQ_ID_ID(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('QuestionId1', c_uint16),
        ('QuestionId2', c_uint16),
    ]

class EFI_IFR_EQ_ID_VAL_LIST(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('QuestionId', c_uint16),
        ('ListLength', c_uint16),
        ('ValueList', c_uint16),  #
    ]


def Refine_EFI_IFR_EQ_ID_VAL_LIST(Nums):
    class EFI_IFR_EQ_ID_VAL_LIST(Structure):
        _pack_ = 1
        _fields_ = [
            ('Header', EFI_IFR_OP_HEADER),
            ('QuestionId', c_uint16),
            ('ListLength', c_uint16),
            ('ValueList', c_uint16 * Nums),  #
        ]
        def __init__(self):
            self.Header = EFI_IFR_OP_HEADER()

    return EFI_IFR_EQ_ID_VAL_LIST()


class EFI_IFR_AND(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_OR(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        # ('Extend', c_ubyte)

    ]


class EFI_IFR_NOT(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_GRAY_OUT_IF(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_DATE(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Question', EFI_IFR_QUESTION_HEADER),
        ('Flags', c_ubyte),
    ]


EFI_QF_DATE_YEAR_SUPPRESS = 0x01
EFI_QF_DATE_MONTH_SUPPRESS = 0x02
EFI_QF_DATE_DAY_SUPPRESS = 0x04

EFI_QF_DATE_STORAGE = 0x30
QF_DATE_STORAGE_NORMAL = 0x00
QF_DATE_STORAGE_TIME = 0x10
QF_DATE_STORAGE_WAKEUP = 0x20


class EFI_IFR_TIME(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Question', EFI_IFR_QUESTION_HEADER),
        ('Flags', c_ubyte),
    ]


QF_TIME_HOUR_SUPPRESS = 0x01
QF_TIME_MINUTE_SUPPRESS = 0x02
QF_TIME_SECOND_SUPPRESS = 0x04
QF_TIME_STORAGE = 0x30
QF_TIME_STORAGE_NORMAL = 0x00
QF_TIME_STORAGE_TIME = 0x10
QF_TIME_STORAGE_WAKEUP = 0x20


class EFI_IFR_STRING(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Question', EFI_IFR_QUESTION_HEADER),
        ('MinSize', c_ubyte),
        ('MaxSize', c_ubyte),
        ('Flags', c_ubyte),
    ]


class EFI_IFR_REFRESH(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('RefreshInterval', c_ubyte),
    ]


class EFI_IFR_DISABLE_IF(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_TO_LOWER(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_TO_UPPER(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_MAP(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_ORDERED_LIST(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Question', EFI_IFR_QUESTION_HEADER),
        ('MaxContainers', c_ubyte),
        ('Flags', c_ubyte),
    ]


EFI_IFR_UNIQUE_SET = 0x01
EFI_IFR_NO_EMPTY_SET = 0x02


class EFI_IFR_VARSTORE_DEVICE(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('DevicePath', c_uint16),
    ]


class EFI_IFR_VERSION(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_END(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_MATCH(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_GET(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('VarStoreId', c_uint16),
        ('VarStoreInfo', VarStoreInfoNode),  ##########
        ('VarStoreType', c_ubyte),
    ]


class EFI_IFR_SET(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('VarStoreId', c_uint16),
        ('VarStoreInfo', VarStoreInfoNode),  ##########
        ('VarStoreType', c_ubyte),
    ]


class EFI_IFR_READ(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_WRITE(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_EQUAL(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_NOT_EQUAL(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_GREATER_THAN(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_GREATER_EQUAL(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_LESS_EQUAL(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_LESS_THAN(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_BITWISE_AND(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_BITWISE_OR(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_BITWISE_NOT(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_SHIFT_LEFT(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_SHIFT_RIGHT(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_ADD(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_SUBTRACT(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_MULTIPLY(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_DIVIDE(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_MODULO(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_RULE_REF(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('RuleId', c_ubyte),
    ]


class EFI_IFR_QUESTION_REF1(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('QuestionId', c_uint16),
    ]


class EFI_IFR_QUESTION_REF2(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_QUESTION_REF3(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_QUESTION_REF3_2(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('DevicePath', c_uint16),
    ]


class EFI_IFR_QUESTION_REF3_3(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('DevicePath', c_uint16),
        ('Guid', EFI_GUID),
    ]


class EFI_IFR_UINT8(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Value', c_ubyte),
    ]


class EFI_IFR_UINT16(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Value', c_uint16),
    ]


class EFI_IFR_UINT32(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Value', c_uint32),
    ]


class EFI_IFR_UINT64(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Value', c_uint64),
    ]


class EFI_IFR_TRUE(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_FALSE(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_TO_UINT(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_TO_STRING(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Format', c_ubyte),
    ]


class EFI_IFR_TO_BOOLEAN(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_MID(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_FIND(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Format', c_ubyte),
    ]


class EFI_IFR_TOKEN(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_STRING_REF1(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('StringId', c_uint16),
    ]


class EFI_IFR_STRING_REF2(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_CONDITIONAL(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_ZERO(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_ONE(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_ONES(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_UNDEFINED(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_LENGTH(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_DUP(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_THIS(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


EFI_IFR_FLAGS_FIRST_MATCHING = 0x00
EFI_IFR_FLAGS_FIRST_NON_MATCHING = 0x01

class EFI_IFR_SPAN(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Flags', c_ubyte),
    ]


class EFI_IFR_VALUE(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_DEFAULT(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('DefaultId', c_uint16),
        ('Type', c_ubyte),
        ('Value', EFI_IFR_TYPE_VALUE),
    ]

def Refine_EFI_IFR_DEFAULT(Type, Nums):
    ValueType = TypeDict[Type]
    class EFI_IFR_DEFAULT(Structure):
        _pack_ = 1
        _fields_ = [
            ('Header', EFI_IFR_OP_HEADER),
            ('DefaultId', c_uint16),
            ('Type', c_ubyte),
            ('Value', ARRAY(ValueType, Nums)),
        ]
        def __init__(self):
            self.Header = EFI_IFR_OP_HEADER()

    return EFI_IFR_DEFAULT()


class EFI_IFR_DEFAULT_2(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('DefaultId', c_uint16),
        ('Type', c_ubyte),
    ]

class EFI_IFR_FORM_MAP(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('FormId', c_uint16),
    ]

class EFI_IFR_FORM_MAP_METHOD(Structure):
    _pack_ = 1
    _fields_ = [
        ('MethodTitle', c_uint16),
        ('MethodIdentifier', EFI_GUID),
    ]
def Refine_EFI_IFR_FORM_MAP(Nums):
    class EFI_IFR_FORM_MAP(Structure):
        _pack_ = 1
        _fields_ = [
            ('Header', EFI_IFR_OP_HEADER),
            ('FormId', c_uint16),
            ('FormMapMethod', ARRAY(EFI_IFR_FORM_MAP_METHOD, Nums))
        ]
    return EFI_IFR_FORM_MAP()

class EFI_IFR_CATENATE(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_SECURITY(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Permissions', EFI_GUID),
    ]


class EFI_IFR_MODAL_TAG(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
    ]


class EFI_IFR_REFRESH_ID(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('RefreshEventGroupId', EFI_GUID),
    ]


class EFI_IFR_WARNING_IF(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Warning', c_uint16),
        ('TimeOut', c_ubyte),
    ]


class EFI_IFR_MATCH2(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('SyntaxType', EFI_GUID),
    ]

class EFI_HII_STRING_BLOCK(Structure):
    _pack_ = 1
    _fields_ = [
        ('BlockType', c_uint8),
    ]

class EFI_HII_SIBT_STRING_SCSU_FONT_BLOCK(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_HII_STRING_BLOCK),
        ('FontIdentifier', c_uint8),
        ('StringText', ARRAY(c_uint8, 2)),
    ]

class EFI_HII_SIBT_STRINGS_SCSU_BLOCK(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_HII_STRING_BLOCK),
        ('StringCount', c_uint16),
        ('StringText', ARRAY(c_uint8, 2)),
    ]

class EFI_HII_SIBT_STRINGS_SCSU_FONT_BLOCK(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_HII_STRING_BLOCK),
        ('FontIdentifier', c_uint8),
        ('StringCount', c_uint16),
        ('StringText', ARRAY(c_uint8, 2)),
    ]

class EFI_HII_SIBT_STRING_UCS2_BLOCK(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_HII_STRING_BLOCK),
        ('StringText', ARRAY(c_ushort, 2)),
    ]

class EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_HII_STRING_BLOCK),
        ('FontIdentifier', c_uint8),
        ('StringText', ARRAY(c_ushort, 2)),
    ]

class EFI_HII_SIBT_STRINGS_UCS2_BLOCK(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_HII_STRING_BLOCK),
        ('StringCount', c_uint16),
        ('StringText', ARRAY(c_ushort, 2)),
    ]

class EFI_HII_SIBT_STRINGS_UCS2_FONT_BLOCK(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_HII_STRING_BLOCK),
        ('FontIdentifier', c_uint8),
        ('StringCount', c_uint16),
        ('StringText', ARRAY(c_ushort, 2)),
    ]

class EFI_HII_SIBT_DUPLICATE_BLOCK(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_HII_STRING_BLOCK),
        ('StringId', c_uint16),
    ]

class EFI_HII_SIBT_EXT1_BLOCK(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_HII_STRING_BLOCK),
        ('BlockType2', c_uint8),
        ('Length', c_uint8),
    ]

class EFI_HII_SIBT_EXT2_BLOCK(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_HII_STRING_BLOCK),
        ('BlockType2', c_uint8),
        ('Length', c_uint16),
    ]

class EFI_HII_SIBT_EXT4_BLOCK(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_HII_STRING_BLOCK),
        ('BlockType2', c_uint8),
        ('Length', c_uint32),
    ]

class EFI_HII_SIBT_SKIP1_BLOCK(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_HII_STRING_BLOCK),
        ('SkipCount', c_uint8),
    ]

class EFI_HII_SIBT_SKIP2_BLOCK(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_HII_STRING_BLOCK),
        ('SkipCount', c_uint16),
    ]



EFI_IFR_FORM_OP = 0x01
EFI_IFR_SUBTITLE_OP = 0x02
EFI_IFR_TEXT_OP = 0x03
EFI_IFR_IMAGE_OP = 0x04
EFI_IFR_ONE_OF_OP = 0x05
EFI_IFR_CHECKBOX_OP = 0x06
EFI_IFR_NUMERIC_OP = 0x07
EFI_IFR_PASSWORD_OP = 0x08
EFI_IFR_ONE_OF_OPTION_OP = 0x09
EFI_IFR_SUPPRESS_IF_OP = 0x0A
EFI_IFR_LOCKED_OP = 0x0B
EFI_IFR_ACTION_OP = 0x0C
EFI_IFR_RESET_BUTTON_OP = 0x0D
EFI_IFR_FORM_SET_OP = 0x0E
EFI_IFR_REF_OP = 0x0F
EFI_IFR_NO_SUBMIT_IF_OP = 0x10
EFI_IFR_INCONSISTENT_IF_OP = 0x11
EFI_IFR_EQ_ID_VAL_OP = 0x12
EFI_IFR_EQ_ID_ID_OP = 0x13
EFI_IFR_EQ_ID_VAL_LIST_OP = 0x14
EFI_IFR_AND_OP = 0x15
EFI_IFR_OR_OP = 0x16
EFI_IFR_NOT_OP = 0x17
EFI_IFR_RULE_OP = 0x18
EFI_IFR_GRAY_OUT_IF_OP = 0x19
EFI_IFR_DATE_OP = 0x1A
EFI_IFR_TIME_OP = 0x1B
EFI_IFR_STRING_OP = 0x1C
EFI_IFR_REFRESH_OP = 0x1D
EFI_IFR_DISABLE_IF_OP = 0x1E
EFI_IFR_TO_LOWER_OP = 0x20
EFI_IFR_TO_UPPER_OP = 0x21
EFI_IFR_MAP_OP = 0x22
EFI_IFR_ORDERED_LIST_OP = 0x23
EFI_IFR_VARSTORE_OP = 0x24
EFI_IFR_VARSTORE_NAME_VALUE_OP = 0x25
EFI_IFR_VARSTORE_EFI_OP = 0x26
EFI_IFR_VARSTORE_DEVICE_OP = 0x27
EFI_IFR_VERSION_OP = 0x28
EFI_IFR_END_OP = 0x29
EFI_IFR_MATCH_OP = 0x2A
EFI_IFR_GET_OP = 0x2B
EFI_IFR_SET_OP = 0x2C
EFI_IFR_READ_OP = 0x2D
EFI_IFR_WRITE_OP = 0x2E
EFI_IFR_EQUAL_OP = 0x2F
EFI_IFR_NOT_EQUAL_OP = 0x30
EFI_IFR_GREATER_THAN_OP = 0x31
EFI_IFR_GREATER_EQUAL_OP = 0x32
EFI_IFR_LESS_THAN_OP = 0x33
EFI_IFR_LESS_EQUAL_OP = 0x34
EFI_IFR_BITWISE_AND_OP = 0x35
EFI_IFR_BITWISE_OR_OP = 0x36
EFI_IFR_BITWISE_NOT_OP = 0x37
EFI_IFR_SHIFT_LEFT_OP = 0x38
EFI_IFR_SHIFT_RIGHT_OP = 0x39
EFI_IFR_ADD_OP = 0x3A
EFI_IFR_SUBTRACT_OP = 0x3B
EFI_IFR_MULTIPLY_OP = 0x3C
EFI_IFR_DIVIDE_OP = 0x3D
EFI_IFR_MODULO_OP = 0x3E
EFI_IFR_RULE_REF_OP = 0x3F
EFI_IFR_QUESTION_REF1_OP = 0x40
EFI_IFR_QUESTION_REF2_OP = 0x41
EFI_IFR_UINT8_OP = 0x42
EFI_IFR_UINT16_OP = 0x43
EFI_IFR_UINT32_OP = 0x44
EFI_IFR_UINT64_OP = 0x45
EFI_IFR_TRUE_OP = 0x46
EFI_IFR_FALSE_OP = 0x47
EFI_IFR_TO_UINT_OP = 0x48
EFI_IFR_TO_STRING_OP = 0x49
EFI_IFR_TO_BOOLEAN_OP = 0x4A
EFI_IFR_MID_OP = 0x4B
EFI_IFR_FIND_OP = 0x4C
EFI_IFR_TOKEN_OP = 0x4D
EFI_IFR_STRING_REF1_OP = 0x4E
EFI_IFR_STRING_REF2_OP = 0x4F
EFI_IFR_CONDITIONAL_OP = 0x50
EFI_IFR_QUESTION_REF3_OP = 0x51
EFI_IFR_ZERO_OP = 0x52
EFI_IFR_ONE_OP = 0x53
EFI_IFR_ONES_OP = 0x54
EFI_IFR_UNDEFINED_OP = 0x55
EFI_IFR_LENGTH_OP = 0x56
EFI_IFR_DUP_OP = 0x57
EFI_IFR_THIS_OP = 0x58
EFI_IFR_SPAN_OP = 0x59
EFI_IFR_VALUE_OP = 0x5A
EFI_IFR_DEFAULT_OP = 0x5B
EFI_IFR_DEFAULTSTORE_OP = 0x5C
EFI_IFR_FORM_MAP_OP = 0x5D
EFI_IFR_CATENATE_OP = 0x5E
EFI_IFR_GUID_OP = 0x5F
EFI_IFR_SECURITY_OP = 0x60
EFI_IFR_MODAL_TAG_OP = 0x61
EFI_IFR_REFRESH_ID_OP = 0x62
EFI_IFR_WARNING_IF_OP = 0x63
EFI_IFR_MATCH2_OP = 0x64
EFI_IFR_SHOWN_DEFAULTSTORE_OP = 0x65

ConditionOps = [EFI_IFR_DISABLE_IF_OP, EFI_IFR_SUPPRESS_IF_OP, EFI_IFR_GRAY_OUT_IF_OP, EFI_IFR_INCONSISTENT_IF_OP]
BasicDataTypes = ['UINT8', 'UINT16', 'UINT32', 'UINT64', 'EFI_HII_DATE', 'EFI_HII_TIME', 'EFI_HII_REF']

def Refine_EFI_IFR_GUID_OPTIONKEY(Type):
    ValueType = TypeDict[Type]
    class EFI_IFR_GUID_OPTIONKEY(Structure):
        _pack_ = 1
        _fields_ = [
            ('Header', EFI_IFR_OP_HEADER),
            ('Guid', EFI_GUID),
            ('ExtendOpCode', c_uint8),
            ('QuestionId', c_uint16),
            ('OptionValue', ValueType),
            ('KeyValue', c_uint16),
        ]
    return EFI_IFR_GUID_OPTIONKEY()


EFI_IFR_EXTEND_OP_OPTIONKEY = 0x0
EFI_IFR_EXTEND_OP_VAREQNAME = 0x1

class EFI_COMPARE_TYPE(Enum):
    EQUAL = 0
    LESS_EQUAL = 1
    LESS_THAN = 2
    GREATER_THAN = 3
    GREATER_EQUAL = 4