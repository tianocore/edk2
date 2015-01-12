/** @file
  Gpio setting for multiplatform..

  Copyright (c) 2013 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/

#include <BoardGpios.h>
#include <Guid/SetupVariable.h>

//
//AlpineValley platform ocde begin
//
#define AV_SC_REG_GPIOS_MUXES_SEL0 0x48
#define AV_SC_REG_GPIOS_MUXES_SEL1 0x4C
#define AV_SC_REG_GPIOS_MUXES_SEL2 0x50
#define AV_SC_REG_GPIOS_MUXES_EN0  0x54
#define AV_SC_REG_GPIOS_MUXES_EN1  0x58
#define AV_SC_REG_GPIOS_MUXES_EN2  0x5C
//
//AlpineValley platform code end
//

EFI_GUID  gPeiSmbusPpiGuid               = EFI_PEI_SMBUS_PPI_GUID;

/**
  @param None

  @retval EFI_SUCCESS    The function completed successfully.

**/
EFI_STATUS
ConfigurePlatformSysCtrlGpio (
  IN EFI_PEI_SERVICES                   **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR          *NotifyDescriptor,
  IN VOID                               *SmbusPpi
  )
{
  //
  //AlpineValley platform code begin
  //
  // Initialize GPIO Settings:
  //
  UINT32        Status;
  EFI_PLATFORM_INFO_HOB               *PlatformInfoHob;

   DEBUG ((EFI_D_INFO, "ConfigurePlatformSysCtrlGpio()...\n"));

  //
  // Obtain Platform Info from HOB.
  //
  Status = GetPlatformInfoHob ((const EFI_PEI_SERVICES **)PeiServices, &PlatformInfoHob);
  ASSERT_EFI_ERROR (Status);

  //
  // The GPIO settings are dependent upon the platform.  Obtain the Board ID through
  // the EC to determine the current platform.
  //
   DEBUG ((EFI_D_INFO, "Platform Flavor | Board ID = 0x%X | 0x%X\n", PlatformInfoHob->PlatformFlavor, PlatformInfoHob->BoardId));



  Status = (**PeiServices).LocatePpi (
                            (const EFI_PEI_SERVICES **)PeiServices,
                            &gPeiSmbusPpiGuid,
                            0,
                            NULL,
                            (void **)&SmbusPpi
                            );
  ASSERT_EFI_ERROR (Status);

  //
  // Select/modify the GPIO initialization data based on the Board ID.
  //
  switch (PlatformInfoHob->BoardId)
  {
    default:
      Status = EFI_SUCCESS;

      //
      // Do nothing for other RVP boards.
      //
      break;
  }
  return Status;
}

static EFI_PEI_NOTIFY_DESCRIPTOR    mNotifyList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK| EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiSmbusPpiGuid,
    ConfigurePlatformSysCtrlGpio
  }
};

EFI_STATUS
InstallPlatformSysCtrlGPIONotify (
  IN CONST EFI_PEI_SERVICES           **PeiServices
  )
{
  EFI_STATUS                    Status;

  DEBUG ((EFI_D_INFO, "InstallPlatformSysCtrlGPIONotify()...\n"));

  Status = (*PeiServices)->NotifyPpi(PeiServices, &mNotifyList[0]);
  ASSERT_EFI_ERROR (Status);
  return EFI_SUCCESS;

}

#define V_PCH_ILB_IRQE_UARTIRQEN_IRQ3             BIT3 // UART IRQ3 Enable

/**
  Returns the Correct GPIO table for Mobile/Desktop respectively.
  Before call it, make sure PlatformInfoHob->BoardId&PlatformFlavor is get correctly.

  @param PeiServices             General purpose services available to every PEIM.
  @param PlatformInfoHob         PlatformInfoHob pointer with PlatformFlavor specified.
  @param BoardId                 BoardId ID as determined through the EC.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_DEVICE_ERROR       KSC fails to respond.

**/
EFI_STATUS
MultiPlatformGpioTableInit (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_PLATFORM_INFO_HOB      *PlatformInfoHob
  )
{
  EFI_STATUS                      Status;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI *PeiReadOnlyVarPpi;
  UINTN                           VarSize;
  SYSTEM_CONFIGURATION            SystemConfiguration;

  DEBUG ((EFI_D_INFO, "MultiPlatformGpioTableInit()...\n"));

  //
  // Select/modify the GPIO initialization data based on the Board ID.
  //
  switch (PlatformInfoHob->BoardId) {

  case BOARD_ID_MINNOW2: // Minnow2
  
   Status = (**PeiServices).LocatePpi (
                             PeiServices,
                             &gEfiPeiReadOnlyVariable2PpiGuid,
                             0,
                             NULL,
                             (void **)&PeiReadOnlyVarPpi
                             );
    ASSERT_EFI_ERROR (Status);
   
    VarSize = sizeof (SYSTEM_CONFIGURATION);
    Status = PeiReadOnlyVarPpi->GetVariable ( 
                                  PeiReadOnlyVarPpi, 
                                  PLATFORM_SETUP_VARIABLE_NAME, 
                                  &gEfiSetupVariableGuid,
                                  NULL,
                                  &VarSize,
                                  &SystemConfiguration
                                  );
                                                                       
     if (SystemConfiguration.GpioWakeCapability == 1) {
      PlatformInfoHob->PlatformCfioData     = (EFI_PHYSICAL_ADDRESS)(UINTN) &mMinnow2CfioInitData2;
     }
     else {
      PlatformInfoHob->PlatformCfioData     = (EFI_PHYSICAL_ADDRESS)(UINTN) &mMinnow2CfioInitData;
     }	
	 
     PlatformInfoHob->PlatformGpioData_NC  = (EFI_PHYSICAL_ADDRESS)(UINTN) &mMinnow2_GpioInitData_NC[0];
     PlatformInfoHob->PlatformGpioData_SC  = (EFI_PHYSICAL_ADDRESS)(UINTN) &mMinnow2_GpioInitData_SC[0];
     PlatformInfoHob->PlatformGpioData_SUS = (EFI_PHYSICAL_ADDRESS)(UINTN) &mMinnow2_GpioInitData_SUS[0];
     break;

  }

  return EFI_SUCCESS;
}

UINT32
GPIORead32 (
  IN  UINT32 mmio_conf
  )
{
  UINT32 conf_val;
  UINT32 i;
  conf_val = MmioRead32(mmio_conf);
  for(i=0;i<5;i++){
    if(conf_val == 0xffffffff)
      conf_val = MmioRead32(mmio_conf);
      else
        break;
  }

  return conf_val;
}

/**

  Set GPIO CONF0 and PAD_VAL registers for NC/SC/SUS GPIO clusters

  @param Gpio_Mmio_Offset          GPIO_SCORE_OFFSET or GPIO_NCORE_OFFSET or GPIO_SSUS_OFFSET.
  @param Gpio_Pin_Num              Pin numbers to config for each GPIO clusters.
  @param Gpio_Conf_Data            GPIO_CONF_PAD_INIT data array for each GPIO clusters.

**/
VOID
InternalGpioConfig (
  IN UINT32             Gpio_Mmio_Offset,
  IN UINT32             Gpio_Pin_Num,
  GPIO_CONF_PAD_INIT*   Gpio_Conf_Data
  )
{
  UINT32    index;
  UINT32    mmio_conf0;
  UINT32    mmio_padval;
  PAD_CONF0 conf0_val;
  PAD_VAL   pad_val;

  //
  // GPIO WELL -- Memory base registers
  //

  // A0 BIOS Spec doesn't mention it although X0 does. comment out now.
  // GPIO write 0x01001002 to IOBASE + Gpio_Mmio_Offset + 0x0900
  //
  for(index=0; index < Gpio_Pin_Num; index++)
  {
  	//
    // Calculate the MMIO Address for specific GPIO pin CONF0 register pointed by index.
    //
    mmio_conf0 = IO_BASE_ADDRESS + Gpio_Mmio_Offset + R_PCH_CFIO_PAD_CONF0 + Gpio_Conf_Data[index].offset * 16;
    mmio_padval= IO_BASE_ADDRESS + Gpio_Mmio_Offset + R_PCH_CFIO_PAD_VAL   + Gpio_Conf_Data[index].offset * 16;

#ifdef EFI_DEBUG
    DEBUG ((EFI_D_INFO, "%s, ", Gpio_Conf_Data[index].pad_name));

#endif
    DEBUG ((EFI_D_INFO, "Usage = %d, Func# = %d, IntType = %d, Pull Up/Down = %d, MMIO Base = 0x%08x, ",
      Gpio_Conf_Data[index].usage,
      Gpio_Conf_Data[index].func,
      Gpio_Conf_Data[index].int_type,
      Gpio_Conf_Data[index].pull,
      mmio_conf0));

    //
    // Step 1: PadVal Programming.
    //
    pad_val.dw = GPIORead32(mmio_padval);

    //
    // Config PAD_VAL only for GPIO (Non-Native) Pin
    //
    if(Native != Gpio_Conf_Data[index].usage)
    {
      pad_val.dw &= ~0x6; // Clear bits 1:2
      pad_val.dw |= (Gpio_Conf_Data[index].usage & 0x6);  // Set bits 1:2 according to PadVal

        //
        // set GPO default value
        //
        if(Gpio_Conf_Data[index].usage == GPO && Gpio_Conf_Data[index].gpod4 != NA)
        {
        pad_val.r.pad_val = Gpio_Conf_Data[index].gpod4;
        }
    }


    DEBUG ((EFI_D_INFO, "Set PAD_VAL = 0x%08x, ", pad_val.dw));

    MmioWrite32(mmio_padval, pad_val.dw);

    //
    // Step 2: CONF0 Programming
    // Read GPIO default CONF0 value, which is assumed to be default value after reset.
    //
    conf0_val.dw = GPIORead32(mmio_conf0);

    //
    // Set Function #
    //
    conf0_val.r.Func_Pin_Mux = Gpio_Conf_Data[index].func;

    if(GPO == Gpio_Conf_Data[index].usage)
    {
      //
      // If used as GPO, then internal pull need to be disabled.
      //
      conf0_val.r.Pull_assign = 0;  // Non-pull
    }
    else
    {
      //
      // Set PullUp / PullDown
      //
      if(P_20K_H == Gpio_Conf_Data[index].pull)
      {
        conf0_val.r.Pull_assign = 0x1;  // PullUp
        conf0_val.r.Pull_strength = 0x2;// 20K
      }
      else if(P_20K_L == Gpio_Conf_Data[index].pull)
      {
        conf0_val.r.Pull_assign = 0x2;  // PullDown
        conf0_val.r.Pull_strength = 0x2;// 20K
      }
      else if(P_10K_H == Gpio_Conf_Data[index].pull)
      {
        conf0_val.r.Pull_assign = 0x1; 	// PullUp
        conf0_val.r.Pull_strength = 0x1;// 10K
      }
      else if(P_10K_L == Gpio_Conf_Data[index].pull)
      {
        conf0_val.r.Pull_assign = 0x2; 	// PullDown
        conf0_val.r.Pull_strength = 0x1;// 10K
      }
      else if(P_2K_H == Gpio_Conf_Data[index].pull)
      {
        conf0_val.r.Pull_assign = 0x1;  // PullUp
        conf0_val.r.Pull_strength = 0x0;// 2K
      }
      else if(P_2K_L == Gpio_Conf_Data[index].pull)
      {
        conf0_val.r.Pull_assign = 0x2;  // PullDown
        conf0_val.r.Pull_strength = 0x0;// 2K
      }
      else if(P_NONE == Gpio_Conf_Data[index].pull)
      {
        conf0_val.r.Pull_assign = 0;	// Non-pull
      }
      else
      {
        ASSERT(FALSE);  // Invalid value
      }
    }


    //
    // Set INT Trigger Type
    //
    conf0_val.dw &= ~0x0f000000;  // Clear bits 27:24

    //
    // Set INT Trigger Type
    //
    if(TRIG_ == Gpio_Conf_Data[index].int_type)
    {
      //
      // Interrupt not capable, clear bits 27:24
      //
    }
    else
    {
      conf0_val.dw |= (Gpio_Conf_Data[index].int_type & 0x0f)<<24;
    }

    DEBUG ((EFI_D_INFO, "Set CONF0 = 0x%08x\n", conf0_val.dw));

    //
    // Write back the targeted GPIO config value according to platform (board) GPIO setting.
    //
    MmioWrite32 (mmio_conf0, conf0_val.dw);
  }

  //
  // A0 BIOS Spec doesn't mention it although X0 does. comment out now.
  // GPIO SCORE write 0x01001002 to IOBASE + 0x0900
  //
}

/**
  Returns the Correct GPIO table for Mobile/Desktop respectively.
  Before call it, make sure PlatformInfoHob->BoardId&PlatformFlavor is get correctly.

  @param PeiServices               General purpose services available to every PEIM.
  @param PlatformInfoHob           PlatformInfoHob pointer with PlatformFlavor specified.
  @param BoardId                   BoardId ID as determined through the EC.

  @retval EFI_SUCCESS              The function completed successfully.
  @retval EFI_DEVICE_ERROR         KSC fails to respond.

**/
EFI_STATUS
MultiPlatformGpioProgram (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_PLATFORM_INFO_HOB      *PlatformInfoHob
  )
{
#if !_SIMIC_
  CFIO_INIT_STRUCT*           PlatformCfioDataPtr;

  PlatformCfioDataPtr = (CFIO_INIT_STRUCT *) (UINTN) PlatformInfoHob->PlatformCfioData;
  DEBUG ((EFI_D_INFO, "MultiPlatformGpioProgram()...\n"));

  //
  //  SCORE GPIO WELL -- IO base registers
  //

  //
  // GPIO_USE_SEL Register -> 1 = GPIO 0 = Native
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SC_USE_SEL, PlatformCfioDataPtr->Use_Sel_SC0);

  //
  // Set GP_LVL Register
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SC_LVL , PlatformCfioDataPtr->GP_Lvl_SC0);

  //
  // GP_IO_SEL Register -> 1 = Input 0 = Output.  If Native Mode don't care
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SC_IO_SEL, PlatformCfioDataPtr->Io_Sel_SC0);

  //
  // GPIO Triger Positive Edge Enable Register
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SC_TPE, PlatformCfioDataPtr->TPE_SC0);

  //
  //  GPIO Trigger Negative Edge Enable Register
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SC_TNE, PlatformCfioDataPtr->TNE_SC0);

  //
  //  GPIO Trigger Status
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SC_TS, PlatformCfioDataPtr->TS_SC0);

  //
  // GPIO_USE_SEL2 Register -> 1 = GPIO 0 = Native
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SC_USE_SEL2, PlatformCfioDataPtr->Use_Sel_SC1);

  //
  // Set GP_LVL2 Register
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SC_LVL2, PlatformCfioDataPtr->GP_Lvl_SC1);

  //
  // GP_IO_SEL2 Register -> 1 = Input 0 = Output.  If Native Mode don't care
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SC_IO_SEL2, PlatformCfioDataPtr->Io_Sel_SC1);

  //
  // GPIO_USE_SEL3 Register -> 1 = GPIO 0 = Native
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SC_USE_SEL3, PlatformCfioDataPtr->Use_Sel_SC2);

  //
  // Set GP_LVL3 Register
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SC_LVL3, PlatformCfioDataPtr->GP_Lvl_SC2);

  //
  // GP_IO_SEL3 Register -> 1 = Input 0 = Output if Native Mode don't care
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SC_IO_SEL3, PlatformCfioDataPtr->Io_Sel_SC2);

  //
  //  SUS GPIO WELL -- IO base registers
  //

  //
  // GPIO_USE_SEL Register -> 1 = GPIO 0 = Native
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SUS_USE_SEL, PlatformCfioDataPtr->Use_Sel_SS);

  //
  // Set GP_LVL Register
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SUS_LVL , PlatformCfioDataPtr->GP_Lvl_SS);

  //
  // GP_IO_SEL Register -> 1 = Input 0 = Output.  If Native Mode don't care.
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SUS_IO_SEL, PlatformCfioDataPtr->Io_Sel_SS);

  //
  // GPIO Triger Positive Edge Enable Register.
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SUS_TPE, PlatformCfioDataPtr->TPE_SS);

  //
  //  GPIO Trigger Negative Edge Enable Register.
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SUS_TNE, PlatformCfioDataPtr->TNE_SS);

  //
  //  GPIO Trigger Status.
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SUS_TS, PlatformCfioDataPtr->TS_SS);

  //
  //  GPIO Wake Enable.
  //
  IoWrite32 (GPIO_BASE_ADDRESS + R_PCH_GPIO_SUS_WAKE_EN, PlatformCfioDataPtr->WE_SS);

  //
  // Config SC/NC/SUS GPIO Pins
  //
  switch (PlatformInfoHob->BoardId) {
    case BOARD_ID_MINNOW2:
      DEBUG ((EFI_D_INFO, "Start to config Minnow2 GPIO pins\n"));
      InternalGpioConfig(GPIO_SCORE_OFFSET, sizeof(mMinnow2_GpioInitData_SC)/sizeof(mMinnow2_GpioInitData_SC[0]),   (GPIO_CONF_PAD_INIT *) (UINTN) PlatformInfoHob->PlatformGpioData_SC);
      InternalGpioConfig(GPIO_NCORE_OFFSET, sizeof(mMinnow2_GpioInitData_NC)/sizeof(mMinnow2_GpioInitData_NC[0]),   (GPIO_CONF_PAD_INIT *) (UINTN) PlatformInfoHob->PlatformGpioData_NC);
      InternalGpioConfig(GPIO_SSUS_OFFSET,  sizeof(mMinnow2_GpioInitData_SUS)/sizeof(mMinnow2_GpioInitData_SUS[0]), (GPIO_CONF_PAD_INIT *) (UINTN) PlatformInfoHob->PlatformGpioData_SUS);
      break;
    default:

      break;
  }

   //
   // configure the CFIO Pnp settings
   //
   if (PlatformInfoHob->CfioEnabled) {
     if (PlatformInfoHob->BoardId == BOARD_ID_MINNOW2){
       InternalGpioConfig(GPIO_SCORE_OFFSET, sizeof(mNB_BB_FAB3_GpioInitData_SC_TRI)/sizeof(mNB_BB_FAB3_GpioInitData_SC_TRI[0]), (GPIO_CONF_PAD_INIT *) (UINTN)PlatformInfoHob->PlatformGpioData_SC_TRI);
     }
   }
#else
   DEBUG ((EFI_D_INFO, "Skip MultiPlatformGpioProgram()...for SIMICS or HYB model\n"));
#endif
  return EFI_SUCCESS;
}

