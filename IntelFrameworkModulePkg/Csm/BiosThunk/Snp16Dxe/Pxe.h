/** @file
  These are PXE Specification 2.1-compliant data structures and defines.

  This file relies upon the existence of a PXE-compliant ROM
  in memory, as defined by the Preboot Execution Environment 
  Specification (PXE), Version 2.1, located at

  http://developer.intel.com/ial/wfm/wfmspecs.htm

Copyright (c) 1999 - 2010, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PXEDEF_H_
#define _PXEDEF_H_

#pragma pack(1)

//
//    PXE structure signatures
//
#define BC_ROMID_SIG        "$BC$"
#define UNDI_ROMID_SIG      "UNDI"
#define BUSD_ROMID_SIG      "BUSD"

#define PXE_SIG             "!PXE"
#define PXENV_SIG           "PXENV+"

#define BC_ROMID_REV        0x00
#define UNDI_ROMID_REV      0x00
#define BUSD_ROMID_REV      0x00

#define PXE_REV             0x00
#define PXENV_REV           0x0201

#define PXENV_PTR           SIGNATURE_32 ('P', 'X', 'E', 'N')
#define PXE_PTR             SIGNATURE_32 ('!', 'P', 'X', 'E')
#define UNDI_ROMID_SIG_PTR  SIGNATURE_32 ('U', 'N', 'D', 'I')

typedef UINT16  SEGSEL; // Real mode segment or protected mode selector.
typedef UINT16  OFF16;  // Unsigned 16bit offset.
typedef UINT32  ADDR32;

//
//    Bus types
//
#define PXENV_BUS_ISA     0
#define PXENV_BUS_EISA    1
#define PXENV_BUS_MCA     2
#define PXENV_BUS_PCI     3
#define PXENV_BUS_VESA    4
#define PXENV_BUS_PCMCIA  5

//
//
//    Result codes returned in AX by a PXENV API service.
//
#define PXENV_EXIT_SUCCESS  0x0000
#define PXENV_EXIT_FAILURE  0x0001

//
//    Status codes returned in the status word of PXENV API parameter structures.
// 
//    Generic API errors - these do not match up with the M0x or E0x messages
//    that are reported by the loader.
//
#define PXENV_STATUS_SUCCESS          0x00
#define PXENV_STATUS_FAILURE          0x01
#define PXENV_STATUS_BAD_FUNC         0x02
#define PXENV_STATUS_UNSUPPORTED      0x03
#define PXENV_STATUS_KEEP_UNDI        0x04
#define PXENV_STATUS_KEEP_ALL         0x05
#define PXENV_STATUS_OUT_OF_RESOURCES 0x06

typedef enum {
  PxeEnvStatus_Success,
  PxeEnvStatus_Failure,
  PxeEnvStatus_BadFunc,
  PxeEnvStatus_Unsupported,
  PxeEnvStatus_KeepUndi,
  PxeEnvStatus_KeepAll
} EFI_PXE_STATUS;

/* Driver errors (0x60 to 0x6F) */

// These errors are for UNDI compatible NIC drivers. 
#define PXENV_STATUS_UNDI_INVALID_FUNCTION          0x60
#define PXENV_STATUS_UNDI_MEDIATEST_FAILED          0x61
#define PXENV_STATUS_UNDI_CANNOT_INIT_NIC_FOR_MCAST 0x62
#define PXENV_STATUS_UNDI_CANNOT_INITIALIZE_NIC     0x63
#define PXENV_STATUS_UNDI_CANNOT_INITIALIZE_PHY     0x64
#define PXENV_STATUS_UNDI_CANNOT_READ_CONFIG_DATA   0x65
#define PXENV_STATUS_UNDI_CANNOT_READ_INIT_DATA     0x66
#define PXENV_STATUS_UNDI_BAD_MAC_ADDR              0x67
#define PXENV_STATUS_UNDI_BAD_EEPROM_CKSUM          0x68
#define PXENV_STATUS_UNDI_ERROR_SETTING_ISR         0x69
#define PXENV_STATUS_UNDI_INVALID_STATE             0x6A
#define PXENV_STATUS_UNDI_TRANSMIT_ERROR            0x6B
#define PXENV_STATUS_UNDI_INVALID_PARAMETER         0x6C

typedef struct {
  UINT16  Seg_Addr;
  UINT32  Phy_Addr;
  UINT16  Seg_Size;
} NEWSEGDESC_T;

typedef struct {
  OFF16   Offset;
  SEGSEL  Segment;
} SEGOFF16;

typedef struct {
  UINT8   Signature[4]; ///< Structure signature is not NULL terminated.
  UINT8   StructLength; ///< Length of this structure in bytes.
  UINT8   StructCksum;  ///< Use to make byte checksum of this structure == zero.
  UINT8   StructRev;    ///< Structure format revision number.
  UINT8   UNDI_Rev[3];  ///< API revision number stored in Intel order.
  //
  // Revision 2.1.0 == 0x00, 0x01, 0x02
  //
  UINT16  UNDI_Loader;  ///< Offset of UNDI loader routine in the option ROM image.
  UINT16  StackSize;    ///< Minimum stack segment size, in bytes, needed to load and run the UNDI.
  UINT16  DataSize;     ///< UNDI runtime code and data
  UINT16  CodeSize;     ///< segment sizes.
  UINT8   BusType[4];   ///< 'ISAR', 'EISA', 'PCIR', 'PCCR'
} UNDI_ROMID_T;

typedef struct {
  UINT8   Signature[4]; ///< Structure signature is not NULL terminated.
  UINT8   StructLength; ///< Length of this structure in bytes.
  UINT8   StructCksum;  ///< Use to make byte checksum of this structure == zero.
  UINT8   StructRev;    ///< Structure format revision number.
  UINT8   BC_Rev[3];    ///< API revision number stored in Intel order.
  //
  // Revision 2.1.0 == 0x00, 0x01, 0x02
  //
  UINT16  BC_Loader;          ///< Offset of base-code loader routine in the option ROM image.
  UINT16  StackSize;          ///< Minimum stack segment size (bytes) needed to load/run base-code.
  UINT16  DataSize;           ///< Base-code runtime code and data
  UINT16  CodeSize;           ///< segment sizes.
} BC_ROMID_T;

typedef struct {
  UINT8         Signature[4]; ///< Structure signature is not NULL terminated.
  UINT8         StructLength; ///< Length of this structure in bytes.
  UINT8         StructCksum;  ///< Use to make byte checksum of this  structure == zero.
  UINT8         StructRev;    ///< Structure format revision number.
  UINT8         Reserved1;    ///< must be zero
  ///
  ///   UNDI_ROMID_T __FAR *UNDI;// Far pointer to UNDI ROMID
  ///
  SEGOFF16      Undi;

  ///
  ///    BC_ROMID_T __FAR *Base;  //   Far pointer to base-code ROMID
  ///
  SEGOFF16      Base;

  ///
  ///    UINT16 (__FAR __CDECL *EntryPointSP)(UINT16 func, VOID __FAR *param);
  /// 16bit stack segment API entry point.  This will be seg:off in
  /// real mode and sel:off in 16:16 protected mode.
  ///
  SEGOFF16      EntryPointSP;

  ///
  ///    UINT16 (__FAR __CDECL *EntryPointESP)(UINT16 func, VOID __FAR *param);
  /// 32bit stack segment API entry point.  This will be sel:off.
  /// In real mode, sel == 0
  ///
  SEGOFF16      EntryPointESP;
  ///
  ///    UINT16 (__FAR __CDECL *StatusCallout)(UINT16 param);
  /// Address of DHCP/TFTP status callout routine.
  ///
  SEGOFF16      StatusCallout;
  UINT8         Reserved2;      ///< must be zero
  UINT8         SegDescCnt;     ///< Number of segment descriptors in this structure.
  UINT16        FirstSelector;  ///< First segment descriptor in GDT assigned to PXE.
  NEWSEGDESC_T  Stack;
  NEWSEGDESC_T  UNDIData;
  NEWSEGDESC_T  UNDICode;
  NEWSEGDESC_T  UNDICodeWrite;
  NEWSEGDESC_T  BC_Data;
  NEWSEGDESC_T  BC_Code;
  NEWSEGDESC_T  BC_CodeWrite;
} PXE_T;

typedef struct {
  CHAR8       Signature[6];     ///< "PXENV+"
  UINT16      Version;          ///< PXE version number.  LSB is minor version.  MSB is major version.
  UINT8       StructLength;     ///< Length of PXE-2.0 Entry Point structure in bytes.
  UINT8       StructCksum;      ///< Used to make structure checksum equal zero.
  UINT32      RMEntry;          ///< Real mode API entry point  segment:offset.
  UINT32      PMEntryOff;       ///< Protected mode API entry point
  UINT16      PMEntrySeg;       ///< segment:offset.  This will always be zero.  Protected mode API calls
                                ///< must be made through the API entry points in the PXE Runtime ID structure.

  UINT16      StackSeg;     ///< Real mode stack segment.
  UINT16      StackSize;    ///< Stack segment size in bytes.
  UINT16      BaseCodeSeg;  ///< Real mode base-code code segment.
  UINT16      BaseCodeSize; ///< Base-code code segment size
  UINT16      BaseDataSeg;  ///< Real mode base-code data segment.
  UINT16      BaseDataSize; ///< Base-code data segment size
  UINT16      UNDIDataSeg;  ///< Real mode UNDI data segment.
  UINT16      UNDIDataSize; ///< UNDI data segment size in bytes.
  UINT16      UNDICodeSeg;  ///< Real mode UNDI code segment.
  UINT16      UNDICodeSize; ///< UNDI code segment size in bytes.
  PXE_T       *RuntimePtr;  ///< Real mode segment:offset pointer to PXE Runtime ID structure.
} PXENV_T;

typedef struct {
  OUT UINT16    Status;
  IN OUT UINT16 Ax;
  IN OUT UINT16 Bx;
  IN OUT UINT16 Dx;
  IN OUT UINT16 Di;
  IN OUT UINT16 Es;
  IN OUT UINT16 Undi_Ds;
  IN OUT UINT16 Undi_Cs;
  OUT SEGOFF16  PXEptr;
  OUT SEGOFF16  PXENVptr;
} UNDI_LOADER_T;

//
//  Put in some UNDI-specific arguments
//
#define PXENV_START_UNDI              0x0000
#define PXENV_UNDI_STARTUP            0x0001
#define PXENV_UNDI_CLEANUP            0x0002
#define PXENV_UNDI_INITIALIZE         0x0003
#define PXENV_UNDI_RESET_NIC          0x0004
#define PXENV_UNDI_SHUTDOWN           0x0005
#define PXENV_UNDI_OPEN               0x0006
#define PXENV_UNDI_CLOSE              0x0007
#define PXENV_UNDI_TRANSMIT           0x0008
#define PXENV_UNDI_SET_MCAST_ADDR     0x0009
#define PXENV_UNDI_SET_STATION_ADDR   0x000A
#define PXENV_UNDI_SET_PACKET_FILTER  0x000B
#define PXENV_UNDI_GET_INFORMATION    0x000C
#define PXENV_UNDI_GET_STATISTICS     0x000D
#define PXENV_UNDI_CLEAR_STATISTICS   0x000E
#define PXENV_UNDI_INITIATE_DIAGS     0x000F
#define PXENV_UNDI_FORCE_INTERRUPT    0x0010
#define PXENV_UNDI_GET_MCAST_ADDR     0x0011
#define PXENV_UNDI_GET_NIC_TYPE       0x0012
#define PXENV_UNDI_GET_NDIS_INFO      0x0013
#define PXENV_UNDI_ISR                0x0014
#define PXENV_STOP_UNDI               0x0015
#define PXENV_UNDI_GET_STATE          0x0016

#define ADDR_LEN                      16
#define MAXNUM_MCADDR                 8
#define IPLEN                         4       ///< length of an IP address 
#define XMT_DESTADDR                  0x0000  ///< destination address given
#define XMT_BROADCAST                 0x0001  ///< use broadcast address

typedef struct {
  UINT16  MCastAddrCount;                     ///< In: Number of multi-cast

  /* addresses. */
  UINT8   MCastAddr[MAXNUM_MCADDR][ADDR_LEN]; /* In: */

  /* list of multi-cast addresses. */

  /* Each address can take up to */

  /* ADDR_LEN bytes and a maximum */

  /* of MAXNUM_MCADDR address can */

  /* be provided*/
} PXENV_UNDI_MCAST_ADDR_T;

/* Definitions of TFTP API parameter structures.
 */
typedef struct {
  OUT UINT16  Status;       ///< Out: PXENV_STATUS_xxx
  IN UINT16   Ax;           ///< In: These register fields must be
  IN UINT16   Bx;           ///<     filled in with the same data
  IN UINT16   Dx;           ///<     that was passed to the MLID
  IN UINT16   Di;           ///<     option ROM boot code by the 
  IN UINT16   Es;           ///<     system BIOS.
} PXENV_START_UNDI_T;

typedef struct {
  OUT UINT16  Status;       ///< Out: PXENV_STATUS_xxx
} PXENV_UNDI_STARTUP_T;

typedef struct {
  OUT UINT16  Status;       ///< Out: PXENV_STATUS_xxx
} PXENV_UNDI_CLEANUP_T;

typedef struct {
  OUT UINT16  Status;       ///< Out: PXENV_STATUS_xxx
  
  ///
  ///  This is an input parameter and is a 32-bit physical address of
  ///  a memory  copy of the  driver module in  the protocol.ini file
  ///  obtained from the  Protocol Manager  driver(refer to  NDIS 2.0
  ///  specifications).   This parameter  is basically  supported for
  ///  the universal NDIS driver to pass the information contained in
  ///  protocol.ini   file  to  the  NIC   driver  for  any  specific
  ///  configuration of   the   NIC.      (Note   that   the   module
  ///  identification in the  protocol.ini  file  was  done  by  NDIS
  ///  itself.)  This value can be NULL for for any other application
  ///  interfacing to the Universal NIC Driver.
  ///
  IN UINT32   ProtocolIni; 
  UINT8       Reserved[8];
} PXENV_UNDI_INITIALIZE_T;

typedef struct {
  OUT UINT16                  Status;       ///< Out: PXENV_STATUS_xxx
  IN PXENV_UNDI_MCAST_ADDR_T  R_Mcast_Buf;  ///< multicast address list
  /* see note below  */
} PXENV_UNDI_RESET_T;

/*++
    Note: The  NIC  driver  does  not  remember  the  multicast
    addresses provided in any  call.    So  the  application  must
    provide the multicast address  list with all  the calls that
    reset the receive unit of the adapter.  
  --*/
typedef struct {
  OUT UINT16  Status;                     ///< Out: PXENV_STATUS_xxx 
} PXENV_UNDI_SHUTDOWN_T;

typedef struct {
  OUT UINT16                  Status;     ///< Out: PXENV_STATUS_xxx
  
  ///
  ///  This is  an input parameter and is  adapter specific.  This is
  ///  supported  for Universal NDIS 2.0 driver to pass down the Open
  ///  flags  provided  by   the  protocol   driver  (See   NDIS  2.0
  ///  specifications).  This can be zero.  
  ///    
  IN UINT16                   OpenFlag;   ///< In: See description below 
  IN UINT16                   PktFilter;  ///< In: Filter for receiving 

  /* packet. It takes the following */

  /* values, multiple values can be */

  /* ORed together. */
#define FLTR_DIRECTED 0x0001                ///< directed/multicast
#define FLTR_BRDCST   0x0002                ///< broadcast packets
#define FLTR_PRMSCS   0x0004                ///< any packet on LAN
#define FLTR_SRC_RTG  0x0008                ///< source routing packet 
  IN PXENV_UNDI_MCAST_ADDR_T  McastBuffer;  /* In: */
  /* See t_PXENV_UNDI_MCAST_ADDR. */
} PXENV_UNDI_OPEN_T;

typedef struct {
  OUT UINT16  Status; ///< Out: PXENV_STATUS_xxx
} PXENV_UNDI_CLOSE_T;

#define MAX_DATA_BLKS 8

typedef struct {
  IN UINT16 ImmedLength;  ///< In: Data buffer length in

  /* bytes. */
  UINT16    XmitOffset;   ///< 16-bit segment & offset of the 
  UINT16    XmitSegment;  ///< immediate data buffer. 
  UINT16    DataBlkCount; ///< In: Number of data blocks. 
  struct DataBlk {
    UINT8   TDPtrType;    ///< 0 => 32 bit Phys pointer in TDDataPtr, not supported in this version of LSA 
                          ///< 1 => seg:offser in TDDataPtr which can be a real mode or 16-bit protected mode pointer
    UINT8   TDRsvdByte;         ///< Reserved, must be zero. 
    UINT16  TDDataLen;          ///< Data block length in bytes. 
    UINT16  TDDataPtrOffset;    ///< Far pointer to data buffer. 
    UINT16  TDDataPtrSegment;   ///< Far pointer to data buffer. 
  } DataBlock[MAX_DATA_BLKS];
}
PXENV_UNDI_TBD_T;

typedef struct {
  OUT UINT16  Status;           ///< Out: PXENV_STATUS_xxx 

  ///
  ///  This is the protocol  of  the  upper  layer  that  is  calling
  ///  NICTransmit call.   If the  upper layer  has filled  the media
  ///  header this field must be 0.
  ///
  IN UINT8    Protocol;
#define P_UNKNOWN 0
#define P_IP      1
#define P_ARP     2
#define P_RARP    3

  ///
  ///  If  this flag is  0, the NIC  driver expects a  pointer to the
  ///  destination media  address in the field  DestMediaAddr.  If 1,
  ///  the   NIC  driver   fills  the   broadcast  address   for  the
  ///  destination.
  ///  
  IN UINT8    XmitFlag;   
#define XMT_DESTADDR  0x0000    ///< destination address given 
#define XMT_BROADCAST 0x0001    ///< use broadcast address

  ///
  ///  This  is a pointer to the  hardware address of the destination
  ///  media.  It  can be  null if  the destination  is not  known in
  ///  which case the XmitFlag contains 1 for broadcast.  Destination
  ///  media address  must be  obtained by  the upper  level protocol
  ///  (with  Address Resolution Protocol) and NIC driver does not do
  ///  any address resolution.
  ///  
  IN UINT16   DestAddrOffset;   ///< 16-bit segment & offset of the
  IN UINT16   DestAddrSegment;  ///< destination media address

  
  IN UINT16   TBDOffset;  ///< 16-bit segment & offset of the 
  IN UINT16   TBDSegment; ///< transmit buffer descriptor of type 

  /// XmitBufferDesc  
  IN UINT32   Reserved[2];
} PXENV_UNDI_TRANSMIT_T;


typedef struct {
  OUT UINT16                  Status;       ///<  Out: PXENV_STATUS_xxx 
  IN PXENV_UNDI_MCAST_ADDR_T  McastBuffer;  ///<  In: 
} PXENV_UNDI_SET_MCAST_ADDR_T;

typedef struct {
  OUT UINT16  Status;                   ///< Out: PXENV_STATUS_xxx 
  IN UINT8    StationAddress[ADDR_LEN]; ///< new address to be set 
} PXENV_UNDI_SET_STATION_ADDR_T;

typedef struct s_PXENV_UNDI_SET_PACKET_FILTER {
  OUT UINT16  Status;                   ///< Out: PXENV_STATUS_xxx 
  IN UINT8    Filter;                   ///< In: Receive filter value. 
} PXENV_UNDI_SET_PACKET_FILTER_T;

typedef struct {
  OUT UINT16  Status;       ///< Out: PXENV_STATUS_xxx 
  OUT UINT16  BaseIo;       ///< Out: Adapter's Base IO 
  OUT UINT16  IntNumber;    ///< Out: IRQ number 
  OUT UINT16  MaxTranUnit;  ///< Out: MTU 
  OUT UINT16  HwType;       ///< Out: type of protocol at hardware level 

#define ETHER_TYPE      1
#define EXP_ETHER_TYPE  2
#define IEEE_TYPE       6
#define ARCNET_TYPE     7
  /*++  
        other numbers can  be obtained from  rfc1010 for "Assigned
        Numbers".  This number may not be validated by the application
        and hence adding new numbers to the list should be fine at any
        time.  
    --*/
  OUT UINT16  HwAddrLen;                    ///< Out: actual length of hardware address 
  OUT UINT8   CurrentNodeAddress[ADDR_LEN]; ///< Out: Current hardware address
  OUT UINT8   PermNodeAddress[ADDR_LEN];    ///< Out: Permanent hardware address
  OUT UINT16  ROMAddress;                   ///< Out: ROM address 
  OUT UINT16  RxBufCt;                      ///< Out: receive Queue length    
  OUT UINT16  TxBufCt;                      ///< Out: Transmit Queue length 
} PXENV_UNDI_GET_INFORMATION_T;

typedef struct {
  OUT UINT16  Status;                       ///< Out: PXENV_STATUS_xxx 
  OUT UINT32  XmtGoodFrames;                ///< Out: No. of good transmissions 
  OUT UINT32  RcvGoodFrames;                ///< Out: No. of good frames received 
  OUT UINT32  RcvCRCErrors;                 ///< Out: No. of frames with CRC error 
  OUT UINT32  RcvResourceErrors;            ///< Out: no. of frames discarded 
  /* Out: receive Queue full */
} PXENV_UNDI_GET_STATISTICS_T;

typedef struct {
  OUT UINT16  Status;               ///< Out: PXENV_STATUS_xxx 
} PXENV_UNDI_CLEAR_STATISTICS_T;

typedef struct {
  OUT UINT16  Status;               ///< Out: PXENV_STATUS_xxx
} PXENV_UNDI_INITIATE_DIAGS_T;

typedef struct {
  OUT UINT16  Status;               ///< Out: PXENV_STATUS_xxx
} PXENV_UNDI_FORCE_INTERRUPT_T;

typedef struct {
  OUT UINT16  Status;               ///< Out: PXENV_STATUS_xxx 
  IN UINT32   InetAddr;             ///< In: IP Multicast Address 
  OUT UINT8   MediaAddr[ADDR_LEN];  ///< Out: corresponding hardware 
  /*      multicast address */
} PXENV_UNDI_GET_MCAST_ADDR_T;

typedef struct {
    OUT UINT16  Vendor_ID;  ///< OUT: 
    OUT UINT16  Dev_ID;     ///< OUT: 
    OUT UINT8   Base_Class; ///< OUT: 
    OUT UINT8   Sub_Class;  ///< OUT: 
    OUT UINT8   Prog_Intf;  ///< OUT: program interface 
    OUT UINT8   Rev;        ///< OUT: Revision number 
    OUT UINT16  BusDevFunc; ///< OUT: Bus, Device  & Function numbers 
    OUT UINT16  SubVendor_ID; ///< OUT: 
    OUT UINT16  SubDevice_ID; ///< OUT: 
} PCI_INFO_T;

typedef struct {
    OUT UINT32  EISA_Dev_ID;  ///< Out: 
    OUT UINT8   Base_Class;   ///< OUT: 
    OUT UINT8   Sub_Class;    ///< OUT: 
    OUT UINT8   Prog_Intf;    ///< OUT: program interface 
    OUT UINT16  CardSelNum;   ///< OUT: Card Selector Number 
    OUT UINT8   Reserved;     ///< to make it 10 bytes 
} PNP_INFO_T;


typedef union {
  PCI_INFO_T Pci;
  PNP_INFO_T Pnp;
} PCI_PNP_INFO_T;

typedef struct {
  OUT UINT16  Status;         ///< OUT: PXENV_STATUS_xxx 
  OUT UINT8   NicType;        ///< OUT: 2=PCI, 3=PnP 
  PCI_PNP_INFO_T PciPnpInfo;
} PXENV_UNDI_GET_NIC_TYPE_T;

typedef struct {
  OUT UINT16  Status;           ///< OUT: PXENV_STATUS_xxx 
  OUT UINT8   IfaceType[16];    ///< OUT: Type name of MAC, AsciiZ 

  /*      format.  This is used by the */

  /*      Universal NDIS Driver to fill */

  /*      the driver type in it's MAC */

  /*      Service specific */

  /*      characteristic table */
  OUT UINT32  LinkSpeed;    ///< OUT: 
  OUT UINT32  ServiceFlags; ///< OUT: as defined in NDIS Spec 2.0X 
  OUT UINT32  Reserved[4];  ///< OUT: will be filled with 0s till defined
} PXENV_UNDI_GET_NDIS_INFO_T;

typedef struct {
  OUT UINT16    Status;   ///< OUT: PXENV_STATUS_xxx 
  IN OUT UINT16 FuncFlag; ///< In: PXENV_UNDI_ISR_IN_xxx 

  /* Out: PXENV_UNDI_ISR_OUT_xxx */
  OUT UINT16    BufferLength;
  OUT UINT16    FrameLength;
  OUT UINT16    FrameHeaderLength;
  OUT UINT16    FrameOffset;
  OUT UINT16    FrameSegSel;
  OUT UINT8     ProtType;
  OUT UINT8     PktType;
} PXENV_UNDI_ISR_T;

#define PXENV_UNDI_ISR_IN_START 1 /* This function must be first */

/* when an interrupt is received. */

/* It will tell us if the intr */

/* was generated by our device. */
#define PXENV_UNDI_ISR_IN_PROCESS 2 /* Call to start processing one of */

/* our interrupts. */
#define PXENV_UNDI_ISR_IN_GET_NEXT  3 /* Call to start/continue receiving */

/* data from receive buffer(s). */

/*++

     Possible responses from PXENV_UNDI_ISR_IN_START

 --*/
#define PXENV_UNDI_ISR_OUT_OURS 0 ///< This is our interrupt.  Deal  with it. 
#define PXENV_UNDI_ISR_OUT_NOT_OURS 1 ///< This is not our interrupt.

/*++

     Possible responses from PXENV_UNDI_ISR_IN_PROCESS and
     PXENV_UNDI_ISR_IN_PROCESS

--*/
#define PXENV_UNDI_ISR_OUT_DONE 0       ///< We are done processing this  interrupt. 
#define PXENV_UNDI_ISR_OUT_TRANSMIT 2   ///< We completed a transmit interrupt. 
#define PXENV_UNDI_ISR_OUT_RECEIVE  3   ///< Get data from receive buffer. 

#define PXENV_UNDI_ISR_OUT_BUSY     4 /* ? */

typedef struct {
  UINT16  Status;                     ///< Out: PXENV_STATUS_xxx 
} PXENV_STOP_UNDI_T;

#define PXENV_UNDI_STARTED      1     ///< not even initialized 
#define PXENV_UNDI_INITIALIZED  2     ///< initialized and closed (not opened) 
#define PXENV_UNDI_OPENED       3     ///< initialized & opened 

typedef struct {
  OUT UINT16  Status;                 ///< Out: PXENV_STATUS_xxx 
  UINT16      UNDI_State;
} PXENV_UNDI_GET_STATE_T;

#pragma pack()

#endif
