## @file
# Common routines used by workspace
#
# Copyright (c) 2012 - 2020, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

from __future__ import absolute_import
from collections import OrderedDict, defaultdict
from Common.DataType import SUP_MODULE_USER_DEFINED
from Common.DataType import SUP_MODULE_HOST_APPLICATION
from .BuildClassObject import LibraryClassObject
import Common.GlobalData as GlobalData
from Workspace.BuildClassObject import StructurePcd
from Common.BuildToolError import RESOURCE_NOT_AVAILABLE
from Common.BuildToolError import OPTION_MISSING
from Common.BuildToolError import BUILD_ERROR
import Common.EdkLogger as EdkLogger

class OrderedListDict(OrderedDict):
    def __init__(self, *args, **kwargs):
        super(OrderedListDict, self).__init__(*args, **kwargs)
        self.default_factory = list

    def __missing__(self, key):
        self[key] = Value = self.default_factory()
        return Value

## Get all packages from platform for specified arch, target and toolchain
#
#  @param Platform: DscBuildData instance
#  @param BuildDatabase: The database saves all data for all metafiles
#  @param Arch: Current arch
#  @param Target: Current target
#  @param Toolchain: Current toolchain
#  @retval: List of packages which are DecBuildData instances
#
def GetPackageList(Platform, BuildDatabase, Arch, Target, Toolchain):
    PkgSet = set()
    if Platform.Packages:
        PkgSet.update(Platform.Packages)
    for ModuleFile in Platform.Modules:
        Data = BuildDatabase[ModuleFile, Arch, Target, Toolchain]
        PkgSet.update(Data.Packages)
        for Lib in GetLiabraryInstances(Data, Platform, BuildDatabase, Arch, Target, Toolchain):
            PkgSet.update(Lib.Packages)
    return list(PkgSet)

## Get all declared PCD from platform for specified arch, target and toolchain
#
#  @param Platform: DscBuildData instance
#  @param BuildDatabase: The database saves all data for all metafiles
#  @param Arch: Current arch
#  @param Target: Current target
#  @param Toolchain: Current toolchain
#  @retval: A dictionary contains instances of PcdClassObject with key (PcdCName, TokenSpaceGuid)
#  @retval: A dictionary contains real GUIDs of TokenSpaceGuid
#
def GetDeclaredPcd(Platform, BuildDatabase, Arch, Target, Toolchain, additionalPkgs):
    PkgList = GetPackageList(Platform, BuildDatabase, Arch, Target, Toolchain)
    PkgList = set(PkgList)
    PkgList |= additionalPkgs
    DecPcds = {}
    GuidDict = {}
    for Pkg in PkgList:
        Guids = Pkg.Guids
        GuidDict.update(Guids)
        for Pcd in Pkg.Pcds:
            PcdCName = Pcd[0]
            PcdTokenName = Pcd[1]
            if GlobalData.MixedPcd:
                for PcdItem in GlobalData.MixedPcd:
                    if (PcdCName, PcdTokenName) in GlobalData.MixedPcd[PcdItem]:
                        PcdCName = PcdItem[0]
                        break
            if (PcdCName, PcdTokenName) not in DecPcds:
                DecPcds[PcdCName, PcdTokenName] = Pkg.Pcds[Pcd]
    return DecPcds, GuidDict

## Get all dependent libraries for a module
#
#  @param Module: InfBuildData instance
#  @param Platform: DscBuildData instance
#  @param BuildDatabase: The database saves all data for all metafiles
#  @param Arch: Current arch
#  @param Target: Current target
#  @param Toolchain: Current toolchain
#  @retval: List of dependent libraries which are InfBuildData instances
#
def GetLiabraryInstances(Module, Platform, BuildDatabase, Arch, Target, Toolchain):
    return GetModuleLibInstances(Module, Platform, BuildDatabase, Arch, Target, Toolchain,Platform.MetaFile,EdkLogger)

def GetModuleLibInstances(Module, Platform, BuildDatabase, Arch, Target, Toolchain, FileName = '', EdkLogger = None):
    if Module.LibInstances:
        return Module.LibInstances
    ModuleType = Module.ModuleType

    # add forced library instances (specified under LibraryClasses sections)
    #
    # If a module has a MODULE_TYPE of USER_DEFINED,
    # do not link in NULL library class instances from the global [LibraryClasses.*] sections.
    #
    if Module.ModuleType != SUP_MODULE_USER_DEFINED:
        for LibraryClass in Platform.LibraryClasses.GetKeys():
            if LibraryClass.startswith("NULL") and Platform.LibraryClasses[LibraryClass, Module.ModuleType]:
                Module.LibraryClasses[LibraryClass] = Platform.LibraryClasses[LibraryClass, Module.ModuleType]

    # add forced library instances (specified in module overrides)
    for LibraryClass in Platform.Modules[str(Module)].LibraryClasses:
        if LibraryClass.startswith("NULL"):
            Module.LibraryClasses[LibraryClass] = Platform.Modules[str(Module)].LibraryClasses[LibraryClass]

    # EdkII module
    LibraryConsumerList = [Module]
    Constructor = []
    ConsumedByList = OrderedListDict()
    LibraryInstance = OrderedDict()

    if not Module.LibraryClass:
        EdkLogger.verbose("")
        EdkLogger.verbose("Library instances of module [%s] [%s]:" % (str(Module), Arch))

    while len(LibraryConsumerList) > 0:
        M = LibraryConsumerList.pop()
        for LibraryClassName in M.LibraryClasses:
            if LibraryClassName not in LibraryInstance:
                # override library instance for this module
                LibraryPath = Platform.Modules[str(Module)].LibraryClasses.get(LibraryClassName,Platform.LibraryClasses[LibraryClassName, ModuleType])
                if LibraryPath is None:
                    LibraryPath = M.LibraryClasses.get(LibraryClassName)
                    if LibraryPath is None:
                        if not Module.LibraryClass:
                            EdkLogger.error("build", RESOURCE_NOT_AVAILABLE,
                                            "Instance of library class [%s] is not found" % LibraryClassName,
                                            File=FileName,
                                            ExtraData="in [%s] [%s]\n\tconsumed by module [%s]" % (str(M), Arch, str(Module)))
                        else:
                            return []

                LibraryModule = BuildDatabase[LibraryPath, Arch, Target, Toolchain]
                # for those forced library instance (NULL library), add a fake library class
                if LibraryClassName.startswith("NULL"):
                    LibraryModule.LibraryClass.append(LibraryClassObject(LibraryClassName, [ModuleType]))
                elif LibraryModule.LibraryClass is None \
                     or len(LibraryModule.LibraryClass) == 0 \
                     or (ModuleType != SUP_MODULE_USER_DEFINED and ModuleType != SUP_MODULE_HOST_APPLICATION
                         and ModuleType not in LibraryModule.LibraryClass[0].SupModList):
                    # only USER_DEFINED can link against any library instance despite of its SupModList
                    if not Module.LibraryClass:
                        EdkLogger.error("build", OPTION_MISSING,
                                        "Module type [%s] is not supported by library instance [%s]" \
                                        % (ModuleType, LibraryPath), File=FileName,
                                        ExtraData="consumed by [%s]" % str(Module))
                    else:
                        return []

                LibraryInstance[LibraryClassName] = LibraryModule
                LibraryConsumerList.append(LibraryModule)
                if not Module.LibraryClass:
                    EdkLogger.verbose("\t" + str(LibraryClassName) + " : " + str(LibraryModule))
            else:
                LibraryModule = LibraryInstance[LibraryClassName]

            if LibraryModule is None:
                continue

            if LibraryModule.ConstructorList != [] and LibraryModule not in Constructor:
                Constructor.append(LibraryModule)

            # don't add current module itself to consumer list
            if M != Module:
                if M in ConsumedByList[LibraryModule]:
                    continue
                ConsumedByList[LibraryModule].append(M)
    #
    # Initialize the sorted output list to the empty set
    #
    SortedLibraryList = []
    #
    # Q <- Set of all nodes with no incoming edges
    #
    LibraryList = [] #LibraryInstance.values()
    Q = []
    for LibraryClassName in LibraryInstance:
        M = LibraryInstance[LibraryClassName]
        LibraryList.append(M)
        if not ConsumedByList[M]:
            Q.append(M)

    #
    # start the  DAG algorithm
    #
    while True:
        EdgeRemoved = True
        while Q == [] and EdgeRemoved:
            EdgeRemoved = False
            # for each node Item with a Constructor
            for Item in LibraryList:
                if Item not in Constructor:
                    continue
                # for each Node without a constructor with an edge e from Item to Node
                for Node in ConsumedByList[Item]:
                    if Node in Constructor:
                        continue
                    # remove edge e from the graph if Node has no constructor
                    ConsumedByList[Item].remove(Node)
                    EdgeRemoved = True
                    if not ConsumedByList[Item]:
                        # insert Item into Q
                        Q.insert(0, Item)
                        break
                if Q != []:
                    break
        # DAG is done if there's no more incoming edge for all nodes
        if Q == []:
            break

        # remove node from Q
        Node = Q.pop()
        # output Node
        SortedLibraryList.append(Node)

        # for each node Item with an edge e from Node to Item do
        for Item in LibraryList:
            if Node not in ConsumedByList[Item]:
                continue
            # remove edge e from the graph
            ConsumedByList[Item].remove(Node)

            if ConsumedByList[Item]:
                continue
            # insert Item into Q, if Item has no other incoming edges
            Q.insert(0, Item)

    #
    # if any remaining node Item in the graph has a constructor and an incoming edge, then the graph has a cycle
    #
    for Item in LibraryList:
        if ConsumedByList[Item] and Item in Constructor and len(Constructor) > 1:
            if not Module.LibraryClass:
                ErrorMessage = "\tconsumed by " + "\n\tconsumed by ".join(str(L) for L in ConsumedByList[Item])
                EdkLogger.error("build", BUILD_ERROR, 'Library [%s] with constructors has a cycle' % str(Item),
                                ExtraData=ErrorMessage, File=FileName)
            else:
                return []
        if Item not in SortedLibraryList:
            SortedLibraryList.append(Item)

    #
    # Build the list of constructor and destructor names
    # The DAG Topo sort produces the destructor order, so the list of constructors must generated in the reverse order
    #
    SortedLibraryList.reverse()
    Module.LibInstances = SortedLibraryList
    SortedLibraryList = [lib.SetReferenceModule(Module) for lib in SortedLibraryList]
    return SortedLibraryList
