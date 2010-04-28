/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  PrintWidth.h

Abstract:

  Unicde option for generic width.
  CHAR_W is Unicode
  STRING_W is L""

--*/

#ifndef _PRINT_WIDTH_H_
#define _PRINT_WIDTH_H_

typedef CHAR16  CHAR_W;
#define STRING_W(_s)                                  L##_s

#define USPrint(Buffer, BufferSize, Format)           SPrint (Buffer, BufferSize, Format)
#define UvSPrint(Buffer, BufferSize, Format, Marker)  VSPrint (Buffer, BufferSize, Format, Marker)

#include "EfiCommonLib.h"

#endif
