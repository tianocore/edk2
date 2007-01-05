#!/usr/bin/env python

# Copyright (c) 2007, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
# 
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

"""An EDK II Build System Framework Database Utility maintains
FrameworkDatabase.db settings in an EDK II Workspace."""

import wx, os, sys, copy
from EdkIIWorkspace import *

class FrameworkDatabaseModel(EdkIIWorkspace):
  def __init__(self):
    self.WorkspaceStatus = EdkIIWorkspace.__init__(self)
    self.Database = {}    
    self.OriginalDatabase = {}
        
  def AddFile (self, DirName, FileName, FileType, Enabled):
    if DirName != '':
      FileName = os.path.join(DirName,FileName)
    if FileType == 'Package':
      Header  = self.XmlParseFileSection (FileName, 'SpdHeader')
      Name    = XmlElement (Header, '/SpdHeader/PackageName')
      Version = XmlElement (Header, '/SpdHeader/Version')
    elif FileType == 'Platform':
      Header  = self.XmlParseFileSection (FileName, 'PlatformHeader')
      Name    = XmlElement (Header, '/PlatformHeader/PlatformName')
      Version = XmlElement (Header, '/PlatformHeader/Version')
    else:
      return
    FileName = FileName.replace('\\','/')
    if Name == '' and Version == '':
      ValidType = 'Invalid'
      OtherType = 'Valid'
      UiName    = FileName
    else:
      ValidType = 'Valid'
      OtherType = 'Invalid'
      UiName    = Name + ' [' + Version + ']'
    self.Database[FileType][OtherType]['PossibleSettings'].pop(FileName, None)
    self.Database[FileType][OtherType]['EnabledSettings'].pop(FileName, None)
    self.Database[FileType][ValidType]['PossibleSettings'][FileName] = UiName
    if Enabled:
      self.Database[FileType][ValidType]['EnabledSettings'][FileName] = UiName
    return

  def NewModel(self):
    self.Database['Platform'] = {'Valid': {'PossibleSettings':{}, 'EnabledSettings':{}},'Invalid': {'PossibleSettings':{}, 'EnabledSettings':{}}}
    self.Database['Package']  = {'Valid': {'PossibleSettings':{}, 'EnabledSettings':{}},'Invalid': {'PossibleSettings':{}, 'EnabledSettings':{}}}

  def RevertModel(self):
    self.Database = copy.deepcopy(self.OriginalDatabase)
    
  def RescanModel(self):
    self.NewModel()
    self.Fd = self.XmlParseFile ('Tools/Conf/FrameworkDatabase.db')
    PackageList = XmlList (self.Fd, '/FrameworkDatabase/PackageList/Filename')
    for File in PackageList:
      SpdFileName = XmlElementData(File)
      self.AddFile ('', SpdFileName, 'Package', True)
    PlatformList = XmlList (self.Fd, '/FrameworkDatabase/PlatformList/Filename')
    for File in PlatformList:
      FpdFileName = XmlElementData(File)
      self.AddFile ('', FpdFileName, 'Platform', True)
    self.OriginalDatabase = copy.deepcopy(self.Database)

  def RefreshModel(self):
    Temp = copy.deepcopy(self.Database)
    for FileType in ['Package','Platform']:
      for Valid in ['Valid','Invalid']:
        for Item in Temp[FileType][Valid]['PossibleSettings']:
          self.AddFile('',Item, FileType, Item in Temp[FileType][Valid]['EnabledSettings'])
    return True

  def ModelModified(self):
    if self.Database['Package']['Valid']['EnabledSettings'] != self.OriginalDatabase['Package']['Valid']['EnabledSettings']:
      return True
    if self.Database['Package']['Invalid']['EnabledSettings'] != self.OriginalDatabase['Package']['Invalid']['EnabledSettings']:
      return True
    if self.Database['Platform']['Valid']['EnabledSettings'] != self.OriginalDatabase['Platform']['Valid']['EnabledSettings']:
      return True
    if self.Database['Platform']['Invalid']['EnabledSettings'] != self.OriginalDatabase['Platform']['Invalid']['EnabledSettings']:
      return True
    return False
    
  def SaveModel(self, Filename='Tools/Conf/FrameworkDatabase.db'):
    EnabledList =  self.Database['Package']['Valid']['EnabledSettings'].keys() 
    EnabledList += self.Database['Package']['Invalid']['EnabledSettings'].keys() 
    PackageList =  XmlList (self.Fd, '/FrameworkDatabase/PackageList/Filename')
    for File in PackageList:
      SpdFileName = XmlElementData(File)
      if SpdFileName in EnabledList:
        EnabledList.remove(SpdFileName)
        continue
      XmlRemoveElement(File)
      
    ParentNode = XmlList (self.Fd, '/FrameworkDatabase/PackageList')[0]
    for SpdFileName in EnabledList:
      XmlAppendChildElement(ParentNode, u'Filename', SpdFileName)

    EnabledList =  self.Database['Platform']['Valid']['EnabledSettings'].keys() 
    EnabledList += self.Database['Platform']['Invalid']['EnabledSettings'].keys() 
    PlatformList = XmlList (self.Fd, '/FrameworkDatabase/PlatformList/Filename')
    for File in PlatformList:
      FpdFileName = XmlElementData(File)
      if FpdFileName in EnabledList:
        EnabledList.remove(FpdFileName)
        continue
      XmlRemoveElement(File)
      
    ParentNode = XmlList (self.Fd, '/FrameworkDatabase/PlatformList')[0]
    for FpdFileName in EnabledList:
      XmlAppendChildElement(ParentNode, u'Filename', FpdFileName)

    self.XmlSaveFile (self.Fd, Filename)
    self.OriginalDatabase = copy.deepcopy(self.Database)

  def CloseModel(self):
    pass
    
class Frame(wx.Frame):
  def __init__(self):
    wx.Frame.__init__(self,None,-1,'EDK II Build System Framework Database Utility')
    panel = wx.Panel(self, style=wx.SUNKEN_BORDER | wx.TAB_TRAVERSAL)
    wx.HelpProvider_Set(wx.SimpleHelpProvider())

    self.Model = FrameworkDatabaseModel()

    #
    #  Help text
    #
    PackagesHelpText = ( 
    "The set of packages that are active in the current WORKSPACE."
    )

    PlatformsHelpText = ( 
    "The set of platforms that are active in the current WORKSPACE."
    )

    InvalidPackagesHelpText = ( 
    "The set of packages that are in Framework Database, but not in the current WORKSPACE."
    )

    InvalidPlatformsHelpText = ( 
    "The set of platforms that are in Framework Database, but not in the current WORKSPACE."
    )
    
    #
    # Status Bar
    #
    self.StatusBar = self.CreateStatusBar()    
    
    #
    # Build Menus
    #    
    MenuBar = wx.MenuBar()
    
    FileMenu = wx.Menu()
    NewMenuItem         = FileMenu.Append(-1, "&New\tCtrl+N",            "New FrameworkDatabase.db")
    SaveMenuItem        = FileMenu.Append(-1, "&Save\tCtrl+S",           "Save FramdworkDatabase.db")
    SaveAsMenuItem      = FileMenu.Append(-1, "Save &As...",             "Save FrameworkDatabase.db as...")
    RevertMenuItem      = FileMenu.Append(-1, "&Revert",                 "Revert to the original FrameworkDatabase.db")
    ScanMenuItem        = FileMenu.Append(-1, "Scan &WORKSPACE\tCtrl+W", "Scan WORKSPACE for additional packages and platforms")
    ScanAndSyncMenuItem = FileMenu.Append(-1, "Scan &WORKSPACE and Sync\tCtrl+W", "Scan WORKSPACE for additional packages and platforms and sync FramdworkDatabase.db")
    ExitMenuItem        = FileMenu.Append(-1, "E&xit\tAlt+F4",           "Exit Framework Database Tool")
    MenuBar.Append(FileMenu, "&File")
    self.Bind(wx.EVT_MENU, self.OnSaveClick,        SaveMenuItem)
    self.Bind(wx.EVT_MENU, self.OnSaveAsClick,      SaveAsMenuItem)
    self.Bind(wx.EVT_MENU, self.OnRevertClick,      RevertMenuItem)
    self.Bind(wx.EVT_MENU, self.OnScanClick,        ScanMenuItem)
    self.Bind(wx.EVT_MENU, self.OnScanAndSyncClick, ScanAndSyncMenuItem)
    self.Bind(wx.EVT_MENU, self.OnExitClick,        ExitMenuItem)

    EditMenu = wx.Menu()
    SelectAllPlatformsMenuItem = EditMenu.Append (-1, "Select All Platforms", "Select all platforms")
    ClearAllPlatformsMenuItem  = EditMenu.Append (-1, "Clear All Platforms",  "Clear all platforms")
    SelectAllPackagesMenuItem  = EditMenu.Append (-1, "Select All Packages",  "Select all packages")
    ClearAllPackagesMenuItem   = EditMenu.Append (-1, "Clear All Packages",   "Clear all packages")
    SelectAllInvalidPlatformsMenuItem = EditMenu.Append (-1, "Select All Invalid Platforms", "Select all invalid platforms")
    ClearAllInvalidPlatformsMenuItem  = EditMenu.Append (-1, "Clear All Invalid Platforms",  "Clear all invalid platforms")
    SelectAllInvalidPackagesMenuItem  = EditMenu.Append (-1, "Select All Invalid Packages",  "Select all invalid packages")
    ClearAllInvalidPackagesMenuItem   = EditMenu.Append (-1, "Clear All Invalid Packages",   "Clear all invalid packages")
    MenuBar.Append(EditMenu, "&Edit")
    self.Bind(wx.EVT_MENU, self.OnSelectAllPlatformsClick, SelectAllPlatformsMenuItem)
    self.Bind(wx.EVT_MENU, self.OnClearAllPlatformsClick,  ClearAllPlatformsMenuItem)
    self.Bind(wx.EVT_MENU, self.OnSelectAllPackagesClick,  SelectAllPackagesMenuItem)
    self.Bind(wx.EVT_MENU, self.OnClearAllPackagesClick,   ClearAllPackagesMenuItem)
    self.Bind(wx.EVT_MENU, self.OnSelectAllInvalidPlatformsClick, SelectAllInvalidPlatformsMenuItem)
    self.Bind(wx.EVT_MENU, self.OnClearAllInvalidPlatformsClick,  ClearAllInvalidPlatformsMenuItem)
    self.Bind(wx.EVT_MENU, self.OnSelectAllInvalidPackagesClick,  SelectAllInvalidPackagesMenuItem)
    self.Bind(wx.EVT_MENU, self.OnClearAllInvalidPackagesClick,   ClearAllInvalidPackagesMenuItem)
    
    ViewMenu = wx.Menu()
    RefreshMenuItem = ViewMenu.Append (-1, "&Refresh\tF5", "Rescan FrameworkDatabase.db")
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
    # Target, ToolChain, and Arch Check List Boxes
    #
    PackagesLabel = wx.StaticText(panel, -1, 'Packages')
    PackagesLabel.SetFont(wx.Font(12, wx.FONTFAMILY_DEFAULT, wx.NORMAL, wx.FONTWEIGHT_BOLD))
    PackagesLabel.SetHelpText(PackagesHelpText)

    PlatformsLabel = wx.StaticText(panel, -1, 'Platforms')
    PlatformsLabel.SetFont(wx.Font(12, wx.FONTFAMILY_DEFAULT, wx.NORMAL, wx.FONTWEIGHT_BOLD))
    PlatformsLabel.SetHelpText(PlatformsHelpText)
    
    #
    # Buttons
    #
    self.SelectAllPackagesButton  = wx.Button(panel, -1, 'Select All')
    self.ClearAllPackagesButton   = wx.Button(panel, -1, 'Clear All')
    self.SelectAllPackagesButton.Bind (wx.EVT_BUTTON, self.OnSelectAllPackagesClick)
    self.ClearAllPackagesButton.Bind  (wx.EVT_BUTTON, self.OnClearAllPackagesClick)
    
    self.PackagesCheckListBox = wx.CheckListBox(panel, -1)
    self.PackagesCheckListBox.Bind(wx.EVT_CHECKLISTBOX, self.OnPackagesCheckListClick)
    self.PackagesCheckListBox.Bind(wx.EVT_SET_FOCUS, self.OnPackagesSetFocus)
    self.PackagesCheckListBox.Bind(wx.EVT_KILL_FOCUS, self.OnPackagesKillFocus)
    self.PackagesCheckListBox.SetHelpText(PackagesHelpText)

    
    self.SelectAllPlatformsButton = wx.Button(panel, -1, 'Select All')
    self.ClearAllPlatformsButton  = wx.Button(panel, -1, 'Clear All')
    self.SelectAllPlatformsButton.Bind(wx.EVT_BUTTON, self.OnSelectAllPlatformsClick)
    self.ClearAllPlatformsButton.Bind (wx.EVT_BUTTON, self.OnClearAllPlatformsClick)

    self.PlatformsCheckListBox = wx.CheckListBox(panel, -1)
    self.PlatformsCheckListBox.Bind(wx.EVT_CHECKLISTBOX, self.OnPlatformsCheckListClick)
    self.PlatformsCheckListBox.Bind(wx.EVT_SET_FOCUS, self.OnPlatformsSetFocus)
    self.PlatformsCheckListBox.Bind(wx.EVT_KILL_FOCUS, self.OnPlatformsKillFocus)
    self.PlatformsCheckListBox.SetHelpText(PlatformsHelpText)

    InvalidPackagesLabel = wx.StaticText(panel, -1, 'Invalid Packages')
    InvalidPackagesLabel.SetFont(wx.Font(12, wx.FONTFAMILY_DEFAULT, wx.NORMAL, wx.FONTWEIGHT_BOLD))
    InvalidPackagesLabel.SetHelpText(InvalidPackagesHelpText)

    InvalidPlatformsLabel = wx.StaticText(panel, -1, 'Invalid Platforms')
    InvalidPlatformsLabel.SetFont(wx.Font(12, wx.FONTFAMILY_DEFAULT, wx.NORMAL, wx.FONTWEIGHT_BOLD))
    InvalidPlatformsLabel.SetHelpText(InvalidPlatformsHelpText)
    
    self.SelectAllInvalidPackagesButton  = wx.Button(panel, -1, 'Select All')
    self.ClearAllInvalidPackagesButton   = wx.Button(panel, -1, 'Clear All')
    self.SelectAllInvalidPackagesButton.Bind (wx.EVT_BUTTON, self.OnSelectAllInvalidPackagesClick)
    self.ClearAllInvalidPackagesButton.Bind  (wx.EVT_BUTTON, self.OnClearAllInvalidPackagesClick)

    self.InvalidPackagesCheckListBox = wx.CheckListBox(panel, -1)
    self.InvalidPackagesCheckListBox.Bind(wx.EVT_CHECKLISTBOX, self.OnInvalidPackagesCheckListClick)
    self.InvalidPackagesCheckListBox.Bind(wx.EVT_SET_FOCUS, self.OnInvalidPackagesSetFocus)
    self.InvalidPackagesCheckListBox.Bind(wx.EVT_KILL_FOCUS, self.OnInvalidPackagesKillFocus)
    self.InvalidPackagesCheckListBox.SetHelpText(PackagesHelpText)

    self.SelectAllInvalidPlatformsButton = wx.Button(panel, -1, 'Select All')
    self.ClearAllInvalidPlatformsButton  = wx.Button(panel, -1, 'Clear All')
    self.SelectAllInvalidPlatformsButton.Bind(wx.EVT_BUTTON, self.OnSelectAllInvalidPlatformsClick)
    self.ClearAllInvalidPlatformsButton.Bind (wx.EVT_BUTTON, self.OnClearAllInvalidPlatformsClick)

    self.InvalidPlatformsCheckListBox = wx.CheckListBox(panel, -1)
    self.InvalidPlatformsCheckListBox.Bind(wx.EVT_CHECKLISTBOX, self.OnInvalidPlatformsCheckListClick)
    self.InvalidPlatformsCheckListBox.Bind(wx.EVT_SET_FOCUS, self.OnInvalidPlatformsSetFocus)
    self.InvalidPlatformsCheckListBox.Bind(wx.EVT_KILL_FOCUS, self.OnInvalidPlatformsKillFocus)
    self.InvalidPlatformsCheckListBox.SetHelpText(PlatformsHelpText)

    #
    # Define layout using sizers
    #
    self.mainSizer = wx.BoxSizer(wx.VERTICAL)

    listSizer = wx.GridBagSizer(hgap=5, vgap=5)
    listSizer.Add(PackagesLabel,  pos=(0,0), span=(1,2), flag=wx.ALIGN_CENTER)
    listSizer.Add(PlatformsLabel, pos=(0,2), span=(1,2), flag=wx.ALIGN_CENTER)
    listSizer.Add(self.SelectAllPackagesButton, pos=(1,0), flag=wx.ALIGN_CENTER)
    listSizer.Add(self.ClearAllPackagesButton,  pos=(1,1), flag=wx.ALIGN_CENTER)
    listSizer.Add(self.SelectAllPlatformsButton, pos=(1,2), flag=wx.ALIGN_CENTER)
    listSizer.Add(self.ClearAllPlatformsButton,  pos=(1,3), flag=wx.ALIGN_CENTER)
    listSizer.Add(self.PackagesCheckListBox,  pos=(2,0), span=(1,2), flag=wx.ALL | wx.EXPAND)
    listSizer.Add(self.PlatformsCheckListBox, pos=(2,2), span=(1,2), flag=wx.ALL | wx.EXPAND)

    listSizer.Add(InvalidPackagesLabel,  pos=(3,0), span=(1,2), flag=wx.ALIGN_CENTER)
    listSizer.Add(InvalidPlatformsLabel, pos=(3,2), span=(1,2), flag=wx.ALIGN_CENTER)
    listSizer.Add(self.SelectAllInvalidPackagesButton, pos=(4,0), flag=wx.ALIGN_CENTER)
    listSizer.Add(self.ClearAllInvalidPackagesButton,  pos=(4,1), flag=wx.ALIGN_CENTER)
    listSizer.Add(self.SelectAllInvalidPlatformsButton, pos=(4,2), flag=wx.ALIGN_CENTER)
    listSizer.Add(self.ClearAllInvalidPlatformsButton,  pos=(4,3), flag=wx.ALIGN_CENTER)
    listSizer.Add(self.InvalidPackagesCheckListBox,  pos=(5,0), span=(1,2), flag=wx.ALL | wx.EXPAND)
    listSizer.Add(self.InvalidPlatformsCheckListBox, pos=(5,2), span=(1,2), flag=wx.ALL | wx.EXPAND)

    listSizer.AddGrowableRow(2)
    listSizer.AddGrowableRow(5)
    listSizer.AddGrowableCol(0)
    listSizer.AddGrowableCol(1)
    listSizer.AddGrowableCol(2)
    listSizer.AddGrowableCol(3)
    
    self.mainSizer.Add (listSizer, wx.EXPAND | wx.ALL, wx.EXPAND | wx.ALL, 10)    
    
    panel.SetSizer (self.mainSizer)

    self.OnViewRefreshClick(self)

  def CheckListFocus(self, CheckListBox, Set):
    Index = 0
    while Index < CheckListBox.GetCount():
      CheckListBox.SetSelection(Index, False)
      Index += 1
    if Set and CheckListBox.GetCount() > 0:
      CheckListBox.SetSelection(0, True)
    
  def CheckListClick(self, CheckListBox, Database):
    Index = 0
    Database['EnabledSettings'] = {}
    while Index < CheckListBox.GetCount():
      if CheckListBox.IsChecked(Index):
        for Item in Database['PossibleSettings']:
          if Database['PossibleSettings'][Item] == CheckListBox.GetString(Index):
            Database['EnabledSettings'][Item] = Database['PossibleSettings'][Item]
      Index += 1
     
  def OnPackagesCheckListClick(self, event):
    self.CheckListClick(self.PackagesCheckListBox, self.Model.Database['Package']['Valid'])

  def OnPackagesSetFocus(self, event):
    self.CheckListFocus(self.PackagesCheckListBox, True)

  def OnPackagesKillFocus(self, event):
    self.CheckListFocus(self.PackagesCheckListBox, False)
    
  def OnPlatformsCheckListClick(self, event):
    self.CheckListClick(self.PlatformsCheckListBox, self.Model.Database['Platform']['Valid'])

  def OnPlatformsSetFocus(self, event):
    self.CheckListFocus(self.PlatformsCheckListBox, True)

  def OnPlatformsKillFocus(self, event):
    self.CheckListFocus(self.PlatformsCheckListBox, False)

  def OnInvalidPackagesCheckListClick(self, event):
    self.CheckListClick(self.InvalidPackagesCheckListBox, self.Model.Database['Package']['Invalid'])

  def OnInvalidPackagesSetFocus(self, event):
    self.CheckListFocus(self.InvalidPackagesCheckListBox, True)

  def OnInvalidPackagesKillFocus(self, event):
    self.CheckListFocus(self.InvalidPackagesCheckListBox, False)
    
  def OnInvalidPlatformsCheckListClick(self, event):
    self.CheckListClick(self.InvalidPlatformsCheckListBox, self.Model.Database['Platform']['Invalid'])

  def OnInvalidPlatformsSetFocus(self, event):
    self.CheckListFocus(self.InvalidPlatformsCheckListBox, True)

  def OnInvalidPlatformsKillFocus(self, event):
    self.CheckListFocus(self.InvalidPlatformsCheckListBox, False)
    
  def OnRevertClick(self, event):
    self.Model.RevertModel()
    self.StatusBar.SetFocus()
    self.OnRefreshClick(self)

  def RefreshCheckListBox(self, CheckListBox, SelectAllButton, ClearAllButton, Database):
    NameList = []
    for Item in Database['PossibleSettings']:
      NameList.append(Database['PossibleSettings'][Item])
    NameList.sort()
    CheckListBox.Set(NameList)
    Index = 0
    MaximumString = '.'
    while Index < CheckListBox.GetCount():
      String = CheckListBox.GetString(Index)
      if len(String) > len(MaximumString):
        MaximumString = String
      Enabled = False
      for Item in Database['EnabledSettings']:
        if String == Database['EnabledSettings'][Item]:
          Enabled = True
      if Enabled:
        CheckListBox.Check(Index, True)
      else:
        CheckListBox.Check(Index, False)
      Index += 1
    Extents = CheckListBox.GetFullTextExtent (MaximumString)
    CheckListBox.SetMinSize((Extents[0] + 30,(CheckListBox.GetCount()+2) * (Extents[1]+Extents[2])))
    if NameList == []:
      CheckListBox.Disable()
      SelectAllButton.Disable()
      ClearAllButton.Disable()
    else:
      CheckListBox.Enable()
      SelectAllButton.Enable()
      ClearAllButton.Enable()
    
  def OnRefreshClick(self, event):
    self.Model.RefreshModel()
    self.RefreshCheckListBox (self.PackagesCheckListBox,  self.SelectAllPackagesButton, self.ClearAllPackagesButton, self.Model.Database['Package']['Valid'])
    self.RefreshCheckListBox (self.PlatformsCheckListBox, self.SelectAllPlatformsButton, self.ClearAllPlatformsButton, self.Model.Database['Platform']['Valid'])
    self.RefreshCheckListBox (self.InvalidPackagesCheckListBox,  self.SelectAllInvalidPackagesButton, self.ClearAllInvalidPackagesButton, self.Model.Database['Package']['Invalid'])
    self.RefreshCheckListBox (self.InvalidPlatformsCheckListBox, self.SelectAllInvalidPlatformsButton, self.ClearAllInvalidPlatformsButton, self.Model.Database['Platform']['Invalid'])
    self.mainSizer.SetSizeHints(self)
    self.mainSizer.Fit(self)
    self.Update()

  def OnViewRefreshClick(self, event):
    self.Model.RescanModel()
    self.StatusBar.SetFocus()
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
      self.AddTool (self.OnNewClick,         wx.ART_NEW,          "New",            "New FrameworkDatabase.db")  
      self.AddTool (self.OnScanAndSyncClick, wx.ART_HARDDISK,     "Scan WORKSPACE and Sync", "Scan WORKSPACE for new Packages and Platforms and sync FrameworkDatabase.db")  
      self.AddTool (self.OnSaveClick,        wx.ART_FILE_SAVE,    "Save",           "Save FrameworkDatabase.db")
      self.AddTool (self.OnSaveAsClick,      wx.ART_FILE_SAVE_AS, "Save As...",     "Save FrameworkDatabase.db as...")
      self.AddTool (self.OnRevertClick,      wx.ART_UNDO,         "Revert",         "Revert to original FrameworkDatabase.db")
      self.AddTool (self.OnHelpClick,        wx.ART_HELP,         "Help",           "Context Sensitive Help")
      self.AddTool (self.OnExitClick,        wx.ART_QUIT,         "Exit",           "Exit EDK II Build System Framework Database Utility")
      self.ToolBar.Realize()
  
  def OnNewClick(self, event):
    self.Model.NewModel()
    self.OnRefreshClick(self)

  def ScanDirectory(self, Data, DirName, FilesInDir):
    WorkspaceDirName = self.Model.WorkspaceRelativePath(DirName)
    self.StatusBar.SetStatusText('Scanning: ' + WorkspaceDirName)
    RemoveList = []
    for File in FilesInDir:
      if File[0] == '.':
        RemoveList.insert(0, File)
    for File in RemoveList:
      FilesInDir.remove(File)
    for File in FilesInDir:
      if os.path.splitext(File)[1].lower() == '.spd':
        self.Model.AddFile (WorkspaceDirName, File, 'Package', False)
        self.OnRefreshClick(self)
      if os.path.splitext(File)[1].lower() == '.fpd':
        self.Model.AddFile (WorkspaceDirName, File, 'Platform', False)
        self.OnRefreshClick(self)
    
  def OnScanClick(self, event):
    os.path.walk(self.Model.WorkspaceFile(''), self.ScanDirectory, None)
    self.StatusBar.SetStatusText('Scanning: Complete')
    self.StatusBar.SetFocus()
    self.OnRefreshClick(self)

  def OnScanAndSyncClick(self, event):
    self.OnSelectAllPackagesClick(self)
    self.OnSelectAllPlatformsClick(self)
    self.OnClearAllInvalidPackagesClick(self)
    self.OnClearAllInvalidPlatformsClick(self)
    self.OnScanClick(self)
    self.OnSelectAllPackagesClick(self)
    self.OnSelectAllPlatformsClick(self)
    self.OnClearAllInvalidPackagesClick(self)
    self.OnClearAllInvalidPlatformsClick(self)
    
  def OnSelectAllPackagesClick(self, event):
    self.Model.Database['Package']['Valid']['EnabledSettings'] = self.Model.Database['Package']['Valid']['PossibleSettings']
    self.OnRefreshClick(self)

  def OnClearAllPackagesClick(self, event):
    self.Model.Database['Package']['Valid']['EnabledSettings'] = {}
    self.OnRefreshClick(self)

  def OnSelectAllPlatformsClick(self, event):
    self.Model.Database['Platform']['Valid']['EnabledSettings'] = self.Model.Database['Platform']['Valid']['PossibleSettings']
    self.OnRefreshClick(self)

  def OnClearAllPlatformsClick(self, event):
    self.Model.Database['Platform']['Valid']['EnabledSettings'] = {}
    self.OnRefreshClick(self)

  def OnSelectAllInvalidPackagesClick(self, event):
    self.Model.Database['Package']['Invalid']['EnabledSettings'] = self.Model.Database['Package']['Invalid']['PossibleSettings']
    self.OnRefreshClick(self)

  def OnClearAllInvalidPackagesClick(self, event):
    self.Model.Database['Package']['Invalid']['EnabledSettings'] = {}
    self.OnRefreshClick(self)

  def OnSelectAllInvalidPlatformsClick(self, event):
    self.Model.Database['Platform']['Invalid']['EnabledSettings'] = self.Model.Database['Platform']['Invalid']['PossibleSettings']
    self.OnRefreshClick(self)

  def OnClearAllInvalidPlatformsClick(self, event):
    self.Model.Database['Platform']['Invalid']['EnabledSettings'] = {}
    self.OnRefreshClick(self)
    
  def OnSaveClick(self, event):
    self.Model.SaveModel()

  def OnSaveAsClick(self, event):
    wildcard = "Text Documents (*.db)|*.db|" \
               "All files (*.*)|*.*"
    dialog = wx.FileDialog (None, 'Save As', self.Model.WorkspaceFile('Tools/Conf'), '', wildcard, wx.SAVE | wx.OVERWRITE_PROMPT)
    if dialog.ShowModal() == wx.ID_OK:
      FrameworkDatabaseDbFile = self.Model.WorkspaceRelativePath(dialog.GetPath())
      if FrameworkDatabaseDbFile != '':
        self.Model.SaveModel(FrameworkDatabaseDbFile)
    dialog.Destroy()
  
  def OnExitClick(self, event):
    if self.Model.ModelModified():
      dialog = wx.MessageDialog(None, 'The contents have changed.\nDo you want to save changes?', 'EDK II Build System Framework Databsase Utility', style = wx.YES_NO | wx.YES_DEFAULT | wx.CANCEL | wx.ICON_EXCLAMATION)
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
    AboutInfo.Name = 'EDK II Build System Framework Database Utility'
    AboutInfo.Version = '0.3'
    AboutInfo.Copyright = 'Copyright (c) 2006, Intel Corporation'
    AboutInfo.Description = """
      The EDK II Build System Framework Database Utility maintains FrameworkDatabase.db 
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
  
