/** @file
  After S3 SMM initialization is done and before S3 boot script is executed,
  this GUID is installed as PPI in PEI and protocol in SMM environment. 
  It allows for PEIMs or SMM drivers to hook this point and do the required tasks.

  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __S3_SMM_INIT_DONE_H__
#define __S3_SMM_INIT_DONE_H__

#define EDKII_S3_SMM_INIT_DONE_GUID \
  { \
    0x8f9d4825, 0x797d, 0x48fc, { 0x84, 0x71, 0x84, 0x50, 0x25, 0x79, 0x2e, 0xf6 } \
  }

extern EFI_GUID gEdkiiS3SmmInitDoneGuid;

#endif
