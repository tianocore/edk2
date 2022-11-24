from ctypes import *
from struct import *

def test():
    value=1 #定义一个字符串变量
    address=id(value) #获取value的地址，赋给address
    get_value=ctypes.cast(address, ctypes.py_object).value #读取地址中的变量
    print(get_value)
    print(address)
class ind():
    def __init__(self) -> None:
        self.Data = 1
x = ind()
y = x
x = None
print(y.Data)

def StructToStream(s) -> bytes:
    Length = sizeof(s)
    P = cast(pointer(s), POINTER(c_char * Length))
    return P.contents.raw
class EFI_IFR_OP_HEADER(Structure):
    _pack_ = 1
    _fields_ = [
        ('OpCode', c_ubyte),
        ('Length', c_ubyte, 7),
        ('Scope', c_ubyte, 1),
    ]

class EFI_IFR_TYPE_VALUE(Union):
    _pack_ = 1
    _fields_ = [
        ('u8', c_ubyte),
        ('u16', c_uint16),
        ('u32', c_uint32),
        ('u64', c_uint64),
        ('b', c_bool),
    ]

def Refine_EFI_IFR_EQ_ID_VAL_LIST(Nums):
    class EFI_IFR_EQ_ID_VAL_LIST(Structure):
        _pack_ = 1
        _fields_ = [
            ('Header', EFI_IFR_OP_HEADER),
            ('QuestionId', c_uint16),
            ('ListLength', c_uint16),
            ('ValueList', ARRAY(EFI_IFR_TYPE_VALUE, Nums)),  #
        ]
        def __init__(self):
            self.Header = EFI_IFR_OP_HEADER()

    #EFI_IFR_EQ_ID_VAL_LIST.Header = EFI_IFR_OP_HEADER()

    return EFI_IFR_EQ_ID_VAL_LIST()

x = Refine_EFI_IFR_EQ_ID_VAL_LIST(5)
print(x.Header)
x.QuestionId = 1
x.ListLength = 2
print(len(x.ValueList))
y = StructToStream(x)
print(int('Mm'))