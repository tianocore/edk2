## @file
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent

from plugins.EdkPlugins.basemodel import ini
from plugins.EdkPlugins.edk2.model import dsc
from plugins.EdkPlugins.edk2.model import inf
from plugins.EdkPlugins.edk2.model import dec
import os
from plugins.EdkPlugins.basemodel.message import *

class SurfaceObject(object):
    _objs = {}

    def __new__(cls, *args, **kwargs):
        """Maintain only a single instance of this object
        @return: instance of this class

        """
        obj = object.__new__(cls)
        if "None" not in cls._objs:
            cls._objs["None"] = []
        cls._objs["None"].append(obj)

        return obj

    def __init__(self, parent, workspace):
        self._parent    = parent
        self._fileObj   = None
        self._workspace = workspace
        self._isModify  = False
        self._modifiedObjs = []

    def __del__(self):
        pass

    def Destroy(self):
        key = self.GetRelativeFilename()
        self.GetFileObj().Destroy(self)
        del self._fileObj
        # dereference self from _objs arrary
        assert key in self._objs, "when destory, object is not in obj list"
        assert self in self._objs[key], "when destory, object is not in obj list"
        self._objs[key].remove(self)
        if len(self._objs[key]) == 0:
            del self._objs[key]

    def GetParent(self):
        return self._parent

    def GetWorkspace(self):
        return self._workspace

    def GetFileObjectClass(self):
        return ini.BaseINIFile

    def GetFilename(self):
        return self.GetFileObj().GetFilename()

    def GetFileObj(self):
        return self._fileObj

    def GetRelativeFilename(self):
        fullPath = self.GetFilename()
        return fullPath[len(self._workspace) + 1:]

    def Load(self, relativePath):
        # if has been loaded, directly return
        if self._fileObj is not None: return True

        relativePath = os.path.normpath(relativePath)
        fullPath = os.path.join(self._workspace, relativePath)
        fullPath = os.path.normpath(fullPath)

        if not os.path.exists(fullPath):
            ErrorMsg("file does not exist!", fullPath)
            return False

        self._fileObj = self.GetFileObjectClass()(fullPath, self)

        if not self._fileObj.Parse():
            ErrorMsg("Fail to parse file!", fullPath)
            return False

        # remove self from None list to list with filename as key
        cls = self.__class__
        if self not in cls._objs["None"]:
            ErrorMsg("Sufrace object does not be create into None list")
        cls._objs["None"].remove(self)
        if relativePath not in cls._objs:
            cls._objs[relativePath] = []
        cls._objs[relativePath].append(self)

        return True

    def Reload(self, force=False):
        ret = True
        # whether require must be update
        if force:
            ret = self.GetFileObj().Reload(True)
        else:
            if self.IsModified():
                if self.GetFileObj().IsModified():
                    ret = self.GetFileObj().Reload()
        return ret

    def Modify(self, modify=True, modifiedObj=None):
        if modify:
            #LogMsg("%s is modified, modified object is %s" % (self.GetFilename(), modifiedObj))
            if issubclass(modifiedObj.__class__, ini.BaseINIFile) and self._isModify:
                return
            self._isModify = modify
            self.GetParent().Modify(modify, self)
        else:
            self._isModify = modify

    def IsModified(self):
        return self._isModify

    def GetModifiedObjs(self):
        return self._modifiedObjs

    def FilterObjsByArch(self, objs, arch):
        arr = []
        for obj in objs:
            if obj.GetArch().lower() == 'common':
                arr.append(obj)
                continue
            if obj.GetArch().lower() == arch.lower():
                arr.append(obj)
                continue
        return arr

class Platform(SurfaceObject):
    def __init__(self, parent, workspace):
        SurfaceObject.__init__(self, parent, workspace)
        self._modules    = []
        self._packages   = []

    def Destroy(self):
        for module in self._modules:
            module.Destroy()
        del self._modules[:]

        del self._packages[:]
        SurfaceObject.Destroy(self)

    def GetName(self):
        return self.GetFileObj().GetDefine("PLATFORM_NAME")

    def GetFileObjectClass(self):
        return dsc.DSCFile

    def GetModuleCount(self):
        if self.GetFileObj() is None:
            ErrorMsg("Fail to get module count because DSC file has not been load!")

        return len(self.GetFileObj().GetComponents())

    def GetSupportArchs(self):
        return self.GetFileObj().GetDefine("SUPPORTED_ARCHITECTURES").strip().split('#')[0].split('|')

    def LoadModules(self, precallback=None, postcallback=None):
        for obj in self.GetFileObj().GetComponents():
            mFilename = obj.GetFilename()
            if precallback is not None:
                precallback(self, mFilename)
            arch = obj.GetArch()
            if arch.lower() == 'common':
                archarr = self.GetSupportArchs()
            else:
                archarr = [arch]
            for arch in archarr:
                module = Module(self, self.GetWorkspace())
                if module.Load(mFilename, arch, obj.GetOveridePcds(), obj.GetOverideLibs()):
                    self._modules.append(module)
                    if postcallback is not None:
                        postcallback(self, module)
                else:
                    del module
                    ErrorMsg("Fail to load module %s" % mFilename)

    def GetModules(self):
        return self._modules

    def GetLibraryPath(self, classname, arch, type):
        objs = self.GetFileObj().GetSectionObjectsByName("libraryclasses")

        for obj in objs:
            if classname.lower() != obj.GetClass().lower():
                continue
            if obj.GetArch().lower() != 'common' and \
               obj.GetArch().lower() != arch.lower():
                continue

            if obj.GetModuleType().lower() != 'common' and \
               obj.GetModuleType().lower() != type.lower():
                continue

            return obj.GetInstance()

        ErrorMsg("Fail to get library class %s [%s][%s] from platform %s" % (classname, arch, type, self.GetFilename()))
        return None

    def GetPackage(self, path):
        package = self.GetParent().GetPackage(path)
        if package not in self._packages:
            self._packages.append(package)
        return package

    def GetPcdBuildObjs(self, name, arch=None):
        arr = []
        objs = self.GetFileObj().GetSectionObjectsByName('pcds')
        for obj in objs:
            if obj.GetPcdName().lower() == name.lower():
                arr.append(obj)
        if arch is not None:
            arr = self.FilterObjsByArch(arr, arch)
        return arr

    def Reload(self, callback=None):
        # do not care force paramter for platform object
        isFileChanged = self.GetFileObj().IsModified()
        ret = SurfaceObject.Reload(self, False)
        if not ret: return False
        if isFileChanged:
            # destroy all modules and reload them again
            for obj in self._modules:
                obj.Destroy()
            del self._modules[:]
            del self._packages[:]
            self.LoadModules(callback)
        else:
            for obj in self._modules:
                callback(self, obj.GetFilename())
                obj.Reload()

        self.Modify(False)
        return True

    def Modify(self, modify=True, modifiedObj=None):
        if modify:
            #LogMsg("%s is modified, modified object is %s" % (self.GetFilename(), modifiedObj))
            if issubclass(modifiedObj.__class__, ini.BaseINIFile) and self._isModify:
                return
            self._isModify = modify
            self.GetParent().Modify(modify, self)
        else:
            if self.GetFileObj().IsModified():
                return
            for obj in self._modules:
                if obj.IsModified():
                    return

            self._isModify = modify
            self.GetParent().Modify(modify, self)

    def GetModuleObject(self, relativePath, arch):
        path = os.path.normpath(relativePath)
        for obj in self._modules:
            if obj.GetRelativeFilename() == path:
                if arch.lower() == 'common':
                    return obj
                if obj.GetArch() == arch:
                    return obj
        return None

    def GenerateFullReferenceDsc(self):
        oldDsc = self.GetFileObj()
        newDsc = dsc.DSCFile()
        newDsc.CopySectionsByName(oldDsc, 'defines')
        newDsc.CopySectionsByName(oldDsc, 'SkuIds')

        #
        # Dynamic common section should also be copied
        #
        newDsc.CopySectionsByName(oldDsc, 'PcdsDynamicDefault')
        newDsc.CopySectionsByName(oldDsc, 'PcdsDynamicHii')
        newDsc.CopySectionsByName(oldDsc, 'PcdsDynamicVpd')
        newDsc.CopySectionsByName(oldDsc, 'PcdsDynamicEx')

        sects = oldDsc.GetSectionByName('Components')
        for oldSect in sects:
            newSect = newDsc.AddNewSection(oldSect.GetName())
            for oldComObj in oldSect.GetObjects():
                module = self.GetModuleObject(oldComObj.GetFilename(), oldSect.GetArch())
                if module is None: continue

                newComObj = dsc.DSCComponentObject(newSect)
                newComObj.SetFilename(oldComObj.GetFilename())

                # add all library instance for override section
                libdict = module.GetLibraries()
                for libclass in libdict.keys():
                    if libdict[libclass] is not None:
                        newComObj.AddOverideLib(libclass, libdict[libclass].GetRelativeFilename().replace('\\', '/'))

                # add all pcds for override section
                pcddict = module.GetPcds()
                for pcd in pcddict.values():
                    buildPcd   = pcd.GetBuildObj()
                    buildType  = buildPcd.GetPcdType()
                    buildValue = None
                    if buildType.lower() == 'pcdsdynamichii' or \
                       buildType.lower() == 'pcdsdynamicvpd' or \
                       buildType.lower() == 'pcdsdynamicdefault':
                        buildType = 'PcdsDynamic'
                    if buildType != 'PcdsDynamic':
                        buildValue = buildPcd.GetPcdValue()
                    newComObj.AddOveridePcd(buildPcd.GetPcdName(),
                                            buildType,
                                            buildValue)
                newSect.AddObject(newComObj)
        return newDsc

class Module(SurfaceObject):
    def __init__(self, parent, workspace):
        SurfaceObject.__init__(self, parent, workspace)
        self._arch        = 'common'
        self._parent      = parent
        self._overidePcds = {}
        self._overideLibs = {}
        self._libs        = {}
        self._pcds        = {}
        self._ppis        = []
        self._protocols   = []
        self._depexs      = []
        self._guids       = []
        self._packages    = []

    def Destroy(self):
        for lib in self._libs.values():
            if lib is not None:
                lib.Destroy()
        self._libs.clear()

        for pcd in self._pcds.values():
            pcd.Destroy()
        self._pcds.clear()

        for ppi in self._ppis:
            ppi.DeRef(self)
        del self._ppis[:]

        for protocol in self._protocols:
            if protocol is not None:
                protocol.DeRef(self)
        del self._protocols[:]

        for guid in self._guids:
            if guid is not None:
                guid.DeRef(self)
        del self._guids[:]

        del self._packages[:]
        del self._depexs[:]
        SurfaceObject.Destroy(self)

    def GetFileObjectClass(self):
        return inf.INFFile

    def GetLibraries(self):
        return self._libs

    def Load(self, filename, arch='common', overidePcds=None, overideLibs=None):
        if not SurfaceObject.Load(self, filename):
            return False

        self._arch = arch
        if overidePcds is not None:
            self._overideLibs = overideLibs
        if overideLibs is not None:
            self._overidePcds = overidePcds

        self._SearchLibraries()
        self._SearchPackage()
        self._SearchSurfaceItems()
        return True

    def GetArch(self):
        return self._arch

    def GetModuleName(self):
        return self.GetFileObj().GetDefine("BASE_NAME")

    def GetModuleType(self):
        return self.GetFileObj().GetDefine("MODULE_TYPE")

    def GetPlatform(self):
        return self.GetParent()

    def GetModuleObj(self):
        return self

    def GetPcds(self):
        pcds = self._pcds.copy()
        for lib in self._libs.values():
            if lib is None: continue
            for name in lib._pcds.keys():
                pcds[name] = lib._pcds[name]
        return pcds

    def GetPpis(self):
        ppis = []
        ppis += self._ppis
        for lib in self._libs.values():
            if lib is None: continue
            ppis += lib._ppis
        return ppis

    def GetProtocols(self):
        pros = []
        pros = self._protocols
        for lib in self._libs.values():
            if lib is None: continue
            pros += lib._protocols
        return pros

    def GetGuids(self):
        guids = []
        guids += self._guids
        for lib in self._libs.values():
            if lib is None: continue
            guids += lib._guids
        return guids

    def GetDepexs(self):
        deps = []
        deps += self._depexs
        for lib in self._libs.values():
            if lib is None: continue
            deps += lib._depexs
        return deps

    def IsLibrary(self):
        return self.GetFileObj().GetDefine("LIBRARY_CLASS") is not None

    def GetLibraryInstance(self, classname, arch, type):
        if classname not in self._libs.keys():
            # find in overide lib firstly
            if classname in self._overideLibs.keys():
                self._libs[classname] = Library(self, self.GetWorkspace())
                self._libs[classname].Load(self._overideLibs[classname])
                return self._libs[classname]

            parent = self.GetParent()
            if issubclass(parent.__class__, Platform):
                path = parent.GetLibraryPath(classname, arch, type)
                if path is None:
                    ErrorMsg('Fail to get library instance for %s' % classname, self.GetFilename())
                    return None
                self._libs[classname] = Library(self, self.GetWorkspace())
                if not self._libs[classname].Load(path, self.GetArch()):
                    self._libs[classname] = None
            else:
                self._libs[classname] = parent.GetLibraryInstance(classname, arch, type)
        return self._libs[classname]

    def GetSourceObjs(self):
        return self.GetFileObj().GetSectionObjectsByName('source')

    def _SearchLibraries(self):
        objs = self.GetFileObj().GetSectionObjectsByName('libraryclasses')
        arch = self.GetArch()
        type = self.GetModuleType()
        for obj in objs:
            if obj.GetArch().lower() != 'common' and \
               obj.GetArch().lower() not in self.GetPlatform().GetSupportArchs():
                continue
            classname = obj.GetClass()
            instance = self.GetLibraryInstance(classname, arch, type)
            if not self.IsLibrary() and instance is not None:
                instance._isInherit = False

            if classname not in self._libs.keys():
                self._libs[classname] = instance

    def _SearchSurfaceItems(self):
        # get surface item from self's inf
        pcds  = []
        ppis  = []
        pros  = []
        deps  = []
        guids = []
        if self.GetFileObj() is not None:
            pcds = self.FilterObjsByArch(self.GetFileObj().GetSectionObjectsByName('pcd'),
                                          self.GetArch())
            for pcd in pcds:
                if pcd.GetPcdName() not in self._pcds.keys():
                    pcdItem = PcdItem(pcd.GetPcdName(), self, pcd)
                    self._pcds[pcd.GetPcdName()] = ModulePcd(self,
                                                             pcd.GetPcdName(),
                                                             pcd,
                                                             pcdItem)

            ppis += self.FilterObjsByArch(self.GetFileObj().GetSectionObjectsByName('ppis'),
                                          self.GetArch())

            for ppi in ppis:
                item = PpiItem(ppi.GetName(), self, ppi)
                if item not in self._ppis:
                    self._ppis.append(item)

            pros += self.FilterObjsByArch(self.GetFileObj().GetSectionObjectsByName('protocols'),
                                          self.GetArch())

            for pro in pros:
                item = ProtocolItem(pro.GetName(), self, pro)
                if item not in self._protocols:
                    self._protocols.append(item)

            deps += self.FilterObjsByArch(self.GetFileObj().GetSectionObjectsByName('depex'),
                                          self.GetArch())
            for dep in deps:
                item = DepexItem(self, dep)
                self._depexs.append(item)

            guids += self.FilterObjsByArch(self.GetFileObj().GetSectionObjectsByName('guids'),
                                          self.GetArch())
            for guid in guids:
                item = GuidItem(guid.GetName(), self, guid)
                if item not in self._guids:
                    self._guids.append(item)

    def _SearchPackage(self):
        objs = self.GetFileObj().GetSectionObjectsByName('packages')
        for obj in objs:
            package = self.GetPlatform().GetPackage(obj.GetPath())
            if package is not None:
                self._packages.append(package)

    def GetPackages(self):
        return self._packages

    def GetPcdObjects(self):
        if self.GetFileObj() is None:
            return []

        return self.GetFileObj().GetSectionObjectsByName('pcd')

    def GetLibraryClassHeaderFilePath(self):
        lcname = self.GetFileObj().GetProduceLibraryClass()
        if lcname is None: return None

        pkgs = self.GetPackages()
        for package in pkgs:
            path = package.GetLibraryClassHeaderPathByName(lcname)
            if path is not None:
                return os.path.realpath(os.path.join(package.GetFileObj().GetPackageRootPath(), path))
        return None

    def Reload(self, force=False, callback=None):
        if callback is not None:
            callback(self, "Starting reload...")

        ret = SurfaceObject.Reload(self, force)
        if not ret: return False

        if not force and not self.IsModified():
            return True

        for lib in self._libs.values():
            if lib is not None:
                lib.Destroy()
        self._libs.clear()

        for pcd in self._pcds.values():
            pcd.Destroy()
        self._pcds.clear()

        for ppi in self._ppis:
            ppi.DeRef(self)
        del self._ppis[:]

        for protocol in self._protocols:
            protocol.DeRef(self)
        del self._protocols[:]

        for guid in self._guids:
            guid.DeRef(self)
        del self._guids[:]

        del self._packages[:]
        del self._depexs[:]

        if callback is not None:
            callback(self, "Searching libraries...")
        self._SearchLibraries()
        if callback is not None:
            callback(self, "Searching packages...")
        self._SearchPackage()
        if callback is not None:
            callback(self, "Searching surface items...")
        self._SearchSurfaceItems()

        self.Modify(False)
        return True

    def Modify(self, modify=True, modifiedObj=None):
        if modify:
            #LogMsg("%s is modified, modified object is %s" % (self.GetFilename(), modifiedObj))
            if issubclass(modifiedObj.__class__, ini.BaseINIFile) and self._isModify:
                return
            self._isModify = modify
            self.GetParent().Modify(modify, self)
        else:
            if self.GetFileObj().IsModified():
                return

            self._isModify = modify
            self.GetParent().Modify(modify, self)

class Library(Module):
    def __init__(self, parent, workspace):
        Module.__init__(self, parent, workspace)
        self._isInherit = True

    def IsInherit(self):
        return self._isInherit

    def GetModuleType(self):
        return self.GetParent().GetModuleType()

    def GetPlatform(self):
        return self.GetParent().GetParent()

    def GetModuleObj(self):
        return self.GetParent()

    def GetArch(self):
        return self.GetParent().GetArch()

    def Destroy(self):
        self._libs.clear()
        self._pcds.clear()
        SurfaceObject.Destroy(self)

class Package(SurfaceObject):
    def __init__(self, parent, workspace):
        SurfaceObject.__init__(self, parent, workspace)
        self._pcds      = {}
        self._guids     = {}
        self._protocols = {}
        self._ppis      = {}

    def GetPcds(self):
        return self._pcds

    def GetPpis(self):
        return list(self._ppis.values())

    def GetProtocols(self):
        return list(self._protocols.values())

    def GetGuids(self):
        return list(self._guids.values())

    def Destroy(self):
        for pcd in self._pcds.values():
            if pcd is not None:
                pcd.Destroy()
        for guid in self._guids.values():
            if guid is not None:
                guid.Destroy()
        for protocol in self._protocols.values():
            if protocol is not None:
                protocol.Destroy()
        for ppi in self._ppis.values():
            if ppi is not None:
                ppi.Destroy()
        self._pcds.clear()
        self._guids.clear()
        self._protocols.clear()
        self._ppis.clear()
        self._pcds.clear()
        SurfaceObject.Destroy(self)

    def Load(self, relativePath):
        ret = SurfaceObject.Load(self, relativePath)
        if not ret: return False
        pcds = self.GetFileObj().GetSectionObjectsByName('pcds')
        for pcd in pcds:
            if pcd.GetPcdName() in self._pcds.keys():
                if self._pcds[pcd.GetPcdName()] is not None:
                    self._pcds[pcd.GetPcdName()].AddDecObj(pcd)
            else:
                self._pcds[pcd.GetPcdName()] = PcdItem(pcd.GetPcdName(), self, pcd)

        guids = self.GetFileObj().GetSectionObjectsByName('guids')
        for guid in guids:
            if guid.GetName() not in self._guids.keys():
                self._guids[guid.GetName()] = GuidItem(guid.GetName(), self, guid)
            else:
                WarnMsg("Duplicate definition for %s" % guid.GetName())

        ppis = self.GetFileObj().GetSectionObjectsByName('ppis')
        for ppi in ppis:
            if ppi.GetName() not in self._ppis.keys():
                self._ppis[ppi.GetName()] = PpiItem(ppi.GetName(), self, ppi)
            else:
                WarnMsg("Duplicate definition for %s" % ppi.GetName())

        protocols = self.GetFileObj().GetSectionObjectsByName('protocols')
        for protocol in protocols:
            if protocol.GetName() not in self._protocols.keys():
                self._protocols[protocol.GetName()] = ProtocolItem(protocol.GetName(), self, protocol)
            else:
                WarnMsg("Duplicate definition for %s" % protocol.GetName())

        return True

    def GetFileObjectClass(self):
        return dec.DECFile

    def GetName(self):
        return self.GetFileObj().GetDefine("PACKAGE_NAME")

    def GetPcdDefineObjs(self, name=None):
        arr = []
        objs = self.GetFileObj().GetSectionObjectsByName('pcds')
        if name is None: return objs

        for obj in objs:
            if obj.GetPcdName().lower() == name.lower():
                arr.append(obj)
        return arr

    def GetLibraryClassObjs(self):
        return self.GetFileObj().GetSectionObjectsByName('libraryclasses')

    def Modify(self, modify=True, modifiedObj=None):
        if modify:
            self._isModify = modify
            self.GetParent().Modify(modify, self)
        else:
            if self.GetFileObj().IsModified():
                return

            self._isModify = modify
            self.GetParent().Modify(modify, self)

    def GetLibraryClassHeaderPathByName(self, clsname):
        objs = self.GetLibraryClassObjs()
        for obj in objs:
            if obj.GetClassName() == clsname:
                return obj.GetHeaderFile()
        return None

class DepexItem(object):
    def __init__(self, parent, infObj):
        self._parent = parent
        self._infObj = infObj

    def GetDepexString(self):
        return str(self._infObj)

    def GetInfObject(self):
        return self._infObj

class ModulePcd(object):
    _type_mapping = {'FeaturePcd': 'PcdsFeatureFlag',
                     'FixedPcd': 'PcdsFixedAtBuild',
                     'PatchPcd': 'PcdsPatchableInModule'}

    def __init__(self, parent, name, infObj, pcdItem):
        assert issubclass(parent.__class__, Module), "Module's PCD's parent must be module!"
        assert pcdItem is not None, 'Pcd %s does not in some package!' % name

        self._name          = name
        self._parent        = parent
        self._pcdItem       = pcdItem
        self._infObj        = infObj

    def GetName(self):
        return self._name

    def GetParent(self):
        return self._name

    def GetArch(self):
        return self._parent.GetArch()

    def Destroy(self):
        self._pcdItem.DeRef(self._parent)
        self._infObj = None

    def GetBuildObj(self):
        platformInfos = self._parent.GetPlatform().GetPcdBuildObjs(self._name, self.GetArch())
        modulePcdType = self._infObj.GetPcdType()

        # if platform do not gives pcd's value, get default value from package
        if len(platformInfos) == 0:
            if modulePcdType.lower() == 'pcd':
                return self._pcdItem.GetDecObject()
            else:
                for obj in self._pcdItem.GetDecObjects():
                    if modulePcdType not in self._type_mapping.keys():
                        ErrorMsg("Invalid PCD type %s" % modulePcdType)
                        return None

                    if self._type_mapping[modulePcdType] == obj.GetPcdType():
                        return obj
                ErrorMsg ('Module PCD type %s does not in valied range [%s] in package!' % \
                          (modulePcdType))
        else:
            if modulePcdType.lower() == 'pcd':
                if len(platformInfos) > 1:
                    WarnMsg("Find more than one value for PCD %s in platform %s" % \
                            (self._name, self._parent.GetPlatform().GetFilename()))
                return platformInfos[0]
            else:
                for obj in platformInfos:
                    if modulePcdType not in self._type_mapping.keys():
                        ErrorMsg("Invalid PCD type %s" % modulePcdType)
                        return None

                    if self._type_mapping[modulePcdType] == obj.GetPcdType():
                        return obj

                ErrorMsg('Can not find value for pcd %s in pcd type %s' % \
                         (self._name, modulePcdType))
        return None


class SurfaceItem(object):
    _objs = {}

    def __new__(cls, *args, **kwargs):
        """Maintain only a single instance of this object
        @return: instance of this class

        """
        name    = args[0]
        parent  = args[1]
        fileObj = args[2]
        if issubclass(parent.__class__, Package):
            if name in cls._objs.keys():
                ErrorMsg("%s item is duplicated defined in packages: %s and %s" %
                         (name, parent.GetFilename(), cls._objs[name].GetParent().GetFilename()))
                return None
            obj = object.__new__(cls)
            cls._objs[name] = obj
            return obj
        elif issubclass(parent.__class__, Module):
            if name not in cls._objs.keys():
                ErrorMsg("%s item does not defined in any package! It is used by module %s" % \
                         (name, parent.GetFilename()))
                return None
            return cls._objs[name]

        return None


    def __init__(self, name, parent, fileObj):
        if issubclass(parent.__class__, Package):
            self._name    = name
            self._parent  = parent
            self._decObj  = [fileObj]
            self._refMods = {}
        else:
            self.RefModule(parent, fileObj)

    @classmethod
    def GetObjectDict(cls):
        return cls._objs

    def GetParent(self):
        return self._parent

    def GetReference(self):
        return self._refMods

    def RefModule(self, mObj, infObj):
        if mObj in self._refMods.keys():
            return
        self._refMods[mObj] = infObj

    def DeRef(self, mObj):
        if mObj not in self._refMods.keys():
            WarnMsg("%s is not referenced by module %s" % (self._name, mObj.GetFilename()))
            return
        del self._refMods[mObj]

    def Destroy(self):
        self._refMods.clear()
        cls = self.__class__
        del cls._objs[self._name]

    def GetName(self):
        return self._name

    def GetDecObject(self):
        return self._decObj[0]

    def GetDecObjects(self):
        return self._decObj

class PcdItem(SurfaceItem):
    def AddDecObj(self, fileObj):
        for decObj in self._decObj:
            if decObj.GetFilename() != fileObj.GetFilename():
                ErrorMsg("Pcd %s defined in more than one packages : %s and %s" % \
                         (self._name, decObj.GetFilename(), fileObj.GetFilename()))
                return
            if decObj.GetPcdType() == fileObj.GetPcdType() and \
               decObj.GetArch().lower() == fileObj.GetArch():
                ErrorMsg("Pcd %s is duplicated defined in pcd type %s in package %s" % \
                         (self._name, decObj.GetPcdType(), decObj.GetFilename()))
                return
        self._decObj.append(fileObj)

    def GetValidPcdType(self):
        types = []
        for obj in self._decObj:
            if obj.GetPcdType() not in types:
                types += obj.GetPcdType()
        return types

class GuidItem(SurfaceItem):
    pass

class PpiItem(SurfaceItem):
    pass

class ProtocolItem(SurfaceItem):
    pass
