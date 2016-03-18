## @file
# Common routines used by workspace
#
# Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

from Common.Misc import sdict
from Common.DataType import SUP_MODULE_USER_DEFINED
from BuildClassObject import LibraryClassObject

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
#
def GetDeclaredPcd(Platform, BuildDatabase, Arch, Target, Toolchain):
    PkgList = GetPackageList(Platform, BuildDatabase, Arch, Target, Toolchain)
    DecPcds = {}
    for Pkg in PkgList:
        for Pcd in Pkg.Pcds:
            DecPcds[Pcd[0], Pcd[1]] = Pkg.Pcds[Pcd]
    return DecPcds

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
    if Module.AutoGenVersion >= 0x00010005:
        return _GetModuleLibraryInstances(Module, Platform, BuildDatabase, Arch, Target, Toolchain)
    else:
        return _ResolveLibraryReference(Module, Platform)

def _GetModuleLibraryInstances(Module, Platform, BuildDatabase, Arch, Target, Toolchain):
    ModuleType = Module.ModuleType

    # for overriding library instances with module specific setting
    PlatformModule = Platform.Modules[str(Module)]

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
    for LibraryClass in PlatformModule.LibraryClasses:
        if LibraryClass.startswith("NULL"):
            Module.LibraryClasses[LibraryClass] = PlatformModule.LibraryClasses[LibraryClass]

    # EdkII module
    LibraryConsumerList = [Module]
    Constructor = []
    ConsumedByList = sdict()
    LibraryInstance = sdict()

    while len(LibraryConsumerList) > 0:
        M = LibraryConsumerList.pop()
        for LibraryClassName in M.LibraryClasses:
            if LibraryClassName not in LibraryInstance:
                # override library instance for this module
                if LibraryClassName in PlatformModule.LibraryClasses:
                    LibraryPath = PlatformModule.LibraryClasses[LibraryClassName]
                else:
                    LibraryPath = Platform.LibraryClasses[LibraryClassName, ModuleType]
                if LibraryPath == None or LibraryPath == "":
                    LibraryPath = M.LibraryClasses[LibraryClassName]
                    if LibraryPath == None or LibraryPath == "":
                        return []

                LibraryModule = BuildDatabase[LibraryPath, Arch, Target, Toolchain]
                # for those forced library instance (NULL library), add a fake library class
                if LibraryClassName.startswith("NULL"):
                    LibraryModule.LibraryClass.append(LibraryClassObject(LibraryClassName, [ModuleType]))
                elif LibraryModule.LibraryClass == None \
                     or len(LibraryModule.LibraryClass) == 0 \
                     or (ModuleType != 'USER_DEFINED'
                         and ModuleType not in LibraryModule.LibraryClass[0].SupModList):
                    # only USER_DEFINED can link against any library instance despite of its SupModList
                    return []

                LibraryInstance[LibraryClassName] = LibraryModule
                LibraryConsumerList.append(LibraryModule)
            else:
                LibraryModule = LibraryInstance[LibraryClassName]

            if LibraryModule == None:
                continue

            if LibraryModule.ConstructorList != [] and LibraryModule not in Constructor:
                Constructor.append(LibraryModule)

            if LibraryModule not in ConsumedByList:
                ConsumedByList[LibraryModule] = []
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
        if ConsumedByList[M] == []:
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
                    if ConsumedByList[Item] == []:
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

            if ConsumedByList[Item] != []:
                continue
            # insert Item into Q, if Item has no other incoming edges
            Q.insert(0, Item)

    #
    # if any remaining node Item in the graph has a constructor and an incoming edge, then the graph has a cycle
    #
    for Item in LibraryList:
        if ConsumedByList[Item] != [] and Item in Constructor and len(Constructor) > 1:
            return []
        if Item not in SortedLibraryList:
            SortedLibraryList.append(Item)

    #
    # Build the list of constructor and destructir names
    # The DAG Topo sort produces the destructor order, so the list of constructors must generated in the reverse order
    #
    SortedLibraryList.reverse()
    return SortedLibraryList

def _ResolveLibraryReference(Module, Platform):
    LibraryConsumerList = [Module]

    # "CompilerStub" is a must for Edk modules
    if Module.Libraries:
        Module.Libraries.append("CompilerStub")
    LibraryList = []
    while len(LibraryConsumerList) > 0:
        M = LibraryConsumerList.pop()
        for LibraryName in M.Libraries:
            Library = Platform.LibraryClasses[LibraryName, ':dummy:']
            if Library == None:
                for Key in Platform.LibraryClasses.data.keys():
                    if LibraryName.upper() == Key.upper():
                        Library = Platform.LibraryClasses[Key, ':dummy:']
                        break
                if Library == None:
                    continue

            if Library not in LibraryList:
                LibraryList.append(Library)
                LibraryConsumerList.append(Library)
    return LibraryList
