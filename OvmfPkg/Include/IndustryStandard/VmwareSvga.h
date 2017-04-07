/** @file

  Macro and enum definitions of a subset of port numbers, register identifiers
  and values required for driving the VMWare SVGA virtual display adapter,
  also implemented by Qemu.

  This file's contents was extracted from file lib/vmware/svga_reg.h in commit
  329dd537456f93a806841ec8a8213aed11395def of VMWare's vmware-svga repository:
  git://git.code.sf.net/p/vmware-svga/git


  Copyright 1998-2009 VMware, Inc.  All rights reserved.
  Portions Copyright 2017 Phil Dennis-Jordan <phil@philjordan.eu>

  Permission is hereby granted, free of charge, to any person
  obtaining a copy of this software and associated documentation
  files (the "Software"), to deal in the Software without
  restriction, including without limitation the rights to use, copy,
  modify, merge, publish, distribute, sublicense, and/or sell copies
  of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
  ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

**/

#ifndef _VMWARE_SVGA_H_
#define _VMWARE_SVGA_H_

#include <Base.h>

//
// IDs for recognising the device
//
#define VMWARE_PCI_VENDOR_ID_VMWARE            0x15AD
#define VMWARE_PCI_DEVICE_ID_VMWARE_SVGA2      0x0405

//
// I/O port BAR offsets for register selection and read/write.
//
// The register index is written to the 32-bit index port, followed by a 32-bit
// read or write on the value port to read or set that register's contents.
//
#define VMWARE_SVGA_INDEX_PORT         0x0
#define VMWARE_SVGA_VALUE_PORT         0x1

//
// Some of the device's register indices for basic framebuffer functionality.
//
typedef enum {
  VmwareSvgaRegId = 0,
  VmwareSvgaRegEnable = 1,
  VmwareSvgaRegWidth = 2,
  VmwareSvgaRegHeight = 3,
  VmwareSvgaRegMaxWidth = 4,
  VmwareSvgaRegMaxHeight = 5,

  VmwareSvgaRegBitsPerPixel = 7,

  VmwareSvgaRegRedMask = 9,
  VmwareSvgaRegGreenMask = 10,
  VmwareSvgaRegBlueMask = 11,
  VmwareSvgaRegBytesPerLine = 12,

  VmwareSvgaRegFbOffset = 14,

  VmwareSvgaRegFbSize = 16,
  VmwareSvgaRegCapabilities = 17,

  VmwareSvgaRegHostBitsPerPixel = 28,
} VMWARE_SVGA_REGISTER;

//
// Values used with VmwareSvgaRegId for sanity-checking the device and getting
// its version.
//
#define VMWARE_SVGA_MAGIC          0x900000U
#define VMWARE_SVGA_MAKE_ID(ver)   (VMWARE_SVGA_MAGIC << 8 | (ver))

#define VMWARE_SVGA_VERSION_2      2
#define VMWARE_SVGA_ID_2           VMWARE_SVGA_MAKE_ID (VMWARE_SVGA_VERSION_2)

#define VMWARE_SVGA_VERSION_1      1
#define VMWARE_SVGA_ID_1           VMWARE_SVGA_MAKE_ID (VMWARE_SVGA_VERSION_1)

#define VMWARE_SVGA_VERSION_0      0
#define VMWARE_SVGA_ID_0           VMWARE_SVGA_MAKE_ID (VMWARE_SVGA_VERSION_0)

//
// One of the capability bits advertised by VmwareSvgaRegCapabilities.
//
#define VMWARE_SVGA_CAP_8BIT_EMULATION     BIT8

#endif
