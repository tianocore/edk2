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

  The SMBIOS dispatcher dispatches the tables that have the default
  order (OrderDef) set before the ordered SMBIOS tables are dispatched.
  The SMBIOS_TABLE_DISPATCHER.Order field is used to establish the
  dispatch order.

  The order specified in the SMBIOS dispatcher table must be unique for all
  orders other than OrderDef. The dependency walk is only done for tables
  that have the default dispatch order.
*/
STATIC
SMBIOS_TABLE_DISPATCHER  mSmBiosDispatcher[MAX_SMBIOS_TABLES] = {
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_BIOS_INFORMATION,                     OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_INFORMATION,                   OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_BASEBOARD_INFORMATION,                OrderL1,  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_ENCLOSURE,                     OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_PROCESSOR_INFORMATION,                OrderDef, SMBIOS_TYPE_CACHE_INFORMATION,              SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MEMORY_CONTROLLER_INFORMATION,        OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MEMORY_MODULE_INFORMATON,             OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_CACHE_INFORMATION,                    OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION,           OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_SLOTS,                         OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_ONBOARD_DEVICE_INFORMATION,           OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_OEM_STRINGS,                          OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_CONFIGURATION_OPTIONS,         OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_BIOS_LANGUAGE_INFORMATION,            OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_GROUP_ASSOCIATIONS,                   OrderL4,  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_EVENT_LOG,                     OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY,                OrderDef, SMBIOS_TYPE_32BIT_MEMORY_ERROR_INFORMATION, SMBIOS_TYPE_64BIT_MEMORY_ERROR_INFORMATION, SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MEMORY_DEVICE,                        OrderDef, SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY,          SMBIOS_TYPE_32BIT_MEMORY_ERROR_INFORMATION, SMBIOS_TYPE_64BIT_MEMORY_ERROR_INFORMATION, SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_32BIT_MEMORY_ERROR_INFORMATION,       OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS,          OrderDef, SMBIOS_TYPE_PHYSICAL_MEMORY_ARRAY,          SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MEMORY_DEVICE_MAPPED_ADDRESS,         OrderDef, SMBIOS_TYPE_MEMORY_DEVICE,                  SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS,    SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_BUILT_IN_POINTING_DEVICE,             OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_PORTABLE_BATTERY,                     OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_RESET,                         OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_HARDWARE_SECURITY,                    OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_POWER_CONTROLS,                OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_VOLTAGE_PROBE,                        OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_COOLING_DEVICE,                       OrderDef, SMBIOS_TYPE_TEMPERATURE_PROBE,              SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_TEMPERATURE_PROBE,                    OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_ELECTRICAL_CURRENT_PROBE,             OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_OUT_OF_BAND_REMOTE_ACCESS,            OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_BOOT_INTEGRITY_SERVICE,               OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_BOOT_INFORMATION,              OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_64BIT_MEMORY_ERROR_INFORMATION,       OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MANAGEMENT_DEVICE,                    OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MANAGEMENT_DEVICE_COMPONENT,          OrderDef, SMBIOS_TYPE_MANAGEMENT_DEVICE,              SMBIOS_TYPE_VOLTAGE_PROBE,                  SMBIOS_TYPE_COOLING_DEVICE,                 SMBIOS_TYPE_TEMPERATURE_PROBE, SMBIOS_TYPE_ELECTRICAL_CURRENT_PROBE, SMBIOS_TYPE_MANAGEMENT_DEVICE_THRESHOLD_DATA),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MANAGEMENT_DEVICE_THRESHOLD_DATA,     OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MEMORY_CHANNEL,                       OrderDef, SMBIOS_TYPE_MEMORY_DEVICE,                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_IPMI_DEVICE_INFORMATION,              OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_SYSTEM_POWER_SUPPLY,                  OrderDef, SMBIOS_TYPE_VOLTAGE_PROBE,                  SMBIOS_TYPE_COOLING_DEVICE,                 SMBIOS_TYPE_ELECTRICAL_CURRENT_PROBE,       SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_ADDITIONAL_INFORMATION,               OrderL3,  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_ONBOARD_DEVICES_EXTENDED_INFORMATION, OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_MANAGEMENT_CONTROLLER_HOST_INTERFACE, OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_TPM_DEVICE,                           OrderDef, SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_PROCESSOR_ADDITIONAL_INFORMATION,     OrderDef, SMBIOS_TYPE_PROCESSOR_INFORMATION,          SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_FIRMWARE_INVENTORY_INFORMATION,       OrderL2,  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL),
  SMBIOS_TABLE_DEP (SMBIOS_TYPE_STRING_PROPERTY_INFORMATION,          OrderL5,  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                                  SMTT_NULL,                     SMTT_NULL,                            SMTT_NULL)
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

/** Dispatch the SMBIOS table.

  @param [in]  Disp                 Pointer to the  SMBIOS table dispatcher
                                    object for the table to dispatch.
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
STATIC
EFI_STATUS
EFIAPI
DispatchTable (
  IN       SMBIOS_TABLE_DISPATCHER               *CONST  Disp,
  IN CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  TableFactoryProtocol,
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN       EFI_SMBIOS_PROTOCOL                           *SmbiosProtocol,
  IN       CM_STD_OBJ_SMBIOS_TABLE_INFO          *CONST  SmbiosTableInfo,
  IN CONST UINT32                                        SmbiosTableCount
  )
{
  UINTN       Index;
  EFI_STATUS  Status;
  BOOLEAN     Found;

  if (Disp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Found = FALSE;

  // Update the dispatcher state to dispatched.
  Disp->State = StDispatched;
  // Find the SMBIOS table info matching the TableType
  for (Index = 0; Index < SmbiosTableCount; Index++) {
    if (SmbiosTableInfo[Index].TableType == Disp->TableType) {
      Found = TRUE;
      break;
    }
  }

  if (!Found) {
    ASSERT (0);
    return EFI_NOT_FOUND;
  }

  // Install the SMBIOS table
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
      " Id = %d Status = %r\n",
      SmbiosTableInfo[Index].TableGeneratorId,
      Status
      ));
  }

  return Status;
}

/** Schedule the dispatch of a default ordered SMBIOS table.

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
  @retval EFI_NO_MAPPING        A dependent table may be optional and
                                may not be in the list of tables to install.
  @retval EFI_ALREADY_STARTED   A table may be in the dependency list of
                                multiple tables and would have been installed
                                when one of the other parent table's dependency
                                list was fulfilled.
**/
STATIC
EFI_STATUS
EFIAPI
DispatchDefaultOrderedSmbiosTable (
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
  SMBIOS_TABLE_TYPE        DepTableType;

  DEBUG ((DEBUG_VERBOSE, "->DP %02d\n", TableType));
  Disp = &mSmBiosDispatcher[TableType];

  if (Disp->Order != OrderDef) {
    DEBUG ((DEBUG_VERBOSE, "<-DP %d : EFI_INVALID_PARAMETER\n", TableType));
    // The table being request for installation is
    // not a default ordered table.
    return EFI_INVALID_PARAMETER;
  }

  if (Disp->State == StNotPresent) {
    DEBUG ((DEBUG_VERBOSE, "<-DP %02d : EFI_NO_MAPPING\n", TableType));
    // A dependent table may be optional and therefore may
    // not be in the list of tables to install.
    return EFI_NO_MAPPING;
  }

  if (Disp->State == StDispatched) {
    DEBUG ((DEBUG_VERBOSE, "<-DP %02d : EFI_ALREADY_STARTED\n", TableType));
    // This table may be in the dependency list of multiple tables
    // and would have been installed when one of the other parent
    // table's dependency list was fulfilled.
    return EFI_ALREADY_STARTED;
  }

  // Table is present so check the dependency.
  for (Index = 0; Index < MAX_SMBIOS_DEPENDENCY; Index++) {
    DepTableType = Disp->Dependency[Index];
    // Check if the dependency list is terminated by SMTT_NULL.
    if (DepTableType == SMTT_NULL) {
      break;
    }

    if (mSmBiosDispatcher[DepTableType].Order != OrderDef) {
      // An incorrect dependency has been set.
      // The default ordered SMBIOS tables must not have
      // a dependency on ordered SMBIOS tables.
      DEBUG ((
        DEBUG_VERBOSE,
        "<-DP %02d : EFI_INVALID_PARAMETER - Invalid dependency\n",
        TableType
        ));
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }

    Status = DispatchDefaultOrderedSmbiosTable (
               DepTableType,
               TableFactoryProtocol,
               CfgMgrProtocol,
               SmbiosProtocol,
               SmbiosTableInfo,
               SmbiosTableCount
               );
    if (EFI_ERROR (Status)) {
      if ((Status == EFI_ALREADY_STARTED) ||
          (Status == EFI_NO_MAPPING))
      {
        // Some dependencies may already be satisfied
        // as other tables may also have similar
        // dependencies i.e. EFI_ALREADY_STARTED
        // Or
        // the dependent table may be optional
        // and not provided i.e. EFI_NO_MAPPING.
        DEBUG ((
          DEBUG_VERBOSE,
          "<-DP %02d : Status = %r - treated as Success, continue\n",
          TableType,
          Status
          ));
        // Therefore, reset Status to success
        Status = EFI_SUCCESS;
        continue;
      }

      DEBUG ((
        DEBUG_VERBOSE,
        "<-DP %02d : Status = %r\n",
        TableType,
        Status
        ));
      return Status;
    }
  }

  // All dependencies satisfied - dispatch SMBIOS table
  Status = DispatchTable (
             Disp,
             TableFactoryProtocol,
             CfgMgrProtocol,
             SmbiosProtocol,
             SmbiosTableInfo,
             SmbiosTableCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to install SMBIOS Table." \
      " TableType = %u Status = %r\n",
      Disp->TableType,
      Status
      ));
  }

  DEBUG_PRINT_DISPATCHER_STATUS (FALSE);
  DEBUG ((DEBUG_VERBOSE, "<-DP %0d\n", TableType));
  return Status;
}

/** Schedule the dispatch of ordered SMBIOS tables.

  The SMBIOS dispatcher dispatches the tables that have the default
  order (OrderDef) set first before the ordered SMBIOS tables are
  dispatched.
  The SMBIOS_TABLE_DISPATCHER.Order field is used to establish the
  dispatch order.

  @param [in]  Order                The dispatch order for the SMBIOS table type
                                    to be scheduled for dispatch.
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
STATIC
EFI_STATUS
EFIAPI
DispatchOrderedSmbiosTables (
  IN CONST DISPATCH_ORDER                                Order,
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

  DEBUG ((DEBUG_VERBOSE, "->DPO %d\n", Order));

  if (Order == OrderDef) {
    DEBUG ((DEBUG_VERBOSE, "<-DPO %d : EFI_INVALID_PARAMETER\n", Order));
    return EFI_INVALID_PARAMETER;
  }

  Disp = NULL;
  for (Index = 0; Index < ARRAY_SIZE (mSmBiosDispatcher); Index++) {
    if (mSmBiosDispatcher[Index].Order == Order) {
      Disp = &mSmBiosDispatcher[Index];
      break;
    }
  } // for

  if (Disp == NULL) {
    // Table with specified order not found.
    DEBUG ((DEBUG_VERBOSE, "<-DPO %d : EFI_INVALID_PARAMETER\n", Order));
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  } else if (Disp->State == StNotPresent) {
    // An ordered table of this order is not present
    // for installation. So, nothing to do here.
    DEBUG ((
      DEBUG_VERBOSE,
      "<-DPO %a - %d {%u}: EFI_SUCCESS\n",
      SmbiosTableStateTxt[Disp->State],
      Order,
      Disp->TableType
      ));
    return EFI_SUCCESS;
  } else if (Disp->State == StDispatched) {
    // Ordered tables are dispatched in their dispatch order and
    // cannot be in the dependency list of any other table.
    // Therefore, the table cannot be already dispatched.
    DEBUG ((
      DEBUG_VERBOSE,
      "<-DPO %a - %d {%u}: EFI_INVALID_PARAMETER\n",
      SmbiosTableStateTxt[Disp->State],
      Order,
      Disp->TableType
      ));

    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  } else if (Disp->State != StPresent) {
    // Invalid state, at this point the only valid state
    // should be StPresent, otherwise the state machine
    // is incorrect.
    DEBUG ((
      DEBUG_VERBOSE,
      "<-DPO %d - %d {%u}: EFI_INVALID_PARAMETER\n",
      Disp->State,
      Order,
      Disp->TableType
      ));
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = DispatchTable (
             Disp,
             TableFactoryProtocol,
             CfgMgrProtocol,
             SmbiosProtocol,
             SmbiosTableInfo,
             SmbiosTableCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to install SMBIOS Table." \
      " TableType = %u Status = %r\n",
      Disp->TableType,
      Status
      ));
  }

  DEBUG_PRINT_DISPATCHER_STATUS (FALSE);
  DEBUG ((DEBUG_VERBOSE, "<-DPO %d {%u}\n", Order, Disp->TableType));
  return Status;
}

/** Schedule the dispatch of SMBIOS tables.

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

  The SMBIOS dispatcher dispatches the tables that have the default
  order (OrderDef) set before the ordered SMBIOS tables are dispatched.
  The SMBIOS_TABLE_DISPATCHER.Order field is used to establish the
  dispatch order.

  The order specified in the SMBIOS dispatcher table must be unique for all
  orders other than OrderDef. The dependency walk is only done for tables
  that have the default dispatch order.

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
DispatchSmbiosTables (
  IN CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  TableFactoryProtocol,
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN       EFI_SMBIOS_PROTOCOL                           *SmbiosProtocol,
  IN       CM_STD_OBJ_SMBIOS_TABLE_INFO          *CONST  SmbiosTableInfo,
  IN CONST UINT32                                        SmbiosTableCount
  )
{
  EFI_STATUS         Status;
  UINTN              Index;
  UINT8              Order;
  SMBIOS_TABLE_TYPE  TableType;

  if ((TableFactoryProtocol == NULL)  ||
      (CfgMgrProtocol == NULL)        ||
      (SmbiosProtocol == NULL)        ||
      (SmbiosTableInfo == NULL)       ||
      (SmbiosTableCount == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  // First dispatch the default ordered SMBIOS tables
  for (Index = 0; Index < SmbiosTableCount; Index++) {
    TableType = SmbiosTableInfo[Index].TableType;
    ASSERT (TableType < MAX_SMBIOS_TABLES);
    if (mSmBiosDispatcher[TableType].Order != OrderDef) {
      // Skip if the table is not a default order table.
      continue;
    }

    Status = DispatchDefaultOrderedSmbiosTable (
               TableType,
               TableFactoryProtocol,
               CfgMgrProtocol,
               SmbiosProtocol,
               SmbiosTableInfo,
               SmbiosTableCount
               );
    if (EFI_ERROR (Status)) {
      if ((Status == EFI_ALREADY_STARTED) ||
          (Status == EFI_NO_MAPPING))
      {
        // Some dependencies may already be satisfied
        // as other tables may also have similar
        // dependencies i.e. EFI_ALREADY_STARTED
        // Or
        // the dependent table may be optional
        // and not provided i.e. EFI_NO_MAPPING.
        DEBUG ((
          DEBUG_VERBOSE,
          "TableType %02d : Status = %r - treated as Success, continue\n",
          SmbiosTableInfo[Index].TableType,
          Status
          ));
        // Therefore, reset Status to success
        Status = EFI_SUCCESS;
        continue;
      }

      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to install SMBIOS Table." \
        " Id = %d Status = %r\n",
        SmbiosTableInfo[Index].TableGeneratorId,
        Status
        ));
      return Status;
    }
  }

  // Now dispatch the ordered SMBIOS tables
  for (Order = OrderL1; Order < OrderMax; Order++) {
    Status = DispatchOrderedSmbiosTables (
               (DISPATCH_ORDER)Order,
               TableFactoryProtocol,
               CfgMgrProtocol,
               SmbiosProtocol,
               SmbiosTableInfo,
               SmbiosTableCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: Failed to install SMBIOS Table." \
        " Order = %d Status = %r\n",
        Order,
        Status
        ));
      break;
    }
  }

  return Status;
}
