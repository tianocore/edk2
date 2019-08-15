## @file
# Build cache intermediate result and state
#
# Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

class ModuleBuildCacheIR():
    def __init__(self, Path, Arch):
        self.ModulePath = Path
        self.ModuleArch = Arch
        self.ModuleFilesHashDigest = None
        self.ModuleFilesHashHexDigest = None
        self.ModuleFilesChain = []
        self.PreMakefileHashHexDigest = None
        self.CreateCodeFileDone = False
        self.CreateMakeFileDone = False
        self.MakefilePath = None
        self.AutoGenFileList = None
        self.DependencyHeaderFileSet = None
        self.MakeHeaderFilesHashChain = None
        self.MakeHeaderFilesHashDigest = None
        self.MakeHeaderFilesHashChain = []
        self.MakeHashDigest = None
        self.MakeHashHexDigest = None
        self.MakeHashChain = []
        self.CacheCrash = False
        self.PreMakeCacheHit = False
        self.MakeCacheHit = False
