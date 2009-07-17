## @file
# build a platform or a module
#
#  Copyright (c) 2007, Intel Corporation
#
#  All rights reserved. This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import os
import re
import sys
import glob
import time
import platform
import traceback

from threading import *
from optparse import OptionParser
from subprocess import *
from Common import Misc as Utils

from Common.TargetTxtClassObject import *
from Common.ToolDefClassObject import *
from Common.DataType import *
from AutoGen.AutoGen import *
from Common.BuildToolError import *
from Workspace.WorkspaceDatabase import *

import Common.EdkLogger
import Common.GlobalData as GlobalData

# Version and Copyright
VersionNumber = "0.5"
__version__ = "%prog Version " + VersionNumber
__copyright__ = "Copyright (c) 2007, Intel Corporation  All rights reserved."

## standard targets of build command
gSupportedTarget = ['all', 'genc', 'genmake', 'modules', 'libraries', 'fds', 'clean', 'cleanall', 'cleanlib', 'run']

## build configuration file
gBuildConfiguration = "Conf/target.txt"
gBuildCacheDir = "Conf/.cache"
gToolsDefinition = "Conf/tools_def.txt"

## Check environment PATH variable to make sure the specified tool is found
#
#   If the tool is found in the PATH, then True is returned
#   Otherwise, False is returned
#
def IsToolInPath(tool):
    if os.environ.has_key('PATHEXT'):
        extns = os.environ['PATHEXT'].split(os.path.pathsep)
    else:
        extns = ('',)
    for pathDir in os.environ['PATH'].split(os.path.pathsep):
        for ext in extns:
            if os.path.exists(os.path.join(pathDir, tool + ext)):
                return True
    return False

## Check environment variables
#
#  Check environment variables that must be set for build. Currently they are
#
#   WORKSPACE           The directory all packages/platforms start from
#   EDK_TOOLS_PATH      The directory contains all tools needed by the build
#   PATH                $(EDK_TOOLS_PATH)/Bin/<sys> must be set in PATH
#
#   If any of above environment variable is not set or has error, the build
#   will be broken.
#
def CheckEnvVariable():
    # check WORKSPACE
    if "WORKSPACE" not in os.environ:
        EdkLogger.error("build", ATTRIBUTE_NOT_AVAILABLE, "Environment variable not found",
                        ExtraData="WORKSPACE")

    WorkspaceDir = os.path.normcase(os.path.normpath(os.environ["WORKSPACE"]))
    if not os.path.exists(WorkspaceDir):
        EdkLogger.error("build", FILE_NOT_FOUND, "WORKSPACE doesn't exist", ExtraData="%s" % WorkspaceDir)
    elif ' ' in WorkspaceDir:
        EdkLogger.error("build", FORMAT_NOT_SUPPORTED, "No space is allowed in WORKSPACE path",
                        ExtraData=WorkspaceDir)
    os.environ["WORKSPACE"] = WorkspaceDir

    #
    # Check EFI_SOURCE (R8 build convention). EDK_SOURCE will always point to ECP
    #
    os.environ["ECP_SOURCE"] = os.path.join(WorkspaceDir, GlobalData.gEdkCompatibilityPkg)
    if "EFI_SOURCE" not in os.environ:
        os.environ["EFI_SOURCE"] = os.environ["ECP_SOURCE"]
    if "EDK_SOURCE" not in os.environ:
        os.environ["EDK_SOURCE"] = os.environ["ECP_SOURCE"]

    #
    # Unify case of characters on case-insensitive systems
    #
    EfiSourceDir = os.path.normcase(os.path.normpath(os.environ["EFI_SOURCE"]))
    EdkSourceDir = os.path.normcase(os.path.normpath(os.environ["EDK_SOURCE"]))
    EcpSourceDir = os.path.normcase(os.path.normpath(os.environ["ECP_SOURCE"]))
 
    os.environ["EFI_SOURCE"] = EfiSourceDir
    os.environ["EDK_SOURCE"] = EdkSourceDir
    os.environ["ECP_SOURCE"] = EcpSourceDir
    os.environ["EDK_TOOLS_PATH"] = os.path.normcase(os.environ["EDK_TOOLS_PATH"])
    
    if not os.path.exists(EcpSourceDir):
        EdkLogger.verbose("ECP_SOURCE = %s doesn't exist. R8 modules could not be built." % EcpSourceDir)
    elif ' ' in EcpSourceDir:
        EdkLogger.error("build", FORMAT_NOT_SUPPORTED, "No space is allowed in ECP_SOURCE path",
                        ExtraData=EcpSourceDir)
    if not os.path.exists(EdkSourceDir):
        if EdkSourceDir == EcpSourceDir:
            EdkLogger.verbose("EDK_SOURCE = %s doesn't exist. R8 modules could not be built." % EdkSourceDir)
        else:
            EdkLogger.error("build", PARAMETER_INVALID, "EDK_SOURCE does not exist",
                            ExtraData=EdkSourceDir)
    elif ' ' in EdkSourceDir:
        EdkLogger.error("build", FORMAT_NOT_SUPPORTED, "No space is allowed in EDK_SOURCE path",
                        ExtraData=EdkSourceDir)
    if not os.path.exists(EfiSourceDir):
        if EfiSourceDir == EcpSourceDir:
            EdkLogger.verbose("EFI_SOURCE = %s doesn't exist. R8 modules could not be built." % EfiSourceDir)
        else:
            EdkLogger.error("build", PARAMETER_INVALID, "EFI_SOURCE does not exist",
                            ExtraData=EfiSourceDir)
    elif ' ' in EfiSourceDir:
        EdkLogger.error("build", FORMAT_NOT_SUPPORTED, "No space is allowed in EFI_SOURCE path",
                        ExtraData=EfiSourceDir)

    # change absolute path to relative path to WORKSPACE
    if EfiSourceDir.upper().find(WorkspaceDir.upper()) != 0:
        EdkLogger.error("build", PARAMETER_INVALID, "EFI_SOURCE is not under WORKSPACE",
                        ExtraData="WORKSPACE = %s\n    EFI_SOURCE = %s" % (WorkspaceDir, EfiSourceDir))
    if EdkSourceDir.upper().find(WorkspaceDir.upper()) != 0:
        EdkLogger.error("build", PARAMETER_INVALID, "EDK_SOURCE is not under WORKSPACE",
                        ExtraData="WORKSPACE = %s\n    EDK_SOURCE = %s" % (WorkspaceDir, EdkSourceDir))
    if EcpSourceDir.upper().find(WorkspaceDir.upper()) != 0:
        EdkLogger.error("build", PARAMETER_INVALID, "ECP_SOURCE is not under WORKSPACE",
                        ExtraData="WORKSPACE = %s\n    ECP_SOURCE = %s" % (WorkspaceDir, EcpSourceDir))

    # check EDK_TOOLS_PATH
    if "EDK_TOOLS_PATH" not in os.environ:
        EdkLogger.error("build", ATTRIBUTE_NOT_AVAILABLE, "Environment variable not found",
                        ExtraData="EDK_TOOLS_PATH")

    # check PATH
    if "PATH" not in os.environ:
        EdkLogger.error("build", ATTRIBUTE_NOT_AVAILABLE, "Environment variable not found",
                        ExtraData="PATH")

    # for macro replacement in R9 DSC/DEC/INF file
    GlobalData.gGlobalDefines["WORKSPACE"] = ""

    # for macro replacement in R8 INF file
    GlobalData.gGlobalDefines["EFI_SOURCE"] = EfiSourceDir
    GlobalData.gGlobalDefines["EDK_SOURCE"] = EdkSourceDir

    GlobalData.gWorkspace = WorkspaceDir
    GlobalData.gEfiSource = EfiSourceDir
    GlobalData.gEdkSource = EdkSourceDir
    GlobalData.gEcpSource = EcpSourceDir

## Get normalized file path
#
# Convert the path to be local format, and remove the WORKSPACE path at the
# beginning if the file path is given in full path.
#
# @param  FilePath      File path to be normalized
# @param  Workspace     Workspace path which the FilePath will be checked against
#
# @retval string        The normalized file path
#
def NormFile(FilePath, Workspace):
    # check if the path is absolute or relative
    if os.path.isabs(FilePath):
        FileFullPath = os.path.normpath(FilePath)
    else:
        FileFullPath = os.path.normpath(os.path.join(Workspace, FilePath))

    # check if the file path exists or not
    if not os.path.isfile(FileFullPath):
        EdkLogger.error("build", FILE_NOT_FOUND, ExtraData="\t%s (Please give file in absolute path or relative to WORKSPACE)"  % FileFullPath)

    # remove workspace directory from the beginning part of the file path
    if Workspace[-1] in ["\\", "/"]:
        return FileFullPath[len(Workspace):]
    else:
        return FileFullPath[(len(Workspace) + 1):]

## Get the output of an external program
#
# This is the entrance method of thread reading output of an external program and
# putting them in STDOUT/STDERR of current program.
#
# @param  From      The stream message read from
# @param  To        The stream message put on
# @param  ExitFlag  The flag used to indicate stopping reading
#
def ReadMessage(From, To, ExitFlag):
    while True:
        # read one line a time
        Line = From.readline()
        # empty string means "end"
        if Line != None and Line != "":
            To(Line.rstrip())
        else:
            break
        if ExitFlag.isSet():
            break

## Launch an external program
#
# This method will call subprocess.Popen to execute an external program with
# given options in specified directory. Because of the dead-lock issue during
# redirecting output of the external program, threads are used to to do the
# redirection work.
#
# @param  Command               A list or string containing the call of the program
# @param  WorkingDir            The directory in which the program will be running
#
def LaunchCommand(Command, WorkingDir):
    # if working directory doesn't exist, Popen() will raise an exception
    if not os.path.isdir(WorkingDir):
        EdkLogger.error("build", FILE_NOT_FOUND, ExtraData=WorkingDir)

    Proc = None
    EndOfProcedure = None
    try:
        # launch the command
        Proc = Popen(Command, stdout=PIPE, stderr=PIPE, env=os.environ, cwd=WorkingDir, bufsize=-1)

        # launch two threads to read the STDOUT and STDERR
        EndOfProcedure = Event()
        EndOfProcedure.clear()
        if Proc.stdout:
            StdOutThread = Thread(target=ReadMessage, args=(Proc.stdout, EdkLogger.info, EndOfProcedure))
            StdOutThread.setName("STDOUT-Redirector")
            StdOutThread.setDaemon(False)
            StdOutThread.start()

        if Proc.stderr:
            StdErrThread = Thread(target=ReadMessage, args=(Proc.stderr, EdkLogger.quiet, EndOfProcedure))
            StdErrThread.setName("STDERR-Redirector")
            StdErrThread.setDaemon(False)
            StdErrThread.start()

        # waiting for program exit
        Proc.wait()
    except: # in case of aborting
        # terminate the threads redirecting the program output
        if EndOfProcedure != None:
            EndOfProcedure.set()
        if Proc == None:
            if type(Command) != type(""):
                Command = " ".join(Command)
            EdkLogger.error("build", COMMAND_FAILURE, "Failed to start command", ExtraData="%s [%s]" % (Command, WorkingDir))

    if Proc.stdout:
        StdOutThread.join()
    if Proc.stderr:
        StdErrThread.join()

    # check the return code of the program
    if Proc.returncode != 0:
        if type(Command) != type(""):
            Command = " ".join(Command)
        EdkLogger.error("build", COMMAND_FAILURE, ExtraData="%s [%s]" % (Command, WorkingDir))

## The smallest unit that can be built in multi-thread build mode
#
# This is the base class of build unit. The "Obj" parameter must provide
# __str__(), __eq__() and __hash__() methods. Otherwise there could be build units
# missing build.
#
# Currently the "Obj" should be only ModuleAutoGen or PlatformAutoGen objects.
#
class BuildUnit:
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  Obj         The object the build is working on
    #   @param  Target      The build target name, one of gSupportedTarget
    #   @param  Dependency  The BuildUnit(s) which must be completed in advance
    #   @param  WorkingDir  The directory build command starts in
    #
    def __init__(self, Obj, BuildCommand, Target, Dependency, WorkingDir="."):
        self.BuildObject = Obj
        self.Dependency = Dependency
        self.WorkingDir = WorkingDir
        self.Target = Target
        self.BuildCommand = BuildCommand
        if BuildCommand == None or len(BuildCommand) == 0:
            EdkLogger.error("build", OPTION_MISSING, "No build command found for",
                            ExtraData=str(Obj))

    ## str() method
    #
    #   It just returns the string representaion of self.BuildObject
    #
    #   @param  self        The object pointer
    #
    def __str__(self):
        return str(self.BuildObject)

    ## "==" operator method
    #
    #   It just compares self.BuildObject with "Other". So self.BuildObject must
    #   provide its own __eq__() method.
    #
    #   @param  self        The object pointer
    #   @param  Other       The other BuildUnit object compared to
    #
    def __eq__(self, Other):
        return Other != None and self.BuildObject == Other.BuildObject \
                and self.BuildObject.Arch == Other.BuildObject.Arch

    ## hash() method
    #
    #   It just returns the hash value of self.BuildObject which must be hashable.
    #
    #   @param  self        The object pointer
    #
    def __hash__(self):
        return hash(self.BuildObject) + hash(self.BuildObject.Arch)

    def __repr__(self):
        return repr(self.BuildObject)

## The smallest module unit that can be built by nmake/make command in multi-thread build mode
#
# This class is for module build by nmake/make build system. The "Obj" parameter
# must provide __str__(), __eq__() and __hash__() methods. Otherwise there could
# be make units missing build.
#
# Currently the "Obj" should be only ModuleAutoGen object.
#
class ModuleMakeUnit(BuildUnit):
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  Obj         The ModuleAutoGen object the build is working on
    #   @param  Target      The build target name, one of gSupportedTarget
    #
    def __init__(self, Obj, Target):
        Dependency = [ModuleMakeUnit(La, Target) for La in Obj.LibraryAutoGenList]
        BuildUnit.__init__(self, Obj, Obj.BuildCommand, Target, Dependency, Obj.MakeFileDir)
        if Target in [None, "", "all"]:
            self.Target = "tbuild"

## The smallest platform unit that can be built by nmake/make command in multi-thread build mode
#
# This class is for platform build by nmake/make build system. The "Obj" parameter
# must provide __str__(), __eq__() and __hash__() methods. Otherwise there could
# be make units missing build.
#
# Currently the "Obj" should be only PlatformAutoGen object.
#
class PlatformMakeUnit(BuildUnit):
    ## The constructor
    #
    #   @param  self        The object pointer
    #   @param  Obj         The PlatformAutoGen object the build is working on
    #   @param  Target      The build target name, one of gSupportedTarget
    #
    def __init__(self, Obj, Target):
        Dependency = [ModuleMakeUnit(Lib, Target) for Lib in self.BuildObject.LibraryAutoGenList]
        Dependency.extend([ModuleMakeUnit(Mod, Target) for Mod in self.BuildObject.ModuleAutoGenList])
        BuildUnit.__init__(self, Obj, Obj.BuildCommand, Target, Dependency, Obj.MakeFileDir)

## The class representing the task of a module build or platform build
#
# This class manages the build tasks in multi-thread build mode. Its jobs include
# scheduling thread running, catching thread error, monitor the thread status, etc.
#
class BuildTask:
    # queue for tasks waiting for schedule
    _PendingQueue = sdict()
    _PendingQueueLock = threading.Lock()

    # queue for tasks ready for running
    _ReadyQueue = sdict()
    _ReadyQueueLock = threading.Lock()

    # queue for run tasks
    _RunningQueue = sdict()
    _RunningQueueLock = threading.Lock()

    # queue containing all build tasks, in case duplicate build
    _TaskQueue = sdict()

    # flag indicating error occurs in a running thread
    _ErrorFlag = threading.Event()
    _ErrorFlag.clear()
    _ErrorMessage = ""

    # BoundedSemaphore object used to control the number of running threads
    _Thread = None

    # flag indicating if the scheduler is started or not
    _SchedulerStopped = threading.Event()
    _SchedulerStopped.set()

    ## Start the task scheduler thread
    #
    #   @param  MaxThreadNumber     The maximum thread number
    #   @param  ExitFlag            Flag used to end the scheduler
    #
    @staticmethod
    def StartScheduler(MaxThreadNumber, ExitFlag):
        SchedulerThread = Thread(target=BuildTask.Scheduler, args=(MaxThreadNumber, ExitFlag))
        SchedulerThread.setName("Build-Task-Scheduler")
        SchedulerThread.setDaemon(False)
        SchedulerThread.start()
        # wait for the scheduler to be started, especially useful in Linux
        while not BuildTask.IsOnGoing():
            time.sleep(0.01)

    ## Scheduler method
    #
    #   @param  MaxThreadNumber     The maximum thread number
    #   @param  ExitFlag            Flag used to end the scheduler
    #
    @staticmethod
    def Scheduler(MaxThreadNumber, ExitFlag):
        BuildTask._SchedulerStopped.clear()
        try:
            # use BoundedSemaphore to control the maximum running threads
            BuildTask._Thread = BoundedSemaphore(MaxThreadNumber)
            #
            # scheduling loop, which will exits when no pending/ready task and
            # indicated to do so, or there's error in running thread
            #
            while (len(BuildTask._PendingQueue) > 0 or len(BuildTask._ReadyQueue) > 0 \
                   or not ExitFlag.isSet()) and not BuildTask._ErrorFlag.isSet():
                EdkLogger.debug(EdkLogger.DEBUG_8, "Pending Queue (%d), Ready Queue (%d)"
                                % (len(BuildTask._PendingQueue), len(BuildTask._ReadyQueue)))

                # get all pending tasks
                BuildTask._PendingQueueLock.acquire()
                BuildObjectList = BuildTask._PendingQueue.keys()
                #
                # check if their dependency is resolved, and if true, move them
                # into ready queue
                #
                for BuildObject in BuildObjectList:
                    Bt = BuildTask._PendingQueue[BuildObject]
                    if Bt.IsReady():
                        BuildTask._ReadyQueue[BuildObject] = BuildTask._PendingQueue.pop(BuildObject)
                BuildTask._PendingQueueLock.release()

                # launch build thread until the maximum number of threads is reached
                while not BuildTask._ErrorFlag.isSet():
                    # empty ready queue, do nothing further
                    if len(BuildTask._ReadyQueue) == 0:
                        break

                    # wait for active thread(s) exit
                    BuildTask._Thread.acquire(True)

                    # start a new build thread
                    Bo = BuildTask._ReadyQueue.keys()[0]
                    Bt = BuildTask._ReadyQueue.pop(Bo)

                    # move into running queue
                    BuildTask._RunningQueueLock.acquire()
                    BuildTask._RunningQueue[Bo] = Bt
                    BuildTask._RunningQueueLock.release()

                    Bt.Start()
                    # avoid tense loop
                    time.sleep(0.01)

                # avoid tense loop
                time.sleep(0.01)

            # wait for all running threads exit
            if BuildTask._ErrorFlag.isSet():
                EdkLogger.quiet("\nWaiting for all build threads exit...")
            # while not BuildTask._ErrorFlag.isSet() and \
            while len(BuildTask._RunningQueue) > 0:
                EdkLogger.verbose("Waiting for thread ending...(%d)" % len(BuildTask._RunningQueue))
                EdkLogger.debug(EdkLogger.DEBUG_8, "Threads [%s]" % ", ".join([Th.getName() for Th in threading.enumerate()]))
                # avoid tense loop
                time.sleep(0.1)
        except BaseException, X:
            #
            # TRICK: hide the output of threads left runing, so that the user can
            #        catch the error message easily
            #
            EdkLogger.SetLevel(EdkLogger.ERROR)
            BuildTask._ErrorFlag.set()
            BuildTask._ErrorMessage = "build thread scheduler error\n\t%s" % str(X)

        BuildTask._PendingQueue.clear()
        BuildTask._ReadyQueue.clear()
        BuildTask._RunningQueue.clear()
        BuildTask._TaskQueue.clear()
        BuildTask._SchedulerStopped.set()

    ## Wait for all running method exit
    #
    @staticmethod
    def WaitForComplete():
        BuildTask._SchedulerStopped.wait()

    ## Check if the scheduler is running or not
    #
    @staticmethod
    def IsOnGoing():
        return not BuildTask._SchedulerStopped.isSet()

    ## Abort the build
    @staticmethod
    def Abort():
        if BuildTask.IsOnGoing():
            BuildTask._ErrorFlag.set()
            BuildTask.WaitForComplete()

    ## Check if there's error in running thread
    #
    #   Since the main thread cannot catch exceptions in other thread, we have to
    #   use threading.Event to communicate this formation to main thread.
    #
    @staticmethod
    def HasError():
        return BuildTask._ErrorFlag.isSet()

    ## Get error message in running thread
    #
    #   Since the main thread cannot catch exceptions in other thread, we have to
    #   use a static variable to communicate this message to main thread.
    #
    @staticmethod
    def GetErrorMessage():
        return BuildTask._ErrorMessage

    ## Factory method to create a BuildTask object
    #
    #   This method will check if a module is building or has been built. And if
    #   true, just return the associated BuildTask object in the _TaskQueue. If
    #   not, create and return a new BuildTask object. The new BuildTask object
    #   will be appended to the _PendingQueue for scheduling later.
    #
    #   @param  BuildItem       A BuildUnit object representing a build object
    #   @param  Dependency      The dependent build object of BuildItem
    #
    @staticmethod
    def New(BuildItem, Dependency=None):
        if BuildItem in BuildTask._TaskQueue:
            Bt = BuildTask._TaskQueue[BuildItem]
            return Bt

        Bt = BuildTask()
        Bt._Init(BuildItem, Dependency)
        BuildTask._TaskQueue[BuildItem] = Bt

        BuildTask._PendingQueueLock.acquire()
        BuildTask._PendingQueue[BuildItem] = Bt
        BuildTask._PendingQueueLock.release()

        return Bt

    ## The real constructor of BuildTask
    #
    #   @param  BuildItem       A BuildUnit object representing a build object
    #   @param  Dependency      The dependent build object of BuildItem
    #
    def _Init(self, BuildItem, Dependency=None):
        self.BuildItem = BuildItem

        self.DependencyList = []
        if Dependency == None:
            Dependency = BuildItem.Dependency
        else:
            Dependency.extend(BuildItem.Dependency)
        self.AddDependency(Dependency)
        # flag indicating build completes, used to avoid unnecessary re-build
        self.CompleteFlag = False

    ## Check if all dependent build tasks are completed or not
    #
    def IsReady(self):
        ReadyFlag = True
        for Dep in self.DependencyList:
            if Dep.CompleteFlag == True:
                continue
            ReadyFlag = False
            break

        return ReadyFlag

    ## Add dependent build task
    #
    #   @param  Dependency      The list of dependent build objects
    #
    def AddDependency(self, Dependency):
        for Dep in Dependency:
            self.DependencyList.append(BuildTask.New(Dep))    # BuildTask list

    ## The thread wrapper of LaunchCommand function
    #
    # @param  Command               A list or string contains the call of the command
    # @param  WorkingDir            The directory in which the program will be running
    #
    def _CommandThread(self, Command, WorkingDir):
        try:
            LaunchCommand(Command, WorkingDir)
            self.CompleteFlag = True
        except:
            #
            # TRICK: hide the output of threads left runing, so that the user can
            #        catch the error message easily
            #
            if not BuildTask._ErrorFlag.isSet():
                GlobalData.gBuildingModule = "%s [%s, %s, %s]" % (str(self.BuildItem.BuildObject),
                                                                  self.BuildItem.BuildObject.Arch,
                                                                  self.BuildItem.BuildObject.ToolChain,
                                                                  self.BuildItem.BuildObject.BuildTarget
                                                                 )
            EdkLogger.SetLevel(EdkLogger.ERROR)
            BuildTask._ErrorFlag.set()
            BuildTask._ErrorMessage = "%s broken\n    %s [%s]" % \
                                      (threading.currentThread().getName(), Command, WorkingDir)
        # indicate there's a thread is available for another build task
        BuildTask._RunningQueueLock.acquire()
        BuildTask._RunningQueue.pop(self.BuildItem)
        BuildTask._RunningQueueLock.release()
        BuildTask._Thread.release()

    ## Start build task thread
    #
    def Start(self):
        EdkLogger.quiet("Building ... %s" % repr(self.BuildItem))
        Command = self.BuildItem.BuildCommand + [self.BuildItem.Target]
        self.BuildTread = Thread(target=self._CommandThread, args=(Command, self.BuildItem.WorkingDir))
        self.BuildTread.setName("build thread")
        self.BuildTread.setDaemon(False)
        self.BuildTread.start()

## The class implementing the EDK2 build process
#
#   The build process includes:
#       1. Load configuration from target.txt and tools_def.txt in $(WORKSPACE)/Conf
#       2. Parse DSC file of active platform
#       3. Parse FDF file if any
#       4. Establish build database, including parse all other files (module, package)
#       5. Create AutoGen files (C code file, depex file, makefile) if necessary
#       6. Call build command
#
class Build():
    ## Constructor
    #
    # Constructor will load all necessary configurations, parse platform, modules
    # and packages and the establish a database for AutoGen.
    #
    #   @param  Target              The build command target, one of gSupportedTarget
    #   @param  WorkspaceDir        The directory of workspace
    #   @param  Platform            The DSC file of active platform
    #   @param  Module              The INF file of active module, if any
    #   @param  Arch                The Arch list of platform or module
    #   @param  ToolChain           The name list of toolchain
    #   @param  BuildTarget         The "DEBUG" or "RELEASE" build
    #   @param  FlashDefinition     The FDF file of active platform
    #   @param  FdList=[]           The FD names to be individually built
    #   @param  FvList=[]           The FV names to be individually built
    #   @param  MakefileType        The type of makefile (for MSFT make or GNU make)
    #   @param  SilentMode          Indicate multi-thread build mode
    #   @param  ThreadNumber        The maximum number of thread if in multi-thread build mode
    #   @param  SkipAutoGen         Skip AutoGen step
    #   @param  Reparse             Re-parse all meta files
    #   @param  SkuId               SKU id from command line
    #
    def __init__(self, Target, WorkspaceDir, Platform, Module, Arch, ToolChain,
                 BuildTarget, FlashDefinition, FdList=[], FvList=[],
                 MakefileType="nmake", SilentMode=False, ThreadNumber=2,
                 SkipAutoGen=False, Reparse=False, SkuId=None):

        self.WorkspaceDir = WorkspaceDir
        self.Target         = Target
        self.PlatformFile   = Platform
        self.ModuleFile     = Module
        self.ArchList       = Arch
        self.ToolChainList  = ToolChain
        self.BuildTargetList= BuildTarget
        self.Fdf            = FlashDefinition
        self.FdList         = FdList
        self.FvList         = FvList
        self.MakefileType   = MakefileType
        self.SilentMode     = SilentMode
        self.ThreadNumber   = ThreadNumber
        self.SkipAutoGen    = SkipAutoGen
        self.Reparse        = Reparse
        self.SkuId          = SkuId
        self.SpawnMode      = True

        self.TargetTxt      = TargetTxtClassObject()
        self.ToolDef        = ToolDefClassObject()
        #self.Db             = WorkspaceDatabase(None, GlobalData.gGlobalDefines, self.Reparse)
        self.Db             = WorkspaceDatabase(None, {}, self.Reparse)
        self.BuildDatabase  = self.Db.BuildObject
        self.Platform       = None

        # print dot charater during doing some time-consuming work
        self.Progress = Utils.Progressor()

        # parse target.txt, tools_def.txt, and platform file
        #self.RestoreBuildData()
        self.LoadConfiguration()
        self.InitBuild()

        # print current build environment and configuration
        EdkLogger.quiet("%-24s = %s" % ("WORKSPACE", os.environ["WORKSPACE"]))
        EdkLogger.quiet("%-24s = %s" % ("ECP_SOURCE", os.environ["ECP_SOURCE"]))
        EdkLogger.quiet("%-24s = %s" % ("EDK_SOURCE", os.environ["EDK_SOURCE"]))
        EdkLogger.quiet("%-24s = %s" % ("EFI_SOURCE", os.environ["EFI_SOURCE"]))
        EdkLogger.quiet("%-24s = %s" % ("EDK_TOOLS_PATH", os.environ["EDK_TOOLS_PATH"]))

        EdkLogger.info('\n%-24s = %s' % ("TARGET_ARCH", ' '.join(self.ArchList)))
        EdkLogger.info('%-24s = %s' % ("TARGET", ' '.join(self.BuildTargetList)))
        EdkLogger.info('%-24s = %s' % ("TOOL_CHAIN_TAG", ' '.join(self.ToolChainList)))

        EdkLogger.info('\n%-24s = %s' % ("Active Platform", self.PlatformFile))

        if self.Fdf != None and self.Fdf != "":
            EdkLogger.info('%-24s = %s' % ("Flash Image Definition", self.Fdf))

        if self.ModuleFile != None and self.ModuleFile != "":
            EdkLogger.info('%-24s = %s' % ("Active Module", self.ModuleFile))

        os.chdir(self.WorkspaceDir)
        self.Progress.Start("\nProcessing meta-data")

    ## Load configuration
    #
    #   This method will parse target.txt and get the build configurations.
    #
    def LoadConfiguration(self):
        #
        # Check target.txt and tools_def.txt and Init them
        #
        BuildConfigurationFile = os.path.normpath(os.path.join(self.WorkspaceDir, gBuildConfiguration))
        if os.path.isfile(BuildConfigurationFile) == True:
            StatusCode = self.TargetTxt.LoadTargetTxtFile(BuildConfigurationFile)

            ToolDefinitionFile = self.TargetTxt.TargetTxtDictionary[DataType.TAB_TAT_DEFINES_TOOL_CHAIN_CONF]
            if ToolDefinitionFile == '':
                ToolDefinitionFile = gToolsDefinition
            ToolDefinitionFile = os.path.normpath(os.path.join(self.WorkspaceDir, ToolDefinitionFile))
            if os.path.isfile(ToolDefinitionFile) == True:
                StatusCode = self.ToolDef.LoadToolDefFile(ToolDefinitionFile)
            else:
                EdkLogger.error("build", FILE_NOT_FOUND, ExtraData=ToolDefinitionFile)
        else:
            EdkLogger.error("build", FILE_NOT_FOUND, ExtraData=BuildConfigurationFile)

        # if no ARCH given in command line, get it from target.txt
        if self.ArchList == None or len(self.ArchList) == 0:
            self.ArchList = self.TargetTxt.TargetTxtDictionary[DataType.TAB_TAT_DEFINES_TARGET_ARCH]

        # if no build target given in command line, get it from target.txt
        if self.BuildTargetList == None or len(self.BuildTargetList) == 0:
            self.BuildTargetList = self.TargetTxt.TargetTxtDictionary[DataType.TAB_TAT_DEFINES_TARGET]

        # if no tool chain given in command line, get it from target.txt
        if self.ToolChainList == None or len(self.ToolChainList) == 0:
            self.ToolChainList = self.TargetTxt.TargetTxtDictionary[DataType.TAB_TAT_DEFINES_TOOL_CHAIN_TAG]
            if self.ToolChainList == None or len(self.ToolChainList) == 0:
                EdkLogger.error("build", RESOURCE_NOT_AVAILABLE, ExtraData="No toolchain given. Don't know how to build.\n")

        # check if the tool chains are defined or not
        NewToolChainList = []
        for ToolChain in self.ToolChainList:
            if ToolChain not in self.ToolDef.ToolsDefTxtDatabase[TAB_TOD_DEFINES_TOOL_CHAIN_TAG]:
                EdkLogger.warn("build", "Tool chain [%s] is not defined" % ToolChain)
            else:
                NewToolChainList.append(ToolChain)
        # if no tool chain available, break the build
        if len(NewToolChainList) == 0:
            EdkLogger.error("build", RESOURCE_NOT_AVAILABLE,
                            ExtraData="[%s] not defined. No toolchain available for build!\n" % ", ".join(self.ToolChainList))
        else:
            self.ToolChainList = NewToolChainList

        if self.ThreadNumber == None:
            self.ThreadNumber = self.TargetTxt.TargetTxtDictionary[DataType.TAB_TAT_DEFINES_MAX_CONCURRENT_THREAD_NUMBER]
            if self.ThreadNumber == '':
                self.ThreadNumber = 0
            else:
                self.ThreadNumber = int(self.ThreadNumber, 0)

        if self.ThreadNumber == 0:
            self.ThreadNumber = 1

        if not self.PlatformFile:
            PlatformFile = self.TargetTxt.TargetTxtDictionary[DataType.TAB_TAT_DEFINES_ACTIVE_PLATFORM]
            if not PlatformFile:
                # Try to find one in current directory
                WorkingDirectory = os.getcwd()
                FileList = glob.glob(os.path.normpath(os.path.join(WorkingDirectory, '*.dsc')))
                FileNum = len(FileList)
                if FileNum >= 2:
                    EdkLogger.error("build", OPTION_MISSING,
                                    ExtraData="There are %d DSC files in %s. Use '-p' to specify one.\n" % (FileNum, WorkingDirectory))
                elif FileNum == 1:
                    PlatformFile = FileList[0]
                else:
                    EdkLogger.error("build", RESOURCE_NOT_AVAILABLE,
                                    ExtraData="No active platform specified in target.txt or command line! Nothing can be built.\n")

            self.PlatformFile = PathClass(NormFile(PlatformFile, self.WorkspaceDir), self.WorkspaceDir)
            ErrorCode, ErrorInfo = self.PlatformFile.Validate(".dsc", False)
            if ErrorCode != 0:
                EdkLogger.error("build", ErrorCode, ExtraData=ErrorInfo)

    ## Initialize build configuration
    #
    #   This method will parse DSC file and merge the configurations from
    #   command line and target.txt, then get the final build configurations.
    #
    def InitBuild(self):
        ErrorCode, ErrorInfo = self.PlatformFile.Validate(".dsc")
        if ErrorCode != 0:
            EdkLogger.error("build", ErrorCode, ExtraData=ErrorInfo)

        # create metafile database
        self.Db.InitDatabase()

        # we need information in platform description file to determine how to build
        self.Platform = self.BuildDatabase[self.PlatformFile, 'COMMON']
        if not self.Fdf:
            self.Fdf = self.Platform.FlashDefinition

        if self.SkuId == None or self.SkuId == '':
            self.SkuId = self.Platform.SkuName

        # check FD/FV build target
        if self.Fdf == None or self.Fdf == "":
            if self.FdList != []:
                EdkLogger.info("No flash definition file found. FD [%s] will be ignored." % " ".join(self.FdList))
                self.FdList = []
            if self.FvList != []:
                EdkLogger.info("No flash definition file found. FV [%s] will be ignored." % " ".join(self.FvList))
                self.FvList = []
        else:
            FdfParserObj = FdfParser(str(self.Fdf))
            FdfParserObj.ParseFile()
            for fvname in self.FvList:
                if fvname.upper() not in FdfParserObj.Profile.FvDict.keys():
                    EdkLogger.error("build", OPTION_VALUE_INVALID,
                                    "No such an FV in FDF file: %s" % fvname)

        #
        # Merge Arch
        #
        if self.ArchList == None or len(self.ArchList) == 0:
            ArchList = set(self.Platform.SupArchList)
        else:
            ArchList = set(self.ArchList) & set(self.Platform.SupArchList)
        if len(ArchList) == 0:
            EdkLogger.error("build", PARAMETER_INVALID,
                            ExtraData = "Active platform supports [%s] only, but [%s] is given."
                                        % (" ".join(self.Platform.SupArchList), " ".join(self.ArchList)))
        elif len(ArchList) != len(self.ArchList):
            SkippedArchList = set(self.ArchList).symmetric_difference(set(self.Platform.SupArchList))
            EdkLogger.verbose("\nArch [%s] is ignored because active platform supports [%s] but [%s] is specified !"
                           % (" ".join(SkippedArchList), " ".join(self.Platform.SupArchList), " ".join(self.ArchList)))
        self.ArchList = tuple(ArchList)

        # Merge build target
        if self.BuildTargetList == None or len(self.BuildTargetList) == 0:
            BuildTargetList = self.Platform.BuildTargets
        else:
            BuildTargetList = list(set(self.BuildTargetList) & set(self.Platform.BuildTargets))
        if BuildTargetList == []:
            EdkLogger.error("build", PARAMETER_INVALID, "Active platform only supports [%s], but [%s] is given"
                                % (" ".join(self.Platform.BuildTargets), " ".join(self.BuildTargetList)))
        self.BuildTargetList = BuildTargetList

    ## Build a module or platform
    #
    # Create autogen code and makfile for a module or platform, and the launch
    # "make" command to build it
    #
    #   @param  Target                      The target of build command
    #   @param  Platform                    The platform file
    #   @param  Module                      The module file
    #   @param  BuildTarget                 The name of build target, one of "DEBUG", "RELEASE"
    #   @param  ToolChain                   The name of toolchain to build
    #   @param  Arch                        The arch of the module/platform
    #   @param  CreateDepModuleCodeFile     Flag used to indicate creating code
    #                                       for dependent modules/Libraries
    #   @param  CreateDepModuleMakeFile     Flag used to indicate creating makefile
    #                                       for dependent modules/Libraries
    #
    def _Build(self, Target, AutoGenObject, CreateDepsCodeFile=True, CreateDepsMakeFile=True):
        if AutoGenObject == None:
            return False

        # skip file generation for cleanxxx targets, run and fds target
        if Target not in ['clean', 'cleanlib', 'cleanall', 'run', 'fds']:
            # for target which must generate AutoGen code and makefile
            if not self.SkipAutoGen or Target == 'genc':
                self.Progress.Start("Generating code")
                AutoGenObject.CreateCodeFile(CreateDepsCodeFile)
                self.Progress.Stop("done!")
            if Target == "genc":
                return True

            if not self.SkipAutoGen or Target == 'genmake':
                self.Progress.Start("Generating makefile")
                AutoGenObject.CreateMakeFile(CreateDepsMakeFile)
                self.Progress.Stop("done!")
            if Target == "genmake":
                return True
        else:
            # always recreate top/platform makefile when clean, just in case of inconsistency
            AutoGenObject.CreateCodeFile(False)
            AutoGenObject.CreateMakeFile(False)

        if EdkLogger.GetLevel() == EdkLogger.QUIET:
            EdkLogger.quiet("Building ... %s" % repr(AutoGenObject))

        BuildCommand = AutoGenObject.BuildCommand
        if BuildCommand == None or len(BuildCommand) == 0:
            EdkLogger.error("build", OPTION_MISSING, ExtraData="No MAKE command found for [%s, %s, %s]" % Key)

        BuildCommand = BuildCommand + [Target]
        LaunchCommand(BuildCommand, AutoGenObject.MakeFileDir)
        if Target == 'cleanall':
            try:
                #os.rmdir(AutoGenObject.BuildDir)
                RemoveDirectory(AutoGenObject.BuildDir, True)
            except WindowsError, X:
                EdkLogger.error("build", FILE_DELETE_FAILURE, ExtraData=str(X))
        return True

    ## Build active platform for different build targets and different tool chains
    #
    def _BuildPlatform(self):
        for BuildTarget in self.BuildTargetList:
            for ToolChain in self.ToolChainList:
                Wa = WorkspaceAutoGen(
                        self.WorkspaceDir,
                        self.Platform,
                        BuildTarget,
                        ToolChain,
                        self.ArchList,
                        self.BuildDatabase,
                        self.TargetTxt,
                        self.ToolDef,
                        self.Fdf,
                        self.FdList,
                        self.FvList,
                        self.SkuId
                        )
                self.Progress.Stop("done!")
                self._Build(self.Target, Wa)

    ## Build active module for different build targets, different tool chains and different archs
    #
    def _BuildModule(self):
        for BuildTarget in self.BuildTargetList:
            for ToolChain in self.ToolChainList:
                #
                # module build needs platform build information, so get platform
                # AutoGen first
                #
                Wa = WorkspaceAutoGen(
                        self.WorkspaceDir,
                        self.Platform,
                        BuildTarget,
                        ToolChain,
                        self.ArchList,
                        self.BuildDatabase,
                        self.TargetTxt,
                        self.ToolDef,
                        self.Fdf,
                        self.FdList,
                        self.FvList,
                        self.SkuId
                        )
                Wa.CreateMakeFile(False)
                self.Progress.Stop("done!")
                MaList = []
                for Arch in self.ArchList:
                    Ma = ModuleAutoGen(Wa, self.ModuleFile, BuildTarget, ToolChain, Arch, self.PlatformFile)
                    if Ma == None: continue
                    MaList.append(Ma)
                    self._Build(self.Target, Ma)
                if MaList == []:
                    EdkLogger.error(
                                'build',
                                BUILD_ERROR,
                                "Module for [%s] is not a component of active platform."\
                                " Please make sure that the ARCH and inf file path are"\
                                " given in the same as in [%s]" %\
                                    (', '.join(self.ArchList), self.Platform),
                                ExtraData=self.ModuleFile
                                )

    ## Build a platform in multi-thread mode
    #
    def _MultiThreadBuildPlatform(self):
        for BuildTarget in self.BuildTargetList:
            for ToolChain in self.ToolChainList:
                Wa = WorkspaceAutoGen(
                        self.WorkspaceDir,
                        self.Platform,
                        BuildTarget,
                        ToolChain,
                        self.ArchList,
                        self.BuildDatabase,
                        self.TargetTxt,
                        self.ToolDef,
                        self.Fdf,
                        self.FdList,
                        self.FvList,
                        self.SkuId
                        )
                Wa.CreateMakeFile(False)

                # multi-thread exit flag
                ExitFlag = threading.Event()
                ExitFlag.clear()
                for Arch in self.ArchList:
                    Pa = PlatformAutoGen(Wa, self.PlatformFile, BuildTarget, ToolChain, Arch)
                    if Pa == None:
                        continue
                    for Module in Pa.Platform.Modules:
                        # Get ModuleAutoGen object to generate C code file and makefile
                        Ma = ModuleAutoGen(Wa, Module, BuildTarget, ToolChain, Arch, self.PlatformFile)
                        if Ma == None:
                            continue
                        # Not to auto-gen for targets 'clean', 'cleanlib', 'cleanall', 'run', 'fds'
                        if self.Target not in ['clean', 'cleanlib', 'cleanall', 'run', 'fds']:
                            # for target which must generate AutoGen code and makefile
                            if not self.SkipAutoGen or self.Target == 'genc':
                                Ma.CreateCodeFile(True)
                            if self.Target == "genc":
                                continue

                            if not self.SkipAutoGen or self.Target == 'genmake':
                                Ma.CreateMakeFile(True)
                            if self.Target == "genmake":
                                continue
                        self.Progress.Stop("done!")
                        # Generate build task for the module
                        Bt = BuildTask.New(ModuleMakeUnit(Ma, self.Target))
                        # Break build if any build thread has error
                        if BuildTask.HasError():
                            # we need a full version of makefile for platform
                            ExitFlag.set()
                            BuildTask.WaitForComplete()
                            Pa.CreateMakeFile(False)
                            EdkLogger.error("build", BUILD_ERROR, "Failed to build module", ExtraData=GlobalData.gBuildingModule)
                        # Start task scheduler
                        if not BuildTask.IsOnGoing():
                            BuildTask.StartScheduler(self.ThreadNumber, ExitFlag)

                    # in case there's an interruption. we need a full version of makefile for platform
                    Pa.CreateMakeFile(False)
                    if BuildTask.HasError():
                        EdkLogger.error("build", BUILD_ERROR, "Failed to build module", ExtraData=GlobalData.gBuildingModule)

                #
                # All modules have been put in build tasks queue. Tell task scheduler
                # to exit if all tasks are completed
                #
                ExitFlag.set()
                BuildTask.WaitForComplete()

                #
                # Check for build error, and raise exception if one
                # has been signaled.
                #
                if BuildTask.HasError():
                    EdkLogger.error("build", BUILD_ERROR, "Failed to build module", ExtraData=GlobalData.gBuildingModule)

                # Generate FD image if there's a FDF file found
                if self.Fdf != '' and self.Target in ["", "all", "fds"]:
                    LaunchCommand(Wa.BuildCommand + ["fds"], Wa.MakeFileDir)

    ## Generate GuidedSectionTools.txt in the FV directories.
    #
    def CreateGuidedSectionToolsFile(self):
        for Arch in self.ArchList:
            for BuildTarget in self.BuildTargetList:
                for ToolChain in self.ToolChainList:
                    FvDir = os.path.join(
                                self.WorkspaceDir,
                                self.Platform.OutputDirectory,
                                '_'.join((BuildTarget, ToolChain)),
                                'FV'
                                )
                    if not os.path.exists(FvDir):
                        continue
                    # Build up the list of supported architectures for this build
                    prefix = '%s_%s_%s_' % (BuildTarget, ToolChain, Arch)

                    # Look through the tool definitions for GUIDed tools
                    guidAttribs = []
                    for (attrib, value) in self.ToolDef.ToolsDefTxtDictionary.iteritems():
                        if attrib.upper().endswith('_GUID'):
                            split = attrib.split('_')
                            thisPrefix = '_'.join(split[0:3]) + '_'
                            if thisPrefix == prefix:
                                guid = self.ToolDef.ToolsDefTxtDictionary[attrib]
                                guid = guid.lower()
                                toolName = split[3]
                                path = '_'.join(split[0:4]) + '_PATH'
                                path = self.ToolDef.ToolsDefTxtDictionary[path]
                                path = self.GetFullPathOfTool(path)
                                guidAttribs.append((guid, toolName, path))

                    # Write out GuidedSecTools.txt
                    toolsFile = os.path.join(FvDir, 'GuidedSectionTools.txt')
                    toolsFile = open(toolsFile, 'wt')
                    for guidedSectionTool in guidAttribs:
                        print >> toolsFile, ' '.join(guidedSectionTool)
                    toolsFile.close()

    ## Returns the full path of the tool.
    #
    def GetFullPathOfTool (self, tool):
        if os.path.exists(tool):
            return os.path.realpath(tool)
        else:
            # We need to search for the tool using the
            # PATH environment variable.
            for dirInPath in os.environ['PATH'].split(os.pathsep):
                foundPath = os.path.join(dirInPath, tool)
                if os.path.exists(foundPath):
                    return os.path.realpath(foundPath)

        # If the tool was not found in the path then we just return
        # the input tool.
        return tool

    ## Launch the module or platform build
    #
    def Launch(self):
        if self.ModuleFile == None or self.ModuleFile == "":
            if not self.SpawnMode or self.Target not in ["", "all"]:
                self.SpawnMode = False
                self._BuildPlatform()
            else:
                self._MultiThreadBuildPlatform()
            self.CreateGuidedSectionToolsFile()
        else:
            self.SpawnMode = False
            self._BuildModule()

    ## Do some clean-up works when error occurred
    def Relinquish(self):
        OldLogLevel = EdkLogger.GetLevel()
        EdkLogger.SetLevel(EdkLogger.ERROR)
        #self.DumpBuildData()
        Utils.Progressor.Abort()
        if self.SpawnMode == True:
            BuildTask.Abort()
        EdkLogger.SetLevel(OldLogLevel)

    def DumpBuildData(self):
        CacheDirectory = os.path.join(self.WorkspaceDir, gBuildCacheDir)
        Utils.CreateDirectory(CacheDirectory)
        Utils.DataDump(Utils.gFileTimeStampCache, os.path.join(CacheDirectory, "gFileTimeStampCache"))
        Utils.DataDump(Utils.gDependencyDatabase, os.path.join(CacheDirectory, "gDependencyDatabase"))

    def RestoreBuildData(self):
        FilePath = os.path.join(self.WorkspaceDir, gBuildCacheDir, "gFileTimeStampCache")
        if Utils.gFileTimeStampCache == {} and os.path.isfile(FilePath):
            Utils.gFileTimeStampCache = Utils.DataRestore(FilePath)
            if Utils.gFileTimeStampCache == None:
                Utils.gFileTimeStampCache = {}

        FilePath = os.path.join(self.WorkspaceDir, gBuildCacheDir, "gDependencyDatabase")
        if Utils.gDependencyDatabase == {} and os.path.isfile(FilePath):
            Utils.gDependencyDatabase = Utils.DataRestore(FilePath)
            if Utils.gDependencyDatabase == None:
                Utils.gDependencyDatabase = {}

def ParseDefines(DefineList=[]):
    DefineDict = {}
    if DefineList != None:
        for Define in DefineList:
            DefineTokenList = Define.split("=", 1)
            if len(DefineTokenList) == 1:
                DefineDict[DefineTokenList[0]] = ""
            else:
                DefineDict[DefineTokenList[0]] = DefineTokenList[1].strip()
    return DefineDict

gParamCheck = []
def SingleCheckCallback(option, opt_str, value, parser):
    if option not in gParamCheck:
        setattr(parser.values, option.dest, value)
        gParamCheck.append(option)
    else:
        parser.error("Option %s only allows one instance in command line!" % option)

## Parse command line options
#
# Using standard Python module optparse to parse command line option of this tool.
#
#   @retval Opt   A optparse.Values object containing the parsed options
#   @retval Args  Target of build command
#
def MyOptionParser():
    Parser = OptionParser(description=__copyright__,version=__version__,prog="build.exe",usage="%prog [options] [all|fds|genc|genmake|clean|cleanall|cleanlib|modules|libraries|run]")
    Parser.add_option("-a", "--arch", action="append", type="choice", choices=['IA32','X64','IPF','EBC','ARM'], dest="TargetArch",
        help="ARCHS is one of list: IA32, X64, IPF, ARM or EBC, which overrides target.txt's TARGET_ARCH definition. To specify more archs, please repeat this option.")
    Parser.add_option("-p", "--platform", action="callback", type="string", dest="PlatformFile", callback=SingleCheckCallback,
        help="Build the platform specified by the DSC file name argument, overriding target.txt's ACTIVE_PLATFORM definition.")
    Parser.add_option("-m", "--module", action="callback", type="string", dest="ModuleFile", callback=SingleCheckCallback,
        help="Build the module specified by the INF file name argument.")
    Parser.add_option("-b", "--buildtarget", action="append", type="choice", choices=['DEBUG','RELEASE'], dest="BuildTarget",
        help="BuildTarget is one of list: DEBUG, RELEASE, which overrides target.txt's TARGET definition. To specify more TARGET, please repeat this option.")
    Parser.add_option("-t", "--tagname", action="append", type="string", dest="ToolChain",
        help="Using the Tool Chain Tagname to build the platform, overriding target.txt's TOOL_CHAIN_TAG definition.")
    Parser.add_option("-x", "--sku-id", action="callback", type="string", dest="SkuId", callback=SingleCheckCallback,
        help="Using this name of SKU ID to build the platform, overriding SKUID_IDENTIFIER in DSC file.")

    Parser.add_option("-n", action="callback", type="int", dest="ThreadNumber", callback=SingleCheckCallback,
        help="Build the platform using multi-threaded compiler. The value overrides target.txt's MAX_CONCURRENT_THREAD_NUMBER. Less than 2 will disable multi-thread builds.")

    Parser.add_option("-f", "--fdf", action="callback", type="string", dest="FdfFile", callback=SingleCheckCallback,
        help="The name of the FDF file to use, which overrides the setting in the DSC file.")
    Parser.add_option("-r", "--rom-image", action="append", type="string", dest="RomImage", default=[],
        help="The name of FD to be generated. The name must be from [FD] section in FDF file.")
    Parser.add_option("-i", "--fv-image", action="append", type="string", dest="FvImage", default=[],
        help="The name of FV to be generated. The name must be from [FV] section in FDF file.")

    Parser.add_option("-u", "--skip-autogen", action="store_true", dest="SkipAutoGen", help="Skip AutoGen step.")
    Parser.add_option("-e", "--re-parse", action="store_true", dest="Reparse", help="Re-parse all meta-data files.")

    Parser.add_option("-c", "--case-insensitive", action="store_true", dest="CaseInsensitive", help="Don't check case of file name.")

    # Parser.add_option("-D", "--define", action="append", dest="Defines", metavar="NAME[=[VALUE]]",
    #     help="Define global macro which can be used in DSC/DEC/INF files.")

    Parser.add_option("-w", "--warning-as-error", action="store_true", dest="WarningAsError", help="Treat warning in tools as error.")
    Parser.add_option("-j", "--log", action="store", dest="LogFile", help="Put log in specified file as well as on console.")

    Parser.add_option("-s", "--silent", action="store_true", type=None, dest="SilentMode",
        help="Make use of silent mode of (n)make.")
    Parser.add_option("-q", "--quiet", action="store_true", type=None, help="Disable all messages except FATAL ERRORS.")
    Parser.add_option("-v", "--verbose", action="store_true", type=None, help="Turn on verbose output with informational messages printed, "\
                                                                               "including library instances selected, final dependency expression, "\
                                                                               "and warning messages, etc.")
    Parser.add_option("-d", "--debug", action="store", type="int", help="Enable debug messages at specified level.")

    (Opt, Args)=Parser.parse_args()
    return (Opt, Args)

## Tool entrance method
#
# This method mainly dispatch specific methods per the command line options.
# If no error found, return zero value so the caller of this tool can know
# if it's executed successfully or not.
#
#   @retval 0     Tool was successful
#   @retval 1     Tool failed
#
def Main():
    StartTime = time.time()

    # Initialize log system
    EdkLogger.Initialize()

    #
    # Parse the options and args
    #
    (Option, Target) = MyOptionParser()
    GlobalData.gOptions = Option
    GlobalData.gCaseInsensitive = Option.CaseInsensitive

    # Set log level
    if Option.verbose != None:
        EdkLogger.SetLevel(EdkLogger.VERBOSE)
    elif Option.quiet != None:
        EdkLogger.SetLevel(EdkLogger.QUIET)
    elif Option.debug != None:
        EdkLogger.SetLevel(Option.debug + 1)
    else:
        EdkLogger.SetLevel(EdkLogger.INFO)

    if Option.LogFile != None:
        EdkLogger.SetLogFile(Option.LogFile)

    if Option.WarningAsError == True:
        EdkLogger.SetWarningAsError()

    if platform.platform().find("Windows") >= 0:
        GlobalData.gIsWindows = True
    else:
        GlobalData.gIsWindows = False

    EdkLogger.quiet(time.strftime("%H:%M:%S, %b.%d %Y ", time.localtime()) + "[%s]\n" % platform.platform())
    ReturnCode = 0
    MyBuild = None
    try:
        if len(Target) == 0:
            Target = "all"
        elif len(Target) >= 2:
            EdkLogger.error("build", OPTION_NOT_SUPPORTED, "More than one targets are not supported.",
                            ExtraData="Please select one of: %s" %(' '.join(gSupportedTarget)))
        else:
            Target = Target[0].lower()

        if Target not in gSupportedTarget:
            EdkLogger.error("build", OPTION_NOT_SUPPORTED, "Not supported target [%s]." % Target,
                            ExtraData="Please select one of: %s" %(' '.join(gSupportedTarget)))

        # GlobalData.gGlobalDefines = ParseDefines(Option.Defines)
        #
        # Check environment variable: EDK_TOOLS_PATH, WORKSPACE, PATH
        #
        CheckEnvVariable()
        Workspace = os.getenv("WORKSPACE")
        #
        # Get files real name in workspace dir
        #
        GlobalData.gAllFiles = Utils.DirCache(Workspace)

        WorkingDirectory = os.getcwd()
        if not Option.ModuleFile:
            FileList = glob.glob(os.path.normpath(os.path.join(WorkingDirectory, '*.inf')))
            FileNum = len(FileList)
            if FileNum >= 2:
                EdkLogger.error("build", OPTION_NOT_SUPPORTED, "There are %d INF files in %s." % (FileNum, WorkingDirectory),
                                ExtraData="Please use '-m <INF_FILE_PATH>' switch to choose one.")
            elif FileNum == 1:
                Option.ModuleFile = NormFile(FileList[0], Workspace)

        if Option.ModuleFile:
            Option.ModuleFile = PathClass(Option.ModuleFile, Workspace)
            ErrorCode, ErrorInfo = Option.ModuleFile.Validate(".inf", False)
            if ErrorCode != 0:
                EdkLogger.error("build", ErrorCode, ExtraData=ErrorInfo)

        if Option.PlatformFile != None:
            Option.PlatformFile = PathClass(Option.PlatformFile, Workspace)
            ErrorCode, ErrorInfo = Option.PlatformFile.Validate(".dsc", False)
            if ErrorCode != 0:
                EdkLogger.error("build", ErrorCode, ExtraData=ErrorInfo)

        if Option.FdfFile != None:
            Option.FdfFile = PathClass(Option.FdfFile, Workspace)
            ErrorCode, ErrorInfo = Option.FdfFile.Validate(".fdf", False)
            if ErrorCode != 0:
                EdkLogger.error("build", ErrorCode, ExtraData=ErrorInfo)

        MyBuild = Build(Target, Workspace, Option.PlatformFile, Option.ModuleFile,
                        Option.TargetArch, Option.ToolChain, Option.BuildTarget,
                        Option.FdfFile, Option.RomImage, Option.FvImage,
                        None, Option.SilentMode, Option.ThreadNumber,
                        Option.SkipAutoGen, Option.Reparse, Option.SkuId)
        MyBuild.Launch()
        #MyBuild.DumpBuildData()
    except FatalError, X:
        if MyBuild != None:
            # for multi-thread build exits safely
            MyBuild.Relinquish()
        if Option != None and Option.debug != None:
            EdkLogger.quiet("(Python %s on %s) " % (platform.python_version(), sys.platform) + traceback.format_exc())
        ReturnCode = X.args[0]
    except Warning, X:
        # error from Fdf parser
        if MyBuild != None:
            # for multi-thread build exits safely
            MyBuild.Relinquish()
        if Option != None and Option.debug != None:
            EdkLogger.quiet("(Python %s on %s) " % (platform.python_version(), sys.platform) + traceback.format_exc())
        else:
            EdkLogger.error(X.ToolName, FORMAT_INVALID, File=X.FileName, Line=X.LineNumber, ExtraData=X.Message, RaiseError = False)
        ReturnCode = FORMAT_INVALID
    except KeyboardInterrupt:
        ReturnCode = ABORT_ERROR
        if Option != None and Option.debug != None:
            EdkLogger.quiet("(Python %s on %s) " % (platform.python_version(), sys.platform) + traceback.format_exc())
    except:
        if MyBuild != None:
            # for multi-thread build exits safely
            MyBuild.Relinquish()

        # try to get the meta-file from the object causing exception
        Tb = sys.exc_info()[-1]
        MetaFile = GlobalData.gProcessingFile
        while Tb != None:
            if 'self' in Tb.tb_frame.f_locals and hasattr(Tb.tb_frame.f_locals['self'], 'MetaFile'):
                MetaFile = Tb.tb_frame.f_locals['self'].MetaFile
            Tb = Tb.tb_next
        EdkLogger.error(
                    "\nbuild",
                    CODE_ERROR,
                    "Unknown fatal error when processing [%s]" % MetaFile,
                    ExtraData="\n(Please send email to dev@buildtools.tianocore.org for help, attaching following call stack trace!)\n",
                    RaiseError=False
                    )
        EdkLogger.quiet("(Python %s on %s) " % (platform.python_version(), sys.platform) + traceback.format_exc())
        ReturnCode = CODE_ERROR
    finally:
        Utils.Progressor.Abort()

    if MyBuild != None:
        MyBuild.Db.Close()

    if ReturnCode == 0:
        Conclusion = "Done"
    elif ReturnCode == ABORT_ERROR:
        Conclusion = "Aborted"
    else:
        Conclusion = "Failed"
    FinishTime = time.time()
    BuildDuration = time.strftime("%M:%S", time.gmtime(int(round(FinishTime - StartTime))))
    EdkLogger.SetLevel(EdkLogger.QUIET)
    EdkLogger.quiet("\n- %s -\n%s [%s]" % (Conclusion, time.strftime("%H:%M:%S, %b.%d %Y", time.localtime()), BuildDuration))

    return ReturnCode

if __name__ == '__main__':
    r = Main()
    ## 0-127 is a safe return range, and 1 is a standard default error
    if r < 0 or r > 127: r = 1
    sys.exit(r)

