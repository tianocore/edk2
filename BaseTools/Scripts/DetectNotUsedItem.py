## @file
# Detect unreferenced PCD and GUID/Protocols/PPIs.
#
# Copyright (c) 2019, Intel Corporation. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
DetectNotUsedItem
'''
import re
import os
import sys
import argparse

#
# Globals for help information
#
__prog__ = 'DetectNotUsedItem'
__version__ = '%s Version %s' % (__prog__, '0.1')
__copyright__ = 'Copyright (c) 2019, Intel Corporation. All rights reserved.'
__description__ = "Detect unreferenced PCD and GUID/Protocols/PPIs.\n"

SectionList = ["LibraryClasses", "Guids", "Ppis", "Protocols", "Pcd"]


class PROCESS(object):

    def __init__(self, DecPath, InfDirs):
        self.Dec = DecPath
        self.InfPath = InfDirs
        self.Log = []

    def ParserDscFdfInfFile(self):
        AllContentList = []
        for File in self.SearchbyExt([".dsc", ".fdf", ".inf"]):
            AllContentList += self.ParseDscFdfInfContent(File)
        return AllContentList

    # Search File by extension name
    def SearchbyExt(self, ExtList):
        FileList = []
        for path in self.InfPath:
            if type(ExtList) == type(''):
                for root, _, files in os.walk(path, topdown=True, followlinks=False):
                    for filename in files:
                        if filename.endswith(ExtList):
                            FileList.append(os.path.join(root, filename))
            elif type(ExtList) == type([]):
                for root, _, files in os.walk(path, topdown=True, followlinks=False):
                    for filename in files:
                        for Ext in ExtList:
                            if filename.endswith(Ext):
                                FileList.append(os.path.join(root, filename))
        return FileList

    # Parse DEC file to get Line number and Name
    # return section name, the Item Name and comments line number
    def ParseDecContent(self):
        SectionRE = re.compile(r'\[(.*)\]')
        Flag = False
        Comments = {}
        Comment_Line = []
        ItemName = {}
        with open(self.Dec, 'r') as F:
            for Index, content in enumerate(F):
                NotComment = not content.strip().startswith("#")
                Section = SectionRE.findall(content)
                if Section and NotComment:
                    Flag = self.IsNeedParseSection(Section[0])
                if Flag:
                    Comment_Line.append(Index)
                    if NotComment:
                        if content != "\n" and content != "\r\n":
                            ItemName[Index] = content.split('=')[0].split('|')[0].split('#')[0].strip()
                            Comments[Index] = Comment_Line
                            Comment_Line = []
        return ItemName, Comments

    def IsNeedParseSection(self, SectionName):
        for item in SectionList:
            if item in SectionName:
                return True
        return False

    # Parse DSC, FDF, INF File, remove comments, return Lines list
    def ParseDscFdfInfContent(self, File):
        with open(File, 'r') as F:
            lines = F.readlines()
        for Index in range(len(lines) - 1, -1, -1):
            if lines[Index].strip().startswith("#") or lines[Index] == "\n" or lines[Index] == "\r\n":
                lines.remove(lines[Index])
            elif "#" in lines[Index]:
                lines[Index] = lines[Index].split("#")[0].strip()
            else:
                lines[Index] = lines[Index].strip()
        return lines

    def DetectNotUsedItem(self):
        NotUsedItem = {}
        DecItem, DecComments = self.ParseDecContent()
        InfDscFdfContent = self.ParserDscFdfInfFile()
        for LineNum in list(DecItem.keys()):
            DecItemName = DecItem[LineNum]
            Match_reg = re.compile("(?<![a-zA-Z0-9_-])%s(?![a-zA-Z0-9_-])" % DecItemName)
            MatchFlag = False
            for Line in InfDscFdfContent:
                if Match_reg.search(Line):
                    MatchFlag = True
                    break
            if not MatchFlag:
                NotUsedItem[LineNum] = DecItemName
        self.Display(NotUsedItem)
        return NotUsedItem, DecComments

    def Display(self, UnuseDict):
        print("DEC File:\n%s\n%s%s" % (self.Dec, "{:<15}".format("Line Number"), "{:<0}".format("Unused Item")))
        self.Log.append(
            "DEC File:\n%s\n%s%s\n" % (self.Dec, "{:<15}".format("Line Number"), "{:<0}".format("Unused Item")))
        for num in list(sorted(UnuseDict.keys())):
            ItemName = UnuseDict[num]
            print("%s%s%s" % (" " * 3, "{:<12}".format(num + 1), "{:<1}".format(ItemName)))
            self.Log.append(("%s%s%s\n" % (" " * 3, "{:<12}".format(num + 1), "{:<1}".format(ItemName))))

    def Clean(self, UnUseDict, Comments):
        removednum = []
        for num in list(UnUseDict.keys()):
            if num in list(Comments.keys()):
                removednum += Comments[num]
        with open(self.Dec, 'r') as Dec:
            lines = Dec.readlines()
        try:
            with open(self.Dec, 'w+') as T:
                for linenum in range(len(lines)):
                    if linenum in removednum:
                        continue
                    else:
                        T.write(lines[linenum])
            print("DEC File has been clean: %s" % (self.Dec))
        except Exception as err:
            print(err)


class Main(object):

    def mainprocess(self, Dec, Dirs, Isclean, LogPath):
        for dir in Dirs:
            if not os.path.exists(dir):
                print("Error: Invalid path for '--dirs': %s" % dir)
                sys.exit(1)
        Pa = PROCESS(Dec, Dirs)
        unuse, comment = Pa.DetectNotUsedItem()
        if Isclean:
            Pa.Clean(unuse, comment)
        self.Logging(Pa.Log, LogPath)

    def Logging(self, content, LogPath):
        if LogPath:
            try:
                if os.path.isdir(LogPath):
                    FilePath = os.path.dirname(LogPath)
                    if not os.path.exists(FilePath):
                        os.makedirs(FilePath)
                with open(LogPath, 'w+') as log:
                    for line in content:
                        log.write(line)
                print("Log save to file: %s" % LogPath)
            except Exception as e:
                print("Save log Error: %s" % e)


def main():
    parser = argparse.ArgumentParser(prog=__prog__,
                                     description=__description__ + __copyright__,
                                     conflict_handler='resolve')
    parser.add_argument('-i', '--input', metavar="", dest='InputDec', help="Input DEC file name.")
    parser.add_argument('--dirs', metavar="", action='append', dest='Dirs',
                        help="The package directory. To specify more directories, please repeat this option.")
    parser.add_argument('--clean', action='store_true', default=False, dest='Clean',
                        help="Clean the unreferenced items from DEC file.")
    parser.add_argument('--log', metavar="", dest="Logfile", default=False,
                        help="Put log in specified file as well as on console.")
    options = parser.parse_args()
    if options.InputDec:
        if not (os.path.exists(options.InputDec) and options.InputDec.endswith(".dec")):
            print("Error: Invalid DEC file input: %s" % options.InputDec)
        if options.Dirs:
            M = Main()
            M.mainprocess(options.InputDec, options.Dirs, options.Clean, options.Logfile)
        else:
            print("Error: the following argument is required:'--dirs'.")
    else:
        print("Error: the following argument is required:'-i/--input'.")


if __name__ == '__main__':
    main()
