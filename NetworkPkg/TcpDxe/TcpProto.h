/** @file
  TCP protocol header file.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _TCP_PROTO_H_
#define _TCP_PROTO_H_

///
/// Tcp states don't change their order. It is used as an
/// index to mTcpOutFlag and other macros.
///
#define TCP_CLOSED       0
#define TCP_LISTEN       1
#define TCP_SYN_SENT     2
#define TCP_SYN_RCVD     3
#define TCP_ESTABLISHED  4
#define TCP_FIN_WAIT_1   5
#define TCP_FIN_WAIT_2   6
#define TCP_CLOSING      7
#define TCP_TIME_WAIT    8
#define TCP_CLOSE_WAIT   9
#define TCP_LAST_ACK     10


///
/// Flags in the TCP header
///
#define TCP_FLG_FIN      0x01
#define TCP_FLG_SYN      0x02
#define TCP_FLG_RST      0x04
#define TCP_FLG_PSH      0x08
#define TCP_FLG_ACK      0x10
#define TCP_FLG_URG      0x20

 //
 // mask for all the flags
 //
#define TCP_FLG_FLAG     0x3F


#define TCP_CONNECT_REFUSED      (-1) ///< TCP error status
#define TCP_CONNECT_RESET        (-2) ///< TCP error status
#define TCP_CONNECT_CLOSED       (-3) ///< TCP error status

//
// Current congestion status as suggested by RFC3782.
//
#define TCP_CONGEST_RECOVER      1  ///< During the NewReno fast recovery.
#define TCP_CONGEST_LOSS         2  ///< Retxmit because of retxmit time out.
#define TCP_CONGEST_OPEN         3  ///< TCP is opening its congestion window.

//
// TCP control flags
//
#define TCP_CTRL_NO_NAGLE        0x0001 ///< Disable Nagle algorithm
#define TCP_CTRL_NO_KEEPALIVE    0x0002 ///< Disable keepalive timer.
#define TCP_CTRL_NO_WS           0x0004 ///< Disable window scale option.
#define TCP_CTRL_RCVD_WS         0x0008 ///< Received a wnd scale option in syn.
#define TCP_CTRL_NO_TS           0x0010 ///< Disable Timestamp option.
#define TCP_CTRL_RCVD_TS         0x0020 ///< Received a Timestamp option in syn.
#define TCP_CTRL_SND_TS          0x0040 ///< Send Timestamp option to remote.
#define TCP_CTRL_SND_URG         0x0080 ///< In urgent send mode.
#define TCP_CTRL_RCVD_URG        0x0100 ///< In urgent receive mode.
#define TCP_CTRL_SND_PSH         0x0200 ///< In PUSH send mode.
#define TCP_CTRL_FIN_SENT        0x0400 ///< FIN is sent.
#define TCP_CTRL_FIN_ACKED       0x0800 ///< FIN is ACKed.
#define TCP_CTRL_TIMER_ON        0x1000 ///< At least one of the timer is on.
#define TCP_CTRL_RTT_ON          0x2000 ///< The RTT measurement is on.
#define TCP_CTRL_ACK_NOW         0x4000 ///< Send the ACK now, don't delay.

//
// Timer related values
//
#define TCP_TIMER_CONNECT        0                  ///< Connection establishment timer.
#define TCP_TIMER_REXMIT         1                  ///< Retransmit timer.
#define TCP_TIMER_PROBE          2                  ///< Window probe timer.
#define TCP_TIMER_KEEPALIVE      3                  ///< Keepalive timer.
#define TCP_TIMER_FINWAIT2       4                  ///< FIN_WAIT_2 timer.
#define TCP_TIMER_2MSL           5                  ///< TIME_WAIT timer.
#define TCP_TIMER_NUMBER         6                  ///< The total number of the TCP timer.
#define TCP_TICK                 200                ///< Every TCP tick is 200ms.
#define TCP_TICK_HZ              5                  ///< The frequence of TCP tick.
#define TCP_RTT_SHIFT            3                  ///< SRTT & RTTVAR scaled by 8.
#define TCP_RTO_MIN              TCP_TICK_HZ        ///< The minium value of RTO.
#define TCP_RTO_MAX              (TCP_TICK_HZ * 60) ///< The maxium value of RTO.
#define TCP_FOLD_RTT             4                  ///< Timeout threshod to fold RTT.

//
// Default values for some timers
//
#define TCP_MAX_LOSS             12                          ///< Default max times to retxmit.
#define TCP_KEEPALIVE_IDLE_MIN   (TCP_TICK_HZ * 60 * 60 * 2) ///< First keepalive.
#define TCP_KEEPALIVE_PERIOD     (TCP_TICK_HZ * 60)
#define TCP_MAX_KEEPALIVE        8
#define TCP_FIN_WAIT2_TIME       (2 * TCP_TICK_HZ)
#define TCP_TIME_WAIT_TIME       (2 * TCP_TICK_HZ)
#define TCP_PAWS_24DAY           (24 * 24 * 60 * 60 * TCP_TICK_HZ)
#define TCP_CONNECT_TIME         (75 * TCP_TICK_HZ)

//
// The header space to be reserved before TCP data to accomodate :
// 60byte IP head + 60byte TCP head + link layer head
//
#define TCP_MAX_HEAD             192

//
// Value ranges for some control option
//
#define TCP_RCV_BUF_SIZE         (2 * 1024 * 1024)
#define TCP_RCV_BUF_SIZE_MIN     (8 * 1024)
#define TCP_SND_BUF_SIZE         (2 * 1024 * 1024)
#define TCP_SND_BUF_SIZE_MIN     (8 * 1024)
#define TCP_BACKLOG              10
#define TCP_BACKLOG_MIN          5
#define TCP_MAX_LOSS_MIN         6
#define TCP_CONNECT_TIME_MIN     (60 * TCP_TICK_HZ)
#define TCP_MAX_KEEPALIVE_MIN    4
#define TCP_KEEPALIVE_IDLE_MAX   (TCP_TICK_HZ * 60 * 60 * 4)
#define TCP_KEEPALIVE_PERIOD_MIN (TCP_TICK_HZ * 30)
#define TCP_FIN_WAIT2_TIME_MAX   (4 * TCP_TICK_HZ)
#define TCP_TIME_WAIT_TIME_MAX   (60 * TCP_TICK_HZ)

///
/// TCP_CONNECTED: both ends have synchronized their ISN.
///
#define TCP_CONNECTED(state)     ((state) > TCP_SYN_RCVD)

#define TCP_FIN_RCVD(State) \
  ( \
    ((State) == TCP_CLOSE_WAIT) || \
    ((State) == TCP_LAST_ACK) || \
    ((State) == TCP_CLOSING) || \
    ((State) == TCP_TIME_WAIT) \
  )

#define TCP_LOCAL_CLOSED(State) \
  ( \
    ((State) == TCP_FIN_WAIT_1) || \
    ((State) == TCP_FIN_WAIT_2) || \
    ((State) == TCP_CLOSING) || \
    ((State) == TCP_TIME_WAIT) || \
    ((State) == TCP_LAST_ACK) \
  )

//
// Get the TCP_SEG point from a net buffer's ProtoData.
//
#define TCPSEG_NETBUF(NBuf)     ((TCP_SEG *) ((NBuf)->ProtoData))

//
// Macros to compare sequence no
//
#define TCP_SEQ_LT(SeqA, SeqB)  ((INT32) ((SeqA) - (SeqB)) < 0)
#define TCP_SEQ_LEQ(SeqA, SeqB) ((INT32) ((SeqA) - (SeqB)) <= 0)
#define TCP_SEQ_GT(SeqA, SeqB)  ((INT32) ((SeqB) - (SeqA)) < 0)
#define TCP_SEQ_GEQ(SeqA, SeqB) ((INT32) ((SeqB) - (SeqA)) <= 0)

//
// TCP_SEQ_BETWEEN return whether b <= m <= e
//
#define TCP_SEQ_BETWEEN(b, m, e)    ((e) - (b) >= (m) - (b))

//
// TCP_SUB_SEQ returns Seq1 - Seq2. Make sure Seq1 >= Seq2
//
#define TCP_SUB_SEQ(Seq1, Seq2)     ((UINT32) ((Seq1) - (Seq2)))

//
// Check whether Flag is on
//
#define TCP_FLG_ON(Value, Flag)     ((BOOLEAN) (((Value) & (Flag)) != 0))
//
// Set and Clear operation on a Flag
//
#define TCP_SET_FLG(Value, Flag)    ((Value) |= (Flag))
#define TCP_CLEAR_FLG(Value, Flag)  ((Value) &= ~(Flag))

//
// Test whether two peers are equal
//
#define TCP_PEER_EQUAL(Pa, Pb, Ver) \
  (((Pa)->Port == (Pb)->Port) && TcpIsIpEqual(&((Pa)->Ip), &((Pb)->Ip), Ver))

//
// Test whether Pa matches Pb, or Pa is more specific
// than pb. Zero means wildcard.
//
#define TCP_PEER_MATCH(Pa, Pb, Ver) \
  ( \
    (((Pb)->Port == 0) || ((Pb)->Port == (Pa)->Port)) && \
    (TcpIsIpZero (&((Pb)->Ip), Ver) || TcpIsIpEqual (&((Pb)->Ip), &((Pa)->Ip), Ver)) \
  )

#define TCP_TIMER_ON(Flag, Timer)     ((Flag) & (1 << (Timer)))
#define TCP_SET_TIMER(Flag, Timer)    ((Flag) = (UINT16) ((Flag) | (1 << (Timer))))
#define TCP_CLEAR_TIMER(Flag, Timer)  ((Flag) = (UINT16) ((Flag) & (~(1 << (Timer)))))


#define TCP_TIME_LT(Ta, Tb)           ((INT32) ((Ta) - (Tb)) < 0)
#define TCP_TIME_LEQ(Ta, Tb)          ((INT32) ((Ta) - (Tb)) <= 0)
#define TCP_SUB_TIME(Ta, Tb)          ((UINT32) ((Ta) - (Tb)))

#define TCP_MAX_WIN                   0xFFFFU

///
/// TCP segmentation data.
///
typedef struct _TCP_SEG {
  TCP_SEQNO Seq;  ///< Starting sequence number.
  TCP_SEQNO End;  ///< The sequence of the last byte + 1, include SYN/FIN. End-Seq = SEG.LEN.
  TCP_SEQNO Ack;  ///< ACK field in the segment.
  UINT8     Flag; ///< TCP header flags.
  UINT16    Urg;  ///< Valid if URG flag is set.
  UINT32    Wnd;  ///< TCP window size field.
} TCP_SEG;

///
/// Network endpoint, IP plus Port structure.
///
typedef struct _TCP_PEER {
  EFI_IP_ADDRESS  Ip;     ///< IP address, in network byte order.
  TCP_PORTNO      Port;   ///< Port number, in network byte order.
} TCP_PEER;

typedef struct _TCP_CONTROL_BLOCK  TCP_CB;

///
/// TCP control block: it includes various states.
///
struct _TCP_CONTROL_BLOCK {
  LIST_ENTRY        List;     ///< Back and forward link entry
  TCP_CB            *Parent;  ///< The parent TCP_CB structure

  SOCKET            *Sk;      ///< The socket it controled.
  TCP_PEER          LocalEnd; ///< Local endpoint.
  TCP_PEER          RemoteEnd;///< Remote endpoint.

  LIST_ENTRY        SndQue;   ///< Retxmission queue.
  LIST_ENTRY        RcvQue;   ///< Reassemble queue.
  UINT32            CtrlFlag; ///< Control flags, such as NO_NAGLE.
  INT32             Error;    ///< Soft error status, such as TCP_CONNECT_RESET.

  //
  // RFC793 and RFC1122 defined variables
  //
  UINT8             State;      ///< TCP state, such as SYN_SENT, LISTEN.
  UINT8             DelayedAck; ///< Number of delayed ACKs.
  UINT16            HeadSum;    ///< Checksum of the fixed parts of pesudo
                                ///< header: Src IP, Dst IP, 0, Protocol,
                                ///< do not include the TCP length.

  TCP_SEQNO         Iss;        ///< Initial Sending Sequence.
  TCP_SEQNO         SndUna;     ///< First unacknowledged data.
  TCP_SEQNO         SndNxt;     ///< Next data sequence to send.
  TCP_SEQNO         SndPsh;     ///< Send PUSH point.
  TCP_SEQNO         SndUp;      ///< Send urgent point.
  UINT32            SndWnd;     ///< Window advertised by the remote peer.
  UINT32            SndWndMax;  ///< Max send window advertised by the peer.
  TCP_SEQNO         SndWl1;     ///< Seq number used for last window update.
  TCP_SEQNO         SndWl2;     ///< Ack no of last window update.
  UINT16            SndMss;     ///< Max send segment size.
  TCP_SEQNO         RcvNxt;     ///< Next sequence no to receive.
  UINT32            RcvWnd;     ///< Window advertised by the local peer.
  TCP_SEQNO         RcvWl2;     ///< The RcvNxt (or ACK) of last window update.
                                ///< It is necessary because of delayed ACK.

  TCP_SEQNO         RcvUp;                   ///< Urgent point;
  TCP_SEQNO         Irs;                     ///< Initial Receiving Sequence.
  UINT16            RcvMss;                  ///< Max receive segment size.
  UINT16            EnabledTimer;            ///< Which timer is currently enabled.
  UINT32            Timer[TCP_TIMER_NUMBER]; ///< When the timer will expire.
  INT32             NextExpire;  ///< Countdown offset for the nearest timer.
  UINT32            Idle;        ///< How long the connection is in idle.
  UINT32            ProbeTime;   ///< The time out value for current window prober.
  BOOLEAN           ProbeTimerOn;///< If TRUE, the probe time is on.

  //
  // RFC1323 defined variables, about window scale,
  // timestamp and PAWS
  //
  UINT8             SndWndScale;  ///< Wndscale received from the peer.
  UINT8             RcvWndScale;  ///< Wndscale used to scale local buffer.
  UINT32            TsRecent;     ///< TsRecent to echo to the remote peer.
  UINT32            TsRecentAge;  ///< When this TsRecent is updated.

  //
  // RFC2988 defined variables. about RTT measurement
  //
  TCP_SEQNO         RttSeq;     ///< The seq of measured segment now.
  UINT32            RttMeasure; ///< Currently measured RTT in heartbeats.
  UINT32            SRtt;       ///< Smoothed RTT, scaled by 8.
  UINT32            RttVar;     ///< RTT variance, scaled by 8.
  UINT32            Rto;        ///< Current RTO, not scaled.

  //
  // RFC2581, and 3782 variables.
  // Congestion control + NewReno fast recovery.
  //
  UINT32            CWnd;         ///< Sender's congestion window.
  UINT32            Ssthresh;     ///< Slow start threshold.
  TCP_SEQNO         Recover;      ///< Recover point for NewReno.
  UINT16            DupAck;       ///< Number of duplicate ACKs.
  UINT8             CongestState; ///< The current congestion state(RFC3782).
  UINT8             LossTimes;    ///< Number of retxmit timeouts in a row.
  TCP_SEQNO         LossRecover;  ///< Recover point for retxmit.

  //
  // configuration parameters, for EFI_TCP4_PROTOCOL specification
  //
  UINT32            KeepAliveIdle;   ///< Idle time before sending first probe.
  UINT32            KeepAlivePeriod; ///< Interval for subsequent keep alive probe.
  UINT8             MaxKeepAlive;    ///< Maxium keep alive probe times.
  UINT8             KeepAliveProbes; ///< The number of keep alive probe.
  UINT16            MaxRexmit;       ///< The maxium number of retxmit before abort.
  UINT32            FinWait2Timeout; ///< The FIN_WAIT_2 timeout.
  UINT32            TimeWaitTimeout; ///< The TIME_WAIT timeout.
  UINT32            ConnectTimeout;  ///< The connect establishment timeout.

  //
  // configuration for tcp provided by user
  //
  BOOLEAN           UseDefaultAddr;
  UINT8             Tos;
  UINT8             Ttl;
  EFI_IPv4_ADDRESS  SubnetMask;


  BOOLEAN           RemoteIpZero;   ///< RemoteEnd.Ip is ZERO when configured.
  IP_IO_IP_INFO     *IpInfo;        ///< Pointer reference to Ip used to send pkt
  UINT32            Tick;           ///< 1 tick = 200ms
};

#endif
