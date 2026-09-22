/** @file
  Host based unit tests for IScsiDhcp6.c.

  Copyright (c) 2026, 20000419. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>

#include <string>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseMemoryLib.h>
  #include "../IScsiImpl.h"
}

#if defined (_WIN32)
extern "C" {
  typedef UINT32 (__stdcall *PARSER_THREAD_START_ROUTINE)(VOID *Context);

  __declspec(dllimport)
  VOID *
  __stdcall
  CreateThread (
    VOID                         *ThreadAttributes,
    UINTN                        StackSize,
    PARSER_THREAD_START_ROUTINE  StartAddress,
    VOID                         *Parameter,
    UINT32                       CreationFlags,
    UINT32                       *ThreadId
    );

  __declspec(dllimport)
  UINT32
  __stdcall
  WaitForSingleObject (
    VOID    *Handle,
    UINT32  Milliseconds
    );

  __declspec(dllimport)
  INT32
  __stdcall
  TerminateThread (
    VOID    *Thread,
    UINT32  ExitCode
    );

  __declspec(dllimport)
  INT32
  __stdcall
  CloseHandle (
    VOID  *Object
    );
}
#elif defined (__linux__)
  #include <signal.h>
  #include <sys/wait.h>
  #include <unistd.h>
#endif

namespace {
constexpr UINT32  PARSER_WATCHDOG_MS = 3000;
 #if defined (_WIN32)
constexpr UINT32  WIN_WAIT_OBJECT_0 = 0;
 #endif

void
InitializeConfig (
  OUT ISCSI_ATTEMPT_CONFIG_NVDATA  *ConfigData
  )
{
  ZeroMem (ConfigData, sizeof (*ConfigData));
  ConfigData->SessionConfigData.IpMode = IP_MODE_AUTOCONFIG;
  ConfigData->AutoConfigureMode        = IP_MODE_IP6;
}

 #if defined (_WIN32)
typedef struct {
  std::string    RootPath;
  EFI_STATUS     Status;
} PARSER_CONTEXT;

UINT32
__stdcall
RunParser (
  IN VOID  *Context
  )
{
  PARSER_CONTEXT               *ParserContext;
  ISCSI_ATTEMPT_CONFIG_NVDATA  ConfigData;

  ParserContext = (PARSER_CONTEXT *)Context;
  InitializeConfig (&ConfigData);
  ParserContext->Status = IScsiDhcp6ExtractRootPath (
                            ParserContext->RootPath.data (),
                            (UINT16)ParserContext->RootPath.size (),
                            &ConfigData
                            );
  return 0;
}

 #endif

bool
RejectsWithinWatchdog (
  IN const std::string  &RootPath
  )
{
 #if defined (_WIN32)
  PARSER_CONTEXT  Context;
  VOID            *Thread;
  UINT32          WaitStatus;

  Context.RootPath = RootPath;
  Context.Status   = EFI_SUCCESS;
  Thread           = CreateThread (NULL, 0, RunParser, &Context, 0, NULL);
  if (Thread == NULL) {
    return false;
  }

  WaitStatus = WaitForSingleObject (Thread, PARSER_WATCHDOG_MS);
  if (WaitStatus != WIN_WAIT_OBJECT_0) {
    TerminateThread (Thread, 1);
    CloseHandle (Thread);
    return false;
  }

  CloseHandle (Thread);
  return Context.Status == EFI_INVALID_PARAMETER;
 #elif defined (__linux__)
  pid_t   ChildPid;
  int     ChildStatus;
  UINT32  ElapsedMs;

  ChildPid = fork ();
  if (ChildPid == -1) {
    return false;
  }

  if (ChildPid == 0) {
    ISCSI_ATTEMPT_CONFIG_NVDATA  ConfigData;
    EFI_STATUS                   Status;
    std::string                  MutableRootPath;

    MutableRootPath = RootPath;
    InitializeConfig (&ConfigData);
    Status = IScsiDhcp6ExtractRootPath (
               MutableRootPath.data (),
               (UINT16)MutableRootPath.size (),
               &ConfigData
               );
    _exit (Status == EFI_INVALID_PARAMETER ? 0 : 1);
  }

  ElapsedMs = 0;
  while (ElapsedMs < PARSER_WATCHDOG_MS) {
    pid_t  WaitResult;

    WaitResult = waitpid (ChildPid, &ChildStatus, WNOHANG);
    if (WaitResult == ChildPid) {
      return WIFEXITED (ChildStatus) && (WEXITSTATUS (ChildStatus) == 0);
    }

    if (WaitResult == -1) {
      return false;
    }

    usleep (10 * 1000);
    ElapsedMs += 10;
  }

  kill (ChildPid, SIGKILL);
  (VOID)waitpid (ChildPid, &ChildStatus, 0);
  return false;
 #else
  return false;
 #endif
}
} // namespace

extern "C" {
  EFI_STATUS
  IScsiAsciiStrToIp (
    IN  CHAR8           *Str,
    IN  UINT8           IpMode,
    OUT EFI_IP_ADDRESS  *Ip
    )
  {
    ZeroMem (Ip, sizeof (*Ip));
    return EFI_SUCCESS;
  }

  EFI_STATUS
  IScsiAsciiStrToLun (
    IN  CHAR8  *Str,
    OUT UINT8  *Lun
    )
  {
    ZeroMem (Lun, 8);
    return EFI_SUCCESS;
  }

  EFI_STATUS
  IScsiNormalizeName (
    IN OUT CHAR8  *Name,
    IN     UINTN  Len
    )
  {
    return EFI_SUCCESS;
  }
}

TEST (IScsiDhcp6ExtractRootPathTest, ParsesDnsRootPath) {
  ISCSI_ATTEMPT_CONFIG_NVDATA  ConfigData;
  CHAR8                        RootPath[] = "iscsi:target.example.com:6:3260:0:iqn.2026-08.com.example.target";

  InitializeConfig (&ConfigData);

  ASSERT_EQ (
    IScsiDhcp6ExtractRootPath (
      RootPath,
      (UINT16)(sizeof (RootPath) - 1),
      &ConfigData
      ),
    EFI_SUCCESS
    );
  EXPECT_TRUE (ConfigData.SessionConfigData.DnsMode);
  EXPECT_EQ (ConfigData.SessionConfigData.TargetPort, 3260);
  EXPECT_STREQ (ConfigData.SessionConfigData.TargetUrl, "target.example.com");
  EXPECT_STREQ (ConfigData.SessionConfigData.TargetName, "iqn.2026-08.com.example.target");
}

TEST (IScsiDhcp6ExtractRootPathTest, RejectsOverlongServerName) {
  std::string  RootPath = "iscsi:" + std::string (256, 'A') + ":0:3260:0:target";

  EXPECT_TRUE (RejectsWithinWatchdog (RootPath));
}
