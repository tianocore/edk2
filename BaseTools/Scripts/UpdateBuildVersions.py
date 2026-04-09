## @file
# Update build revisions of the tools when performing a developer build
#
# This script will modife the C/Include/Common/BuildVersion.h file and the two
# Python scripts, Python/Common/BuildVersion.py and Python/UPT/BuildVersion.py.
# If SVN is available, the tool will obtain the current checked out version of
# the source tree for including the --version commands.

#  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##
""" This program will update the BuildVersion.py and BuildVersion.h files used to set a tool's version value """
from __future__ import absolute_import

import os
import shlex
import subprocess
import sys

from argparse import ArgumentParser, SUPPRESS
from tempfile import NamedTemporaryFile
from types import IntType, ListType


SYS_ENV_ERR = "ERROR : %s system environment variable must be set prior to running this tool.\n"

__execname__ = "UpdateBuildVersions.py"
SVN_REVISION = "$LastChangedRevision: 3 $"
SVN_REVISION = SVN_REVISION.replace("$LastChangedRevision:", "").replace("$", "").strip()
__copyright__ = "Copyright (c) 2014, Intel Corporation. All rights reserved."
VERSION_NUMBER = "0.7.0"
__version__ = "Version %s.%s" % (VERSION_NUMBER, SVN_REVISION)


def ParseOptions():
    """
    Parse the command-line options.
    The options for this tool will be passed along to the MkBinPkg tool.
    """
    parser = ArgumentParser(
        usage=("%s [options]" % __execname__),
        description=__copyright__,
        conflict_handler='resolve')

    # Standard Tool Options
    parser.add_argument("--version", action="version",
                        version=__execname__ + " " + __version__)
    parser.add_argument("-s", "--silent", action="store_true",
                        dest="silent",
                        help="All output will be disabled, pass/fail determined by the exit code")
    parser.add_argument("-v", "--verbose", action="store_true",
                        dest="verbose",
                        help="Enable verbose output")
    # Tool specific options
    parser.add_argument("--revert", action="store_true",
                        dest="REVERT", default=False,
                        help="Revert the BuildVersion files only")
    parser.add_argument("--svn-test", action="store_true",
                        dest="TEST_SVN", default=False,
                        help="Test if the svn command is available")
    parser.add_argument("--svnFlag", action="store_true",
                        dest="HAVE_SVN", default=False,
                        help=SUPPRESS)

    return(parser.parse_args())


def ShellCommandResults(CmdLine, Opt):
    """ Execute the command, returning the output content """
    file_list = NamedTemporaryFile(delete=False)
    filename = file_list.name
    Results = []

    returnValue = 0
    try:
        subprocess.check_call(args=shlex.split(CmdLine), stderr=subprocess.STDOUT, stdout=file_list)
    except subprocess.CalledProcessError as err_val:
        file_list.close()
        if not Opt.silent:
            sys.stderr.write("ERROR : %d : %s\n" % (err_val.returncode, err_val.__str__()))
            if os.path.exists(filename):
                sys.stderr.write("      : Partial results may be in this file: %s\n" % filename)
            sys.stderr.flush()
        returnValue = err_val.returncode

    except IOError as err_val:
        (errno, strerror) = err_val.args
        file_list.close()
        if not Opt.silent:
            sys.stderr.write("I/O ERROR : %s : %s\n" % (str(errno), strerror))
            sys.stderr.write("ERROR : this command failed : %s\n" % CmdLine)
            if os.path.exists(filename):
                sys.stderr.write("      : Partial results may be in this file: %s\n" % filename)
            sys.stderr.flush()
        returnValue = errno

    except OSError as err_val:
        (errno, strerror) = err_val.args
        file_list.close()
        if not Opt.silent:
            sys.stderr.write("OS ERROR : %s : %s\n" % (str(errno), strerror))
            sys.stderr.write("ERROR : this command failed : %s\n" % CmdLine)
            if os.path.exists(filename):
                sys.stderr.write("      : Partial results may be in this file: %s\n" % filename)
            sys.stderr.flush()
        returnValue = errno

    except KeyboardInterrupt:
        file_list.close()
        if not Opt.silent:
            sys.stderr.write("ERROR : Command terminated by user : %s\n" % CmdLine)
            if os.path.exists(filename):
                sys.stderr.write("      : Partial results may be in this file: %s\n" % filename)
            sys.stderr.flush()
        returnValue = 1

    finally:
        if not file_list.closed:
            file_list.flush()
            os.fsync(file_list.fileno())
            file_list.close()

    if os.path.exists(filename):
        fd_ = open(filename, 'r')
        Results = fd_.readlines()
        fd_.close()
        os.unlink(filename)

    if returnValue > 0:
        return returnValue

    return Results


def UpdateBuildVersionPython(Rev, UserModified, opts):
    """ This routine will update the BuildVersion.h files in the C source tree """
    for SubDir in ["Common", "UPT"]:
        PyPath = os.path.join(os.environ['BASE_TOOLS_PATH'], "Source", "Python", SubDir)
        BuildVersionPy = os.path.join(PyPath, "BuildVersion.py")
        fd_ = open(os.path.normpath(BuildVersionPy), 'r')
        contents = fd_.readlines()
        fd_.close()
        if opts.HAVE_SVN is False:
            BuildVersionOrig = os.path.join(PyPath, "orig_BuildVersion.py")
            fd_ = open (BuildVersionOrig, 'w')
            for line in contents:
                fd_.write(line)
            fd_.flush()
            fd_.close()
        new_content = []
        for line in contents:
            if line.strip().startswith("gBUILD_VERSION"):
                new_line = "gBUILD_VERSION = \"Developer Build based on Revision: %s\"" % Rev
                if UserModified:
                    new_line = "gBUILD_VERSION = \"Developer Build based on Revision: %s with Modified Sources\"" % Rev
                new_content.append(new_line)
                continue
            new_content.append(line)

        fd_ = open(os.path.normpath(BuildVersionPy), 'w')
        for line in new_content:
            fd_.write(line)
        fd_.close()


def UpdateBuildVersionH(Rev, UserModified, opts):
    """ This routine will update the BuildVersion.h files in the C source tree """
    CPath = os.path.join(os.environ['BASE_TOOLS_PATH'], "Source", "C", "Include", "Common")
    BuildVersionH = os.path.join(CPath, "BuildVersion.h")
    fd_ = open(os.path.normpath(BuildVersionH), 'r')
    contents = fd_.readlines()
    fd_.close()
    if opts.HAVE_SVN is False:
        BuildVersionOrig = os.path.join(CPath, "orig_BuildVersion.h")
        fd_ = open(BuildVersionOrig, 'w')
        for line in contents:
            fd_.write(line)
        fd_.flush()
        fd_.close()

    new_content = []
    for line in contents:
        if line.strip().startswith("#define"):
            new_line = "#define __BUILD_VERSION \"Developer Build based on Revision: %s\"" % Rev
            if UserModified:
                new_line = "#define __BUILD_VERSION \"Developer Build based on Revision: %s with Modified Sources\"" % \
                            Rev
            new_content.append(new_line)
            continue
        new_content.append(line)

    fd_ = open(os.path.normpath(BuildVersionH), 'w')
    for line in new_content:
        fd_.write(line)
    fd_.close()


def RevertCmd(Filename, Opt):
    """ This is the shell command that does the SVN revert """
    CmdLine = "svn revert %s" % Filename.replace("\\", "/").strip()
    try:
        subprocess.check_output(args=shlex.split(CmdLine))
    except subprocess.CalledProcessError as err_val:
        if not Opt.silent:
            sys.stderr.write("Subprocess ERROR : %s\n" % err_val)
            sys.stderr.flush()

    except IOError as err_val:
        (errno, strerror) = err_val.args
        if not Opt.silent:
            sys.stderr.write("I/O ERROR : %d : %s\n" % (str(errno), strerror))
            sys.stderr.write("ERROR : this command failed : %s\n" % CmdLine)
            sys.stderr.flush()

    except OSError as err_val:
        (errno, strerror) = err_val.args
        if not Opt.silent:
            sys.stderr.write("OS ERROR : %d : %s\n" % (str(errno), strerror))
            sys.stderr.write("ERROR : this command failed : %s\n" % CmdLine)
            sys.stderr.flush()

    except KeyboardInterrupt:
        if not Opt.silent:
            sys.stderr.write("ERROR : Command terminated by user : %s\n" % CmdLine)
            sys.stderr.flush()

    if Opt.verbose:
        sys.stdout.write("Reverted this file: %s\n" % Filename)
        sys.stdout.flush()


def GetSvnRevision(opts):
    """ Get the current revision of the BaseTools/Source tree, and check if any of the files have been modified """
    Revision = "Unknown"
    Modified = False

    if opts.HAVE_SVN is False:
        sys.stderr.write("WARNING: the svn command-line tool is not available.\n")
        return (Revision, Modified)

    SrcPath = os.path.join(os.environ['BASE_TOOLS_PATH'], "Source")
    # Check if there are modified files.
    Cwd = os.getcwd()
    os.chdir(SrcPath)

    StatusCmd = "svn st -v --depth infinity --non-interactive"
    contents = ShellCommandResults(StatusCmd, opts)
    os.chdir(Cwd)
    if isinstance(contents, ListType):
        for line in contents:
            if line.startswith("M "):
                Modified = True
                break

    # Get the repository revision of BaseTools/Source
    InfoCmd = "svn info %s" % SrcPath.replace("\\", "/").strip()
    Revision = 0
    contents = ShellCommandResults(InfoCmd, opts)
    if isinstance(contents, IntType):
        return 0, Modified
    for line in contents:
        line = line.strip()
        if line.startswith("Revision:"):
            Revision = line.replace("Revision:", "").strip()
            break

    return (Revision, Modified)


def CheckSvn(opts):
    """
    This routine will return True if an svn --version command succeeds, or False if it fails.
    If it failed, SVN is not available.
    """
    OriginalSilent = opts.silent
    opts.silent = True
    VerCmd = "svn --version"
    contents = ShellCommandResults(VerCmd, opts)
    opts.silent = OriginalSilent
    if isinstance(contents, IntType):
        if opts.verbose:
            sys.stdout.write("SVN does not appear to be available.\n")
            sys.stdout.flush()
        return False

    if opts.verbose:
        sys.stdout.write("Found %s" % contents[0])
        sys.stdout.flush()
    return True


def CopyOrig(Src, Dest, Opt):
    """ Overwrite the Dest File with the Src File content """
    try:
        fd_ = open(Src, 'r')
        contents = fd_.readlines()
        fd_.close()
        fd_ = open(Dest, 'w')
        for line in contents:
            fd_.write(line)
        fd_.flush()
        fd_.close()
    except IOError:
        if not Opt.silent:
            sys.stderr.write("Unable to restore this file: %s\n" % Dest)
            sys.stderr.flush()
        return 1

    os.remove(Src)
    if Opt.verbose:
        sys.stdout.write("Restored this file: %s\n" % Src)
        sys.stdout.flush()

    return 0


def CheckOriginals(Opts):
    """
    If SVN was not available, then the tools may have made copies of the original BuildVersion.* files using
    orig_BuildVersion.* for the name. If they exist, replace the existing BuildVersion.* file with the corresponding
    orig_BuildVersion.* file.
    Returns 0 if this succeeds, or 1 if the copy function fails. It will also return 0 if the orig_BuildVersion.* file
    does not exist.
    """
    CPath = os.path.join(os.environ['BASE_TOOLS_PATH'], "Source", "C", "Include", "Common")
    BuildVersionH = os.path.join(CPath, "BuildVersion.h")
    OrigBuildVersionH = os.path.join(CPath, "orig_BuildVersion.h")
    if not os.path.exists(OrigBuildVersionH):
        return 0
    if CopyOrig(OrigBuildVersionH, BuildVersionH, Opts):
        return 1
    for SubDir in ["Common", "UPT"]:
        PyPath = os.path.join(os.environ['BASE_TOOLS_PATH'], "Source", "Python", SubDir)
        BuildVersionPy = os.path.join(PyPath, "BuildVersion.h")
        OrigBuildVersionPy = os.path.join(PyPath, "orig_BuildVersion.h")
        if not os.path.exists(OrigBuildVersionPy):
            return 0
        if CopyOrig(OrigBuildVersionPy, BuildVersionPy, Opts):
            return 1

    return 0


def RevertBuildVersionFiles(opts):
    """
    This routine will attempt to perform an SVN --revert on each of the BuildVersion.* files
    """
    if not opts.HAVE_SVN:
        if CheckOriginals(opts):
            return 1
        return 0
    # SVN is available
    BuildVersionH = os.path.join(os.environ['BASE_TOOLS_PATH'], "Source", "C", "Include", "Common", "BuildVersion.h")
    RevertCmd(BuildVersionH, opts)
    for SubDir in ["Common", "UPT"]:
        BuildVersionPy = os.path.join(os.environ['BASE_TOOLS_PATH'], "Source", "Python", SubDir, "BuildVersion.py")
        RevertCmd(BuildVersionPy, opts)

def UpdateRevisionFiles():
    """ Main routine that will update the BuildVersion.py and BuildVersion.h files."""
    options = ParseOptions()
    # Check the working environment
    if "WORKSPACE" not in os.environ.keys():
        sys.stderr.write(SYS_ENV_ERR % 'WORKSPACE')
        return 1
    if 'BASE_TOOLS_PATH' not in os.environ.keys():
        sys.stderr.write(SYS_ENV_ERR % 'BASE_TOOLS_PATH')
        return 1
    if not os.path.exists(os.environ['BASE_TOOLS_PATH']):
        sys.stderr.write("Unable to locate the %s directory." % os.environ['BASE_TOOLS_PATH'])
        return 1


    options.HAVE_SVN = CheckSvn(options)
    if options.TEST_SVN:
        return (not options.HAVE_SVN)
    # done processing the option, now use the option.HAVE_SVN as a flag. True = Have it, False = Don't have it.
    if options.REVERT:
        # Just revert the tools an exit
        RevertBuildVersionFiles(options)
    else:
        # Revert any changes in the BuildVersion.* files before setting them again.
        RevertBuildVersionFiles(options)
        Revision, Modified = GetSvnRevision(options)
        if options.verbose:
            sys.stdout.write("Revision: %s is Modified: %s\n" % (Revision, Modified))
            sys.stdout.flush()
        UpdateBuildVersionH(Revision, Modified, options)
        UpdateBuildVersionPython(Revision, Modified, options)

    return 0


if __name__ == "__main__":
    sys.exit(UpdateRevisionFiles())


