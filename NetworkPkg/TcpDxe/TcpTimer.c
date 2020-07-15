/** @file
  TCP timer related functions.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TcpMain.h"

UINT32    mTcpTick = 1000;

/**
  Connect timeout handler.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpConnectTimeout (
  IN OUT TCP_CB *Tcb
  );

/**
  Timeout handler for TCP retransmission timer.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpRexmitTimeout (
  IN OUT TCP_CB *Tcb
  );

/**
  Timeout handler for window probe timer.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpProbeTimeout (
  IN OUT TCP_CB *Tcb
  );

/**
  Timeout handler for keepalive timer.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpKeepaliveTimeout (
  IN OUT TCP_CB *Tcb
  );

/**
  Timeout handler for FIN_WAIT_2 timer.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpFinwait2Timeout (
  IN OUT TCP_CB *Tcb
  );

/**
  Timeout handler for 2MSL timer.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
Tcp2MSLTimeout (
  IN OUT TCP_CB *Tcb
  );

TCP_TIMER_HANDLER mTcpTimerHandler[TCP_TIMER_NUMBER] = {
  TcpConnectTimeout,
  TcpRexmitTimeout,
  TcpProbeTimeout,
  TcpKeepaliveTimeout,
  TcpFinwait2Timeout,
  Tcp2MSLTimeout,
};

/**
  Close the TCP connection.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpClose (
  IN OUT TCP_CB *Tcb
  )
{
  NetbufFreeList (&Tcb->SndQue);
  NetbufFreeList (&Tcb->RcvQue);

  TcpSetState (Tcb, TCP_CLOSED);
}

/**
  Backoff the RTO.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpBackoffRto (
  IN OUT TCP_CB *Tcb
  )
{
  //
  // Fold the RTT estimate if too many times, the estimate
  // may be wrong, fold it. So the next time a valid
  // measurement is sampled, we can start fresh.
  //
  if ((Tcb->LossTimes >= TCP_FOLD_RTT) && (Tcb->SRtt != 0)) {
    Tcb->RttVar += Tcb->SRtt >> 2;
    Tcb->SRtt = 0;
  }

  Tcb->Rto <<= 1;

  if (Tcb->Rto < TCP_RTO_MIN) {

    Tcb->Rto = TCP_RTO_MIN;
  } else if (Tcb->Rto > TCP_RTO_MAX) {

    Tcb->Rto = TCP_RTO_MAX;
  }
}

/**
  Connect timeout handler.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpConnectTimeout (
  IN OUT TCP_CB *Tcb
  )
{
  if (!TCP_CONNECTED (Tcb->State)) {
    DEBUG (
      (EFI_D_ERROR,
      "TcpConnectTimeout: connection closed because connection timer timeout for TCB %p\n",
      Tcb)
      );

    if (EFI_ABORTED == Tcb->Sk->SockError) {
      SOCK_ERROR (Tcb->Sk, EFI_TIMEOUT);
    }

    if (TCP_SYN_RCVD == Tcb->State) {
      DEBUG (
        (EFI_D_WARN,
        "TcpConnectTimeout: send reset because connection timer timeout for TCB %p\n",
        Tcb)
        );

      TcpResetConnection (Tcb);

    }

    TcpClose (Tcb);
  }
}


/**
  Timeout handler for TCP retransmission timer.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpRexmitTimeout (
  IN OUT TCP_CB *Tcb
  )
{
  UINT32  FlightSize;

  DEBUG (
    (EFI_D_WARN,
    "TcpRexmitTimeout: transmission timeout for TCB %p\n",
    Tcb)
    );

  //
  // Set the congestion window. FlightSize is the
  // amount of data that has been sent but not
  // yet ACKed.
  //
  FlightSize        = TCP_SUB_SEQ (Tcb->SndNxt, Tcb->SndUna);
  Tcb->Ssthresh     = MAX ((UINT32) (2 * Tcb->SndMss), FlightSize / 2);

  Tcb->CWnd         = Tcb->SndMss;
  Tcb->LossRecover  = Tcb->SndNxt;

  Tcb->LossTimes++;
  if ((Tcb->LossTimes > Tcb->MaxRexmit) && !TCP_TIMER_ON (Tcb->EnabledTimer, TCP_TIMER_CONNECT)) {

    DEBUG (
      (EFI_D_ERROR,
      "TcpRexmitTimeout: connection closed because too many timeouts for TCB %p\n",
      Tcb)
      );

    if (EFI_ABORTED == Tcb->Sk->SockError) {
      SOCK_ERROR (Tcb->Sk, EFI_TIMEOUT);
    }

    TcpClose (Tcb);
    return ;
  }

  TcpBackoffRto (Tcb);
  TcpRetransmit (Tcb, Tcb->SndUna);
  TcpSetTimer (Tcb, TCP_TIMER_REXMIT, Tcb->Rto);

  Tcb->CongestState = TCP_CONGEST_LOSS;

  TCP_CLEAR_FLG (Tcb->CtrlFlag, TCP_CTRL_RTT_ON);
}

/**
  Timeout handler for window probe timer.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpProbeTimeout (
  IN OUT TCP_CB *Tcb
  )
{
  //
  // This is the timer for sender's SWSA. RFC1122 requires
  // a timer set for sender's SWSA, and suggest combine it
  // with window probe timer. If data is sent, don't set
  // the probe timer, since retransmit timer is on.
  //
  if ((TcpDataToSend (Tcb, 1) != 0) && (TcpToSendData (Tcb, 1) > 0)) {

    ASSERT (TCP_TIMER_ON (Tcb->EnabledTimer, TCP_TIMER_REXMIT) != 0);
    Tcb->ProbeTimerOn = FALSE;
    return ;
  }

  TcpSendZeroProbe (Tcb);
  TcpSetProbeTimer (Tcb);
}

/**
  Timeout handler for keepalive timer.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpKeepaliveTimeout (
  IN OUT TCP_CB *Tcb
  )
{
  Tcb->KeepAliveProbes++;

  //
  // Too many Keep-alive probes, drop the connection
  //
  if (Tcb->KeepAliveProbes > Tcb->MaxKeepAlive) {

    if (EFI_ABORTED == Tcb->Sk->SockError) {
      SOCK_ERROR (Tcb->Sk, EFI_TIMEOUT);
    }

    TcpClose (Tcb);
    return ;
  }

  TcpSendZeroProbe (Tcb);
  TcpSetKeepaliveTimer (Tcb);
}

/**
  Timeout handler for FIN_WAIT_2 timer.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpFinwait2Timeout (
  IN OUT TCP_CB *Tcb
  )
{
  DEBUG (
    (EFI_D_WARN,
    "TcpFinwait2Timeout: connection closed because FIN_WAIT2 timer timeouts for TCB %p\n",
    Tcb)
    );

  TcpClose (Tcb);
}

/**
  Timeout handler for 2MSL timer.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
Tcp2MSLTimeout (
  IN OUT TCP_CB *Tcb
  )
{
  DEBUG (
    (EFI_D_WARN,
    "Tcp2MSLTimeout: connection closed because TIME_WAIT timer timeouts for TCB %p\n",
    Tcb)
    );

  TcpClose (Tcb);
}

/**
  Update the timer status and the next expire time according to the timers
  to expire in a specific future time slot.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpUpdateTimer (
  IN OUT TCP_CB *Tcb
  )
{
  UINT16  Index;

  //
  // Don't use a too large value to init NextExpire
  // since mTcpTick wraps around as sequence no does.
  //
  Tcb->NextExpire = TCP_EXPIRE_TIME;
  TCP_CLEAR_FLG (Tcb->CtrlFlag, TCP_CTRL_TIMER_ON);

  for (Index = 0; Index < TCP_TIMER_NUMBER; Index++) {

    if (TCP_TIMER_ON (Tcb->EnabledTimer, Index) &&
        TCP_TIME_LT (Tcb->Timer[Index], mTcpTick + Tcb->NextExpire)
        ) {

      Tcb->NextExpire = TCP_SUB_TIME (Tcb->Timer[Index], mTcpTick);
      TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_TIMER_ON);
    }
  }
}

/**
  Enable a TCP timer.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param[in]       Timer    The index of the timer to be enabled.
  @param[in]       TimeOut  The timeout value of this timer.

**/
VOID
TcpSetTimer (
  IN OUT TCP_CB *Tcb,
  IN     UINT16 Timer,
  IN     UINT32 TimeOut
  )
{
  TCP_SET_TIMER (Tcb->EnabledTimer, Timer);
  Tcb->Timer[Timer] = mTcpTick + TimeOut;

  TcpUpdateTimer (Tcb);
}

/**
  Clear one TCP timer.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param[in]       Timer    The index of the timer to be cleared.

**/
VOID
TcpClearTimer (
  IN OUT TCP_CB *Tcb,
  IN     UINT16 Timer
  )
{
  TCP_CLEAR_TIMER (Tcb->EnabledTimer, Timer);
  TcpUpdateTimer (Tcb);
}

/**
  Clear all TCP timers.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpClearAllTimer (
  IN OUT TCP_CB *Tcb
  )
{
  Tcb->EnabledTimer = 0;
  TcpUpdateTimer (Tcb);
}

/**
  Enable the window prober timer and set the timeout value.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpSetProbeTimer (
  IN OUT TCP_CB *Tcb
  )
{
  if (!Tcb->ProbeTimerOn) {
    Tcb->ProbeTime    = Tcb->Rto;
    Tcb->ProbeTimerOn = TRUE;

  } else {
    Tcb->ProbeTime <<= 1;
  }

  if (Tcb->ProbeTime < TCP_RTO_MIN) {

    Tcb->ProbeTime = TCP_RTO_MIN;
  } else if (Tcb->ProbeTime > TCP_RTO_MAX) {

    Tcb->ProbeTime = TCP_RTO_MAX;
  }

  TcpSetTimer (Tcb, TCP_TIMER_PROBE, Tcb->ProbeTime);
}

/**
  Enable the keepalive timer and set the timeout value.

  @param[in, out]  Tcb      Pointer to the TCP_CB of this TCP instance.

**/
VOID
TcpSetKeepaliveTimer (
  IN OUT TCP_CB *Tcb
  )
{
  if (TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_NO_KEEPALIVE)) {
    return ;

  }

  //
  // Set the timer to KeepAliveIdle if either
  // 1. the keepalive timer is off
  // 2. The keepalive timer is on, but the idle
  // is less than KeepAliveIdle, that means the
  // connection is alive since our last probe.
  //
  if (!TCP_TIMER_ON (Tcb->EnabledTimer, TCP_TIMER_KEEPALIVE) ||
      (Tcb->Idle < Tcb->KeepAliveIdle)
      ) {

    TcpSetTimer (Tcb, TCP_TIMER_KEEPALIVE, Tcb->KeepAliveIdle);
    Tcb->KeepAliveProbes = 0;

  } else {

    TcpSetTimer (Tcb, TCP_TIMER_KEEPALIVE, Tcb->KeepAlivePeriod);
  }
}

/**
  Heart beat timer handler.

  @param[in]  Context        Context of the timer event, ignored.

**/
VOID
EFIAPI
TcpTickingDpc (
  IN VOID       *Context
  )
{
  LIST_ENTRY      *Entry;
  LIST_ENTRY      *Next;
  TCP_CB          *Tcb;
  INT16           Index;

  mTcpTick++;
  mTcpGlobalIss += TCP_ISS_INCREMENT_2;

  //
  // Don't use LIST_FOR_EACH, which isn't delete safe.
  //
  for (Entry = mTcpRunQue.ForwardLink; Entry != &mTcpRunQue; Entry = Next) {

    Next  = Entry->ForwardLink;

    Tcb   = NET_LIST_USER_STRUCT (Entry, TCP_CB, List);

    if (Tcb->State == TCP_CLOSED) {
      continue;
    }
    //
    // The connection is doing RTT measurement.
    //
    if (TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_RTT_ON)) {
      Tcb->RttMeasure++;
    }

    Tcb->Idle++;

    if (Tcb->DelayedAck != 0) {
      TcpSendAck (Tcb);
    }

    if (Tcb->IpInfo->IpVersion == IP_VERSION_6 && Tcb->Tick > 0) {
      Tcb->Tick--;
    }

    //
    // No timer is active or no timer expired
    //
    if (!TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_TIMER_ON) || ((--Tcb->NextExpire) > 0)) {

      continue;
    }

    //
    // Call the timeout handler for each expired timer.
    //
    for (Index = 0; Index < TCP_TIMER_NUMBER; Index++) {

      if (TCP_TIMER_ON (Tcb->EnabledTimer, Index) && TCP_TIME_LEQ (Tcb->Timer[Index], mTcpTick)) {
        //
        // disable the timer before calling the handler
        // in case the handler enables it again.
        //
        TCP_CLEAR_TIMER (Tcb->EnabledTimer, Index);
        mTcpTimerHandler[Index](Tcb);

        //
        // The Tcb may have been deleted by the timer, or
        // no other timer is set.
        //
        if ((Next->BackLink != Entry) || (Tcb->EnabledTimer == 0)) {
          break;
        }
      }
    }

    //
    // If the Tcb still exist or some timer is set, update the timer
    //
    if (Index == TCP_TIMER_NUMBER) {
      TcpUpdateTimer (Tcb);
    }
  }
}

/**
  Heart beat timer handler, queues the DPC at TPL_CALLBACK.

  @param[in]  Event    Timer event signaled, ignored.
  @param[in]  Context  Context of the timer event, ignored.

**/
VOID
EFIAPI
TcpTicking (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  QueueDpc (TPL_CALLBACK, TcpTickingDpc, Context);
}

