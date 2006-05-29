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
  IN  CONST EFI_GUID  *Guid,
  IN  UINT32          CallBackToken,
  IN  VOID            *TokenData,
  IN  UINTN           TokenDataSize
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
  UINT8     u8;
  UINT16    u16;
  UINT32    u32;
  UINT64    u64;
  PCD_TOKEN_NUMBER Token;
  

  LibPcdCallbackOnSet (NULL, PcdToken(PcdTestDynamicUint32), OnsetCallback1);
  
  u32 = 0xafafafaf;
  PcdSet32(PcdTestDynamicUint32, u32);

  u64 = 0xafafafaf00000000;
  PcdSet64(PcdTestDynamicUint64, u64);

  u8 = PcdGet8(PcdTestDynamicUint8);
  u16 = PcdGet16(PcdTestDynamicUint16);


  ASSERT (u8 == 0x01);
  ASSERT (u16 == 0x1234);
  ASSERT (u64 == PcdGet64(PcdTestDynamicUint64));
  ASSERT (u32 == PcdGet32(PcdTestDynamicUint32));


  Token = PCD_INVALID_TOKEN_NUMBER;

  do {
    Token = LibPcdGetNextToken (NULL, Token);
    DebugPrint (EFI_D_ERROR, "Next Token Number is %d\n", Token);
  } while (Token != PCD_INVALID_TOKEN_NUMBER);
  
  
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

