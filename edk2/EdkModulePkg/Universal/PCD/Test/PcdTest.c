/** @file
PCD TEST PEIM

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name: PcdTest.c

**/
#define GUID1 \
  {0xF9349C58, 0xB767, 0x42c8, 0xB3, 0x6B, 0x41, 0x25, 0xDE, 0x3A, 0xEF, 0xEB}

CONST GUID Guid1  = GUID1;


EFI_STATUS
EFIAPI
OnsetCallback1 (
  IN  UINT32    CallBackToken,
  IN  VOID      *TokenData,
  IN  UINTN     TokenDataSize
  )
{
  DebugPrint (0x80000000, "In CallbackOnSet %x %d\n", * ((UINT32 *)TokenData), TokenDataSize);    
  return EFI_SUCCESS;
}


VOID
DoTest(
  VOID
  )
{
  PCD_TOKEN_NUMBER tn;
  UINTN Size;
  VOID * Ptr;
  UINT32 Uint32;
  UINT32 Uint32a;
  UINT64 Uint64;
  UINT64 Uint64a;
  INTN i;

  tn = 0x00001000;

  Size = LibPcdGetSize (tn);
  Ptr = LibPcdGetPtr (tn); /* a:RW;2880;512!e:RW;262144;512 */
  
  tn = 0x00001001;
  Size = LibPcdGetSize (tn); /* FW;40960;512 */
  
  tn = 0x00001002;
  Size = LibPcdGetSize (tn); /* FW;40960;512 */
  Ptr = LibPcdGetPtr (tn);
  
  LibPcdSetSku (0x0a);
  tn = 0x2233;
  Uint64 = LibPcdGet64 (tn);
  
  LibPcdSetSku (0x0b);
  Uint64 = LibPcdGet64 (tn);
  
  LibPcdSetSku (0x0c);
  Uint64a = LibPcdGet64 (tn);
  
  LibPcdSetSku (0);
  tn = 0x2233;
  Uint64 = LibPcdGet64 (tn);
  
  
  tn = 0xfaceface;
  Size = LibPcdGetExSize (&Guid1, tn);
  Uint32 = LibPcdGetEx32 (&Guid1, tn);
  
  LibPcdCallBackOnSet (&Guid1, tn, OnsetCallback1);
  
  LibPcdCancelCallBackOnSet (&Guid1, tn, OnsetCallback1);
  
  for (i = 0; i < 2; i++) {
    Uint32a = LibPcdSetEx32 (&Guid1, tn, Uint32 + i);
    DebugPrint (0x80000000, "%x\n", Uint32a);
  }
  
  
  
  Uint32 = LibPcdGet32 (tn);
  
  
  return;
}

EFI_STATUS
EFIAPI
PcdTestPeimInit (
  IN EFI_FFS_FILE_HEADER      *FfsHeader,
  IN EFI_PEI_SERVICES         **PeiServices
  )
{

  DoTest();
  
  return EFI_SUCCESS;
}

