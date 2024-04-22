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
from core.FMMTOperation import *

parser = argparse.ArgumentParser(description='''
View the Binary Structure of FD/FV/Ffs/Section, and Delete/Extract/Add/Replace a Ffs from/into a FV.
''')
parser.add_argument("--version", action="version", version='%(prog)s Version 1.0',
                    help="Print debug information.")
parser.add_argument("-v", "--View", dest="View", nargs='+',
                    help="View each FV and the named files within each FV: '-v inputfile outputfile, inputfiletype(.Fd/.Fv/.ffs/.sec)'")
parser.add_argument("-d", "--Delete", dest="Delete", nargs='+',
                    help="Delete a Ffs from FV: '-d inputfile TargetFvName(Optional) TargetFfsName outputfile\
                        If not given TargetFvName, all the existed target Ffs will be deleted'")
parser.add_argument("-e", "--Extract", dest="Extract", nargs='+',
                    help="Extract a Ffs Info: '-e inputfile TargetFvName(Optional) TargetFfsName outputfile\
                        If not given TargetFvName, the first found target Ffs will be extracted.\
                        If only given TargetFvName, not given TargetFfsName, the TargetFv will be extracted to output file'")
parser.add_argument("-a", "--Add", dest="Add", nargs='+',
                    help="Add a Ffs into a FV:'-a inputfile TargetFvName newffsfile outputfile'")
parser.add_argument("-r", "--Replace", dest="Replace", nargs='+',
                    help="Replace a Ffs in a FV: '-r inputfile TargetFvName(Optional) TargetFfsName newffsfile outputfile\
                        If not given TargetFvName, all the existed target Ffs will be replaced with new Ffs file)'")
parser.add_argument("-l", "--LayoutFileName", dest="LayoutFileName", nargs='+',
                    help="The output file which saves Binary layout: '-l xxx.txt'/'-l xxx.json'\
                        If only provide file format as 'txt', \
                        the file will be generated with default name (Layout_'InputFileName'.txt). \
                        Currently supports two formats: json, txt. More formats will be added in the future")
parser.add_argument("-c", "--ConfigFilePath", dest="ConfigFilePath", nargs='+',
                    help="Provide the target FmmtConf.ini file path: '-c C:\\Code\\FmmtConf.ini' \
                        FmmtConf file saves the target guidtool used in compress/uncompress process.\
                        If do not provide, FMMT tool will search the inputfile folder for FmmtConf.ini firstly, if not found,\
                        the FmmtConf.ini saved in FMMT tool's folder will be used as default.")
parser.add_argument("-s", "--ShrinkFv", dest="ShrinkFv", nargs='+',
                    help="Shrink the Fv file: '-s InputFvfile OutputFvfile")

def print_banner():
    print("")

class FMMT():
    def __init__(self) -> None:
        self.firmware_packet = {}

    def SetConfigFilePath(self, configfilepath:str) -> str:
        os.environ['FmmtConfPath'] = os.path.abspath(configfilepath)

    def SetDestPath(self, inputfile:str) -> str:
        os.environ['FmmtConfPath'] = ''
        self.dest_path = os.path.dirname(os.path.abspath(inputfile))
        old_env = os.environ['PATH']
        os.environ['PATH'] = self.dest_path + os.pathsep + old_env

    def CheckFfsName(self, FfsName:str) -> str:
        try:
            return uuid.UUID(FfsName)
        except:
            return FfsName

    def GetFvName(self, FvName:str) -> str:
        try:
            return uuid.UUID(FvName)
        except:
            return FvName

    def View(self, inputfile: str, layoutfilename: str=None, outputfile: str=None) -> None:
        # ViewFile(inputfile, ROOT_TYPE, logfile, outputfile)
        self.SetDestPath(inputfile)
        filetype = os.path.splitext(inputfile)[1].lower()
        if filetype == '.fd':
            ROOT_TYPE = ROOT_TREE
        elif filetype == '.fv':
            ROOT_TYPE = ROOT_FV_TREE
        elif filetype == '.ffs':
            ROOT_TYPE = ROOT_FFS_TREE
        elif filetype == '.sec':
            ROOT_TYPE = ROOT_SECTION_TREE
        elif filetype == '.elf':
            ROOT_TYPE = ROOT_ELF_TREE
        else:
            ROOT_TYPE = ROOT_TREE
        ViewFile(inputfile, ROOT_TYPE, layoutfilename, outputfile)

    def Delete(self, inputfile: str, TargetFfs_name: str, outputfile: str, Fv_name: str=None) -> None:
        self.SetDestPath(inputfile)
        if Fv_name:
            DeleteFfs(inputfile, self.CheckFfsName(TargetFfs_name), outputfile, self.GetFvName(Fv_name))
        else:
            DeleteFfs(inputfile, self.CheckFfsName(TargetFfs_name), outputfile)

    def Extract(self, inputfile: str, Ffs_name: str, outputfile: str, Fv_name: str=None) -> None:
        self.SetDestPath(inputfile)
        if Fv_name:
            ExtractFfs(inputfile, self.CheckFfsName(Ffs_name), outputfile, self.GetFvName(Fv_name))
        else:
            ExtractFfs(inputfile, self.CheckFfsName(Ffs_name), outputfile)

    def Add(self, inputfile: str, Fv_name: str, newffsfile: str, outputfile: str) -> None:
        self.SetDestPath(inputfile)
        AddNewFfs(inputfile, self.CheckFfsName(Fv_name), newffsfile, outputfile)

    def Replace(self,inputfile: str, Ffs_name: str, newffsfile: str, outputfile: str, Fv_name: str=None) -> None:
        self.SetDestPath(inputfile)
        if Fv_name:
            ReplaceFfs(inputfile, self.CheckFfsName(Ffs_name), newffsfile, outputfile, self.GetFvName(Fv_name))
        else:
            ReplaceFfs(inputfile, self.CheckFfsName(Ffs_name), newffsfile, outputfile)

    def Shrink(self,inputfile: str, outputfile: str) -> None:
        self.SetDestPath(inputfile)
        ShrinkFv(inputfile, outputfile)

def main():
    args=parser.parse_args()
    status=0

    try:
        fmmt=FMMT()
        if args.ConfigFilePath:
            fmmt.SetConfigFilePath(args.ConfigFilePath[0])
        if args.View:
            if args.LayoutFileName:
                fmmt.View(args.View[0], args.LayoutFileName[0])
            else:
                fmmt.View(args.View[0])
        elif args.Delete:
            if len(args.Delete) == 4:
                fmmt.Delete(args.Delete[0],args.Delete[2],args.Delete[3],args.Delete[1])
            else:
                fmmt.Delete(args.Delete[0],args.Delete[1],args.Delete[2])
        elif args.Extract:
            if len(args.Extract) == 4:
                fmmt.Extract(args.Extract[0],args.Extract[2],args.Extract[3], args.Extract[1])
            else:
                fmmt.Extract(args.Extract[0],args.Extract[1],args.Extract[2])
        elif args.Add:
            fmmt.Add(args.Add[0],args.Add[1],args.Add[2],args.Add[3])
        elif args.Replace:
            if len(args.Replace) == 5:
                fmmt.Replace(args.Replace[0],args.Replace[2],args.Replace[3],args.Replace[4],args.Replace[1])
            else:
                fmmt.Replace(args.Replace[0],args.Replace[1],args.Replace[2],args.Replace[3])
        elif args.ShrinkFv:
            fmmt.Shrink(args.ShrinkFv[0], args.ShrinkFv[1])
        else:
            parser.print_help()
    except Exception as e:
        print(e)

    return status


if __name__ == "__main__":
    exit(main())
