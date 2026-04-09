## @file
# This file is used to define common string related functions used in parsing
# process
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import os, sys, wx, logging

import wx.stc
import wx.lib.newevent
import wx.lib.agw.genericmessagedialog as GMD
from plugins.EdkPlugins.edk2.model import baseobject
from plugins.EdkPlugins.edk2.model import doxygengen

if hasattr(sys, "frozen"):
    appPath = os.path.abspath(os.path.dirname(sys.executable))
else:
    appPath = os.path.abspath(os.path.dirname(__file__))

AppCallBackEvent, EVT_APP_CALLBACK = wx.lib.newevent.NewEvent()
LogEvent, EVT_LOG = wx.lib.newevent.NewEvent()

class PackageDocApp(wx.App):

    def OnInit(self):
        logfile = os.path.join(appPath, 'log.txt')
        logging.basicConfig(format='%(name)-8s %(levelname)-8s %(message)s',
                            filename=logfile, level=logging.ERROR)

        self.SetAppName('Package Doxygen Generate Application')
        frame = PackageDocMainFrame(None, "Package Document Generation Application!")
        self.SetTopWindow(frame)

        frame.Show(True)

        EVT_APP_CALLBACK( self, self.OnAppCallBack)
        return True

    def GetLogger(self):
        return logging.getLogger('')

    def ForegroundProcess(self, function, args):
        wx.PostEvent(self, AppCallBackEvent(callback=function, args=args))

    def OnAppCallBack(self, event):
        try:
            event.callback(*event.args)
        except:
            self._logger.exception( 'OnAppCallBack<%s.%s>\n' %
                (event.callback.__module__, event.callback.__name__ ))

class PackageDocMainFrame(wx.Frame):
    def __init__(self, parent, title):
        wx.Frame.__init__(self, parent, -1, title, size=(550, 290), style=wx.MINIMIZE_BOX|wx.SYSTEM_MENU|wx.CAPTION|wx.CLOSE_BOX )

        panel = wx.Panel(self)
        sizer = wx.BoxSizer(wx.VERTICAL)

        subsizer = wx.GridBagSizer(5, 10)
        subsizer.AddGrowableCol(1)
        subsizer.Add(wx.StaticText(panel, -1, "Workspace Location : "), (0, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        self._workspacePathCtrl = wx.ComboBox(panel, -1)
        list = self.GetConfigure("WorkspacePath")
        if len(list) != 0:
            for item in list:
                self._workspacePathCtrl.Append(item)
            self._workspacePathCtrl.SetValue(list[len(list) - 1])

        subsizer.Add(self._workspacePathCtrl, (0, 1), flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND)
        self._workspacePathBt = wx.BitmapButton(panel, -1, bitmap=wx.ArtProvider_GetBitmap(wx.ART_FILE_OPEN))
        subsizer.Add(self._workspacePathBt, (0, 2), flag=wx.ALIGN_CENTER_VERTICAL)
        wx.EVT_BUTTON(self._workspacePathBt, self._workspacePathBt.GetId(), self.OnBrowsePath)

        subsizer.Add(wx.StaticText(panel, -1, "Package DEC Location : "), (1, 0), flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND)
        self._packagePathCtrl = wx.ComboBox(panel, -1)
        list = self.GetConfigure("PackagePath")
        if len(list) != 0:
            for item in list:
                self._packagePathCtrl.Append(item)
            self._packagePathCtrl.SetValue(list[len(list) - 1])
        subsizer.Add(self._packagePathCtrl, (1, 1), flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND)
        self._packagePathBt = wx.BitmapButton(panel, -1, bitmap=wx.ArtProvider_GetBitmap(wx.ART_FILE_OPEN))
        subsizer.Add(self._packagePathBt, (1, 2), flag=wx.ALIGN_CENTER_VERTICAL)
        wx.EVT_BUTTON(self._packagePathBt, self._packagePathBt.GetId(), self.OnBrowsePath)

        subsizer.Add(wx.StaticText(panel, -1, "Doxygen Tool Location : "), (2, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        self._doxygenPathCtrl = wx.TextCtrl(panel, -1)
        list = self.GetConfigure('DoxygenPath')
        if len(list) != 0:
            self._doxygenPathCtrl.SetValue(list[0])
        else:
            if wx.Platform == '__WXMSW__':
                self._doxygenPathCtrl.SetValue('C:\\Program Files\\Doxygen\\bin\\doxygen.exe')
            else:
                self._doxygenPathCtrl.SetValue('/usr/bin/doxygen')

        self._doxygenPathBt = wx.BitmapButton(panel, -1, bitmap=wx.ArtProvider_GetBitmap(wx.ART_FILE_OPEN))
        subsizer.Add(self._doxygenPathCtrl, (2, 1), flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND)
        subsizer.Add(self._doxygenPathBt, (2, 2), flag=wx.ALIGN_CENTER_VERTICAL)
        wx.EVT_BUTTON(self._doxygenPathBt, self._doxygenPathBt.GetId(), self.OnBrowsePath)

        subsizer.Add(wx.StaticText(panel, -1, "CHM Tool Location : "), (3, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        self._chmPathCtrl = wx.TextCtrl(panel, -1)
        list = self.GetConfigure('CHMPath')
        if len(list) != 0:
            self._chmPathCtrl.SetValue(list[0])
        else:
            self._chmPathCtrl.SetValue('C:\\Program Files\\HTML Help Workshop\\hhc.exe')

        self._chmPathBt = wx.BitmapButton(panel, -1, bitmap=wx.ArtProvider_GetBitmap(wx.ART_FILE_OPEN))
        subsizer.Add(self._chmPathCtrl, (3, 1), flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND)
        subsizer.Add(self._chmPathBt, (3, 2), flag=wx.ALIGN_CENTER_VERTICAL)
        wx.EVT_BUTTON(self._chmPathBt, self._chmPathBt.GetId(), self.OnBrowsePath)

        subsizer.Add(wx.StaticText(panel, -1, "Output Location : "), (4, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        self._outputPathCtrl = wx.ComboBox(panel, -1)
        list = self.GetConfigure("OutputPath")
        if len(list) != 0:
            for item in list:
                self._outputPathCtrl.Append(item)
            self._outputPathCtrl.SetValue(list[len(list) - 1])

        subsizer.Add(self._outputPathCtrl, (4, 1), flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND)
        self._outputPathBt = wx.BitmapButton(panel, -1, bitmap=wx.ArtProvider_GetBitmap(wx.ART_FILE_OPEN))
        subsizer.Add(self._outputPathBt, (4, 2), flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND)
        wx.EVT_BUTTON(self._outputPathBt, self._outputPathBt.GetId(), self.OnBrowsePath)

        subsizer.Add(wx.StaticText(panel, -1, "Architecture Specified : "), (5, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        self._archCtrl = wx.ComboBox(panel, -1, value='ALL', choices=['ALL', 'IA32/MSFT', 'IA32/GNU', 'X64/INTEL', 'X64/GNU', 'IPF/MSFT', 'IPF/GNU', 'EBC/INTEL'],
                                     style=wx.CB_READONLY)
        self._archCtrl.Bind(wx.EVT_COMBOBOX, self.OnArchtectureSelectChanged)
        subsizer.Add(self._archCtrl, (5, 1), (1, 2), flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND)
        sizer.Add(subsizer, 0, wx.EXPAND|wx.TOP|wx.BOTTOM|wx.LEFT|wx.RIGHT, 5)

        sizer6 = wx.BoxSizer(wx.HORIZONTAL)
        self._modesel = wx.RadioBox(panel, -1, 'Generated Document Mode', majorDimension=2, choices=['CHM', 'HTML'], style=wx.RA_SPECIFY_COLS)
        self._modesel.SetStringSelection('HTML')

        self._includeonlysel = wx.CheckBox(panel, -1, 'Only document public include')

        sizer6.Add(self._modesel, 0 , wx.EXPAND)
        sizer6.Add(self._includeonlysel, 0, wx.EXPAND|wx.LEFT, 5)

        sizer.Add(sizer6, 0, wx.EXPAND|wx.TOP|wx.LEFT|wx.RIGHT, 5)

        self._generateBt = wx.Button(panel, -1, "Generate Package Document!")
        self._generateBt.Bind(wx.EVT_BUTTON, self.OnGenerate)
        sizer.Add(self._generateBt, 0, wx.EXPAND|wx.TOP|wx.LEFT|wx.RIGHT, 5)

        panel.SetSizer(sizer)
        panel.Layout()
        panel.SetAutoLayout(True)
        self.CenterOnScreen()

    def SaveConfigure(self, name, value):
        if value ==None or len(value) == 0:
            return
        config = wx.ConfigBase_Get()
        oldvalues = config.Read(name, '').split(';')
        if len(oldvalues) >= 10:
            oldvalues.remove(oldvalues[0])
        if value not in oldvalues:
            oldvalues.append(value)
        else:
            oldvalues.remove(value)
            oldvalues.append(value)

        config.Write(name, ';'.join(oldvalues))

    def GetConfigure(self, name):
        config = wx.ConfigBase_Get()
        values = config.Read(name, '').split(';')
        list = []
        for item in values:
            if len(item) != 0:
                list.append(item)
        return list

    def OnBrowsePath(self, event):
        id       = event.GetId()
        editctrl = None
        startdir = ''
        isFile   = False
        if id == self._packagePathBt.GetId():
            dlgTitle = "Choose package path:"
            editctrl = self._packagePathCtrl
            isFile   = True
            if os.path.exists(self.GetWorkspace()):
                startdir = self.GetWorkspace()
        elif id == self._workspacePathBt.GetId():
            dlgTitle = "Choose workspace path:"
            editctrl = self._workspacePathCtrl
            startdir = editctrl.GetValue()
        elif id == self._doxygenPathBt.GetId():
            isFile  = True
            dlgTitle = "Choose doxygen installation path:"
            editctrl = self._doxygenPathCtrl
            startdir = editctrl.GetValue()
        elif id == self._outputPathBt.GetId():
            dlgTitle = "Choose document output path:"
            editctrl = self._outputPathCtrl
            if os.path.exists(self.GetWorkspace()):
                startdir = self.GetWorkspace()
            startdir = editctrl.GetValue()
        elif id == self._chmPathBt.GetId():
            isFile = True
            dlgTitle = "Choose installation path for Microsoft HTML workshop software"
            editctrl = self._chmPathCtrl
            startdir = editctrl.GetValue()
        else:
            return

        if not isFile:
            dlg = wx.DirDialog(self, dlgTitle, defaultPath=startdir)
        else:
            dlg = wx.FileDialog(self, dlgTitle, defaultDir=startdir)

        if dlg.ShowModal() == wx.ID_OK:
            editctrl.SetValue(dlg.GetPath())
        dlg.Destroy()

    def OnArchtectureSelectChanged(self, event):
        str = ''
        selarch = self._archCtrl.GetValue()
        if selarch == 'ALL':
            str += 'MDE_CPU_IA32 MDE_CPU_X64 MDE_CPU_EBC MDE_CPU_IPF _MSC_EXTENSIONS __GNUC__ __INTEL_COMPILER'
        elif selarch == 'IA32/MSFT':
            str += 'MDE_CPU_IA32 _MSC_EXTENSIONS'
        elif selarch == 'IA32/GNU':
            str += 'MDE_CPU_IA32 __GNUC__'
        elif selarch == 'X64/MSFT':
            str += 'MDE_CPU_X64 _MSC_EXTENSIONS'
        elif selarch == 'X64/GNU':
            str += 'MDE_CPU_X64 __GNUC__'
        elif selarch == 'IPF/MSFT':
            str += 'MDE_CPU_IPF _MSC_EXTENSIONS'
        elif selarch == 'IPF/GNU':
            str += 'MDE_CPU_IPF __GNUC__'
        elif selarch == 'EBC/INTEL':
            str += 'MDE_CPU_EBC __INTEL_COMPILER'

        str += ' ASM_PFX= OPTIONAL= '

    def OnMacroText(self, event):
        str = ''
        selarch = self._archCtrl.GetValue()
        if selarch == 'ALL':
            str += 'MDE_CPU_IA32 MDE_CPU_X64 MDE_CPU_EBC MDE_CPU_IPF _MSC_EXTENSIONS __GNUC__ __INTEL_COMPILER'
        elif selarch == 'IA32/MSFT':
            str += 'MDE_CPU_IA32 _MSC_EXTENSIONS'
        elif selarch == 'IA32/GNU':
            str += 'MDE_CPU_IA32 __GNUC__'
        elif selarch == 'X64/MSFT':
            str += 'MDE_CPU_X64 _MSC_EXTENSIONS'
        elif selarch == 'X64/GNU':
            str += 'MDE_CPU_X64 __GNUC__'
        elif selarch == 'IPF/MSFT':
            str += 'MDE_CPU_IPF _MSC_EXTENSIONS'
        elif selarch == 'IPF/GNU':
            str += 'MDE_CPU_IPF __GNUC__'
        elif selarch == 'EBC/INTEL':
            str += 'MDE_CPU_EBC __INTEL_COMPILER'

        str += ' ASM_PFX= OPTIONAL= '

    def OnGenerate(self, event):
        if not self.CheckInput(): return

        dlg = ProgressDialog(self)
        dlg.ShowModal()
        dlg.Destroy()

    def CheckInput(self):
        pPath = self.GetPackagePath()
        wPath = self.GetWorkspace()
        dPath = self.GetDoxygenToolPath()
        cPath = self.GetChmToolPath()
        oPath = self.GetOutputPath()

        if len(wPath) == 0 or not os.path.exists(wPath):
            self._Error("Please input existing workspace path!")
            return False
        else:
            self.SaveConfigure('WorkspacePath', wPath)

        if len(pPath) == 0 or not os.path.exists(pPath) or not pPath.lower().endswith('.dec'):
            self._Error("Please input existing package file location!")
            return False
        elif pPath.lower().find(wPath.lower()) == -1:
            self._Error("Package patch should starts with workspace path, such as if workspace path is c:\\edk2, package patch could be c:\\edk2\MdePkg")
            return False
        else:
            self.SaveConfigure('PackagePath', pPath)

        if len(dPath) == 0 or not os.path.exists(dPath):
            self._Error("Can not find doxygen tool from path %s! Please download it from www.stack.nl/~dimitri/doxygen/download.html" % dPath)
            return False
        else:
            self.SaveConfigure('DoxygenPath', dPath)

        if self._modesel.GetStringSelection() == 'CHM':
            if (len(cPath) == 0 or not os.path.exists(cPath)):
                self._Error("You select CHM mode to generate document, but can not find software of Microsoft HTML Help Workshop.\nPlease\
 download it from http://www.microsoft.com/downloads/details.aspx?FamilyID=00535334-c8a6-452f-9aa0-d597d16580cc&displaylang=en\n\
and install!")
                return False
            else:
                self.SaveConfigure('CHMPath', cPath)

        if len(oPath) == 0:
            self._Error("You must specific document output path")
            return False
        else:
            self.SaveConfigure('OutputPath', oPath)

            if os.path.exists(oPath):
                # add checking whether there is old doxygen config file here
                files = os.listdir(oPath)
                for file in files:
                    if os.path.isfile(os.path.join(oPath,file)):
                        basename, ext = os.path.splitext(file)
                        if ext.lower() == '.doxygen_config':
                            dlg = GMD.GenericMessageDialog(self, "Existing doxygen document in output directory will be overwritten\n, Are you sure?",
                                                           "Info", wx.ICON_WARNING|wx.YES_NO)
                            if dlg.ShowModal() == wx.ID_YES:
                                break
                            else:
                                return False
            else:
                try:
                    os.makedirs(oPath)
                except:
                    self._Error("Fail to create output directory, please select another output directory!")
                    return False

        return True

    def _Error(self, message):
        dlg = GMD.GenericMessageDialog(self, message,
                                       "Error", wx.ICON_ERROR|wx.OK)
        dlg.ShowModal()
        dlg.Destroy()

    def GetWorkspace(self):
        return os.path.normpath(self._workspacePathCtrl.GetValue())

    def GetPackagePath(self):
        return os.path.normpath(self._packagePathCtrl.GetValue())

    def GetOutputPath(self):
        return os.path.normpath(self._outputPathCtrl.GetValue())

    def GetDoxygenToolPath(self):
        return os.path.normpath(self._doxygenPathCtrl.GetValue())

    def GetChmToolPath(self):
        return os.path.normpath(self._chmPathCtrl.GetValue())

    def GetDocumentMode(self):
        return self._modesel.GetStringSelection()

    def GetArchitecture(self):
        value = self._archCtrl.GetValue()
        return value.split('/')[0]

    def GetToolTag(self):
        value = self._archCtrl.GetValue()
        if value == 'ALL':
            return 'ALL'
        return value.split('/')[1]

    def GetIsOnlyDocumentInclude(self):
        return self._includeonlysel.IsChecked()

class ProgressDialog(wx.Dialog):
    def __init__(self, parent, id=wx.ID_ANY):
        title = "Generate Document for " + parent.GetPackagePath()
        wx.Dialog.__init__(self, parent, id, title=title, style=wx.CAPTION, size=(600, 300))
        self.Freeze()
        sizer = wx.BoxSizer(wx.VERTICAL)
        self._textCtrl   = wx.StaticText(self, -1, "Start launching!")
        self._gaugeCtrl  = wx.Gauge(self, -1, 100, size=(-1, 10))
        self._resultCtrl = wx.stc.StyledTextCtrl(self, -1)
        self._closeBt     = wx.Button(self, -1, "Close")
        self._gotoOuputBt = wx.Button(self, -1, "Goto Output")

        # clear all margin
        self._resultCtrl.SetMarginWidth(0, 0)
        self._resultCtrl.SetMarginWidth(1, 0)
        self._resultCtrl.SetMarginWidth(2, 0)

        sizer.Add(self._textCtrl, 0, wx.EXPAND|wx.LEFT|wx.TOP|wx.RIGHT, 5)
        sizer.Add(self._gaugeCtrl, 0, wx.EXPAND|wx.LEFT|wx.TOP|wx.RIGHT, 5)
        sizer.Add(self._resultCtrl, 1, wx.EXPAND|wx.LEFT|wx.TOP|wx.RIGHT, 5)
        btsizer  = wx.BoxSizer(wx.HORIZONTAL)
        btsizer.Add(self._gotoOuputBt, 0, wx.ALIGN_CENTER_HORIZONTAL|wx.LEFT|wx.TOP|wx.LEFT|wx.BOTTOM, 5)
        btsizer.Add(self._closeBt, 0, wx.ALIGN_CENTER_HORIZONTAL|wx.LEFT|wx.TOP|wx.LEFT|wx.BOTTOM, 5)
        sizer.Add(btsizer, 0, wx.ALIGN_CENTER_HORIZONTAL)

        self.SetSizer(sizer)
        self.CenterOnScreen()
        self.Thaw()

        self._logger    = logging.getLogger('')
        self._loghandle = ResultHandler(self)
        logging.getLogger('edk').addHandler(self._loghandle)
        logging.getLogger('').addHandler(self._loghandle)
        logging.getLogger('app').addHandler(self._loghandle)

        wx.EVT_BUTTON(self._closeBt, self._closeBt.GetId(), self.OnButtonClose)
        wx.EVT_UPDATE_UI(self, self._closeBt.GetId(), self.OnUpdateCloseButton)
        wx.EVT_BUTTON(self._gotoOuputBt, self._gotoOuputBt.GetId(), self.OnGotoOutput)
        EVT_LOG(self, self.OnPostLog)

        self._process     = None
        self._pid         = None
        self._input       = None
        self._output      = None
        self._error       = None
        self._inputThread = None
        self._errorThread = None
        self._isBusy      = True
        self._pObj        = None

        wx.CallAfter(self.GenerateAction)

    def OnUpdateCloseButton(self, event):
        self._closeBt.Enable(not self._isBusy)
        return True

    def OnButtonClose(self, event):
        if self._isBusy:
            self._InfoDialog("Please don't close in progressing...")
            return

        if self._process != None:
            self._process.CloseOutput()

        if self._inputThread:
            self._inputThread.Terminate()
        if self._errorThread:
            self._errorThread.Terminate()

        if self._pid != None:
            wx.Process.Kill(self._pid, wx.SIGKILL, wx.KILL_CHILDREN)

        logging.getLogger('edk').removeHandler(self._loghandle)
        logging.getLogger('').removeHandler(self._loghandle)
        logging.getLogger('app').removeHandler(self._loghandle)

        if self._pObj != None:
            self._pObj.Destroy()

        self.EndModal(0)

    def OnGotoOutput(self, event):
        output = self.GetParent().GetOutputPath()
        if os.path.exists(output):
            if wx.Platform == '__WXMSW__':
                os.startfile(self.GetParent().GetOutputPath())
            else:
                import webbrowser
                webbrowser.open(self.GetParent().GetOutputPath())
        else:
            self._ErrorDialog("Output directory does not exist!")

    def _ErrorDialog(self, message):
        dlg = GMD.GenericMessageDialog(self, message,
                                       "Error", wx.ICON_ERROR|wx.OK)
        dlg.ShowModal()
        dlg.Destroy()

    def _InfoDialog(self, message):
        dlg = GMD.GenericMessageDialog(self, message,
                                       "Info", wx.ICON_INFORMATION|wx.OK)
        dlg.ShowModal()
        dlg.Destroy()

    def _LogStep(self, index, message):
        stepstr = "Step %d: %s" % (index, message)
        self._textCtrl.SetLabel(stepstr)
        self.LogMessage(os.linesep + stepstr + os.linesep)
        self._gaugeCtrl.SetValue(index * 100 / 6 )

    def OnPostLog(self, event):
        self.LogMessage(event.message)

    def GenerateAction(self):
        self._LogStep(1, "Create Package Object Model")
        wsPath = self.GetParent().GetWorkspace()
        pkPath = self.GetParent().GetPackagePath()[len(wsPath) + 1:]

        try:
            pObj = baseobject.Package(None, self.GetParent().GetWorkspace())
            pObj.Load(pkPath)
        except:
            self._ErrorDialog("Fail to create package object model! Please check log.txt under this application folder!")
            self._isBusy = False
            return
        self._pObj = pObj

        self.LogMessage(str(pObj.GetPcds()))

        self._LogStep(2, "Preprocess and Generate Doxygen Config File")
        try:
            action = doxygengen.PackageDocumentAction(self.GetParent().GetDoxygenToolPath(),
                                                      self.GetParent().GetChmToolPath(),
                                                      self.GetParent().GetOutputPath(),
                                                      pObj,
                                                      self.GetParent().GetDocumentMode(),
                                                      self.LogMessage,
                                                      self.GetParent().GetArchitecture(),
                                                      self.GetParent().GetToolTag(),
                                                      self.GetParent().GetIsOnlyDocumentInclude(),
                                                      True)
        except:
            self._ErrorDialog("Fail to preprocess! Please check log.txt under this application folder!")
            self._isBusy = False
            return

        action.RegisterCallbackDoxygenProcess(self.CreateDoxygeProcess)

        try:
            if not action.Generate():
                self._isBusy = False
                self.LogMessage("Fail to generate package document! Please check log.txt under this application folder!", 'error')
        except:
            import traceback
            message = traceback.format_exception(*sys.exc_info())
            logging.getLogger('').error(''.join(message))
            self._isBusy = False
            self._ErrorDialog("Fail to generate package document! Please check log.txt under this application folder!")

    def LogMessage(self, message, level='info'):
        self._resultCtrl.DocumentEnd()
        self._resultCtrl.SetReadOnly(False)
        self._resultCtrl.AppendText(message)
        self._resultCtrl.Home()
        self._resultCtrl.Home()
        self._resultCtrl.SetReadOnly(True)
        if level == 'error':
            wx.GetApp().GetLogger().error(message)

    def CreateDoxygeProcess(self, doxPath, configFile):
        self._LogStep(3, "Launch Doxygen Tool and Generate Package Document")

        cmd = '"%s" %s' % (doxPath, configFile)
        try:
            self._process = DoxygenProcess()
            self._process.SetParent(self)
            self._process.Redirect()
            self._pid    = wx.Execute(cmd, wx.EXEC_ASYNC, self._process)
            self._input  = self._process.GetInputStream()
            self._output = self._process.GetOutputStream()
            self._error  = self._process.GetErrorStream()
        except:
            self._ErrorDialog('Fail to launch doxygen cmd %s! Please check log.txt under this application folder!' % cmd)
            self._isBusy = False
            return False

        self._inputThread = MonitorThread(self._input, self.LogMessage)
        self._errorThread = MonitorThread(self._error, self.LogMessage)
        self._inputThread.start()
        self._errorThread.start()
        return True

    def OnTerminateDoxygenProcess(self):
        if self._inputThread:
            self._inputThread.Terminate()
            self._inputThread = None
        if self._errorThread:
            self._errorThread.Terminate()
            self._errorThread = None

        if self._error:
            while self._error.CanRead():
                text = self._error.read()
                self.LogMessage(text)

        if self._input:
            while self._input.CanRead():
                text = self._input.read()
                self.LogMessage(text)
        self._process.Detach()

        self._process.CloseOutput()
        self._process = None
        self._pid     = None

        self.DocumentFixup()

        if self.GetParent().GetDocumentMode().lower() == 'chm':
            hhcfile = os.path.join(self.GetParent().GetOutputPath(), 'html', 'index.hhc')
            hhpfile = os.path.join(self.GetParent().GetOutputPath(), 'html', 'index.hhp')
            self.FixDecDoxygenFileLink(hhcfile, None)
            if not self.CreateCHMProcess(self.GetParent().GetChmToolPath(), hhpfile):
                self._ErrorDialog("Fail to Create %s process for %s" % (self.GetParent().GetChmToolPath(), hhpfile))
                self._isBusy = False
        else:
            self._LogStep(6, "Finished Document Generation!")
            self._isBusy = False
            indexpath = os.path.realpath(os.path.join(self.GetParent().GetOutputPath(), 'html', 'index.html'))
            if wx.Platform == '__WXMSW__':
                os.startfile(indexpath)
            else:
                import webbrowser
                webbrowser.open(indexpath)

            self._InfoDialog('Success create HTML doxgen document %s' % indexpath)

    def CreateCHMProcess(self, chmPath, hhpfile):
        self.LogMessage("    >>>>>> Start Microsoft HTML workshop process...Zzz...\n")
        cmd = '"%s" %s' % (chmPath, hhpfile)
        try:
            self._process = CHMProcess()
            self._process.SetParent(self)
            self._process.Redirect()
            self._pid    = wx.Execute(cmd, wx.EXEC_ASYNC, self._process)
            self._input  = self._process.GetInputStream()
            self._output = self._process.GetOutputStream()
            self._error  = self._process.GetErrorStream()
        except:
            self.LogMessage('\nFail to launch hhp cmd %s!\n' % cmd)
            self._isBusy = False
            return False
        self._inputThread = MonitorThread(self._input, self.LogMessage)
        self._errorThread = MonitorThread(self._error, self.LogMessage)
        self._inputThread.start()
        self._errorThread.start()
        return True

    def OnTerminateCHMProcess(self):
        if self._inputThread:
            self._inputThread.Terminate()
            self._inputThread = None
        if self._errorThread:
            self._errorThread.Terminate()
            self._errorThread = None

        if self._error:
            while self._error.CanRead():
                text = self._error.read()
                self.LogMessage(text)
        if self._input:
            while self._input.CanRead():
                text = self._input.read()
                self.LogMessage(text)
        self._process.Detach()

        self._process.CloseOutput()
        self._process = None
        self._pid     = None
        self._isBusy  = False
        indexpath = os.path.realpath(os.path.join(self.GetParent().GetOutputPath(), 'html', 'index.chm'))
        if os.path.exists(indexpath):
            if wx.Platform == '__WXMSW__':
                os.startfile(indexpath)
            else:
                import webbrowser
                webbrowser.open(indexpath)

        self._LogStep(6, "Finished Document Generation!")
        self.LogMessage('\nSuccess create CHM doxgen document %s\n' % indexpath)
        self._InfoDialog('Success create CHM doxgen document %s' % indexpath)

    def DocumentFixup(self):
        # find BASE_LIBRARY_JUMP_BUFFER structure reference page
        self._LogStep(4, "Fixup Package Document!")
        self.LogMessage('\n    >>> Start fixup document \n')

        for root, dirs, files in os.walk(os.path.join(self.GetParent().GetOutputPath(), 'html')):
            for dir in dirs:
                if dir.lower() in ['.svn', '_svn', 'cvs']:
                    dirs.remove(dir)
            for file in files:
                wx.YieldIfNeeded()
                if not file.lower().endswith('.html'): continue
                fullpath = os.path.join(self.GetParent().GetOutputPath(), root, file)
                try:
                    f = open(fullpath, 'r')
                    text = f.read()
                    f.close()
                except:
                    self.LogMessage('\nFail to open file %s\n' % fullpath)
                    continue
                if text.find('BASE_LIBRARY_JUMP_BUFFER Struct Reference') != -1 and self.GetParent().GetArchitecture() == 'ALL':
                    self.FixPageBASE_LIBRARY_JUMP_BUFFER(fullpath, text)
                if text.find('MdePkg/Include/Library/BaseLib.h File Reference') != -1  and self.GetParent().GetArchitecture() == 'ALL':
                    self.FixPageBaseLib(fullpath, text)
                if text.find('IA32_IDT_GATE_DESCRIPTOR Union Reference') != -1  and self.GetParent().GetArchitecture() == 'ALL':
                    self.FixPageIA32_IDT_GATE_DESCRIPTOR(fullpath, text)
                if text.find('MdePkg/Include/Library/UefiDriverEntryPoint.h File Reference') != -1:
                    self.FixPageUefiDriverEntryPoint(fullpath, text)
                if text.find('MdePkg/Include/Library/UefiApplicationEntryPoint.h File Reference') != -1:
                    self.FixPageUefiApplicationEntryPoint(fullpath, text)
                if text.lower().find('.s.dox') != -1 or \
                   text.lower().find('.asm.dox') != -1 or \
                   text.lower().find('.uni.dox') != -1:
                   self.FixDoxFileLink(fullpath, text)

        self.RemoveFileList()
        self.LogMessage('    >>> Finish all document fixing up! \n')

    def RemoveFileList(self):
        path_html = os.path.join(self.GetParent().GetOutputPath(), "html", "tree.html")
        path_chm  = os.path.join(self.GetParent().GetOutputPath(), "html", "index.hhc")
        if os.path.exists(path_html):
            self.LogMessage('    >>>Remove FileList item from generated HTML document.\n');
            lines = []
            f = open (path_html, "r")
            lines = f.readlines()
            f.close()
            bfound = False
            for index in range(len(lines)):
                if lines[index].find('<a class="el" href="files.html" target="basefrm">File List</a>') != -1:
                    lines[index] = "<!-- %s" % lines[index]
                    bfound = True
                    continue
                if bfound:
                    if lines[index].find('</div>') != -1:
                        lines[index] = "%s -->" % lines[index]
                        break
            if bfound:
                f = open(path_html, "w")
                f.write("".join(lines))
                f.close()
            else:
                self.LogMessage ('    !!!Can not found FileList item in HTML document!\n')

        if os.path.exists(path_chm):
            self.LogMessage("    >>>Warning: Can not remove FileList for CHM files!\n");
            """
            self.LogMessage('    >>>Remove FileList item from generated CHM document!\n');
            lines = []
            f = open (path_chm, "r")
            lines = f.readlines()
            f.close()
            bfound = False
            for index in xrange(len(lines)):
                if not bfound:
                    if lines[index].find('<param name="Local" value="files.html">') != -1:
                        lines[index] = '<!-- %s' % lines[index]
                        bfound = True
                        continue
                if bfound:
                    if lines[index].find('</UL>') != -1:
                        lines[index] = '%s -->\n' % lines[index].rstrip()
                        break
            if bfound:
                f = open(path_chm, "w")
                f.write("".join(lines))
                f.close()
                import time
                time.sleep(2)
            else:
                self.LogMessage('   !!!Can not found the FileList item in CHM document!')
            """
    def FixPageBaseLib(self, path, text):
        self.LogMessage('    >>> Fixup BaseLib file page at file %s \n' % path)
        lines = text.split('\n')
        lastBaseJumpIndex = -1
        lastIdtGateDescriptor = -1
        for index in range(len(lines) - 1, -1, -1):
            line = lines[index]
            if line.strip() == '<td class="memname">#define BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT&nbsp;&nbsp;&nbsp;4          </td>':
                lines[index] = '<td class="memname">#define BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT&nbsp;&nbsp;&nbsp;4&nbsp;[IA32]    </td>'
            if line.strip() == '<td class="memname">#define BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT&nbsp;&nbsp;&nbsp;0x10          </td>':
                lines[index] = '<td class="memname">#define BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT&nbsp;&nbsp;&nbsp;0x10&nbsp;[IPF]   </td>'
            if line.strip() == '<td class="memname">#define BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT&nbsp;&nbsp;&nbsp;8          </td>':
                lines[index] = '<td class="memname">#define BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT&nbsp;&nbsp;&nbsp;9&nbsp;[EBC, x64]   </td>'
            if line.find('BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;4') != -1:
                lines[index] = lines[index].replace('BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;4',
                                     'BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;4&nbsp;[IA32]')
            if line.find('BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;0x10') != -1:
                lines[index] = lines[index].replace('BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;0x10',
                                     'BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;0x10&nbsp;[IPF]')
            if line.find('BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;8') != -1:
                lines[index] = lines[index].replace('BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;8',
                                     'BASE_LIBRARY_JUMP_BUFFER_ALIGNMENT</a>&nbsp;&nbsp;&nbsp;8&nbsp;[x64, EBC]')
            if line.find('>BASE_LIBRARY_JUMP_BUFFER</a>') != -1:
                if lastBaseJumpIndex != -1:
                    del lines[lastBaseJumpIndex]
                lastBaseJumpIndex = index
            if line.find('>IA32_IDT_GATE_DESCRIPTOR</a></td>') != -1:
                if lastIdtGateDescriptor != -1:
                    del lines[lastIdtGateDescriptor]
                lastIdtGateDescriptor = index
        try:
            f = open(path, 'w')
            f.write('\n'.join(lines))
            f.close()
        except:
            self._isBusy = False
            self.LogMessage("     <<< Fail to fixup file %s\n" % path)
        self.LogMessage("    <<< Finish to fixup file %s\n" % path)

    def FixPageIA32_IDT_GATE_DESCRIPTOR(self, path, text):
        self.LogMessage('    >>> Fixup structure reference IA32_IDT_GATE_DESCRIPTOR at file %s \n' % path)
        lines = text.split('\n')
        for index in range(len(lines) - 1, -1, -1):
            line = lines[index].strip()
            if line.find('struct {</td>') != -1 and lines[index - 2].find('>Uint64</a></td>') != -1:
                lines.insert(index, '<tr><td colspan="2"><br><h2>Data Fields For X64</h2></td></tr>')
            if line.find('struct {</td>') != -1 and lines[index - 1].find('Data Fields') != -1:
                lines.insert(index, '<tr><td colspan="2"><br><h2>Data Fields For IA32</h2></td></tr>')
        try:
            f = open(path, 'w')
            f.write('\n'.join(lines))
            f.close()
        except:
            self._isBusy = False
            self.LogMessage("     <<< Fail to fixup file %s\n" % path)
        self.LogMessage("    <<< Finish to fixup file %s\n" % path)

    def FixPageBASE_LIBRARY_JUMP_BUFFER(self, path, text):
        self.LogMessage('    >>> Fixup structure reference BASE_LIBRARY_JUMP_BUFFER at file %s \n' % path)
        lines = text.split('\n')
        bInDetail = True
        bNeedRemove = False
        for index in range(len(lines) - 1, -1, -1):
            line = lines[index]
            if line.find('Detailed Description') != -1:
                bInDetail = False
            if line.startswith('EBC context buffer used by') and lines[index - 1].startswith('x64 context buffer'):
                lines[index] = "IA32/IPF/X64/" + line
                bNeedRemove  = True
            if line.startswith("x64 context buffer") or line.startswith('IPF context buffer used by') or \
               line.startswith('IA32 context buffer used by'):
                if bNeedRemove:
                    lines.remove(line)
            if line.find('>R0</a>') != -1 and not bInDetail:
                if lines[index - 1] != '<tr><td colspan="2"><br><h2>Data Fields For EBC</h2></td></tr>':
                    lines.insert(index, '<tr><td colspan="2"><br><h2>Data Fields For EBC</h2></td></tr>')
            if line.find('>Rbx</a>') != -1 and not bInDetail:
                if lines[index - 1] != '<tr><td colspan="2"><br><h2>Data Fields For X64</h2></td></tr>':
                    lines.insert(index, '<tr><td colspan="2"><br><h2>Data Fields For X64</h2></td></tr>')
            if line.find('>F2</a>') != -1 and not bInDetail:
                if lines[index - 1] != '<tr><td colspan="2"><br><h2>Data Fields For IPF</h2></td></tr>':
                    lines.insert(index, '<tr><td colspan="2"><br><h2>Data Fields For IPF</h2></td></tr>')
            if line.find('>Ebx</a>') != -1 and not bInDetail:
                if lines[index - 1] != '<tr><td colspan="2"><br><h2>Data Fields For IA32</h2></td></tr>':
                    lines.insert(index, '<tr><td colspan="2"><br><h2>Data Fields For IA32</h2></td></tr>')
        try:
            f = open(path, 'w')
            f.write('\n'.join(lines))
            f.close()
        except:
            self._isBusy = False
            self.LogMessage("     <<< Fail to fixup file %s" % path)
        self.LogMessage("    <<< Finish to fixup file %s\n" % path)

    def FixPageUefiDriverEntryPoint(self, path, text):
        self.LogMessage('    >>> Fixup file reference MdePkg/Include/Library/UefiDriverEntryPoint.h at file %s \n' % path)
        lines = text.split('\n')
        bInModuleEntry = False
        bInEfiMain     = False
        ModuleEntryDlCount  = 0
        ModuleEntryDelStart = 0
        ModuleEntryDelEnd   = 0
        EfiMainDlCount      = 0
        EfiMainDelStart     = 0
        EfiMainDelEnd       = 0

        for index in range(len(lines)):
            line = lines[index].strip()
            if line.find('EFI_STATUS</a> EFIAPI _ModuleEntryPoint           </td>') != -1:
                bInModuleEntry = True
            if line.find('EFI_STATUS</a> EFIAPI EfiMain           </td>') != -1:
                bInEfiMain = True
            if line.startswith('<p>References <a'):
                if bInModuleEntry:
                    ModuleEntryDelEnd = index - 1
                    bInModuleEntry = False
                elif bInEfiMain:
                    EfiMainDelEnd = index - 1
                    bInEfiMain = False
            if bInModuleEntry:
                if line.startswith('</dl>'):
                    ModuleEntryDlCount = ModuleEntryDlCount + 1
                if ModuleEntryDlCount == 1:
                    ModuleEntryDelStart = index + 1
            if bInEfiMain:
                if line.startswith('</dl>'):
                    EfiMainDlCount = EfiMainDlCount + 1
                if EfiMainDlCount == 1:
                    EfiMainDelStart = index + 1

        if EfiMainDelEnd > EfiMainDelStart:
            for index in range(EfiMainDelEnd, EfiMainDelStart, -1):
                del lines[index]
        if ModuleEntryDelEnd > ModuleEntryDelStart:
            for index in range(ModuleEntryDelEnd, ModuleEntryDelStart, -1):
                del lines[index]

        try:
            f = open(path, 'w')
            f.write('\n'.join(lines))
            f.close()
        except:
            self._isBusy = False
            self.LogMessage("     <<< Fail to fixup file %s" % path)
        self.LogMessage("    <<< Finish to fixup file %s\n" % path)

    def FixPageUefiApplicationEntryPoint(self, path, text):
        self.LogMessage('    >>> Fixup file reference MdePkg/Include/Library/UefiApplicationEntryPoint.h at file %s \n' % path)
        lines = text.split('\n')
        bInModuleEntry = False
        bInEfiMain     = False
        ModuleEntryDlCount  = 0
        ModuleEntryDelStart = 0
        ModuleEntryDelEnd   = 0
        EfiMainDlCount      = 0
        EfiMainDelStart     = 0
        EfiMainDelEnd       = 0

        for index in range(len(lines)):
            line = lines[index].strip()
            if line.find('EFI_STATUS</a> EFIAPI _ModuleEntryPoint           </td>') != -1:
                bInModuleEntry = True
            if line.find('EFI_STATUS</a> EFIAPI EfiMain           </td>') != -1:
                bInEfiMain = True
            if line.startswith('<p>References <a'):
                if bInModuleEntry:
                    ModuleEntryDelEnd = index - 1
                    bInModuleEntry = False
                elif bInEfiMain:
                    EfiMainDelEnd = index - 1
                    bInEfiMain = False
            if bInModuleEntry:
                if line.startswith('</dl>'):
                    ModuleEntryDlCount = ModuleEntryDlCount + 1
                if ModuleEntryDlCount == 1:
                    ModuleEntryDelStart = index + 1
            if bInEfiMain:
                if line.startswith('</dl>'):
                    EfiMainDlCount = EfiMainDlCount + 1
                if EfiMainDlCount == 1:
                    EfiMainDelStart = index + 1

        if EfiMainDelEnd > EfiMainDelStart:
            for index in range(EfiMainDelEnd, EfiMainDelStart, -1):
                del lines[index]
        if ModuleEntryDelEnd > ModuleEntryDelStart:
            for index in range(ModuleEntryDelEnd, ModuleEntryDelStart, -1):
                del lines[index]

        try:
            f = open(path, 'w')
            f.write('\n'.join(lines))
            f.close()
        except:
            self._isBusy = False
            self.LogMessage("     <<< Fail to fixup file %s" % path)
        self.LogMessage("    <<< Finish to fixup file %s\n" % path)


    def FixDoxFileLink(self, path, text):
        self.LogMessage('    >>> Fixup .dox postfix for file %s \n' % path)
        try:
            fd = open(path, 'r')
            text = fd.read()
            fd.close()
        except Exception as e:
            self.LogMessage ("   <<<Fail to open file %s" % path)
            return
        text = text.replace ('.s.dox', '.s')
        text = text.replace ('.S.dox', '.S')
        text = text.replace ('.asm.dox', '.asm')
        text = text.replace ('.Asm.dox', '.Asm')
        text = text.replace ('.uni.dox', '.uni')
        text = text.replace ('.Uni.dox', '.Uni')
        try:
            fd = open(path, 'w')
            fd.write(text)
            fd.close()
        except Exception as e:
            self.LogMessage ("    <<<Fail to fixup file %s" % path)
            return
        self.LogMessage('    >>> Finish to fixup .dox postfix for file %s \n' % path)

    def FixDecDoxygenFileLink(self, path, text):
        self.LogMessage('    >>> Fixup .decdoxygen postfix for file %s \n' % path)
        try:
            fd = open(path, 'r')
            lines = fd.readlines()
            fd.close()
        except Exception as e:
            self.LogMessage ("   <<<Fail to open file %s" % path)
            return
        for line in lines:
            if line.find('.decdoxygen') != -1:
                lines.remove(line)
                break
        try:
            fd = open(path, 'w')
            fd.write("".join(lines))
            fd.close()
        except Exception as e:
            self.LogMessage ("    <<<Fail to fixup file %s" % path)
            return
        self.LogMessage('    >>> Finish to fixup .decdoxygen postfix for file %s \n' % path)

import threading
class MonitorThread(threading.Thread):
    def __init__(self, pipe, callback):
        threading.Thread.__init__(self)
        self._pipe = pipe
        self._callback = callback
        self._isCancel = False

    def run(self):
        while (not self._isCancel):
            self._pipe.Peek()
            if self._pipe.LastRead() == 0:
                break
            text = self._pipe.read()
            if len(text.strip()) != 0:
                wx.GetApp().ForegroundProcess(self._callback, (text,))

    def Terminate(self):
        self._pipe.flush()
        self._isCancel = True

class DoxygenProcess(wx.Process):
    def OnTerminate(self, id, status):
        self._parent.OnTerminateDoxygenProcess()

    def SetParent(self, parent):
        self._parent = parent

class CHMProcess(wx.Process):
    def OnTerminate(self, id, status):
        self._parent.OnTerminateCHMProcess()

    def SetParent(self, parent):
        self._parent = parent

class ResultHandler:
    def __init__(self, parent):
        self._parent = parent
        self.level   = 0

    def emit(self, record):
        self._parent.LogMessage(record)

    def handle(self, record):
        wx.PostEvent(self._parent, LogEvent(message=record.getMessage()))

    def acquire(self):
        pass

    def release(self):
        pass

if __name__ == '__main__':
    app = PackageDocApp(redirect=False)
    app.MainLoop()
