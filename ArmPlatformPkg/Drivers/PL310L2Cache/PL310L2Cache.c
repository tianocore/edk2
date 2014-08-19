/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/ArmLib.h>
#include <Drivers/PL310L2Cache.h>
#include <Library/PcdLib.h>

#define L2x0WriteReg(reg,val)               MmioWrite32(PcdGet32(PcdL2x0ControllerBase) + reg, val)
#define L2x0ReadReg(reg)                    MmioRead32(PcdGet32(PcdL2x0ControllerBase) + reg)

// Initialize PL320 L2 Cache Controller
VOID
L2x0CacheInit (
  IN  UINTN   L2x0Base,
  IN  UINT32  L2x0TagLatencies,
  IN  UINT32  L2x0DataLatencies,
  IN  UINT32  L2x0AuxValue,
  IN  UINT32  L2x0AuxMask,
  IN  BOOLEAN CacheEnabled
  )
{
  UINT32 Data;
  UINT32 Revision;
  UINT32 Aux;
  UINT32 PfCtl;
  UINT32 PwrCtl;

  // Check if L2x0 is present and is an ARM implementation
  Data = L2x0ReadReg(L2X0_CACHEID);
  if ((Data >> 24) != L2X0_CACHEID_IMPLEMENTER_ARM) {
    ASSERT(0);
    return;
  }

  // Check if L2x0 is PL310
  if (((Data >> 6) & 0xF) != L2X0_CACHEID_PARTNUM_PL310) {
    ASSERT(0);
    return;
  }

  // RTL release
  Revision = Data & 0x3F;

  // Check if L2x0 is already enabled then we disable it
  Data = L2x0ReadReg(L2X0_CTRL);
  if (Data & L2X0_CTRL_ENABLED) {
    L2x0WriteReg(L2X0_CTRL, L2X0_CTRL_DISABLED);
  }

  //
  // Set up global configurations
  //

  // Auxiliary register: Non-secure interrupt access Control + Event monitor bus enable + SBO
  Aux = L2X0_AUXCTRL_NSAC | L2X0_AUXCTRL_EM | L2X0_AUXCTRL_SBO;
  // Use AWCACHE attributes for WA
  Aux |= L2x0_AUXCTRL_AW_AWCACHE;
  // Use default Size
  Data = L2x0ReadReg(L2X0_AUXCTRL);
  Aux |= Data & L2X0_AUXCTRL_WAYSIZE_MASK;
  // Use default associativity
  Aux |= Data & L2X0_AUXCTRL_ASSOCIATIVITY;
  // Enabled I & D Prefetch
  Aux |= L2x0_AUXCTRL_IPREFETCH | L2x0_AUXCTRL_DPREFETCH;

  if (Revision >= 5) {
    // Prefetch Offset Register
    PfCtl = L2x0ReadReg(L2X0_PFCTRL);
    // - Prefetch increment set to 0
    // - Prefetch dropping off
    // - Double linefills off
    L2x0WriteReg(L2X0_PFCTRL, PfCtl);

    // Power Control Register - L2X0_PWRCTRL
    PwrCtl = L2x0ReadReg(L2X0_PWRCTRL);
    // - Standby when idle off
    // - Dynamic clock gating off
    // - Nc,NC-shared dropping off
    L2x0WriteReg(L2X0_PWRCTRL, PwrCtl);
  }

  if (Revision >= 2) {
    L2x0WriteReg(L230_TAG_LATENCY, L2x0TagLatencies);
    L2x0WriteReg(L230_DATA_LATENCY, L2x0DataLatencies);
  } else {
    // PL310 old style latency is not supported yet
    ASSERT(0);
  }

  // Set the platform specific values
  Aux = (Aux & L2x0AuxMask) | L2x0AuxValue;

  // Write Auxiliary value
  L2x0WriteReg(L2X0_AUXCTRL, Aux);

  //
  // Invalidate all entries in cache
  //
  L2x0WriteReg(L2X0_INVWAY, 0xffff);
  // Poll cache maintenance register until invalidate operation is complete
  while(L2x0ReadReg(L2X0_INVWAY) & 0xffff);

  // Write to the Lockdown D and Lockdown I Register 9 if required
  // - Not required

  // Clear any residual raw interrupts
  L2x0WriteReg(L2X0_INTCLEAR, 0x1FF);

  // Enable the cache
  if (CacheEnabled) {
    L2x0WriteReg(L2X0_CTRL, L2X0_CTRL_ENABLED);
  }
}
