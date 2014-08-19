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

#define PL301_QOS_TIDEMARK_MI_0                  0x400
#define PL301_QOS_ACCESSCONTROL_MI_0             0x404

#define PL301_QOS_TIDEMARK_MI_1                  0x420
#define PL301_QOS_ACCESSCONTROL_MI_1             0x424

#define PL301_QOS_TIDEMARK_MI_2                  0x440
#define PL301_QOS_ACCESSCONTROL_MI_2             0x444

#define PL301_AR_ARB_MI_0                        0x408
#define PL301_AW_ARB_MI_0                        0x40C

#define PL301_AR_ARB_MI_1                        0x428
#define PL301_AW_ARB_MI_1                        0x42C

#define PL301_AR_ARB_MI_2                        0x448
#define PL301_AW_ARB_MI_2                        0x44C

#define PL301_MI_1_OFFSET                        0x20
#define PL301_MI_2_OFFSET                        0x40
#define PL301_MI_3_OFFSET                        0x60
#define PL301_MI_4_OFFSET                        0x80
#define PL301_MI_5_OFFSET                        0xa0

#define V2P_CA9_FAXI_MI0_TIDEMARK_VAL            0x6
#define V2P_CA9_FAXI_MI0_ACCESSCNTRL_VAL         0x1

#define V2P_CA9_FAXI_MI1_TIDEMARK_VAL            0x6
#define V2P_CA9_FAXI_MI1_ACCESSCNTRL_VAL         0x1

#define V2P_CA9_FAXI_MI2_TIDEMARK_VAL            0x6
#define V2P_CA9_FAXI_MI2_ACCESSCNTRL_VAL         0x1


#define FAxiWriteReg(reg,val)                    MmioWrite32(FAxiBase + reg, val)
#define FAxiReadReg(reg)                         MmioRead32(FAxiBase + reg)

// IN FAxiBase
// Initialize PL301 Dynamic Memory Controller
VOID PL301AxiInit(UINTN FAxiBase) {
    // Configure Tidemark Register for Master Port 0 (MI 0)
    FAxiWriteReg(PL301_QOS_TIDEMARK_MI_0, V2P_CA9_FAXI_MI0_TIDEMARK_VAL);

    // Configure the Access Control Register (MI 0)
    FAxiWriteReg(PL301_QOS_ACCESSCONTROL_MI_0, V2P_CA9_FAXI_MI0_ACCESSCNTRL_VAL);

    // MP0
    // Set priority for Read
    FAxiWriteReg(PL301_AR_ARB_MI_0, 0x00000100);
    FAxiWriteReg(PL301_AR_ARB_MI_0, 0x01000200);
    FAxiWriteReg(PL301_AR_ARB_MI_0, 0x02000200);
    FAxiWriteReg(PL301_AR_ARB_MI_0, 0x03000200);
    FAxiWriteReg(PL301_AR_ARB_MI_0, 0x04000200);

    // Set priority for Write
    FAxiWriteReg(PL301_AW_ARB_MI_0, 0x00000100);
    FAxiWriteReg(PL301_AW_ARB_MI_0, 0x01000200);
    FAxiWriteReg(PL301_AW_ARB_MI_0, 0x02000200);
    FAxiWriteReg(PL301_AW_ARB_MI_0, 0x03000200);
    FAxiWriteReg(PL301_AW_ARB_MI_0, 0x04000200);

    // MP1
    // Set priority for Read
    FAxiWriteReg(PL301_AR_ARB_MI_1, 0x00000100);
    FAxiWriteReg(PL301_AR_ARB_MI_1, 0x01000200);
    FAxiWriteReg(PL301_AR_ARB_MI_1, 0x02000200);
    FAxiWriteReg(PL301_AR_ARB_MI_1, 0x03000200);
    FAxiWriteReg(PL301_AR_ARB_MI_1, 0x04000200);

    // Set priority for Write
    FAxiWriteReg(PL301_AW_ARB_MI_1, 0x00000100);
    FAxiWriteReg(PL301_AW_ARB_MI_1, 0x01000200);
    FAxiWriteReg(PL301_AW_ARB_MI_1, 0x02000200);
    FAxiWriteReg(PL301_AW_ARB_MI_1, 0x03000200);
    FAxiWriteReg(PL301_AW_ARB_MI_1, 0x04000200);

    // MP2
    // Set priority for Read
    FAxiWriteReg(PL301_AR_ARB_MI_2, 0x00000100);
    FAxiWriteReg(PL301_AR_ARB_MI_2, 0x01000100);
    FAxiWriteReg(PL301_AR_ARB_MI_2, 0x02000100);
    FAxiWriteReg(PL301_AR_ARB_MI_2, 0x03000100);
    FAxiWriteReg(PL301_AR_ARB_MI_2, 0x04000100);

    // Set priority for Write
    FAxiWriteReg(PL301_AW_ARB_MI_2, 0x00000100);
    FAxiWriteReg(PL301_AW_ARB_MI_2, 0x01000200);
    FAxiWriteReg(PL301_AW_ARB_MI_2, 0x02000200);
    FAxiWriteReg(PL301_AW_ARB_MI_2, 0x03000200);
    FAxiWriteReg(PL301_AW_ARB_MI_2, 0x04000200);
}
