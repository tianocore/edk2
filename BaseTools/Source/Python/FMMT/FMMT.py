# @file
#  Firmware Module Management Tool.
#
#  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

# Import Modules
#
import argparse
import sys
from core.FMMTOperation import *

parser = argparse.ArgumentParser(description='''
View the Binary Structure of FD/FV/Ffs/Section, and Delete/Extract/Add/Replace a Ffs from/into a FV.
''')
parser.add_argument("--version", action="version", version='%(prog)s Version 1.0',
                    help="Print debug information.")
parser.add_argument("-v", "--View", dest="View", nargs='+',
                    help="View each FV and the named files within each FV: '-v inputfile outputfile, inputfiletype(.Fd/.Fv/.ffs/.sec)'")
parser.add_argument("-d", "--Delete", dest="Delete", nargs='+',
                    help="Delete a Ffs from FV: '-d inputfile TargetFfsName outputfile TargetFvName(Optional,\
                    If not given, wil delete all the existed target Ffs)'")
parser.add_argument("-e", "--Extract", dest="Extract", nargs='+',
                    help="Extract a Ffs Info: '-e inputfile TargetFfsName outputfile'")
parser.add_argument("-a", "--Add", dest="Add", nargs='+',
                    help="Add a Ffs into a FV:'-a inputfile TargetFvName newffsfile outputfile'")
parser.add_argument("-r", "--Replace", dest="Replace", nargs='+',
                    help="Replace a Ffs in a FV: '-r inputfile TargetFfsName newffsfile outputfile TargetFvName(Optional,\
                        If not given, wil replace all the existed target Ffs with new Ffs file)'")
parser.add_argument("-l", "--LogFileType", dest="LogFileType", nargs='+',
                    help="The format of log file which saves Binary layout. Currently supports: json, txt. More formats will be added in the future")

def print_banner():
    print("")

class FMMT():
    def __init__(self) -> None:
        self.firmware_packet = {}

    def CheckFfsName(self, FfsName:str) -> str:
        try:
            return uuid.UUID(FfsName)
        except:
            return FfsName

    def View(self, inputfile: str, logfiletype: str=None, outputfile: str=None) -> None:
        # ParserFile(inputfile, ROOT_TYPE, logfile, outputfile)
        filetype = os.path.splitext(inputfile)[1].lower()
        if filetype == '.fd':
            ROOT_TYPE = ROOT_TREE
        elif filetype == '.fv':
            ROOT_TYPE = ROOT_FV_TREE
        elif filetype == '.ffs':
            ROOT_TYPE = ROOT_FFS_TREE
        elif filetype == '.sec':
            ROOT_TYPE = ROOT_SECTION_TREE
        else:
            ROOT_TYPE = ROOT_TREE
        ParserFile(inputfile, ROOT_TYPE, logfiletype, outputfile)

    def Delete(self, inputfile: str, TargetFfs_name: str, outputfile: str, Fv_name: str=None) -> None:
        if Fv_name:
            DeleteFfs(inputfile, self.CheckFfsName(TargetFfs_name), outputfile, uuid.UUID(Fv_name))
        else:
            DeleteFfs(inputfile, self.CheckFfsName(TargetFfs_name), outputfile)

    def Extract(self, inputfile: str, Ffs_name: str, outputfile: str) -> None:
        ExtractFfs(inputfile, self.CheckFfsName(Ffs_name), outputfile)

    def Add(self, inputfile: str, Fv_name: str, newffsfile: str, outputfile: str) -> None:
        AddNewFfs(inputfile, self.CheckFfsName(Fv_name), newffsfile, outputfile)

    def Replace(self,inputfile: str, Ffs_name: str, newffsfile: str, outputfile: str, Fv_name: str=None) -> None:
        if Fv_name:
            ReplaceFfs(inputfile, self.CheckFfsName(Ffs_name), newffsfile, outputfile, uuid.UUID(Fv_name))
        else:
            ReplaceFfs(inputfile, self.CheckFfsName(Ffs_name), newffsfile, outputfile)


def main():
    args=parser.parse_args()
    status=0

    try:
        fmmt=FMMT()
        if args.View:
            if args.LogFileType:
                fmmt.View(args.View[0], args.LogFileType[0])
            else:
                fmmt.View(args.View[0])
        if args.Delete:
            if len(args.Delete) == 4:
                fmmt.Delete(args.Delete[0],args.Delete[1],args.Delete[2],args.Delete[3])
            else:
                fmmt.Delete(args.Delete[0],args.Delete[1],args.Delete[2])
        if args.Extract:
            fmmt.Extract(args.Extract[0],args.Extract[1],args.Extract[2])
        if args.Add:
            fmmt.Add(args.Add[0],args.Add[1],args.Add[2],args.Add[3])
        if args.Replace:
            if len(args.Replace) == 5:
                fmmt.Replace(args.Replace[0],args.Replace[1],args.Replace[2],args.Replace[3],args.Replace[4])
            else:
                fmmt.Replace(args.Replace[0],args.Replace[1],args.Replace[2],args.Replace[3])
        # TODO:
        '''Do the main work'''
    except Exception as e:
        print(e)

    return status


if __name__ == "__main__":
    exit(main())
