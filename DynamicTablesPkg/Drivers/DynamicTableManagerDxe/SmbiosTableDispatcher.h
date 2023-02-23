/** @file

  Copyright (c) 2022 - 2023, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMBIOS_TABLE_DISPATCHER_H_
#define SMBIOS_TABLE_DISPATCHER_H_

#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>

/**
  A SMBIOS Table Type from the OEM range reserved for terminating
  the SMBIOS table dispatch dependency.

  Note: According to the SMBIOS specification, Table Types 0
  through 127 (7Fh) are reserved for and defined by the
  SMBIOS specification.
  Types 128 through 256 (80h to FFh) are available for system and
  OEM-specific information.

  This Dynamic SMBIOS table generation implementation defines
  TableType FFh as a NULL table which is used by the Dynamic
  SMBIOS table dispatcher to terminate the dependency list.
*/
#define SMTT_NULL  0xFF

/**
  A macro defining the maximum number of dependendant SMBIOS tables
  represented by the SMBIOS table dispatcher.
*/
#define MAX_SMBIOS_DEPENDENCY  6

/**
  A macro defining the maximum table types handled by the SMBIOS
  table dispatcher.
*/
#define MAX_SMBIOS_TABLES  (SMBIOS_TYPE_STRING_PROPERTY_INFORMATION + 1)

/**
  A helper macro to populate the SMBIOS table dispatcher table
*/
#define SMBIOS_TABLE_DEP(TableId, Dep1, Dep2, Dep3, Dep4, Dep5, Dep6) \
  { \
    TableId, \
    StNotPresent, \
    { Dep1, Dep2, Dep3, Dep4, Dep5, Dep6 } \
  }

/**
  An enum describing the states of the SMBIOS table dispatcher.
*/
typedef enum SmbiosTableState {
  StNotPresent,   ///< SMBIOS table is not present for installation.
  StPresent,      ///< SMBIOS table is present for installation.
  StDispatched    ///< SMBIOS table generators have been dispatched.
} SMBIOS_TABLE_STATE;

/**
  A structure describing the dependencies for a SMBIOS table and
  the dispatcher state information.
*/
typedef struct SmBiosTableDispatcher {
  /// SMBIOS Structure/Table Type
  SMBIOS_TABLE_TYPE     TableType;
  /// SMBIOS dispatcher state
  SMBIOS_TABLE_STATE    State;
  /// SMBIOS Structure/Table dependency list
  /// The list is terminated using SMTT_NULL.
  SMBIOS_TABLE_TYPE     Dependency[MAX_SMBIOS_DEPENDENCY];
} SMBIOS_TABLE_DISPATCHER;

/**
  A helper function to build and install a SMBIOS table.

  @param [in]  TableFactoryProtocol Pointer to the Table Factory Protocol
                                    interface.
  @param [in]  CfgMgrProtocol       Pointer to the Configuration Manager
                                    Protocol Interface.
  @param [in]  SmbiosProtocol       Pointer to the SMBIOS protocol.
  @param [in]  SmbiosTableInfo      Pointer to the SMBIOS table Info.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         Required object is not found.
  @retval EFI_BAD_BUFFER_SIZE   Size returned by the Configuration Manager
                                is less than the Object size for the
                                requested object.
**/
EFI_STATUS
EFIAPI
BuildAndInstallSmbiosTable (
  IN CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  TableFactoryProtocol,
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN       EFI_SMBIOS_PROTOCOL                           *SmbiosProtocol,
  IN       CM_STD_OBJ_SMBIOS_TABLE_INFO          *CONST  SmbiosTableInfo
  );

/**
  Initialise the SMBIOS table dispatcher.

  @param SmbiosTableInfo  Pointer to the list of SMBIOS tables to be installed.
  @param SmbiosTableCount Count of SMBIOS tables to be installed.
**/
VOID
EFIAPI
InitSmbiosTableDispatcher (
  IN  CM_STD_OBJ_SMBIOS_TABLE_INFO  *SmbiosTableInfo,
  IN  UINT32                        SmbiosTableCount
  );

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
  );

#endif // SMBIOS_TABLE_DISPATCHER_H_
