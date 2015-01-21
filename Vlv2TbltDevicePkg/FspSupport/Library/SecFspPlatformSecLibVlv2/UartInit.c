/** @file
  This PEIM will parse the hoblist from fsp and report them into pei core.
  This file contains the main entrypoint of the PEIM.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <PiPei.h>
#include <Library\IoLib.h>
#include <Library\SerialPortLib.h>

#define PCI_IDX        0xCF8
#define PCI_DAT        0xCFC

#define PCI_LPC_BASE    (0x8000F800)
#define PCI_LPC_REG(x)  (PCI_LPC_BASE + (x))

#define PMC_BASE_ADDRESS                  0xFED03000    // PMC Memory Base Address
#define R_PCH_LPC_PMC_BASE                        0x44  // PBASE, 32bit, 512 Bytes
#define B_PCH_LPC_PMC_BASE_EN                     BIT1  // Enable Bit
#define R_PCH_PMC_GEN_PMCON_1                     0x20  // General PM Configuration 1
#define B_PCH_PMC_GEN_PMCON_SUS_PWR_FLR           BIT14 // SUS Well Power Failure
#define B_PCH_PMC_GEN_PMCON_PWROK_FLR             BIT16 // PWROK Failure

#define R_PCH_LPC_UART_CTRL                       0x80  // UART Control
#define B_PCH_LPC_UART_CTRL_COM1_EN               BIT0  // COM1 Enable

#define ILB_BASE_ADDRESS                  0xFED08000    // ILB Memory Base Address
#define R_PCH_ILB_IRQE                            0x88  // IRQ Enable Control

#define IO_BASE_ADDRESS                   0xFED0C000    // IO Memory Base Address

#define V_PCH_ILB_IRQE_UARTIRQEN_IRQ3             BIT3  // UART IRQ3 Enable
#define V_PCH_ILB_IRQE_UARTIRQEN_IRQ4             BIT4  // UART IRQ4 Enable
#define PCIEX_BASE_ADDRESS                        0xE0000000
#define PCI_EXPRESS_BASE_ADDRESS                  PCIEX_BASE_ADDRESS
#define PciD31F0RegBase                           PCIEX_BASE_ADDRESS + (UINT32) (31 << 15)
#define SB_RCBA                                   0xfed1c000

typedef enum {
  PchA0         = 0,
  PchA1         = 1,
  PchB0         = 2,
  PchB1         = 3,
  PchB2         = 4,
  PchB3         = 5,
  PchC0         = 6,
  PchSteppingMax
} PCH_STEPPING;

#define MmPciAddress( Segment, Bus, Device, Function, Register ) \
  ( (UINTN)PCI_EXPRESS_BASE_ADDRESS + \
    (UINTN)(Bus << 20) + \
    (UINTN)(Device << 15) + \
    (UINTN)(Function << 12) + \
    (UINTN)(Register) \
  )

#define DEFAULT_PCI_BUS_NUMBER_PCH  0
#define PCI_DEVICE_NUMBER_PCH_LPC                 31
#define PCI_FUNCTION_NUMBER_PCH_LPC               0

#define R_PCH_LPC_RID_CC                          0x08  // Revision ID & Class Code

#define V_PCH_LPC_RID_0                           0x01  // A0 Stepping (17 x 17)
#define V_PCH_LPC_RID_1                           0x02  // A0 Stepping (25 x 27)
#define V_PCH_LPC_RID_2                           0x03  // A1 Stepping (17 x 17)
#define V_PCH_LPC_RID_3                           0x04  // A1 Stepping (25 x 27)
#define V_PCH_LPC_RID_4                           0x05  // B0 Stepping (17 x 17)
#define V_PCH_LPC_RID_5                           0x06  // B0 Stepping (25 x 27)
#define V_PCH_LPC_RID_6                           0x07  // B1 Stepping (17 x 17)
#define V_PCH_LPC_RID_7                           0x08  // B1 Stepping (25 x 27)
#define V_PCH_LPC_RID_8                           0x09  // B2 Stepping (17 x 17)
#define V_PCH_LPC_RID_9                           0x0A  // B2 Stepping (25 x 27)
#define V_PCH_LPC_RID_A                           0x0B  // B3 Stepping (17 x 17)
#define V_PCH_LPC_RID_B                           0x0C  // B3 Stepping (25 x 27)
#define V_PCH_LPC_RID_C                           0x0D  // C0 Stepping (17 x 17)
#define V_PCH_LPC_RID_D                           0x0E  // C0 Stepping (25 x 27)

/**
  Return Pch stepping type

  @param[in] None

  @retval PCH_STEPPING            Pch stepping type

**/
PCH_STEPPING
EFIAPI
PchStepping (
  VOID
  )
{
  UINT8 RevId;

  RevId = MmioRead8 (
          MmPciAddress (0,
            DEFAULT_PCI_BUS_NUMBER_PCH,
            PCI_DEVICE_NUMBER_PCH_LPC,
            PCI_FUNCTION_NUMBER_PCH_LPC,
            R_PCH_LPC_RID_CC)
          );

  switch (RevId) {
    case V_PCH_LPC_RID_0:
    case V_PCH_LPC_RID_1:
      return PchA0;
      break;

    case V_PCH_LPC_RID_2:
    case V_PCH_LPC_RID_3:
      return PchA1;
      break;

    case V_PCH_LPC_RID_4:
    case V_PCH_LPC_RID_5:
      return PchB0;
      break;

    case V_PCH_LPC_RID_6:
    case V_PCH_LPC_RID_7:
      return PchB1;
      break;

    case V_PCH_LPC_RID_8:
    case V_PCH_LPC_RID_9:
      return PchB2;
      break;

    case V_PCH_LPC_RID_A:
    case V_PCH_LPC_RID_B:
      return PchB3;
      break;

    case V_PCH_LPC_RID_C:
    case V_PCH_LPC_RID_D:
      return PchC0;
      break;

    default:
      return PchSteppingMax;
      break;

  }
}

/**
  Enable legacy decoding on ICH6

 @param[in] none

 @retval EFI_SUCCESS     Always returns success.

**/
VOID
EnableInternalUart(
  VOID
  )
{

  //
  // Program and enable PMC Base.
  //
  IoWrite32 (PCI_IDX,  PCI_LPC_REG(R_PCH_LPC_PMC_BASE));
  IoWrite32 (PCI_DAT,  (PMC_BASE_ADDRESS | B_PCH_LPC_PMC_BASE_EN));

  //
  // Enable COM1 for debug message output.
  //
  MmioAndThenOr32 (PMC_BASE_ADDRESS + R_PCH_PMC_GEN_PMCON_1, (UINT32) (~(B_PCH_PMC_GEN_PMCON_SUS_PWR_FLR + B_PCH_PMC_GEN_PMCON_PWROK_FLR)), BIT24);

  //
  // Silicon Steppings
  //
  if (PchStepping()>= PchB0)
    MmioOr8 (ILB_BASE_ADDRESS + R_PCH_ILB_IRQE, (UINT8) V_PCH_ILB_IRQE_UARTIRQEN_IRQ4);
  else
    MmioOr8 (ILB_BASE_ADDRESS + R_PCH_ILB_IRQE, (UINT8) V_PCH_ILB_IRQE_UARTIRQEN_IRQ3);
  MmioAnd32(IO_BASE_ADDRESS + 0x0520, (UINT32)~(0x00000187));
  MmioOr32 (IO_BASE_ADDRESS + 0x0520, (UINT32)0x81); // UART3_RXD-L
  MmioAnd32(IO_BASE_ADDRESS + 0x0530, (UINT32)~(0x00000007));
  MmioOr32 (IO_BASE_ADDRESS + 0x0530, (UINT32)0x1); // UART3_RXD-L
  MmioOr8 (PciD31F0RegBase + R_PCH_LPC_UART_CTRL, (UINT8) B_PCH_LPC_UART_CTRL_COM1_EN);

  SerialPortInitialize ();
  SerialPortWrite ("EnableInternalUart!\r\n", sizeof("EnableInternalUart!\r\n") - 1);

  return  ;
}
