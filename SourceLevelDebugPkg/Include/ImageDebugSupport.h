/** @file
  Public include file for Debug Agent Library instance and PE/COFF Extra
  Action Library instance.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __IMAGE_DEBUG_SUPPORT_H__
#define __IMAGE_DEBUG_SUPPORT_H__

#define IO_PORT_BREAKPOINT_ADDRESS  0x84
#define IMAGE_LOAD_SIGNATURE        SIGNATURE_32('L','O','A','D')
#define IMAGE_UNLOAD_SIGNATURE      SIGNATURE_32('U','N','L','O')

#define DEBUG_AGENT_IMAGE_WAIT      0x00
#define DEBUG_AGENT_IMAGE_CONTINUE  0x01

#endif

