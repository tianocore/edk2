## @file
# Create makefile for MS nmake and GNU make
#
# Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
from __future__ import absolute_import
import multiprocessing as mp
import threading
from Common.Misc import PathClass
from AutoGen.ModuleAutoGen import ModuleAutoGen
from AutoGen.ModuleAutoGenHelper import WorkSpaceInfo,AutoGenInfo
import Common.GlobalData as GlobalData
import Common.EdkLogger as EdkLogger
import os
from Common.MultipleWorkspace import MultipleWorkspace as mws
from AutoGen.AutoGen import AutoGen
from Workspace.WorkspaceDatabase import BuildDB
try:
    from queue import Empty
except:
    from Queue import Empty
import traceback
import sys
from AutoGen.DataPipe import MemoryDataPipe
import logging
import time

def clearQ(q):
    try:
        while True:
            q.get_nowait()
    except Empty:
        pass

class LogAgent(threading.Thread):
    def __init__(self,log_q,log_level,log_file=None):
        super(LogAgent,self).__init__()
        self.log_q = log_q
        self.log_level = log_level
        self.log_file = log_file
    def InitLogger(self):
        # For DEBUG level (All DEBUG_0~9 are applicable)
        self._DebugLogger_agent = logging.getLogger("tool_debug_agent")
        _DebugFormatter = logging.Formatter("[%(asctime)s.%(msecs)d]: %(message)s", datefmt="%H:%M:%S")
        self._DebugLogger_agent.setLevel(self.log_level)
        _DebugChannel = logging.StreamHandler(sys.stdout)
        _DebugChannel.setFormatter(_DebugFormatter)
        self._DebugLogger_agent.addHandler(_DebugChannel)

        # For VERBOSE, INFO, WARN level
        self._InfoLogger_agent = logging.getLogger("tool_info_agent")
        _InfoFormatter = logging.Formatter("%(message)s")
        self._InfoLogger_agent.setLevel(self.log_level)
        _InfoChannel = logging.StreamHandler(sys.stdout)
        _InfoChannel.setFormatter(_InfoFormatter)
        self._InfoLogger_agent.addHandler(_InfoChannel)

        # For ERROR level
        self._ErrorLogger_agent = logging.getLogger("tool_error_agent")
        _ErrorFormatter = logging.Formatter("%(message)s")
        self._ErrorLogger_agent.setLevel(self.log_level)
        _ErrorCh = logging.StreamHandler(sys.stderr)
        _ErrorCh.setFormatter(_ErrorFormatter)
        self._ErrorLogger_agent.addHandler(_ErrorCh)

        if self.log_file:
            if os.path.exists(self.log_file):
                os.remove(self.log_file)
            _Ch = logging.FileHandler(self.log_file)
            _Ch.setFormatter(_DebugFormatter)
            self._DebugLogger_agent.addHandler(_Ch)

            _Ch= logging.FileHandler(self.log_file)
            _Ch.setFormatter(_InfoFormatter)
            self._InfoLogger_agent.addHandler(_Ch)

            _Ch = logging.FileHandler(self.log_file)
            _Ch.setFormatter(_ErrorFormatter)
            self._ErrorLogger_agent.addHandler(_Ch)

    def run(self):
        self.InitLogger()
        while True:
            log_message = self.log_q.get()
            if log_message is None:
                break
            if log_message.name == "tool_error":
                self._ErrorLogger_agent.log(log_message.levelno,log_message.getMessage())
            elif log_message.name == "tool_info":
                self._InfoLogger_agent.log(log_message.levelno,log_message.getMessage())
            elif log_message.name == "tool_debug":
                self._DebugLogger_agent.log(log_message.levelno,log_message.getMessage())
            else:
                self._InfoLogger_agent.log(log_message.levelno,log_message.getMessage())

    def kill(self):
        self.log_q.put(None)
class AutoGenManager(threading.Thread):
    def __init__(self,autogen_workers, feedback_q,error_event):
        super(AutoGenManager,self).__init__()
        self.autogen_workers = autogen_workers
        self.feedback_q = feedback_q
        self.Status = True
        self.error_event = error_event
    def run(self):
        try:
            fin_num = 0
            while True:
                badnews = self.feedback_q.get()
                if badnews is None:
                    break
                if badnews == "Done":
                    fin_num += 1
                elif badnews == "QueueEmpty":
                    EdkLogger.debug(EdkLogger.DEBUG_9, "Worker %s: %s" % (os.getpid(), badnews))
                    self.TerminateWorkers()
                else:
                    EdkLogger.debug(EdkLogger.DEBUG_9, "Worker %s: %s" % (os.getpid(), badnews))
                    self.Status = False
                    self.TerminateWorkers()
                if fin_num == len(self.autogen_workers):
                    self.clearQueue()
                    for w in self.autogen_workers:
                        w.join()
                    break
        except Exception:
            return

    def clearQueue(self):
        taskq = self.autogen_workers[0].module_queue
        logq = self.autogen_workers[0].log_q
        clearQ(taskq)
        clearQ(self.feedback_q)
        clearQ(logq)
        # Copy the cache queue itmes to parent thread before clear
        cacheq = self.autogen_workers[0].cache_q
        try:
            cache_num = 0
            while True:
                item = cacheq.get()
                if item == "CacheDone":
                    cache_num += 1
                else:
                    GlobalData.gModuleAllCacheStatus.add(item)
                if cache_num  == len(self.autogen_workers):
                    break
        except:
            print ("cache_q error")

    def TerminateWorkers(self):
        self.error_event.set()
    def kill(self):
        self.feedback_q.put(None)
class AutoGenWorkerInProcess(mp.Process):
    def __init__(self,module_queue,data_pipe_file_path,feedback_q,file_lock,cache_q,log_q,error_event):
        mp.Process.__init__(self)
        self.module_queue = module_queue
        self.data_pipe_file_path =data_pipe_file_path
        self.data_pipe = None
        self.feedback_q = feedback_q
        self.PlatformMetaFileSet = {}
        self.file_lock = file_lock
        self.cache_q = cache_q
        self.log_q = log_q
        self.error_event = error_event
    def GetPlatformMetaFile(self,filepath,root):
        try:
            return self.PlatformMetaFileSet[(filepath,root)]
        except:
            self.PlatformMetaFileSet[(filepath,root)]  = filepath
            return self.PlatformMetaFileSet[(filepath,root)]
    def run(self):
        try:
            taskname = "Init"
            with self.file_lock:
                try:
                    self.data_pipe = MemoryDataPipe()
                    self.data_pipe.load(self.data_pipe_file_path)
                except:
                    self.feedback_q.put(taskname + ":" + "load data pipe %s failed." % self.data_pipe_file_path)
            EdkLogger.LogClientInitialize(self.log_q)
            loglevel = self.data_pipe.Get("LogLevel")
            if not loglevel:
                loglevel = EdkLogger.INFO
            EdkLogger.SetLevel(loglevel)
            target = self.data_pipe.Get("P_Info").get("Target")
            toolchain = self.data_pipe.Get("P_Info").get("ToolChain")
            archlist = self.data_pipe.Get("P_Info").get("ArchList")

            active_p = self.data_pipe.Get("P_Info").get("ActivePlatform")
            workspacedir = self.data_pipe.Get("P_Info").get("WorkspaceDir")
            PackagesPath = os.getenv("PACKAGES_PATH")
            mws.setWs(workspacedir, PackagesPath)
            self.Wa = WorkSpaceInfo(
                workspacedir,active_p,target,toolchain,archlist
                )
            self.Wa._SrcTimeStamp = self.data_pipe.Get("Workspace_timestamp")
            GlobalData.gGlobalDefines = self.data_pipe.Get("G_defines")
            GlobalData.gCommandLineDefines = self.data_pipe.Get("CL_defines")
            GlobalData.gCommandMaxLength = self.data_pipe.Get('gCommandMaxLength')
            os.environ._data = self.data_pipe.Get("Env_Var")
            GlobalData.gWorkspace = workspacedir
            GlobalData.gDisableIncludePathCheck = False
            GlobalData.gFdfParser = self.data_pipe.Get("FdfParser")
            GlobalData.gDatabasePath = self.data_pipe.Get("DatabasePath")

            GlobalData.gUseHashCache = self.data_pipe.Get("UseHashCache")
            GlobalData.gBinCacheSource = self.data_pipe.Get("BinCacheSource")
            GlobalData.gBinCacheDest = self.data_pipe.Get("BinCacheDest")
            GlobalData.gPlatformHashFile = self.data_pipe.Get("PlatformHashFile")
            GlobalData.gModulePreMakeCacheStatus = dict()
            GlobalData.gModuleMakeCacheStatus = dict()
            GlobalData.gHashChainStatus = dict()
            GlobalData.gCMakeHashFile = dict()
            GlobalData.gModuleHashFile = dict()
            GlobalData.gFileHashDict = dict()
            GlobalData.gEnableGenfdsMultiThread = self.data_pipe.Get("EnableGenfdsMultiThread")
            GlobalData.gPlatformFinalPcds = self.data_pipe.Get("gPlatformFinalPcds")
            GlobalData.file_lock = self.file_lock
            GlobalData.gLogLibraryMismatch = False
            CommandTarget = self.data_pipe.Get("CommandTarget")
            pcd_from_build_option = []
            for pcd_tuple in self.data_pipe.Get("BuildOptPcd"):
                pcd_id = ".".join((pcd_tuple[0],pcd_tuple[1]))
                if pcd_tuple[2].strip():
                    pcd_id = ".".join((pcd_id,pcd_tuple[2]))
                pcd_from_build_option.append("=".join((pcd_id,pcd_tuple[3])))
            GlobalData.BuildOptionPcd = pcd_from_build_option
            module_count = 0
            FfsCmd = self.data_pipe.Get("FfsCommand")
            if FfsCmd is None:
                FfsCmd = {}
            GlobalData.FfsCmd = FfsCmd
            PlatformMetaFile = self.GetPlatformMetaFile(self.data_pipe.Get("P_Info").get("ActivePlatform"),
                                             self.data_pipe.Get("P_Info").get("WorkspaceDir"))
            while True:
                if self.error_event.is_set():
                    break
                module_count += 1
                try:
                    module_file,module_root,module_path,module_basename,module_originalpath,module_arch,IsLib = self.module_queue.get_nowait()
                except Empty:
                    EdkLogger.debug(EdkLogger.DEBUG_9, "Worker %s: %s" % (os.getpid(), "Fake Empty."))
                    time.sleep(0.01)
                    continue
                if module_file is None:
                    EdkLogger.debug(EdkLogger.DEBUG_9, "Worker %s: %s" % (os.getpid(), "Worker get the last item in the queue."))
                    self.feedback_q.put("QueueEmpty")
                    time.sleep(0.01)
                    continue

                modulefullpath = os.path.join(module_root,module_file)
                taskname = " : ".join((modulefullpath,module_arch))
                module_metafile = PathClass(module_file,module_root)
                if module_path:
                    module_metafile.Path = module_path
                if module_basename:
                    module_metafile.BaseName = module_basename
                if module_originalpath:
                    module_metafile.OriginalPath = PathClass(module_originalpath,module_root)
                arch = module_arch
                target = self.data_pipe.Get("P_Info").get("Target")
                toolchain = self.data_pipe.Get("P_Info").get("ToolChain")
                Ma = ModuleAutoGen(self.Wa,module_metafile,target,toolchain,arch,PlatformMetaFile,self.data_pipe)
                Ma.IsLibrary = IsLib
                # SourceFileList calling sequence impact the makefile string sequence.
                # Create cached SourceFileList here to unify its calling sequence for both
                # CanSkipbyPreMakeCache and CreateCodeFile/CreateMakeFile.
                RetVal = Ma.SourceFileList
                if GlobalData.gUseHashCache and not GlobalData.gBinCacheDest and CommandTarget in [None, "", "all"]:
                    try:
                        CacheResult = Ma.CanSkipbyPreMakeCache()
                    except:
                        CacheResult = False
                        self.feedback_q.put(taskname)

                    if CacheResult:
                        self.cache_q.put((Ma.MetaFile.Path, Ma.Arch, "PreMakeCache", True))
                        continue
                    else:
                        self.cache_q.put((Ma.MetaFile.Path, Ma.Arch, "PreMakeCache", False))

                Ma.CreateCodeFile(False)
                Ma.CreateMakeFile(False,GenFfsList=FfsCmd.get((Ma.MetaFile.Path, Ma.Arch),[]))
                Ma.CreateAsBuiltInf()
                if GlobalData.gBinCacheSource and CommandTarget in [None, "", "all"]:
                    try:
                        CacheResult = Ma.CanSkipbyMakeCache()
                    except:
                        CacheResult = False
                        self.feedback_q.put(taskname)

                    if CacheResult:
                        self.cache_q.put((Ma.MetaFile.Path, Ma.Arch, "MakeCache", True))
                        continue
                    else:
                        self.cache_q.put((Ma.MetaFile.Path, Ma.Arch, "MakeCache", False))

        except Exception as e:
            EdkLogger.debug(EdkLogger.DEBUG_9, "Worker %s: %s" % (os.getpid(), str(e)))
            self.feedback_q.put(taskname)
        finally:
            EdkLogger.debug(EdkLogger.DEBUG_9, "Worker %s: %s" % (os.getpid(), "Done"))
            self.feedback_q.put("Done")
            self.cache_q.put("CacheDone")

    def printStatus(self):
        print("Processs ID: %d Run %d modules in AutoGen " % (os.getpid(),len(AutoGen.Cache())))
        print("Processs ID: %d Run %d modules in AutoGenInfo " % (os.getpid(),len(AutoGenInfo.GetCache())))
        groupobj = {}
        for buildobj in BuildDB.BuildObject.GetCache().values():
            if str(buildobj).lower().endswith("dec"):
                try:
                    groupobj['dec'].append(str(buildobj))
                except:
                    groupobj['dec'] = [str(buildobj)]
            if str(buildobj).lower().endswith("dsc"):
                try:
                    groupobj['dsc'].append(str(buildobj))
                except:
                    groupobj['dsc'] = [str(buildobj)]

            if str(buildobj).lower().endswith("inf"):
                try:
                    groupobj['inf'].append(str(buildobj))
                except:
                    groupobj['inf'] = [str(buildobj)]

        print("Processs ID: %d Run %d pkg in WDB " % (os.getpid(),len(groupobj.get("dec",[]))))
        print("Processs ID: %d Run %d pla in WDB " % (os.getpid(),len(groupobj.get("dsc",[]))))
        print("Processs ID: %d Run %d inf in WDB " % (os.getpid(),len(groupobj.get("inf",[]))))
