/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module name:
  hton.h

Abstract:
  Byte swapping macros.

--*/

#ifndef _HTON_H_
#define _HTON_H_

//
// Only Intel order functions are defined at this time.
//
#define HTONS(v)  (UINT16) ((((v) << 8) & 0xff00) + (((v) >> 8) & 0x00ff))

#define HTONL(v) \
  (UINT32) ((((v) << 24) & 0xff000000) + (((v) << 8) & 0x00ff0000) + (((v) >> 8) & 0x0000ff00) + (((v) >> 24) & 0x000000ff))

#define HTONLL(v) swap64 (v)

#define U8PTR(na) ((UINT8 *) &(na))

#define NTOHS(ns) ((UINT16) (((*U8PTR (ns)) << 8) +*(U8PTR (ns) + 1)))

#define NTOHL(ns) \
    ((UINT32) (((*U8PTR (ns)) << 24) + ((*(U8PTR (ns) + 1)) << 16) + ((*(U8PTR (ns) + 2)) << 8) +*(U8PTR (ns) + 3)))

#endif /* _HTON_H_ */

/* EOF - hton.h */
