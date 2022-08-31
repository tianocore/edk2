from ctypes import *
import ctypes
from telnetlib import X3PAD
from tkinter import YView
import uuid
from VfrError import *

EFI_STRING_ID_INVALID = 0x0
EFI_HII_DEFAULT_CLASS_STANDARD = 0x0000
EFI_HII_DEFAULT_CLASS_MANUFACTURING = 0x0001
EFI_IFR_MAX_LENGTH = 0xFF
EFI_VARSTORE_ID_INVALID = 0
EFI_VARSTORE_ID_START = 0x20
EFI_VAROFFSET_INVALID  = 0xFFFF
EFI_IMAGE_ID_INVALID  =  0xFFFF

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

INVALID_ARRAY_INDEX  = 0xFFFFFFFF

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

class EFI_GUID(Structure):
    _pack_ = 1
    _fields_ = [
        ('Data1', c_uint32),
        ('Data2', c_uint16),
        ('Data3', c_uint16),
        ('Data4', c_uint8 * 8),
    ]

    def from_list(self, listformat: list) -> None:
        self.Data1 = listformat[0]
        self.Data2 = listformat[1]
        self.Data3 = listformat[2]
        for i in range(8):
            self.Data4[i] = listformat[i+3]

    def __cmp__(self, otherguid) -> bool:
        if not isinstance(otherguid, EFI_GUID):
            return 'Input is not the GUID instance!'
        rt = False
        if self.Data1 == otherguid.Data1 and self.Data2 == otherguid.Data2 and self.Data3 == otherguid.Data3:
            rt = True
            for i in range(8):
                rt = rt & (self.Data4[i] == otherguid.Data4[i])
        return rt

GuidArray = c_uint8 * 8
EFI_HII_PLATFORM_SETUP_FORMSET_GUID  = EFI_GUID(0x93039971, 0x8545, 0x4b04, GuidArray(0xb4, 0x5e, 0x32, 0xeb, 0x83, 0x26, 0x4, 0xe))
EFI_IFR_TIANO_GUID = EFI_GUID(0xf0b1735, 0x87a0, 0x4193, GuidArray(0xb2, 0x66, 0x53, 0x8c, 0x38, 0xaf, 0x48, 0xce))

class EFI_IFR_OP_HEADER(Structure):
    _pack_ = 1
    _fields_ = [
        ('OpCode', c_uint8),
        ('Length', c_uint8, 7),  #
        ('Scope', c_uint8, 1),  #
    ]



class EFI_IFR_FORM_SET(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Guid', EFI_GUID),
        ('FormSetTitle', c_uint16),
        ('Help', c_uint16),
        ('Flags', c_uint8),
    ]


class EFI_IFR_GUID_CLASS(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header', EFI_IFR_OP_HEADER),
        ('Guid', EFI_GUID),
        ('ExtendOpCode', c_uint8),
        ('Class', c_uint16),
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
        ('Name', c_uint8 * 2), #################
    ]


class EFI_IFR_VARSTORE_EFI(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header',             EFI_IFR_OP_HEADER),
        ('Guid',               EFI_GUID),
        ('VarStoreId',         c_uint16),
        ('Attributes',         c_uint32),
        ('Size',               c_uint16),
        ('Name',               c_uint8 * 2), ######################
    ]

class EFI_IFR_GUID(Structure):
    _pack_ = 1
    _fields_ = [
        ('Header',             EFI_IFR_OP_HEADER),
        ('Guid',               EFI_GUID),
    ]


class EFI_HII_PACKAGE_HEADER(Structure):
    _pack_ = 1
    _fields_ = [
        ('Length', c_uint32,   24),
        ('Type',   c_uint32,   8),
    ]

class EFI_HII_STRING_PACKAGE_HDR(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',             EFI_HII_PACKAGE_HEADER),
        ('HdrSize',            c_uint32),
        ('StringInfoOffset',   c_uint32),
        ('LanguageWindow',     c_ushort * 16),
        ('LanguageName',       c_uint16),
        ('Language',           c_char * 2),
    ]

class EFI_IFR_VARSTORE_NAME_VALUE(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('VarStoreId',          c_uint16),
        ('Guid',                EFI_GUID),
    ]

class EFI_IFR_FORM(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('FormId',              c_uint16),
        ('FormTitle',           c_uint16),
    ]

class EFI_IFR_GUID_BANNER(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Guid',                EFI_GUID),
        ('ExtendOpCode',        c_uint8),
        ('Title',               c_uint16),
        ('LineNumber',          c_uint16),
        ('Alignment',           c_uint8),
    ]

class EFI_IFR_GUID_TIMEOUT(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Guid',                EFI_GUID),
        ('ExtendOpCode',        c_uint8),
        ('TimeOut',             c_uint16),
    ]

class EFI_IFR_GUID_LABEL(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Guid',                EFI_GUID),
        ('ExtendOpCode',        c_uint8),
        ('Number',              c_uint16),
    ]

class EFI_IFR_RULE(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('RuleId',              c_uint8),
    ]

class EFI_IFR_STATEMENT_HEADER(Structure):
    _pack_= 1
    _fields_ = [
        ('Prompt',              c_uint16),
        ('Help',                c_uint16),
    ]

class EFI_IFR_SUBTITLE(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Statement',           EFI_IFR_STATEMENT_HEADER),
        ('Flags',               c_uint8),
    ]

class EFI_IFR_TEXT(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Statement',           EFI_IFR_STATEMENT_HEADER),
        ('TextTwo',             c_uint16),

    ]
class EFI_IFR_IMAGE(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Id',                  c_uint16),
    ]

class VarStoreInfoNode(Union):
    _pack_= 1
    _fields_ = [
        ('VarName',              c_uint16),
        ('VarOffset',            c_uint16),
    ]

class EFI_IFR_QUESTION_HEADER(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_STATEMENT_HEADER),
        ('QuestionId',          c_uint16),
        ('VarStoreId',          c_uint16),
        ('VarStoreInfo',        VarStoreInfoNode), ##########
        ('Flags',               c_uint8),
    ]

class u8Node(Structure):
    _pack_= 1
    _fields_ = [
        ('MinValue',             c_uint8),
        ('MaxValue',             c_uint8),
        ('Step',                 c_uint8),
    ]

class u16Node(Structure):
    _pack_= 1
    _fields_ = [
        ('MinValue',             c_uint16),
        ('MaxValue',             c_uint16),
        ('Step',                 c_uint16),
    ]

class u32Node(Structure):
    _pack_= 1
    _fields_ = [
        ('MinValue',             c_uint32),
        ('MaxValue',             c_uint32),
        ('Step',                 c_uint32),
    ]

class u64Node(Structure):
    _pack_= 1
    _fields_ = [
        ('MinValue',             c_uint64),
        ('MaxValue',             c_uint64),
        ('Step',                 c_uint64),
    ]


class MINMAXSTEP_DATA(Union):
    _pack_= 1
    _fields_ = [
        ('u8',                   u8Node),
        ('u16',                  u16Node),
        ('u32',                  u32Node),
        ('u64',                  u64Node),
    ]

class EFI_IFR_ONE_OF(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Question',            EFI_IFR_QUESTION_HEADER),
        ('Flags',               c_uint8),
        ('data',                MINMAXSTEP_DATA),
    ]

class EFI_IFR_CHECKBOX(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Question',            EFI_IFR_QUESTION_HEADER),
        ('Flags',               c_uint8),
    ]

class EFI_IFR_NUMERIC(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Question',            EFI_IFR_QUESTION_HEADER),
        ('Flags',               c_uint8),
        ('data',                MINMAXSTEP_DATA),
    ]

class EFI_IFR_PASSWORD(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Question',            EFI_IFR_QUESTION_HEADER),
        ('MinSize',             c_uint16),
        ('MaxSize',             c_uint16),
    ]

class EFI_HII_TIME(Structure):
    _pack_= 1
    _fields_ = [
        ('Hour',                c_uint8),
        ('Minute',              c_uint8),
        ('Second',              c_uint8),
    ]

class EFI_HII_DATE(Structure):
    _pack_= 1
    _fields_ = [
        ('Year',                c_uint16),
        ('Month',               c_uint8),
        ('Day',                 c_uint8),
    ]

class EFI_HII_REF(Structure):
    _pack_= 1
    _fields_ = [
        ('QuestionId',          c_uint16),
        ('FormId',              c_uint16),
        ('FormSetGuid',         EFI_GUID),
        ('DevicePath',          c_uint16),
    ]


class EFI_IFR_TYPE_VALUE(Union):
    _pack_= 1
    _fields_ = [
        ('u8',                  c_uint8),
        ('u16',                 c_uint16),
        ('u32',                 c_uint32),
        ('u64',                 c_uint64),
        ('b',                   c_bool),
        ('time',                EFI_HII_TIME),
        ('date',                EFI_HII_DATE),
        ('string',              c_uint16),
        ('ref',                 EFI_HII_REF),

    ]

class EFI_IFR_ONE_OF_OPTION(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Option',              c_uint16),
        ('Flags',               c_uint8),
        ('Type',                c_uint8),
        ('Value',               EFI_IFR_TYPE_VALUE),
    ]

class EFI_IFR_SUPPRESS_IF(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_LOCKED(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_ACTION(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Question',            EFI_IFR_QUESTION_HEADER),
        ('QuestionConfig',      c_uint16),
    ]

class EFI_IFR_REF(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Question',            EFI_IFR_QUESTION_HEADER),
        ('FormId',              c_uint16),
    ]

class EFI_IFR_REF2(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Question',            EFI_IFR_QUESTION_HEADER),
        ('FormId',              c_uint16),
        ('QuestionId',          c_uint16),
    ]

class EFI_IFR_REF3(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Question',            EFI_IFR_QUESTION_HEADER),
        ('FormId',              c_uint16),
        ('QuestionId',          c_uint16),
        ('FormSetId',           EFI_GUID),
    ]

class EFI_IFR_REF4(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Question',            EFI_IFR_QUESTION_HEADER),
        ('FormId',              c_uint16),
        ('QuestionId',          c_uint16),
        ('FormSetId',           EFI_GUID),
        ('DevicePath',          c_uint16),
    ]

class EFI_IFR_REF5(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Question',            EFI_IFR_QUESTION_HEADER),
    ]

class EFI_IFR_RESET_BUTTON(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Statement',           EFI_IFR_STATEMENT_HEADER),
        ('DefaultId',           c_uint16),
    ]

class EFI_IFR_NO_SUBMIT_IF(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Error',               c_uint16),
    ]


class EFI_IFR_INCONSISTENT_IF(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Error',               c_uint16),
    ]

class EFI_IFR_EQ_ID_VAL(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('QuestionId',          c_uint16),
        ('Value',               c_uint16),
    ]

class EFI_IFR_EQ_ID_ID(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('QuestionId1',         c_uint16),
        ('QuestionId2',         c_uint16),
    ]

class EFI_IFR_EQ_ID_VAL_LIST(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('QuestionId',          c_uint16),
        ('ListLength',          c_uint16),
        ('ValueList',           c_uint16 * 2), #########
    ]

class EFI_IFR_AND(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_OR(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_NOT(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_GRAY_OUT_IF(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_DATE(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Question',            EFI_IFR_QUESTION_HEADER),
        ('Flags',               c_uint8),
    ]

class EFI_IFR_TIME(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Question',            EFI_IFR_QUESTION_HEADER),
        ('Flags',               c_uint8),
    ]

class EFI_IFR_STRING(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Question',            EFI_IFR_QUESTION_HEADER),
        ('MinSize',             c_uint8),
        ('MaxSize',             c_uint8),
        ('Flags',               c_uint8),
    ]

class EFI_IFR_REFRESH(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('RefreshInterval',     c_uint8),
    ]

class EFI_IFR_DISABLE_IF(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_TO_LOWER(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_TO_UPPER(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_MAP(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_ORDERED_LIST(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Question',            EFI_IFR_QUESTION_HEADER),
        ('MaxContainers',       c_uint8),
        ('Flags',               c_uint8),
    ]

class EFI_IFR_VARSTORE_DEVICE(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('DevicePath',          c_uint16),
    ]

class EFI_IFR_VERSION(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_END(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_MATCH(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_GET(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('VarStoreId',          c_uint16),
        ('VarStoreInfo',        VarStoreInfoNode), ##########
        ('VarStoreType',        c_uint8),
    ]

class EFI_IFR_SET(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('VarStoreId',          c_uint16),
        ('VarStoreInfo',        VarStoreInfoNode), ##########
        ('VarStoreType',        c_uint8),
    ]

class EFI_IFR_READ(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_WRITE(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_EQUAL(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_NOT_EQUAL(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_GREATER_THAN(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_GREATER_EQUAL(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_LESS_EQUAL(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_LESS_THAN(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_BITWISE_AND(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_BITWISE_OR(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_BITWISE_NOT(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_SHIFT_LEFT(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_SHIFT_RIGHT(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_ADD(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_SUBTRACT(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_MULTIPLY(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_DIVIDE(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_MODULO(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_RULE_REF(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('RuleId',              c_uint8),
    ]

class EFI_IFR_QUESTION_REF1(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('QuestionId',          c_uint16),
    ]

class EFI_IFR_QUESTION_REF2(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_QUESTION_REF3(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_UINT8(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Value',               c_uint8),
    ]

class EFI_IFR_UINT16(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Value',               c_uint16),
    ]

class EFI_IFR_UINT32(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Value',               c_uint32),
    ]

class EFI_IFR_UINT64(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Value',               c_uint64),
    ]

class EFI_IFR_TRUE(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_FALSE(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_TO_UINT(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_TO_STRING(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Format',              c_uint8),
    ]

class EFI_IFR_TO_BOOLEAN(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_MID(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_FIND(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Format',              c_uint8),
    ]

class EFI_IFR_TOKEN(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_STRING_REF1(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('StringId',            c_uint16),
    ]

class EFI_IFR_STRING_REF2(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_CONDITIONAL(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_ZERO(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_ONE(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_ONES(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_UNDEFINED(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_LENGTH(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_DUP(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_THIS(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_SPAN(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Flags',               c_uint8),
    ]

class EFI_IFR_VALUE(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_DEFAULT(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('DefaultId',           c_uint16),
        ('Type',                c_uint8),
        ('Value',               EFI_IFR_TYPE_VALUE),
    ]

class EFI_IFR_FORM_MAP(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('FormId',              c_uint16),
    ]

class EFI_IFR_CATENATE(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_SECURITY(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Permissions',         EFI_GUID),
    ]

class EFI_IFR_MODAL_TAG(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
    ]

class EFI_IFR_REFRESH_ID(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('RefreshEventGroupId', EFI_GUID),
    ]

class EFI_IFR_WARNING_IF(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('Warning',             c_uint16),
        ('TimeOut',             c_uint8),
    ]

class EFI_IFR_MATCH2(Structure):
    _pack_= 1
    _fields_ = [
        ('Header',              EFI_IFR_OP_HEADER),
        ('SyntaxType',          EFI_GUID),
    ]

a = 'x'
b =a 
b = b + 'aa'
print(b)
x =[]
x.append(a+'a')