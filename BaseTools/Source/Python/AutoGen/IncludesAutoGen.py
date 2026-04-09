## @file
# Build cache intermediate result and state
#
# Copyright (c) 2019 - 2020, Intel Corporation. All rights reserved.<BR>
# Copyright (c) 2020, ARM Limited. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
from Common.caching import cached_property
import Common.EdkLogger as EdkLogger
import Common.LongFilePathOs as os
from Common.BuildToolError import *
from Common.Misc import SaveFileOnChange, PathClass
from Common.Misc import TemplateString
import sys
gIsFileMap = {}

DEP_FILE_TAIL = "# Updated \n"

class IncludesAutoGen():
    """ This class is to manage the dependent files witch are used in Makefile to support incremental build.
        1. C files:
            1. MSVS.
               cl.exe has a build option /showIncludes to display include files on stdout. Build tool captures
               that messages and generate dependency files, .deps files.
            2. CLANG and GCC
               -MMD -MF build option are used to generate dependency files by compiler. Build tool updates the
               .deps files.
        2. ASL files:
            1. Trim find out all the included files with asl specific include format and generate .trim.deps file.
            2. ASL PP use c preprocessor to find out all included files with #include format and generate a .deps file
            3. build tool updates the .deps file
        3. ASM files (.asm, .s or .nasm):
            1. Trim find out all the included files with asl specific include format and generate .trim.deps file.
            2. ASM PP use c preprocessor to find out all included files with #include format and generate a deps file
            3. build tool updates the .deps file
    """
    def __init__(self, makefile_folder, ModuleAuto):
        self.d_folder = makefile_folder
        self.makefile_folder = makefile_folder
        self.module_autogen = ModuleAuto
        self.ToolChainFamily = ModuleAuto.ToolChainFamily
        self.workspace = ModuleAuto.WorkspaceDir

    def CreateModuleDeps(self):
        SaveFileOnChange(os.path.join(self.makefile_folder,"deps.txt"),"\n".join(self.DepsCollection),False)

    def CreateDepsInclude(self):
        deps_file = {'deps_file':self.deps_files}

        MakePath = self.module_autogen.BuildOption.get('MAKE', {}).get('PATH')
        if not MakePath:
            EdkLogger.error("build", PARAMETER_MISSING, Message="No Make path available.")
        elif "nmake" in MakePath:
            _INCLUDE_DEPS_TEMPLATE = TemplateString('''
${BEGIN}
!IF EXIST(${deps_file})
!INCLUDE ${deps_file}
!ENDIF
${END}
               ''')
        else:
            _INCLUDE_DEPS_TEMPLATE = TemplateString('''
${BEGIN}
-include ${deps_file}
${END}
               ''')

        try:
            deps_include_str = _INCLUDE_DEPS_TEMPLATE.Replace(deps_file)
        except Exception as e:
            print(e)
        SaveFileOnChange(os.path.join(self.makefile_folder,"dependency"),deps_include_str,False)

    def CreateDepsTarget(self):
        SaveFileOnChange(os.path.join(self.makefile_folder,"deps_target"),"\n".join([item +":" for item in self.DepsCollection]),False)

    @cached_property
    def deps_files(self):
        """ Get all .deps file under module build folder. """
        deps_files = []
        for root, _, files in os.walk(self.d_folder, topdown=False):
            for name in files:
                if not name.endswith(".deps"):
                    continue
                abspath = os.path.join(root, name)
                deps_files.append(abspath)
        return deps_files

    @cached_property
    def DepsCollection(self):
        """ Collect all the dependency files list from all .deps files under a module's build folder """
        includes = set()
        targetname = [item[0].Name for item in self.TargetFileList.values()]
        for abspath in self.deps_files:
            try:
                with open(abspath,"r") as fd:
                    lines = fd.readlines()

                firstlineitems = lines[0].split(": ")
                dependency_file = firstlineitems[1].strip(" \\\n")
                dependency_file = dependency_file.strip('''"''')
                if dependency_file:
                    if os.path.normpath(dependency_file +".deps") == abspath:
                        continue
                    filename = os.path.basename(dependency_file).strip()
                    if filename not in targetname:
                        includes.add(dependency_file.strip())

                for item in lines[1:]:
                    if item == DEP_FILE_TAIL:
                        continue
                    dependency_file = item.strip(" \\\n")
                    dependency_file = dependency_file.strip('''"''')
                    if dependency_file == '':
                        continue
                    if os.path.normpath(dependency_file +".deps") == abspath:
                        continue
                    filename = os.path.basename(dependency_file).strip()
                    if filename in targetname:
                        continue
                    includes.add(dependency_file.strip())
            except Exception as e:
                EdkLogger.error("build",FILE_NOT_FOUND, "%s doesn't exist" % abspath, ExtraData=str(e), RaiseError=False)
                continue
        rt = sorted(list(set([item.strip(' " \\\n') for item in includes])))
        return rt

    @cached_property
    def SourceFileList(self):
        """ Get a map of module's source files name to module's source files path """
        source = {os.path.basename(item.File):item.Path for item in self.module_autogen.SourceFileList}
        middle_file = {}
        for afile in source:
            if afile.upper().endswith(".VFR"):
                middle_file.update({afile.split(".")[0]+".c":os.path.join(self.module_autogen.DebugDir,afile.split(".")[0]+".c")})
            if afile.upper().endswith((".S","ASM")):
                middle_file.update({afile.split(".")[0]+".i":os.path.join(self.module_autogen.OutputDir,afile.split(".")[0]+".i")})
            if afile.upper().endswith(".ASL"):
                middle_file.update({afile.split(".")[0]+".i":os.path.join(self.module_autogen.OutputDir,afile.split(".")[0]+".i")})
        source.update({"AutoGen.c":os.path.join(self.module_autogen.OutputDir,"AutoGen.c")})
        source.update(middle_file)
        return source

    @cached_property
    def HasNamesakeSourceFile(self):
        source_base_name = set([os.path.basename(item.File) for item in self.module_autogen.SourceFileList])
        rt = len(source_base_name) != len(self.module_autogen.SourceFileList)
        return rt
    @cached_property
    def CcPPCommandPathSet(self):
        rt = set()
        rt.add(self.module_autogen.BuildOption.get('CC',{}).get('PATH'))
        rt.add(self.module_autogen.BuildOption.get('ASLCC',{}).get('PATH'))
        rt.add(self.module_autogen.BuildOption.get('ASLPP',{}).get('PATH'))
        rt.add(self.module_autogen.BuildOption.get('VFRPP',{}).get('PATH'))
        rt.add(self.module_autogen.BuildOption.get('PP',{}).get('PATH'))
        rt.add(self.module_autogen.BuildOption.get('APP',{}).get('PATH'))
        rt.discard(None)
        return rt
    @cached_property
    def TargetFileList(self):
        """ Get a map of module's target name to a tuple of module's targets path and whose input file path """
        targets = {}
        targets["AutoGen.obj"] = (PathClass(os.path.join(self.module_autogen.OutputDir,"AutoGen.obj")),PathClass(os.path.join(self.module_autogen.DebugDir,"AutoGen.c")))
        for item in self.module_autogen.Targets.values():
            for block in item:
                targets[block.Target.Path] = (block.Target,block.Inputs[0])
        return targets

    def GetRealTarget(self,source_file_abs):
        """ Get the final target file based on source file abspath """
        source_target_map = {item[1].Path:item[0].Path for item in self.TargetFileList.values()}
        source_name_map = {item[1].File:item[0].Path for item in self.TargetFileList.values()}
        target_abs = source_target_map.get(source_file_abs)
        if target_abs is None:
            if source_file_abs.strip().endswith(".i"):
                sourcefilename = os.path.basename(source_file_abs.strip())
                for sourcefile in source_name_map:
                    if sourcefilename.split(".")[0] == sourcefile.split(".")[0]:
                        target_abs = source_name_map[sourcefile]
                        break
                else:
                    target_abs = source_file_abs
            else:
                target_abs = source_file_abs
        return target_abs

    def CreateDepsFileForMsvc(self, DepList):
        """ Generate dependency files, .deps file from /showIncludes output message """
        if not DepList:
            return
        ModuleDepDict = {}
        current_source = ""
        SourceFileAbsPathMap = self.SourceFileList
        for line in DepList:
            line = line.strip()
            if self.HasNamesakeSourceFile:
                for cc_cmd in self.CcPPCommandPathSet:
                    if cc_cmd in line:
                        if '''"'''+cc_cmd+'''"''' in line:
                            cc_options = line[len(cc_cmd)+2:].split()
                        else:
                            cc_options = line[len(cc_cmd):].split()
                        for item in cc_options:
                            if not item.startswith("/"):
                                if item.endswith(".txt") and item.startswith("@"):
                                    with open(item[1:], "r") as file:
                                        source_files = file.readlines()[0].split()
                                        SourceFileAbsPathMap = {os.path.basename(file): file for file in source_files if
                                                                os.path.exists(file)}
                                else:
                                    if os.path.exists(item):
                                        SourceFileAbsPathMap.update({os.path.basename(item): item.strip()})
                        # SourceFileAbsPathMap = {os.path.basename(item):item for item in cc_options if not item.startswith("/") and os.path.exists(item)}
            if line in SourceFileAbsPathMap:
                current_source = line
                if current_source not in ModuleDepDict:
                    ModuleDepDict[SourceFileAbsPathMap[current_source]] = []
            elif "Note: including file:" ==  line.lstrip()[:21]:
                if not current_source:
                    EdkLogger.error("build",BUILD_ERROR, "Parse /showIncludes output failed. line: %s. \n" % line, RaiseError=False)
                else:
                    ModuleDepDict[SourceFileAbsPathMap[current_source]].append(line.lstrip()[22:].strip())

        for source_abs in ModuleDepDict:
            if ModuleDepDict[source_abs]:
                target_abs = self.GetRealTarget(source_abs)
                dep_file_name = os.path.basename(source_abs) + ".deps"
                SaveFileOnChange(os.path.join(os.path.dirname(target_abs),dep_file_name)," \\\n".join([target_abs+":"] + ['''"''' + item +'''"''' for item in ModuleDepDict[source_abs]]),False)

    def UpdateDepsFileforNonMsvc(self):
        """ Update .deps files.
            1. Update target path to absolute path.
            2. Update middle target to final target.
        """

        for abspath in self.deps_files:
            if abspath.endswith(".trim.deps"):
                continue
            try:
                newcontent = []
                with open(abspath,"r") as fd:
                    lines = fd.readlines()
                if lines[-1] == DEP_FILE_TAIL:
                    continue
                firstlineitems = lines[0].strip().split(" ")

                if len(firstlineitems) > 2:
                    sourceitem = firstlineitems[1]
                else:
                    sourceitem = lines[1].strip().split(" ")[0]

                source_abs = self.SourceFileList.get(sourceitem,sourceitem)
                firstlineitems[0] = self.GetRealTarget(source_abs)
                p_target = firstlineitems
                if not p_target[0].strip().endswith(":"):
                    p_target[0] += ": "

                if len(p_target) == 2:
                    p_target[0] += lines[1]
                    newcontent.append(p_target[0])
                    newcontent.extend(lines[2:])
                else:
                    line1 = " ".join(p_target).strip()
                    line1 += "\n"
                    newcontent.append(line1)
                    newcontent.extend(lines[1:])

                newcontent.append("\n")
                newcontent.append(DEP_FILE_TAIL)
                with open(abspath,"w") as fw:
                    fw.write("".join(newcontent))
            except Exception as e:
                EdkLogger.error("build",FILE_NOT_FOUND, "%s doesn't exist" % abspath, ExtraData=str(e), RaiseError=False)
                continue

    def UpdateDepsFileforTrim(self):
        """ Update .deps file which generated by trim. """

        for abspath in self.deps_files:
            if not abspath.endswith(".trim.deps"):
                continue
            try:
                newcontent = []
                with open(abspath,"r") as fd:
                    lines = fd.readlines()
                if lines[-1] == DEP_FILE_TAIL:
                    continue

                source_abs = lines[0].strip().split(" ")[0]
                targetitem = self.GetRealTarget(source_abs.strip(" :"))

                targetitem += ": "
                if len(lines)>=2:
                    targetitem += lines[1]
                newcontent.append(targetitem)
                newcontent.extend(lines[2:])
                newcontent.append("\n")
                newcontent.append(DEP_FILE_TAIL)
                with open(abspath,"w") as fw:
                    fw.write("".join(newcontent))
            except Exception as e:
                EdkLogger.error("build",FILE_NOT_FOUND, "%s doesn't exist" % abspath, ExtraData=str(e), RaiseError=False)
                continue
