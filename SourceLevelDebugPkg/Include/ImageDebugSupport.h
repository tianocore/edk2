/** @file
  Public include file for Debug Agent Library instance and PE/COFF Extra
  Action Library instance.

  Copyright (c) 2010 - 2013, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __IMAGE_DEBUG_SUPPORT_H__
#define __IMAGE_DEBUG_SUPPORT_H__

#define IO_PORT_BREAKPOINT_ADDRESS  0x84
#define IMAGE_LOAD_SIGNATURE        SIGNATURE_32('L','O','A','D')
#define IMAGE_UNLOAD_SIGNATURE      SIGNATURE_32('U','N','L','O')
#define AGENT_HANDLER_SIGNATURE     SIGNATURE_32('A','G','T','H')

#define DEBUG_AGENT_IMAGE_WAIT      0x00
#define DEBUG_AGENT_IMAGE_CONTINUE  0x01

#endif

