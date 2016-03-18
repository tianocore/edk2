/** @file 
  setitimer and getitimer functions.

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include <LibConfig.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <sys/signal.h>
#include <signal.h>
#include  <unistd.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiLib.h>

STATIC EFI_EVENT RealTimer    = NULL;
STATIC EFI_EVENT VirtualTimer = NULL;
STATIC EFI_EVENT ProfTimer    = NULL;

STATIC struct itimerval RealTimerInfo    = {{0,0},{0,0}};
STATIC struct itimerval VirtualTimerInfo = {{0,0},{0,0}};
STATIC struct itimerval ProfTimerInfo    = {{0,0},{0,0}};

/**
  Function to queue the next iteration of the timer.

  This will copy the interval part of the struct into the value and (if 
  non-zero), then queue the next timer event.

  @param[in] TimerInfo  The timer info structure.  
  @param[in] Event      The EFI timer event.
**/
VOID
EFIAPI
SetNext (
  IN struct itimerval *TimerInfo,
  IN EFI_EVENT        Event
  )
{
  EFI_STATUS Status;

  CopyMem(&(TimerInfo->it_value), &(TimerInfo->it_interval), sizeof(struct timeval));

  //
  // If now zero then close and be done.
  //
  if (TimerInfo->it_value.tv_sec+TimerInfo->it_value.tv_usec == 0) {
    if (Event != NULL) {
      gBS->CloseEvent(Event);
      Event = NULL;
    }
    return;
  }

  //
  // Set up for the next loop.
  //
  Status = gBS->SetTimer (
    Event,
    TimerRelative,
    TimerInfo->it_value.tv_sec*10000000+TimerInfo->it_value.tv_usec*1000);

  if (EFI_ERROR(Status)) {
    gBS->CloseEvent(Event);
    Event = NULL;
  }
}

/**
  Notification function for real timer.

  @param[in] Event    The event.
  @param[in] Context  Ignored.
**/
VOID
EFIAPI
iTimerRealNotifyFunction (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  raise(SIGALRM);
  SetNext(&RealTimerInfo, RealTimer);
}

/**
  Notification function for virtual timer.

  @param[in] Event    The event.
  @param[in] Context  Ignored.
**/
VOID
EFIAPI
iTimerVirtualNotifyFunction (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  raise(SIGVTALRM);
  SetNext(&VirtualTimerInfo, VirtualTimer);
}

/**
  Notification function for prof timer.

  @param[in] Event    The event.
  @param[in] Context  Ignored.
**/
VOID
EFIAPI
iTimerProfNotifyFunction (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  raise(SIGPROF);
  SetNext(&ProfTimerInfo, ProfTimer);
}

/**
  The setitimer() function sets the timer specified by which to the value 
  specified in the structure pointed to by value, and if ovalue is not a null 
  pointer, stores the previous value of the timer in the structure pointed to
  by ovalue. 

  A timer value is defined by the itimerval structure. If it_value is non-zero,
  it indicates the time to the next timer expiration. If it_interval is 
  non-zero, it specifies a value to be used in reloading it_value when the 
  timer expires. Setting it_value to 0 disables a timer, regardless of the 
  value of it_interval. Setting it_interval to 0 disables a timer after its 
  next expiration (assuming it_value is non-zero). 

  ITIMER_REAL
  Decrements in real time. A SIGALRM signal is delivered when this timer 
  expires. 
  
  ITIMER_VIRTUAL
  Decrements in process virtual time. It runs only when the process is 
  executing. A SIGVTALRM signal is delivered when it expires. 

  ITIMER_PROF
  Decrements both in process virtual time and when the system is running on 
  behalf of the process. It is designed to be used by interpreters in 
  statistically profiling the execution of interpreted programs. Each time 
  the ITIMER_PROF timer expires, the SIGPROF signal is delivered. 

  @param[in] which      Which timer to set.  Possible values are described above.
  @param[in] value      The new value for this timer.
  @param[out] ovalue    The old value for this timer.

  @retval 0 The operation was successful.
  @retval -1 The operation failed. see errno for more details.
**/

int setitimer(
  int which, 
  const struct itimerval *value,
  struct itimerval *ovalue
  )
{
  EFI_EVENT         *EventPointer;
  EFI_EVENT_NOTIFY  NotifyFunction;
  EFI_STATUS        Status;

  if (value == NULL) {
    errno = EINVAL;
    return (-1);
  }

  if (which == ITIMER_REAL) {
    EventPointer    = &RealTimer;
    NotifyFunction  = iTimerRealNotifyFunction;
    if (ovalue != NULL) {
      CopyMem(ovalue, &RealTimerInfo, sizeof(struct itimerval));
    }
    CopyMem(&RealTimerInfo, value, sizeof(struct itimerval));
  } else if (which == ITIMER_VIRTUAL) {
    EventPointer    = &VirtualTimer;
    NotifyFunction  = iTimerVirtualNotifyFunction;
    if (ovalue != NULL) {
      CopyMem(ovalue, &VirtualTimerInfo, sizeof(struct itimerval));
    }
    CopyMem(&VirtualTimerInfo, value, sizeof(struct itimerval));
  } else if (which == ITIMER_PROF) {
    EventPointer    = &ProfTimer;
    NotifyFunction  = iTimerProfNotifyFunction;
    if (ovalue != NULL) {
      CopyMem(ovalue, &ProfTimerInfo, sizeof(struct itimerval));
    }
    CopyMem(&ProfTimerInfo, value, sizeof(struct itimerval));
  } else {
    errno = EINVAL;
    return (-1);
  }

  if (*EventPointer != NULL) {
    gBS->CloseEvent(*EventPointer);
    *EventPointer = NULL;
  }

  //
  // This was a 'please cancel me' request.
  //
  if (value->it_value.tv_sec+value->it_value.tv_usec == 0) {
    return 0;
  }

  Status = gBS->CreateEvent (
    EVT_TIMER|EVT_NOTIFY_SIGNAL,
    EfiGetCurrentTpl(),
    NotifyFunction,
    NULL, // no context
    EventPointer);

  if (EFI_ERROR(Status)) {
    errno = EINVAL;
    return (-1);
  }

  Status = gBS->SetTimer (
    *EventPointer,
    TimerRelative,
    value->it_value.tv_sec*10000000+value->it_value.tv_usec*1000);

  if (EFI_ERROR(Status)) {
    gBS->CloseEvent(*EventPointer);
    *EventPointer = NULL;
    errno = EINVAL;
    return (-1);
  }

  return 0;
}

/**
  Function to get the current state of a timer.

  @param[in] which    The identifier of the timer to get.  See setitimer for 
                      details.
  @param[in] value    The pointer to populate.  must be pre-allocated to size.

  @return 0           The operation was successful.
  @return -1          The operation failed.  
                      This means that value or which had an invalid value.
**/
int getitimer(
  int which, 
  struct itimerval *value
  )
{

  if (value == NULL) {
    errno = EINVAL;
    return (-1);
  }

  if (which == ITIMER_REAL) {
      CopyMem(value, &RealTimerInfo, sizeof(struct itimerval));
  } else if (which == ITIMER_VIRTUAL) {
      CopyMem(value, &VirtualTimerInfo, sizeof(struct itimerval));
  } else if (which == ITIMER_PROF) {
      CopyMem(value, &ProfTimerInfo, sizeof(struct itimerval));
  } else {
    errno = EINVAL;
    return (-1);
  }

  return 0;
}
