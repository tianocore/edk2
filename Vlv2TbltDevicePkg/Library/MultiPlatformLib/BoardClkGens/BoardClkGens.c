/** @file
  Clock generator setting for multiplatform.

  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/

#include <BoardClkGens.h>
#include <Guid/SetupVariable.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Library/BaseMemoryLib.h>

#ifndef __GNUC__
#pragma optimize( "", off )
#endif

#define CLKGEN_EN 1
#define EFI_DEBUG 1

CLOCK_GENERATOR_DETAILS   mSupportedClockGeneratorTable[] =
{
  { ClockGeneratorCk410, CK410_GENERATOR_ID , CK410_GENERATOR_SPREAD_SPECTRUM_BYTE, CK410_GENERATOR_SPREAD_SPECTRUM_BIT },
  { ClockGeneratorCk505, CK505_GENERATOR_ID , CK505_GENERATOR_SPREAD_SPECTRUM_BYTE, CK505_GENERATOR_SPREAD_SPECTRUM_BIT }
};

/**
  Configure the clock generator using the SMBUS PPI services.

  This function performs a block write, and dumps debug information.

  @param  PeiServices                General purpose services available to every PEIM.
  @param  ClockType                  Clock generator's model name.
  @param  ClockAddress               SMBUS address of clock generator.
  @param  ConfigurationTableLength   Length of configuration table.
  @param  ConfigurationTable         Pointer of configuration table.

  @retval EFI_SUCCESS - Operation success.

**/
EFI_STATUS
ConfigureClockGenerator (
  IN     EFI_PEI_SERVICES              **PeiServices,
  IN     EFI_PEI_SMBUS_PPI                 *SmbusPpi,
  IN     CLOCK_GENERATOR_TYPE          ClockType,
  IN     UINT8                         ClockAddress,
  IN     UINTN                         ConfigurationTableLength,
  IN OUT UINT8                         *ConfigurationTable
  )
{

  EFI_STATUS                    Status;
  EFI_SMBUS_DEVICE_ADDRESS      SlaveAddress;
  UINT8                         Buffer[MAX_CLOCK_GENERATOR_BUFFER_LENGTH];
  UINTN                         Length;
  EFI_SMBUS_DEVICE_COMMAND      Command;
#if CLKGEN_CONFIG_EXTRA
  UINT8                         j;
#endif

  //
  // Verify input arguments
  //
  ASSERT_EFI_ERROR (ConfigurationTableLength >= 6);
  ASSERT_EFI_ERROR (ConfigurationTableLength <= MAX_CLOCK_GENERATOR_BUFFER_LENGTH);
  ASSERT_EFI_ERROR (ClockType < ClockGeneratorMax);
  ASSERT_EFI_ERROR (ConfigurationTable != NULL);

  //
  // Read the clock generator
  //
  SlaveAddress.SmbusDeviceAddress = ClockAddress >> 1;
  Length = sizeof (Buffer);
  Command = 0;
  Status = SmbusPpi->Execute (
    PeiServices,
    SmbusPpi,
    SlaveAddress,
    Command,
    EfiSmbusReadBlock,
    FALSE,
    &Length,
    Buffer
    );
  ASSERT_EFI_ERROR (Status);

#ifdef EFI_DEBUG
  {
    UINT8 i;
    for (i = 0; i < sizeof (Buffer); i++) {
      DEBUG((EFI_D_ERROR, "CK505 default Clock Generator Byte %d: %x\n", i, Buffer[i]));
    }
#if CLKGEN_EN
    for (i = 0; i < ConfigurationTableLength; i++) {
      DEBUG((EFI_D_ERROR, "BIOS structure Clock Generator Byte %d: %x\n", i, ConfigurationTable[i]));
    }
#endif
  }
#endif

  DEBUG((EFI_D_ERROR, "Expected Clock Generator ID is %x, expecting %x\n", mSupportedClockGeneratorTable[ClockType].ClockId,(Buffer[7]&0xF)));

  //
  // Program clock generator
  //
  Command = 0;
#if CLKGEN_EN
#if CLKGEN_CONFIG_EXTRA
  for (j = 0; j < ConfigurationTableLength; j++) {
    Buffer[j] = ConfigurationTable[j];
  }

  Buffer[30] = 0x00;

  Status = SmbusPpi->Execute (
    PeiServices,
    SmbusPpi,
    SlaveAddress,
    Command,
    EfiSmbusWriteBlock,
    FALSE,
    &Length,
    Buffer
    );
#else
  Status = SmbusPpi->Execute (
    PeiServices,
    SmbusPpi,
    SlaveAddress,
    Command,
    EfiSmbusWriteBlock,
    FALSE,
    &ConfigurationTableLength,
    ConfigurationTable
    );
#endif // CLKGEN_CONFIG_EXTRA
#else
    ConfigurationTable[4] = (ConfigurationTable[4] & 0x3) | (Buffer[4] & 0xFC);
    Command = 4;
    Length = 1;
  Status = SmbusPpi->Execute (
    PeiServices,
    SmbusPpi,
    SlaveAddress,
    Command,
    EfiSmbusWriteBlock,
    FALSE,
    &Length,
    &ConfigurationTable[4]
    );
#endif //CLKGEN_EN
  ASSERT_EFI_ERROR (Status);

  //
  // Dump contents after write
  //
  #ifdef EFI_DEBUG
    {
      UINT8   i;
    SlaveAddress.SmbusDeviceAddress = ClockAddress >> 1;
    Length = sizeof (Buffer);
      Command = 0;
      Status =  SmbusPpi->Execute (
        PeiServices,
        SmbusPpi,
        SlaveAddress,
        Command,
        EfiSmbusReadBlock,
        FALSE,
        &Length,
        Buffer
        );

      for (i = 0; i < ConfigurationTableLength; i++) {
        DEBUG((EFI_D_ERROR, "Clock Generator Byte %d: %x\n", i, Buffer[i]));
      }
    }
    #endif

  return EFI_SUCCESS;
}

/**
  Configure the clock generator using the SMBUS PPI services.

  This function performs a block write, and dumps debug information.

  @param  PeiServices                General purpose services available to every PEIM.
  @param  ClockType                  Clock generator's model name.
  @param  ClockAddress               SMBUS address of clock generator.
  @param  ConfigurationTableLength   Length of configuration table.
  @param  ConfigurationTable         Pointer of configuration table.


  @retval  EFI_SUCCESS  Operation success.

**/
UINT8
ReadClockGeneratorID (
  IN     EFI_PEI_SERVICES              **PeiServices,
  IN     EFI_PEI_SMBUS_PPI                 *SmbusPpi,
  IN     UINT8                         ClockAddress
  )
{
  EFI_STATUS                    Status;
  EFI_SMBUS_DEVICE_ADDRESS      SlaveAddress;
  UINT8                         Buffer[MAX_CLOCK_GENERATOR_BUFFER_LENGTH];
  UINTN                         Length;
  EFI_SMBUS_DEVICE_COMMAND      Command;

  //
  // Read the clock generator
  //
  SlaveAddress.SmbusDeviceAddress = ClockAddress >> 1;
  Length = sizeof (Buffer);
  Command = 0;
  Status = SmbusPpi->Execute (
    PeiServices,
    SmbusPpi,
    SlaveAddress,
    Command,
    EfiSmbusReadBlock,
    FALSE,
    &Length,
    Buffer
    );

  //
  // Sanity check that the requested clock type is present in our supported clocks table
  //
  DEBUG((EFI_D_ERROR, "Expected Clock Generator ID is 0x%x\n", Buffer[7]));

  return (Buffer[7]);
}

/**
  Configure the clock generator to enable free-running operation.  This keeps
  the clocks from being stopped when the system enters C3 or C4.

  @param None

  @retval EFI_SUCCESS    The function completed successfully.

**/
EFI_STATUS
ConfigurePlatformClocks (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *SmbusPpi
  )
{
  //
  // Comment it out for now
  // Not supported by Hybrid model.
  //
  EFI_STATUS                    Status;
  UINT8                         *ConfigurationTable;

  CLOCK_GENERATOR_TYPE          ClockType = ClockGeneratorCk505;
  UINT8                         ConfigurationTable_Desktop[] = CLOCK_GENERATOR_SETTINGS_DESKTOP;
  UINT8                         ConfigurationTable_Mobile[] = CLOCK_GENERATOR_SETTINGS_MOBILE;
  UINT8                         ConfigurationTable_Tablet[] = CLOCK_GENERATOR_SEETINGS_TABLET;

  EFI_PLATFORM_INFO_HOB         *PlatformInfoHob;
  BOOLEAN                       EnableSpreadSpectrum;
  UINT8                         ClockGenID=0;
  SYSTEM_CONFIGURATION          SystemConfiguration;

  UINTN                         Length;
  EFI_SMBUS_DEVICE_COMMAND      Command;
  EFI_SMBUS_DEVICE_ADDRESS      SlaveAddress;
  UINT8                         Data;

  UINT8                         ClockAddress = CLOCK_GENERATOR_ADDRESS;
  UINTN                         VariableSize;
  EFI_PEI_READ_ONLY_VARIABLE2_PPI   *Variable;

  //
  // Obtain Platform Info from HOB.
  //
  Status = GetPlatformInfoHob ((CONST EFI_PEI_SERVICES **) PeiServices, &PlatformInfoHob);
  ASSERT_EFI_ERROR (Status);

  DEBUG((EFI_D_ERROR, "PlatformInfo protocol is working in ConfigurePlatformClocks()...%x\n",PlatformInfoHob->PlatformFlavor));

  //
  // Locate SMBUS PPI
  //
  Status = (**PeiServices).LocatePpi (
                             (CONST EFI_PEI_SERVICES **) PeiServices,
                             &gEfiPeiSmbusPpiGuid,
                             0,
                             NULL,
                             &SmbusPpi
                             );
  ASSERT_EFI_ERROR (Status);

  Data  = 0;
  SlaveAddress.SmbusDeviceAddress = ClockAddress >> 1;
  Length = 1;
  Command = 0x87;   //Control Register 7 Vendor ID Check
  Status = ((EFI_PEI_SMBUS_PPI *) SmbusPpi)->Execute (
                                               PeiServices,
                                               SmbusPpi,
                                               SlaveAddress,
                                               Command,
                                               EfiSmbusReadByte,
                                               FALSE,
                                               &Length,
                                               &Data
                                               );

  if (EFI_ERROR (Status) || ((Data & 0x0F) != CK505_GENERATOR_ID)) {
      DEBUG((EFI_D_ERROR, "Clock Generator CK505 Not Present, vendor ID on board is %x\n",(Data & 0x0F)));
      return EFI_SUCCESS;
}
  ClockGenID = Data & 0x0F;

  EnableSpreadSpectrum = FALSE;
  VariableSize = sizeof (SYSTEM_CONFIGURATION);
  ZeroMem (&SystemConfiguration, sizeof (SYSTEM_CONFIGURATION));

  Status = (*PeiServices)->LocatePpi (
                             (CONST EFI_PEI_SERVICES **) PeiServices,
                             &gEfiPeiReadOnlyVariable2PpiGuid,
                             0,
                             NULL,
                             (VOID **) &Variable
                             );
  //
  // Use normal setup default from NVRAM variable,
  // the Platform Mode (manufacturing/safe/normal) is handle in PeiGetVariable.
  //
  VariableSize = sizeof(SYSTEM_CONFIGURATION);
  Status = Variable->GetVariable (Variable,
                                   L"Setup",
                                   &gEfiSetupVariableGuid,
                                   NULL,
                                   &VariableSize,
                                   &SystemConfiguration);
  if (EFI_ERROR (Status) || VariableSize != sizeof(SYSTEM_CONFIGURATION)) {
    //The setup variable is corrupted
    VariableSize = sizeof(SYSTEM_CONFIGURATION);
    Status = Variable->GetVariable(Variable,
              L"SetupRecovery",
              &gEfiSetupVariableGuid,
              NULL,
              &VariableSize,
              &SystemConfiguration
              );
    ASSERT_EFI_ERROR (Status);
  }  
  if(!EFI_ERROR (Status)){
    EnableSpreadSpectrum = SystemConfiguration.EnableClockSpreadSpec;
  }

  //
  // Perform platform-specific intialization dependent upon Board ID:
  //
  DEBUG((EFI_D_ERROR, "board id is %x, platform id is %x\n",PlatformInfoHob->BoardId,PlatformInfoHob->PlatformFlavor));


  switch (PlatformInfoHob->BoardId) {
    case BOARD_ID_MINNOW2:
    default:
      switch(PlatformInfoHob->PlatformFlavor) {
      case FlavorTablet:
        ConfigurationTable = ConfigurationTable_Tablet;
        Length = sizeof (ConfigurationTable_Tablet);
        break;
      case FlavorMobile:
        ConfigurationTable = ConfigurationTable_Mobile;
        Length = sizeof (ConfigurationTable_Mobile);
        break;
      case FlavorDesktop:
      default:
        ConfigurationTable = ConfigurationTable_Desktop;
        Length = sizeof (ConfigurationTable_Desktop);
        break;
      }
    break;
    }

  //
  // Perform common clock initialization:
  //
  // Program Spread Spectrum function.
  //
  if (EnableSpreadSpectrum)
  {
    ConfigurationTable[mSupportedClockGeneratorTable[ClockType].SpreadSpectrumByteOffset] |= mSupportedClockGeneratorTable[ClockType].SpreadSpectrumBitOffset;
  } else {
    ConfigurationTable[mSupportedClockGeneratorTable[ClockType].SpreadSpectrumByteOffset] &= ~(mSupportedClockGeneratorTable[ClockType].SpreadSpectrumBitOffset);
  }


#if CLKGEN_EN
  Status = ConfigureClockGenerator (PeiServices, SmbusPpi, ClockType, ClockAddress, Length, ConfigurationTable);
  ASSERT_EFI_ERROR (Status);
#endif // CLKGEN_EN
  return EFI_SUCCESS;
}

static EFI_PEI_NOTIFY_DESCRIPTOR    mNotifyList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK| EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiSmbusPpiGuid,
    ConfigurePlatformClocks
  }
};

EFI_STATUS
InstallPlatformClocksNotify (
  IN CONST EFI_PEI_SERVICES           **PeiServices
  )
{
  EFI_STATUS                    Status;

  DEBUG ((EFI_D_INFO, "InstallPlatformClocksNotify()...\n"));

  Status = (*PeiServices)->NotifyPpi(PeiServices, &mNotifyList[0]);
  ASSERT_EFI_ERROR (Status);
  return EFI_SUCCESS;

}

#ifndef __GNUC__
#pragma optimize( "", on )
#endif
