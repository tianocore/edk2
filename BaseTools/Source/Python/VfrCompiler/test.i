#line 1 "c:\\edk2\\MdeModulePkg\\Universal\\DriverSampleDxe\\Vfr.vfr"
#line 1 "c:\\edk2\\Build\\Ovmf3264\\DEBUG_VS2019\\X64\\MdeModulePkg\\Universal\\DriverSampleDxe\\DriverSampleDxe\\DEBUG\\DriverSampleStrDefs.h"





































































































































































































































extern unsigned char DriverSampleStrings[];



























#line 259 "c:\\edk2\\Build\\Ovmf3264\\DEBUG_VS2019\\X64\\MdeModulePkg\\Universal\\DriverSampleDxe\\DriverSampleDxe\\DEBUG\\DriverSampleStrDefs.h"

#line 261 "c:\\edk2\\Build\\Ovmf3264\\DEBUG_VS2019\\X64\\MdeModulePkg\\Universal\\DriverSampleDxe\\DriverSampleDxe\\DEBUG\\DriverSampleStrDefs.h"
#line 1 "c:\\edk2\\MdeModulePkg\\Universal\\DriverSampleDxe\\Vfr.vfr"











#line 1 "c:\\edk2\\MdePkg\\Include\\Uefi/UefiMultiPhase.h"




































































































































































































































#line 230 "c:\\edk2\\MdePkg\\Include\\Uefi/UefiMultiPhase.h"
#line 13 "c:\\edk2\\MdeModulePkg\\Universal\\DriverSampleDxe\\Vfr.vfr"
#line 1 "c:\\edk2\\MdeModulePkg\\Universal\\DriverSampleDxe\\NVDataStruc.h"






















#line 1 "c:\\edk2\\MdePkg\\Include\\Guid/HiiPlatformSetupFormset.h"



























extern EFI_GUID  gEfiHiiPlatformSetupFormsetGuid;
extern EFI_GUID  gEfiHiiDriverHealthFormsetGuid;
extern EFI_GUID  gEfiHiiUserCredentialFormsetGuid;
extern EFI_GUID  gEfiHiiRestStyleFormsetGuid;

#line 34 "c:\\edk2\\MdePkg\\Include\\Guid/HiiPlatformSetupFormset.h"
#line 24 "c:\\edk2\\MdeModulePkg\\Universal\\DriverSampleDxe\\NVDataStruc.h"
#line 1 "c:\\edk2\\MdePkg\\Include\\Guid/HiiFormMapMethodGuid.h"
















extern EFI_GUID  gEfiHiiStandardFormGuid;

#line 20 "c:\\edk2\\MdePkg\\Include\\Guid/HiiFormMapMethodGuid.h"
#line 25 "c:\\edk2\\MdeModulePkg\\Universal\\DriverSampleDxe\\NVDataStruc.h"
#line 1 "c:\\edk2\\MdeModulePkg\\Include\\Guid/DriverSampleHii.h"


























extern EFI_GUID  { 0xA04A27f4, 0xDF00, 0x4D42, { 0xB5, 0x52, 0x39, 0x51, 0x13, 0x02, 0x11, 0x3D }};
extern EFI_GUID  { 0xb3f56470, 0x6141, 0x4621, { 0x8f, 0x19, 0x70, 0x4e, 0x57, 0x7a, 0xa9, 0xe8 }};
extern EFI_GUID  { 0xF5E655D9, 0x02A6, 0x46f2, { 0x9E, 0x76, 0xB8, 0xBE, 0x8E, 0x60, 0xAB, 0x22 }};

#line 32 "c:\\edk2\\MdeModulePkg\\Include\\Guid/DriverSampleHii.h"
#line 26 "c:\\edk2\\MdeModulePkg\\Universal\\DriverSampleDxe\\NVDataStruc.h"
#line 1 "c:\\edk2\\MdeModulePkg\\Include\\Guid/ZeroGuid.h"
















extern EFI_GUID  gZeroGuid;

#line 20 "c:\\edk2\\MdeModulePkg\\Include\\Guid/ZeroGuid.h"
#line 27 "c:\\edk2\\MdeModulePkg\\Universal\\DriverSampleDxe\\NVDataStruc.h"




#pragma pack(1)









typedef struct {
  UINT16    NestByteField;
  UINT8                     : 1;
  UINT8     NestBitCheckbox : 1;
  UINT8     NestBitOneof    : 2;
  UINT8                     : 0;
  UINT8     NestBitNumeric  : 4;
} MY_BITS_DATA;

typedef union {
  UINT8    UnionNumeric;
  UINT8    UnionNumericAlias;
} MY_EFI_UNION_DATA;

typedef struct {
  UINT16               MyStringData[40];
  UINT16               SomethingHiddenForHtml;
  UINT8                HowOldAreYouInYearsManual;
  UINT16               HowTallAreYouManual;
  UINT8                HowOldAreYouInYears;
  UINT16               HowTallAreYou;
  UINT8                MyFavoriteNumber;
  UINT8                TestLateCheck;
  UINT8                TestLateCheck2;
  UINT8                QuestionAboutTreeHugging;
  UINT8                ChooseToActivateNuclearWeaponry;
  UINT8                SuppressGrayOutSomething;
  UINT8                OrderedList[8];
  UINT16               BootOrder[8];
  UINT8                BootOrderLarge;
  UINT8                DynamicRefresh;
  UINT8                DynamicOneof;
  UINT8                DynamicOrderedList[5];
  UINT8                Reserved;
  EFI_HII_REF          RefData;
  UINT8                NameValueVar0;
  UINT16               NameValueVar1;
  UINT16               NameValueVar2[20];
  UINT8                SerialPortNo;
  UINT8                SerialPortStatus;
  UINT16               SerialPortIo;
  UINT8                SerialPortIrq;
  UINT8                GetDefaultValueFromCallBack;
  UINT8                GetDefaultValueFromAccess;
  EFI_HII_TIME         Time;
  UINT8                RefreshGuidCount;
  UINT8                Match2;
  UINT8                GetDefaultValueFromCallBackForOrderedList[3];
  UINT8                BitCheckbox  : 1;
  UINT8                ReservedBits : 7;
  UINT16               BitOneof     : 6;
  UINT16                            : 0;
  UINT16               BitNumeric   : 12;
  MY_BITS_DATA         MyBitData;
  MY_EFI_UNION_DATA    MyUnionData;
  UINT8                QuestionXUefiKeywordRestStyle;
  UINT8                QuestionNonXUefiKeywordRestStyle;
} DRIVER_SAMPLE_CONFIGURATION;




typedef struct {
  UINT8     Field8;
  UINT16    Field16;
  UINT8     OrderedList[3];
  UINT16    SubmittedCallback;
} MY_EFI_VARSTORE_DATA;



// Data struct def in vfr
typedef struct {
  MY_BITS_DATA    BitsData;
  UINT32          EfiBitGrayoutTest : 5;
  UINT32          EfiBitNumeric     : 4;
  UINT32          EfiBitOneof       : 10;
  UINT32          EfiBitCheckbox    : 1;
  UINT32                            : 0;
} MY_EFI_BITS_VARSTORE_DATA;









#pragma pack()

#line 133 "c:\\edk2\\MdeModulePkg\\Universal\\DriverSampleDxe\\NVDataStruc.h"
#line 14 "c:\\edk2\\MdeModulePkg\\Universal\\DriverSampleDxe\\Vfr.vfr"

































formset
  guid      = { 0xA04A27f4, 0xDF00, 0x4D42, {0xB5, 0x52, 0x39, 0x51, 0x13, 0x02, 0x11, 0x3D} },
  title     = STRING_TOKEN(0x0013),
  help      = STRING_TOKEN(0x0014),
  classguid = { 0x93039971, 0x8545, 0x4b04, { 0xb4, 0x5e, 0x32, 0xeb, 0x83, 0x26, 0x4, 0xe } },













  varstore DRIVER_SAMPLE_CONFIGURATION,
    varid = 0x1234,
    name  = MyIfrNVData,
    guid  = { 0xA04A27f4, 0xDF00, 0x4D42, {0xB5, 0x52, 0x39, 0x51, 0x13, 0x02, 0x11, 0x3D} };




  efivarstore MY_EFI_VARSTORE_DATA,
    attribute = 0x00000002 | 0x00000001,
    name  = MyEfiVar,
    guid  = { 0xA04A27f4, 0xDF00, 0x4D42, {0xB5, 0x52, 0x39, 0x51, 0x13, 0x02, 0x11, 0x3D} };




  efivarstore MY_EFI_BITS_VARSTORE_DATA,
    attribute = 0x00000002 | 0x00000001,
    name  = MyEfiBitVar,
    guid  = { 0xA04A27f4, 0xDF00, 0x4D42, {0xB5, 0x52, 0x39, 0x51, 0x13, 0x02, 0x11, 0x3D} };

  efivarstore MY_EFI_UNION_DATA,
    attribute = 0x00000002 | 0x00000001,
    name  = MyEfiUnionVar,
    guid  = { 0xA04A27f4, 0xDF00, 0x4D42, {0xB5, 0x52, 0x39, 0x51, 0x13, 0x02, 0x11, 0x3D} };




  namevaluevarstore MyNameValueVar,
    name = STRING_TOKEN(0x0092),
    name = STRING_TOKEN(0x0093),
    name = STRING_TOKEN(0x0094),
    guid = { 0xA04A27f4, 0xDF00, 0x4D42, {0xB5, 0x52, 0x39, 0x51, 0x13, 0x02, 0x11, 0x3D} };

  defaultstore MyStandardDefault,
    prompt      = STRING_TOKEN(0x005B),
    attribute   = 0x0000;

  defaultstore MyManufactureDefault,
    prompt      = STRING_TOKEN(0x005D),
    attribute   = 0x0001;




  form formid = 1,
       title  = STRING_TOKEN(0x0015);

    subtitle text = STRING_TOKEN(0x001A);
    subtitle text = STRING_TOKEN(0x001B);




    text
      help   = STRING_TOKEN(0x0050),
      text   = STRING_TOKEN(0x001C),
        text   = STRING_TOKEN(0x001D);

    text
      help   = STRING_TOKEN(0x0054),
      text   = STRING_TOKEN(0x0054),
      flags  = INTERACTIVE,
      key    = 0x1237;

    text
      help   = STRING_TOKEN(0x0055),
      text   = STRING_TOKEN(0x0055),
      flags  = INTERACTIVE,
      key    = 0x1238;

    oneof name = MyOneOf,
      varid   = MyIfrNVData.SuppressGrayOutSomething,
      prompt  = STRING_TOKEN(0x001F),
      help    = STRING_TOKEN(0x0026),



      option text = STRING_TOKEN(0x002A), value = 0x0, flags = 0;
      option text = STRING_TOKEN(0x002B), value = 0x1, flags = 0;



      option text = STRING_TOKEN(0x002C), value = 0x2, flags = DEFAULT;
    endoneof;

    oneof varid  = MyIfrNVData.BootOrderLarge,
      prompt      = STRING_TOKEN(0x001F),
      help        = STRING_TOKEN(0x0026),
      default value = cond (pushthis == 0 ? 0 : cond ((questionref(MyOneOf) >> 0x4 & 0xF00) == 0x0 + 0x2 ? 0 : 1)),
      option text = STRING_TOKEN(0x002D), value = 0x0, flags = 0;
      option text = STRING_TOKEN(0x002E), value = 0x1, flags = 0;
    endoneof;

    grayoutif  ideqval MyIfrNVData.SuppressGrayOutSomething == 0x1;
      suppressif questionref(MyOneOf) == 0x0;

        checkbox varid   = MyIfrNVData.ChooseToActivateNuclearWeaponry,
                 prompt   = STRING_TOKEN(0x002F),
                 help     = STRING_TOKEN(0x0030),




                 flags    = CHECKBOX_DEFAULT | CHECKBOX_DEFAULT_MFG,
                 default  = TRUE,
        endcheckbox;
      endif;
    endif;
      label 0x2222;

      orderedlist
                varid       = MyIfrNVData.BootOrder,
                prompt      = STRING_TOKEN(0x008D),
                help        = STRING_TOKEN(0x005A),
                flags       = RESET_REQUIRED,
                option text = STRING_TOKEN(0x008F), value = 2, flags = 0;
                option text = STRING_TOKEN(0x008E), value = 1, flags = 0;
                option text = STRING_TOKEN(0x0090), value = 3, flags = 0;
              suppressif ideqval MyIfrNVData.BootOrderLarge == 0;
                option text = STRING_TOKEN(0x0091), value = 4, flags = 0;
              endif;
      endlist;

  endform;

endformset;
