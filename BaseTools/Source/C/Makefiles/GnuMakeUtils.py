#!/usr/bin/env python3
#
## @file GnuMakeUtils.py
#
#
# Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
from collections import namedtuple
import glob
import os
import re
import shutil
import subprocess
import sys
import traceback
if sys.platform == 'win32':
    from ctypes import windll, POINTER, byref, GetLastError, Structure, WinError
    from ctypes import c_void_p, c_ushort, c_int, c_long, c_ulong, c_wchar, sizeof

ARCH_UNKNOWN = 'Unknown'
ARCH_IA32 = 'IA32'
ARCH_X64 = 'X64'
ARCH_AARCH64 = 'AARCH64'
ARCH_RISCV64 = 'RISCV64'
ARCH_LOONGARCH64 = 'LOONGARCH64'
_Process = namedtuple('Process', ['process_id', 'parent_process_id', 'exe_filename'])

def _get_win32_process_architecture(pid):
    IMAGE_FILE_MACHINE_I386 = 0x014c
    IMAGE_FILE_MACHINE_AMD64 = 0x8664
    IMAGE_FILE_MACHINE_ARM64 = 0xAA64
    def _get_machine_type(machine_id):
        if machine_id == IMAGE_FILE_MACHINE_I386:
            return ARCH_IA32
        elif machine_id == IMAGE_FILE_MACHINE_AMD64:
            return ARCH_X64
        elif machine_id == IMAGE_FILE_MACHINE_ARM64:
            return ARCH_AARCH64
        return ARCH_UNKNOWN
    PROCESS_QUERY_LIMITED_INFORMATION = 0x1000
    kernel32 = windll.kernel32
    OpenProcess = kernel32.OpenProcess
    OpenProcess.argtypes = [c_ulong, c_int, c_ulong]
    OpenProcess.restype = c_void_p
    CloseHandle = kernel32.CloseHandle
    CloseHandle.argtypes = [c_void_p]
    CloseHandle.restype = c_int

    IsWow64Process2 = None
    IMAGE_FILE_MACHINE_UNKNOWN = 0
    try:
        #IsWow64Process2() is only available on Win10 TH2 or later
        IsWow64Process2 = kernel32.IsWow64Process2
    except AttributeError:
        IsWow64Process2 = None
    if IsWow64Process2 is not None:
        IsWow64Process2.argtypes = [c_void_p, POINTER(c_ushort), POINTER(c_ushort)]
        IsWow64Process2.restype = c_int
        ProcessMachine = c_ushort(1)
        NativeMachine = c_ushort(1)
        hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, pid)
        if hProcess == c_void_p(0):
            raise WinError(GetLastError())
        if IsWow64Process2(hProcess, byref(ProcessMachine), byref(NativeMachine)) != 0:
            CloseHandle(hProcess)
            if ProcessMachine.value == IMAGE_FILE_MACHINE_UNKNOWN:
                return _get_machine_type(NativeMachine.value)
            else:
                return _get_machine_type(ProcessMachine.value)
        else:
            CloseHandle(hProcess)
            raise WinError(GetLastError())
    else:
        #Graceful fallback for older OSes
        PROCESSOR_ARCHITECTURE_INTEL = 0
        PROCESSOR_ARCHITECTURE_AMD64 = 9
        class _SYSTEM_INFO(Structure):
            _fields_ = [('wProcessorArchitecture', c_ushort),
                        ('wReserved', c_ushort),
                        ('dwPageSize', c_ulong),
                        ('lpMinimumApplicationAddress', c_void_p),
                        ('lpMaximumApplicationAddress', c_void_p),
                        ('dwActiveProcessorMask', c_void_p),
                        ('dwNumberOfProcessors', c_ulong),
                        ('dwProcessorType', c_ulong),
                        ('dwAllocationGranularity', c_ulong),
                        ('wProcessorLevel', c_ushort),
                        ('wProcessorRevision', c_ushort)]
        GetNativeSystemInfo = kernel32.GetNativeSystemInfo
        GetNativeSystemInfo.argtypes = [POINTER(_SYSTEM_INFO)]
        systemInfo = _SYSTEM_INFO()
        GetNativeSystemInfo(byref(systemInfo))
        if systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64:
            hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, pid)
            if hProcess == c_void_p(0):
                raise WinError(GetLastError())
            IsWow64Process = kernel32.IsWow64Process
            IsWow64Process.argtypes = [c_void_p, POINTER(c_int)]
            IsWow64Process.restype = c_int
            is_wow64 = c_int(0)
            if IsWow64Process(hProcess, byref(is_wow64)) != 0:
                CloseHandle(hProcess)
                if is_wow64.value != 0:
                    return ARCH_IA32
                else:
                    return ARCH_X64
            else:
                CloseHandle(hProcess)
                raise WinError(GetLastError())
        elif systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL:
            return ARCH_IA32
        return ARCH_UNKNOWN

def _get_win32_process_list():
    class _PROCESSENTRY32W(Structure):
        _fields_ = [('dwSize', c_ulong),
                    ('cntUsage', c_ulong),
                    ('th32ProcessID', c_ulong),
                    ('th32DefaultHeapID', c_void_p),
                    ('th32ModuleID', c_ulong),
                    ('cntThreads', c_ulong),
                    ('th32ParentProcessID', c_ulong),
                    ('pcPriClassBase', c_long),
                    ('dwFlags', c_ulong),
                    ('szExeFile', (c_wchar * 260))]
    INVALID_HANDLE_VALUE = c_void_p(-1)
    TH32CS_SNAPPROCESS = 2
    ERROR_NO_MORE_FILES = 18
    kernel32 = windll.kernel32
    CreateToolhelp32Snapshot = kernel32.CreateToolhelp32Snapshot
    CreateToolhelp32Snapshot.argtypes = [c_ulong, c_ulong]
    CreateToolhelp32Snapshot.restype = c_void_p
    Process32First = kernel32.Process32FirstW
    Process32First.argtypes = [c_void_p, POINTER(_PROCESSENTRY32W)]
    Process32First.restype = c_int
    Process32Next = kernel32.Process32NextW
    Process32Next.argtypes = [c_void_p, POINTER(_PROCESSENTRY32W)]
    Process32Next.restype = c_int
    CloseHandle = kernel32.CloseHandle
    CloseHandle.argtypes = [c_void_p]
    CloseHandle.restype = c_int

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    if hSnapshot == INVALID_HANDLE_VALUE.value:
        raise WinError(GetLastError())
    process_list = []
    processEntry = _PROCESSENTRY32W()
    processEntry.dwSize = sizeof(processEntry)
    more_processes = True
    if Process32First(hSnapshot, byref(processEntry)) == 0:
        raise WinError(GetLastError())
    while more_processes:
        process_list.append(_Process(processEntry.th32ProcessID, processEntry.th32ParentProcessID, processEntry.szExeFile))
        if Process32Next(hSnapshot, byref(processEntry)) == 0:
            status = GetLastError()
            if status == ERROR_NO_MORE_FILES:
                more_processes = False
            else:
                raise WinError(status)
    CloseHandle(hSnapshot)
    return process_list

def _get_win32_parent_processes():
    kernel32 = windll.kernel32
    GetCurrentProcessId = kernel32.GetCurrentProcessId
    GetCurrentProcessId.argtypes = []
    GetCurrentProcessId.restype = c_ulong

    process_list = _get_win32_process_list()
    pid = GetCurrentProcessId()
    parent_processes = []
    found_parent = True
    while found_parent:
        found_parent = False
        for process in process_list:
            if process.process_id == pid:
                found_parent = True
                parent_processes.append(process)
                pid = process.parent_process_id
                break
    return parent_processes

def _get_mingw_target_architecture():
    parent_processes = _get_win32_parent_processes()
    for process in parent_processes:
        if 'make' in process.exe_filename.lower():
            return _get_win32_process_architecture(process.process_id)
    return ARCH_UNKNOWN

def get_host_arch():
    if sys.platform == 'win32':
        host_arch = _get_mingw_target_architecture()
    else:
        result = subprocess.run('uname -m', universal_newlines=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True, check=True)
        uname_m = result.stdout.strip()
        ia32_regex = re.compile(r".*i[8]?[0-9]86.*")
        ia32_match = ia32_regex.match(uname_m)
        if 'x86_64' in uname_m or 'amd64' in uname_m:
            host_arch = ARCH_X64
        elif ia32_match:
            host_arch = ARCH_IA32
        elif 'aarch64' in uname_m or 'arm64' in uname_m:
            host_arch = ARCH_AARCH64
        elif 'riscv64' in uname_m:
            host_arch = ARCH_RISCV64
        elif 'loongarch64' in uname_m:
            host_arch = ARCH_LOONGARCH64
    print(host_arch)
    return 0

def main():
    if sys.argv[1] == 'get_host_arch':
        return get_host_arch()
    elif sys.argv[1] == 'cp':
        shutil.copy(os.path.normpath(sys.argv[2]), os.path.normpath(sys.argv[3]))
    elif sys.argv[1] == 'mv':
        shutil.move(os.path.normpath(sys.argv[2]), os.path.normpath(sys.argv[3]))
    elif sys.argv[1] == 'rm':
        paths = [os.path.normpath(x) for x in sys.argv[2:]]
        files = []
        for path in paths:
            if '*' in path:
                files.extend(glob.glob(path))
            else:
                files.append(path)
        for file in files:
            if os.path.exists(file):
                if os.path.isfile(file):
                    os.remove(file)
                else:
                    sys.stderr.writelines(['{} is not a file.\n'.format(file)])
            else:
                sys.stderr.writelines(['File {} does not exist.\n'.format(file)])
    elif sys.argv[1] == 'md':
        path = os.path.normpath(sys.argv[2])
        if not os.path.exists(path):
            os.makedirs(path)
        else:
            if os.path.isdir(path):
                sys.stderr.writelines(['Directory {} already exists.\n'.format(path)])
            else:
                sys.stderr.writelines(['{} is a file.\n'.format(path)])
                return 1
    elif sys.argv[1] == 'rd':
        paths = [os.path.normpath(x) for x in sys.argv[2:]]
        for path in paths:
            if os.path.exists(path):
                if os.path.isdir(path):
                    shutil.rmtree(path)
                else:
                    sys.stderr.writelines(['{} is not a directory.\n'.format(path)])
            else:
                sys.stderr.writelines(['Directory {} does not exist.\n'.format(path)])
    elif sys.argv[1] == 'rm_pyc_files':
        path = os.path.normpath(sys.argv[2])
        files = glob.glob(os.path.join(path, '*.pyc'))
        for file in files:
            if os.path.exists(file):
                if os.path.isfile(file):
                    os.remove(file)
                else:
                    sys.stderr.writelines(['{} is not a file.\n'.format(file)])
            else:
                sys.stderr.writelines(['File {} does not exist.\n'.format(file)])
        py_cache = os.path.join(path, '__pycache__')
        if os.path.isdir(py_cache):
            shutil.rmtree(py_cache)
    else:
        sys.stderr.writelines(['Unsupported command.\n'])
        return 1
    return 0

if __name__ == '__main__':
    try:
        sys.exit(main())
    except Exception as e:
        traceback.print_exc()
        sys.exit(1)
