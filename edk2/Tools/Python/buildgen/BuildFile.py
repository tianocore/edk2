#!/usr/bin/env python

# Copyright (c) 2007, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

"""Generate build file for given platform"""

import os, sys, copy
import xml.dom.minidom, pprint
import FrameworkElement

from SurfaceAreaElement import *
from XmlRoutines import *
from AntTasks import *
from sets import Set

class BuildFile:
    def __init__(self, workspace, platform, toolchain, target):
        if workspace == None or workspace == "":
            raise Exception("No workspace, no build")
        if platform == None or platform == "":
            raise Exception("No platform, no build")
        if toolchain == None or toolchain == "":
            raise Exception("No toolchain, no build")
        if target == None or target == "":
            raise Exception("No target, no build")
        
        self.Workspace = workspace
        self.Platform = platform
        self.Toolchain = toolchain
        self.Target = target
        self.Path = ""

    def Generate(self):
        """Generate the build file"""
        pass

# generating build.xml for platform
class AntPlatformBuildFile(BuildFile):
    def __init__(self, workspace, platform, toolchain, target):
        BuildFile.__init__(self, workspace, platform, toolchain, target)
        # Form the build file path, hard-coded at present. It should be specified by a configuration file
        self.Path = os.path.join(self.Workspace.Path, self.Platform.OutputPath, target + "_" + toolchain, "build.xml")
        print ""
        # Generate a common build option property file in the format of Java's property file
        self.DefaultBuildOptions()
        
        # new a build file object
        self.BuildXml = AntBuildFile(name="platform", basedir=".", default="all")
        
        # generate the top level properties, tasks, etc.
        self.Header()
        
        # generate "prebuild" target
        self.PreBuild()
        
        # generate "libraries" target for building library modules
        self.Libraries()
        
        # generate "modules" target for building non-library modules
        self.Modules()
        
        # generate "fvs" target for building firmware volume. (not supported yet)
        
        # generate "fds" target for building FlashDevice. (not supported yet)
        
        # generate "postbuild" target
        self.PostBuild()

    def Generate(self):
        print "Generating platform build file ...", self.Path
        self.BuildXml.Create(self.Path)

    def Header(self):
        _topLevel = self.BuildXml
        # import external tasks
        _topLevel.SubTask("taskdef", resource="GenBuild.tasks")
        _topLevel.SubTask("taskdef", resource="frameworktasks.tasks")
        _topLevel.SubTask("taskdef", resource="net/sf/antcontrib/antlib.xml")

        # platform wide properties
        _topLevel.Blankline()
        _topLevel.Comment("WORKSPACE wide attributes")
        _topLevel.SubTask("property", environment="env")
        _topLevel.SubTask("property", name="WORKSPACE_DIR", value="${env.WORKSPACE}")
        _topLevel.SubTask("property", name="CONFIG_DIR", value="${WORKSPACE_DIR}/Tools/Conf")

        _topLevel.Blankline()
        _topLevel.Comment("Common build attributes")
        _topLevel.SubTask("property", name="THREAD_COUNT", value=self.Workspace.ThreadCount)
        _topLevel.SubTask("property", name="SINGLE_MODULE_BUILD", value="no")
        _topLevel.SubTask("property", name="MODULE_BUILD_TARGET", value="platform_module_build")

        _topLevel.Blankline()
        _topLevel.SubTask("property", name="TOOLCHAIN", value=self.Toolchain)
        _topLevel.SubTask("property", name="TARGET", value=self.Target)
        
        _topLevel.Blankline()
        _topLevel.Comment("Platform attributes")
        _topLevel.SubTask("property", name="PLATFORM", value=self.Platform.Name)
        _topLevel.SubTask("property", name="PLATFORM_GUID", value=self.Platform.GuidValue)
        _topLevel.SubTask("property", name="PLATFORM_VERSION", value=self.Platform.Version)
        _topLevel.SubTask("property", name="PLATFORM_RELATIVE_DIR", value=self.Platform.Dir)
        _topLevel.SubTask("property", name="PLATFORM_DIR", value="${WORKSPACE_DIR}/${PLATFORM_RELATIVE_DIR}")
        _topLevel.SubTask("property", name="PLATFORM_OUTPUT_DIR", value=self.Platform.OutputPath)

        # user configurable build path for platform
        _topLevel.Blankline()
        _topLevel.Comment("Common path definition for platform build")
        _topLevel.SubTask("property", file="${WORKSPACE_DIR}/Tools/Python/buildgen/platform_build_path.txt")
        
        # common build tasks in the form of Ant macro
        _topLevel.Blankline()
        _topLevel.Comment("Task Macros for Compiling, Assembling, Linking, etc.")
        _topLevel.SubTask("import", file="${CONFIG_DIR}/BuildMacro.xml")
        _topLevel.Blankline()
        _topLevel.SubTask("echo", message="${PLATFORM}-${PLATFORM_VERSION} (${PLATFORM_RELATIVE_DIR})", level="info")
        
        # define the targets execution sequence
        _topLevel.Blankline()
        _topLevel.Comment("Default target")
        _topLevel.SubTask("target", name="all", depends="prebuild, libraries, modules, postbuild")

    def PreBuild(self):
        _topLevel = self.BuildXml
        _topLevel.Blankline()
        _topLevel.Comment(" TARGET: prebuild ")
        
        # prebuild is defined by user in the fpd file through <UserExtionsion> element,
        # which has attribute "identifier=0" or "identifier=prebuild"
        prebuildTasks = []
        if self.Platform.UserExtensions.has_key("0"):
            prebuildTasks = self.Platform.UserExtensions["0"]
        elif self.Platform.UserExtensions.has_key("postbuild"):
            prebuildTasks = self.Platform.UserExtensions["prebuild"]

        _topLevel.SubTask("target", prebuildTasks, name="prebuild")

    def Libraries(self):
        _topLevel = self.BuildXml
        _topLevel.Blankline()
        _topLevel.Comment(" TARGET: libraries ")
        
        librariesTarget = _topLevel.SubTask("target", name="libraries")
        parallelBuild = librariesTarget.SubTask("parallel", threadCount="${THREAD_COUNT}")
        
        libraryNumber = 0
        for arch in self.Platform.Libraries:
            libraryNumber += len(self.Platform.Libraries[arch])
        libraryIndex = 0
        for arch in self.Platform.Libraries:
            for lib in self.Platform.Libraries[arch]:
                libraryIndex += 1
                print "Generating library build files ... %d%%\r" % int((float(libraryIndex) / float(libraryNumber)) * 100),
                buildFile = AntModuleBuildFile(self.Workspace, self.Platform, lib, self.Toolchain, self.Target, arch)
                buildFile.Generate()
                buildDir = os.path.join("${TARGET_DIR}", arch, lib.Module.Package.SubPath(lib.Module.Dir),
                                        lib.Module.FileBaseName)
                parallelBuild.SubTask("ant", dir=buildDir,
                                      #antfile="build.xml",
                                      inheritAll="true",
                                      target="${MODULE_BUILD_TARGET}")
        print ""

    def Modules(self):
        _topLevel = self.BuildXml
        _topLevel.Blankline()
        _topLevel.Comment(" TARGET: modules ")

        modulesTarget = _topLevel.SubTask("target", name="modules")
        parallelBuild = modulesTarget.SubTask("parallel", threadCount="${THREAD_COUNT}")

        moduleNumber = 0
        for arch in self.Platform.Modules:
            moduleNumber += len(self.Platform.Modules[arch])
            
        moduleIndex = 0
        for arch in self.Platform.Modules:
            for module in self.Platform.Modules[arch]:
                moduleIndex += 1
                print "Generating module build files ... %d%%\r" % int((float(moduleIndex) / float(moduleNumber)) * 100),
                
                buildDir = os.path.join("${TARGET_DIR}", arch, module.Module.Package.SubPath(module.Module.Dir),
                                        module.Module.FileBaseName)
                parallelBuild.SubTask("ant", dir=buildDir,
                                      #antfile="build.xml",
                                      inheritAll="true",
                                      target="${MODULE_BUILD_TARGET}")
                buildFile = AntModuleBuildFile(self.Workspace, self.Platform, module, self.Toolchain, self.Target, arch)
                buildFile.Generate()
        print ""

    def Fvs(self):
        pass

    def Fds(self):
        pass

    def PostBuild(self):
        _topLevel = self.BuildXml
        _topLevel.Blankline()
        _topLevel.Comment(" TARGET: postbuild ")
        
        # postbuild is defined by user in the fpd file through <UserExtionsion> element,
        # which has attribute "identifier=1" or "identifier=postbuild"
        postbuildTasks = []
        if self.Platform.UserExtensions.has_key("1"):
            postbuildTasks = self.Platform.UserExtensions["1"]
        elif self.Platform.UserExtensions.has_key("postbuild"):
            postbuildTasks = self.Platform.UserExtensions["postbuild"]
            
        _topLevel.SubTask("target", postbuildTasks, name="postbuild")

    def Clean(self):
        pass

    def CleanAll(self):
        pass

    def UserExtensions(self):
        pass

    def DefaultBuildOptions(self):
        """Generate ${ARCH}_build.opt which contains the default build&tool definitions"""
        tools = self.Workspace.ToolConfig.ToolCodes
        for arch in self.Workspace.ActiveArchs:
            validTools = []
            for tool in tools:
                key = (self.Toolchain, self.Target, arch, tool, "NAME")
                if self.Workspace.ToolConfig[key] == "": continue
                validTools.append(tool)
                
            optFileDir = os.path.join(self.Workspace.Path, self.Platform.OutputPath,
                                      self.Target + "_" + self.Toolchain)
            optFileName = arch + "_build.opt"
            if not os.path.exists(optFileDir): os.makedirs(optFileDir)
            f = open(os.path.join(optFileDir, optFileName), "w")
            
            for tool in validTools:
                key = (self.Toolchain, self.Target, arch, tool, "FLAGS")
                if key in self.Platform.BuildOptions:
                    flag = self.Platform.BuildOptions[key]
                else:
                    key = (self.Toolchain, self.Target, arch, tool, "FAMILY")
                    family = self.Workspace.ToolConfig[key]
                    key = (family, self.Target, arch, tool, "FLAGS")
                    if key in self.Platform.BuildOptions:
                        flag = self.Platform.BuildOptions[key]
                    else:
                        flag = ""
                f.write("PLATFORM_%s_FLAGS=%s\n" % (tool, flag))
            f.write("\n")

            for tool in validTools:
                key = (self.Toolchain, self.Target, arch, tool, "FLAGS")
                flag = self.Workspace.ToolConfig[key]
                f.write("DEFAULT_%s_FLAGS=%s\n" % (tool, flag))

            f.write("\n")
            for tool in validTools:
                for attr in self.Workspace.ToolConfig.Attributes:
                    if attr == "FLAGS": continue
                    key = (self.Toolchain, self.Target, arch, tool, attr)
                    value = self.Workspace.ToolConfig[key]
                    if attr == "NAME":
                        path = self.Workspace.ToolConfig[(self.Toolchain, self.Target, arch, tool, "PATH")]
                        f.write("%s=%s\n" % (tool, os.path.join(path, value)))
                    else:
                        f.write("%s_%s=%s\n" % (tool, attr, value))
                f.write("%s_FLAGS=${DEFAULT_%s_FLAGS} ${DEFAULT_MODULE_%s_FLAGS} ${PLATFORM_%s_FLAGS} ${MODULE_%s_FLAGS}\n" %
                        (tool, tool, tool, tool, tool))
                f.write("\n")

            f.close()

class AntModuleBuildFile(BuildFile):
    def __init__(self, workspace, platform, module, toolchain, target, arch):
        BuildFile.__init__(self, workspace, platform, toolchain, target)
        self.Module = module
        self.Arch = arch
        self.Path = os.path.join(self.Workspace.Path, self.Platform.OutputPath,
                                 target + "_" + toolchain, arch, self.Module.Module.Package.Dir,
                                 self.Module.Module.Dir, self.Module.Module.FileBaseName, "build.xml")
        self.BuildXml = AntBuildFile(self.Module.Module.Name)

        self.SourceFiles = self.GetSourceFiles()
        
        self.Header()
        self.PreBuild()
        self.Libraries()
        self.Sourcefiles()
        self.Sections()
        self.Ffs()
        self.PostBuild()

    def Generate(self):
        # print self.Path,"\r",
        self.BuildXml.Create(self.Path)

    def Header(self):
        _topLevel = self.BuildXml
        _topLevel.SubTask("taskdef", resource="frameworktasks.tasks")
        _topLevel.SubTask("taskdef", resource="cpptasks.tasks")
        _topLevel.SubTask("taskdef", resource="cpptasks.types")
        _topLevel.SubTask("taskdef", resource="net/sf/antcontrib/antlib.xml")

        _topLevel.Blankline()
        _topLevel.Comment(" TODO ")
        _topLevel.SubTask("property", environment="env")
        _topLevel.SubTask("property", name="WORKSPACE_DIR", value="${env.WORKSPACE}")

        _topLevel.Blankline()
        _topLevel.Comment("Common build attributes")
        _topLevel.SubTask("property", name="SINGLE_MODULE_BUILD", value="yes")
        _topLevel.SubTask("property", name="MODULE_BUILD_TARGET", value="single_module_build")
        _topLevel.SubTask("property", name="PLATFORM_PREBUILD", value="yes")
        _topLevel.SubTask("property", name="PLATFORM_POSTBUILD", value="no")
        
        _topLevel.Blankline()
        _topLevel.Comment(" TODO ")
        ifTask = _topLevel.SubTask("if")
        ifTask.SubTask("istrue", value="${SINGLE_MODULE_BUILD}")
        thenTask = ifTask.SubTask("then")
        platformBuildFile = os.path.join("${WORKSPACE_DIR}", self.Platform.OutputPath,
                                         self.Target + "_" + self.Toolchain, "build.xml")
        thenTask.SubTask("import", file=platformBuildFile)
        
        _topLevel.Blankline()
        _topLevel.SubTask("property", name="ARCH", value=self.Arch)
        
        module = self.Module.Module
        package = module.Package
        _topLevel.Blankline()
        _topLevel.SubTask("property", name="PACKAGE", value=package.Name)
        _topLevel.SubTask("property", name="PACKAGE_GUID", value=package.GuidValue)
        _topLevel.SubTask("property", name="PACKAGE_VERSION", value=package.Version)
        _topLevel.SubTask("property", name="PACKAGE_RELATIVE_DIR", value=package.Dir)
        _topLevel.SubTask("property", name="PACKAGE_DIR", value=os.path.join("${WORKSPACE_DIR}","${PACKAGE_RELATIVE_DIR}"))
        
        _topLevel.Blankline()
        _topLevel.SubTask("property", name="MODULE", value=module.Name)
        _topLevel.SubTask("property", name="MODULE_GUID", value=module.GuidValue)
        _topLevel.SubTask("property", name="MODULE_VERSION", value=module.Version)
        _topLevel.SubTask("property", name="MODULE_TYPE", value=module.Type)
        _topLevel.SubTask("property", name="MODULE_FILE_BASE_NAME", value=module.FileBaseName)
        _topLevel.SubTask("property", name="MODULE_RELATIVE_DIR", value=module.Dir)
        _topLevel.SubTask("property", name="MODULE_DIR", value=os.path.join("${PACKAGE_DIR}", "${MODULE_RELATIVE_DIR}"))
        _topLevel.SubTask("property", name="BASE_NAME", value=module.BaseName)

        _topLevel.Blankline()
        _topLevel.SubTask("property", file="${WORKSPACE_DIR}/Tools/Python/buildgen/module_build_path.txt")
        
        self._BuildOption()
        
        _topLevel.Blankline()
        _topLevel.SubTask("property", name="ENTRYPOINT", value="_ModuleEntryPoint")
        
        _topLevel.SubTask("property", name="SOURCE_FILES", value="\n    ".join(self._GetSourceFileList()))
        _topLevel.SubTask("property", name="LIBS", value="\n    ".join(self._GetLibList()))
        
        _topLevel.Blankline()
        _topLevel.SubTask("property", file="${PLATFORM_BUILD_DIR}/%s_build.opt" % self.Arch)
        _topLevel.Blankline()
        _topLevel.SubTask("echo", message="${MODULE}-${MODULE_VERSION} [${ARCH}] from package ${PACKAGE}-${PACKAGE_VERSION} (${MODULE_RELATIVE_DIR})", level="info")

        _topLevel.Blankline()
        _topLevel.Comment("Default target")
        _topLevel.SubTask("target", name="all", depends="single_module_build")
        _topLevel.SubTask("target", name="platform_module_build", depends="prebuild, sourcefiles, sections, output, postbuild")
        _topLevel.SubTask("target", name="single_module_build", depends="prebuild, libraries, sourcefiles, sections, output, postbuild")

    def _BuildOption(self):
        _topLevel = self.BuildXml
        _topLevel.Blankline()
        baseModule = self.Module.Module
        tools = self.Workspace.ToolConfig.ToolCodes

        for tool in tools:
            key = (self.Toolchain, self.Target, self.Arch, tool, "FLAGS")
            flag = ""
            if key in baseModule.BuildOptions:
                flag = baseModule.BuildOptions[key]
            _topLevel.SubTask("property", name="DEFAULT_MODULE_%s_FLAGS" % tool, value=flag)

        _topLevel.Blankline()
        for tool in tools:
            key = (self.Toolchain, self.Target, self.Arch, tool, "FLAGS")
            flag = ""
            if key in self.Module.BuildOptions:
                flag = self.Module.BuildOptions[key]
            _topLevel.SubTask("property", name="MODULE_%s_FLAGS" % tool, value=flag)

    def PreBuild(self):
        _topLevel = self.BuildXml
        _topLevel.Blankline()
        _topLevel.Comment(" TARGET: prebuild ")

        prebuildTasks = []
        module = self.Module.Module
        if module.UserExtensions.has_key("0"):
            prebuildTasks = module.UserExtensions["0"]
        elif module.UserExtensions.has_key("postbuild"):
            prebuildTasks = module.UserExtensions["prebuild"]

        _topLevel.SubTask("target", prebuildTasks, name="prebuild")


    def Libraries(self):
        _topLevel = self.BuildXml
        _topLevel.Blankline()
        _topLevel.Comment(" TARGET: libraries ")
        
        librariesTarget = _topLevel.SubTask("target", name="libraries")
        parallelBuild = librariesTarget.SubTask("parallel", threadCount="${THREAD_COUNT}")
        for lib in self.Module.Libraries:
            module = lib.Module
            buildDir = os.path.join("${BIN_DIR}", module.Package.SubPath(module.Dir), module.FileBaseName)
            libTask = parallelBuild.SubTask("ant", dir=buildDir,
                                            inheritAll="false",
                                            target="${MODULE_BUILD_TARGET}")
            libTask.SubTask("property", name="PLATFORM_PREBUILD", value="false")
            libTask.SubTask("property", name="PLATFORM_POSTBUILD", value="false")

    def Sourcefiles(self):
        _topLevel = self.BuildXml
        _topLevel.Blankline()
        _topLevel.Comment(" TARGET: sourcefiles ")
        _sourcefilesTarget = _topLevel.SubTask("target", name="sourcefiles")
        
        _incTask = AntTask(self.BuildXml.Document, "EXTRA.INC")
        _incTask.SubTask("includepath", path="${WORKSPACE_DIR}")
        _incTask.SubTask("includepath", path="${MODULE_DIR}")
        _incTask.SubTask("includepath", path=os.path.join("${MODULE_DIR}", self.Arch.capitalize()))
        _incTask.SubTask("includepath", path="${DEST_DIR_DEBUG}")
        if self.Arch in self.Module.Module.IncludePaths:
            for inc in self.Module.Module.IncludePaths[self.Arch]:
                _incTask.SubTask("includepath", path=os.path.join("${WORKSPACE_DIR}", inc))
            
        # init
        if not self.Module.Module.IsBinary:
            _buildTask = _sourcefilesTarget.SubTask("Build_Init")
            _buildTask.AddSubTask(_incTask)
        
            # AutoGen firt
            _buildTask = _sourcefilesTarget.SubTask("Build_AUTOGEN", FILEEXT="c", FILENAME="AutoGen", FILEPATH=".")
            _buildTask.AddSubTask(_incTask)

        # uni file follows
        type = "UNI"
        if type in self.SourceFiles:
            for srcpath in self.SourceFiles[type]:
                taskName = "Build_" + type
                fileDir = os.path.dirname(srcpath)
                if fileDir == "": fileDir = "."
                fileBaseName,fileExt = os.path.basename(srcpath).rsplit(".", 1)
                _buildTask = _sourcefilesTarget.SubTask(taskName, FILENAME=fileBaseName, FILEEXT=fileExt, FILEPATH=fileDir)
                _buildTask.AddSubTask(_incTask)

        # others: c, vfr, ...
        for type in self.SourceFiles:
            if type == "Unicode": continue
            for srcpath in self.SourceFiles[type]:
                taskName = "Build_" + type
                fileDir = os.path.dirname(srcpath)
                if fileDir == "": fileDir = "."
                fileBaseName,fileExt = os.path.basename(srcpath).rsplit(".", 1)
                _buildTask = _sourcefilesTarget.SubTask(taskName, FILENAME=fileBaseName, FILEEXT=fileExt, FILEPATH=fileDir)
                _buildTask.AddSubTask(_incTask)

    def Sections(self):
        _topLevel = self.BuildXml
        _topLevel.Blankline()
        _topLevel.Comment(" TARGET: sections ")
        _sectionsTarget = _topLevel.SubTask("target", name="sections")

    def Ffs(self):
        _topLevel = self.BuildXml
        _topLevel.Blankline()
        _topLevel.Comment(" TARGET: output ")
        _sectionsTarget = _topLevel.SubTask("target", name="output")

    def PostBuild(self):
        _topLevel = self.BuildXml
        _topLevel.Blankline()
        _topLevel.Comment(" TARGET: postbuild ")

        postbuildTasks = []
        module = self.Module.Module
        if module.UserExtensions.has_key("1"):
            postbuildTasks = module.UserExtensions["1"]
        elif module.UserExtensions.has_key("postbuild"):
            postbuildTasks = module.UserExtensions["postbuild"]

        _topLevel.SubTask("target", postbuildTasks, name="postbuild")

    def Clean(self):
        pass

    def CleanAll(self):
        pass

    def UserExtensions(self):
        pass
    
    def GetSourceFiles(self):
        ## check arch, toolchain, family, toolcode, ext
        ##  if the arch of source file supports active arch
        ##  if the toolchain of source file supports active toolchain
        ##  if the toolchain family of source file supports active toolchain family
        ##  if the ext of the source file is supported by the toolcode
        module = self.Module.Module
        files = {}  # file type -> src
        for type in module.SourceFiles[self.Arch]:
            if not module.IsBuildable(type):
                # print type,"is not buildable"
                continue
        
            if type not in files:
                files[type] = []
            for src in module.SourceFiles[self.Arch][type]:
                if self.Toolchain not in src.Toolchains:
                    # print self.Toolchain,"not in ",src.Toolchains
                    continue
                if not self.IsCompatible(src.Families, self.Workspace.ActiveFamilies):
                    # print src.Families,"not compatible with",self.Workspace.ActiveFamilies
                    continue
                toolcode = src.GetToolCode(src.Type)
                if toolcode != "":
                    ext = self.Workspace.GetToolDef(self.Toolchain, self.Target, self.Arch, toolcode, "EXT")
                    if ext != "" and ext != src.Ext:
                        # print ext,"in tools_def.txt is not the same as",src.Ext
                        continue
                ## fileFullPath = os.path.join("${MODULE_DIR}", )
                ## print fileFullPath
                files[type].append(src.Path)

        return files

    def Intersection(self, list1, list2):
        return list(Set(list1) & Set(list2))

    def IsCompatible(self, list1, list2):
        return len(self.Intersection(list1, list2)) > 0

    def _GetLibList(self):
        libList = []
        for lib in self.Module.Libraries:
            module = lib.Module
            libList.append(os.path.join("${BIN_DIR}", module.Name + ".lib"))
        return libList
    
    def _GetSourceFileList(self):
        srcList = []
        for type in self.SourceFiles:
            srcList.extend(self.SourceFiles[type])
        return srcList

class NmakeFile(BuildFile):
    pass

class GmakeFile(BuildFile):
    pass

# for test
if __name__ == "__main__":
    workspacePath = os.getenv("WORKSPACE", os.getcwd())
    startTime = time.clock()
    ws = Workspace(workspacePath, [], [])
    
    # generate build.xml
    ap = ws.ActivePlatform
    for target in ws.ActiveTargets:
        ant = AntPlatformBuildFile(ws, ap, ws.ActiveToolchain, target)
        ant.Generate()
    
    print "\n[Finished in %fs]" % (time.clock() - startTime)
