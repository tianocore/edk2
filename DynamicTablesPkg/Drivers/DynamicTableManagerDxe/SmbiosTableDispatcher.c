/** @file
  Dynamic Smbios Table Dispatcher

  Copyright (c) 2022 - 2023, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/DebugLib.h>
#include <Protocol/Smbios.h>

#include <Include/StandardNameSpaceObjects.h>
#include <SmbiosTableDispatcher.h>

/**
  The SMBIOS dispatcher state table.

  The SMBIOS dispatcher state table is used to establish the dependency
  order in which the SMBIOS tables are installed. This allows the SMBIOS
  dispatcher to dispatch the dependent tables for installation before the
  parent table is installed.
  The SMBIOS_TABLE_DISPATCHER.Dependency[] field is used to establish the
  dependency list.
  Elements in the Dependency list are resolved by increasing index. However,
  all orders are equivalent as:
  - the Parent SMBIOS table will only be installed once all dependencies
    have been satisfied.
  - no cyclic dependency is allowed.
  The dependency list is terminated by SMTT_NULL.
*/
STATIC
SMBIOS_TABLE_DISPATCHER  mSmBiosDispatcher[MAX_SMBIOS_TABLES] = {
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_BIOS_INFORMATION,                     SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_INFORMATION,                   SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_BASEBOARD_INFORMATION,                SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_ENCLOSURE,                     SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_PROCESSOR_INFORMATION,                SMBIOS_TYPE_CACHE_INFORMATION,              SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MEMORY_CONTROLLER_INFORMATION,        SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MEMORY_MODULE_INFORMATON,             SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_CACHE_INFORMATION,                    SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION,           SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_SLOTS,                         SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_ONBOARD_DEVICE_INFORMATION,           SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_OEM_STRINGS,                          SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_CONFIGURATION_OPTIONS,         SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_BIOS_LANGUAGE_INFORMATION,            SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_GROUP_ASSOCIATIONS,                   SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_EVENT_LOG,                     SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY,                SMBIOS_TYPE_32BIT_MEMORY_ERROR_INFORMATION, SMBIOS_TYPE_64BIT_MEMORY_ERROR_INFORMATION, SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MEMORY_DEVICE,                        SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY,          SMBIOS_TYPE_32BIT_MEMORY_ERROR_INFORMATION, SMBIOS_TYPE_64BIT_MEMORY_ERROR_INFORMATION, SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_32BIT_MEMORY_ERROR_INFORMATION,       SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS,          SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS,         SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_BUILT_IN_POINTING_DEVICE,             SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_PORTABLE_BATTERY,                     SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_RESET,                         SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_HARDWARE_SECURITY,                    SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_POWER_CONTROLS,                SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_VOLTAGE_PROBE,                        SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_COOLING_DEVICE,                       SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_TEMPERATURE_PROBE,                    SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_ELECTRICAL_CURRENT_PROBE,             SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_OUT_OF_BAND_REMOTE_ACCESS,            SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_BOOT_INTEGRITY_SERVICE,               SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION,              SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_64BIT_MEMORY_ERROR_INFORMATION,       SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MANAGEMENT_DEVICE,                    SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MANAGEMENT_DEVICE_COMPONENT,          SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MANAGEMENT_DEVICE_THRESHOLD_DATA,     SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MEMORY_CHANNEL,                       SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_IPMI_DEVICE_INFORMATION,              SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_POWER_SUPPLY,                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_ADDITIONAL_INFORMATION,               SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_ONBOARD_DEVICES_EXTENDED_INFORMATION, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MANAGEMENT_CONTROLLER_HOST_INTERFACE, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_TPM_DEVICE,                           SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_PROCESSOR_ADDITIONAL_INFORMATION,     SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_FIRMWARE_INVENTORY_INFORMATION,       SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_STRING_PROPERTY_INFORMATION,          SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL, SMTT_NULL)
};

/**
  A string table describing the SMBIOS dispatcher states.
*/
STATIC
CONST CHAR8  *SmbiosTableStateTxt[] = {
  "StNotPresent",
  "StPresent",
  "StDispatched"
};

#if !defined (MDEPKG_NDEBUG)

/**
  Print the SMBIOS Table Dispatcher state information.

  @param [in] Verbose Print detailed report
**/
STATIC
VOID
EFIAPI
PrintDispatcherStatus (
  IN BOOLEAN  Verbose
  )
{
  UINTN  Index;

  DEBUG ((DEBUG_VERBOSE, "Dispatcher Status:\n"));
  for (Index = 0; Index < ARRAY_SIZE (mSmBiosDispatcher); Index++) {
    if ((!Verbose) && (mSmBiosDispatcher[Index].State == StNotPresent)) {
      continue;
    }

    DEBUG ((
      DEBUG_VERBOSE,
      "%02d: %10a [%02d, %02d, %02d, %02d, %02d]\n",
      mSmBiosDispatcher[Index].TableType,
      SmbiosTableStateTxt[mSmBiosDispatcher[Index].State],
      mSmBiosDispatcher[Index].Dependency[0],
      mSmBiosDispatcher[Index].Dependency[1],
      mSmBiosDispatcher[Index].Dependency[2],
      mSmBiosDispatcher[Index].Dependency[3],
      mSmBiosDispatcher[Index].Dependency[4]
      ));
  } // for

  DEBUG ((DEBUG_VERBOSE, "\n"));
}

#define DEBUG_PRINT_DISPATCHER_STATUS(Verbose)  PrintDispatcherStatus (Verbose)
#else
#define DEBUG_PRINT_DISPATCHER_STATUS(x)
#endif

/**
  Initialise the SMBIOS table dispatcher.

  @param [in] SmbiosTableInfo  Pointer to the list of SMBIOS tables to be
                               installed.
  @param [in] SmbiosTableCount Count of SMBIOS tables to be installed.
**/
VOID
EFIAPI
InitSmbiosTableDispatcher (
  IN  CM_STD_OBJ_SMBIOS_TABLE_INFO  *SmbiosTableInfo,
  IN  UINT32                        SmbiosTableCount
  )
{
  UINTN              Index;
  SMBIOS_TABLE_TYPE  TableType;

  // Search for the list of SMBIOS tables presented
  // for installation and update the dispatcher status.
  for (Index = 0; Index < SmbiosTableCount; Index++) {
    TableType = SmbiosTableInfo[Index].TableType;
    ASSERT (TableType < MAX_SMBIOS_TABLES);
    ASSERT (mSmBiosDispatcher[TableType].State != StPresent);
    mSmBiosDispatcher[TableType].State = StPresent;
  }

  DEBUG_PRINT_DISPATCHER_STATUS (FALSE);
}

/** Schedule the dispatch of a SMBIOS table.

  The SMBIOS dispatcher state table is used to establish the dependency
  order in which the SMBIOS tables are installed. This allows the SMBIOS
  dispatcher to dispatch the dependent tables for installation before the
  parent table is installed.
  The SMBIOS_TABLE_DISPATCHER.Dependency[] field is used to establish the
  dependency list.
  Elements in the Dependency list are resolved by increasing index. However,
  all orders are equivalent as:
  - the Parent SMBIOS table will only be installed once all dependencies
    have been satisfied.
  - no cyclic dependency is allowed.
  The dependency list is terminated by SMTT_NULL.

  @param [in]  TableType            The SMBIOS table type to schedule for
                                    dispatch.
  @param [in]  TableFactoryProtocol Pointer to the Table Factory Protocol
                                    interface.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol Interface.
  @param [in]  SmbiosProtocol       Pointer to the SMBIOS protocol.
  @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table Info.
  @param [in]  SmbiosTableCount     Count of SMBIOS table info objects.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         Required object is not found.
  @retval EFI_BAD_BUFFER_SIZE   Size returned by the Configuration Manager
                                is less than the Object size for the
                                requested object.
**/
EFI_STATUS
EFIAPI
DispatchSmbiosTable (
  IN CONST SMBIOS_TABLE_TYPE                             TableType,
  IN CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  TableFactoryProtocol,
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN       EFI_SMBIOS_PROTOCOL                           *SmbiosProtocol,
  IN       CM_STD_OBJ_SMBIOS_TABLE_INFO          *CONST  SmbiosTableInfo,
  IN CONST UINT32                                        SmbiosTableCount
  )
{
  EFI_STATUS               Status;
  UINTN                    Index;
  SMBIOS_TABLE_DISPATCHER  *Disp;

  DEBUG ((DEBUG_VERBOSE, "->DP %02d\n", TableType));
  Disp = &mSmBiosDispatcher[TableType];
  if (Disp->State == StNotPresent) {
    DEBUG ((DEBUG_VERBOSE, "<-DP %02d : EFI_NOT_FOUND\n", TableType));
    return EFI_NOT_FOUND;
  }

  if (Disp->State == StDispatched) {
    DEBUG ((DEBUG_VERBOSE, "<-DP %02d : EFI_ALREADY_STARTED\n", TableType));
    return EFI_ALREADY_STARTED;
  }

  // Table is present so check the dependency.
  for (Index = 0; Index < MAX_SMBIOS_DEPENDENCY; Index++) {
    // Check if the dependency list is terminated by SMTT_NULL.
    if (Disp->Dependency[Index] == SMTT_NULL) {
      break;
    }

    Status = DispatchSmbiosTable (
               Disp->Dependency[Index],
               TableFactoryProtocol,
               CfgMgrProtocol,
               SmbiosProtocol,
               SmbiosTableInfo,
               SmbiosTableCount
               );
    if (EFI_ERROR (Status)) {
      if ((Status == EFI_ALREADY_STARTED) || (Status == EFI_NOT_FOUND)) {
        // Some dependencies may already be satisfied
        // as other tables may also have similar
        // dependencies i.e. EFI_ALREADY_STARTED
        // Or
        // the dependent table may be optional
        // and not provided i.e. EFI_NOT_FOUND.
        continue;
      }

      DEBUG ((DEBUG_VERBOSE, "<-DP %02d : Status = %d\n", TableType, Status));
      return Status;
    }
  }

  DEBUG ((DEBUG_VERBOSE, "DP %02d : Status = %d\n", TableType, Status));

  // All dependencies satisfied - Install SMBIOS table
  Disp->State = StDispatched;
  // Find the SMBIOS table info matching the TableType
  for (Index = 0; Index < SmbiosTableCount; Index++) {
    if (SmbiosTableInfo[Index].TableType == TableType) {
      break;
    }
  }

  Status = BuildAndInstallSmbiosTable (
             TableFactoryProtocol,
             CfgMgrProtocol,
             SmbiosProtocol,
             &SmbiosTableInfo[Index]
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to install SMBIOS Table." \
      " Id = %u Status = %r\n",
      SmbiosTableInfo[Index].TableGeneratorId,
      Status
      ));
  }

  DEBUG_PRINT_DISPATCHER_STATUS (FALSE);
  DEBUG ((DEBUG_VERBOSE, "<-DP %0d\n", TableType));
  return Status;
}
