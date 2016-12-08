/** @file
  MSR Definitions.

  Provides defines for Machine Specific Registers(MSR) indexes. Data structures
  are provided for MSRs that contain one or more bit fields.  If the MSR value
  returned is a single 32-bit or 64-bit value, then a data structure is not
  provided for that MSR.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  @par Specification Reference:
  Intel(R) 64 and IA-32 Architectures Software Developer's Manual, Volume 3,
  September 2016, Chapter 35 Model-Specific-Registers (MSR), Chapter 35.

**/

#ifndef __MSR_H__
#define __MSR_H__

#include <Register/ArchitecturalMsr.h>
#include <Register/Msr/Core2Msr.h>
#include <Register/Msr/AtomMsr.h>
#include <Register/Msr/SilvermontMsr.h>
#include <Register/Msr/GoldmontMsr.h>
#include <Register/Msr/NehalemMsr.h>
#include <Register/Msr/Xeon5600Msr.h>
#include <Register/Msr/XeonE7Msr.h>
#include <Register/Msr/SandyBridgeMsr.h>
#include <Register/Msr/IvyBridgeMsr.h>
#include <Register/Msr/HaswellMsr.h>
#include <Register/Msr/HaswellEMsr.h>
#include <Register/Msr/BroadwellMsr.h>
#include <Register/Msr/XeonDMsr.h>
#include <Register/Msr/SkylakeMsr.h>
#include <Register/Msr/XeonPhiMsr.h>
#include <Register/Msr/Pentium4Msr.h>
#include <Register/Msr/CoreMsr.h>
#include <Register/Msr/PentiumMMsr.h>
#include <Register/Msr/P6Msr.h>
#include <Register/Msr/PentiumMsr.h>

#endif
