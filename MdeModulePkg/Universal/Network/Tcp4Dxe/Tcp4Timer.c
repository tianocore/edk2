/** @file

Copyright (c) 2005 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Tcp4Timer.c

Abstract:

  TCP timer related functions.


**/

#include "Tcp4Main.h"

UINT32    mTcpTick = 1000;

STATIC
VOID
TcpConnectTimeout (
  IN TCP_CB *Tcb
  );

STATIC
VOID
TcpRexmitTimeout (
  IN TCP_CB *Tcb
  );

STATIC
VOID
TcpProbeTimeout (
  IN TCP_CB *Tcb
  );

STATIC
VOID
TcpKeepaliveTimeout (
  IN TCP_CB *Tcb
  );

STATIC
VOID
TcpFinwait2Timeout (
  IN TCP_CB *Tcb
  );

STATIC
VOID
Tcp2MSLTimeout (
  IN TCP_CB *Tcb
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

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

  @return None.

**/
VOID
TcpClose (
  IN TCP_CB *Tcb
  )
{
  NetbufFreeList (&Tcb->SndQue);
  NetbufFreeList (&Tcb->RcvQue);

  TcpSetState (Tcb, TCP_CLOSED);
}


/**
  Connect timeout handler.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

  @return None.

**/
STATIC
VOID
TcpConnectTimeout (
  IN TCP_CB *Tcb
  )
{
  if (!TCP_CONNECTED (Tcb->State)) {
    DEBUG ((EFI_D_ERROR, "TcpConnectTimeout: connection closed "
      "because conenction timer timeout for TCB %x\n", Tcb));

    if (EFI_ABORTED == Tcb->Sk->SockError) {
      SOCK_ERROR (Tcb->Sk, EFI_TIMEOUT);
    }

    if (TCP_SYN_RCVD == Tcb->State) {
      DEBUG ((EFI_D_WARN, "TcpConnectTimeout: send reset because "
        "connection timer timeout for TCB %x\n", Tcb));

      TcpResetConnection (Tcb);

    }

    TcpClose (Tcb);
  }
}


/**
  Timeout handler for TCP retransmission timer.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

  @return None.

**/
STATIC
VOID
TcpRexmitTimeout (
  IN TCP_CB *Tcb
  )
{
  UINT32  FlightSize;

  DEBUG ((EFI_D_WARN, "TcpRexmitTimeout: transmission "
    "timeout for TCB %x\n", Tcb));

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
  if (Tcb->LossTimes > Tcb->MaxRexmit &&
      !TCP_TIMER_ON (Tcb->EnabledTimer, TCP_TIMER_CONNECT)) {

    DEBUG ((EFI_D_ERROR, "TcpRexmitTimeout: connection closed "
      "because too many timeouts for TCB %x\n", Tcb));

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

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

  @return None.

**/
STATIC
VOID
TcpProbeTimeout (
  IN TCP_CB *Tcb
  )
{
  //
  // This is the timer for sender's SWSA. RFC1122 requires
  // a timer set for sender's SWSA, and suggest combine it
  // with window probe timer. If data is sent, don't set
  // the probe timer, since retransmit timer is on.
  //
  if ((TcpDataToSend (Tcb, 1) != 0) && (TcpToSendData (Tcb, 1) > 0)) {

    ASSERT (TCP_TIMER_ON (Tcb->EnabledTimer, TCP_TIMER_REXMIT));
    return ;
  }

  TcpSendZeroProbe (Tcb);
  TcpSetProbeTimer (Tcb);
}


/**
  Timeout handler for keepalive timer.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

  @return None.

**/
STATIC
VOID
TcpKeepaliveTimeout (
  IN TCP_CB *Tcb
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

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

  @return None.

**/
STATIC
VOID
TcpFinwait2Timeout (
  IN TCP_CB *Tcb
  )
{
  DEBUG ((EFI_D_WARN, "TcpFinwait2Timeout: connection closed "
    "because FIN_WAIT2 timer timeouts for TCB %x\n", Tcb));

  TcpClose (Tcb);
}


/**
  Timeout handler for 2MSL timer.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

  @return None.

**/
STATIC
VOID
Tcp2MSLTimeout (
  IN TCP_CB *Tcb
  )
{
  DEBUG ((EFI_D_WARN, "Tcp2MSLTimeout: connection closed "
    "because TIME_WAIT timer timeouts for TCB %x\n", Tcb));

  TcpClose (Tcb);
}


/**
  Update the timer status and the next expire time
  according to the timers to expire in a specific
  future time slot.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

  @return None.

**/
STATIC
VOID
TcpUpdateTimer (
  IN TCP_CB *Tcb
  )
{
  UINT16  Index;

  //
  // Don't use a too large value to init NextExpire
  // since mTcpTick wraps around as sequence no does.
  //
  Tcb->NextExpire = 65535;
  TCP_CLEAR_FLG (Tcb->CtrlFlag, TCP_CTRL_TIMER_ON);

  for (Index = 0; Index < TCP_TIMER_NUMBER; Index++) {

    if (TCP_TIMER_ON (Tcb->EnabledTimer, Index) &&
        TCP_TIME_LT (Tcb->Timer[Index], mTcpTick + Tcb->NextExpire)) {

      Tcb->NextExpire = TCP_SUB_TIME (Tcb->Timer[Index], mTcpTick);
      TCP_SET_FLG (Tcb->CtrlFlag, TCP_CTRL_TIMER_ON);
    }
  }
}


/**
  Enable a TCP timer.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param  Timer    The index of the timer to be enabled.
  @param  TimeOut  The timeout value of this timer.

  @return None.

**/
VOID
TcpSetTimer (
  IN TCP_CB *Tcb,
  IN UINT16 Timer,
  IN UINT32 TimeOut
  )
{
  TCP_SET_TIMER (Tcb->EnabledTimer, Timer);
  Tcb->Timer[Timer] = mTcpTick + TimeOut;

  TcpUpdateTimer (Tcb);
}


/**
  Clear one TCP timer.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.
  @param  Timer    The index of the timer to be cleared.

  @return None.

**/
VOID
TcpClearTimer (
  IN TCP_CB *Tcb,
  IN UINT16 Timer
  )
{
  TCP_CLEAR_TIMER (Tcb->EnabledTimer, Timer);
  TcpUpdateTimer (Tcb);
}


/**
  Clear all TCP timers.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

  @return None.

**/
VOID
TcpClearAllTimer (
  IN TCP_CB *Tcb
  )
{
  Tcb->EnabledTimer = 0;
  TcpUpdateTimer (Tcb);
}


/**
  Enable the window prober timer and set the timeout value.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

  @return None.

**/
VOID
TcpSetProbeTimer (
  IN TCP_CB *Tcb
  )
{
  if (!TCP_TIMER_ON (Tcb->EnabledTimer, TCP_TIMER_PROBE)) {
    Tcb->ProbeTime = Tcb->Rto;

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

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

  @return None.

**/
VOID
TcpSetKeepaliveTimer (
  IN TCP_CB *Tcb
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
      (Tcb->Idle < Tcb->KeepAliveIdle)) {

    TcpSetTimer (Tcb, TCP_TIMER_KEEPALIVE, Tcb->KeepAliveIdle);
    Tcb->KeepAliveProbes = 0;

  } else {

    TcpSetTimer (Tcb, TCP_TIMER_KEEPALIVE, Tcb->KeepAlivePeriod);
  }
}


/**
  Backoff the RTO.

  @param  Tcb      Pointer to the TCP_CB of this TCP instance.

  @return None.

**/
VOID
TcpBackoffRto (
  IN TCP_CB *Tcb
  )
{
  //
  // Fold the RTT estimate if too many times, the estimate
  // may be wrong, fold it. So the next time a valid
  // measurement is sampled, we can start fresh.
  //
  if ((Tcb->LossTimes >= TCP_FOLD_RTT) && Tcb->SRtt) {
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
  Heart beat timer handler.

  @param  Context  Context of the timer event, ignored.

  @return None.

**/
VOID
EFIAPI
TcpTickingDpc (
  IN VOID      *Context
  )
{
  LIST_ENTRY      *Entry;
  LIST_ENTRY      *Next;
  TCP_CB          *Tcb;
  INT16           Index;

  mTcpTick++;
  mTcpGlobalIss += 100;

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

    if (Tcb->DelayedAck) {
      TcpSendAck (Tcb);
    }

    //
    // No timer is active or no timer expired
    //
    if (!TCP_FLG_ON (Tcb->CtrlFlag, TCP_CTRL_TIMER_ON) ||
        ((--Tcb->NextExpire) > 0)) {

      continue;
    }

    //
    // Call the timeout handler for each expired timer.
    //
    for (Index = 0; Index < TCP_TIMER_NUMBER; Index++) {

      if (TCP_TIMER_ON (Tcb->EnabledTimer, Index) &&
          TCP_TIME_LEQ (Tcb->Timer[Index], mTcpTick)) {
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
        if ((Next->BackLink != Entry) ||
            (Tcb->EnabledTimer == 0)) {

          goto NextConnection;
        }
      }
    }

    TcpUpdateTimer (Tcb);
NextConnection:
    ;
  }
}

/**
  Heart beat timer handler, queues the DPC at TPL_CALLBACK.

  @param  Event    Timer event signaled, ignored.
  @param  Context  Context of the timer event, ignored.

  @return None.

**/
VOID
EFIAPI
TcpTicking (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  NetLibQueueDpc (TPL_CALLBACK, TcpTickingDpc, Context);
}

