/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:  

  CommonUtils.h
  
Abstract:

  Common utility defines and structure definitions.
  
--*/

#ifndef _COMMON_UTILS_H_
#define _COMMON_UTILS_H_

//
// Basic types
//
typedef unsigned char UINT8;
typedef char INT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;

typedef UINT8 BOOLEAN;
typedef UINT32 STATUS;

#define TRUE            1
#define FALSE           0

#define STATUS_SUCCESS  0
#define STATUS_WARNING  1
#define STATUS_ERROR    2

//
// Linked list of strings
//
typedef struct _STRING_LIST {
  struct _STRING_LIST *Next;
  char                *Str;
} STRING_LIST;

int
CreateGuidList (
  INT8    *OutFileName
  );

#endif // #ifndef _COMMON_UTILS_H_
