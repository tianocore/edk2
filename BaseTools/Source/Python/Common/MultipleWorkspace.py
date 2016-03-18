## @file
# manage multiple workspace file.
#
# This file is required to make Python interpreter treat the directory
# as containing package.
#
# Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

import Common.LongFilePathOs as os
from Common.DataType import TAB_WORKSPACE

## MultipleWorkspace
#
# This class manage multiple workspace behavior
# 
# @param class:
#
# @var WORKSPACE:      defined the current WORKSPACE
# @var PACKAGES_PATH:  defined the other WORKSAPCE, if current WORKSPACE is invalid, search valid WORKSPACE from PACKAGES_PATH
# 
class MultipleWorkspace(object):
    WORKSPACE = ''
    PACKAGES_PATH = None
    
    ## convertPackagePath()
    #
    #   Convert path to match workspace.
    #
    #   @param  cls          The class pointer
    #   @param  Ws           The current WORKSPACE
    #   @param  Path         Path to be converted to match workspace.
    #
    @classmethod
    def convertPackagePath(cls, Ws, Path):
        if str(os.path.normcase (Path)).startswith(Ws):
            return os.path.join(Ws, os.path.relpath(Path, Ws))
        return Path

    ## setWs()
    #
    #   set WORKSPACE and PACKAGES_PATH environment
    #
    #   @param  cls          The class pointer
    #   @param  Ws           initialize WORKSPACE variable
    #   @param  PackagesPath initialize PackagesPath variable
    #
    @classmethod
    def setWs(cls, Ws, PackagesPath=None):
        cls.WORKSPACE = Ws
        if PackagesPath:
            cls.PACKAGES_PATH = [cls.convertPackagePath (Ws, os.path.normpath(Path.strip())) for Path in PackagesPath.split(os.pathsep)]
        else:
            cls.PACKAGES_PATH = []
    
    ## join()
    #
    #   rewrite os.path.join function
    #
    #   @param  cls       The class pointer
    #   @param  Ws        the current WORKSPACE
    #   @param  *p        path of the inf/dec/dsc/fdf/conf file
    #   @retval Path      the absolute path of specified file
    #
    @classmethod
    def join(cls, Ws, *p):
        Path = os.path.join(Ws, *p)
        if not os.path.exists(Path):
            for Pkg in cls.PACKAGES_PATH:
                Path = os.path.join(Pkg, *p)
                if os.path.exists(Path):
                    return Path
            Path = os.path.join(Ws, *p)
        return Path
    
    ## relpath()
    #
    #   rewrite os.path.relpath function
    #
    #   @param  cls       The class pointer
    #   @param  Path      path of the inf/dec/dsc/fdf/conf file
    #   @param  Ws        the current WORKSPACE
    #   @retval Path      the relative path of specified file
    #
    @classmethod
    def relpath(cls, Path, Ws):
        for Pkg in cls.PACKAGES_PATH:
            if Path.lower().startswith(Pkg.lower()):
                Path = os.path.relpath(Path, Pkg)
                return Path
        if Path.lower().startswith(Ws.lower()):
            Path = os.path.relpath(Path, Ws)
        return Path
    
    ## getWs()
    #
    #   get valid workspace for the path
    #
    #   @param  cls       The class pointer
    #   @param  Ws        the current WORKSPACE
    #   @param  Path      path of the inf/dec/dsc/fdf/conf file
    #   @retval Ws        the valid workspace relative to the specified file path
    #
    @classmethod
    def getWs(cls, Ws, Path):
        absPath = os.path.join(Ws, Path)
        if not os.path.exists(absPath):
            for Pkg in cls.PACKAGES_PATH:
                absPath = os.path.join(Pkg, Path)
                if os.path.exists(absPath):
                    return Pkg
        return Ws
    
    ## handleWsMacro()
    #
    #   handle the $(WORKSPACE) tag, if current workspace is invalid path relative the tool, replace it.
    #
    #   @param  cls       The class pointer
    #   @retval PathStr   Path string include the $(WORKSPACE)
    #
    @classmethod
    def handleWsMacro(cls, PathStr):
        if TAB_WORKSPACE in PathStr:
            Path = PathStr.replace(TAB_WORKSPACE, cls.WORKSPACE).strip()
            if not os.path.exists(Path):
                for Pkg in cls.PACKAGES_PATH:
                    Path = PathStr.replace(TAB_WORKSPACE, Pkg).strip()
                    if os.path.exists(Path):
                        return Path
        return PathStr
    
    ## getPkgPath()
    #
    #   get all package pathes.
    #
    #   @param  cls       The class pointer
    #
    @classmethod
    def getPkgPath(cls):
        return cls.PACKAGES_PATH
            