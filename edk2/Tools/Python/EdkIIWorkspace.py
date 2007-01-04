#!/usr/bin/env python

# This is the base class for applications that operate on an EDK II Workspace 

import os, sys
from XmlRoutines import *

class EdkIIWorkspace:
  def __init__(self):
    """Collect WorkspaceDir from the environment, the Verbose command line flag, and detect an icon bitmap file."""
    if os.environ.get('WORKSPACE') == None:
      print 'ERROR: WORKSPACE not defined.  Please run EdkSetup from the EDK II install directory.'
      return False

    self.WorkspaceDir = os.path.realpath(os.environ.get('WORKSPACE'))
    (Drive, Path) = os.path.splitdrive(self.WorkspaceDir)
    if Drive == '':
      (Drive, CwdPath) = os.path.splitdrive(os.getcwd())
      if Drive != '':
        self.WorkspaceDir = Drive + Path
    else:
      self.WorkspaceDir = Drive.upper() + Path

    try:
      self.Icon = wx.Icon(self.WorkspaceFile('tools/Python/TianoCoreOrgLogo.gif'),wx.BITMAP_TYPE_GIF)
    except:
      self.Icon = None
      
    self.Verbose = False
    for arg in sys.argv:
      if arg.lower() == '-v':
        self.Verbose = True      

    return True

  def WorkspaceRelativePath(self, FileName):
    """Convert a full path filename to a workspace relative filename."""
    FileName = os.path.realpath(FileName)
    if FileName.find(self.WorkspaceDir) != 0:
      return ''
    return FileName.replace (self.WorkspaceDir, '').strip('\\').strip('/')
    
  def WorkspaceFile(self, FileName):
    """Convert a workspace relative filename to a full path filename."""
    return os.path.realpath(os.path.join(self.WorkspaceDir,FileName))

  def XmlParseFile (self, FileName):
    """Parse an XML file into a DOM and return the DOM."""
    if self.Verbose:
      print FileName
    return XmlParseFile (self.WorkspaceFile(FileName))
    
  def XmlParseFileSection (self, FileName, SectionTag):
    """Parse a section of an XML file into a DOM(Document Object Model) and return the DOM."""
    if self.Verbose:
      print FileName
    return XmlParseFileSection (self.WorkspaceFile(FileName), SectionTag)    

  def XmlSaveFile (self, Dom, FileName):
    """Save a DOM(Document Object Model) into an XML file."""
    if self.Verbose:
      print FileName
    return XmlSaveFile (Dom, self.WorkspaceFile(FileName))

  def ConvertTextFileToDictionary(self, FileName, Dictionary, CommentCharacter, KeySplitCharacter, ValueSplitFlag, ValueSplitCharacter):
    """Convert a workspace relative text file to a dictionary of (name:value) pairs."""
    if self.Verbose:
      print FileName
    return ConvertTextFileToDictionary(self.WorkspaceFile(FileName), Dictionary, CommentCharacter, KeySplitCharacter, ValueSplitFlag, ValueSplitCharacter)
  
  def ConvertDictionaryToTextFile(self, FileName, Dictionary, CommentCharacter, KeySplitCharacter, ValueSplitFlag, ValueSplitCharacter):
    """Convert a dictionary of (name:value) pairs to a workspace relative text file."""
    if self.Verbose:
      print FileName
    return ConvertDictionaryToTextFile(self.WorkspaceFile(FileName), Dictionary, CommentCharacter, KeySplitCharacter, ValueSplitFlag, ValueSplitCharacter)

#
# Convert a text file to a dictionary
#
def ConvertTextFileToDictionary(FileName, Dictionary, CommentCharacter, KeySplitCharacter, ValueSplitFlag, ValueSplitCharacter):
  """Convert a text file to a dictionary of (name:value) pairs."""
  try:
    f = open(FileName,'r')
  except:
    return False
  Keys = []
  for Line in f:
    LineList = Line.split(KeySplitCharacter,1)
    if len(LineList) >= 2:
      Key = LineList[0].split()
      if len(Key) == 1 and Key[0][0] != CommentCharacter and Key[0] not in Keys: 
        if ValueSplitFlag:
          Dictionary[Key[0]] = LineList[1].replace('\\','/').split(ValueSplitCharacter)
        else:
          Dictionary[Key[0]] = LineList[1].strip().replace('\\','/')
        Keys += [Key[0]]
  f.close()
  return True

def ConvertDictionaryToTextFile(FileName, Dictionary, CommentCharacter, KeySplitCharacter, ValueSplitFlag, ValueSplitCharacter):
  """Convert a dictionary of (name:value) pairs to a text file."""
  try:
    f = open(FileName,'r')
    Lines = []
    Lines = f.readlines()
    f.close()
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
    f = open(FileName,'w')
  except:
    return False
  f.writelines(Lines)
  f.close()
  return True
    
# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
if __name__ == '__main__':

  # Nothing to do here. Could do some unit tests.
  pass
