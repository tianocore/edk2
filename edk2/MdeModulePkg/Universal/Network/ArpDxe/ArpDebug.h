/** @file

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  ArpDebug.h

Abstract:


**/

#ifndef _ARP_DEBUG_H_
#define _ARP_DEBUG_H_


#define ARP_DEBUG_TRACE(PrintArg) NET_DEBUG_TRACE   ("Arp", PrintArg)
#define ARP_DEBUG_WARN(PrintArg)  NET_DEBUG_WARNING ("Arp", PrintArg)
#define ARP_DEBUG_ERROR(PrintArg) NET_DEBUG_ERROR   ("Arp", PrintArg)

#endif

