/**********************************************************************
  regenc.c -  Oniguruma (regular expression library)
**********************************************************************/
/*-
 * Copyright (c) 2002-2007  K.Kosako  <sndgk393 AT ybb DOT ne DOT jp>
 * All rights reserved.
 * 
 * (C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "regint.h"

OnigEncoding OnigEncDefaultCharEncoding = ONIG_ENCODING_INIT_DEFAULT;

extern int
onigenc_init(void)
{
  return 0;
}

extern OnigEncoding
onigenc_get_default_encoding(void)
{
  return OnigEncDefaultCharEncoding;
}

extern int
onigenc_set_default_encoding(OnigEncoding enc)
{
  OnigEncDefaultCharEncoding = enc;
  return 0;
}

extern UChar*
onigenc_get_right_adjust_char_head(OnigEncoding enc, const UChar* start, const UChar* s)
{
  UChar* p = ONIGENC_LEFT_ADJUST_CHAR_HEAD(enc, start, s);
  if (p < s) {
    p += enclen(enc, p);
  }
  return p;
}

extern UChar*
onigenc_get_right_adjust_char_head_with_prev(OnigEncoding enc,
				   const UChar* start, const UChar* s, const UChar** prev)
{
  UChar* p = ONIGENC_LEFT_ADJUST_CHAR_HEAD(enc, start, s);

  if (p < s) {
    if (prev) *prev = (const UChar* )p;
    p += enclen(enc, p);
  }
  else {
    if (prev) *prev = (const UChar* )NULL; /* Sorry */
  }
  return p;
}

extern UChar*
onigenc_get_prev_char_head(OnigEncoding enc, const UChar* start, const UChar* s)
{
  if (s <= start)
    return (UChar* )NULL;

  return ONIGENC_LEFT_ADJUST_CHAR_HEAD(enc, start, s - 1);
}

extern UChar*
onigenc_step_back(OnigEncoding enc, const UChar* start, const UChar* s, int n)
{
  while (ONIG_IS_NOT_NULL(s) && n-- > 0) {
    if (s <= start)
      return (UChar* )NULL;

    s = ONIGENC_LEFT_ADJUST_CHAR_HEAD(enc, start, s - 1);
  }
  return (UChar* )s;
}

extern UChar*
onigenc_step(OnigEncoding enc, const UChar* p, const UChar* end, int n)
{
  UChar* q = (UChar* )p;
  while (n-- > 0) {
    q += ONIGENC_MBC_ENC_LEN(enc, q);
  }
  return (q <= end ? q : NULL);
}

extern int
onigenc_strlen(OnigEncoding enc, const UChar* p, const UChar* end)
{
  int n = 0;
  UChar* q = (UChar* )p;
  
  while (q < end) {
    q += ONIGENC_MBC_ENC_LEN(enc, q);
    n++;
  }
  return n;
}

extern int
onigenc_strlen_null(OnigEncoding enc, const UChar* s)
{
  int n = 0;
  UChar* p = (UChar* )s;
  
  while (1) {
    if (*p == '\0') {
      UChar* q;
      int len = ONIGENC_MBC_MINLEN(enc);

      if (len == 1) return n;
      q = p + 1;
      while (len > 1) {
        if (*q != '\0') break;
        q++;
        len--;
      }
      if (len == 1) return n;
    }
    p += ONIGENC_MBC_ENC_LEN(enc, p);
    n++;
  }
}

extern int
onigenc_str_bytelen_null(OnigEncoding enc, const UChar* s)
{
  UChar* start = (UChar* )s;
  UChar* p = (UChar* )s;

  while (1) {
    if (*p == '\0') {
      UChar* q;
      int len = ONIGENC_MBC_MINLEN(enc);

      if (len == 1) return (int )(p - start);
      q = p + 1;
      while (len > 1) {
        if (*q != '\0') break;
        q++;
        len--;
      }
      if (len == 1) return (int )(p - start);
    }
    p += ONIGENC_MBC_ENC_LEN(enc, p);
  }
}

const UChar OnigEncAsciiToLowerCaseTable[] = {
  0000, 0001, 0002, 0003, 0004, 0005, 0006, 0007,
  0010, 0011, 0012, 0013, 0014, 0015, 0016, 0017,
  0020, 0021, 0022, 0023, 0024, 0025, 0026, 0027,
  0030, 0031, 0032, 0033, 0034, 0035, 0036, 0037,
  0040, 0041, 0042, 0043, 0044, 0045, 0046, 0047,
  0050, 0051, 0052, 0053, 0054, 0055, 0056, 0057,
  0060, 0061, 0062, 0063, 0064, 0065, 0066, 0067,
  0070, 0071, 0072, 0073, 0074, 0075, 0076, 0077,
  0100, 0141, 0142, 0143, 0144, 0145, 0146, 0147,
  0150, 0151, 0152, 0153, 0154, 0155, 0156, 0157,
  0160, 0161, 0162, 0163, 0164, 0165, 0166, 0167,
  0170, 0171, 0172, 0133, 0134, 0135, 0136, 0137,
  0140, 0141, 0142, 0143, 0144, 0145, 0146, 0147,
  0150, 0151, 0152, 0153, 0154, 0155, 0156, 0157,
  0160, 0161, 0162, 0163, 0164, 0165, 0166, 0167,
  0170, 0171, 0172, 0173, 0174, 0175, 0176, 0177,
  0200, 0201, 0202, 0203, 0204, 0205, 0206, 0207,
  0210, 0211, 0212, 0213, 0214, 0215, 0216, 0217,
  0220, 0221, 0222, 0223, 0224, 0225, 0226, 0227,
  0230, 0231, 0232, 0233, 0234, 0235, 0236, 0237,
  0240, 0241, 0242, 0243, 0244, 0245, 0246, 0247,
  0250, 0251, 0252, 0253, 0254, 0255, 0256, 0257,
  0260, 0261, 0262, 0263, 0264, 0265, 0266, 0267,
  0270, 0271, 0272, 0273, 0274, 0275, 0276, 0277,
  0300, 0301, 0302, 0303, 0304, 0305, 0306, 0307,
  0310, 0311, 0312, 0313, 0314, 0315, 0316, 0317,
  0320, 0321, 0322, 0323, 0324, 0325, 0326, 0327,
  0330, 0331, 0332, 0333, 0334, 0335, 0336, 0337,
  0340, 0341, 0342, 0343, 0344, 0345, 0346, 0347,
  0350, 0351, 0352, 0353, 0354, 0355, 0356, 0357,
  0360, 0361, 0362, 0363, 0364, 0365, 0366, 0367,
  0370, 0371, 0372, 0373, 0374, 0375, 0376, 0377,
};

#ifdef USE_UPPER_CASE_TABLE
const UChar OnigEncAsciiToUpperCaseTable[256] = {
  0000, 0001, 0002, 0003, 0004, 0005, 0006, 0007,
  0010, 0011, 0012, 0013, 0014, 0015, 0016, 0017,
  0020, 0021, 0022, 0023, 0024, 0025, 0026, 0027,
  0030, 0031, 0032, 0033, 0034, 0035, 0036, 0037,
  0040, 0041, 0042, 0043, 0044, 0045, 0046, 0047,
  0050, 0051, 0052, 0053, 0054, 0055, 0056, 0057,
  0060, 0061, 0062, 0063, 0064, 0065, 0066, 0067,
  0070, 0071, 0072, 0073, 0074, 0075, 0076, 0077,
  0100, 0101, 0102, 0103, 0104, 0105, 0106, 0107,
  0110, 0111, 0112, 0113, 0114, 0115, 0116, 0117,
  0120, 0121, 0122, 0123, 0124, 0125, 0126, 0127,
  0130, 0131, 0132, 0133, 0134, 0135, 0136, 0137,
  0140, 0101, 0102, 0103, 0104, 0105, 0106, 0107,
  0110, 0111, 0112, 0113, 0114, 0115, 0116, 0117,
  0120, 0121, 0122, 0123, 0124, 0125, 0126, 0127,
  0130, 0131, 0132, 0173, 0174, 0175, 0176, 0177,
  0200, 0201, 0202, 0203, 0204, 0205, 0206, 0207,
  0210, 0211, 0212, 0213, 0214, 0215, 0216, 0217,
  0220, 0221, 0222, 0223, 0224, 0225, 0226, 0227,
  0230, 0231, 0232, 0233, 0234, 0235, 0236, 0237,
  0240, 0241, 0242, 0243, 0244, 0245, 0246, 0247,
  0250, 0251, 0252, 0253, 0254, 0255, 0256, 0257,
  0260, 0261, 0262, 0263, 0264, 0265, 0266, 0267,
  0270, 0271, 0272, 0273, 0274, 0275, 0276, 0277,
  0300, 0301, 0302, 0303, 0304, 0305, 0306, 0307,
  0310, 0311, 0312, 0313, 0314, 0315, 0316, 0317,
  0320, 0321, 0322, 0323, 0324, 0325, 0326, 0327,
  0330, 0331, 0332, 0333, 0334, 0335, 0336, 0337,
  0340, 0341, 0342, 0343, 0344, 0345, 0346, 0347,
  0350, 0351, 0352, 0353, 0354, 0355, 0356, 0357,
  0360, 0361, 0362, 0363, 0364, 0365, 0366, 0367,
  0370, 0371, 0372, 0373, 0374, 0375, 0376, 0377,
};
#endif

const unsigned short OnigEncAsciiCtypeTable[256] = {
  0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008,
  0x4008, 0x420c, 0x4209, 0x4208, 0x4208, 0x4208, 0x4008, 0x4008,
  0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008,
  0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008, 0x4008,
  0x4284, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0,
  0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0,
  0x78b0, 0x78b0, 0x78b0, 0x78b0, 0x78b0, 0x78b0, 0x78b0, 0x78b0,
  0x78b0, 0x78b0, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x41a0,
  0x41a0, 0x7ca2, 0x7ca2, 0x7ca2, 0x7ca2, 0x7ca2, 0x7ca2, 0x74a2,
  0x74a2, 0x74a2, 0x74a2, 0x74a2, 0x74a2, 0x74a2, 0x74a2, 0x74a2,
  0x74a2, 0x74a2, 0x74a2, 0x74a2, 0x74a2, 0x74a2, 0x74a2, 0x74a2,
  0x74a2, 0x74a2, 0x74a2, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x51a0,
  0x41a0, 0x78e2, 0x78e2, 0x78e2, 0x78e2, 0x78e2, 0x78e2, 0x70e2,
  0x70e2, 0x70e2, 0x70e2, 0x70e2, 0x70e2, 0x70e2, 0x70e2, 0x70e2,
  0x70e2, 0x70e2, 0x70e2, 0x70e2, 0x70e2, 0x70e2, 0x70e2, 0x70e2,
  0x70e2, 0x70e2, 0x70e2, 0x41a0, 0x41a0, 0x41a0, 0x41a0, 0x4008,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

const UChar OnigEncISO_8859_1_ToLowerCaseTable[256] = {
  0000, 0001, 0002, 0003, 0004, 0005, 0006, 0007,
  0010, 0011, 0012, 0013, 0014, 0015, 0016, 0017,
  0020, 0021, 0022, 0023, 0024, 0025, 0026, 0027,
  0030, 0031, 0032, 0033, 0034, 0035, 0036, 0037,
  0040, 0041, 0042, 0043, 0044, 0045, 0046, 0047,
  0050, 0051, 0052, 0053, 0054, 0055, 0056, 0057,
  0060, 0061, 0062, 0063, 0064, 0065, 0066, 0067,
  0070, 0071, 0072, 0073, 0074, 0075, 0076, 0077,
  0100, 0141, 0142, 0143, 0144, 0145, 0146, 0147,
  0150, 0151, 0152, 0153, 0154, 0155, 0156, 0157,
  0160, 0161, 0162, 0163, 0164, 0165, 0166, 0167,
  0170, 0171, 0172, 0133, 0134, 0135, 0136, 0137,
  0140, 0141, 0142, 0143, 0144, 0145, 0146, 0147,
  0150, 0151, 0152, 0153, 0154, 0155, 0156, 0157,
  0160, 0161, 0162, 0163, 0164, 0165, 0166, 0167,
  0170, 0171, 0172, 0173, 0174, 0175, 0176, 0177,
  0200, 0201, 0202, 0203, 0204, 0205, 0206, 0207,
  0210, 0211, 0212, 0213, 0214, 0215, 0216, 0217,
  0220, 0221, 0222, 0223, 0224, 0225, 0226, 0227,
  0230, 0231, 0232, 0233, 0234, 0235, 0236, 0237,
  0240, 0241, 0242, 0243, 0244, 0245, 0246, 0247,
  0250, 0251, 0252, 0253, 0254, 0255, 0256, 0257,
  0260, 0261, 0262, 0263, 0264, 0265, 0266, 0267,
  0270, 0271, 0272, 0273, 0274, 0275, 0276, 0277,
  0340, 0341, 0342, 0343, 0344, 0345, 0346, 0347,
  0350, 0351, 0352, 0353, 0354, 0355, 0356, 0357,
  0360, 0361, 0362, 0363, 0364, 0365, 0366, 0327,
  0370, 0371, 0372, 0373, 0374, 0375, 0376, 0337,
  0340, 0341, 0342, 0343, 0344, 0345, 0346, 0347,
  0350, 0351, 0352, 0353, 0354, 0355, 0356, 0357,
  0360, 0361, 0362, 0363, 0364, 0365, 0366, 0367,
  0370, 0371, 0372, 0373, 0374, 0375, 0376, 0377
};

#ifdef USE_UPPER_CASE_TABLE
const UChar OnigEncISO_8859_1_ToUpperCaseTable[256] = {
  0000, 0001, 0002, 0003, 0004, 0005, 0006, 0007,
  0010, 0011, 0012, 0013, 0014, 0015, 0016, 0017,
  0020, 0021, 0022, 0023, 0024, 0025, 0026, 0027,
  0030, 0031, 0032, 0033, 0034, 0035, 0036, 0037,
  0040, 0041, 0042, 0043, 0044, 0045, 0046, 0047,
  0050, 0051, 0052, 0053, 0054, 0055, 0056, 0057,
  0060, 0061, 0062, 0063, 0064, 0065, 0066, 0067,
  0070, 0071, 0072, 0073, 0074, 0075, 0076, 0077,
  0100, 0101, 0102, 0103, 0104, 0105, 0106, 0107,
  0110, 0111, 0112, 0113, 0114, 0115, 0116, 0117,
  0120, 0121, 0122, 0123, 0124, 0125, 0126, 0127,
  0130, 0131, 0132, 0133, 0134, 0135, 0136, 0137,
  0140, 0101, 0102, 0103, 0104, 0105, 0106, 0107,
  0110, 0111, 0112, 0113, 0114, 0115, 0116, 0117,
  0120, 0121, 0122, 0123, 0124, 0125, 0126, 0127,
  0130, 0131, 0132, 0173, 0174, 0175, 0176, 0177,
  0200, 0201, 0202, 0203, 0204, 0205, 0206, 0207,
  0210, 0211, 0212, 0213, 0214, 0215, 0216, 0217,
  0220, 0221, 0222, 0223, 0224, 0225, 0226, 0227,
  0230, 0231, 0232, 0233, 0234, 0235, 0236, 0237,
  0240, 0241, 0242, 0243, 0244, 0245, 0246, 0247,
  0250, 0251, 0252, 0253, 0254, 0255, 0256, 0257,
  0260, 0261, 0262, 0263, 0264, 0265, 0266, 0267,
  0270, 0271, 0272, 0273, 0274, 0275, 0276, 0277,
  0300, 0301, 0302, 0303, 0304, 0305, 0306, 0307,
  0310, 0311, 0312, 0313, 0314, 0315, 0316, 0317,
  0320, 0321, 0322, 0323, 0324, 0325, 0326, 0327,
  0330, 0331, 0332, 0333, 0334, 0335, 0336, 0337,
  0300, 0301, 0302, 0303, 0304, 0305, 0306, 0307,
  0310, 0311, 0312, 0313, 0314, 0315, 0316, 0317,
  0320, 0321, 0322, 0323, 0324, 0325, 0326, 0367,
  0330, 0331, 0332, 0333, 0334, 0335, 0336, 0377,
};
#endif

extern void
onigenc_set_default_caseconv_table(const UChar* table ARG_UNUSED)
{
  /* nothing */
  /* obsoleted. */
}

extern UChar*
onigenc_get_left_adjust_char_head(OnigEncoding enc, const UChar* start, const UChar* s)
{
  return ONIGENC_LEFT_ADJUST_CHAR_HEAD(enc, start, s);
}

const OnigPairCaseFoldCodes OnigAsciiLowerMap[] = {
  { 0x41, 0x61 },
  { 0x42, 0x62 },
  { 0x43, 0x63 },
  { 0x44, 0x64 },
  { 0x45, 0x65 },
  { 0x46, 0x66 },
  { 0x47, 0x67 },
  { 0x48, 0x68 },
  { 0x49, 0x69 },
  { 0x4a, 0x6a },
  { 0x4b, 0x6b },
  { 0x4c, 0x6c },
  { 0x4d, 0x6d },
  { 0x4e, 0x6e },
  { 0x4f, 0x6f },
  { 0x50, 0x70 },
  { 0x51, 0x71 },
  { 0x52, 0x72 },
  { 0x53, 0x73 },
  { 0x54, 0x74 },
  { 0x55, 0x75 },
  { 0x56, 0x76 },
  { 0x57, 0x77 },
  { 0x58, 0x78 },
  { 0x59, 0x79 },
  { 0x5a, 0x7a }
};

extern int
onigenc_ascii_apply_all_case_fold(OnigCaseFoldType flag ARG_UNUSED,
				  OnigApplyAllCaseFoldFunc f, void* arg)
{
  OnigCodePoint code;
  int i, r;

  for (i = 0;
       i < (int )(sizeof(OnigAsciiLowerMap)/sizeof(OnigPairCaseFoldCodes));
       i++) {
    code = OnigAsciiLowerMap[i].to;
    r = (*f)(OnigAsciiLowerMap[i].from, &code, 1, arg);
    if (r != 0) return r;

    code = OnigAsciiLowerMap[i].from;
    r = (*f)(OnigAsciiLowerMap[i].to, &code, 1, arg);
    if (r != 0) return r;
  }

  return 0;
}

extern int
onigenc_ascii_get_case_fold_codes_by_str(OnigCaseFoldType flag ARG_UNUSED,
	 const OnigUChar* p, const OnigUChar* end ARG_UNUSED,
	 OnigCaseFoldCodeItem items[])
{
  if (0x41 <= *p && *p <= 0x5a) {
    items[0].byte_len = 1;
    items[0].code_len = 1;
    items[0].code[0] = (OnigCodePoint )(*p + 0x20);
    return 1;
  }
  else if (0x61 <= *p && *p <= 0x7a) {
    items[0].byte_len = 1;
    items[0].code_len = 1;
    items[0].code[0] = (OnigCodePoint )(*p - 0x20);
    return 1;
  }
  else
    return 0;
}

static int
ss_apply_all_case_fold(OnigCaseFoldType flag ARG_UNUSED,
		       OnigApplyAllCaseFoldFunc f, void* arg)
{
  static OnigCodePoint ss[] = { 0x73, 0x73 };

  return (*f)((OnigCodePoint )0xdf, ss, 2, arg);
}

extern int
onigenc_apply_all_case_fold_with_map(int map_size,
    const OnigPairCaseFoldCodes map[],
    int ess_tsett_flag, OnigCaseFoldType flag,
    OnigApplyAllCaseFoldFunc f, void* arg)
{
  OnigCodePoint code;
  int i, r;

  r = onigenc_ascii_apply_all_case_fold(flag, f, arg);
  if (r != 0) return r;

  for (i = 0; i < map_size; i++) {
    code = map[i].to;
    r = (*f)(map[i].from, &code, 1, arg);
    if (r != 0) return r;

    code = map[i].from;
    r = (*f)(map[i].to, &code, 1, arg);
    if (r != 0) return r;
  }

  if (ess_tsett_flag != 0)
    return ss_apply_all_case_fold(flag, f, arg);

  return 0;
}

extern int
onigenc_get_case_fold_codes_by_str_with_map(int map_size,
    const OnigPairCaseFoldCodes map[],
    int ess_tsett_flag, OnigCaseFoldType flag ARG_UNUSED,
    const OnigUChar* p, const OnigUChar* end, OnigCaseFoldCodeItem items[])
{
  if (0x41 <= *p && *p <= 0x5a) {
    items[0].byte_len = 1;
    items[0].code_len = 1;
    items[0].code[0] = (OnigCodePoint )(*p + 0x20);
    if (*p == 0x53 && ess_tsett_flag != 0 && end > p + 1
	&& (*(p+1) == 0x53 || *(p+1) == 0x73)) {
      /* SS */
      items[1].byte_len = 2;
      items[1].code_len = 1;
      items[1].code[0] = (OnigCodePoint )0xdf;
      return 2;
    }
    else
      return 1;
  }
  else if (0x61 <= *p && *p <= 0x7a) {
    items[0].byte_len = 1;
    items[0].code_len = 1;
    items[0].code[0] = (OnigCodePoint )(*p - 0x20);
    if (*p == 0x73 && ess_tsett_flag != 0 && end > p + 1
	&& (*(p+1) == 0x73 || *(p+1) == 0x53)) {
      /* ss */
      items[1].byte_len = 2;
      items[1].code_len = 1;
      items[1].code[0] = (OnigCodePoint )0xdf;
      return 2;
    }
    else
      return 1;
  }
  else if (*p == 0xdf && ess_tsett_flag != 0) {
    items[0].byte_len = 1;
    items[0].code_len = 2;
    items[0].code[0] = (OnigCodePoint )'s';
    items[0].code[1] = (OnigCodePoint )'s';

    items[1].byte_len = 1;
    items[1].code_len = 2;
    items[1].code[0] = (OnigCodePoint )'S';
    items[1].code[1] = (OnigCodePoint )'S';

    items[2].byte_len = 1;
    items[2].code_len = 2;
    items[2].code[0] = (OnigCodePoint )'s';
    items[2].code[1] = (OnigCodePoint )'S';

    items[3].byte_len = 1;
    items[3].code_len = 2;
    items[3].code[0] = (OnigCodePoint )'S';
    items[3].code[1] = (OnigCodePoint )'s';

    return 4;
  }
  else {
    int i;

    for (i = 0; i < map_size; i++) {
      if (*p == map[i].from) {
	items[0].byte_len = 1;
	items[0].code_len = 1;
	items[0].code[0] = map[i].to;
	return 1;
      }
      else if (*p == map[i].to) {
	items[0].byte_len = 1;
	items[0].code_len = 1;
	items[0].code[0] = map[i].from;
	return 1;
      }
    }
  }

  return 0;
}


extern int
onigenc_not_support_get_ctype_code_range(OnigCtype ctype ARG_UNUSED,
	 OnigCodePoint* sb_out ARG_UNUSED,
	 const OnigCodePoint* ranges[] ARG_UNUSED)
{
  return ONIG_NO_SUPPORT_CONFIG;
}

extern int
onigenc_is_mbc_newline_0x0a(const UChar* p, const UChar* end)
{
  if (p < end) {
    if (*p == 0x0a) return 1;
  }
  return 0;
}

/* for single byte encodings */
extern int
onigenc_ascii_mbc_case_fold(OnigCaseFoldType flag ARG_UNUSED, const UChar** p,
	    const UChar*end ARG_UNUSED, UChar* lower)
{
  *lower = ONIGENC_ASCII_CODE_TO_LOWER_CASE(**p);

  (*p)++;
  return 1; /* return byte length of converted char to lower */
}

#if 0
extern int
onigenc_ascii_is_mbc_ambiguous(OnigCaseFoldType flag,
			       const UChar** pp, const UChar* end)
{
  const UChar* p = *pp;

  (*pp)++;
  return ONIGENC_IS_ASCII_CODE_CASE_AMBIG(*p);
}
#endif

extern int
onigenc_single_byte_mbc_enc_len(const UChar* p ARG_UNUSED)
{
  return 1;
}

extern OnigCodePoint
onigenc_single_byte_mbc_to_code(const UChar* p, const UChar* end ARG_UNUSED)
{
  return (OnigCodePoint )(*p);
}

extern int
onigenc_single_byte_code_to_mbclen(OnigCodePoint code ARG_UNUSED)
{
  return (code < 0x100 ? 1 : ONIGERR_INVALID_CODE_POINT_VALUE);
}

extern int
onigenc_single_byte_code_to_mbc(OnigCodePoint code, UChar *buf)
{
  *buf = (UChar )(code & 0xff);
  return 1;
}

extern UChar*
onigenc_single_byte_left_adjust_char_head(const UChar* start ARG_UNUSED,
					  const UChar* s)
{
  return (UChar* )s;
}

extern int
onigenc_always_true_is_allowed_reverse_match(const UChar* s   ARG_UNUSED,
					     const UChar* end ARG_UNUSED)
{
  return TRUE;
}

extern int
onigenc_always_false_is_allowed_reverse_match(const UChar* s   ARG_UNUSED,
					      const UChar* end ARG_UNUSED)
{
  return FALSE;
}

extern OnigCodePoint
onigenc_mbn_mbc_to_code(OnigEncoding enc, const UChar* p, const UChar* end)
{
  int c, i, len;
  OnigCodePoint n;

  len = enclen(enc, p);
  n = (OnigCodePoint )(*p++);
  if (len == 1) return n;

  for (i = 1; i < len; i++) {
    if (p >= end) break;
    c = *p++;
    n <<= 8;  n += c;
  }
  return n;
}

extern int
onigenc_mbn_mbc_case_fold(OnigEncoding enc, OnigCaseFoldType flag ARG_UNUSED,
                          const UChar** pp, const UChar* end ARG_UNUSED,
			  UChar* lower)
{
  int len;
  const UChar *p = *pp;

  if (ONIGENC_IS_MBC_ASCII(p)) {
    *lower = ONIGENC_ASCII_CODE_TO_LOWER_CASE(*p);
    (*pp)++;
    return 1;
  }
  else {
    int i;

    len = enclen(enc, p);
    for (i = 0; i < len; i++) {
      *lower++ = *p++;
    }
    (*pp) += len;
    return len; /* return byte length of converted to lower char */
  }
}

#if 0
extern int
onigenc_mbn_is_mbc_ambiguous(OnigEncoding enc, OnigCaseFoldType flag,
                             const UChar** pp, const UChar* end)
{
  const UChar* p = *pp;

  if (ONIGENC_IS_MBC_ASCII(p)) {
    (*pp)++;
    return ONIGENC_IS_ASCII_CODE_CASE_AMBIG(*p);
  }

  (*pp) += enclen(enc, p);
  return FALSE;
}
#endif

extern int
onigenc_mb2_code_to_mbclen(OnigCodePoint code)
{
  if ((code & 0xff00) != 0) return 2;
  else return 1;
}

extern int
onigenc_mb4_code_to_mbclen(OnigCodePoint code)
{
       if ((code & 0xff000000) != 0) return 4;
  else if ((code & 0xff0000) != 0) return 3;
  else if ((code & 0xff00) != 0) return 2;
  else return 1;
}

extern int
onigenc_mb2_code_to_mbc(OnigEncoding enc, OnigCodePoint code, UChar *buf)
{
  UChar *p = buf;

  if ((code & 0xff00) != 0) {
    *p++ = (UChar )((code >>  8) & 0xff);
  }
  *p++ = (UChar )(code & 0xff);

#if 1
  if (enclen(enc, buf) != (p - buf))
    return ONIGERR_INVALID_CODE_POINT_VALUE;
#endif
  return (int)(p - buf);
}

extern int
onigenc_mb4_code_to_mbc(OnigEncoding enc, OnigCodePoint code, UChar *buf)
{
  UChar *p = buf;

  if ((code & 0xff000000) != 0) {
    *p++ = (UChar )((code >> 24) & 0xff);
  }
  if ((code & 0xff0000) != 0 || p != buf) {
    *p++ = (UChar )((code >> 16) & 0xff);
  }
  if ((code & 0xff00) != 0 || p != buf) {
    *p++ = (UChar )((code >> 8) & 0xff);
  }
  *p++ = (UChar )(code & 0xff);

#if 1
  if (enclen(enc, buf) != (p - buf))
    return ONIGERR_INVALID_CODE_POINT_VALUE;
#endif
  return (int)(p - buf);
}

extern int
onigenc_minimum_property_name_to_ctype(OnigEncoding enc, UChar* p, UChar* end)
{
  static PosixBracketEntryType PBS[] = {
    { (UChar* )"Alnum",  ONIGENC_CTYPE_ALNUM,  5 },
    { (UChar* )"Alpha",  ONIGENC_CTYPE_ALPHA,  5 },
    { (UChar* )"Blank",  ONIGENC_CTYPE_BLANK,  5 },
    { (UChar* )"Cntrl",  ONIGENC_CTYPE_CNTRL,  5 },
    { (UChar* )"Digit",  ONIGENC_CTYPE_DIGIT,  5 },
    { (UChar* )"Graph",  ONIGENC_CTYPE_GRAPH,  5 },
    { (UChar* )"Lower",  ONIGENC_CTYPE_LOWER,  5 },
    { (UChar* )"Print",  ONIGENC_CTYPE_PRINT,  5 },
    { (UChar* )"Punct",  ONIGENC_CTYPE_PUNCT,  5 },
    { (UChar* )"Space",  ONIGENC_CTYPE_SPACE,  5 },
    { (UChar* )"Upper",  ONIGENC_CTYPE_UPPER,  5 },
    { (UChar* )"XDigit", ONIGENC_CTYPE_XDIGIT, 6 },
    { (UChar* )"ASCII",  ONIGENC_CTYPE_ASCII,  5 },
    { (UChar* )"Word",   ONIGENC_CTYPE_WORD,   4 },
    { (UChar* )NULL, -1, 0 }
  };

  PosixBracketEntryType *pb;
  int len;

  len = onigenc_strlen(enc, p, end);
  for (pb = PBS; IS_NOT_NULL(pb->name); pb++) {
    if (len == pb->len &&
        onigenc_with_ascii_strncmp(enc, p, end, pb->name, pb->len) == 0)
      return pb->ctype;
  }

  return ONIGERR_INVALID_CHAR_PROPERTY_NAME;
}

extern int
onigenc_mb2_is_code_ctype(OnigEncoding enc, OnigCodePoint code,
			  unsigned int ctype)
{
  if (code < 128)
    return ONIGENC_IS_ASCII_CODE_CTYPE(code, ctype);
  else {
    if (CTYPE_IS_WORD_GRAPH_PRINT(ctype)) {
      return (ONIGENC_CODE_TO_MBCLEN(enc, code) > 1 ? TRUE : FALSE);
    }
  }

  return FALSE;
}

extern int
onigenc_mb4_is_code_ctype(OnigEncoding enc, OnigCodePoint code,
			  unsigned int ctype)
{
  if (code < 128)
    return ONIGENC_IS_ASCII_CODE_CTYPE(code, ctype);
  else {
    if (CTYPE_IS_WORD_GRAPH_PRINT(ctype)) {
      return (ONIGENC_CODE_TO_MBCLEN(enc, code) > 1 ? TRUE : FALSE);
    }
  }

  return FALSE;
}

extern int
onigenc_with_ascii_strncmp(OnigEncoding enc, const UChar* p, const UChar* end,
                           const UChar* sascii /* ascii */, int n)
{
  int x, c;

  while (n-- > 0) {
    if (p >= end) return (int )(*sascii);

    c = (int )ONIGENC_MBC_TO_CODE(enc, p, end);
    x = *sascii - c;
    if (x) return x;

    sascii++;
    p += enclen(enc, p);
  }
  return 0;
}

/* Property management */
static int
resize_property_list(int new_size, const OnigCodePoint*** plist, int* psize)
{
  int size;
  const OnigCodePoint **list = *plist;

  size = sizeof(OnigCodePoint*) * new_size;
  if (IS_NULL(list)) {
    list = (const OnigCodePoint** )xmalloc(size);
  }
  else {
    list = (const OnigCodePoint** )xrealloc((void* )list, size, *psize * sizeof(OnigCodePoint*));
  }

  if (IS_NULL(list)) return ONIGERR_MEMORY;

  *plist = list;
  *psize = new_size;

  return 0;
}

extern int
onigenc_property_list_add_property(UChar* name, const OnigCodePoint* prop,
     hash_table_type **table, const OnigCodePoint*** plist, int *pnum,
     int *psize)
{
#define PROP_INIT_SIZE     16

  int r;

  if (*psize <= *pnum) {
    int new_size = (*psize == 0 ? PROP_INIT_SIZE : *psize * 2);
    r = resize_property_list(new_size, plist, psize);
    if (r != 0) return r;
  }

  (*plist)[*pnum] = prop;

  if (ONIG_IS_NULL(*table)) {
    *table = onig_st_init_strend_table_with_size(PROP_INIT_SIZE);
    if (ONIG_IS_NULL(*table)) return ONIGERR_MEMORY;
  }

  *pnum = *pnum + 1;
  onig_st_insert_strend(*table, name, name + strlen_s((char* )name, MAX_STRING_SIZE),
			(hash_data_type )(*pnum + ONIGENC_MAX_STD_CTYPE));
  return 0;
}

extern int
onigenc_property_list_init(int (*f)(void))
{
  int r;

  THREAD_ATOMIC_START;

  r = f();

  THREAD_ATOMIC_END;
  return r;
}
