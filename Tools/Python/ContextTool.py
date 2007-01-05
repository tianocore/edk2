#!/usr/bin/env python

# Copyright (c) 2007, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
# 
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

"""The EDK II Build System Context Tool Utility maintains Target.txt settings
in an EDK II Workspace."""

import wx, os, sys, copy
from EdkIIWorkspace import *

class ContextToolModel(EdkIIWorkspace):
  def __init__(self):
    self.WorkspaceStatus = EdkIIWorkspace.__init__(self)
    self.Database = {}    
    self.OriginalDatabase = {}
        
  def LoadTargetTxtFile(self):
    self.ConvertTextFileToDictionary('Tools/Conf/Target.txt', self.TargetTxtDictionary, '#', '=', True, None)
    if self.TargetTxtDictionary['ACTIVE_PLATFORM'] == []:  
      self.TargetTxtDictionary['ACTIVE_PLATFORM']            = ['']
    else:
      self.TargetTxtDictionary['ACTIVE_PLATFORM']            = [self.TargetTxtDictionary['ACTIVE_PLATFORM'][0]]
    self.TargetTxtDictionary['TOOL_CHAIN_CONF']              = [self.TargetTxtDictionary['TOOL_CHAIN_CONF'][0]]
    self.TargetTxtDictionary['MULTIPLE_THREAD']              = [self.TargetTxtDictionary['MULTIPLE_THREAD'][0]]
    self.TargetTxtDictionary['MAX_CONCURRENT_THREAD_NUMBER'] = [self.TargetTxtDictionary['MAX_CONCURRENT_THREAD_NUMBER'][0]]
    self.TargetTxtDictionary['TARGET']                       = list(set(self.TargetTxtDictionary['TARGET']))
    self.TargetTxtDictionary['TOOL_CHAIN_TAG']               = list(set(self.TargetTxtDictionary['TOOL_CHAIN_TAG']))
    self.TargetTxtDictionary['TARGET_ARCH']                  = list(set(self.TargetTxtDictionary['TARGET_ARCH']))
    if self.TargetTxtDictionary['TARGET'] == []:
      self.TargetTxtDictionary['TARGET'] = ['']
    if self.TargetTxtDictionary['TOOL_CHAIN_TAG'] == []:
      self.TargetTxtDictionary['TOOL_CHAIN_TAG'] = ['']
    if self.TargetTxtDictionary['TARGET_ARCH'] == []:
      self.TargetTxtDictionary['TARGET_ARCH'] = ['']
    self.TargetTxtDictionary['TARGET'].sort()
    self.TargetTxtDictionary['TOOL_CHAIN_TAG'].sort()
    self.TargetTxtDictionary['TARGET_ARCH'].sort()
    self.OriginalTargetTxtDictionary = copy.deepcopy(self.TargetTxtDictionary)
    
  def LoadToolsDefTxtFile(self):
    self.ToolsDefTxtDictionary = {}
    if self.TargetTxtDictionary['TOOL_CHAIN_CONF'] != ['']:
      self.ConvertTextFileToDictionary(self.TargetTxtDictionary['TOOL_CHAIN_CONF'][0], self.ToolsDefTxtDictionary, '#', '=', False, None)

  def LoadFrameworkDatabase(self):
    self.PlatformDatabase = {}
    Fd = self.XmlParseFile ('Tools/Conf/FrameworkDatabase.db')
    PlatformList = XmlList (Fd, '/FrameworkDatabase/PlatformList/Filename')
    for File in PlatformList:
      FpdFileName            = XmlElementData(File)
      FpdPlatformHeader      = self.XmlParseFileSection (FpdFileName, 'PlatformHeader')
      FpdPlatformDefinitions = self.XmlParseFileSection (FpdFileName,'PlatformDefinitions')
      PlatformName           = XmlElement (FpdPlatformHeader, '/PlatformHeader/PlatformName')
      PlatformVersion        = XmlElement (FpdPlatformHeader, '/PlatformHeader/Version')
      PlatformUiName         = PlatformName + '[' + PlatformVersion + ']'
      if PlatformUiName not in self.PlatformDatabase:
        self.PlatformDatabase[PlatformUiName] = {}
      self.PlatformDatabase[PlatformUiName]['XmlFileName']            = FpdFileName
      self.PlatformDatabase[PlatformUiName]['SupportedArchitectures'] = set(XmlElement (FpdPlatformDefinitions, '/PlatformSurfaceArea/PlatformDefinitions/SupportedArchitectures').split(' '))
      self.PlatformDatabase[PlatformUiName]['BuildTargets']           = set(XmlElement (FpdPlatformDefinitions, '/PlatformSurfaceArea/PlatformDefinitions/BuildTargets').split(' '))
      
  def ComputeToolsDefTxtDatabase(self):
    self.ToolsDefTxtDatabase = {
      'TARGET'                       : [],
      'TOOL_CHAIN_TAG'               : [],
      'TARGET_ARCH'                  : []
    }
    for Key in dict(self.ToolsDefTxtDictionary):
      List = Key.split('_')
      if len(List) != 5:
        del self.ToolsDefTxtDictionary[Key]
      elif List[4] == '*':
        del self.ToolsDefTxtDictionary[Key]
      else:
        if List[0] != '*':
          self.ToolsDefTxtDatabase['TARGET'] += [List[0]] 
        if List[1] != '*':
          self.ToolsDefTxtDatabase['TOOL_CHAIN_TAG'] += [List[1]] 
        if List[2] != '*':
          self.ToolsDefTxtDatabase['TARGET_ARCH'] += [List[2]] 
    self.ToolsDefTxtDatabase['TARGET']         = list(set(self.ToolsDefTxtDatabase['TARGET']))
    self.ToolsDefTxtDatabase['TOOL_CHAIN_TAG'] = list(set(self.ToolsDefTxtDatabase['TOOL_CHAIN_TAG']))
    self.ToolsDefTxtDatabase['TARGET_ARCH']    = list(set(self.ToolsDefTxtDatabase['TARGET_ARCH']))
    self.ToolsDefTxtDatabase['TARGET'].sort()
    self.ToolsDefTxtDatabase['TOOL_CHAIN_TAG'].sort()
    self.ToolsDefTxtDatabase['TARGET_ARCH'].sort()

  def NewModel(self):
    self.TargetTxtDictionary = {
      'ACTIVE_PLATFORM'              : [''],
      'TOOL_CHAIN_CONF'              : [''],
      'MULTIPLE_THREAD'              : ['Disable'],
      'MAX_CONCURRENT_THREAD_NUMBER' : ['2'],
      'TARGET'                       : [''],
      'TOOL_CHAIN_TAG'               : [''],
      'TARGET_ARCH'                  : ['']
    }

  def RevertModel(self):
    self.TargetTxtDictionary = copy.deepcopy(self.OriginalTargetTxtDictionary)
    
  def RescanModel(self):
    self.NewModel()
    self.LoadTargetTxtFile()
    
  def RefreshModel(self):
    self.LoadFrameworkDatabase()
    self.LoadToolsDefTxtFile()
    self.ComputeToolsDefTxtDatabase()

    if self.Verbose:
      print self.TargetTxtDictionary
      print 'ActivePlatform    = ', self.TargetTxtDictionary['ACTIVE_PLATFORM'][0]
      print 'ToolChainConf     = ', self.TargetTxtDictionary['TOOL_CHAIN_CONF'][0]
      print 'MultipleThread    = ', self.TargetTxtDictionary['MULTIPLE_THREAD'][0]
      print 'MaxThreads        = ', self.TargetTxtDictionary['MAX_CONCURRENT_THREAD_NUMBER'][0]
      print 'TargetSet         = ', self.TargetTxtDictionary['TARGET']
      print 'ToolChainSet      = ', self.TargetTxtDictionary['TOOL_CHAIN_TAG']
      print 'TargetArchSet     = ', self.TargetTxtDictionary['TARGET_ARCH']
      Platforms = self.PlatformDatabase.keys()
      print 'Possible Settings:'
      print '  Platforms       = ', Platforms
      print '  TargetSet       = ', self.ToolsDefTxtDatabase['TARGET']
      print '  ToolChainSet    = ', self.ToolsDefTxtDatabase['TOOL_CHAIN_TAG']
      print '  TargetArchSet   = ', self.ToolsDefTxtDatabase['TARGET_ARCH']
    return True

  def ModelModified(self):
    if self.TargetTxtDictionary != self.OriginalTargetTxtDictionary:
      return True
    return False

  def SaveModel(self, Filename='Tools/Conf/Target.txt'):
    if self.Verbose:
      for Item in self.TargetTxtDictionary:
        print Item,'=',self.TargetTxtDictionary[Item]
    self.ConvertDictionaryToTextFile(Filename, self.TargetTxtDictionary, '#', '=', True, None)
    self.OriginalTargetTxtDictionary = copy.deepcopy(self.TargetTxtDictionary)

  def CloseModel(self):
    pass
    
class Frame(wx.Frame):
  def __init__(self):
    wx.Frame.__init__(self,None,-1,'EDK II Build System Context Tool')
    panel = wx.Panel(self, style=wx.SUNKEN_BORDER | wx.TAB_TRAVERSAL)
    wx.HelpProvider_Set(wx.SimpleHelpProvider())
    self.Model = ContextToolModel()
    if not self.Model.WorkspaceStatus:
      self.Close()
      return

    #
    #  Help text
    #
    ActivePlatformHelpText = ( 
    "Specifies the Platform Name and Platform Version of the platform that will be "
    "used for build.  If set to [Build Directory] and the current directory contains "
    "an FPD file, then a plaform build on that FPD file will be performed.  If set "
    "to [Build Directory] and there is no FPD file in the current directory, then no "
    "build will be performed."
    )

    ToolChainConfHelpText = ( 
    "Specifies the name of the file that declares all the tools and flag settings "
    "required to complete a build.  This is typically set to Tools/Conf/tools_def.txt."
    )

    MultipleThreadHelpText = (  
    "Flag to enable or disable multi-thread builds. If your computer is multi-core "
    "or contans multiple CPUs, enabling this feature will improve build performance.  "
    "For multi-thread builds, a log will be written to ${BUILD_DIR}/build.log.  This "
    "feature is only for platform builds.  Clean, cleanall, and stand-alone module "
    "builds only use one thread."
    )

    ThreadsHelpText = (
    "The number of concurrent threads. The best performance is achieved if this "
    "value is set to one greater than the number or cores or CPUs in the build system."
    )

    TargetHelpText = (
    "Specifies the set of targets to build.  If set to All, then all build targets "
    "are built.  Otherwise, the subset of enabled build targets are built.  The "
    "standard build targets are RELEASE and DEBUG, but additional user-defined build "
    "targets may be declared in the TOOL_CHAIN_CONF file.  The DEBUG builds with "
    "source level debugging enabled.  RELEASE builds with source level debugging "
    "disabled and results in smaller firmware images."
    )

    ToolChainTagHelpText = (
    "Specifies the set of tool chains to use during a build.  If set to All, then "
    "all of the supported tools chains are used.  Otherwise, only the subset of "
    "enabled tool chains are used.  The TOOL_CHAIN_CONF file declares one or more "
    "tool chains that may be used."
    )

    TargetArchHelpText = (
    "Specifies the set of CPU architectures to build.  If set to All, then all the "
    "CPU architectures supported by the platform FPD file are built.  Otherwise, "
    "only the subset of enabled CPU architectures are built.  The standard CPU  "
    "architectures are IA32, X64, IPF, and EBC, but additional CPU architectures "
    "may be declared in the TOOL_CHAIN_CONF file."
    )
    
    #
    # Status Bar
    #
    self.CreateStatusBar()    
    
    #
    # Build Menus
    #    
    MenuBar = wx.MenuBar()
    
    FileMenu = wx.Menu()
    NewMenuItem    = FileMenu.Append(-1, "&New\tCtrl+N",  "New target.txt")
    SaveMenuItem   = FileMenu.Append(-1, "&Save\tCtrl+S", "Save target.txt")
    SaveAsMenuItem = FileMenu.Append(-1, "Save &As...",   "Save target.txt as...")
    RevertMenuItem = FileMenu.Append(-1, "&Revert",       "Revert to the original target.txt")
    ExitMenuItem   = FileMenu.Append(-1, "E&xit\tAlt+F4", "Exit ContextTool")
    MenuBar.Append(FileMenu, "&File")
    self.Bind(wx.EVT_MENU, self.OnSaveClick,   SaveMenuItem)
    self.Bind(wx.EVT_MENU, self.OnSaveAsClick, SaveAsMenuItem)
    self.Bind(wx.EVT_MENU, self.OnRevertClick, RevertMenuItem)
    self.Bind(wx.EVT_MENU, self.OnExitClick,   ExitMenuItem)

    ViewMenu = wx.Menu()
    RefreshMenuItem = ViewMenu.Append (-1, "&Refresh\tF5", "Rescan target.txt")
    ShowToolBarMenuItem = ViewMenu.AppendCheckItem (-1, "Show &Toolbar", "Shows or hides the toolbar")
    ShowToolBarMenuItem.Check(True)
    MenuBar.Append(ViewMenu, "&View")
    self.Bind(wx.EVT_MENU, self.OnViewRefreshClick, RefreshMenuItem)
    self.Bind(wx.EVT_MENU, self.OnShowToolBarClick, ShowToolBarMenuItem)

    HelpMenu = wx.Menu()
    AboutMenuItem = HelpMenu.Append (-1, "&About...", "About")
    MenuBar.Append(HelpMenu, "&Help")
    self.Bind(wx.EVT_MENU, self.OnAboutClick, AboutMenuItem)
    
    self.SetMenuBar (MenuBar)

    #
    # Build Toolbar
    #    
    self.ShowToolBar = False
    self.OnShowToolBarClick(self)
    
    #
    # Active Platform Combo Box
    #
    ActivePlatformLabel = wx.StaticText(panel, -1, 'ACTIVE_PLATFORM')
    ActivePlatformLabel.SetHelpText(ActivePlatformHelpText)
    self.ActivePlatformText = wx.ComboBox(panel,-1, style=wx.CB_DROPDOWN | wx.CB_SORT | wx.CB_READONLY)
    self.ActivePlatformText.SetHelpText(ActivePlatformHelpText)
    self.ActivePlatformText.Bind(wx.EVT_TEXT, self.OnActivePlatformClick)
    
    #
    # Tool Chain Configuration Text Control and Browse Button for a File Dialog Box
    #
    ToolChainConfFileLabel = wx.StaticText(panel, -1, 'TOOL_CHAIN_CONF')
    ToolChainConfFileLabel.SetHelpText(ToolChainConfHelpText)
    self.ToolChainConfFileText  = wx.TextCtrl(panel, -1, style=wx.TE_PROCESS_ENTER)
    self.ToolChainConfFileText.Bind(wx.EVT_TEXT_ENTER, self.OnToolChainConfClick)
    self.ToolChainConfFileText.Bind(wx.EVT_KILL_FOCUS, self.OnToolChainConfClick)
    self.ToolChainConfFileText.SetHelpText(ToolChainConfHelpText)
    self.BrowseButton = wx.Button(panel, -1, 'Browse...')
    self.BrowseButton.Bind(wx.EVT_BUTTON, self.OnBrowseButtonClick)

    #
    # Multiple Thread enable/disable radio button 
    #
    MultipleThreadLabel = wx.StaticText(panel, -1, 'MULTIPLE_THREAD')
    MultipleThreadLabel.SetHelpText(MultipleThreadHelpText)
    self.MultipleThreadRadioBox = wx.RadioBox(panel, -1, choices=['Enable','Disable'], style=wx.RA_SPECIFY_COLS)
    self.MultipleThreadRadioBox.Bind(wx.EVT_RADIOBOX, self.OnMultipleThreadRadioBox)
    self.MultipleThreadRadioBox.SetHelpText(MultipleThreadHelpText)

    #
    # Thread count spin control
    #
    ThreadsLabel = wx.StaticText(panel, -1, 'THREADS')
    ThreadsLabel.SetHelpText(ThreadsHelpText)
    self.ThreadsSpinCtrl = wx.SpinCtrl(panel, -1, size=(50, -1), min=2)
    self.ThreadsSpinCtrl.Bind(wx.EVT_TEXT, self.OnThreadsSpinCtrl)
    self.ThreadsSpinCtrl.SetHelpText(ThreadsHelpText)

    #
    # Target, ToolChain, and Arch Check List Boxes
    #
    TargetLabel = wx.StaticText(panel, -1, 'TARGET')
    TargetLabel.SetHelpText(TargetHelpText)
    
    ToolChainTagLabel = wx.StaticText(panel, -1, 'TOOL_CHAIN_TAG')
    ToolChainTagLabel.SetHelpText(ToolChainTagHelpText)
    
    TargetArchLabel = wx.StaticText(panel, -1, 'TARGET_ARCH')
    TargetArchLabel.SetHelpText(TargetArchHelpText)
    
    self.TargetCheckListBox = wx.CheckListBox(panel, -1)
    self.TargetCheckListBox.Bind(wx.EVT_CHECKLISTBOX, self.OnTargetCheckListClick)
    self.TargetCheckListBox.Bind(wx.EVT_SET_FOCUS, self.OnTargetSetFocus)
    self.TargetCheckListBox.Bind(wx.EVT_KILL_FOCUS, self.OnTargetKillFocus)
    self.TargetCheckListBox.SetHelpText(TargetHelpText)

    self.ToolChainTagCheckListBox = wx.CheckListBox(panel, -1)
    self.ToolChainTagCheckListBox.Bind(wx.EVT_CHECKLISTBOX, self.OnToolChainTagCheckListClick)
    self.ToolChainTagCheckListBox.Bind(wx.EVT_SET_FOCUS, self.OnToolChainTagSetFocus)
    self.ToolChainTagCheckListBox.Bind(wx.EVT_KILL_FOCUS, self.OnToolChainTagKillFocus)
    self.ToolChainTagCheckListBox.SetHelpText(ToolChainTagHelpText)

    self.TargetArchCheckListBox = wx.CheckListBox(panel, -1)
    self.TargetArchCheckListBox.Bind(wx.EVT_CHECKLISTBOX, self.OnTargetArchCheckListClick)
    self.TargetArchCheckListBox.Bind(wx.EVT_SET_FOCUS, self.OnTargetArchSetFocus)
    self.TargetArchCheckListBox.Bind(wx.EVT_KILL_FOCUS, self.OnTargetArchKillFocus)
    self.TargetArchCheckListBox.SetHelpText(TargetArchHelpText)

    #
    # Define layout using sizers
    #
    self.mainSizer = wx.BoxSizer(wx.VERTICAL)
    
    flexSizer = wx.FlexGridSizer(cols=3, hgap=5, vgap=5)
    flexSizer.AddGrowableCol(1)
    flexSizer.Add(ActivePlatformLabel, 0, wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
    flexSizer.Add(self.ActivePlatformText, 0, wx.EXPAND)
    flexSizer.Add((0,0), wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)

    flexSizer.Add(ToolChainConfFileLabel, 0, wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
    flexSizer.Add(self.ToolChainConfFileText, 0, wx.EXPAND)
    flexSizer.Add(self.BrowseButton, 0, wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)

    self.mainSizer.Add (flexSizer, 0, wx.EXPAND | wx.ALL, 10)    
    
    threadsSizer = wx.FlexGridSizer(cols = 5, hgap=5, vgap=5)
    threadsSizer.Add(MultipleThreadLabel, 0, wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
    threadsSizer.Add(self.MultipleThreadRadioBox, 0, wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)
    threadsSizer.Add(ThreadsLabel, 0, wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
    threadsSizer.Add(self.ThreadsSpinCtrl, 0, wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL)
    
    self.mainSizer.Add (threadsSizer, 0, wx.ALL, 10)

    listSizer = wx.FlexGridSizer(rows = 2, cols = 3, hgap=5, vgap=5)
    listSizer.AddGrowableRow(1)
    listSizer.AddGrowableCol(0)
    listSizer.AddGrowableCol(1)
    listSizer.AddGrowableCol(2)
    listSizer.Add(TargetLabel, 0, wx.ALIGN_CENTER)
    listSizer.Add(ToolChainTagLabel, 0, wx.ALIGN_CENTER)
    listSizer.Add(TargetArchLabel, 0, wx.ALIGN_CENTER)
    listSizer.Add(self.TargetCheckListBox, 0, wx.ALL | wx.EXPAND)
    listSizer.Add(self.ToolChainTagCheckListBox, 0, wx.ALL | wx.EXPAND)
    listSizer.Add(self.TargetArchCheckListBox, 0, wx.ALL | wx.EXPAND)

    self.mainSizer.Add (listSizer, wx.EXPAND | wx.ALL, wx.EXPAND | wx.ALL, 10)    
    
    panel.SetSizer (self.mainSizer)
   
    self.Model.RescanModel()
    self.OnRefreshClick(self)

  def OnActivePlatformClick(self, event):
    Platform = self.ActivePlatformText.GetValue()
    if Platform == ' [Build Directory]':
      self.Model.TargetTxtDictionary['ACTIVE_PLATFORM'][0] = ''
    else:
      self.Model.TargetTxtDictionary['ACTIVE_PLATFORM'][0] = self.Model.PlatformDatabase[Platform]['XmlFileName']

  def OnToolChainConfClick(self, event):
    if self.Model.TargetTxtDictionary['TOOL_CHAIN_CONF'][0] != self.ToolChainConfFileText.GetValue():
      self.Model.TargetTxtDictionary['TOOL_CHAIN_CONF'][0] = self.ToolChainConfFileText.GetValue()
      self.OnRefreshClick(self)
      
  def OnBrowseButtonClick(self, event):
    wildcard = "Text Documents (*.txt)|*.txt|" \
               "All files (*.*)|*.*"
    dialog = wx.FileDialog (None, 'Choose a Tool Chain Configuration File', self.Model.WorkspaceFile('Tools/Conf'), '', wildcard, wx.OPEN)
    if dialog.ShowModal() == wx.ID_OK:
      print dialog.GetPath()
      ToolChainConfFile = self.Model.WorkspaceRelativePath(dialog.GetPath())
      self.ToolChainConfFileText.SetValue(ToolChainConfFile)
      self.Model.TargetTxtDictionary['TOOL_CHAIN_CONF'][0] = self.ToolChainConfFileText.GetValue()
      self.OnRefreshClick(self)
    dialog.Destroy()

  def OnMultipleThreadRadioBox (self, event):
    self.Model.TargetTxtDictionary['MULTIPLE_THREAD'] = [self.MultipleThreadRadioBox.GetStringSelection()]
    if self.MultipleThreadRadioBox.GetStringSelection() == 'Disable':
      self.ThreadsSpinCtrl.Disable()
    else:
      self.ThreadsSpinCtrl.Enable()

  def OnThreadsSpinCtrl(self, event):
    self.Model.TargetTxtDictionary['MAX_CONCURRENT_THREAD_NUMBER'] = [str(self.ThreadsSpinCtrl.GetValue())]

  def CheckListFocus(self, CheckListBox, Set):
    Index = 0
    while Index < CheckListBox.GetCount():
      CheckListBox.SetSelection(Index, False)
      Index += 1
    if Set:
      CheckListBox.SetSelection(0, True)
    
  def CheckListClick(self, CheckListBox, Name):
    if CheckListBox.IsChecked(0):
      Index = 1
      while Index < CheckListBox.GetCount():
        CheckListBox.Check(Index, False)
        Index += 1
    if CheckListBox.IsChecked(0):
      self.Model.TargetTxtDictionary[Name] = ['']
    else:
      self.Model.TargetTxtDictionary[Name] = []
      Index = 1
      while Index < CheckListBox.GetCount():
        if CheckListBox.IsChecked(Index):
          self.Model.TargetTxtDictionary[Name] += [CheckListBox.GetString(Index)]
        Index += 1
      if self.Model.TargetTxtDictionary[Name] == []:
        self.Model.TargetTxtDictionary[Name] = ['']
  
  def OnTargetCheckListClick(self, event):
    self.CheckListClick(self.TargetCheckListBox, 'TARGET')

  def OnTargetSetFocus(self, event):
    self.CheckListFocus(self.TargetCheckListBox, True)

  def OnTargetKillFocus(self, event):
    self.CheckListFocus(self.TargetCheckListBox, False)

  def OnToolChainTagCheckListClick(self, event):
    self.CheckListClick(self.ToolChainTagCheckListBox, 'TOOL_CHAIN_TAG')

  def OnToolChainTagSetFocus(self, event):
    self.CheckListFocus(self.ToolChainTagCheckListBox, True)

  def OnToolChainTagKillFocus(self, event):
    self.CheckListFocus(self.ToolChainTagCheckListBox, False)
    
  def OnTargetArchCheckListClick(self, event):
    self.CheckListClick(self.TargetArchCheckListBox, 'TARGET_ARCH')
  
  def OnTargetArchSetFocus(self, event):
    self.CheckListFocus(self.TargetArchCheckListBox, True)

  def OnTargetArchKillFocus(self, event):
    self.CheckListFocus(self.TargetArchCheckListBox, False)

  def OnRevertClick(self, event):
    self.Model.RevertModel()
    self.OnRefreshClick(self)

  def RefreshCheckListBox(self, CheckListBox, Name):
    CheckListBox.Set(['All'] + self.Model.ToolsDefTxtDatabase[Name])
    Index = 0
    MaximumString = ''
    while Index < CheckListBox.GetCount():
      String = CheckListBox.GetString(Index)
      if len(String) > len(MaximumString):
        MaximumString = String
      if String in self.Model.TargetTxtDictionary[Name]:
        CheckListBox.Check(Index, True)
      else:
        CheckListBox.Check(Index, False)
      Index += 1
    if self.Model.TargetTxtDictionary[Name] == ['']:
      CheckListBox.Check(0, True)
    Extents = CheckListBox.GetFullTextExtent (MaximumString)
    CheckListBox.SetMinSize((Extents[0],(CheckListBox.GetCount()+1) * (Extents[1]+Extents[2])))
    
  def OnRefreshClick(self, event):
    self.Model.RefreshModel()
    Platforms = self.Model.PlatformDatabase.keys()
    Platforms.sort()
    self.ActivePlatformText.SetItems([' [Build Directory]'] + Platforms)
    self.ActivePlatformText.SetValue(' [Build Directory]')
    for Platform in self.Model.PlatformDatabase:
      if self.Model.PlatformDatabase[Platform]['XmlFileName'] == self.Model.TargetTxtDictionary['ACTIVE_PLATFORM'][0]:
        self.ActivePlatformText.SetValue(Platform)
    if self.ActivePlatformText.GetValue() == ' [Build Directory]':
      self.Model.TargetTxtDictionary['ACTIVE_PLATFORM'][0] = ''
    MaximumString = ' [Build Directory]'
    for String in Platforms:
      if len(String) > len(MaximumString):
        MaximumString = String
    Extents = self.ActivePlatformText.GetFullTextExtent (MaximumString)
    self.ActivePlatformText.SetMinSize((Extents[0] + 24,-1))

    self.ToolChainConfFileText.SetValue(self.Model.TargetTxtDictionary['TOOL_CHAIN_CONF'][0])
    Extents = self.ToolChainConfFileText.GetFullTextExtent (self.Model.TargetTxtDictionary['TOOL_CHAIN_CONF'][0])
    self.ToolChainConfFileText.SetMinSize((Extents[0] + 24,-1))
        
    self.MultipleThreadRadioBox.SetStringSelection(self.Model.TargetTxtDictionary['MULTIPLE_THREAD'][0])
    if self.MultipleThreadRadioBox.GetStringSelection() == 'Disable':
      self.ThreadsSpinCtrl.Disable()
    self.ThreadsSpinCtrl.SetValue(int(self.Model.TargetTxtDictionary['MAX_CONCURRENT_THREAD_NUMBER'][0]))

    self.RefreshCheckListBox (self.TargetCheckListBox,       'TARGET')
    self.RefreshCheckListBox (self.ToolChainTagCheckListBox, 'TOOL_CHAIN_TAG')
    self.RefreshCheckListBox (self.TargetArchCheckListBox,   'TARGET_ARCH')

    self.mainSizer.SetSizeHints(self)
    self.mainSizer.Fit(self)

  def OnViewRefreshClick(self, event):
    self.Model.RescanModel()
    self.OnRefreshClick(self)
    
  def AddTool (self, Handler, ArtId, Label, HelpText):
    Tool = self.ToolBar.AddSimpleTool(
      -1, 
      wx.ArtProvider.GetBitmap(ArtId, wx.ART_TOOLBAR, self.ToolSize), 
      Label, 
      HelpText
      )
    self.Bind(wx.EVT_MENU, Handler, Tool)
    
  def OnShowToolBarClick(self, event):
    if self.ShowToolBar:
      self.ShowToolBar = False
      self.ToolBar.Destroy()
    else:
      self.ShowToolBar = True
      self.ToolBar = self.CreateToolBar()
      self.ToolSize = (24,24)
      self.ToolBar.SetToolBitmapSize(self.ToolSize)
      self.AddTool (self.OnNewClick,    wx.ART_NEW,          "New",        "New target.txt")  
      self.AddTool (self.OnSaveClick,   wx.ART_FILE_SAVE,    "Save",       "Save target.txt")  
      self.AddTool (self.OnSaveAsClick, wx.ART_FILE_SAVE_AS, "Save As...", "Save target.txt as...")
      self.AddTool (self.OnRevertClick, wx.ART_UNDO,         "Revert",     "Revert to original target.txt")
      self.AddTool (self.OnHelpClick,   wx.ART_HELP,         "Help",       "Context Sensitive Help")
      self.AddTool (self.OnExitClick,   wx.ART_QUIT,         "Exit",       "Exit Context Tool application")
      self.ToolBar.Realize()
  
  def OnNewClick(self, event):
    self.Model.NewModel()
    self.OnRefreshClick(self)

  def OnSaveClick(self, event):
    self.Model.SaveModel()

  def OnSaveAsClick(self, event):
    wildcard = "Text Documents (*.txt)|*.txt|" \
               "All files (*.*)|*.*"
    dialog = wx.FileDialog (None, 'Save As', self.Model.WorkspaceFile('Tools/Conf'), '', wildcard, wx.SAVE | wx.OVERWRITE_PROMPT)
    if dialog.ShowModal() == wx.ID_OK:
      TargetTxtFile = self.Model.WorkspaceRelativePath(dialog.GetPath())
      if TargetTxtFile != '':
        self.Model.SaveModel(TargetTxtFile)
    dialog.Destroy()
  
  def OnExitClick(self, event):
    if self.Model.ModelModified():
      dialog = wx.MessageDialog(None, 'The contents have changed.\nDo you want to save changes?', 'EDK II Build System Context Tool', style = wx.YES_NO | wx.YES_DEFAULT | wx.CANCEL | wx.ICON_EXCLAMATION)
      Status = dialog.ShowModal()
      dialog.Destroy()
      if Status == wx.ID_YES:
        self.OnSaveClick (self)
      elif Status == wx.ID_CANCEL:
        return
    self.Model.CloseModel()
    self.Close()

  def OnHelpClick(self, event):
    wx.ContextHelp().BeginContextHelp()

  def OnAboutClick(self, event):
    AboutInfo = wx.AboutDialogInfo()
    AboutInfo.Name = 'EDK II Build System Context Tool'
    AboutInfo.Version = '0.3'
    AboutInfo.Copyright = 'Copyright (c) 2006, Intel Corporation'
    AboutInfo.Description = """
      The EDK II Build System Context Tool maintains the target.txt 
      settings in an EDK II Workspace."""
    AboutInfo.WebSite = ("http://tianocore.org", "Tiano Core home page")
    AboutInfo.License = """
      All rights reserved. This program and the accompanying materials are 
      licensed and made available under the terms and conditions of the BSD 
      License which accompanies this distribution.  The full text of the 
      license may be found at http://opensource.org/licenses/bsd-license.php
      
      THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" 
      BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, 
      EITHER EXPRESS OR IMPLIED."""
    if self.Model.Icon != None:
      AboutInfo.Icon = self.Model.Icon
    wx.AboutBox(AboutInfo)
    
if __name__ == '__main__':
  app = wx.PySimpleApp()
  frame = Frame()
  frame.Show()
  app.MainLoop()
