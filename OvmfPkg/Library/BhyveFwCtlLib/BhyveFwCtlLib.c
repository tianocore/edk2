/** @file

  Copyright (c) 2020, Rebecca Cran <rebecca@bsdio.com>
  Copyright (c) 2011 - 2013, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2013, Red Hat, Inc.
  Copyright (c) 2015, Nahanni Systems.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Uefi.h"
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BhyveFwCtlLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define FW_PORT    0x510
#define FW_IPORT   0x511

/* Transport protocol basic operations */
#define OP_NULL      1
#define OP_ECHO      2
#define OP_GET       3
#define OP_GET_LEN   4
#define OP_SET       5

/* Transport protocol error returns */
#define T_ESUCCESS  0
#define T_ENOENT    2
#define T_E2BIG     7
#define T_EMSGSIZE  40

#define ROUNDUP(x, y) ((((x)+((y)-1))/(y))*(y))

STATIC CONST CHAR8 mBhyveSig[4] = { 'B', 'H', 'Y', 'V' };

STATIC BOOLEAN mBhyveFwCtlSupported = FALSE;

STATIC INT32 mBhyveFwCtlTxid = 0xa5;

/* XXX Maybe a better inbuilt version of this ? */
typedef struct {
  VOID     *Base;
  UINT32    Len;
} BIO_VEC;

typedef struct {
  UINT32    Sz;
  UINT32    Op;
  UINT32    TxId;
  UINT32    Err;
} MSG_RX_HDR;

STATIC
RETURN_STATUS
EFIAPI
BhyveFwCtl_CvtErr (
  IN UINT32    errno
  )
{
  RETURN_STATUS        Status;

  switch (errno) {
  case T_ESUCCESS:
    Status = RETURN_SUCCESS;
    break;
  case T_ENOENT:
    Status = RETURN_NOT_FOUND;
    break;
  case T_E2BIG:
    Status = RETURN_INVALID_PARAMETER;
    break;
  case T_EMSGSIZE:
    Status = RETURN_BUFFER_TOO_SMALL;
    break;
  default:
    Status = RETURN_PROTOCOL_ERROR;
    break;
  }

  return Status;
}

STATIC
UINT32
EFIAPI
BIov_WLen (
  IN BIO_VEC b[]
  )
{
  UINT32        i;
  UINT32        tLen;

  tLen = 0;

  if (b != NULL) {
    for (i = 0; b[i].Base != NULL; i++)
      tLen += ROUNDUP (b[i].Len, sizeof(UINT32));
  }

  return tLen;
}

/**
   Utility to send 1-3 bytes of input as a 4-byte value
   with trailing zeroes.
 **/
STATIC
UINT32
BIov_Send_Rem (
  IN UINT32  *Data,
  IN UINT32   Len
  )
{
  union {
    UINT8    c[4];
    UINT32    w;
  } u;
  UINT8        *cdata;
  UINT32        i;

  cdata = (UINT8 *)Data;
  u.w = 0;

  for (i = 0; i < Len; i++)
    u.c[i] = *cdata++;

  return u.w;
}

/**
   Send a block of data out the i/o port as 4-byte quantities,
   appending trailing zeroes on the last if required.
 **/
STATIC
VOID
BIov_Send (
  IN char    *Data,
  IN UINT32   Len
  )
{
  UINT32    *LData;

  LData = (UINT32 *)Data;

  while (Len > sizeof(UINT32)) {
    IoWrite32 (FW_PORT, *LData++);
    Len -= sizeof(UINT32);
  }

  if (Len > 0) {
    IoWrite32 (FW_PORT, BIov_Send_Rem (LData, Len));
  }
}

/**
   Send data described by an array of iovecs out the i/o port.
 **/
STATIC
VOID
BIov_SendAll (
   IN BIO_VEC b[]
   )
{
  INT32        i;

  if (b != NULL) {
    for (i = 0; b[i].Base; i++) {
      BIov_Send (b[i].Base, b[i].Len);
    }
  }
}

/**
   Prepend the transport header to a block of data and send.
 **/
STATIC
VOID
EFIAPI
BhyveFwCtl_MsgSend(
  IN  UINT32  OpCode,
  IN  BIO_VEC  Data[]
  )
{
  BIO_VEC  hIov[4];
  UINT32  Hdr[3];
  UINT32  i;

  /* Set up header as an iovec */
  for (i = 0; i < 3; i++) {
    hIov[i].Base = &Hdr[i];
    hIov[i].Len  = sizeof(Hdr[0]);
  }
  hIov[i].Base = NULL;
  hIov[i].Len = 0;

  /* Initialize header */
  Hdr[0] = BIov_WLen (hIov) + BIov_WLen (Data);
  Hdr[1] = (UINT32)OpCode;
  Hdr[2] = mBhyveFwCtlTxid;

  /* Send header and data */
  BIov_SendAll (hIov);
  BIov_SendAll (Data);
}

/**
   Read a transport response and optional data from the i/o port.
 **/
STATIC
RETURN_STATUS
EFIAPI
BhyveFwCtl_MsgRecv(
  OUT  MSG_RX_HDR *Rhdr,
  OUT  BIO_VEC    Data[]
  )
{
  RETURN_STATUS  Status;
  UINT32        *Dp;
  UINT32         Rd;
  UINT32         remLen;
  INT32          oLen;
  INT32          xLen;

  Rd = IoRead32 (FW_PORT);
  if (Rd < sizeof (MSG_RX_HDR)) {
    ;
  }

  /* Read in header and setup initial error */
  Rhdr->Sz   = Rd;
  Rhdr->Op   = IoRead32 (FW_PORT);
  Rhdr->TxId = IoRead32 (FW_PORT);
  Rhdr->Err  = IoRead32 (FW_PORT);

  /* Convert transport errno into UEFI error status */
  Status = BhyveFwCtl_CvtErr (Rhdr->Err);

  remLen = Rd - sizeof (MSG_RX_HDR);
  xLen = 0;

  /*
   * A few cases to handle:
   *  - the user didn't supply a read buffer
   *  - the buffer is too small for the response
   *  - the response is zero-length
   */
  if (Data != NULL) {
    Dp = (UINT32 *)Data[0].Base;
    oLen = remLen;
    if (remLen > Data[0].Len) {
      Status = RETURN_BUFFER_TOO_SMALL;
      xLen = remLen - Data[0].Len;
      oLen = remLen = Data[0].Len;
    }
    while (remLen > 0) {
      *Dp++ = IoRead32 (FW_PORT);
      remLen -= sizeof (UINT32);
    }
    Data[0].Len = oLen;
  } else {
    /* No user data, but data returned - drop */
    if (remLen > 0) {
      Status = RETURN_BUFFER_TOO_SMALL;
      xLen = remLen;
    }
  }

  /* Drop additional data */
  while (xLen > 0) {
    (void) IoRead32 (FW_PORT);
    xLen -= sizeof (UINT32);
  }

  return Status;
}


STATIC
RETURN_STATUS
EFIAPI
BhyveFwCtl_Msg(
   IN   UINT32 OpCode,
   IN   BIO_VEC Sdata[],
   OUT  BIO_VEC Rdata[]
   )
{
  MSG_RX_HDR  Rh;
  RETURN_STATUS    Status;

  Status = RETURN_SUCCESS;

  BhyveFwCtl_MsgSend (OpCode, Sdata);
  Status = BhyveFwCtl_MsgRecv (&Rh, Rdata);

  mBhyveFwCtlTxid++;

  return Status;
}

STATIC
RETURN_STATUS
EFIAPI
BhyveFwCtlGetLen (
  IN CONST CHAR8  *Name,
  IN OUT   UINT32 *Size
  )
{
  BIO_VEC         Req[2], Resp[2];
  RETURN_STATUS  Status;

  Req[0].Base = (VOID *)Name;
  Req[0].Len  = (UINT32)AsciiStrLen (Name) + 1;
  Req[1].Base = NULL;

  Resp[0].Base = Size;
  Resp[0].Len  = sizeof (UINT32);
  Resp[1].Base = NULL;

  Status = BhyveFwCtl_Msg (OP_GET_LEN, Req, Resp);

  return Status;
}

#define FMAXSZ    1024
STATIC struct {
  UINT64    fSize;
  UINT32    fData[FMAXSZ];
} FwGetvalBuf;

STATIC
RETURN_STATUS
EFIAPI
BhyveFwCtlGetVal (
  IN CONST CHAR8 *Name,
  OUT      VOID  *Item,
  IN OUT   UINT32   *Size
  )
{
  BIO_VEC         Req[2];
  BIO_VEC         Resp[2];
  RETURN_STATUS  Status;

  /* Make sure temp buffer is larger than passed-in size */
  if (*Size > sizeof (FwGetvalBuf.fData))
      return RETURN_INVALID_PARAMETER;

  Req[0].Base = (VOID *)Name;
  Req[0].Len  = (UINT32)AsciiStrLen (Name) + 1;
  Req[1].Base = NULL;

  Resp[0].Base = &FwGetvalBuf;
  Resp[0].Len  = sizeof (UINT64) + *Size;
  Resp[1].Base = NULL;

  Status = BhyveFwCtl_Msg (OP_GET, Req, Resp);

  /*
   * Copy out data on success (or on a truncated message).
   * XXX This step can be eliminted with Msg() supporting
   *     multiple iovecs.
   */
  if ((Status == RETURN_SUCCESS) || (Status == RETURN_BUFFER_TOO_SMALL)) {
    *Size = (UINT32)FwGetvalBuf.fSize;
    CopyMem (Item, FwGetvalBuf.fData, *Size);
  }

  return Status;
}

/**
   Front end to the internal GET_LEN and GET protocols
 **/
RETURN_STATUS
EFIAPI
BhyveFwCtlGet (
  IN   CONST CHAR8 *Name,
  OUT  VOID        *Item,
  IN OUT  UINTN    *Size
  )
{
  RETURN_STATUS        Status;

  if (mBhyveFwCtlSupported == FALSE)
    return RETURN_UNSUPPORTED;

  if (Item == NULL) {
    Status = BhyveFwCtlGetLen (Name, (UINT32*)Size);
  } else {
    Status = BhyveFwCtlGetVal (Name, Item, (UINT32*)Size);
  }

  return Status;
}


/**
   Library initialization. Probe the host to see if the f/w ctl
   interface is supported.
 **/
RETURN_STATUS
EFIAPI
BhyveFwCtlInitialize (
  VOID
  )
{
  UINT32  i;
  UINT8   ch;

  DEBUG ((DEBUG_INFO, "FwCtlInitialize\n"));

  IoWrite16 (FW_PORT, 0x0000);
  for (i = 0; i < 4; i++) {
    ch = IoRead8 (FW_IPORT);
    if (ch != mBhyveSig[i]) {
      DEBUG ((DEBUG_INFO, "Host f/w sig mismatch %c/%c\n", ch, mBhyveSig[i]));
      return RETURN_SUCCESS;
    }
  }

  mBhyveFwCtlSupported = TRUE;

  return RETURN_SUCCESS;
}
