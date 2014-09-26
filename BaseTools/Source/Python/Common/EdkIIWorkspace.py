## @file
# This is the base class for applications that operate on an EDK II Workspace 
#
# Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import Common.LongFilePathOs as os, sys, time
from DataType import *
from Common.LongFilePathSupport import OpenLongFilePath as open

## EdkIIWorkspace
#
# Collect WorkspaceDir from the environment, the Verbose command line flag, and detect an icon bitmap file.
# 
# @var StartTime:       Time of build system starting
# @var PrintRunTime:    Printable time of build system running
# @var PrintRunStatus:  Printable status of build system running
# @var RunStatus:       Status of build system running
#
class EdkIIWorkspace:
    def __init__(self):
        self.StartTime = time.time()
        self.PrintRunTime = False
        self.PrintRunStatus = False
        self.RunStatus = ''
        
        #
        # Check environment valiable 'WORKSPACE'
        #
        if os.environ.get('WORKSPACE') == None:
            print 'ERROR: WORKSPACE not defined.    Please run EdkSetup from the EDK II install directory.'
            return False

        self.CurrentWorkingDir = os.getcwd()
        
        self.WorkspaceDir = os.path.realpath(os.environ.get('WORKSPACE'))
        (Drive, Path) = os.path.splitdrive(self.WorkspaceDir)
        if Drive == '':
            (Drive, CwdPath) = os.path.splitdrive(self.CurrentWorkingDir)
            if Drive != '':
                self.WorkspaceDir = Drive + Path
        else:
            self.WorkspaceDir = Drive.upper() + Path

        self.WorkspaceRelativeWorkingDir = self.WorkspaceRelativePath (self.CurrentWorkingDir)
            
        try:
            #
            # Load TianoCoreOrgLogo, used for GUI tool
            #
            self.Icon = wx.Icon(self.WorkspaceFile('tools/Python/TianoCoreOrgLogo.gif'),wx.BITMAP_TYPE_GIF)
        except:
            self.Icon = None
            
        self.Verbose = False
        for Arg in sys.argv:
            if Arg.lower() == '-v':
                self.Verbose = True
    
    ## Close build system
    #
    # Close build system and print running time and status
    #
    def Close(self):
        if self.PrintRunTime:
            Seconds = int(time.time() - self.StartTime)
            if Seconds < 60:
                print 'Run Time: %d seconds' % (Seconds)
            else:
                Minutes = Seconds / 60
                Seconds = Seconds % 60
                if Minutes < 60:
                    print 'Run Time: %d minutes %d seconds' % (Minutes, Seconds)
                else:
                    Hours = Minutes / 60
                    Minutes = Minutes % 60
                    print 'Run Time: %d hours %d minutes %d seconds' % (Hours, Minutes, Seconds)
        if self.RunStatus != '':
            print self.RunStatus

    ## Convert to a workspace relative filename
    #
    # Convert a full path filename to a workspace relative filename.
    #
    # @param FileName:  The filename to be Converted
    #
    # @retval None    Workspace dir is not found in the full path
    # @retval string  The relative filename
    #
    def WorkspaceRelativePath(self, FileName):
        FileName = os.path.realpath(FileName)
        if FileName.find(self.WorkspaceDir) != 0:
            return None
        return FileName.replace (self.WorkspaceDir, '').strip('\\').strip('/')

    ## Convert to a full path filename
    #
    # Convert a workspace relative filename to a full path filename.
    #
    # @param FileName:  The filename to be Converted
    #
    # @retval string  The full path filename
    #
    def WorkspaceFile(self, FileName):
        return os.path.realpath(os.path.join(self.WorkspaceDir,FileName))

    ## Convert to a real path filename
    #
    # Convert ${WORKSPACE} to real path
    #
    # @param FileName:  The filename to be Converted
    #
    # @retval string  The full path filename
    #
    def WorkspacePathConvert(self, FileName):
        return os.path.realpath(FileName.replace(TAB_WORKSPACE, self.WorkspaceDir))

    ## Convert XML into a DOM
    #
    # Parse an XML file into a DOM and return the DOM.
    #
    # @param FileName:  The filename to be parsed
    #
    # @retval XmlParseFile (self.WorkspaceFile(FileName))
    #
    def XmlParseFile (self, FileName):
        if self.Verbose:
            print FileName
        return XmlParseFile (self.WorkspaceFile(FileName))

    ## Convert a XML section
    #
    # Parse a section of an XML file into a DOM(Document Object Model) and return the DOM.
    #
    # @param FileName:    The filename to be parsed
    # @param SectionTag:  The tag name of the section to be parsed
    #
    # @retval XmlParseFileSection (self.WorkspaceFile(FileName), SectionTag)
    #
    def XmlParseFileSection (self, FileName, SectionTag):
        if self.Verbose:
            print FileName
        return XmlParseFileSection (self.WorkspaceFile(FileName), SectionTag)        

    ## Save a XML file
    #
    # Save a DOM(Document Object Model) into an XML file.
    #
    # @param Dom:       The Dom to be saved
    # @param FileName:  The filename
    #
    # @retval XmlSaveFile (Dom, self.WorkspaceFile(FileName))
    #
    def XmlSaveFile (self, Dom, FileName):
        if self.Verbose:
            print FileName
        return XmlSaveFile (Dom, self.WorkspaceFile(FileName))

    ## Convert Text File To Dictionary
    #
    # Convert a workspace relative text file to a dictionary of (name:value) pairs.
    #
    # @param FileName:             Text filename
    # @param Dictionary:           Dictionary to store data
    # @param CommentCharacter:     Comment char, be used to ignore comment content
    # @param KeySplitCharacter:    Key split char, between key name and key value. Key1 = Value1, '=' is the key split char
    # @param ValueSplitFlag:       Value split flag, be used to decide if has multiple values
    # @param ValueSplitCharacter:  Value split char, be used to split multiple values. Key1 = Value1|Value2, '|' is the value split char
    #
    # @retval ConvertTextFileToDictionary(self.WorkspaceFile(FileName), Dictionary, CommentCharacter, KeySplitCharacter, ValueSplitFlag, ValueSplitCharacter)
    #
    def ConvertTextFileToDictionary(self, FileName, Dictionary, CommentCharacter, KeySplitCharacter, ValueSplitFlag, ValueSplitCharacter):
        if self.Verbose:
            print FileName
        return ConvertTextFileToDictionary(self.WorkspaceFile(FileName), Dictionary, CommentCharacter, KeySplitCharacter, ValueSplitFlag, ValueSplitCharacter)

    ## Convert Dictionary To Text File
    #
    # Convert a dictionary of (name:value) pairs to a workspace relative text file.
    #
    # @param FileName:             Text filename
    # @param Dictionary:           Dictionary to store data
    # @param CommentCharacter:     Comment char, be used to ignore comment content
    # @param KeySplitCharacter:    Key split char, between key name and key value. Key1 = Value1, '=' is the key split char
    # @param ValueSplitFlag:       Value split flag, be used to decide if has multiple values
    # @param ValueSplitCharacter:  Value split char, be used to split multiple values. Key1 = Value1|Value2, '|' is the value split char
    #
    # @retval ConvertDictionaryToTextFile(self.WorkspaceFile(FileName), Dictionary, CommentCharacter, KeySplitCharacter, ValueSplitFlag, ValueSplitCharacter)
    #
    def ConvertDictionaryToTextFile(self, FileName, Dictionary, CommentCharacter, KeySplitCharacter, ValueSplitFlag, ValueSplitCharacter):
        if self.Verbose:
            print FileName
        return ConvertDictionaryToTextFile(self.WorkspaceFile(FileName), Dictionary, CommentCharacter, KeySplitCharacter, ValueSplitFlag, ValueSplitCharacter)

## Convert Text File To Dictionary
#
# Convert a text file to a dictionary of (name:value) pairs.
#
# @param FileName:             Text filename
# @param Dictionary:           Dictionary to store data
# @param CommentCharacter:     Comment char, be used to ignore comment content
# @param KeySplitCharacter:    Key split char, between key name and key value. Key1 = Value1, '=' is the key split char
# @param ValueSplitFlag:       Value split flag, be used to decide if has multiple values
# @param ValueSplitCharacter:  Value split char, be used to split multiple values. Key1 = Value1|Value2, '|' is the value split char
#
# @retval True  Convert successfully
# @retval False Open file failed
#
def ConvertTextFileToDictionary(FileName, Dictionary, CommentCharacter, KeySplitCharacter, ValueSplitFlag, ValueSplitCharacter):
    try:
        F = open(FileName,'r')
    except:
        return False
    Keys = []
    for Line in F:
        LineList = Line.split(KeySplitCharacter,1)
        if len(LineList) >= 2:
            Key = LineList[0].split()
            if len(Key) == 1 and Key[0][0] != CommentCharacter and Key[0] not in Keys: 
                if ValueSplitFlag:
                    Dictionary[Key[0]] = LineList[1].replace('\\','/').split(ValueSplitCharacter)
                else:
                    Dictionary[Key[0]] = LineList[1].strip().replace('\\','/')
                Keys += [Key[0]]
    F.close()
    return True

## Convert Dictionary To Text File
#
# Convert a dictionary of (name:value) pairs to a text file.
#
# @param FileName:             Text filename
# @param Dictionary:           Dictionary to store data
# @param CommentCharacter:     Comment char, be used to ignore comment content
# @param KeySplitCharacter:    Key split char, between key name and key value. Key1 = Value1, '=' is the key split char
# @param ValueSplitFlag:       Value split flag, be used to decide if has multiple values
# @param ValueSplitCharacter:  Value split char, be used to split multiple values. Key1 = Value1|Value2, '|' is the value split char
#
# @retval True  Convert successfully
# @retval False Open file failed
#
def ConvertDictionaryToTextFile(FileName, Dictionary, CommentCharacter, KeySplitCharacter, ValueSplitFlag, ValueSplitCharacter):
    try:
        F = open(FileName,'r')
        Lines = []
        Lines = F.readlines()
        F.close()
    except:
        Lines = []
    Keys = Dictionary.keys()
    MaxLength = 0
    for Key in Keys:
        if len(Key) > MaxLength:
            MaxLength = len(Key)
    Index = 0
    for Line in Lines:
        LineList = Line.split(KeySplitCharacter,1)
        if len(LineList) >= 2:
            Key = LineList[0].split()
            if len(Key) == 1 and Key[0][0] != CommentCharacter and Key[0] in Dictionary:
                if ValueSplitFlag:
                    Line = '%-*s %c %s\n' % (MaxLength, Key[0], KeySplitCharacter, ' '.join(Dictionary[Key[0]]))
                else:
                    Line = '%-*s %c %s\n' % (MaxLength, Key[0], KeySplitCharacter, Dictionary[Key[0]])
                Lines.pop(Index)
                if Key[0] in Keys:
                    Lines.insert(Index,Line)
                    Keys.remove(Key[0])
        Index += 1
    for RemainingKey in Keys:
        if ValueSplitFlag:
            Line = '%-*s %c %s\n' % (MaxLength, RemainingKey, KeySplitCharacter,' '.join(Dictionary[RemainingKey])) 
        else:
            Line = '%-*s %c %s\n' % (MaxLength, RemainingKey, KeySplitCharacter, Dictionary[RemainingKey])
        Lines.append(Line)
    try:
        F = open(FileName,'w')
    except:
        return False
    F.writelines(Lines)
    F.close()
    return True

## Create a new directory
#
# @param Directory:           Directory to be created
#
def CreateDirectory(Directory):
    if not os.access(Directory, os.F_OK):
        os.makedirs (Directory)

## Create a new file
#
# @param Directory:  Directory to be created
# @param FileName:   Filename to be created
# @param Mode:       The mode of open file, defautl is 'w'
#
def CreateFile(Directory, FileName, Mode='w'):
    CreateDirectory (Directory)
    return open(os.path.join(Directory, FileName), Mode)

##
#
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
#
if __name__ == '__main__':
    # Nothing to do here. Could do some unit tests
    pass