#line 1 "c:\\edk2\\NetworkPkg\\IScsiDxe\\IScsiConfigVfr.vfr"
#line 1 "c:\\edk2\\Build\\Ovmf3264\\DEBUG_VS2019\\X64\\NetworkPkg\\IScsiDxe\\IScsiDxe\\DEBUG\\IScsiDxeStrDefs.h"






















































































extern unsigned char IScsiDxeStrings[];

























































#line 146 "c:\\edk2\\Build\\Ovmf3264\\DEBUG_VS2019\\X64\\NetworkPkg\\IScsiDxe\\IScsiDxe\\DEBUG\\IScsiDxeStrDefs.h"

#line 148 "c:\\edk2\\Build\\Ovmf3264\\DEBUG_VS2019\\X64\\NetworkPkg\\IScsiDxe\\IScsiDxe\\DEBUG\\IScsiDxeStrDefs.h"
#line 1 "c:\\edk2\\NetworkPkg\\IScsiDxe\\IScsiConfigVfr.vfr"








#line 1 "c:\\edk2\\NetworkPkg\\IScsiDxe\\IScsiConfigNVDataStruc.h"











#line 1 "c:\\edk2\\NetworkPkg\\Include\\Guid/IScsiConfigHii.h"

















extern EFI_GUID  { 0x4b47d616, 0xa8d6, 0x4552, { 0x9d, 0x44, 0xcc, 0xad, 0x2e, 0xf, 0x4c, 0xf9}};

#line 21 "c:\\edk2\\NetworkPkg\\Include\\Guid/IScsiConfigHii.h"
#line 13 "c:\\edk2\\NetworkPkg\\IScsiDxe\\IScsiConfigNVDataStruc.h"







































































































































#pragma pack(1)




typedef struct {
  CHAR16    ISCSIIsId[13];
  CHAR16    ISCSIInitiatorIpAddress[16];
  CHAR16    ISCSIInitiatorNetmask[16];
  CHAR16    ISCSIInitiatorGateway[16];
  CHAR16    ISCSITargetName[224];
  CHAR16    ISCSITargetIpAddress[255];
  CHAR16    ISCSILun[21];
  CHAR16    ISCSIChapUsername[127];
  CHAR16    ISCSIChapSecret[17];
  CHAR16    ISCSIReverseChapUsername[127];
  CHAR16    ISCSIReverseChapSecret[17];
} KEYWORD_STR;

typedef struct _ISCSI_CONFIG_IFR_NVDATA {
  CHAR16         InitiatorName[224];
  CHAR16         AttemptName[12];
  UINT8          Enabled;
  UINT8          IpMode;

  UINT8          ConnectRetryCount;
  UINT8          Padding1;
  UINT16         ConnectTimeout;

  UINT8          InitiatorInfoFromDhcp;
  UINT8          TargetInfoFromDhcp;
  CHAR16         LocalIp[16];
  CHAR16         SubnetMask[16];
  CHAR16         Gateway[16];

  CHAR16         TargetName[224];
  CHAR16         TargetIp[255];
  UINT16         TargetPort;
  CHAR16         BootLun[21];

  UINT8          AuthenticationType;

  UINT8          CHAPType;
  CHAR16         CHAPName[127];
  CHAR16         CHAPSecret[17];
  CHAR16         ReverseCHAPName[127];
  CHAR16         ReverseCHAPSecret[17];

  BOOLEAN        MutualRequired;
  UINT8          Padding2;
  CHAR16         KerberosUserName[96];
  CHAR16         KerberosUserSecret[17];
  CHAR16         KerberosKDCName[96];
  CHAR16         KerberosKDCRealm[96];
  CHAR16         KerberosKDCIp[40];
  UINT16         KerberosKDCPort;

  UINT8          DynamicOrderedList[0x08];
  UINT8          DeleteAttemptList[0x08];
  UINT8          AddAttemptList[0x08];
  CHAR16         IsId[13];




  CHAR16         ISCSIMacAddr[96];
  CHAR16         ISCSIAttemptOrder[96];
  CHAR16         ISCSIAddAttemptList[96];
  CHAR16         ISCSIDeleteAttemptList[96];
  CHAR16         ISCSIDisplayAttemptList[96];
  CHAR16         ISCSIAttemptName[96];
  UINT8          ISCSIBootEnableList[0x08];
  UINT8          ISCSIIpAddressTypeList[0x08];
  UINT8          ISCSIConnectRetry[0x08];
  UINT16         ISCSIConnectTimeout[0x08];
  UINT8          ISCSIInitiatorInfoViaDHCP[0x08];
  UINT8          ISCSITargetInfoViaDHCP[0x08];
  UINT16         ISCSITargetTcpPort[0x08];
  UINT8          ISCSIAuthenticationMethod[0x08];
  UINT8          ISCSIChapType[0x08];
  KEYWORD_STR    Keyword[0x08];
} ISCSI_CONFIG_IFR_NVDATA;
#pragma pack()

#line 233 "c:\\edk2\\NetworkPkg\\IScsiDxe\\IScsiConfigNVDataStruc.h"
#line 10 "c:\\edk2\\NetworkPkg\\IScsiDxe\\IScsiConfigVfr.vfr"



formset
  guid     = { 0x4b47d616, 0xa8d6, 0x4552, { 0x9d, 0x44, 0xcc, 0xad, 0x2e, 0xf, 0x4c, 0xf9 } },
  title    = STRING_TOKEN(0x0002),
  help     = STRING_TOKEN(0x0003),

  varstore ISCSI_CONFIG_IFR_NVDATA,
    varid = 0x6666,
    name = ISCSI_CONFIG_IFR_NVDATA,
    guid = { 0x4b47d616, 0xa8d6, 0x4552, { 0x9d, 0x44, 0xcc, 0xad, 0x2e, 0xf, 0x4c, 0xf9 } };

  form formid = 1,
    title  = STRING_TOKEN(0x0004);

    string  varid   = ISCSI_CONFIG_IFR_NVDATA.InitiatorName,
            prompt  = STRING_TOKEN(0x0006),
            help    = STRING_TOKEN(0x0007),
            flags   = INTERACTIVE,
            key     = 0x101,
            minsize = 4,
            maxsize = 223,
    endstring;

    subtitle text = STRING_TOKEN(0x003A);

    goto 2,
         prompt = STRING_TOKEN(0x000E),
         help   = STRING_TOKEN(0x000E),
         flags  = INTERACTIVE,
         key    = 0x10e;

    label 0x9000;
    label 0xffff;

    subtitle text = STRING_TOKEN(0x003A);

    goto 5,
      prompt = STRING_TOKEN (0x0010),
      help   = STRING_TOKEN (0x0011),
      flags  = INTERACTIVE,
      key    = 0x116;

    subtitle text = STRING_TOKEN(0x003A);

    goto 4,
      prompt = STRING_TOKEN (0x0012),
      help   = STRING_TOKEN (0x0012),
      flags  = INTERACTIVE,
      key    = 0x110;

    subtitle text = STRING_TOKEN(0x003A);

  endform;

  form formid = 2,
    title  = STRING_TOKEN(0x0005);

    label 0x3000;
    label 0xffff;

  endform;

  form formid = 4,
    title  = STRING_TOKEN(0x0012);

    label 0x4000;
    label 0xffff;

    text
      help   = STRING_TOKEN (0x003B),
      text   = STRING_TOKEN (0x003B),
      flags  = INTERACTIVE,
      key    = 0x111;

    text
      help   = STRING_TOKEN (0x003C),
      text   = STRING_TOKEN (0x003C),
      flags  = INTERACTIVE,
      key    = 0x112;
  endform;

  form formid = 5,
    title  = STRING_TOKEN(0x0010);

    label 0x5000;
    label 0xffff;

    text
      help   = STRING_TOKEN (0x003B),
      text   = STRING_TOKEN (0x003B),
      flags  = INTERACTIVE,
      key    = 0x114;

    text
      help   = STRING_TOKEN (0x003C),
      text   = STRING_TOKEN (0x003C),
      flags  = INTERACTIVE,
      key    = 0x115;
  endform;

  form formid = 3,
    title  = STRING_TOKEN(0x000F);

    string  varid   = ISCSI_CONFIG_IFR_NVDATA.AttemptName,
            prompt  = STRING_TOKEN(0x0008),
            help    = STRING_TOKEN(0x0009),
            flags   = READ_ONLY,
            key     = 0x113,
            minsize = 0,
            maxsize = 12,
    endstring;

    subtitle text = STRING_TOKEN(0x003A);

    oneof varid  = ISCSI_CONFIG_IFR_NVDATA.Enabled,
          prompt = STRING_TOKEN(0x0013),
          help   = STRING_TOKEN(0x0014),
          option text = STRING_TOKEN(0x0015),         value = 0,         flags = DEFAULT;
          option text = STRING_TOKEN(0x0016),          value = 1,          flags = 0;
          option text = STRING_TOKEN(0x0017), value = 2, flags = 0;
    endoneof;

    subtitle text = STRING_TOKEN(0x003A);

    oneof varid  = ISCSI_CONFIG_IFR_NVDATA.IpMode,
          questionid = 0x11c,
          prompt = STRING_TOKEN(0x0018),
          help   = STRING_TOKEN(0x0019),
          option text = STRING_TOKEN(0x001A),        value = 0,        flags = INTERACTIVE;
          option text = STRING_TOKEN(0x001B),        value = 1,        flags = INTERACTIVE;
          option text = STRING_TOKEN(0x001C), value = 2, flags = INTERACTIVE;
    endoneof;

    subtitle text = STRING_TOKEN(0x003A);

    numeric varid   = ISCSI_CONFIG_IFR_NVDATA.ConnectRetryCount,
            prompt  = STRING_TOKEN(0x000A),
            help    = STRING_TOKEN(0x000B),
            flags   = 0,
            minimum = 0,
            maximum = 16,
            step    = 0,
    endnumeric;

    numeric varid   = ISCSI_CONFIG_IFR_NVDATA.ConnectTimeout,
            prompt  = STRING_TOKEN(0x000C),
            help    = STRING_TOKEN(0x000D),
            flags   = 0,
            minimum = 100,
            maximum = 20000,
            step    = 0,
            default = 1000,
    endnumeric;

    subtitle text = STRING_TOKEN(0x003A);

    string  varid   = ISCSI_CONFIG_IFR_NVDATA.IsId,
            prompt  = STRING_TOKEN(0x003D),
            help    = STRING_TOKEN(0x003E),
            flags   = INTERACTIVE,
            key     = 0x11e,
            minsize = 6,
            maxsize = 12,
    endstring;

    subtitle text = STRING_TOKEN(0x003A);

    suppressif ideqval ISCSI_CONFIG_IFR_NVDATA.IpMode == 2;
    checkbox varid = ISCSI_CONFIG_IFR_NVDATA.InitiatorInfoFromDhcp,
            prompt = STRING_TOKEN(0x002C),
            help   = STRING_TOKEN(0x002C),
            flags  = INTERACTIVE,
            key    = 0x102,
    endcheckbox;
    endif;

    suppressif ideqval ISCSI_CONFIG_IFR_NVDATA.IpMode == 1 OR
               ideqval ISCSI_CONFIG_IFR_NVDATA.IpMode == 2;

    grayoutif ideqval ISCSI_CONFIG_IFR_NVDATA.InitiatorInfoFromDhcp == 0x01;
    string  varid   = ISCSI_CONFIG_IFR_NVDATA.LocalIp,
            prompt  = STRING_TOKEN(0x0021),
            help    = STRING_TOKEN(0x0024),
            flags   = INTERACTIVE,
            key     = 0x103,
            minsize = 7,
            maxsize = 15,
    endstring;

    string  varid   = ISCSI_CONFIG_IFR_NVDATA.SubnetMask,
            prompt  = STRING_TOKEN(0x0022),
            help    = STRING_TOKEN(0x0024),
            flags   = INTERACTIVE,
            key     = 0x104,
            minsize = 7,
            maxsize = 15,
    endstring;

    string  varid   = ISCSI_CONFIG_IFR_NVDATA.Gateway,
            prompt  = STRING_TOKEN(0x0023),
            help    = STRING_TOKEN(0x0024),
            flags   = INTERACTIVE,
            key     = 0x105,
            minsize = 7,
            maxsize = 15,
    endstring;
    endif;

    endif;

    suppressif ideqval ISCSI_CONFIG_IFR_NVDATA.IpMode == 2;
    subtitle text = STRING_TOKEN(0x003A);
    endif;

    suppressif ideqval ISCSI_CONFIG_IFR_NVDATA.IpMode == 2 OR
               ideqval ISCSI_CONFIG_IFR_NVDATA.InitiatorInfoFromDhcp == 0x00;
    checkbox varid  = ISCSI_CONFIG_IFR_NVDATA.TargetInfoFromDhcp,
             prompt = STRING_TOKEN(0x002D),
             help   = STRING_TOKEN(0x002D),
             flags  = 0,
    endcheckbox;
    endif;

    suppressif ideqval ISCSI_CONFIG_IFR_NVDATA.IpMode == 2 OR
               ideqval ISCSI_CONFIG_IFR_NVDATA.TargetInfoFromDhcp == 0x01;

    string  varid   = ISCSI_CONFIG_IFR_NVDATA.TargetName,
            prompt  = STRING_TOKEN(0x0025),
            help    = STRING_TOKEN(0x0026),
            flags   = INTERACTIVE,
            key     = 0x10c,
            minsize = 4,
            maxsize = 223,
    endstring;

    string  varid   = ISCSI_CONFIG_IFR_NVDATA.TargetIp,
            prompt  = STRING_TOKEN(0x0027),
            help    = STRING_TOKEN(0x0028),
            flags   = INTERACTIVE,
            key     = 0x106,
            minsize = 0,
            maxsize = 255,
    endstring;

    numeric varid   = ISCSI_CONFIG_IFR_NVDATA.TargetPort,
            prompt  = STRING_TOKEN(0x0029),
            help    = STRING_TOKEN(0x0029),
            flags   = 0,
            minimum = 0,
            maximum = 65535,
            step    = 0,
    endnumeric;

    string varid    = ISCSI_CONFIG_IFR_NVDATA.BootLun,
            prompt  = STRING_TOKEN(0x002A),
            help    = STRING_TOKEN(0x002B),
            flags   = INTERACTIVE,
            key     = 0x10d,
            minsize = 1,
            maxsize = 20,
    endstring;

    endif;

    suppressif ideqval ISCSI_CONFIG_IFR_NVDATA.IpMode == 2;
    subtitle text = STRING_TOKEN(0x003A);
    endif;

    oneof varid  = ISCSI_CONFIG_IFR_NVDATA.AuthenticationType,
          questionid = 0x11d,
          prompt = STRING_TOKEN(0x001D),
          help   = STRING_TOKEN(0x001E),
          option text = STRING_TOKEN(0x001F),     value = 1, flags = 0;
          option text = STRING_TOKEN(0x0020),     value = 0, flags = DEFAULT;
    endoneof;

    suppressif NOT ideqval ISCSI_CONFIG_IFR_NVDATA.AuthenticationType == 1;
    oneof varid  = ISCSI_CONFIG_IFR_NVDATA.CHAPType,
          prompt = STRING_TOKEN(0x002E),
          help   = STRING_TOKEN(0x002F),
          option text = STRING_TOKEN(0x0030),    value = 0,    flags = 0;
          option text = STRING_TOKEN(0x0031), value = 1, flags = DEFAULT;
    endoneof;
    endif;

    suppressif NOT ideqval ISCSI_CONFIG_IFR_NVDATA.AuthenticationType == 1;
    string  varid   = ISCSI_CONFIG_IFR_NVDATA.CHAPName,
            prompt  = STRING_TOKEN(0x0032),
            help    = STRING_TOKEN(0x0032),
            flags   = INTERACTIVE,
            key     = 0x107,
            minsize = 0,
            maxsize = 126,
    endstring;

    string  varid    = ISCSI_CONFIG_IFR_NVDATA.CHAPSecret,
            prompt   = STRING_TOKEN(0x0033),
            help     = STRING_TOKEN(0x0034),
            flags    = INTERACTIVE,
            key      = 0x108,
            minsize  = 12,
            maxsize  = 16,
    endstring;

    endif;

    suppressif NOT ideqval ISCSI_CONFIG_IFR_NVDATA.AuthenticationType == 1 OR
               NOT ideqval ISCSI_CONFIG_IFR_NVDATA.CHAPType == 1;

    string  varid   = ISCSI_CONFIG_IFR_NVDATA.ReverseCHAPName,
            prompt  = STRING_TOKEN(0x0035),
            help    = STRING_TOKEN(0x0035),
            flags   = INTERACTIVE,
            key     = 0x109,
            minsize = 0,
            maxsize = 126,
    endstring;

    string  varid    = ISCSI_CONFIG_IFR_NVDATA.ReverseCHAPSecret,
            prompt   = STRING_TOKEN(0x0036),
            help     = STRING_TOKEN(0x0034),
            flags    = INTERACTIVE,
            key      = 0x10a,
            minsize  = 12,
            maxsize  = 16,
    endstring;

    endif;

    suppressif TRUE;

    string  varid   = ISCSI_CONFIG_IFR_NVDATA.ISCSIMacAddr,
            prompt  = STRING_TOKEN(0x003F),
            help    = STRING_TOKEN(0x003F),
            minsize = 0,
            maxsize = 96,
    endstring;

    string  varid   = ISCSI_CONFIG_IFR_NVDATA.ISCSIAttemptOrder,
            prompt  = STRING_TOKEN(0x0043),
            help    = STRING_TOKEN(0x0043),
            minsize = 0,
            maxsize = 96,
    endstring;

    string  varid   = ISCSI_CONFIG_IFR_NVDATA.ISCSIAddAttemptList,
            prompt  = STRING_TOKEN(0x0040),
            help    = STRING_TOKEN(0x0040),
            minsize = 0,
            maxsize = 96,
    endstring;

    string  varid   = ISCSI_CONFIG_IFR_NVDATA.ISCSIDeleteAttemptList,
            prompt  = STRING_TOKEN(0x0041),
            help    = STRING_TOKEN(0x0041),
            minsize = 0,
            maxsize = 96,
    endstring;

    string  varid   = ISCSI_CONFIG_IFR_NVDATA.ISCSIDisplayAttemptList,
            prompt  = STRING_TOKEN(0x0042),
            help    = STRING_TOKEN(0x0042),
            flags   = READ_ONLY,
            minsize = 0,
            maxsize = 96,
    endstring;

    label 0x6000;
    label 0xffff;
    endif;

    subtitle text = STRING_TOKEN(0x003A);

    text
      help   = STRING_TOKEN (0x0039),
      text   = STRING_TOKEN (0x0038),
      flags  = INTERACTIVE,
      key    = 0x10f;

    goto 1,
    prompt = STRING_TOKEN (0x0037),
    help   = STRING_TOKEN (0x0037),
    flags  = 0;

  endform;

endformset;

