## @file
# manage multiple workspace file.
#
# This file is required to make Python interpreter treat the directory
# as containing package.
#
# Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import re

import Common.LongFilePathOs as Os
from Common.DataType import TAB_WORKSPACE


## MultipleWorkspace
#
# This class manage multiple workspace behavior
#
# @param class:
#
# @var WORKSPACE:      defined the current WORKSPACE
# @var PACKAGES_PATH:  defined the other WORKSPACE, if current WORKSPACE is invalid, search valid WORKSPACE from PACKAGES_PATH
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
        if str(Os.path.normcase (Os.path.normpath(Path))).startswith(Os.path.normcase(Os.path.normpath(Ws))):
            return Os.path.join(Ws, Os.path.relpath(Path, Ws))
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
            cls.PACKAGES_PATH = [cls.convertPackagePath (Ws, Os.path.normpath(Path.strip())) for Path in PackagesPath.split(Os.pathsep)]
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
        Path = Os.path.join(Ws, *p)
        if not Os.path.exists(Path):
            for Pkg in cls.PACKAGES_PATH:
                Path = Os.path.join(Pkg, *p)
                if Os.path.exists(Path):
                    return Path
            Path = Os.path.join(Ws, *p)
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
                Path = Os.path.relpath(Path, Pkg)
                return Path
        if Path.lower().startswith(Ws.lower()):
            Path = Os.path.relpath(Path, Ws)
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
        absPath = Os.path.join(Ws, Path)
        if not Os.path.exists(absPath):
            for Pkg in cls.PACKAGES_PATH:
                absPath = Os.path.join(Pkg, Path)
                if Os.path.exists(absPath):
                    return Pkg
        return Ws

    ## handleWsMacro()
    #
    #   Resolve $(WORKSPACE) paths against the workspace, then PACKAGES_PATH.
    #   Preserve quoting and whitespace; keep missing paths rooted in WORKSPACE.
    #
    #   @param  cls       The class pointer
    #   @retval PathStr   Path string include the $(WORKSPACE)
    #
    @classmethod
    def handleWsMacro(cls, PathStr):
        if TAB_WORKSPACE not in PathStr:
            return PathStr

        def ReplaceMacro(Match):
            Token = Match.group(0)
            if TAB_WORKSPACE not in Token:
                return Token

            # Group 1 captures the opening quote; \1 requires the same closing quote.
            # Replace with group 2 (the contents) only for filesystem checks.
            # The original token is unchanged, and Windows backslashes are not interpreted.
            Unquoted = re.sub(r"""(["'])(.*?)\1""", r"\2", Token, flags=re.DOTALL)
            Substr = Unquoted[Unquoted.find(TAB_WORKSPACE):]
            Path = Substr.replace(TAB_WORKSPACE, cls.WORKSPACE)
            if not Os.path.exists(Path):
                for Pkg in cls.PACKAGES_PATH:
                    Path = Substr.replace(TAB_WORKSPACE, Pkg)
                    if Os.path.exists(Path):
                        return Token.replace(TAB_WORKSPACE, Pkg)
            return Token.replace(TAB_WORKSPACE, cls.WORKSPACE)

        # A token combines unquoted non-whitespace characters with complete quoted
        # segments, keeping options such as --config=".../file name" together.
        # Replace tokens in place so whitespace between them remains unchanged.
        return re.sub(r"""(?:[^\s"']|"[^"]*"|'[^']*')+""", ReplaceMacro, PathStr)

    ## getPkgPath()
    #
    #   get all package paths.
    #
    #   @param  cls       The class pointer
    #
    @classmethod
    def getPkgPath(cls):
        return cls.PACKAGES_PATH
