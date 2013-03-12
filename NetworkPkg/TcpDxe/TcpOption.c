/** @file
  Routines to process TCP option.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "TcpMain.h"

/**
  Get a UINT16 value from buffer.

  @param[in] Buf              Pointer to input buffer.

  @return                     The UINT16 value obtained from the buffer.

**/
UINT16
TcpGetUint16 (
  IN UINT8 *Buf
  )
{
  UINT16  Value;
  CopyMem (&Value, Buf, sizeof (UINT16));
  return NTOHS (Value);
}

/**
  Get a UINT32 value from buffer.

  @param[in] Buf              Pointer to input buffer.

  @return                     The UINT32 value obtained from the buffer.

**/
UINT32
TcpGetUint32 (
  IN UINT8 *Buf
  )
{
  UINT32  Value;
  CopyMem (&Value, Buf, sizeof (UINT32));
  return NTOHL (Value);
}

/**
  Put a UINT32 value in buffer.

  @param[out] Buf             Pointer to the buffer.
  @param[in] Data             The UINT32 Date to put in the buffer.

**/
VOID
TcpPutUint32 (
     OUT UINT8  *Buf,
  IN     UINT32 Data
  )
{
  Data = HTONL (Data);
  CopyMem (Buf, &Data, sizeof (UINT32));
}

/**
  Compute the window scale value according to the given buffer size.

  @param[in]  Tcb Pointer to the TCP_CB of this TCP instance.

  @return         The scale value.

**/
UINT8
TcpComputeScale (
  IN TCP_CB *Tcb
  )
{
  UINT8   Scale;
  UINT32  BufSize;

  ASSERT ((Tcb != NULL) && (Tcb->Sk != NULL));

  BufSize = GET_RCV_BUFFSIZE (Tcb->Sk);

  Scale   = 0;
  while ((Scale < TCP_OPTION_MAX_WS) && ((UINT32) (TCP_OPTION_MAX_WIN << Scale) < BufSize)) {

    Scale++;
  }

  return Scale;
}

/**
  Build the TCP option in three-way handshake.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param[in]  Nbuf    Pointer to the buffer to store the options.

  @return             The total length of the TCP option field.

**/
UINT16
TcpSynBuildOption (
  IN TCP_CB  *Tcb,
  IN NET_BUF *Nbuf
  )
{
  UINT8   *Data;
  UINT16  Len;

  ASSERT ((Tcb != NULL) && (Nbuf != NULL) && (Nbuf->Tcp == NULL));

  Len = 0;

  //
  // Add a timestamp option if not disabled by the application
  // and it is the first SYN segment, or the peer has sent
  // us its timestamp.
  //
  if (!TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_NO_TS) &&
      (!TCP_FLG_ON (TCPSEG_NETBUF (Nbuf)->Flag, TCP_FLG_ACK) ||
        TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_RCVD_TS))
      ) {

    Data = NetbufAllocSpace (
             Nbuf,
             TCP_OPTION_TS_ALIGNED_LEN,
             NET_BUF_HEAD
             );

    ASSERT (Data != NULL);
    Len += TCP_OPTION_TS_ALIGNED_LEN;

    TcpPutUint32 (Data, TCP_OPTION_TS_FAST);
    TcpPutUint32 (Data + 4, mTcpTick);
    TcpPutUint32 (Data + 8, 0);
  }

  //
  // Build window scale option, only when configured
  // to send WS option, and either we are doing active
  // open or we have received WS option from peer.
  //
  if (!TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_NO_WS) &&
      (!TCP_FLG_ON (TCPSEG_NETBUF (Nbuf)->Flag, TCP_FLG_ACK) ||
        TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_RCVD_WS))
      ) {

    Data = NetbufAllocSpace (
             Nbuf,
             TCP_OPTION_WS_ALIGNED_LEN,
             NET_BUF_HEAD
             );

    ASSERT (Data != NULL);

    Len += TCP_OPTION_WS_ALIGNED_LEN;
    TcpPutUint32 (Data, TCP_OPTION_WS_FAST | TcpComputeScale (Tcb));
  }

  //
  // Build the MSS option.
  //
  Data = NetbufAllocSpace (Nbuf, TCP_OPTION_MSS_LEN, 1);
  ASSERT (Data != NULL);

  Len += TCP_OPTION_MSS_LEN;
  TcpPutUint32 (Data, TCP_OPTION_MSS_FAST | Tcb->RcvMss);

  return Len;
}

/**
  Build the TCP option in synchronized states.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param[in]  Nbuf    Pointer to the buffer to store the options.

  @return             The total length of the TCP option field.

**/
UINT16
TcpBuildOption (
  IN TCP_CB  *Tcb,
  IN NET_BUF *Nbuf
  )
{
  UINT8   *Data;
  UINT16  Len;

  ASSERT ((Tcb != NULL) && (Nbuf != NULL) && (Nbuf->Tcp == NULL));
  Len = 0;

  //
  // Build the Timestamp option.
  //
  if (TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_SND_TS) &&
      !TCP_FLG_ON (TCPSEG_NETBUF (Nbuf)->Flag, TCP_FLG_RST)
      ) {

    Data = NetbufAllocSpace (
            Nbuf,
            TCP_OPTION_TS_ALIGNED_LEN,
            NET_BUF_HEAD
            );

    ASSERT (Data != NULL);
    Len += TCP_OPTION_TS_ALIGNED_LEN;

    TcpPutUint32 (Data, TCP_OPTION_TS_FAST);
    TcpPutUint32 (Data + 4, mTcpTick);
    TcpPutUint32 (Data + 8, Tcb->TsRecent);
  }

  return Len;
}

/**
  Parse the supported options.

  @param[in]       Tcp     Pointer to the TCP_CB of this TCP instance.
  @param[in, out]  Option  Pointer to the TCP_OPTION used to store the
                           successfully pasrsed options.

  @retval          0       The options are successfully pasrsed.
  @retval          -1      Ilegal option was found.

**/
INTN
TcpParseOption (
  IN     TCP_HEAD   *Tcp,
  IN OUT TCP_OPTION *Option
  )
{
  UINT8 *Head;
  UINT8 TotalLen;
  UINT8 Cur;
  UINT8 Type;
  UINT8 Len;

  ASSERT ((Tcp != NULL) && (Option != NULL));

  Option->Flag  = 0;

  TotalLen      = (UINT8) ((Tcp->HeadLen << 2) - sizeof (TCP_HEAD));
  if (TotalLen <= 0) {
    return 0;
  }

  Head = (UINT8 *) (Tcp + 1);

  //
  // Fast process of the timestamp option.
  //
  if ((TotalLen == TCP_OPTION_TS_ALIGNED_LEN) && (TcpGetUint32 (Head) == TCP_OPTION_TS_FAST)) {

    Option->TSVal = TcpGetUint32 (Head + 4);
    Option->TSEcr = TcpGetUint32 (Head + 8);
    Option->Flag  = TCP_OPTION_RCVD_TS;

    return 0;
  }
  //
  // Slow path to process the options.
  //
  Cur = 0;

  while (Cur < TotalLen) {
    Type = Head[Cur];

    switch (Type) {
    case TCP_OPTION_MSS:
      Len = Head[Cur + 1];

      if ((Len != TCP_OPTION_MSS_LEN) || (TotalLen - Cur < TCP_OPTION_MSS_LEN)) {

        return -1;
      }

      Option->Mss = TcpGetUint16 (&Head[Cur + 2]);
      TCP_SET_FLG (Option->Flag, TCP_OPTION_RCVD_MSS);

      Cur += TCP_OPTION_MSS_LEN;
      break;

    case TCP_OPTION_WS:
      Len = Head[Cur + 1];

      if ((Len != TCP_OPTION_WS_LEN) || (TotalLen - Cur < TCP_OPTION_WS_LEN)) {

        return -1;
      }

      Option->WndScale = (UINT8) MIN (14, Head[Cur + 2]);
      TCP_SET_FLG (Option->Flag, TCP_OPTION_RCVD_WS);

      Cur += TCP_OPTION_WS_LEN;
      break;

    case TCP_OPTION_TS:
      Len = Head[Cur + 1];

      if ((Len != TCP_OPTION_TS_LEN) || (TotalLen - Cur < TCP_OPTION_TS_LEN)) {

        return -1;
      }

      Option->TSVal = TcpGetUint32 (&Head[Cur + 2]);
      Option->TSEcr = TcpGetUint32 (&Head[Cur + 6]);
      TCP_SET_FLG (Option->Flag, TCP_OPTION_RCVD_TS);

      Cur += TCP_OPTION_TS_LEN;
      break;

    case TCP_OPTION_NOP:
      Cur++;
      break;

    case TCP_OPTION_EOP:
      Cur = TotalLen;
      break;

    default:
      Len = Head[Cur + 1];

      if ((TotalLen - Cur) < Len || Len < 2) {
        return -1;
      }

      Cur = (UINT8) (Cur + Len);
      break;
    }

  }

  return 0;
}

/**
  Check the segment against PAWS.

  @param[in]  Tcb     Pointer to the TCP_CB of this TCP instance.
  @param[in]  TSVal   The timestamp value.

  @retval     1       The segment passed the PAWS check.
  @retval     0       The segment failed to pass the PAWS check.

**/
UINT32
TcpPawsOK (
  IN TCP_CB *Tcb,
  IN UINT32 TSVal
  )
{
  //
  // PAWS as defined in RFC1323, buggy...
  //
  if (TCP_TIME_LT (TSVal, Tcb->TsRecent) &&
      TCP_TIME_LT (Tcb->TsRecentAge + TCP_PAWS_24DAY, mTcpTick)
      ) {

    return 0;

  }

  return 1;
}
