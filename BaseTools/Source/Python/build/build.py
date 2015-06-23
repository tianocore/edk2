## @file
# build a platform or a module
#
#  Copyright (c) 2014, Hewlett-Packard Development Company, L.P.<BR>
#  Copyright (c) 2007 - 2015, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
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
import Common.LongFilePathOs as os
import re
import StringIO
import sys
import glob
import time
import platform
import traceback
import encodings.ascii

from struct import *
from threading import *
from optparse import OptionParser
from subprocess import *
from Common import Misc as Utils

from Common.LongFilePathSupport import OpenLongFilePath as open
from Common.LongFilePathSupport import LongFilePath
from Common.TargetTxtClassObject import *
from Common.ToolDefClassObject import *
from Common.DataType import *
from Common.BuildVersion import gBUILD_VERSION
from AutoGen.AutoGen import *
from Common.BuildToolError import *
from Workspace.WorkspaceDatabase import *

from BuildReport import BuildReport
from GenPatchPcdTable.GenPatchPcdTable import *
from PatchPcdValue.PatchPcdValue import *

import Common.EdkLogger
import Common.GlobalData as GlobalData

# Version and Copyright
VersionNumber = "0.60" + ' ' + gBUILD_VERSION
__version__ = "%prog Version " + VersionNumber
__copyright__ = "Copyright (c) 2007 - 2014, Intel Corporation  All rights reserved."

## standard targets of build command
gSupportedTarget = ['all', 'genc', 'genmake', 'modules', 'libraries', 'fds', 'clean', 'cleanall', 'cleanlib', 'run']

## build configuration file
gBuildConfiguration = "target.txt"
gToolsDefinition = "tools_def.txt"

TemporaryTablePattern = re.compile(r'^_\d+_\d+_[a-fA-F0-9]+$')
TmpTableDict = {}

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
    # Check EFI_SOURCE (Edk build convention). EDK_SOURCE will always point to ECP
    #
    if "ECP_SOURCE" not in os.environ:
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
        EdkLogger.verbose("ECP_SOURCE = %s doesn't exist. Edk modules could not be built." % EcpSourceDir)
    elif ' ' in EcpSourceDir:
        EdkLogger.error("build", FORMAT_NOT_SUPPORTED, "No space is allowed in ECP_SOURCE path",
                        ExtraData=EcpSourceDir)
    if not os.path.exists(EdkSourceDir):
        if EdkSourceDir == EcpSourceDir:
            EdkLogger.verbose("EDK_SOURCE = %s doesn't exist. Edk modules could not be built." % EdkSourceDir)
        else:
            EdkLogger.error("build", PARAMETER_INVALID, "EDK_SOURCE does not exist",
                            ExtraData=EdkSourceDir)
    elif ' ' in EdkSourceDir:
        EdkLogger.error("build", FORMAT_NOT_SUPPORTED, "No space is allowed in EDK_SOURCE path",
                        ExtraData=EdkSourceDir)
    if not os.path.exists(EfiSourceDir):
        if EfiSourceDir == EcpSourceDir:
            EdkLogger.verbose("EFI_SOURCE = %s doesn't exist. Edk modules could not be built." % EfiSourceDir)
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

    GlobalData.gWorkspace = WorkspaceDir
    GlobalData.gEfiSource = EfiSourceDir
    GlobalData.gEdkSource = EdkSourceDir
    GlobalData.gEcpSource = EcpSourceDir

    GlobalData.gGlobalDefines["WORKSPACE"]  = WorkspaceDir
    GlobalData.gGlobalDefines["EFI_SOURCE"] = EfiSourceDir
    GlobalData.gGlobalDefines["EDK_SOURCE"] = EdkSourceDir
    GlobalData.gGlobalDefines["ECP_SOURCE"] = EcpSourceDir
    GlobalData.gGlobalDefines["EDK_TOOLS_PATH"] = os.environ["EDK_TOOLS_PATH"]

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
    
    # Command is used as the first Argument in following Popen().
    # It could be a string or sequence. We find that if command is a string in following Popen(),
    # ubuntu may fail with an error message that the command is not found.
    # So here we may need convert command from string to list instance.
    if not isinstance(Command, list):
        if platform.system() != 'Windows':
            Command = Command.split()

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
        if not BuildCommand:
            EdkLogger.error("build", OPTION_MISSING,
                            "No build command found for this module. "
                            "Please check your setting of %s_%s_%s_MAKE_PATH in Conf/tools_def.txt file." %
                                (Obj.BuildTarget, Obj.ToolChain, Obj.Arch),
                            ExtraData=str(Obj))


    ## str() method
    #
    #   It just returns the string representation of self.BuildObject
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
            if not Dep.BuildObject.IsBinaryModule:
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

## The class contains the information related to EFI image
#
class PeImageInfo():
    ## Constructor
    #
    # Constructor will load all required image information.
    #
    #   @param  BaseName          The full file path of image.
    #   @param  Guid              The GUID for image.
    #   @param  Arch              Arch of this image.
    #   @param  OutputDir         The output directory for image.
    #   @param  DebugDir          The debug directory for image.
    #   @param  ImageClass        PeImage Information
    #
    def __init__(self, BaseName, Guid, Arch, OutputDir, DebugDir, ImageClass):
        self.BaseName         = BaseName
        self.Guid             = Guid
        self.Arch             = Arch
        self.OutputDir        = OutputDir
        self.DebugDir         = DebugDir
        self.Image            = ImageClass
        self.Image.Size       = (self.Image.Size / 0x1000 + 1) * 0x1000

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
    #   @param  BuildOptions        Build options passed from command line
    #
    def __init__(self, Target, WorkspaceDir, BuildOptions):
        self.WorkspaceDir   = WorkspaceDir
        self.Target         = Target
        self.PlatformFile   = BuildOptions.PlatformFile
        self.ModuleFile     = BuildOptions.ModuleFile
        self.ArchList       = BuildOptions.TargetArch
        self.ToolChainList  = BuildOptions.ToolChain
        self.BuildTargetList= BuildOptions.BuildTarget
        self.Fdf            = BuildOptions.FdfFile
        self.FdList         = BuildOptions.RomImage
        self.FvList         = BuildOptions.FvImage
        self.CapList        = BuildOptions.CapName
        self.SilentMode     = BuildOptions.SilentMode
        self.ThreadNumber   = BuildOptions.ThreadNumber
        self.SkipAutoGen    = BuildOptions.SkipAutoGen
        self.Reparse        = BuildOptions.Reparse
        self.SkuId          = BuildOptions.SkuId
        self.ConfDirectory = BuildOptions.ConfDirectory
        self.SpawnMode      = True
        self.BuildReport    = BuildReport(BuildOptions.ReportFile, BuildOptions.ReportType)
        self.TargetTxt      = TargetTxtClassObject()
        self.ToolDef        = ToolDefClassObject()
        #Set global flag for build mode
        GlobalData.gIgnoreSource = BuildOptions.IgnoreSources

        if self.ConfDirectory:
            # Get alternate Conf location, if it is absolute, then just use the absolute directory name
            ConfDirectoryPath = os.path.normpath(self.ConfDirectory)

            if not os.path.isabs(ConfDirectoryPath):
                # Since alternate directory name is not absolute, the alternate directory is located within the WORKSPACE
                # This also handles someone specifying the Conf directory in the workspace. Using --conf=Conf
                ConfDirectoryPath = os.path.join(self.WorkspaceDir, ConfDirectoryPath)
        else:
            # Get standard WORKSPACE/Conf use the absolute path to the WORKSPACE/Conf
            ConfDirectoryPath = os.path.join(self.WorkspaceDir, 'Conf')
        GlobalData.gConfDirectory = ConfDirectoryPath
        GlobalData.gDatabasePath = os.path.normpath(os.path.join(ConfDirectoryPath, GlobalData.gDatabasePath))

        if BuildOptions.DisableCache:
            self.Db         = WorkspaceDatabase(":memory:")
        else:
            self.Db = WorkspaceDatabase(GlobalData.gDatabasePath, self.Reparse)
        self.BuildDatabase = self.Db.BuildObject
        self.Platform = None
        self.LoadFixAddress = 0
        self.UniFlag        = BuildOptions.Flag
        self.BuildModules = []

        # print dot character during doing some time-consuming work
        self.Progress = Utils.Progressor()

        self.InitBuild()

        # print current build environment and configuration
        EdkLogger.quiet("%-16s = %s" % ("WORKSPACE", os.environ["WORKSPACE"]))
        EdkLogger.quiet("%-16s = %s" % ("ECP_SOURCE", os.environ["ECP_SOURCE"]))
        EdkLogger.quiet("%-16s = %s" % ("EDK_SOURCE", os.environ["EDK_SOURCE"]))
        EdkLogger.quiet("%-16s = %s" % ("EFI_SOURCE", os.environ["EFI_SOURCE"]))
        EdkLogger.quiet("%-16s = %s" % ("EDK_TOOLS_PATH", os.environ["EDK_TOOLS_PATH"]))

        EdkLogger.info("")

        os.chdir(self.WorkspaceDir)

    ## Load configuration
    #
    #   This method will parse target.txt and get the build configurations.
    #
    def LoadConfiguration(self):
        #
        # Check target.txt and tools_def.txt and Init them
        #
        BuildConfigurationFile = os.path.normpath(os.path.join(GlobalData.gConfDirectory, gBuildConfiguration))
        if os.path.isfile(BuildConfigurationFile) == True:
            StatusCode = self.TargetTxt.LoadTargetTxtFile(BuildConfigurationFile)

            ToolDefinitionFile = self.TargetTxt.TargetTxtDictionary[DataType.TAB_TAT_DEFINES_TOOL_CHAIN_CONF]
            if ToolDefinitionFile == '':
                ToolDefinitionFile = gToolsDefinition
                ToolDefinitionFile = os.path.normpath(os.path.join(self.WorkspaceDir, 'Conf', ToolDefinitionFile))
            if os.path.isfile(ToolDefinitionFile) == True:
                StatusCode = self.ToolDef.LoadToolDefFile(ToolDefinitionFile)
            else:
                EdkLogger.error("build", FILE_NOT_FOUND, ExtraData=ToolDefinitionFile)
        else:
            EdkLogger.error("build", FILE_NOT_FOUND, ExtraData=BuildConfigurationFile)

        # if no ARCH given in command line, get it from target.txt
        if not self.ArchList:
            self.ArchList = self.TargetTxt.TargetTxtDictionary[DataType.TAB_TAT_DEFINES_TARGET_ARCH]
        self.ArchList = tuple(self.ArchList)

        # if no build target given in command line, get it from target.txt
        if not self.BuildTargetList:
            self.BuildTargetList = self.TargetTxt.TargetTxtDictionary[DataType.TAB_TAT_DEFINES_TARGET]

        # if no tool chain given in command line, get it from target.txt
        if not self.ToolChainList:
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

    ## Initialize build configuration
    #
    #   This method will parse DSC file and merge the configurations from
    #   command line and target.txt, then get the final build configurations.
    #
    def InitBuild(self):
        # parse target.txt, tools_def.txt, and platform file
        self.LoadConfiguration()

        # Allow case-insensitive for those from command line or configuration file
        ErrorCode, ErrorInfo = self.PlatformFile.Validate(".dsc", False)
        if ErrorCode != 0:
            EdkLogger.error("build", ErrorCode, ExtraData=ErrorInfo)

        # create metafile database
        self.Db.InitDatabase()

    ## Build a module or platform
    #
    # Create autogen code and makefile for a module or platform, and the launch
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
    def _BuildPa(self, Target, AutoGenObject, CreateDepsCodeFile=True, CreateDepsMakeFile=True, BuildModule=False):
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
            EdkLogger.error("build", OPTION_MISSING,
                            "No build command found for this module. "
                            "Please check your setting of %s_%s_%s_MAKE_PATH in Conf/tools_def.txt file." %
                                (AutoGenObject.BuildTarget, AutoGenObject.ToolChain, AutoGenObject.Arch),
                            ExtraData=str(AutoGenObject))

        makefile = GenMake.BuildFile(AutoGenObject)._FILE_NAME_[GenMake.gMakeType]

        # run
        if Target == 'run':
            RunDir = os.path.normpath(os.path.join(AutoGenObject.BuildDir, GlobalData.gGlobalDefines['ARCH']))
            Command = '.\SecMain'
            os.chdir(RunDir)
            LaunchCommand(Command, RunDir)
            return True

        # build modules
        if BuildModule:
            BuildCommand = BuildCommand + [Target]
            LaunchCommand(BuildCommand, AutoGenObject.MakeFileDir)
            self.CreateAsBuiltInf()
            return True

        # build library
        if Target == 'libraries':
            for Lib in AutoGenObject.LibraryBuildDirectoryList:
                NewBuildCommand = BuildCommand + ['-f', os.path.normpath(os.path.join(Lib, makefile)), 'pbuild']
                LaunchCommand(NewBuildCommand, AutoGenObject.MakeFileDir)
            return True

        # build module
        if Target == 'modules':
            for Lib in AutoGenObject.LibraryBuildDirectoryList:
                NewBuildCommand = BuildCommand + ['-f', os.path.normpath(os.path.join(Lib, makefile)), 'pbuild']
                LaunchCommand(NewBuildCommand, AutoGenObject.MakeFileDir)
            for Mod in AutoGenObject.ModuleBuildDirectoryList:
                NewBuildCommand = BuildCommand + ['-f', os.path.normpath(os.path.join(Mod, makefile)), 'pbuild']
                LaunchCommand(NewBuildCommand, AutoGenObject.MakeFileDir)
            self.CreateAsBuiltInf()
            return True

        # cleanlib
        if Target == 'cleanlib':
            for Lib in AutoGenObject.LibraryBuildDirectoryList:
                LibMakefile = os.path.normpath(os.path.join(Lib, makefile))
                if os.path.exists(LibMakefile):
                    NewBuildCommand = BuildCommand + ['-f', LibMakefile, 'cleanall']
                    LaunchCommand(NewBuildCommand, AutoGenObject.MakeFileDir)
            return True

        # clean
        if Target == 'clean':
            for Mod in AutoGenObject.ModuleBuildDirectoryList:
                ModMakefile = os.path.normpath(os.path.join(Mod, makefile))
                if os.path.exists(ModMakefile):
                    NewBuildCommand = BuildCommand + ['-f', ModMakefile, 'cleanall']
                    LaunchCommand(NewBuildCommand, AutoGenObject.MakeFileDir)
            for Lib in AutoGenObject.LibraryBuildDirectoryList:
                LibMakefile = os.path.normpath(os.path.join(Lib, makefile))
                if os.path.exists(LibMakefile):
                    NewBuildCommand = BuildCommand + ['-f', LibMakefile, 'cleanall']
                    LaunchCommand(NewBuildCommand, AutoGenObject.MakeFileDir)
            return True

        # cleanall
        if Target == 'cleanall':
            try:
                #os.rmdir(AutoGenObject.BuildDir)
                RemoveDirectory(AutoGenObject.BuildDir, True)
            except WindowsError, X:
                EdkLogger.error("build", FILE_DELETE_FAILURE, ExtraData=str(X))
        return True

    ## Build a module or platform
    #
    # Create autogen code and makefile for a module or platform, and the launch
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
    def _Build(self, Target, AutoGenObject, CreateDepsCodeFile=True, CreateDepsMakeFile=True, BuildModule=False):
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
                #AutoGenObject.CreateAsBuiltInf()
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
            EdkLogger.error("build", OPTION_MISSING,
                            "No build command found for this module. "
                            "Please check your setting of %s_%s_%s_MAKE_PATH in Conf/tools_def.txt file." %
                                (AutoGenObject.BuildTarget, AutoGenObject.ToolChain, AutoGenObject.Arch),
                            ExtraData=str(AutoGenObject))

        # build modules
        if BuildModule:
            if Target != 'fds':
                BuildCommand = BuildCommand + [Target]
            LaunchCommand(BuildCommand, AutoGenObject.MakeFileDir)
            self.CreateAsBuiltInf()
            return True

        # genfds
        if Target == 'fds':
            LaunchCommand(AutoGenObject.GenFdsCommand, AutoGenObject.MakeFileDir)
            return True

        # run
        if Target == 'run':
            RunDir = os.path.normpath(os.path.join(AutoGenObject.BuildDir, GlobalData.gGlobalDefines['ARCH']))
            Command = '.\SecMain'
            os.chdir(RunDir)
            LaunchCommand(Command, RunDir)
            return True

        # build library
        if Target == 'libraries':
            pass

        # not build modules


        # cleanall
        if Target == 'cleanall':
            try:
                #os.rmdir(AutoGenObject.BuildDir)
                RemoveDirectory(AutoGenObject.BuildDir, True)
            except WindowsError, X:
                EdkLogger.error("build", FILE_DELETE_FAILURE, ExtraData=str(X))
        return True

    ## Rebase module image and Get function address for the input module list.
    #
    def _RebaseModule (self, MapBuffer, BaseAddress, ModuleList, AddrIsOffset = True, ModeIsSmm = False):
        if ModeIsSmm:
            AddrIsOffset = False
        InfFileNameList = ModuleList.keys()
        #InfFileNameList.sort()
        for InfFile in InfFileNameList:
            sys.stdout.write (".")
            sys.stdout.flush()
            ModuleInfo = ModuleList[InfFile]
            ModuleName = ModuleInfo.BaseName
            ModuleOutputImage = ModuleInfo.Image.FileName
            ModuleDebugImage  = os.path.join(ModuleInfo.DebugDir, ModuleInfo.BaseName + '.efi')
            ## for SMM module in SMRAM, the SMRAM will be allocated from base to top.
            if not ModeIsSmm:
                BaseAddress = BaseAddress - ModuleInfo.Image.Size
                #
                # Update Image to new BaseAddress by GenFw tool
                #
                LaunchCommand(["GenFw", "--rebase", str(BaseAddress), "-r", ModuleOutputImage], ModuleInfo.OutputDir)
                LaunchCommand(["GenFw", "--rebase", str(BaseAddress), "-r", ModuleDebugImage],  ModuleInfo.DebugDir)
            else:
                #
                # Set new address to the section header only for SMM driver.
                #
                LaunchCommand(["GenFw", "--address", str(BaseAddress), "-r", ModuleOutputImage], ModuleInfo.OutputDir)
                LaunchCommand(["GenFw", "--address", str(BaseAddress), "-r", ModuleDebugImage],  ModuleInfo.DebugDir)
            #
            # Collect funtion address from Map file
            #
            ImageMapTable = ModuleOutputImage.replace('.efi', '.map')
            FunctionList = []
            if os.path.exists(ImageMapTable):
                OrigImageBaseAddress = 0
                ImageMap = open (ImageMapTable, 'r')
                for LinStr in ImageMap:
                    if len (LinStr.strip()) == 0:
                        continue
                    #
                    # Get the preferred address set on link time.
                    #
                    if LinStr.find ('Preferred load address is') != -1:
                        StrList = LinStr.split()
                        OrigImageBaseAddress = int (StrList[len(StrList) - 1], 16)

                    StrList = LinStr.split()
                    if len (StrList) > 4:
                        if StrList[3] == 'f' or StrList[3] =='F':
                            Name = StrList[1]
                            RelativeAddress = int (StrList[2], 16) - OrigImageBaseAddress
                            FunctionList.append ((Name, RelativeAddress))
                            if ModuleInfo.Arch == 'IPF' and Name.endswith('_ModuleEntryPoint'):
                                #
                                # Get the real entry point address for IPF image.
                                #
                                ModuleInfo.Image.EntryPoint = RelativeAddress
                ImageMap.close()
            #
            # Add general information.
            #
            if ModeIsSmm:
                MapBuffer.write('\n\n%s (Fixed SMRAM Offset,   BaseAddress=0x%010X,  EntryPoint=0x%010X)\n' % (ModuleName, BaseAddress, BaseAddress + ModuleInfo.Image.EntryPoint))
            elif AddrIsOffset:
                MapBuffer.write('\n\n%s (Fixed Memory Offset,  BaseAddress=-0x%010X, EntryPoint=-0x%010X)\n' % (ModuleName, 0 - BaseAddress, 0 - (BaseAddress + ModuleInfo.Image.EntryPoint)))
            else:
                MapBuffer.write('\n\n%s (Fixed Memory Address, BaseAddress=0x%010X,  EntryPoint=0x%010X)\n' % (ModuleName, BaseAddress, BaseAddress + ModuleInfo.Image.EntryPoint))
            #
            # Add guid and general seciton section.
            #
            TextSectionAddress = 0
            DataSectionAddress = 0
            for SectionHeader in ModuleInfo.Image.SectionHeaderList:
                if SectionHeader[0] == '.text':
                    TextSectionAddress = SectionHeader[1]
                elif SectionHeader[0] in ['.data', '.sdata']:
                    DataSectionAddress = SectionHeader[1]
            if AddrIsOffset:
                MapBuffer.write('(GUID=%s, .textbaseaddress=-0x%010X, .databaseaddress=-0x%010X)\n' % (ModuleInfo.Guid, 0 - (BaseAddress + TextSectionAddress), 0 - (BaseAddress + DataSectionAddress)))
            else:
                MapBuffer.write('(GUID=%s, .textbaseaddress=0x%010X, .databaseaddress=0x%010X)\n' % (ModuleInfo.Guid, BaseAddress + TextSectionAddress, BaseAddress + DataSectionAddress))
            #
            # Add debug image full path.
            #
            MapBuffer.write('(IMAGE=%s)\n\n' % (ModuleDebugImage))
            #
            # Add funtion address
            #
            for Function in FunctionList:
                if AddrIsOffset:
                    MapBuffer.write('  -0x%010X    %s\n' % (0 - (BaseAddress + Function[1]), Function[0]))
                else:
                    MapBuffer.write('  0x%010X    %s\n' % (BaseAddress + Function[1], Function[0]))
            ImageMap.close()

            #
            # for SMM module in SMRAM, the SMRAM will be allocated from base to top.
            #
            if ModeIsSmm:
                BaseAddress = BaseAddress + ModuleInfo.Image.Size

    ## Collect MAP information of all FVs
    #
    def _CollectFvMapBuffer (self, MapBuffer, Wa, ModuleList):
        if self.Fdf:
            # First get the XIP base address for FV map file.
            GuidPattern = re.compile("[-a-fA-F0-9]+")
            GuidName = re.compile("\(GUID=[-a-fA-F0-9]+")
            for FvName in Wa.FdfProfile.FvDict.keys():
                FvMapBuffer = os.path.join(Wa.FvDir, FvName + '.Fv.map')
                if not os.path.exists(FvMapBuffer):
                    continue
                FvMap = open(FvMapBuffer, 'r')
                #skip FV size information
                FvMap.readline()
                FvMap.readline()
                FvMap.readline()
                FvMap.readline()
                for Line in FvMap:
                    MatchGuid = GuidPattern.match(Line)
                    if MatchGuid != None:
                        #
                        # Replace GUID with module name
                        #
                        GuidString = MatchGuid.group()
                        if GuidString.upper() in ModuleList:
                            Line = Line.replace(GuidString, ModuleList[GuidString.upper()].Name)
                    MapBuffer.write('%s' % (Line))
                    #
                    # Add the debug image full path.
                    #
                    MatchGuid = GuidName.match(Line)
                    if MatchGuid != None:
                        GuidString = MatchGuid.group().split("=")[1]
                        if GuidString.upper() in ModuleList:
                            MapBuffer.write('(IMAGE=%s)\n' % (os.path.join(ModuleList[GuidString.upper()].DebugDir, ModuleList[GuidString.upper()].Name + '.efi')))

                FvMap.close()

    ## Collect MAP information of all modules
    #
    def _CollectModuleMapBuffer (self, MapBuffer, ModuleList):
        sys.stdout.write ("Generate Load Module At Fix Address Map")
        sys.stdout.flush()
        PatchEfiImageList = []
        PeiModuleList  = {}
        BtModuleList   = {}
        RtModuleList   = {}
        SmmModuleList  = {}
        PeiSize = 0
        BtSize  = 0
        RtSize  = 0
        # reserve 4K size in SMRAM to make SMM module address not from 0.
        SmmSize = 0x1000
        IsIpfPlatform = False
        if 'IPF' in self.ArchList:
            IsIpfPlatform = True
        for ModuleGuid in ModuleList:
            Module = ModuleList[ModuleGuid]
            GlobalData.gProcessingFile = "%s [%s, %s, %s]" % (Module.MetaFile, Module.Arch, Module.ToolChain, Module.BuildTarget)

            OutputImageFile = ''
            for ResultFile in Module.CodaTargetList:
                if str(ResultFile.Target).endswith('.efi'):
                    #
                    # module list for PEI, DXE, RUNTIME and SMM
                    #
                    OutputImageFile = os.path.join(Module.OutputDir, Module.Name + '.efi')
                    ImageClass = PeImageClass (OutputImageFile)
                    if not ImageClass.IsValid:
                        EdkLogger.error("build", FILE_PARSE_FAILURE, ExtraData=ImageClass.ErrorInfo)
                    ImageInfo = PeImageInfo(Module.Name, Module.Guid, Module.Arch, Module.OutputDir, Module.DebugDir, ImageClass)
                    if Module.ModuleType in ['PEI_CORE', 'PEIM', 'COMBINED_PEIM_DRIVER','PIC_PEIM', 'RELOCATABLE_PEIM', 'DXE_CORE']:
                        PeiModuleList[Module.MetaFile] = ImageInfo
                        PeiSize += ImageInfo.Image.Size
                    elif Module.ModuleType in ['BS_DRIVER', 'DXE_DRIVER', 'UEFI_DRIVER']:
                        BtModuleList[Module.MetaFile] = ImageInfo
                        BtSize += ImageInfo.Image.Size
                    elif Module.ModuleType in ['DXE_RUNTIME_DRIVER', 'RT_DRIVER', 'DXE_SAL_DRIVER', 'SAL_RT_DRIVER']:
                        RtModuleList[Module.MetaFile] = ImageInfo
                        #IPF runtime driver needs to be at 2 page alignment.
                        if IsIpfPlatform and ImageInfo.Image.Size % 0x2000 != 0:
                            ImageInfo.Image.Size = (ImageInfo.Image.Size / 0x2000 + 1) * 0x2000
                        RtSize += ImageInfo.Image.Size
                    elif Module.ModuleType in ['SMM_CORE', 'DXE_SMM_DRIVER']:
                        SmmModuleList[Module.MetaFile] = ImageInfo
                        SmmSize += ImageInfo.Image.Size
                        if Module.ModuleType == 'DXE_SMM_DRIVER':
                            PiSpecVersion = '0x00000000'
                            if 'PI_SPECIFICATION_VERSION' in Module.Module.Specification:
                                PiSpecVersion = Module.Module.Specification['PI_SPECIFICATION_VERSION']
                            # for PI specification < PI1.1, DXE_SMM_DRIVER also runs as BOOT time driver.
                            if int(PiSpecVersion, 16) < 0x0001000A:
                                BtModuleList[Module.MetaFile] = ImageInfo
                                BtSize += ImageInfo.Image.Size
                    break
            #
            # EFI image is final target.
            # Check EFI image contains patchable FixAddress related PCDs.
            #
            if OutputImageFile != '':
                ModuleIsPatch = False
                for Pcd in Module.ModulePcdList:
                    if Pcd.Type == TAB_PCDS_PATCHABLE_IN_MODULE and Pcd.TokenCName in TAB_PCDS_PATCHABLE_LOAD_FIX_ADDRESS_LIST:
                        ModuleIsPatch = True
                        break
                if not ModuleIsPatch:
                    for Pcd in Module.LibraryPcdList:
                        if Pcd.Type == TAB_PCDS_PATCHABLE_IN_MODULE and Pcd.TokenCName in TAB_PCDS_PATCHABLE_LOAD_FIX_ADDRESS_LIST:
                            ModuleIsPatch = True
                            break

                if not ModuleIsPatch:
                    continue
                #
                # Module includes the patchable load fix address PCDs.
                # It will be fixed up later.
                #
                PatchEfiImageList.append (OutputImageFile)

        #
        # Get Top Memory address
        #
        ReservedRuntimeMemorySize = 0
        TopMemoryAddress = 0
        if self.LoadFixAddress == 0xFFFFFFFFFFFFFFFF:
            TopMemoryAddress = 0
        else:
            TopMemoryAddress = self.LoadFixAddress
            if TopMemoryAddress < RtSize + BtSize + PeiSize:
                EdkLogger.error("build", PARAMETER_INVALID, "FIX_LOAD_TOP_MEMORY_ADDRESS is too low to load driver")
            # Make IPF runtime driver at 2 page alignment.
            if IsIpfPlatform:
                ReservedRuntimeMemorySize = TopMemoryAddress % 0x2000
                RtSize = RtSize + ReservedRuntimeMemorySize

        #
        # Patch FixAddress related PCDs into EFI image
        #
        for EfiImage in PatchEfiImageList:
            EfiImageMap = EfiImage.replace('.efi', '.map')
            if not os.path.exists(EfiImageMap):
                continue
            #
            # Get PCD offset in EFI image by GenPatchPcdTable function
            #
            PcdTable = parsePcdInfoFromMapFile(EfiImageMap, EfiImage)
            #
            # Patch real PCD value by PatchPcdValue tool
            #
            for PcdInfo in PcdTable:
                ReturnValue = 0
                if PcdInfo[0] == TAB_PCDS_PATCHABLE_LOAD_FIX_ADDRESS_PEI_PAGE_SIZE:
                    ReturnValue, ErrorInfo = PatchBinaryFile (EfiImage, PcdInfo[1], TAB_PCDS_PATCHABLE_LOAD_FIX_ADDRESS_PEI_PAGE_SIZE_DATA_TYPE, str (PeiSize/0x1000))
                elif PcdInfo[0] == TAB_PCDS_PATCHABLE_LOAD_FIX_ADDRESS_DXE_PAGE_SIZE:
                    ReturnValue, ErrorInfo = PatchBinaryFile (EfiImage, PcdInfo[1], TAB_PCDS_PATCHABLE_LOAD_FIX_ADDRESS_DXE_PAGE_SIZE_DATA_TYPE, str (BtSize/0x1000))
                elif PcdInfo[0] == TAB_PCDS_PATCHABLE_LOAD_FIX_ADDRESS_RUNTIME_PAGE_SIZE:
                    ReturnValue, ErrorInfo = PatchBinaryFile (EfiImage, PcdInfo[1], TAB_PCDS_PATCHABLE_LOAD_FIX_ADDRESS_RUNTIME_PAGE_SIZE_DATA_TYPE, str (RtSize/0x1000))
                elif PcdInfo[0] == TAB_PCDS_PATCHABLE_LOAD_FIX_ADDRESS_SMM_PAGE_SIZE and len (SmmModuleList) > 0:
                    ReturnValue, ErrorInfo = PatchBinaryFile (EfiImage, PcdInfo[1], TAB_PCDS_PATCHABLE_LOAD_FIX_ADDRESS_SMM_PAGE_SIZE_DATA_TYPE, str (SmmSize/0x1000))
                if ReturnValue != 0:
                    EdkLogger.error("build", PARAMETER_INVALID, "Patch PCD value failed", ExtraData=ErrorInfo)

        MapBuffer.write('PEI_CODE_PAGE_NUMBER      = 0x%x\n' % (PeiSize/0x1000))
        MapBuffer.write('BOOT_CODE_PAGE_NUMBER     = 0x%x\n' % (BtSize/0x1000))
        MapBuffer.write('RUNTIME_CODE_PAGE_NUMBER  = 0x%x\n' % (RtSize/0x1000))
        if len (SmmModuleList) > 0:
            MapBuffer.write('SMM_CODE_PAGE_NUMBER      = 0x%x\n' % (SmmSize/0x1000))

        PeiBaseAddr = TopMemoryAddress - RtSize - BtSize
        BtBaseAddr  = TopMemoryAddress - RtSize
        RtBaseAddr  = TopMemoryAddress - ReservedRuntimeMemorySize

        self._RebaseModule (MapBuffer, PeiBaseAddr, PeiModuleList, TopMemoryAddress == 0)
        self._RebaseModule (MapBuffer, BtBaseAddr, BtModuleList, TopMemoryAddress == 0)
        self._RebaseModule (MapBuffer, RtBaseAddr, RtModuleList, TopMemoryAddress == 0)
        self._RebaseModule (MapBuffer, 0x1000, SmmModuleList, AddrIsOffset = False, ModeIsSmm = True)
        MapBuffer.write('\n\n')
        sys.stdout.write ("\n")
        sys.stdout.flush()

    ## Save platform Map file
    #
    def _SaveMapFile (self, MapBuffer, Wa):
        #
        # Map file path is got.
        #
        MapFilePath = os.path.join(Wa.BuildDir, Wa.Name + '.map')
        #
        # Save address map into MAP file.
        #
        SaveFileOnChange(MapFilePath, MapBuffer.getvalue(), False)
        MapBuffer.close()
        if self.LoadFixAddress != 0:
            sys.stdout.write ("\nLoad Module At Fix Address Map file can be found at %s\n" %(MapFilePath))
        sys.stdout.flush()

    ## Build active platform for different build targets and different tool chains
    #
    def _BuildPlatform(self):
        for BuildTarget in self.BuildTargetList:
            GlobalData.gGlobalDefines['TARGET'] = BuildTarget
            for ToolChain in self.ToolChainList:
                GlobalData.gGlobalDefines['TOOLCHAIN'] = ToolChain
                GlobalData.gGlobalDefines['TOOL_CHAIN_TAG'] = ToolChain
                Wa = WorkspaceAutoGen(
                        self.WorkspaceDir,
                        self.PlatformFile,
                        BuildTarget,
                        ToolChain,
                        self.ArchList,
                        self.BuildDatabase,
                        self.TargetTxt,
                        self.ToolDef,
                        self.Fdf,
                        self.FdList,
                        self.FvList,
                        self.CapList,
                        self.SkuId,
                        self.UniFlag,
                        self.Progress
                        )
                self.Fdf = Wa.FdfFile
                self.LoadFixAddress = Wa.Platform.LoadFixAddress
                self.BuildReport.AddPlatformReport(Wa)
                self.Progress.Stop("done!")
                for Arch in Wa.ArchList:
                    GlobalData.gGlobalDefines['ARCH'] = Arch
                    Pa = PlatformAutoGen(Wa, self.PlatformFile, BuildTarget, ToolChain, Arch)
                    for Module in Pa.Platform.Modules:
                        # Get ModuleAutoGen object to generate C code file and makefile
                        Ma = ModuleAutoGen(Wa, Module, BuildTarget, ToolChain, Arch, self.PlatformFile)
                        if Ma == None:
                            continue
                        self.BuildModules.append(Ma)
                    self._BuildPa(self.Target, Pa)

                # Create MAP file when Load Fix Address is enabled.
                if self.Target in ["", "all", "fds"]:
                    for Arch in Wa.ArchList:
                        GlobalData.gGlobalDefines['ARCH'] = Arch
                        #
                        # Check whether the set fix address is above 4G for 32bit image.
                        #
                        if (Arch == 'IA32' or Arch == 'ARM') and self.LoadFixAddress != 0xFFFFFFFFFFFFFFFF and self.LoadFixAddress >= 0x100000000:
                            EdkLogger.error("build", PARAMETER_INVALID, "FIX_LOAD_TOP_MEMORY_ADDRESS can't be set to larger than or equal to 4G for the platform with IA32 or ARM arch modules")
                    #
                    # Get Module List
                    #
                    ModuleList = {}
                    for Pa in Wa.AutoGenObjectList:
                        for Ma in Pa.ModuleAutoGenList:
                            if Ma == None:
                                continue
                            if not Ma.IsLibrary:
                                ModuleList[Ma.Guid.upper()] = Ma

                    MapBuffer = StringIO('')
                    if self.LoadFixAddress != 0:
                        #
                        # Rebase module to the preferred memory address before GenFds
                        #
                        self._CollectModuleMapBuffer(MapBuffer, ModuleList)
                    if self.Fdf:
                        #
                        # create FDS again for the updated EFI image
                        #
                        self._Build("fds", Wa)
                        #
                        # Create MAP file for all platform FVs after GenFds.
                        #
                        self._CollectFvMapBuffer(MapBuffer, Wa, ModuleList)
                    #
                    # Save MAP buffer into MAP file.
                    #
                    self._SaveMapFile (MapBuffer, Wa)

    ## Build active module for different build targets, different tool chains and different archs
    #
    def _BuildModule(self):
        for BuildTarget in self.BuildTargetList:
            GlobalData.gGlobalDefines['TARGET'] = BuildTarget
            for ToolChain in self.ToolChainList:
                GlobalData.gGlobalDefines['TOOLCHAIN'] = ToolChain
                GlobalData.gGlobalDefines['TOOL_CHAIN_TAG'] = ToolChain
                #
                # module build needs platform build information, so get platform
                # AutoGen first
                #
                Wa = WorkspaceAutoGen(
                        self.WorkspaceDir,
                        self.PlatformFile,
                        BuildTarget,
                        ToolChain,
                        self.ArchList,
                        self.BuildDatabase,
                        self.TargetTxt,
                        self.ToolDef,
                        self.Fdf,
                        self.FdList,
                        self.FvList,
                        self.CapList,
                        self.SkuId,
                        self.UniFlag,
                        self.Progress,
                        self.ModuleFile
                        )
                self.Fdf = Wa.FdfFile
                self.LoadFixAddress = Wa.Platform.LoadFixAddress
                Wa.CreateMakeFile(False)
                self.Progress.Stop("done!")
                MaList = []
                for Arch in Wa.ArchList:
                    GlobalData.gGlobalDefines['ARCH'] = Arch
                    Ma = ModuleAutoGen(Wa, self.ModuleFile, BuildTarget, ToolChain, Arch, self.PlatformFile)
                    if Ma == None: continue
                    MaList.append(Ma)
                    self.BuildModules.append(Ma)
                    if not Ma.IsBinaryModule:
                        self._Build(self.Target, Ma, BuildModule=True)

                self.BuildReport.AddPlatformReport(Wa, MaList)
                if MaList == []:
                    EdkLogger.error(
                                'build',
                                BUILD_ERROR,
                                "Module for [%s] is not a component of active platform."\
                                " Please make sure that the ARCH and inf file path are"\
                                " given in the same as in [%s]" %\
                                    (', '.join(Wa.ArchList), self.PlatformFile),
                                ExtraData=self.ModuleFile
                                )
                # Create MAP file when Load Fix Address is enabled.
                if self.Target == "fds" and self.Fdf:
                    for Arch in Wa.ArchList:
                        #
                        # Check whether the set fix address is above 4G for 32bit image.
                        #
                        if (Arch == 'IA32' or Arch == 'ARM') and self.LoadFixAddress != 0xFFFFFFFFFFFFFFFF and self.LoadFixAddress >= 0x100000000:
                            EdkLogger.error("build", PARAMETER_INVALID, "FIX_LOAD_TOP_MEMORY_ADDRESS can't be set to larger than or equal to 4G for the platorm with IA32 or ARM arch modules")
                    #
                    # Get Module List
                    #
                    ModuleList = {}
                    for Pa in Wa.AutoGenObjectList:
                        for Ma in Pa.ModuleAutoGenList:
                            if Ma == None:
                                continue
                            if not Ma.IsLibrary:
                                ModuleList[Ma.Guid.upper()] = Ma

                    MapBuffer = StringIO('')
                    if self.LoadFixAddress != 0:
                        #
                        # Rebase module to the preferred memory address before GenFds
                        #
                        self._CollectModuleMapBuffer(MapBuffer, ModuleList)
                    #
                    # create FDS again for the updated EFI image
                    #
                    self._Build("fds", Wa)
                    #
                    # Create MAP file for all platform FVs after GenFds.
                    #
                    self._CollectFvMapBuffer(MapBuffer, Wa, ModuleList)
                    #
                    # Save MAP buffer into MAP file.
                    #
                    self._SaveMapFile (MapBuffer, Wa)

    ## Build a platform in multi-thread mode
    #
    def _MultiThreadBuildPlatform(self):
        for BuildTarget in self.BuildTargetList:
            GlobalData.gGlobalDefines['TARGET'] = BuildTarget
            for ToolChain in self.ToolChainList:
                GlobalData.gGlobalDefines['TOOLCHAIN'] = ToolChain
                GlobalData.gGlobalDefines['TOOL_CHAIN_TAG'] = ToolChain
                Wa = WorkspaceAutoGen(
                        self.WorkspaceDir,
                        self.PlatformFile,
                        BuildTarget,
                        ToolChain,
                        self.ArchList,
                        self.BuildDatabase,
                        self.TargetTxt,
                        self.ToolDef,
                        self.Fdf,
                        self.FdList,
                        self.FvList,
                        self.CapList,
                        self.SkuId,
                        self.UniFlag,
                        self.Progress
                        )
                self.Fdf = Wa.FdfFile
                self.LoadFixAddress = Wa.Platform.LoadFixAddress
                self.BuildReport.AddPlatformReport(Wa)
                Wa.CreateMakeFile(False)

                # multi-thread exit flag
                ExitFlag = threading.Event()
                ExitFlag.clear()
                for Arch in Wa.ArchList:
                    GlobalData.gGlobalDefines['ARCH'] = Arch
                    Pa = PlatformAutoGen(Wa, self.PlatformFile, BuildTarget, ToolChain, Arch)
                    if Pa == None:
                        continue
                    ModuleList = []
                    for Inf in Pa.Platform.Modules:
                        ModuleList.append(Inf)
                    # Add the INF only list in FDF
                    if GlobalData.gFdfParser != None:
                        for InfName in GlobalData.gFdfParser.Profile.InfList:
                            Inf = PathClass(NormPath(InfName), self.WorkspaceDir, Arch)
                            if Inf in Pa.Platform.Modules:
                                continue
                            ModuleList.append(Inf)
                    for Module in ModuleList:
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
                        self.BuildModules.append(Ma)
                    self.Progress.Stop("done!")

                    for Ma in self.BuildModules:
                        # Generate build task for the module
                        if not Ma.IsBinaryModule:
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
                # Save temp tables to a TmpTableDict.
                #
                for Key in Wa.BuildDatabase._CACHE_:
                    if Wa.BuildDatabase._CACHE_[Key]._RawData and Wa.BuildDatabase._CACHE_[Key]._RawData._Table and Wa.BuildDatabase._CACHE_[Key]._RawData._Table.Table:
                        if TemporaryTablePattern.match(Wa.BuildDatabase._CACHE_[Key]._RawData._Table.Table):
                            TmpTableDict[Wa.BuildDatabase._CACHE_[Key]._RawData._Table.Table] = Wa.BuildDatabase._CACHE_[Key]._RawData._Table.Cur
                #
                #
                # All modules have been put in build tasks queue. Tell task scheduler
                # to exit if all tasks are completed
                #
                ExitFlag.set()
                BuildTask.WaitForComplete()
                self.CreateAsBuiltInf()

                #
                # Check for build error, and raise exception if one
                # has been signaled.
                #
                if BuildTask.HasError():
                    EdkLogger.error("build", BUILD_ERROR, "Failed to build module", ExtraData=GlobalData.gBuildingModule)

                # Create MAP file when Load Fix Address is enabled.
                if self.Target in ["", "all", "fds"]:
                    for Arch in Wa.ArchList:
                        #
                        # Check whether the set fix address is above 4G for 32bit image.
                        #
                        if (Arch == 'IA32' or Arch == 'ARM') and self.LoadFixAddress != 0xFFFFFFFFFFFFFFFF and self.LoadFixAddress >= 0x100000000:
                            EdkLogger.error("build", PARAMETER_INVALID, "FIX_LOAD_TOP_MEMORY_ADDRESS can't be set to larger than or equal to 4G for the platorm with IA32 or ARM arch modules")
                    #
                    # Get Module List
                    #
                    ModuleList = {}
                    for Pa in Wa.AutoGenObjectList:
                        for Ma in Pa.ModuleAutoGenList:
                            if Ma == None:
                                continue
                            if not Ma.IsLibrary:
                                ModuleList[Ma.Guid.upper()] = Ma
                    #
                    # Rebase module to the preferred memory address before GenFds
                    #
                    MapBuffer = StringIO('')
                    if self.LoadFixAddress != 0:
                        self._CollectModuleMapBuffer(MapBuffer, ModuleList)

                    if self.Fdf:
                        #
                        # Generate FD image if there's a FDF file found
                        #
                        LaunchCommand(Wa.GenFdsCommand, os.getcwd())

                        #
                        # Create MAP file for all platform FVs after GenFds.
                        #
                        self._CollectFvMapBuffer(MapBuffer, Wa, ModuleList)
                    #
                    # Save MAP buffer into MAP file.
                    #
                    self._SaveMapFile(MapBuffer, Wa)

    ## Generate GuidedSectionTools.txt in the FV directories.
    #
    def CreateGuidedSectionToolsFile(self):
        for BuildTarget in self.BuildTargetList:
            for ToolChain in self.ToolChainList:
                Wa = WorkspaceAutoGen(
                        self.WorkspaceDir,
                        self.PlatformFile,
                        BuildTarget,
                        ToolChain,
                        self.ArchList,
                        self.BuildDatabase,
                        self.TargetTxt,
                        self.ToolDef,
                        self.Fdf,
                        self.FdList,
                        self.FvList,
                        self.CapList,
                        self.SkuId,
                        self.UniFlag
                        )
                FvDir = Wa.FvDir
                if not os.path.exists(FvDir):
                    continue

                for Arch in self.ArchList:
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
        if not self.ModuleFile:
            if not self.SpawnMode or self.Target not in ["", "all"]:
                self.SpawnMode = False
                self._BuildPlatform()
            else:
                self._MultiThreadBuildPlatform()
            self.CreateGuidedSectionToolsFile()
        else:
            self.SpawnMode = False
            self._BuildModule()

        if self.Target == 'cleanall':
            self.Db.Close()
            RemoveDirectory(os.path.dirname(GlobalData.gDatabasePath), True)

    def CreateAsBuiltInf(self):
        for Module in self.BuildModules:
            Module.CreateAsBuiltInf()
        self.BuildModules = []
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
        CacheDirectory = os.path.dirname(GlobalData.gDatabasePath)
        Utils.CreateDirectory(CacheDirectory)
        Utils.DataDump(Utils.gFileTimeStampCache, os.path.join(CacheDirectory, "gFileTimeStampCache"))
        Utils.DataDump(Utils.gDependencyDatabase, os.path.join(CacheDirectory, "gDependencyDatabase"))

    def RestoreBuildData(self):
        FilePath = os.path.join(os.path.dirname(GlobalData.gDatabasePath), "gFileTimeStampCache")
        if Utils.gFileTimeStampCache == {} and os.path.isfile(FilePath):
            Utils.gFileTimeStampCache = Utils.DataRestore(FilePath)
            if Utils.gFileTimeStampCache == None:
                Utils.gFileTimeStampCache = {}

        FilePath = os.path.join(os.path.dirname(GlobalData.gDatabasePath), "gDependencyDatabase")
        if Utils.gDependencyDatabase == {} and os.path.isfile(FilePath):
            Utils.gDependencyDatabase = Utils.DataRestore(FilePath)
            if Utils.gDependencyDatabase == None:
                Utils.gDependencyDatabase = {}

def ParseDefines(DefineList=[]):
    DefineDict = {}
    if DefineList != None:
        for Define in DefineList:
            DefineTokenList = Define.split("=", 1)
            if not GlobalData.gMacroNamePattern.match(DefineTokenList[0]):
                EdkLogger.error('build', FORMAT_INVALID,
                                "The macro name must be in the pattern [A-Z][A-Z0-9_]*",
                                ExtraData=DefineTokenList[0])

            if len(DefineTokenList) == 1:
                DefineDict[DefineTokenList[0]] = "TRUE"
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
    Parser.add_option("-a", "--arch", action="append", type="choice", choices=['IA32','X64','IPF','EBC','ARM', 'AARCH64'], dest="TargetArch",
        help="ARCHS is one of list: IA32, X64, IPF, ARM, AARCH64 or EBC, which overrides target.txt's TARGET_ARCH definition. To specify more archs, please repeat this option.")
    Parser.add_option("-p", "--platform", action="callback", type="string", dest="PlatformFile", callback=SingleCheckCallback,
        help="Build the platform specified by the DSC file name argument, overriding target.txt's ACTIVE_PLATFORM definition.")
    Parser.add_option("-m", "--module", action="callback", type="string", dest="ModuleFile", callback=SingleCheckCallback,
        help="Build the module specified by the INF file name argument.")
    Parser.add_option("-b", "--buildtarget", type="string", dest="BuildTarget", help="Using the TARGET to build the platform, overriding target.txt's TARGET definition.",
                      action="append")
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
    Parser.add_option("-C", "--capsule-image", action="append", type="string", dest="CapName", default=[],
        help="The name of Capsule to be generated. The name must be from [Capsule] section in FDF file.")
    Parser.add_option("-u", "--skip-autogen", action="store_true", dest="SkipAutoGen", help="Skip AutoGen step.")
    Parser.add_option("-e", "--re-parse", action="store_true", dest="Reparse", help="Re-parse all meta-data files.")

    Parser.add_option("-c", "--case-insensitive", action="store_true", dest="CaseInsensitive", default=False, help="Don't check case of file name.")

    Parser.add_option("-w", "--warning-as-error", action="store_true", dest="WarningAsError", help="Treat warning in tools as error.")
    Parser.add_option("-j", "--log", action="store", dest="LogFile", help="Put log in specified file as well as on console.")

    Parser.add_option("-s", "--silent", action="store_true", type=None, dest="SilentMode",
        help="Make use of silent mode of (n)make.")
    Parser.add_option("-q", "--quiet", action="store_true", type=None, help="Disable all messages except FATAL ERRORS.")
    Parser.add_option("-v", "--verbose", action="store_true", type=None, help="Turn on verbose output with informational messages printed, "\
                                                                               "including library instances selected, final dependency expression, "\
                                                                               "and warning messages, etc.")
    Parser.add_option("-d", "--debug", action="store", type="int", help="Enable debug messages at specified level.")
    Parser.add_option("-D", "--define", action="append", type="string", dest="Macros", help="Macro: \"Name [= Value]\".")

    Parser.add_option("-y", "--report-file", action="store", dest="ReportFile", help="Create/overwrite the report to the specified filename.")
    Parser.add_option("-Y", "--report-type", action="append", type="choice", choices=['PCD','LIBRARY','FLASH','DEPEX','BUILD_FLAGS','FIXED_ADDRESS', 'EXECUTION_ORDER'], dest="ReportType", default=[],
        help="Flags that control the type of build report to generate.  Must be one of: [PCD, LIBRARY, FLASH, DEPEX, BUILD_FLAGS, FIXED_ADDRESS, EXECUTION_ORDER].  "\
             "To specify more than one flag, repeat this option on the command line and the default flag set is [PCD, LIBRARY, FLASH, DEPEX, BUILD_FLAGS, FIXED_ADDRESS]")
    Parser.add_option("-F", "--flag", action="store", type="string", dest="Flag",
        help="Specify the specific option to parse EDK UNI file. Must be one of: [-c, -s]. -c is for EDK framework UNI file, and -s is for EDK UEFI UNI file. "\
             "This option can also be specified by setting *_*_*_BUILD_FLAGS in [BuildOptions] section of platform DSC. If they are both specified, this value "\
             "will override the setting in [BuildOptions] section of platform DSC.")
    Parser.add_option("-N", "--no-cache", action="store_true", dest="DisableCache", default=False, help="Disable build cache mechanism")
    Parser.add_option("--conf", action="store", type="string", dest="ConfDirectory", help="Specify the customized Conf directory.")
    Parser.add_option("--check-usage", action="store_true", dest="CheckUsage", default=False, help="Check usage content of entries listed in INF file.")
    Parser.add_option("--ignore-sources", action="store_true", dest="IgnoreSources", default=False, help="Focus to a binary build and ignore all source files")

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

    EdkLogger.quiet("Build environment: %s" % platform.platform())
    EdkLogger.quiet(time.strftime("Build start time: %H:%M:%S, %b.%d %Y\n", time.localtime()));
    ReturnCode = 0
    MyBuild = None
    BuildError = True
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

        #
        # Check environment variable: EDK_TOOLS_PATH, WORKSPACE, PATH
        #
        CheckEnvVariable()
        GlobalData.gCommandLineDefines.update(ParseDefines(Option.Macros))

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
            if os.path.isabs (Option.ModuleFile):
                if os.path.normcase (os.path.normpath(Option.ModuleFile)).find (Workspace) == 0:
                    Option.ModuleFile = NormFile(os.path.normpath(Option.ModuleFile), Workspace)
            Option.ModuleFile = PathClass(Option.ModuleFile, Workspace)
            ErrorCode, ErrorInfo = Option.ModuleFile.Validate(".inf", False)
            if ErrorCode != 0:
                EdkLogger.error("build", ErrorCode, ExtraData=ErrorInfo)

        if Option.PlatformFile != None:
            if os.path.isabs (Option.PlatformFile):
                if os.path.normcase (os.path.normpath(Option.PlatformFile)).find (Workspace) == 0:
                    Option.PlatformFile = NormFile(os.path.normpath(Option.PlatformFile), Workspace)
            Option.PlatformFile = PathClass(Option.PlatformFile, Workspace)

        if Option.FdfFile != None:
            if os.path.isabs (Option.FdfFile):
                if os.path.normcase (os.path.normpath(Option.FdfFile)).find (Workspace) == 0:
                    Option.FdfFile = NormFile(os.path.normpath(Option.FdfFile), Workspace)
            Option.FdfFile = PathClass(Option.FdfFile, Workspace)
            ErrorCode, ErrorInfo = Option.FdfFile.Validate(".fdf", False)
            if ErrorCode != 0:
                EdkLogger.error("build", ErrorCode, ExtraData=ErrorInfo)

        if Option.Flag != None and Option.Flag not in ['-c', '-s']:
            EdkLogger.error("build", OPTION_VALUE_INVALID, "UNI flag must be one of -c or -s")

        MyBuild = Build(Target, Workspace, Option)
        GlobalData.gCommandLineDefines['ARCH'] = ' '.join(MyBuild.ArchList)
        MyBuild.Launch()
        # Drop temp tables to avoid database locked.
        for TmpTableName in TmpTableDict:
            SqlCommand = """drop table IF EXISTS %s""" % TmpTableName
            TmpTableDict[TmpTableName].execute(SqlCommand)
        #MyBuild.DumpBuildData()
        #
        # All job done, no error found and no exception raised
        #
        BuildError = False
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
                    ExtraData="\n(Please send email to edk2-devel@lists.sourceforge.net for help, attaching following call stack trace!)\n",
                    RaiseError=False
                    )
        EdkLogger.quiet("(Python %s on %s) " % (platform.python_version(), sys.platform) + traceback.format_exc())
        ReturnCode = CODE_ERROR
    finally:
        Utils.Progressor.Abort()
        Utils.ClearDuplicatedInf()

    if ReturnCode == 0:
        Conclusion = "Done"
    elif ReturnCode == ABORT_ERROR:
        Conclusion = "Aborted"
    else:
        Conclusion = "Failed"
    FinishTime = time.time()
    BuildDuration = time.gmtime(int(round(FinishTime - StartTime)))
    BuildDurationStr = ""
    if BuildDuration.tm_yday > 1:
        BuildDurationStr = time.strftime("%H:%M:%S", BuildDuration) + ", %d day(s)"%(BuildDuration.tm_yday - 1)
    else:
        BuildDurationStr = time.strftime("%H:%M:%S", BuildDuration)
    if MyBuild != None:
        if not BuildError:
            MyBuild.BuildReport.GenerateReport(BuildDurationStr)
        MyBuild.Db.Close()
    EdkLogger.SetLevel(EdkLogger.QUIET)
    EdkLogger.quiet("\n- %s -" % Conclusion)
    EdkLogger.quiet(time.strftime("Build end time: %H:%M:%S, %b.%d %Y", time.localtime()))
    EdkLogger.quiet("Build total time: %s\n" % BuildDurationStr)
    return ReturnCode

if __name__ == '__main__':
    r = Main()
    ## 0-127 is a safe return range, and 1 is a standard default error
    if r < 0 or r > 127: r = 1
    sys.exit(r)

