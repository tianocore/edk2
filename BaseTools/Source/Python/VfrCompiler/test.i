#line 1 "e:\\code\\servergen3\\intel\\ServerPlatformPkg\\Features\\IPMI\\Features\\SetupBmcCfg\\BmcConfig.vfr"
#line 1 "e:\\code\\servergen3\\intel\\Build\\BirchStreamRpPkg\\DEBUG_VS2015x86\\X64\\ServerPlatformPkg\\Features\\IPMI\\Features\\SetupBmcCfg\\SetupBmcCfg\\DEBUG\\SetupBmcCfgStrDefs.h"




























































































extern unsigned char SetupBmcCfgStrings[];







































#line 134 "e:\\code\\servergen3\\intel\\Build\\BirchStreamRpPkg\\DEBUG_VS2015x86\\X64\\ServerPlatformPkg\\Features\\IPMI\\Features\\SetupBmcCfg\\SetupBmcCfg\\DEBUG\\SetupBmcCfgStrDefs.h"

#line 136 "e:\\code\\servergen3\\intel\\Build\\BirchStreamRpPkg\\DEBUG_VS2015x86\\X64\\ServerPlatformPkg\\Features\\IPMI\\Features\\SetupBmcCfg\\SetupBmcCfg\\DEBUG\\SetupBmcCfgStrDefs.h"
#line 1 "e:\\code\\servergen3\\intel\\ServerPlatformPkg\\Features\\IPMI\\Features\\SetupBmcCfg\\BmcConfig.vfr"



























#line 1 "e:\\code\\servergen3\\intel\\serverplatformpkg\\features\\ipmi\\features\\setupbmccfg\\BmcConfig.h"








































#pragma pack(1)
typedef struct
{
  UINT8  OnBoardNIC_IpMode;
  UINT8  Reserved1;
  UINT16 OnBoardNIC_IpAddress[20];
  UINT16 OnBoardNIC_SubnetMask[20];
  UINT16 OnBoardNIC_GatewayIp[20];
  UINT16 Dhcp_Name[64];

  UINT8  RmmNic_IpMode;
  UINT8  SecureKcsMode;
  UINT16 RmmNic_IpAddress[20];
  UINT16 RmmNic_SubnetMask[20];
  UINT16 RmmNic_GatewayIp[20];
  UINT8  RmmPresent;

  UINT8  NicPresent;

  UINT16 UserName1[20];
  UINT16 UserPassword1[20];
  UINT8  EnableChannelUser1;
  UINT8  Privilege1;

  UINT16 UserName2[20];
  UINT16 UserPassword2[20];
  UINT8  EnableChannelUser2;
  UINT8  Privilege2;

  UINT16 UserName3[20];
  UINT16 UserPassword3[20];
  UINT8  EnableChannelUser3;
  UINT8  Privilege3;

  UINT16 UserName4[20];
  UINT16 UserPassword4[20];
  UINT8  EnableChannelUser4;
  UINT8  Privilege4;

  UINT16 UserName5[20];
  UINT16 UserPassword5[20];
  UINT8  EnableChannelUser5;
  UINT8  Privilege5;

  UINT8   User;
  UINT32  BmcStatus;

  UINT8  OnBoardNIC_Ipv6Mode;



  UINT16 OnBoardNIC_Ipv6Address[46];
  UINT16 OnBoardNIC_GatewayIpv6[46];
  UINT8  OnBoardNIC_Ipv6PreLen;
  UINT8  Reserved3;
  UINT16 Dhcp_Name_Ipv6[64];
  UINT8  OnBoardNIC_IpSelection;

  UINT8  RmmNic_Ipv6Mode;



  UINT16 RmmNic_Ipv6Address[46];
  UINT16 RmmNic_GatewayIpv6[46];
  UINT8  RmmNic_PrefixLength;
  UINT8  Ipv6RmmPresent;



  UINT8  DedicatedIPv6;
  UINT8  EnableComplexPasswordCheck;

  UINT8  OnboardNICPresent;
  UINT8  DedicatedIPv6OptionHidden;
}BMC_LAN_CONFIG;
#pragma pack()

#line 119 "e:\\code\\servergen3\\intel\\serverplatformpkg\\features\\ipmi\\features\\setupbmccfg\\BmcConfig.h"
#line 29 "e:\\code\\servergen3\\intel\\ServerPlatformPkg\\Features\\IPMI\\Features\\SetupBmcCfg\\BmcConfig.vfr"
#line 1 "e:\\code\\servergen3\\intel\\ServerPlatformPkg\\Include\\Configuration.h"
































#line 34 "e:\\code\\servergen3\\intel\\ServerPlatformPkg\\Include\\Configuration.h"



























































































































































































































































































































































































































































































































#line 542 "e:\\code\\servergen3\\intel\\ServerPlatformPkg\\Include\\Configuration.h"
#line 30 "e:\\code\\servergen3\\intel\\ServerPlatformPkg\\Features\\IPMI\\Features\\SetupBmcCfg\\BmcConfig.vfr"
#line 1 "e:\\code\\servergen3\\intel\\ServerPlatformPkg\\Include\\Guid/PcPassword.h"











































typedef struct _PC_PASSWORD_VARIABLE {
  UINT8  UserPassword[32];
  UINT8  AdminPassword[32];
} PC_PASSWORD_VARIABLE;






























extern EFI_GUID gPcPasswordGuid;

#line 81 "e:\\code\\servergen3\\intel\\ServerPlatformPkg\\Include\\Guid/PcPassword.h"
#line 31 "e:\\code\\servergen3\\intel\\ServerPlatformPkg\\Features\\IPMI\\Features\\SetupBmcCfg\\BmcConfig.vfr"




formset
  guid  = { 0x8236697e, 0xf0f6, 0x405f, {0x99, 0x13, 0xac, 0xbc, 0x50, 0xaa, 0x45, 0xd1} },
  title = STRING_TOKEN(0x0002),
  help = STRING_TOKEN(0x0003),
  class = 0x01,
  subclass = 0,

    efivarstore UINT8, attribute = 0x00000002, name = Access, guid = { 0x3e710061, 0x647a, 0x4c03, { 0xbe, 0x85, 0xfa, 0xd6, 0xcc, 0xfa, 0x5a, 0x18 } };

  varstore BMC_LAN_CONFIG,
    name = BmcLanConfig,
    guid = { 0xec87d643, 0xeba4, 0x4bb5, {0xa1, 0xe5, 0x3f, 0x3e, 0x36, 0xb2, 0xd, 0xa9} };

    defaultstore MyStandardDefault,
      prompt      = STRING_TOKEN(0x003A),
      attribute   = 0x0000;                         

    defaultstore MyManufactureDefault,
      prompt      = STRING_TOKEN(0x003B),
      attribute   = 0x0001;

  form formid = 0x0C,
    title  = STRING_TOKEN (0x0002);
    
    

  suppressif NOT ideqval BMC_LAN_CONFIG.SecureKcsMode == 0x04;
    subtitle text = STRING_TOKEN (0x0041);
  endif;

  suppressif NOT ideqval BMC_LAN_CONFIG.SecureKcsMode == 0x05;
    subtitle text = STRING_TOKEN (0x0040);
  endif;

  grayoutif ideqval BMC_LAN_CONFIG.SecureKcsMode == 0x04;
    suppressif  ideqval BMC_LAN_CONFIG.SecureKcsMode == 0x05;

    goto 0x38,
        prompt  = STRING_TOKEN(0x0007),
        help    = STRING_TOKEN(0x0008);

suppressif ideqval BMC_LAN_CONFIG.OnboardNICPresent == 0;
    subtitle text = STRING_TOKEN(0x0037);

    subtitle text = STRING_TOKEN(0x0004);


  oneof varid     = BMC_LAN_CONFIG.OnBoardNIC_IpMode,
  questionid     = 2020,
  prompt        = STRING_TOKEN(0x0009),
  help          = STRING_TOKEN(0x000A),
      option text = STRING_TOKEN(0x000C), value=1, flags=INTERACTIVE | RESET_REQUIRED ;
      option text = STRING_TOKEN(0x000D),  value=2, flags=INTERACTIVE ;
  endoneof;

grayoutif ideqval BMC_LAN_CONFIG.OnBoardNIC_IpMode == 2;
  string    varid    = BMC_LAN_CONFIG.OnBoardNIC_IpAddress,
      prompt   = STRING_TOKEN(0x000E),
      help     = STRING_TOKEN(0x000F),
      flags    = INTERACTIVE | RESET_REQUIRED,
      key      = 2001,
      minsize  = 6,
      maxsize  = 15,
  endstring;
endif;

grayoutif ideqval BMC_LAN_CONFIG.OnBoardNIC_IpMode == 2;
  string    varid    = BMC_LAN_CONFIG.OnBoardNIC_SubnetMask,
      prompt   = STRING_TOKEN(0x0010),
      help     = STRING_TOKEN(0x0011),
      flags    = INTERACTIVE | RESET_REQUIRED,
      key      = 2002,
      minsize  = 6,
      maxsize  = 15,
  endstring;
endif;

grayoutif ideqval BMC_LAN_CONFIG.OnBoardNIC_IpMode == 2;
  string    varid    = BMC_LAN_CONFIG.OnBoardNIC_GatewayIp,
      prompt   = STRING_TOKEN(0x0012),
      help     = STRING_TOKEN(0x0013),
      flags    = INTERACTIVE | RESET_REQUIRED,
      key      = 2003,
      minsize  = 6,
      maxsize  = 15,
  endstring;
endif;




    subtitle text = STRING_TOKEN(0x0037);
    subtitle text = STRING_TOKEN(0x0021);
        oneof varid     = BMC_LAN_CONFIG.OnBoardNIC_IpSelection,
            questionid     = 2021,
            prompt      = STRING_TOKEN(0x001F),
            help        = STRING_TOKEN(0x0020),
            option text = STRING_TOKEN(0x0038),  value = 1, flags = INTERACTIVE | RESET_REQUIRED;
            option text = STRING_TOKEN(0x0039), value = 0, flags = INTERACTIVE | RESET_REQUIRED;
        endoneof;

suppressif ideqval BMC_LAN_CONFIG.OnBoardNIC_IpSelection == 0;
  oneof varid     = BMC_LAN_CONFIG.OnBoardNIC_Ipv6Mode,
  questionid     = 2022,
  prompt        = STRING_TOKEN(0x0014),
  help          = STRING_TOKEN(0x0015),
      option text = STRING_TOKEN(0x0017), value=1, flags= INTERACTIVE | RESET_REQUIRED ;
      option text = STRING_TOKEN(0x0018),   value=2, flags= INTERACTIVE ;
  endoneof;
endif;




suppressif ideqval BMC_LAN_CONFIG.OnBoardNIC_IpSelection == 0;
grayoutif ideqval BMC_LAN_CONFIG.OnBoardNIC_Ipv6Mode == 2   OR
          ideqval BMC_LAN_CONFIG.OnBoardNIC_Ipv6Mode == 5;
  string    varid    = BMC_LAN_CONFIG.OnBoardNIC_Ipv6Address,
      prompt   = STRING_TOKEN(0x0019),
      help     = STRING_TOKEN(0x001A),
      flags    = INTERACTIVE | RESET_REQUIRED,
      key      = 2011,
      minsize  = 6,
      maxsize  = 46,
  endstring;
endif;
endif;



suppressif ideqval BMC_LAN_CONFIG.OnBoardNIC_IpSelection == 0;
grayoutif ideqval BMC_LAN_CONFIG.OnBoardNIC_Ipv6Mode == 2   OR
          ideqval BMC_LAN_CONFIG.OnBoardNIC_Ipv6Mode == 5;

  string    varid    = BMC_LAN_CONFIG.OnBoardNIC_GatewayIpv6,
      prompt   = STRING_TOKEN(0x001D),
      help     = STRING_TOKEN(0x001E),
      flags    = INTERACTIVE | RESET_REQUIRED,
      key      = 2012,
      minsize  = 8,
      maxsize  = 46,
  endstring;
endif;
endif;




suppressif ideqval BMC_LAN_CONFIG.OnBoardNIC_IpSelection == 0;
grayoutif ideqval BMC_LAN_CONFIG.OnBoardNIC_Ipv6Mode == 2   OR
          ideqval BMC_LAN_CONFIG.OnBoardNIC_Ipv6Mode == 5;
      numeric varid = BMC_LAN_CONFIG.OnBoardNIC_Ipv6PreLen,
          prompt    = STRING_TOKEN(0x001B),
          help      = STRING_TOKEN(0x001C),
          flags     = RESET_REQUIRED,
          minimum   = 0,
          maximum   = 128,
          step      = 1,
      endnumeric;
endif;
endif;
endif;




subtitle text = STRING_TOKEN(0x0037);
subtitle text = STRING_TOKEN(0x0005);

  oneof varid     = BMC_LAN_CONFIG.RmmNic_IpMode,
  questionid     = 2023,
  prompt        = STRING_TOKEN(0x0009),
  help          = STRING_TOKEN(0x000B),
      option text = STRING_TOKEN(0x000C), value=1, flags= INTERACTIVE | RESET_REQUIRED ;
      option text = STRING_TOKEN(0x000D),  value=2, flags= INTERACTIVE ;
  endoneof;

grayoutif  ideqval BMC_LAN_CONFIG.RmmNic_IpMode == 2;
  string    varid    = BMC_LAN_CONFIG.RmmNic_IpAddress,
      prompt   = STRING_TOKEN(0x000E),
      help     = STRING_TOKEN(0x000F),
      flags    = INTERACTIVE | RESET_REQUIRED,
      key      = 2004,
      minsize  = 6,
      maxsize  = 15,
  endstring;

  string    varid    = BMC_LAN_CONFIG.RmmNic_SubnetMask,
      prompt   = STRING_TOKEN(0x0010),
      help     = STRING_TOKEN(0x0011),
      flags    = INTERACTIVE | RESET_REQUIRED,
      key      = 2005,
      minsize  = 6,
      maxsize  = 15,
  endstring;

  string    varid    = BMC_LAN_CONFIG.RmmNic_GatewayIp,
      prompt   = STRING_TOKEN(0x0012),
      help     = STRING_TOKEN(0x0013),
      flags    = INTERACTIVE | RESET_REQUIRED,
      key      = 2006,
      minsize  = 6,
      maxsize  = 15,
  endstring;
endif;
endif;




subtitle text = STRING_TOKEN(0x0037);
subtitle text = STRING_TOKEN(0x0006);

suppressif ideqval BMC_LAN_CONFIG.DedicatedIPv6OptionHidden == 1;
  oneof varid     = BMC_LAN_CONFIG.DedicatedIPv6,
      questionid  = 2036,
      prompt      = STRING_TOKEN(0x003C),
      help        = STRING_TOKEN(0x003D),
      option text = STRING_TOKEN(0x0038),  value = 1, flags = INTERACTIVE | RESET_REQUIRED;
      option text = STRING_TOKEN(0x0039), value = 0, flags = INTERACTIVE | RESET_REQUIRED;
  endoneof;
endif;

suppressif ideqval BMC_LAN_CONFIG.DedicatedIPv6 == 0;
  oneof varid     = BMC_LAN_CONFIG.RmmNic_Ipv6Mode,
  questionid     = 2024,
  prompt        = STRING_TOKEN(0x0014),
  help          = STRING_TOKEN(0x0016),
      option text = STRING_TOKEN(0x0017), value=1, flags= INTERACTIVE  | RESET_REQUIRED;
      option text = STRING_TOKEN(0x0018),   value=2, flags= INTERACTIVE ;
  endoneof;
endif;

grayoutif  ideqval BMC_LAN_CONFIG.RmmNic_Ipv6Mode == 2;
  string    varid    = BMC_LAN_CONFIG.RmmNic_Ipv6Address,
      prompt   = STRING_TOKEN(0x0019),
      help     = STRING_TOKEN(0x001A),
      flags    = INTERACTIVE | RESET_REQUIRED,
      key      = 2013,
      minsize  = 6,
      maxsize  = 46,
  endstring;
endif;

grayoutif ideqval BMC_LAN_CONFIG.RmmNic_Ipv6Mode == 2;
  string    varid    = BMC_LAN_CONFIG.RmmNic_GatewayIpv6,
      prompt   = STRING_TOKEN(0x001D),
      help     = STRING_TOKEN(0x001E),
      flags    = INTERACTIVE | RESET_REQUIRED,
      key      = 2014,
      minsize  = 6,
      maxsize  = 46,
  endstring;
endif;

grayoutif ideqval BMC_LAN_CONFIG.RmmNic_Ipv6Mode == 2;
      numeric varid = BMC_LAN_CONFIG.RmmNic_PrefixLength,
          prompt    = STRING_TOKEN(0x001B),
          help      = STRING_TOKEN(0x001C),
          flags     = RESET_REQUIRED,
          minimum   = 0,
          maximum   = 128,
          step      = 1,
      endnumeric;
endif;

subtitle text = STRING_TOKEN(0x0037);

  string    varid    = BMC_LAN_CONFIG.Dhcp_Name,
      prompt   = STRING_TOKEN(0x0022),
      help     = STRING_TOKEN(0x0023),
      flags    = INTERACTIVE | RESET_REQUIRED,
      key      = 2010,
      minsize  = 2,
      maxsize  = 63,
  endstring;


endif;
endform;

  form formid = 0x38,
        title    = STRING_TOKEN(0x0007);
        
        

    oneof varid   = BMC_LAN_CONFIG.EnableComplexPasswordCheck,
    prompt        = STRING_TOKEN(0x003E),
        help        = STRING_TOKEN(0x003F),
        option text = STRING_TOKEN(0x0038), value = 01 , flags  = RESET_REQUIRED ;
        option text = STRING_TOKEN(0x0039), value = 00 , flags = RESET_REQUIRED ;
    endoneof;

    grayoutif TRUE;
    text
      help   = STRING_TOKEN(0x0037),
      text   = STRING_TOKEN(0x0024),
      text   = STRING_TOKEN(0x0025),
      flags  = 0,
      key    = 0;
    endif;

    oneof varid     = BMC_LAN_CONFIG.Privilege1,
    questionid    = 2026,
    prompt        = STRING_TOKEN(0x002A),
    help          = STRING_TOKEN(0x002B),
        option text = STRING_TOKEN(0x002C),     value=1, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x002D),         value=2, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x002E),     value=3, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x002F),value=4, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x0030),     value=0xF, flags = INTERACTIVE | RESET_REQUIRED ;
    endoneof;

    oneof varid     = BMC_LAN_CONFIG.EnableChannelUser1,
    questionid    = 2027,
    prompt        = STRING_TOKEN(0x0031),
    help          = STRING_TOKEN(0x0032),
      option text = STRING_TOKEN(0x0038),      value=1, flags= INTERACTIVE ;
      option text = STRING_TOKEN(0x0039),     value=0, flags= INTERACTIVE | RESET_REQUIRED;
    endoneof;

    string    varid    = BMC_LAN_CONFIG.UserName1,
      prompt   = STRING_TOKEN(0x0033),
      help     = STRING_TOKEN(0x0034),
      flags    = INTERACTIVE | RESET_REQUIRED,
      key      = 2016,
      minsize  = 1,
      maxsize  = 16,
    endstring;

    password  varid    = BMC_LAN_CONFIG.UserPassword1,
    prompt   = STRING_TOKEN(0x0035),
    help     = STRING_TOKEN(0x0036),
    flags  = 0 | INTERACTIVE | RESET_REQUIRED,
    key      = 2037,
    minsize  = 6,
    maxsize  = 20,
    encoding = 0,
    endpassword;

    subtitle text = STRING_TOKEN(0x0037);

    grayoutif TRUE;
    text
      help   = STRING_TOKEN(0x0037),
      text   = STRING_TOKEN(0x0024),
      text   = STRING_TOKEN(0x0026),
      flags  = 0,
      key    = 0;
    endif;

    oneof varid     = BMC_LAN_CONFIG.Privilege2,
      questionid    = 2028,
      prompt        = STRING_TOKEN(0x002A),
      help          = STRING_TOKEN(0x002B),
        option text = STRING_TOKEN(0x002C),     value=1, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x002D),         value=2, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x002E),     value=3, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x002F),value=4, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x0030),     value=0xF, flags = INTERACTIVE | RESET_REQUIRED ;
    endoneof;

    oneof varid     = BMC_LAN_CONFIG.EnableChannelUser2,
    questionid    = 2029,
    prompt        = STRING_TOKEN(0x0031),
    help          = STRING_TOKEN(0x0032),
      option text = STRING_TOKEN(0x0038),      value=1, flags= INTERACTIVE ;
      option text = STRING_TOKEN(0x0039),     value=0, flags= INTERACTIVE | RESET_REQUIRED;
    endoneof;

    string    varid    = BMC_LAN_CONFIG.UserName2,
      prompt   = STRING_TOKEN(0x0033),
      help     = STRING_TOKEN(0x0034),
      flags    = INTERACTIVE | RESET_REQUIRED,
      key      = 2015,
      minsize  = 1,
      maxsize  = 16,
    endstring;

    password  varid    = BMC_LAN_CONFIG.UserPassword2,
    prompt   = STRING_TOKEN(0x0035),
    help     = STRING_TOKEN(0x0036),
    flags  = 0 | INTERACTIVE | RESET_REQUIRED,
    key      = 2038,
    minsize  = 6,
    maxsize  = 20,
    encoding = 0,
    endpassword;

    subtitle text = STRING_TOKEN(0x0037);

    grayoutif TRUE;
    text
      help   = STRING_TOKEN(0x0037),
      text   = STRING_TOKEN(0x0024),
      text   = STRING_TOKEN(0x0027),
      flags  = 0,
      key    = 0;
    endif;

    oneof varid     = BMC_LAN_CONFIG.Privilege3,
    questionid    = 2030,
    prompt        = STRING_TOKEN(0x002A),
    help          = STRING_TOKEN(0x002B),
        option text = STRING_TOKEN(0x002C),     value=1, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x002D),         value=2, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x002E),     value=3, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x002F),value=4, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x0030),     value=0xF, flags = INTERACTIVE | RESET_REQUIRED ;
    endoneof;

    oneof varid     = BMC_LAN_CONFIG.EnableChannelUser3,
    questionid    = 2031,
    prompt        = STRING_TOKEN(0x0031),
    help          = STRING_TOKEN(0x0032),
      option text = STRING_TOKEN(0x0038),      value=1, flags= INTERACTIVE ;
      option text = STRING_TOKEN(0x0039),     value=0, flags= INTERACTIVE | RESET_REQUIRED;
    endoneof;

    string    varid    = BMC_LAN_CONFIG.UserName3,
      prompt   = STRING_TOKEN(0x0033),
      help     = STRING_TOKEN(0x0034),
      flags    = INTERACTIVE | RESET_REQUIRED,
      key      = 2007,
      minsize  = 1,
      maxsize  = 16,
    endstring;

    password  varid    = BMC_LAN_CONFIG.UserPassword3,
    prompt   = STRING_TOKEN(0x0035),
    help     = STRING_TOKEN(0x0036),
    flags  = 0 | INTERACTIVE | RESET_REQUIRED,
    key      = 2039,
    minsize  = 6,
    maxsize  = 20,
    encoding = 0,
    endpassword;

    subtitle text = STRING_TOKEN(0x0037);

    grayoutif TRUE;
    text
      help   = STRING_TOKEN(0x0037),
      text   = STRING_TOKEN(0x0024),
      text   = STRING_TOKEN(0x0028),
      flags  = 0,
      key    = 0;
    endif;

    oneof varid     = BMC_LAN_CONFIG.Privilege4,
    questionid    = 2032,
    prompt        = STRING_TOKEN(0x002A),
    help          = STRING_TOKEN(0x002B),
        option text = STRING_TOKEN(0x002C),     value=1, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x002D),         value=2, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x002E),     value=3, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x002F),value=4, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x0030),     value=0xF, flags = INTERACTIVE | RESET_REQUIRED ;
    endoneof;

    oneof varid     = BMC_LAN_CONFIG.EnableChannelUser4,
    questionid    = 2033,
    prompt        = STRING_TOKEN(0x0031),
    help          = STRING_TOKEN(0x0032),
      option text = STRING_TOKEN(0x0038),      value=1, flags= INTERACTIVE ;
      option text = STRING_TOKEN(0x0039),     value=0, flags= INTERACTIVE | RESET_REQUIRED;
    endoneof;

    string    varid    = BMC_LAN_CONFIG.UserName4,
      prompt   = STRING_TOKEN(0x0033),
      help     = STRING_TOKEN(0x0034),
      flags    = INTERACTIVE | RESET_REQUIRED,
      key      = 2008,
      minsize  = 1,
      maxsize  = 16,
    endstring;

    password  varid    = BMC_LAN_CONFIG.UserPassword4,
    prompt   = STRING_TOKEN(0x0035),
    help     = STRING_TOKEN(0x0036),
    flags  = 0 | INTERACTIVE | RESET_REQUIRED,
    key      = 2040,
    minsize  = 6,
    maxsize  = 20,
    encoding = 0,
    endpassword;

    subtitle text = STRING_TOKEN(0x0037);

    grayoutif TRUE;
    text
      help   = STRING_TOKEN(0x0037),
      text   = STRING_TOKEN(0x0024),
      text   = STRING_TOKEN(0x0029),
      flags  = 0,
      key    = 0;
    endif;

    oneof varid     = BMC_LAN_CONFIG.Privilege5,
    questionid    = 2034,
    prompt        = STRING_TOKEN(0x002A),
    help          = STRING_TOKEN(0x002B),
        option text = STRING_TOKEN(0x002C),     value=1, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x002D),         value=2, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x002E),     value=3, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x002F),value=4, flags= INTERACTIVE ;
        option text = STRING_TOKEN(0x0030),     value=0xF, flags = INTERACTIVE | RESET_REQUIRED ;
    endoneof;

    oneof varid     = BMC_LAN_CONFIG.EnableChannelUser5,
    questionid    = 2035,
    prompt        = STRING_TOKEN(0x0031),
    help          = STRING_TOKEN(0x0032),
      option text = STRING_TOKEN(0x0038),      value=1, flags= INTERACTIVE ;
      option text = STRING_TOKEN(0x0039),     value=0, flags= INTERACTIVE | RESET_REQUIRED;
    endoneof;

    string    varid    = BMC_LAN_CONFIG.UserName5,
      prompt   = STRING_TOKEN(0x0033),
      help     = STRING_TOKEN(0x0034),
      flags    = INTERACTIVE | RESET_REQUIRED,
      key      = 2009,
      minsize  = 1,
      maxsize  = 16,
    endstring;

    password  varid    = BMC_LAN_CONFIG.UserPassword5,
    prompt   = STRING_TOKEN(0x0035),
    help     = STRING_TOKEN(0x0036),
    flags  = 0 | INTERACTIVE | RESET_REQUIRED,
    key      = 2041,
    minsize  = 6,
    maxsize  = 20,
    encoding = 0,
        endpassword;
 endform;
endformset;
