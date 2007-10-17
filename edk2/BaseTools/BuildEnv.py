## @file BuildEnv.py
# Initialize Environment for building
#
#  Copyright (c) 2007, Intel Corporation
#
#  All rights reserved. This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import os
import os.path
import pickle
import shutil
import sys

from optparse import OptionParser

# Version and Copyright
VersionNumber = "0.01"
__version__ = "%prog Version " + VersionNumber
__copyright__ = "Copyright (c) 2007, Intel Corporation  All rights reserved."

class SetupBuildEnvironmentApp:

  def __init__(self):
    (self.Opt, self.Args) = self.ProcessCommandLine()
    self.SetupDefaults()
    self.DetermineEnvironment()
    self.CopyTemplates()
    self.WriteEnvironmentConfigurationScript()

  def SetupDefaults(self):
    self.itemsToConfigure = (
      'compiler',
      'compiler-prefix',
      'templates and Conf directory',
      )

    self.defaults = {
      'compiler': {
        'options': ('gcc', 'icc'),
        'default': 'gcc',
        },
      'compiler-prefix': {
        'options': ('/usr/bin', '/usr/bin/x86_64-pc-mingw32-'),
        'freeform': True,
        },
      'templates and Conf directory': {
        'options': (
          'copy once (no-overwrite)',
          'copy with overwrite',
          'symlink to templates',
          'do nothing',
          ),
        'default': 'copy once (no-overwrite)',
        },
      }

  def ProcessCommandLine(self):
    Parser = OptionParser(description=__copyright__,version=__version__,prog="Tools/BuildEnv")
    Parser.add_option("-q", "--quiet", action="store_true", type=None, help="Disable all messages except FATAL ERRORS.")
    Parser.add_option("-v", "--verbose", action="store_true", type=None, help="Turn on verbose output with informational messages printed, "\
                                                                               "including library instances selected, final dependency expression, "\
                                                                               "and warning messages, etc.")
    Parser.add_option("-d", "--debug", action="store", type="int", help="Enable debug messages at specified level.")

    if os.environ.has_key('WORKSPACE'):
      default = os.environ['WORKSPACE']
    else:
      default = os.getcwd()
    Parser.add_option("--workspace", action="store", type="string", help="Base director of tree to configure", default=default)

    (Opt, Args)=Parser.parse_args()
    Parser.print_version()

    return (Opt, Args)

  def DetermineEnvironment(self):
    confFilename = os.path.join(os.path.expanduser('~'), '.edk-build-env.pickle')
    try:
      confFile = open(confFilename, 'r')
      conf = pickle.load(confFile)
      confFile.close()
    except Exception:
      conf = {}
    self.conf = conf

    for item in self.itemsToConfigure:
      if not conf.has_key(item):
        self.AskForValueOfOption(item)

    while True:
      self.SummarizeCurrentState()

      if not self.Opt.quiet:
        response = raw_input('Would you like to change anything? (default=no): ')
        response = response.strip()
      else:
        response = ''

      if response.lower() in ('', 'n', 'no'):
        break

      for item in self.itemsToConfigure:
        self.AskForValueOfOption(item)

    confFile = open(confFilename, 'w')
    pickle.dump(conf, confFile)
    confFile.close()

  def AskCompiler(self):
    self.AskForValueOfOption('compiler')

  def AskCompilerPrefix(self):
    self.AskForValueOfOption('compiler-prefix')

  def AskForValueOfOption(self, option):
    options = self.defaults[option]['options']
    if self.defaults[option].has_key('default'):
      default = self.defaults[option]['default']
    else:
      default = None
    if self.defaults[option].has_key('freeform'):
      freeform = self.defaults[option]['freeform']
    else:
      freeform = False
    self.AskForValue(option, options, default, freeform)

  def AskForValue(self, index, options, default=None, freeform=False):
    conf = self.conf
    if conf.has_key(index):
      default = conf[index]
    options = list(options) # in case options came in as a tuple
    assert((default == '') or (default is None) or ('' not in options))
    if (default is not None) and (default not in options):
      options.append(default)
    if (freeform and ('' not in options)):
      options.append('')
    options.sort()
    while True:
      print
      if len(options) > 0:
        print 'Options for', index
        for i in range(len(options)):
          print '  %d.' % (i + 1),
          if options[i] != '':
            print options[i],
          else:
            print '(empty string)',
          if options[i] == default:
            print '(default)'
          else:
            print

      if len(options) > 0:
        prompt = 'Select number or type value: '
      else:
        prompt = 'Type value for %s: ' % index
      response = raw_input(prompt)
      response = response.strip()

      if response.isdigit():
        response = int(response)
        if response > len(options):
          print 'ERROR: Invalid number selection!'
          continue
        response = options[response - 1]
      elif (response == '') and (default is not None):
        response = default

      if (not freeform) and (response not in options):
        print 'ERROR: Invalid selection! (must be from list)'
        continue

      break

    conf[index] = response
    print 'Using', conf[index], 'for', index

  def SummarizeCurrentState(self):
    print
    print 'Current configuration:'
    conf = self.conf
    for item in self.itemsToConfigure:
      value = conf[item]
      if value == '': value = '(empty string)'
      print ' ', item, '->', value

  def CopyTemplates(self):
    todo = self.conf['templates and Conf directory']
    workspace = os.path.realpath(self.Opt.workspace)
    templatesDir = \
      os.path.join(workspace, 'Tools', 'BaseTools', 'ConfTemplates', sys.platform.title())
    confDir = \
      os.path.join(workspace, 'Conf')
    print
    print 'Templates & Conf directory'
    print '  Templates dir:', self.RelativeToWorkspace(templatesDir)
    for filename in os.listdir(templatesDir):
      if filename.startswith('.'): continue

      srcFilename = os.path.join(templatesDir, filename)
      destFilename = os.path.join(confDir, filename)
      print ' ', self.RelativeToWorkspace(destFilename),

      if todo == 'copy once (no-overwrite)':
        if os.path.exists(destFilename):
          print '[skipped, already exists]'
        else:
          shutil.copy(srcFilename, destFilename)
          print '[copied]'
      elif todo == 'copy with overwrite':
        overwrite = ''
        if os.path.exists(destFilename):
          os.remove(destFilename)
          overwrite = ', overwritten'
        shutil.copy(srcFilename, destFilename)
        print '[copied' + overwrite + ']'
      elif todo == 'symlink to templates':
        if os.path.exists(destFilename):
          if not os.path.islink(destFilename):
            raise Exception, '%s is not a symlink! (remove file if you want to start using symlinks)' % \
                             (self.RelativeToWorkspace(destFilename))
          os.remove(destFilename)
        os.symlink(srcFilename, destFilename)
        print '[symlinked]'
      elif todo == 'do nothing':
        print '[skipped by user request]'
      else:
        raise Exception, 'Unknown action for templates&conf: %s' % todo

  def WriteEnvironmentConfigurationScript(self):
    workspace = os.path.realpath(self.Opt.workspace)
    scriptFilename = os.path.join(workspace, 'Conf', 'BuildEnv.sh')
    print
    print 'Storing environment configuration into',
    print   self.RelativeToWorkspace(scriptFilename)
    script = open(scriptFilename, 'w')

    print >> script, 'export WORKSPACE="%s"' % workspace
    print >> script, 'export TOOLCHAIN="%s"' % self.conf['compiler']
    print >> script, 'export EDK_CC_PATH_PREFIX="%s"' % self.conf['compiler-prefix']

    #
    # Change PATH variable
    #
    newPath = os.environ['PATH'].split(os.path.pathsep)
    binDir = \
      os.path.join(workspace, 'Tools', 'BaseTools', 'Bin', sys.platform.title())
    if binDir not in newPath:
      newPath.append(binDir)
    newPath = os.path.pathsep.join(newPath)
    print >> script, 'export PATH=%s' % newPath

    script.close()

  def RelativeToWorkspace(self, path):
    workspace = os.path.realpath(self.Opt.workspace)
    for prefix in (workspace + os.path.sep, workspace):
      if path.startswith(prefix):
        return path[len(prefix):]
    

if __name__ == '__main__':
  SetupBuildEnvironmentApp()

