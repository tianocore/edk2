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




typedef struct {
  MY_BITS_DATA    BitsData;
  UINT32          EfiBitGrayoutTest : 5;
  UINT32          EfiBitNumeric     : 4;
  UINT32          EfiBitOneof       : 10;
  UINT32          EfiBitCheckbox    : 1;
  UINT32                            : 0;
} MY_EFI_BITS_VARSTORE_DATA;