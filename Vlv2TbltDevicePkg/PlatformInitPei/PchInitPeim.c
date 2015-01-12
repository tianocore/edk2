/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

  PchInitPeim.c

Abstract:

  Do Early PCH platform initialization.


--*/

#include "PlatformEarlyInit.h"
#include "Ppi/PchPlatformPolicy.h"
#include "PchRegs.h"
#include <Ppi/PchUsbPolicy.h>
#include "Ppi/PchInit.h"
#include <Library/PcdLib.h>

EFI_GUID  gPchPlatformPolicyPpiGuid = PCH_PLATFORM_POLICY_PPI_GUID;

#define MC_PMSTS_OFFSET                 0xC

#define DEFAULT_BUS_INFO                0x2020


#define PCI_LPC_BASE    (0x8000F800)
#define PCI_LPC_REG(x)  (PCI_LPC_BASE + (x))
#define PCIEX_BASE_ADDRESS                        0xE0000000
#define PciD31F0RegBase                           PCIEX_BASE_ADDRESS + (UINT32) (31 << 15)

VOID
PchPolicySetupInit (
  IN CONST EFI_PEI_SERVICES **PeiServices,
  IN SYSTEM_CONFIGURATION   *SystemConfiguration
  );

VOID
PchInitInterrupt (
  IN SYSTEM_CONFIGURATION  *SystemConfiguration
  );

#ifndef __GNUC__
#pragma warning (push)
#pragma warning (disable : 4245)
#pragma warning (pop)
#endif

UINT8
ReadCmosBank1Byte (
  IN UINT8                      Address
  )
{
  UINT8                           Data;

  IoWrite8(R_PCH_RTC_EXT_INDEX, Address);
  Data = IoRead8 (R_PCH_RTC_EXT_TARGET);
  return Data;
}

VOID
WriteCmosBank1Byte (
  IN UINT8                     Address,
  IN UINT8                     Data
  )
{
  IoWrite8(R_PCH_RTC_EXT_INDEX, Address);
  IoWrite8(R_PCH_RTC_EXT_TARGET, Data);
}

/**
  Turn off system if needed.

  @param PeiServices Pointer to PEI Services
  @param CpuIo       Pointer to CPU I/O Protocol

  @retval None.

**/
VOID
CheckPowerOffNow (
  VOID
  )
{
  UINT16  Pm1Sts;

  //
  // Read and check the ACPI registers
  //
  Pm1Sts = IoRead16 (ACPI_BASE_ADDRESS + R_PCH_ACPI_PM1_STS);
  if ((Pm1Sts & B_PCH_ACPI_PM1_STS_PWRBTN) == B_PCH_ACPI_PM1_STS_PWRBTN) {
    IoWrite16 (ACPI_BASE_ADDRESS + R_PCH_ACPI_PM1_STS, B_PCH_ACPI_PM1_STS_PWRBTN);
    IoWrite16 (ACPI_BASE_ADDRESS + R_PCH_ACPI_PM1_CNT, V_PCH_ACPI_PM1_CNT_S5);
    IoWrite16 (ACPI_BASE_ADDRESS + R_PCH_ACPI_PM1_CNT, V_PCH_ACPI_PM1_CNT_S5 + B_PCH_ACPI_PM1_CNT_SLP_EN);

    //
    // Should not return
    //
    CpuDeadLoop();
  }
}

VOID
ClearPowerState (
  IN SYSTEM_CONFIGURATION        *SystemConfiguration
  )
{
  UINT8   Data8;
  UINT16  Data16;
  UINT32  Data32;

  //
  // Check for PowerState option for AC power loss and program the chipset
  //

  //
  // Clear PWROK (Set to Clear)
  //
  MmioOr32 (PMC_BASE_ADDRESS + R_PCH_PMC_GEN_PMCON_1, B_PCH_PMC_GEN_PMCON_PWROK_FLR);

  //
  // Clear Power Failure Bit (Set to Clear)
  //
  // TODO: Check if it is OK to clear here
  //

  MmioOr32 (PMC_BASE_ADDRESS + R_PCH_PMC_GEN_PMCON_1, B_PCH_PMC_GEN_PMCON_SUS_PWR_FLR);

  //
  // Clear the GPE and PM enable
  //
  IoWrite16 (ACPI_BASE_ADDRESS + R_PCH_ACPI_PM1_EN, (UINT16) 0x00);
  IoWrite32 (ACPI_BASE_ADDRESS + R_PCH_ACPI_GPE0a_EN, (UINT32) 0x00);

  //
  // Halt the TCO timer
  //
  Data16 = IoRead16 (ACPI_BASE_ADDRESS + R_PCH_TCO_CNT);
  Data16 |= B_PCH_TCO_CNT_TMR_HLT;
  IoWrite16 (ACPI_BASE_ADDRESS + R_PCH_TCO_CNT, Data16);

  //
  // if NMI_NOW_STS is set
  // NMI NOW bit is "Write '1' to clear"
  //
  Data8 = MmioRead8(ILB_BASE_ADDRESS + R_PCH_ILB_GNMI);
  if ((Data8 & B_PCH_ILB_GNMI_NMINS) == B_PCH_ILB_GNMI_NMINS) {
    MmioOr8 (ILB_BASE_ADDRESS + R_PCH_ILB_GNMI, B_PCH_ILB_GNMI_NMIN);
  }

  //
  // Before we clear the TO status bit here we need to save the results in a CMOS bit for later use.
  //
  Data32 = IoRead32 (ACPI_BASE_ADDRESS + R_PCH_TCO_STS);
  if ((Data32 & B_PCH_TCO_STS_SECOND_TO) == B_PCH_TCO_STS_SECOND_TO)
  {
#if (defined(HW_WATCHDOG_TIMER_SUPPORT) && (HW_WATCHDOG_TIMER_SUPPORT != 0))
    WriteCmosBank1Byte (
      EFI_CMOS_PERFORMANCE_FLAGS,
      ReadCmosBank1Byte (EFI_CMOS_PERFORMANCE_FLAGS) | B_CMOS_TCO_WDT_RESET
      );
#endif
  }
}

/*++

  Clear any SMI status or wake status left over from boot.

**/
VOID
ClearSmiAndWake (
  VOID
  )
{
  UINT16  Pm1Sts;
  UINT32  Gpe0Sts;
  UINT32  SmiSts;

  //
  // Read the ACPI registers
  //
  Pm1Sts  = IoRead16 (ACPI_BASE_ADDRESS + R_PCH_ACPI_PM1_STS);
  Gpe0Sts = IoRead32 (ACPI_BASE_ADDRESS + R_PCH_ACPI_GPE0a_STS);
  SmiSts  = IoRead32 (ACPI_BASE_ADDRESS + R_PCH_SMI_STS);

  //
  // Register Wake up reason for S4.  This information is used to notify
  // WinXp of wake up reason because S4 wake up path doesn't keep SCI.
  // This is important for Viiv(Quick resume) platform.
  //

  //
  // First Clear CMOS S4 Wake up flag.
  //
  WriteCmosBank1Byte(CMOS_S4_WAKEUP_FLAG_ADDRESS, 0);

  //
  // Check wake up reason and set CMOS accordingly.  Currently checks
  // Power button, USB, PS/2.
  // Note : PS/2 wake up is using GPI13 (IO_PME).  This must be changed depending
  // on board design.
  //
  if ((Pm1Sts & B_PCH_ACPI_PM1_STS_PWRBTN) || (Gpe0Sts & (B_PCH_ACPI_GPE0a_STS_CORE_GPIO | B_PCH_ACPI_GPE0a_STS_SUS_GPIO))) {
    WriteCmosBank1Byte(CMOS_S4_WAKEUP_FLAG_ADDRESS, 1);
  }

  //
  // Clear any SMI or wake state from the boot
  //
  Pm1Sts = (B_PCH_ACPI_PM1_STS_PRBTNOR | B_PCH_ACPI_PM1_STS_PWRBTN);

  Gpe0Sts |=
    (
      B_PCH_ACPI_GPE0a_STS_CORE_GPIO |
      B_PCH_ACPI_GPE0a_STS_SUS_GPIO |
      B_PCH_ACPI_GPE0a_STS_PME_B0 |
      B_PCH_ACPI_GPE0a_STS_BATLOW |
      B_PCH_ACPI_GPE0a_STS_PCI_EXP |
      B_PCH_ACPI_GPE0a_STS_GUNIT_SCI |
      B_PCH_ACPI_GPE0a_STS_PUNIT_SCI |
      B_PCH_ACPI_GPE0a_STS_SWGPE |
      B_PCH_ACPI_GPE0a_STS_HOT_PLUG
    );

  SmiSts |=
    (
      B_PCH_SMI_STS_SMBUS |
      B_PCH_SMI_STS_PERIODIC |
      B_PCH_SMI_STS_TCO |
      B_PCH_SMI_STS_SWSMI_TMR |
      B_PCH_SMI_STS_APM |
      B_PCH_SMI_STS_ON_SLP_EN |
      B_PCH_SMI_STS_BIOS
    );

  //
  // Write them back
  //
  IoWrite16 (ACPI_BASE_ADDRESS + R_PCH_ACPI_PM1_STS, Pm1Sts);
  IoWrite32 (ACPI_BASE_ADDRESS + R_PCH_ACPI_GPE0a_STS, Gpe0Sts);
  IoWrite32 (ACPI_BASE_ADDRESS + R_PCH_SMI_STS, SmiSts);
}

/**
  Issue PCI-E Secondary Bus Reset

  @param Bus  Bus number of the bridge
  @param Dev  Devices number of the bridge
  @param Fun  Function number of the bridge

  @retval EFI_SUCCESS

**/
EFI_STATUS
PcieSecondaryBusReset (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN UINT8             Bus,
  IN UINT8             Dev,
  IN UINT8             Fun
  )
{
  EFI_PEI_STALL_PPI   *PeiStall;
  EFI_STATUS          Status;

  Status = (**PeiServices).LocatePpi (
                             PeiServices,
                             &gEfiPeiStallPpiGuid,
                             0,
                             NULL,
                    (void **)&PeiStall
                             );
  ASSERT_EFI_ERROR (Status);

  //
  // Issue secondary bus reset
  //
  MmPci16Or(0, Bus, Dev, Fun, PCI_BRIDGE_CONTROL_REGISTER_OFFSET, EFI_PCI_BRIDGE_CONTROL_RESET_SECONDARY_BUS);

  //
  // Wait 1ms
  //
  PeiStall->Stall (PeiServices, PeiStall, 1000);


  //
  // Clear the reset bit
  // Note: The PCIe spec suggests 100ms delay between clearing this bit and accessing
  // the device's config space. Since we will not access the config space until we enter DXE
  // we don't put delay expressly here.
  //
  MmPci16And(0, Bus, Dev, Fun, PCI_BRIDGE_CONTROL_REGISTER_OFFSET, ~(EFI_PCI_BRIDGE_CONTROL_RESET_SECONDARY_BUS));

  return EFI_SUCCESS;
}

/**
  Provide hard reset PPI service.
  To generate full hard reset, write 0x0E to ICH RESET_GENERATOR_PORT (0xCF9).

  @param PeiServices        General purpose services available to every PEIM.

  @retval Not return        System reset occured.
  @retval EFI_DEVICE_ERROR  Device error, could not reset the system.

**/
EFI_STATUS
EFIAPI
IchReset (
  IN CONST EFI_PEI_SERVICES          **PeiServices
  )
{
  IoWrite8 (
    R_PCH_RST_CNT,
    V_PCH_RST_CNT_HARDSTARTSTATE
    );

  IoWrite8 (
    R_PCH_RST_CNT,
    V_PCH_RST_CNT_HARDRESET
    );

  //
  // System reset occured, should never reach at this line.
  //
  ASSERT_EFI_ERROR (EFI_DEVICE_ERROR);
  CpuDeadLoop();

  return EFI_DEVICE_ERROR;
}

VOID
PchPlatformLpcInit (
  IN  CONST EFI_PEI_SERVICES          **PeiServices,
  IN SYSTEM_CONFIGURATION       *SystemConfiguration
  )
{
  EFI_BOOT_MODE BootMode;
  UINT8         Data8;
  UINT16                Data16;

  (*PeiServices)->GetBootMode(PeiServices, &BootMode);

  if ((BootMode != BOOT_ON_S3_RESUME)) {

    //
    // Clear all pending SMI. On S3 clear power button enable so it wll not generate an SMI
    //
    ClearSmiAndWake ();
  }

  ClearPowerState (SystemConfiguration);

  //
  // Need to set and clear SET bit (RTC_REGB Bit 7) as requested by the ICH EDS
  // early in POST after each power up directly after coin-cell battery insertion.
  // This is to avoid the UIP bit (RTC_REGA Bit 7) from stuck at "1".
  // The UIP bit status may be polled by software (i.e ME FW) during POST.
  //
  if (MmioRead8 (PMC_BASE_ADDRESS + R_PCH_PMC_GEN_PMCON_1) & B_PCH_PMC_GEN_PMCON_RTC_PWR_STS) {
  	//
    // Set and clear SET bit in RTC_REGB
    //
    IoWrite8(R_PCH_RTC_INDEX, R_PCH_RTC_REGISTERB);
    Data8 = IoRead8(R_PCH_RTC_TARGET);
    Data8 |= B_PCH_RTC_REGISTERB_SET;
    IoWrite8(R_PCH_RTC_TARGET, Data8);

    IoWrite8(R_PCH_RTC_INDEX, R_PCH_RTC_REGISTERB);
    Data8 &= (~B_PCH_RTC_REGISTERB_SET);
    IoWrite8(R_PCH_RTC_TARGET, Data8);

    //
    // Clear the UIP bit in RTC_REGA
    //
    IoWrite8(R_PCH_RTC_INDEX, R_PCH_RTC_REGISTERA);
    IoWrite8(R_PCH_RTC_TARGET, 0x00);
  }

  //
  // Disable SERR NMI and IOCHK# NMI in port 61
  //
  Data8 = IoRead8 (R_PCH_NMI_SC);
  IoWrite8(R_PCH_NMI_SC, (UINT8) (Data8 | B_PCH_NMI_SC_PCI_SERR_EN | B_PCH_NMI_SC_IOCHK_NMI_EN));

  //
  // Enable Bus Master, I/O, Mem, and SERR on LPC bridge
  //
  Data16 = PchLpcPciCfg16 (R_PCH_LPC_COMMAND);
  MmioWrite16 (
    MmPciAddress (0,
      DEFAULT_PCI_BUS_NUMBER_PCH,
      PCI_DEVICE_NUMBER_PCH_LPC,
      PCI_FUNCTION_NUMBER_PCH_LPC,
      R_PCH_LPC_COMMAND
    ),
    (Data16 |
     B_PCH_LPC_COMMAND_IOSE |
     B_PCH_LPC_COMMAND_MSE |
     B_PCH_LPC_COMMAND_BME |
     B_PCH_LPC_COMMAND_SERR_EN)
  );

  //
  // Set Stretch S4 to 1-2s per marketing request.
  // Note: This register is powered by RTC well.
  //
  MmioAndThenOr8 (
    PMC_BASE_ADDRESS + R_PCH_PMC_GEN_PMCON_1 ,
    (UINT8) (~B_PCH_PMC_GEN_PMCON_SLP_S4_MAW),
    (UINT8) (B_PCH_PMC_GEN_PMCON_SLP_S4_ASE | V_PCH_PMC_GEN_PMCON_SLP_S4_MAW_4S)
    );

}

#define V_PCH_ILB_IRQE_UARTIRQEN_IRQ3             BIT3 // UART IRQ3 Enable

VOID
UARTInit (
  IN SYSTEM_CONFIGURATION        *SystemConfiguration
  )
{
  if (0) { // for fix cr4 issue
    //
    // Program and enable PMC Base.
    //
    IoWrite32 (0xCF8,  PCI_LPC_REG(R_PCH_LPC_PMC_BASE));
    IoWrite32 (0xCFC,  (PMC_BASE_ADDRESS | B_PCH_LPC_PMC_BASE_EN));

    if( (SystemConfiguration->PcuUart1 == 1) &&
        (SystemConfiguration->LpssHsuart0Enabled == 0)){
      //
      // Enable COM1 for debug message output.
      //
      MmioOr32 (PMC_BASE_ADDRESS + R_PCH_PMC_GEN_PMCON_1, BIT24);

      //
      //Enable internal UART3 port(COM1)
      //
      MmioOr8 (ILB_BASE_ADDRESS + R_PCH_ILB_IRQE, (UINT8) V_PCH_ILB_IRQE_UARTIRQEN_IRQ3);
      MmioOr32 (IO_BASE_ADDRESS + 0x0520, 0x01); // UART3_RXD-L
      MmioOr32 (IO_BASE_ADDRESS + 0x0530, 0x01); // UART3_TXD-0
      MmioOr8 (PciD31F0RegBase + R_PCH_LPC_UART_CTRL, (UINT8) B_PCH_LPC_UART_CTRL_COM1_EN);
    } else {
    	//
      //Disable UART3(COM1)
      //
      MmioAnd8 (ILB_BASE_ADDRESS + R_PCH_ILB_IRQE, (UINT8) ~V_PCH_ILB_IRQE_UARTIRQEN_IRQ3);
      MmioAnd32 (IO_BASE_ADDRESS + 0x0520, ~(UINT32)0x07);
      MmioAnd32 (IO_BASE_ADDRESS + 0x0530, ~(UINT32)0x07);
      MmioAnd8 (PciD31F0RegBase + R_PCH_LPC_UART_CTRL, (UINT8) ~B_PCH_LPC_UART_CTRL_COM1_EN);


      if (SystemConfiguration->LpssHsuart0Enabled == 1){
        //
        //Valleyview BIOS Specification Vol2,17.2
        //LPSS_UART1 ¨C set each pad PAD_CONF0.Func_Pin_Mux to function 1:
        //
        MmioAnd8 (IO_BASE_ADDRESS + 0x0090, (UINT8)~0x07);
        MmioOr8 (IO_BASE_ADDRESS + 0x0090, 0x01);
        MmioAnd8 (IO_BASE_ADDRESS + 0x00D0, (UINT8)~0x07);
        MmioOr8 (IO_BASE_ADDRESS + 0x00D0, 0x01);

      }
    }


    DEBUG ((EFI_D_ERROR, "EnableInternalUart\n"));
  } else {
  	//
    // If SIO UART interface selected
    //Disable internal UART port(COM1)
    //
    if (0) {; // For fix CR4 issue
      MmioAnd8 (ILB_BASE_ADDRESS + R_PCH_ILB_IRQE, (UINT8) ~V_PCH_ILB_IRQE_UARTIRQEN_IRQ3);
      MmioAnd8 (IO_BASE_ADDRESS + 0x0090, (UINT8)~0x07);
      MmioAnd8 (IO_BASE_ADDRESS + 0x00D0, (UINT8)~0x07);
      MmioAnd8 (PciD31F0RegBase + R_PCH_LPC_UART_CTRL, (UINT8) ~B_PCH_LPC_UART_CTRL_COM1_EN);

    }
  }
}

VOID
IchRcrbInit (
  IN CONST EFI_PEI_SERVICES            **PeiServices,
  IN SYSTEM_CONFIGURATION        *SystemConfiguration
  )
{
  UINT8                           LpcRevisionID;
  EFI_PLATFORM_CPU_INFO           *PlatformCpuInfo;
  EFI_PEI_HOB_POINTERS            Hob;
  EFI_BOOT_MODE                   BootMode;

  //
  // Get Platform Info HOB
  //
  Hob.Raw = GetFirstGuidHob (&gEfiPlatformCpuInfoGuid);
  ASSERT (Hob.Raw != NULL);
  PlatformCpuInfo = GET_GUID_HOB_DATA(Hob.Raw);

  (*PeiServices)->GetBootMode(PeiServices, &BootMode);

  //
  // If not recovery or flash update boot path. set the BIOS interface lock down bit.
  // It locks the top swap bit and BIOS boot strap bits from being changed.
  //
  if ((BootMode != BOOT_IN_RECOVERY_MODE) && (BootMode != BOOT_ON_FLASH_UPDATE)) {
    MmioOr8 (RCBA_BASE_ADDRESS + R_PCH_RCRB_GCS, B_PCH_RCRB_GCS_BILD);
  }

  //
  // Disable the Watchdog timer expiration from causing a system reset
  //
  MmioOr8 (PMC_BASE_ADDRESS + R_PCH_PMC_PM_CFG, B_PCH_PMC_PM_CFG_NO_REBOOT);

  //
  // Initial RCBA according to the PeiRCBA table
  //
  LpcRevisionID = PchLpcPciCfg8 (R_PCH_LPC_RID_CC);

  if ((BootMode == BOOT_ON_S3_RESUME)) {
    //
    // We are resuming from S3
    // Enable HPET if enabled in Setup
    // ICH Config register Offset 0x3404 bit 7 (Enable) = 1,
    // Bit 1:0 (Mem I/O address) = 0 (0xFED00000)
    //
    MmioOr8 (R_PCH_PCH_HPET + R_PCH_PCH_HPET_GCFG, B_PCH_PCH_HPET_GCFG_EN);

  }

}


EFI_STATUS
PlatformPchInit (
  IN SYSTEM_CONFIGURATION        *SystemConfiguration,
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN UINT16                      PlatformType
  )
{
  IchRcrbInit (PeiServices, SystemConfiguration);

  //
  // PCH Policy Initialization based on Setup variable.
  //
  PchPolicySetupInit (PeiServices, SystemConfiguration);

  UARTInit(SystemConfiguration);

  PchPlatformLpcInit (PeiServices, SystemConfiguration);

  return EFI_SUCCESS;
}

/**

  Returns the state of A16 inversion

  @retval TRUE    A16 is inverted
  @retval FALSE   A16 is not inverted

**/
BOOLEAN
IsA16Inverted (
  )
{
  UINT8  Data;
  Data = MmioRead8 (RCBA_BASE_ADDRESS + R_PCH_RCRB_GCS);
  return (Data & B_PCH_RCRB_GCS_TS) ? TRUE : FALSE;
}

VOID
PchPolicySetupInit (
  IN CONST EFI_PEI_SERVICES **PeiServices,
  IN SYSTEM_CONFIGURATION   *SystemConfiguration
  )
{
  EFI_STATUS                  Status;
  EFI_PEI_PPI_DESCRIPTOR      *PchPlatformPolicyPpiDesc;
  PCH_PLATFORM_POLICY_PPI     *PchPlatformPolicyPpi;
  PCH_HPET_CONFIG             *HpetConfig;
  PCH_PCIE_CONFIG             *PcieConfig;
  UINT8                       Index;
  PCH_IOAPIC_CONFIG           *IoApicConfig;
  PCH_LPSS_CONFIG             *LpssConfig;
  UINT32                      SpiHsfsReg;
  UINT32                      SpiFdodReg;

//
// Disable codec ALC-262
//
  UINT32                      IoBase;

  //
  // Install Pch Platform Policy PPI. As we depend on Pch Init PPI so we are executed after
  // PchInit PEIM. Thus we can insure PCH Initialization is performed when we install the Pch Platform Policy PPI,
  // as PchInit PEIM registered a notification function on our policy PPI.
  //
  // --cr-- For better code structure / modularity, we should use a notification function on Pch Init PPI to perform
  // actions that depend on PchInit PEIM's initialization.
  //
  //Todo: confirm if we need update to PCH_PLATFORM_POLICY_PPI_REVISION_5
  //
  DEBUG ((EFI_D_ERROR, "PchPolicySetupInit() - Start\n"));

  Status = (*PeiServices)->AllocatePool (PeiServices, sizeof (EFI_PEI_PPI_DESCRIPTOR), (void **)&PchPlatformPolicyPpiDesc);
  ASSERT_EFI_ERROR (Status);

  Status = (*PeiServices)->AllocatePool (PeiServices, sizeof (PCH_PLATFORM_POLICY_PPI), (void **)&PchPlatformPolicyPpi);
  ASSERT_EFI_ERROR (Status);

  Status = (*PeiServices)->AllocatePool (PeiServices, sizeof (PCH_HPET_CONFIG), (void **)&HpetConfig);
  ASSERT_EFI_ERROR (Status);

  Status = (*PeiServices)->AllocatePool (PeiServices, sizeof (PCH_PCIE_CONFIG), (void **)&PcieConfig);
  ASSERT_EFI_ERROR (Status);

  Status = (*PeiServices)->AllocatePool (PeiServices, sizeof (PCH_IOAPIC_CONFIG), (void **)&IoApicConfig);
  ASSERT_EFI_ERROR (Status);

  Status = (*PeiServices)->AllocatePool (PeiServices, sizeof (PCH_LPSS_CONFIG), (void **)&LpssConfig);
  ASSERT_EFI_ERROR (Status);

  PchPlatformPolicyPpi->Revision                = PCH_PLATFORM_POLICY_PPI_REVISION_1;
  PchPlatformPolicyPpi->BusNumber               = DEFAULT_PCI_BUS_NUMBER_PCH;
  PchPlatformPolicyPpi->SpiBase                 = SPI_BASE_ADDRESS;
  PchPlatformPolicyPpi->PmcBase                 = PMC_BASE_ADDRESS;
  PchPlatformPolicyPpi->IoBase                  = IO_BASE_ADDRESS;
  PchPlatformPolicyPpi->IlbBase                 = ILB_BASE_ADDRESS;
  PchPlatformPolicyPpi->PUnitBase               = PUNIT_BASE_ADDRESS;
  PchPlatformPolicyPpi->MphyBase                = MPHY_BASE_ADDRESS;
  PchPlatformPolicyPpi->Rcba                    = RCBA_BASE_ADDRESS;
  PchPlatformPolicyPpi->AcpiBase                = ACPI_BASE_ADDRESS;
  PchPlatformPolicyPpi->GpioBase                = GPIO_BASE_ADDRESS;
  PchPlatformPolicyPpi->SataMode                = SystemConfiguration->SataType;
  PchPlatformPolicyPpi->EnableRmh               = SystemConfiguration->PchUsbRmh;

  PchPlatformPolicyPpi->EhciPllCfgEnable        = SystemConfiguration->EhciPllCfgEnable;


  PchPlatformPolicyPpi->HpetConfig              = HpetConfig;
  PchPlatformPolicyPpi->PcieConfig              = PcieConfig;
  PchPlatformPolicyPpi->IoApicConfig            = IoApicConfig;

  PchPlatformPolicyPpi->HpetConfig->Enable      = SystemConfiguration->Hpet;
  PchPlatformPolicyPpi->HpetConfig->Base        = HPET_BASE_ADDRESS;
  PchPlatformPolicyPpi->IoApicConfig->IoApicId  = 0x01;

  //
  // Set LPSS configuration according to setup value.
  //
  PchPlatformPolicyPpi->LpssConfig->LpssPciModeEnabled   = SystemConfiguration->LpssPciModeEnabled;

  PchPlatformPolicyPpi->LpssConfig->Dma1Enabled    = SystemConfiguration->LpssDma1Enabled;
  PchPlatformPolicyPpi->LpssConfig->I2C0Enabled    = SystemConfiguration->LpssI2C0Enabled;
  PchPlatformPolicyPpi->LpssConfig->I2C1Enabled    = SystemConfiguration->LpssI2C1Enabled;
  PchPlatformPolicyPpi->LpssConfig->I2C2Enabled    = SystemConfiguration->LpssI2C2Enabled;
  PchPlatformPolicyPpi->LpssConfig->I2C3Enabled    = SystemConfiguration->LpssI2C3Enabled;
  PchPlatformPolicyPpi->LpssConfig->I2C4Enabled    = SystemConfiguration->LpssI2C4Enabled;
  PchPlatformPolicyPpi->LpssConfig->I2C5Enabled    = SystemConfiguration->LpssI2C5Enabled;
  PchPlatformPolicyPpi->LpssConfig->I2C6Enabled    = SystemConfiguration->LpssI2C6Enabled;

  PchPlatformPolicyPpi->LpssConfig->Dma0Enabled    = SystemConfiguration->LpssDma0Enabled;;
  PchPlatformPolicyPpi->LpssConfig->Pwm0Enabled    = SystemConfiguration->LpssPwm0Enabled;
  PchPlatformPolicyPpi->LpssConfig->Pwm1Enabled    = SystemConfiguration->LpssPwm1Enabled;
  PchPlatformPolicyPpi->LpssConfig->Hsuart0Enabled = SystemConfiguration->LpssHsuart0Enabled;
  PchPlatformPolicyPpi->LpssConfig->Hsuart1Enabled = SystemConfiguration->LpssHsuart1Enabled;
  PchPlatformPolicyPpi->LpssConfig->SpiEnabled     = SystemConfiguration->LpssSpiEnabled;


  for (Index = 0; Index < PCH_PCIE_MAX_ROOT_PORTS; Index++) {
    PchPlatformPolicyPpi->PcieConfig->PcieSpeed[Index] = SystemConfiguration->PcieRootPortSpeed[Index];
  }

  SpiHsfsReg = MmioRead32 (SPI_BASE_ADDRESS + R_PCH_SPI_HSFS);
  if ((SpiHsfsReg & B_PCH_SPI_HSFS_FDV) == B_PCH_SPI_HSFS_FDV) {
    MmioWrite32 (SPI_BASE_ADDRESS + R_PCH_SPI_FDOC, V_PCH_SPI_FDOC_FDSS_FSDM);
    SpiFdodReg = MmioRead32 (SPI_BASE_ADDRESS + R_PCH_SPI_FDOD);
    if (SpiFdodReg == V_PCH_SPI_FDBAR_FLVALSIG) {
    }
  //
  // Disable codec ALC-262
  //
  if (SystemConfiguration->DisableCodec262 == 1) {
      IoBase = MmioRead32 (MmPciAddress (0,
                        PchPlatformPolicyPpi->BusNumber,
                        PCI_DEVICE_NUMBER_PCH_LPC,
                        PCI_FUNCTION_NUMBER_PCH_LPC,
                        0
                      ) + R_PCH_LPC_IO_BASE) & B_PCH_LPC_IO_BASE_BAR;
      MmioAnd32 ((UINTN) (IoBase + 0x270), (UINT32) (~0x07));
  }
  }

  PchPlatformPolicyPpiDesc->Flags = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  PchPlatformPolicyPpiDesc->Guid = &gPchPlatformPolicyPpiGuid;
  PchPlatformPolicyPpiDesc->Ppi = PchPlatformPolicyPpi;

  //
  // Install PCH Platform Policy PPI
  //
  Status = (**PeiServices).InstallPpi (
              PeiServices,
              PchPlatformPolicyPpiDesc
              );
  ASSERT_EFI_ERROR (Status);

  DEBUG ((EFI_D_ERROR, "PchPolicySetupInit() - End\n"));
}

EFI_STATUS
InstallPeiPchUsbPolicy (
  IN CONST  EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS              Status = EFI_SUCCESS;

  EFI_PEI_PPI_DESCRIPTOR  *PeiPchUsbPolicyPpiDesc;
  PCH_USB_POLICY_PPI      *PeiPchUsbPolicyPpi;
  PCH_USB_CONFIG          *UsbConfig;
  EFI_PLATFORM_INFO_HOB   PlatformInfo;

  //
  // Allocate descriptor and PPI structures.  Since these are dynamically updated
  // we cannot do a global variable PPI.
  //
  Status = (*PeiServices)->AllocatePool (PeiServices, sizeof (EFI_PEI_PPI_DESCRIPTOR), (void **)&PeiPchUsbPolicyPpiDesc);


  Status = (*PeiServices)->AllocatePool (PeiServices, sizeof (PCH_USB_POLICY_PPI), (void **)&PeiPchUsbPolicyPpi);



  Status = (*PeiServices)->AllocatePool (PeiServices, sizeof (PCH_USB_CONFIG), (void **)&UsbConfig);


  //
  // Initiate PCH USB policy.
  //
  PeiPchUsbPolicyPpi->Revision = PCH_USB_POLICY_PPI_REVISION_1;
  UsbConfig->Usb20Settings[0].Enable  = PCH_DEVICE_ENABLE;
  UsbConfig->UsbPerPortCtl            = PCH_DEVICE_DISABLE;
  UsbConfig->Ehci1Usbr                = PCH_DEVICE_DISABLE;

  //
  // Initialize PlatformInfo HOB
  //
  ZeroMem (&PlatformInfo, sizeof(PlatformInfo));
  MultiPlatformInfoInit(PeiServices, &PlatformInfo);

  UsbConfig->Usb20OverCurrentPins[0] = PchUsbOverCurrentPin0;

  UsbConfig->Usb20OverCurrentPins[1] = PchUsbOverCurrentPin0;

  UsbConfig->Usb20OverCurrentPins[2] = PchUsbOverCurrentPin1;

  UsbConfig->Usb20OverCurrentPins[3] = PchUsbOverCurrentPin1;


  //
  // Enable USB Topology control and program the topology setting for every USB port
  // See Platform Design Guide for description of topologies
  //
    //
    // Port 0: ~10.9", Port 1: ~10.1", Port 2: ~11.2", Port 3: ~11.5", Port 4: ~3.7", Port 5: ~2.7", Port 6: ~4.1"
    // Port 7: ~4.5", Port 8: ~10.7", Port 9: ~10.5", Port 10: ~4.2", Port 11: ~4.3", Port 12: ~3.1", Port 13: ~2.9"
    //

    //
    // Port 0: ~3.5", Port 1: ~4.1", Port 2: ~4.6", Port 3: ~4.6", Port 4: ~12.5", Port 5: ~12", Port 6: ~5.1"
    // Port 7: ~5.1", Port 8: ~4.1", Port 9: ~4.1", Port 10: ~14.5", Port 11: ~12.8", Port 12: ~12.9", Port 13: ~14.6"
    //
  UsbConfig->Usb20PortLength[0]  = 0x53;
  UsbConfig->Usb20PortLength[1]  = 0x49;
  UsbConfig->Usb20PortLength[2]  = 0x47;
  UsbConfig->Usb20PortLength[3]  = 0x80;

  PeiPchUsbPolicyPpi->Mode = EHCI_MODE;

  PeiPchUsbPolicyPpi->EhciMemBaseAddr = PcdGet32(PcdPeiIchEhciControllerMemoryBaseAddress);

  PeiPchUsbPolicyPpi->EhciMemLength   = (UINT32) 0x400 * PchEhciControllerMax;

  PeiPchUsbPolicyPpi->UsbConfig       = UsbConfig;

  PeiPchUsbPolicyPpiDesc->Flags       = EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;

  PeiPchUsbPolicyPpiDesc->Guid        = &gPchUsbPolicyPpiGuid;

  PeiPchUsbPolicyPpiDesc->Ppi         = PeiPchUsbPolicyPpi;

  //
  // Install PCH USB Policy PPI
  //
  Status = (**PeiServices).InstallPpi (PeiServices, PeiPchUsbPolicyPpiDesc);

  return Status;
}
