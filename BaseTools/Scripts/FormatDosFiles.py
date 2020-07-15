# @file FormatDosFiles.py
# This script format the source files to follow dos style.
# It supports Python2.x and Python3.x both.
#
#  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

#
# Import Modules
#
from __future__ import print_function
import argparse
import os
import os.path
import re
import sys
import copy

__prog__        = 'FormatDosFiles'
__version__     = '%s Version %s' % (__prog__, '0.10 ')
__copyright__   = 'Copyright (c) 2018-2019, Intel Corporation. All rights reserved.'
__description__ = 'Convert source files to meet the EDKII C Coding Standards Specification.\n'
DEFAULT_EXT_LIST = ['.h', '.c', '.nasm', '.nasmb', '.asm', '.S', '.inf', '.dec', '.dsc', '.fdf', '.uni', '.asl', '.aslc', '.vfr', '.idf', '.txt', '.bat', '.py']

#For working in python2 and python3 environment, re pattern should use binary string, which is bytes type in python3.
#Because in python3,read from file in binary mode will return bytes type,and in python3 bytes type can not be mixed with str type.
def FormatFile(FilePath, Args):
    with open(FilePath, 'rb') as Fd:
        Content = Fd.read()
        # Convert the line endings to CRLF
        Content = re.sub(br'([^\r])\n', br'\1\r\n', Content)
        Content = re.sub(br'^\n', br'\r\n', Content, flags=re.MULTILINE)
        # Add a new empty line if the file is not end with one
        Content = re.sub(br'([^\r\n])$', br'\1\r\n', Content)
        # Remove trailing white spaces
        Content = re.sub(br'[ \t]+(\r\n)', br'\1', Content, flags=re.MULTILINE)
        # Replace '\t' with two spaces
        Content = re.sub(b'\t', b'  ', Content)
        with open(FilePath, 'wb') as Fd:
            Fd.write(Content)
            if not Args.Quiet:
                print(FilePath)

def FormatFilesInDir(DirPath, ExtList, Args):

    FileList = []
    ExcludeDir = DirPath
    for DirPath, DirNames, FileNames in os.walk(DirPath):
        if Args.Exclude:
            DirNames[:] = [d for d in DirNames if d not in Args.Exclude]
            FileNames[:] = [f for f in FileNames if f not in Args.Exclude]
            Continue = False
            for Path in Args.Exclude:
                Path = Path.strip('\\').strip('/')
                if not os.path.isdir(Path) and not os.path.isfile(Path):
                    Path = os.path.join(ExcludeDir, Path)
                if os.path.isdir(Path) and Path.endswith(DirPath):
                    DirNames[:] = []
                    Continue = True
                elif os.path.isfile(Path):
                    FilePaths = FileNames
                    for ItemPath in FilePaths:
                        FilePath = os.path.join(DirPath, ItemPath)
                        if Path.endswith(FilePath):
                            FileNames.remove(ItemPath)
            if Continue:
                continue
        for FileName in [f for f in FileNames if any(f.endswith(ext) for ext in ExtList)]:
                FileList.append(os.path.join(DirPath, FileName))
    for File in FileList:
        FormatFile(File, Args)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog=__prog__, description=__description__ + __copyright__, conflict_handler = 'resolve')

    parser.add_argument('Path', nargs='+',
                        help='the path for files to be converted.It could be directory or file path.')
    parser.add_argument('--version', action='version', version=__version__)
    parser.add_argument('--append-extensions', dest='AppendExt', nargs='+',
                        help='append file extensions filter to default extensions. (Example: .txt .c .h)')
    parser.add_argument('--override-extensions', dest='OverrideExt', nargs='+',
                        help='override file extensions filter on default extensions. (Example: .txt .c .h)')
    parser.add_argument('-v', '--verbose', dest='Verbose', action='store_true',
                        help='increase output messages')
    parser.add_argument('-q', '--quiet', dest='Quiet', action='store_true',
                        help='reduce output messages')
    parser.add_argument('--debug', dest='Debug', type=int, metavar='[0-9]', choices=range(0, 10), default=0,
                        help='set debug level')
    parser.add_argument('--exclude', dest='Exclude', nargs='+', help="directory name or file name which will be excluded")
    args = parser.parse_args()
    DefaultExt = copy.copy(DEFAULT_EXT_LIST)

    if args.OverrideExt is not None:
        DefaultExt = args.OverrideExt
    if args.AppendExt is not None:
        DefaultExt = list(set(DefaultExt + args.AppendExt))

    for Path in args.Path:
        if not os.path.exists(Path):
            print("not exists path: {0}".format(Path))
            sys.exit(1)
        if os.path.isdir(Path):
            FormatFilesInDir(Path, DefaultExt, args)
        elif os.path.isfile(Path):
            FormatFile(Path, args)
