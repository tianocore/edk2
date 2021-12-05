/** @file
    Declaration of structures and functions for SnpDxe driver.

Copyright (c) 2004 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SNP_H_
#define _SNP_H_

#include <Uefi.h>

#include <Protocol/SimpleNetwork.h>
#include <Protocol/PciIo.h>
#include <Protocol/NetworkInterfaceIdentifier.h>
#include <Protocol/DevicePath.h>

#include <Guid/EventGroup.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/PcdLib.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Acpi.h>

#define FOUR_GIGABYTES  (UINT64) 0x100000000ULL

#define SNP_DRIVER_SIGNATURE  SIGNATURE_32 ('s', 'n', 'd', 's')
#define MAX_MAP_LENGTH        100

#define PCI_BAR_IO_MASK  0x00000003
#define PCI_BAR_IO_MODE  0x00000001

#define PCI_BAR_MEM_MASK   0x0000000F
#define PCI_BAR_MEM_MODE   0x00000000
#define PCI_BAR_MEM_64BIT  0x00000004

#define SNP_TX_BUFFER_INCREASEMENT  MAX_XMIT_BUFFERS
#define SNP_MAX_TX_BUFFER_NUM       65536

typedef
EFI_STATUS
(EFIAPI *ISSUE_UNDI32_COMMAND)(
  UINT64         Cdb
  );

typedef struct {
  UINT32                         Signature;
  EFI_LOCK                       Lock;

  EFI_SIMPLE_NETWORK_PROTOCOL    Snp;
  EFI_SIMPLE_NETWORK_MODE        Mode;

  EFI_HANDLE                     DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL       *DevicePath;

  //
  //  Local instance data needed by SNP driver
  //
  //  Pointer to S/W UNDI API entry point
  //  This will be NULL for H/W UNDI
  //
  ISSUE_UNDI32_COMMAND           IssueUndi32Command;

  BOOLEAN                        IsSwUndi;

  //
  // undi interface number, if one undi manages more nics
  //
  PXE_IFNUM                      IfNum;

  //
  //  Allocated tx/rx buffer that was passed to UNDI Initialize.
  //
  UINT32                         TxRxBufferSize;
  VOID                           *TxRxBuffer;
  //
  // mappable buffers for receive and fill header for undi3.0
  // these will be used if the user buffers are above 4GB limit (instead of
  // mapping the user buffers)
  //
  UINT8                          *ReceiveBufffer;
  VOID                           *ReceiveBufferUnmap;
  UINT8                          *FillHeaderBuffer;
  VOID                           *FillHeaderBufferUnmap;

  EFI_PCI_IO_PROTOCOL            *PciIo;
  UINT8                          IoBarIndex;
  UINT8                          MemoryBarIndex;

  //
  // Buffers for command descriptor block, command parameter block
  // and data block.
  //
  PXE_CDB                        Cdb;
  VOID                           *Cpb;
  VOID                           *CpbUnmap;
  VOID                           *Db;

  //
  // UNDI structure, we need to remember the init info for a long time!
  //
  PXE_DB_GET_INIT_INFO           InitInfo;

  VOID                           *SnpDriverUnmap;
  //
  // when ever we map an address, we must remember it's address and the un-map
  // cookie so that we can unmap later
  //
  struct MAP_LIST {
    EFI_PHYSICAL_ADDRESS    VirtualAddress;
    VOID                    *MapCookie;
  } MapList[MAX_MAP_LENGTH];

  EFI_EVENT    ExitBootServicesEvent;

  //
  // Whether UNDI support reporting media status from GET_STATUS command,
  // i.e. PXE_STATFLAGS_GET_STATUS_NO_MEDIA_SUPPORTED or
  //      PXE_STATFLAGS_GET_STATUS_NO_MEDIA_NOT_SUPPORTED
  //
  BOOLEAN      MediaStatusSupported;

  //
  // Whether UNDI support cable detect for INITIALIZE command,
  // i.e. PXE_STATFLAGS_CABLE_DETECT_SUPPORTED or
  //      PXE_STATFLAGS_CABLE_DETECT_NOT_SUPPORTED
  //
  BOOLEAN      CableDetectSupported;

  //
  // Array of the recycled transmit buffer address from UNDI.
  //
  UINT64       *RecycledTxBuf;
  //
  // The maximum number of recycled buffer pointers in RecycledTxBuf.
  //
  UINT32       MaxRecycledTxBuf;
  //
  // Current number of recycled buffer pointers in RecycledTxBuf.
  //
  UINT32       RecycledTxBufCount;
} SNP_DRIVER;

#define EFI_SIMPLE_NETWORK_DEV_FROM_THIS(a)  CR (a, SNP_DRIVER, Snp, SNP_DRIVER_SIGNATURE)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gSimpleNetworkDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gSimpleNetworkComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gSimpleNetworkComponentName2;

/**
  this routine calls undi to start the interface and changes the snp state.

  @param  Snp                    pointer to snp driver structure

  @retval EFI_DEVICE_ERROR       UNDI could not be started
  @retval EFI_SUCCESS            UNDI is started successfully

**/
EFI_STATUS
PxeStart (
  IN SNP_DRIVER  *Snp
  );

/**
  this routine calls undi to stop the interface and changes the snp state.

  @param  Snp   pointer to snp driver structure

  @retval EFI_INVALID_PARAMETER  invalid parameter
  @retval EFI_NOT_STARTED        SNP is not started
  @retval EFI_DEVICE_ERROR       SNP is not initialized
  @retval EFI_UNSUPPORTED        operation unsupported

**/
EFI_STATUS
PxeStop (
  SNP_DRIVER  *Snp
  );

/**
  this routine calls undi to initialize the interface.

  @param  Snp                   pointer to snp driver structure
  @param  CableDetectFlag       Do/don't detect the cable (depending on what undi supports)

  @retval EFI_SUCCESS           UNDI is initialized successfully
  @retval EFI_DEVICE_ERROR      UNDI could not be initialized
  @retval Other                 other errors

**/
EFI_STATUS
PxeInit (
  SNP_DRIVER  *Snp,
  UINT16      CableDetectFlag
  );

/**
  this routine calls undi to shut down the interface.

  @param  Snp   pointer to snp driver structure

  @retval EFI_SUCCESS        UNDI is shut down successfully
  @retval EFI_DEVICE_ERROR   UNDI could not be shut down

**/
EFI_STATUS
PxeShutdown (
  IN SNP_DRIVER  *Snp
  );

/**
  this routine calls undi to read the MAC address of the NIC and updates the
  mode structure with the address.

  @param  Snp         pointer to snp driver structure.

  @retval EFI_SUCCESS       the MAC address of the NIC is read successfully.
  @retval EFI_DEVICE_ERROR  failed to read the MAC address of the NIC.

**/
EFI_STATUS
PxeGetStnAddr (
  SNP_DRIVER  *Snp
  );

/**
  Call undi to get the status of the interrupts, get the list of recycled transmit
  buffers that completed transmitting. The recycled transmit buffer address will
  be saved into Snp->RecycledTxBuf. This function will also update the MediaPresent
  field of EFI_SIMPLE_NETWORK_MODE if UNDI support it.

  @param[in]   Snp                     Pointer to snp driver structure.
  @param[out]  InterruptStatusPtr      A non null pointer to contain the interrupt
                                       status.
  @param[in]   GetTransmittedBuf       Set to TRUE to retrieve the recycled transmit
                                       buffer address.

  @retval      EFI_SUCCESS             The status of the network interface was retrieved.
  @retval      EFI_DEVICE_ERROR        The command could not be sent to the network
                                       interface.

**/
EFI_STATUS
PxeGetStatus (
  IN     SNP_DRIVER  *Snp,
  OUT UINT32         *InterruptStatusPtr,
  IN     BOOLEAN     GetTransmittedBuf
  );

/**
  This is a callback routine supplied to UNDI3.1 at undi_start time.
  UNDI call this routine when it wants to have exclusive access to a critical
  section of the code/data.
  New callbacks for 3.1:
  there won't be a virtual2physical callback for UNDI 3.1 because undi3.1 uses
  the MemMap call to map the required address by itself!

  @param UniqueId  This was supplied to UNDI at Undi_Start, SNP uses this to
                   store Undi interface context (Undi does not read or write
                   this variable)
  @param Enable    non-zero indicates acquire
                   zero indicates release
**/
VOID
EFIAPI
SnpUndi32CallbackBlock (
  IN UINT64  UniqueId,
  IN UINT32  Enable
  );

/**
  This is a callback routine supplied to UNDI at undi_start time.
  UNDI call this routine with the number of micro seconds when it wants to
  pause.

  @param UniqueId      This was supplied to UNDI at Undi_Start, SNP uses this to
                       store Undi interface context (Undi does not read or write
                       this variable)
  @param MicroSeconds  number of micro seconds to pause, usually multiple of 10.
**/
VOID
EFIAPI
SnpUndi32CallbackDelay (
  IN UINT64  UniqueId,
  IN UINT64  MicroSeconds
  );

/**
  This is a callback routine supplied to UNDI at undi_start time.
  This is the IO routine for UNDI3.1 to start CPB.

  @param UniqueId       This was supplied to UNDI at Undi_Start, SNP uses this
                        to store Undi interface context (Undi does not read or
                        write this variable)
  @param ReadOrWrite    indicates read or write, IO or Memory.
  @param NumBytes       number of bytes to read or write.
  @param MemOrPortAddr  IO or memory address to read from or write to.
  @param BufferPtr      memory location to read into or that contains the bytes
                        to write.
**/
VOID
EFIAPI
SnpUndi32CallbackMemio (
  IN UINT64      UniqueId,
  IN UINT8       ReadOrWrite,
  IN UINT8       NumBytes,
  IN UINT64      MemOrPortAddr,
  IN OUT UINT64  BufferPtr
  );

/**
  This is a callback routine supplied to UNDI at undi_start time.
  UNDI call this routine when it has to map a CPU address to a device
  address.

  @param UniqueId      - This was supplied to UNDI at Undi_Start, SNP uses this to store
                         Undi interface context (Undi does not read or write this variable)
  @param CpuAddr       - Virtual address to be mapped!
  @param NumBytes      - size of memory to be mapped
  @param Direction     - direction of data flow for this memory's usage:
                         cpu->device, device->cpu or both ways
  @param DeviceAddrPtr - pointer to return the mapped device address

**/
VOID
EFIAPI
SnpUndi32CallbackMap (
  IN UINT64      UniqueId,
  IN UINT64      CpuAddr,
  IN UINT32      NumBytes,
  IN UINT32      Direction,
  IN OUT UINT64  DeviceAddrPtr
  );

/**
  This is a callback routine supplied to UNDI at undi_start time.
  UNDI call this routine when it wants to unmap an address that was previously
  mapped using map callback.

  @param UniqueId    This was supplied to UNDI at Undi_Start, SNP uses this to store.
                     Undi interface context (Undi does not read or write this variable)
  @param CpuAddr     Virtual address that was mapped!
  @param NumBytes    size of memory mapped
  @param Direction   direction of data flow for this memory's usage:
                     cpu->device, device->cpu or both ways
  @param DeviceAddr  the mapped device address

**/
VOID
EFIAPI
SnpUndi32CallbackUnmap (
  IN UINT64  UniqueId,
  IN UINT64  CpuAddr,
  IN UINT32  NumBytes,
  IN UINT32  Direction,
  IN UINT64  DeviceAddr
  );

/**
  This is a callback routine supplied to UNDI at undi_start time.
  UNDI call this routine when it wants synchronize the virtual buffer contents
  with the mapped buffer contents. The virtual and mapped buffers need not
  correspond to the same physical memory (especially if the virtual address is
  > 4GB). Depending on the direction for which the buffer is mapped, undi will
  need to synchronize their contents whenever it writes to/reads from the buffer
  using either the cpu address or the device address.

  EFI does not provide a sync call, since virt=physical, we should just do
  the synchronization ourself here!

  @param UniqueId    This was supplied to UNDI at Undi_Start, SNP uses this to store
                     Undi interface context (Undi does not read or write this variable)
  @param CpuAddr     Virtual address that was mapped!
  @param NumBytes    size of memory mapped.
  @param Direction   direction of data flow for this memory's usage:
                     cpu->device, device->cpu or both ways.
  @param DeviceAddr  the mapped device address.

**/
VOID
EFIAPI
SnpUndi32CallbackSync (
  IN UINT64  UniqueId,
  IN UINT64  CpuAddr,
  IN UINT32  NumBytes,
  IN UINT32  Direction,
  IN UINT64  DeviceAddr
  );

/**
  Changes the state of a network interface from "stopped" to "started".

  This function starts a network interface. If the network interface successfully
  starts, then EFI_SUCCESS will be returned.

  @param  This                   A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.

  @retval EFI_SUCCESS            The network interface was started.
  @retval EFI_ALREADY_STARTED    The network interface is already in the started state.
  @retval EFI_INVALID_PARAMETER  This parameter was NULL or did not point to a valid
                                 EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR       The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED        This function is not supported by the network interface.

**/
EFI_STATUS
EFIAPI
SnpUndi32Start (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This
  );

/**
  Changes the state of a network interface from "started" to "stopped".

  This function stops a network interface. This call is only valid if the network
  interface is in the started state. If the network interface was successfully
  stopped, then EFI_SUCCESS will be returned.

  @param  This                    A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.


  @retval EFI_SUCCESS             The network interface was stopped.
  @retval EFI_NOT_STARTED         The network interface has not been started.
  @retval EFI_INVALID_PARAMETER   This parameter was NULL or did not point to a valid
                                  EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR        The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED         This function is not supported by the network interface.

**/
EFI_STATUS
EFIAPI
SnpUndi32Stop (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This
  );

/**
  Resets a network adapter and allocates the transmit and receive buffers
  required by the network interface; optionally, also requests allocation of
  additional transmit and receive buffers.

  This function allocates the transmit and receive buffers required by the network
  interface. If this allocation fails, then EFI_OUT_OF_RESOURCES is returned.
  If the allocation succeeds and the network interface is successfully initialized,
  then EFI_SUCCESS will be returned.

  @param This               A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.

  @param ExtraRxBufferSize  The size, in bytes, of the extra receive buffer space
                            that the driver should allocate for the network interface.
                            Some network interfaces will not be able to use the
                            extra buffer, and the caller will not know if it is
                            actually being used.
  @param ExtraTxBufferSize  The size, in bytes, of the extra transmit buffer space
                            that the driver should allocate for the network interface.
                            Some network interfaces will not be able to use the
                            extra buffer, and the caller will not know if it is
                            actually being used.

  @retval EFI_SUCCESS           The network interface was initialized.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_OUT_OF_RESOURCES  There was not enough memory for the transmit and
                                receive buffers.
  @retval EFI_INVALID_PARAMETER This parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       The increased buffer size feature is not supported.

**/
EFI_STATUS
EFIAPI
SnpUndi32Initialize (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN UINTN                        ExtraRxBufferSize OPTIONAL,
  IN UINTN                        ExtraTxBufferSize OPTIONAL
  );

/**
  Resets a network adapter and reinitializes it with the parameters that were
  provided in the previous call to Initialize().

  This function resets a network adapter and reinitializes it with the parameters
  that were provided in the previous call to Initialize(). The transmit and
  receive queues are emptied and all pending interrupts are cleared.
  Receive filters, the station address, the statistics, and the multicast-IP-to-HW
  MAC addresses are not reset by this call. If the network interface was
  successfully reset, then EFI_SUCCESS will be returned. If the driver has not
  been initialized, EFI_DEVICE_ERROR will be returned.

  @param This                 A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.
  @param ExtendedVerification Indicates that the driver may perform a more
                              exhaustive verification operation of the device
                              during reset.

  @retval EFI_SUCCESS           The network interface was reset.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EFIAPI
SnpUndi32Reset (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                      ExtendedVerification
  );

/**
  Resets a network adapter and leaves it in a state that is safe for another
  driver to initialize.

  This function releases the memory buffers assigned in the Initialize() call.
  Pending transmits and receives are lost, and interrupts are cleared and disabled.
  After this call, only the Initialize() and Stop() calls may be used. If the
  network interface was successfully shutdown, then EFI_SUCCESS will be returned.
  If the driver has not been initialized, EFI_DEVICE_ERROR will be returned.

  @param  This  A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.

  @retval EFI_SUCCESS           The network interface was shutdown.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER This parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.

**/
EFI_STATUS
EFIAPI
SnpUndi32Shutdown (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This
  );

/**
  Manages the multicast receive filters of a network interface.

  This function is used enable and disable the hardware and software receive
  filters for the underlying network device.
  The receive filter change is broken down into three steps:
  * The filter mask bits that are set (ON) in the Enable parameter are added to
    the current receive filter settings.
  * The filter mask bits that are set (ON) in the Disable parameter are subtracted
    from the updated receive filter settings.
  * If the resulting receive filter setting is not supported by the hardware a
    more liberal setting is selected.
  If the same bits are set in the Enable and Disable parameters, then the bits
  in the Disable parameter takes precedence.
  If the ResetMCastFilter parameter is TRUE, then the multicast address list
  filter is disabled (irregardless of what other multicast bits are set in the
  Enable and Disable parameters). The SNP->Mode->MCastFilterCount field is set
  to zero. The Snp->Mode->MCastFilter contents are undefined.
  After enabling or disabling receive filter settings, software should verify
  the new settings by checking the Snp->Mode->ReceiveFilterSettings,
  Snp->Mode->MCastFilterCount and Snp->Mode->MCastFilter fields.
  Note: Some network drivers and/or devices will automatically promote receive
    filter settings if the requested setting can not be honored. For example, if
    a request for four multicast addresses is made and the underlying hardware
    only supports two multicast addresses the driver might set the promiscuous
    or promiscuous multicast receive filters instead. The receiving software is
    responsible for discarding any extra packets that get through the hardware
    receive filters.
    Note: Note: To disable all receive filter hardware, the network driver must
      be Shutdown() and Stopped(). Calling ReceiveFilters() with Disable set to
      Snp->Mode->ReceiveFilterSettings will make it so no more packets are
      returned by the Receive() function, but the receive hardware may still be
      moving packets into system memory before inspecting and discarding them.
      Unexpected system errors, reboots and hangs can occur if an OS is loaded
      and the network devices are not Shutdown() and Stopped().
  If ResetMCastFilter is TRUE, then the multicast receive filter list on the
  network interface will be reset to the default multicast receive filter list.
  If ResetMCastFilter is FALSE, and this network interface allows the multicast
  receive filter list to be modified, then the MCastFilterCnt and MCastFilter
  are used to update the current multicast receive filter list. The modified
  receive filter list settings can be found in the MCastFilter field of
  EFI_SIMPLE_NETWORK_MODE. If the network interface does not allow the multicast
  receive filter list to be modified, then EFI_INVALID_PARAMETER will be returned.
  If the driver has not been initialized, EFI_DEVICE_ERROR will be returned.
  If the receive filter mask and multicast receive filter list have been
  successfully updated on the network interface, EFI_SUCCESS will be returned.

  @param This             A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.
  @param Enable           A bit mask of receive filters to enable on the network
                          interface.
  @param Disable          A bit mask of receive filters to disable on the network
                          interface. For backward compatibility with EFI 1.1
                          platforms, the EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST bit
                          must be set when the ResetMCastFilter parameter is TRUE.
  @param ResetMCastFilter Set to TRUE to reset the contents of the multicast
                          receive filters on the network interface to their
                          default values.
  @param MCastFilterCnt   Number of multicast HW MAC addresses in the new MCastFilter
                          list. This value must be less than or equal to the
                          MCastFilterCnt field of EFI_SIMPLE_NETWORK_MODE.
                          This field is optional if ResetMCastFilter is TRUE.
  @param MCastFilter      A pointer to a list of new multicast receive filter HW
                          MAC addresses. This list will replace any existing
                          multicast HW MAC address list. This field is optional
                          if ResetMCastFilter is TRUE.

  @retval EFI_SUCCESS            The multicast receive filter list was updated.
  @retval EFI_NOT_STARTED        The network interface has not been started.
  @retval EFI_INVALID_PARAMETER  One or more of the following conditions is TRUE:
                                 * This is NULL
                                 * There are bits set in Enable that are not set
                                   in Snp->Mode->ReceiveFilterMask
                                 * There are bits set in Disable that are not set
                                   in Snp->Mode->ReceiveFilterMask
                                 * Multicast is being enabled (the
                                   EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST bit is
                                   set in Enable, it is not set in Disable, and
                                   ResetMCastFilter is FALSE) and MCastFilterCount
                                   is zero
                                 * Multicast is being enabled and MCastFilterCount
                                   is greater than Snp->Mode->MaxMCastFilterCount
                                 * Multicast is being enabled and MCastFilter is NULL
                                 * Multicast is being enabled and one or more of
                                   the addresses in the MCastFilter list are not
                                   valid multicast MAC addresses
  @retval EFI_DEVICE_ERROR       One or more of the following conditions is TRUE:
                                 * The network interface has been started but has
                                   not been initialized
                                 * An unexpected error was returned by the
                                   underlying network driver or device
  @retval EFI_UNSUPPORTED        This function is not supported by the network
                                 interface.

**/
EFI_STATUS
EFIAPI
SnpUndi32ReceiveFilters (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN UINT32                       Enable,
  IN UINT32                       Disable,
  IN BOOLEAN                      ResetMCastFilter,
  IN UINTN                        MCastFilterCnt   OPTIONAL,
  IN EFI_MAC_ADDRESS              *MCastFilter     OPTIONAL
  );

/**
  Modifies or resets the current station address, if supported.

  This function modifies or resets the current station address of a network
  interface, if supported. If Reset is TRUE, then the current station address is
  set to the network interface's permanent address. If Reset is FALSE, and the
  network interface allows its station address to be modified, then the current
  station address is changed to the address specified by New. If the network
  interface does not allow its station address to be modified, then
  EFI_INVALID_PARAMETER will be returned. If the station address is successfully
  updated on the network interface, EFI_SUCCESS will be returned. If the driver
  has not been initialized, EFI_DEVICE_ERROR will be returned.

  @param This  A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.
  @param Reset Flag used to reset the station address to the network interface's
               permanent address.
  @param New   New station address to be used for the network interface.


  @retval EFI_SUCCESS           The network interface's station address was updated.
  @retval EFI_NOT_STARTED       The Simple Network Protocol interface has not been
                                started by calling Start().
  @retval EFI_INVALID_PARAMETER The New station address was not accepted by the NIC.
  @retval EFI_INVALID_PARAMETER Reset is FALSE and New is NULL.
  @retval EFI_DEVICE_ERROR      The Simple Network Protocol interface has not
                                been initialized by calling Initialize().
  @retval EFI_DEVICE_ERROR      An error occurred attempting to set the new
                                station address.
  @retval EFI_UNSUPPORTED       The NIC does not support changing the network
                                interface's station address.

**/
EFI_STATUS
EFIAPI
SnpUndi32StationAddress (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                      Reset,
  IN EFI_MAC_ADDRESS              *New  OPTIONAL
  );

/**
  Resets or collects the statistics on a network interface.

  This function resets or collects the statistics on a network interface. If the
  size of the statistics table specified by StatisticsSize is not big enough for
  all the statistics that are collected by the network interface, then a partial
  buffer of statistics is returned in StatisticsTable, StatisticsSize is set to
  the size required to collect all the available statistics, and
  EFI_BUFFER_TOO_SMALL is returned.
  If StatisticsSize is big enough for all the statistics, then StatisticsTable
  will be filled, StatisticsSize will be set to the size of the returned
  StatisticsTable structure, and EFI_SUCCESS is returned.
  If the driver has not been initialized, EFI_DEVICE_ERROR will be returned.
  If Reset is FALSE, and both StatisticsSize and StatisticsTable are NULL, then
  no operations will be performed, and EFI_SUCCESS will be returned.
  If Reset is TRUE, then all of the supported statistics counters on this network
  interface will be reset to zero.

  @param This            A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.
  @param Reset           Set to TRUE to reset the statistics for the network interface.
  @param StatisticsSize  On input the size, in bytes, of StatisticsTable. On output
                         the size, in bytes, of the resulting table of statistics.
  @param StatisticsTable A pointer to the EFI_NETWORK_STATISTICS structure that
                         contains the statistics. Type EFI_NETWORK_STATISTICS is
                         defined in "Related Definitions" below.

  @retval EFI_SUCCESS           The requested operation succeeded.
  @retval EFI_NOT_STARTED       The Simple Network Protocol interface has not been
                                started by calling Start().
  @retval EFI_BUFFER_TOO_SMALL  StatisticsSize is not NULL and StatisticsTable is
                                NULL. The current buffer size that is needed to
                                hold all the statistics is returned in StatisticsSize.
  @retval EFI_BUFFER_TOO_SMALL  StatisticsSize is not NULL and StatisticsTable is
                                not NULL. The current buffer size that is needed
                                to hold all the statistics is returned in
                                StatisticsSize. A partial set of statistics is
                                returned in StatisticsTable.
  @retval EFI_INVALID_PARAMETER StatisticsSize is NULL and StatisticsTable is not
                                NULL.
  @retval EFI_DEVICE_ERROR      The Simple Network Protocol interface has not
                                been initialized by calling Initialize().
  @retval EFI_DEVICE_ERROR      An error was encountered collecting statistics
                                from the NIC.
  @retval EFI_UNSUPPORTED       The NIC does not support collecting statistics
                                from the network interface.

**/
EFI_STATUS
EFIAPI
SnpUndi32Statistics (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                      Reset,
  IN OUT UINTN                    *StatisticsSize   OPTIONAL,
  IN OUT EFI_NETWORK_STATISTICS   *StatisticsTable  OPTIONAL
  );

/**
  Converts a multicast IP address to a multicast HW MAC address.

  This function converts a multicast IP address to a multicast HW MAC address
  for all packet transactions. If the mapping is accepted, then EFI_SUCCESS will
  be returned.

  @param This A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.
  @param IPv6 Set to TRUE if the multicast IP address is IPv6 [RFC 2460].
              Set to FALSE if the multicast IP address is IPv4 [RFC 791].
  @param IP   The multicast IP address that is to be converted to a multicast
              HW MAC address.
  @param MAC  The multicast HW MAC address that is to be generated from IP.

  @retval EFI_SUCCESS           The multicast IP address was mapped to the
                                multicast HW MAC address.
  @retval EFI_NOT_STARTED       The Simple Network Protocol interface has not
                                been started by calling Start().
  @retval EFI_INVALID_PARAMETER IP is NULL.
  @retval EFI_INVALID_PARAMETER MAC is NULL.
  @retval EFI_INVALID_PARAMETER IP does not point to a valid IPv4 or IPv6
                                multicast address.
  @retval EFI_DEVICE_ERROR      The Simple Network Protocol interface has not
                                been initialized by calling Initialize().
  @retval EFI_UNSUPPORTED       IPv6 is TRUE and the implementation does not
                                support IPv6 multicast to MAC address conversion.

**/
EFI_STATUS
EFIAPI
SnpUndi32McastIpToMac (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                      IPv6,
  IN EFI_IP_ADDRESS               *IP,
  OUT EFI_MAC_ADDRESS             *MAC
  );

/**
  Performs read and write operations on the NVRAM device attached to a network
  interface.

  This function performs read and write operations on the NVRAM device attached
  to a network interface. If ReadWrite is TRUE, a read operation is performed.
  If ReadWrite is FALSE, a write operation is performed. Offset specifies the
  byte offset at which to start either operation. Offset must be a multiple of
  NvRamAccessSize , and it must have a value between zero and NvRamSize.
  BufferSize specifies the length of the read or write operation. BufferSize must
  also be a multiple of NvRamAccessSize, and Offset + BufferSize must not exceed
  NvRamSize.
  If any of the above conditions is not met, then EFI_INVALID_PARAMETER will be
  returned.
  If all the conditions are met and the operation is "read," the NVRAM device
  attached to the network interface will be read into Buffer and EFI_SUCCESS
  will be returned. If this is a write operation, the contents of Buffer will be
  used to update the contents of the NVRAM device attached to the network
  interface and EFI_SUCCESS will be returned.

  It does the basic checking on the input parameters and retrieves snp structure
  and then calls the read_nvdata() call which does the actual reading

  @param This       A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.
  @param ReadWrite  TRUE for read operations, FALSE for write operations.
  @param Offset     Byte offset in the NVRAM device at which to start the read or
                    write operation. This must be a multiple of NvRamAccessSize
                    and less than NvRamSize. (See EFI_SIMPLE_NETWORK_MODE)
  @param BufferSize The number of bytes to read or write from the NVRAM device.
                    This must also be a multiple of NvramAccessSize.
  @param Buffer     A pointer to the data buffer.

  @retval EFI_SUCCESS           The NVRAM access was performed.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                * The This parameter is NULL
                                * The This parameter does not point to a valid
                                  EFI_SIMPLE_NETWORK_PROTOCOL  structure
                                * The Offset parameter is not a multiple of
                                  EFI_SIMPLE_NETWORK_MODE.NvRamAccessSize
                                * The Offset parameter is not less than
                                  EFI_SIMPLE_NETWORK_MODE.NvRamSize
                                * The BufferSize parameter is not a multiple of
                                  EFI_SIMPLE_NETWORK_MODE.NvRamAccessSize
                                * The Buffer parameter is NULL
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network
                                interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network
                                interface.

**/
EFI_STATUS
EFIAPI
SnpUndi32NvData (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN BOOLEAN                      ReadWrite,
  IN UINTN                        Offset,
  IN UINTN                        BufferSize,
  IN OUT VOID                     *Buffer
  );

/**
  Reads the current interrupt status and recycled transmit buffer status from a
  network interface.

  This function gets the current interrupt and recycled transmit buffer status
  from the network interface. The interrupt status is returned as a bit mask in
  InterruptStatus. If InterruptStatus is NULL, the interrupt status will not be
  read. If TxBuf is not NULL, a recycled transmit buffer address will be retrieved.
  If a recycled transmit buffer address is returned in TxBuf, then the buffer has
  been successfully transmitted, and the status for that buffer is cleared. If
  the status of the network interface is successfully collected, EFI_SUCCESS
  will be returned. If the driver has not been initialized, EFI_DEVICE_ERROR will
  be returned.

  @param This            A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.
  @param InterruptStatus A pointer to the bit mask of the currently active
                         interrupts (see "Related Definitions"). If this is NULL,
                         the interrupt status will not be read from the device.
                         If this is not NULL, the interrupt status will be read
                         from the device. When the interrupt status is read, it
                         will also be cleared. Clearing the transmit interrupt does
                         not empty the recycled transmit buffer array.
  @param TxBuf           Recycled transmit buffer address. The network interface
                         will not transmit if its internal recycled transmit
                         buffer array is full. Reading the transmit buffer does
                         not clear the transmit interrupt. If this is NULL, then
                         the transmit buffer status will not be read. If there
                         are no transmit buffers to recycle and TxBuf is not NULL,
                         TxBuf will be set to NULL.

  @retval EFI_SUCCESS           The status of the network interface was retrieved.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_INVALID_PARAMETER This parameter was NULL or did not point to a valid
                                EFI_SIMPLE_NETWORK_PROTOCOL structure.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network
                                interface.

**/
EFI_STATUS
EFIAPI
SnpUndi32GetStatus (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  OUT UINT32                      *InterruptStatus  OPTIONAL,
  OUT VOID                        **TxBuf           OPTIONAL
  );

/**
  Places a packet in the transmit queue of a network interface.

  This function places the packet specified by Header and Buffer on the transmit
  queue. If HeaderSize is nonzero and HeaderSize is not equal to
  This->Mode->MediaHeaderSize, then EFI_INVALID_PARAMETER will be returned. If
  BufferSize is less than This->Mode->MediaHeaderSize, then EFI_BUFFER_TOO_SMALL
  will be returned. If Buffer is NULL, then EFI_INVALID_PARAMETER will be
  returned. If HeaderSize is nonzero and DestAddr or Protocol is NULL, then
  EFI_INVALID_PARAMETER will be returned. If the transmit engine of the network
  interface is busy, then EFI_NOT_READY will be returned. If this packet can be
  accepted by the transmit engine of the network interface, the packet contents
  specified by Buffer will be placed on the transmit queue of the network
  interface, and EFI_SUCCESS will be returned. GetStatus() can be used to
  determine when the packet has actually been transmitted. The contents of the
  Buffer must not be modified until the packet has actually been transmitted.
  The Transmit() function performs nonblocking I/O. A caller who wants to perform
  blocking I/O, should call Transmit(), and then GetStatus() until the
  transmitted buffer shows up in the recycled transmit buffer.
  If the driver has not been initialized, EFI_DEVICE_ERROR will be returned.

  @param This       A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.
  @param HeaderSize The size, in bytes, of the media header to be filled in by the
                    Transmit() function. If HeaderSize is nonzero, then it must
                    be equal to This->Mode->MediaHeaderSize and the DestAddr and
                    Protocol parameters must not be NULL.
  @param BufferSize The size, in bytes, of the entire packet (media header and
                    data) to be transmitted through the network interface.
  @param Buffer     A pointer to the packet (media header followed by data) to be
                    transmitted. This parameter cannot be NULL. If HeaderSize is
                    zero, then the media header in Buffer must already be filled
                    in by the caller. If HeaderSize is nonzero, then the media
                    header will be filled in by the Transmit() function.
  @param SrcAddr    The source HW MAC address. If HeaderSize is zero, then this
                    parameter is ignored. If HeaderSize is nonzero and SrcAddr
                    is NULL, then This->Mode->CurrentAddress is used for the
                    source HW MAC address.
  @param DestAddr   The destination HW MAC address. If HeaderSize is zero, then
                    this parameter is ignored.
  @param Protocol   The type of header to build. If HeaderSize is zero, then this
                    parameter is ignored. See RFC 1700, section "Ether Types,"
                    for examples.

  @retval EFI_SUCCESS           The packet was placed on the transmit queue.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_NOT_READY         The network interface is too busy to accept this
                                transmit request.
  @retval EFI_BUFFER_TOO_SMALL  The BufferSize parameter is too small.
  @retval EFI_INVALID_PARAMETER One or more of the parameters has an unsupported
                                value.
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.
  @retval EFI_UNSUPPORTED       This function is not supported by the network interface.

**/
EFI_STATUS
EFIAPI
SnpUndi32Transmit (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  IN UINTN                        HeaderSize,
  IN UINTN                        BufferSize,
  IN VOID                         *Buffer,
  IN EFI_MAC_ADDRESS              *SrcAddr   OPTIONAL,
  IN EFI_MAC_ADDRESS              *DestAddr  OPTIONAL,
  IN UINT16                       *Protocol  OPTIONAL
  );

/**
  Receives a packet from a network interface.

  This function retrieves one packet from the receive queue of a network interface.
  If there are no packets on the receive queue, then EFI_NOT_READY will be
  returned. If there is a packet on the receive queue, and the size of the packet
  is smaller than BufferSize, then the contents of the packet will be placed in
  Buffer, and BufferSize will be updated with the actual size of the packet.
  In addition, if SrcAddr, DestAddr, and Protocol are not NULL, then these values
  will be extracted from the media header and returned. EFI_SUCCESS will be
  returned if a packet was successfully received.
  If BufferSize is smaller than the received packet, then the size of the receive
  packet will be placed in BufferSize and EFI_BUFFER_TOO_SMALL will be returned.
  If the driver has not been initialized, EFI_DEVICE_ERROR will be returned.

  @param This       A pointer to the EFI_SIMPLE_NETWORK_PROTOCOL instance.
  @param HeaderSize The size, in bytes, of the media header received on the network
                    interface. If this parameter is NULL, then the media header size
                    will not be returned.
  @param BufferSize On entry, the size, in bytes, of Buffer. On exit, the size, in
                    bytes, of the packet that was received on the network interface.
  @param Buffer     A pointer to the data buffer to receive both the media
                    header and the data.
  @param SrcAddr    The source HW MAC address. If this parameter is NULL, the HW
                    MAC source address will not be extracted from the media header.
  @param DestAddr   The destination HW MAC address. If this parameter is NULL,
                    the HW MAC destination address will not be extracted from
                    the media header.
  @param Protocol   The media header type. If this parameter is NULL, then the
                    protocol will not be extracted from the media header. See
                    RFC 1700 section "Ether Types" for examples.

  @retval EFI_SUCCESS           The received data was stored in Buffer, and
                                BufferSize has been updated to the number of
                                bytes received.
  @retval EFI_NOT_STARTED       The network interface has not been started.
  @retval EFI_NOT_READY         No packets have been received on the network interface.
  @retval EFI_BUFFER_TOO_SMALL  BufferSize is too small for the received packets.
                                BufferSize has been updated to the required size.
  @retval EFI_INVALID_PARAMETER One or more of the following conditions is TRUE:
                                * The This parameter is NULL
                                * The This parameter does not point to a valid
                                  EFI_SIMPLE_NETWORK_PROTOCOL structure.
                                * The BufferSize parameter is NULL
                                * The Buffer parameter is NULL
  @retval EFI_DEVICE_ERROR      The command could not be sent to the network interface.

**/
EFI_STATUS
EFIAPI
SnpUndi32Receive (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *This,
  OUT UINTN                       *HeaderSize OPTIONAL,
  IN OUT UINTN                    *BufferSize,
  OUT VOID                        *Buffer,
  OUT EFI_MAC_ADDRESS             *SrcAddr OPTIONAL,
  OUT EFI_MAC_ADDRESS             *DestAddr OPTIONAL,
  OUT UINT16                      *Protocol OPTIONAL
  );

/**
  Notification call back function for WaitForPacket event.

  @param  Event       EFI Event.
  @param  SnpPtr      Pointer to SNP_DRIVER structure.

**/
VOID
EFIAPI
SnpWaitForPacketNotify (
  EFI_EVENT  Event,
  VOID       *SnpPtr
  );

#define SNP_MEM_PAGES(x)  (((x) - 1) / 4096 + 1)

#endif /*  _SNP_H_  */
