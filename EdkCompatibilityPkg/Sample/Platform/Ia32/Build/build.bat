@rem #/*++
@rem #  
@rem #  Copyright (c) 2007, Intel Corporation                                                         
@rem #  All rights reserved. This program and the accompanying materials                          
@rem #  are licensed and made available under the terms and conditions of the BSD License         
@rem #  which accompanies this distribution.  The full text of the license may be found at        
@rem #  http://opensource.org/licenses/bsd-license.php                                            
@rem #                                                                                            
@rem #  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
@rem #  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
@rem #  
@rem #  Module Name:
@rem #  
@rem #    build.bat
@rem #  
@rem #  Abstract:
@rem #    
@rem #    This script provides single module build, clean and find function
@rem #    based on module name.
@rem #    
@rem #    The auto-generated module.list file records all module names 
@rem #    described in the current platform.dsc.
@rem #    
@rem #--*/ 

@echo off

setlocal
@rem initilize local variable
set FUNCTION=build
set MODULE=
set UPDATE=
set VERBOSE=

:parse
if /I "%1"=="/c" (
  set FUNCTION=clean
  shift
  goto parse
)
if /I "%1"=="clean" (
  set FUNCTION=clean
  shift
  goto parse
)
if /I "%1"=="/r" (
  set FUNCTION=rebuild
  shift
  goto parse
)
if /I "%1"=="/h" (
  set FUNCTION=usage
  shift
  goto parse
)
if /I "%1"=="/?" (
  set FUNCTION=usage
  shift
  goto parse
)
if /I "%1"=="/f" (
  set FUNCTION=find
  shift
  goto parse
)
if /I "%1"=="/a" (
  set UPDATE=TRUE
  shift
  goto parse
)
if /I "%1"=="/v" (
  set VERBOSE=TRUE
  shift
  goto parse
)
if not "%1"=="" (
  set MODULE=%1
  shift
  goto parse
) else (
  @rem no other paramters
  if "%FUNCTION%"=="rebuild" goto build
  goto %FUNCTION%
)

:build
set StartTime=%time%
@rem if no input, build all
if "%MODULE%"=="" (
  if "%FUNCTION%"=="rebuild" (
    nmake -nologo -f Makefile clean
  )
  nmake -nologo -f Makefile all
  goto endtime
)
@rem then try to build special targets: all, fast, tools.
if /I "%MODULE%"=="all" (
  @rem build all modules and tools.
  if "%FUNCTION%"=="rebuild" (
    nmake -nologo -f Makefile cleanall
  )
  nmake -nologo -f Makefile all
  goto endtime
)
if /I "%MODULE%"=="fast" (
  @rem build fast target to skip FV=NULL modules.
  if "%FUNCTION%"=="rebuild" (
    nmake -nologo -f Makefile clean
  )
  nmake -nologo -f Makefile fast
  goto endtime
)
if /I "%MODULE%"=="tools" (
  @rem build all tools.
  if "%FUNCTION%"=="rebuild" (
    nmake -nologo -f Makefile cleantools
  )
  nmake -nologo -f Makefile build_tools
  echo.
  echo All tools are built.
  goto endtime
)
@rem build single module, update build makefiles
if "%FUNCTION%"=="rebuild" (
  nmake -nologo -f module.mak %MODULE%Clean 2>NUL
)
nmake -nologo -f Makefile flashmap > NUL 2>&1
nmake -nologo -f Makefile makefiles
if errorlevel 1 goto builderror
@rem check whether input module name is described in current dsc file. 
findstr /I /C:" %MODULE% " module.list > NUL
if errorlevel 1 (
  echo.
  echo.
  findstr /I /C:"%MODULE%" module.list > NUL
  if errorlevel 1 goto finderror
  for /F %%A in ('findstr /I /C:"%MODULE%" module.list') do echo  %%A
  echo.
  echo Warning!!! Your specified module name can't be found. 
  echo One of the above modules may be what you want to build.
  goto end
)
@rem build this module
nmake -nologo -f module.mak %MODULE%Build 
echo.
if "%FUNCTION%"=="rebuild" (
  echo Module %MODULE% is rebuilt.
) else (
  echo Module %MODULE% is built.
)
:endtime
@rem output build time.
set EndTime=%time%
echo.
echo Start time %StartTime%
echo End   time %EndTime%
goto end

:clean
@rem if no input, default clean all build directories.
if "%MODULE%"=="" (
  nmake -nologo -f Makefile clean
  goto end
)
@rem first try to clean special tasks: all, modules and tools
if /I "%MODULE%"=="all" (
  nmake -nologo -f Makefile cleanall
  goto end
)
if /I "%MODULE%"=="modules" (
  nmake -nologo -f Makefile cleanbuilds
  goto end
)
if /I "%MODULE%"=="tools" (
  nmake -nologo -f Makefile cleantools
  goto end
)
@rem clean single module  
@rem check whether input module name is in module.list file. 
if not exist module.list (
  echo Module list info doesn't exist.
  echo Processing dsc file to generate module list info.
  nmake -nologo -f Makefile flashmap > NUL 2>&1
  nmake -nologo -f Makefile makefiles > error.log 2>&1
  if errorlevel 1 goto builderror
)
findstr /I /C:" %MODULE% " module.list > NUL
if errorlevel 1 (
  echo.
  findstr /I /C:"%MODULE%" module.list > NUL
  if errorlevel 1 goto finderror
  for /F %%A in ('findstr /I /C:"%MODULE%" module.list') do echo  %%A
  echo.
  echo Warning!!! Your specified module name can't be found. 
  echo One of the above modules may be what you want to clean. 
  goto end
)
@rem clean this module 
nmake -nologo -f module.mak %MODULE%Clean 2>NUL
echo.
echo Module %MODULE% is cleaned.
goto end

:find
@rem find match module name by subname.
if "%UPDATE%"=="TRUE" (
  echo Processing dsc file to update module list info.
  nmake -nologo -f Makefile flashmap > NUL 2>&1
  nmake -nologo -f Makefile makefiles > error.log 2>&1
  if errorlevel 1 goto builderror
)
if not exist module.list (
  echo Module list info doesn't exist.
  echo Processing dsc file to generate module list info.
  nmake -nologo -f Makefile flashmap > NUL 2>&1
  nmake -nologo -f Makefile makefiles > error.log 2>&1
  if errorlevel 1 goto builderror
)
if "%MODULE%"=="" (
  @rem display all
  echo.
  if "%VERBOSE%"=="TRUE" ( 
    type module.list
  ) else (
    @rem only output module name without module.inf file name
    for /F %%A in (module.list) do echo  %%A
  )
) else (
  @rem display match module name
  echo.
  if "%VERBOSE%"=="TRUE" (
    findstr /I /C:"%MODULE%" module.list
    if errorlevel 1 goto notfind
  ) else (
    findstr /I /C:"%MODULE%" module.list > NUL
    if errorlevel 1 goto notfind
    for /F %%A in ('findstr /I /C:"%MODULE%" module.list') do echo  %%A
  )
)
goto end

:usage
echo build or clean single module based on module name after tools are built.
echo.
echo build [/r] [/c] [/f] [/h] [modulename]
echo.
echo build [/r] [modulename]
echo       build single module, such as build DxeMain.
echo       If /r is specified, the target will be rebuit after cleaned first.
echo       if no input modulename or modulename is all, then build all
echo       Specail build targets: all, fast, tools.
echo       These special targets may not exist in your tip main makefile.
echo       build       - build all tools and modules
echo       build all   - build all tools and modules
echo       build fast  - build all without FV=NULL modules
echo       build tools - build all tools.
echo.
echo build /c    [modulename]
echo build clean [modulename]
echo       remove the temp generated files for single module  
echo       if no input modulename, then clean all
echo       Specail clean targets: all, modules, tools.
echo       These special targets may not exist in your tip main makefile.
echo       build /c         - clean up all build directories
echo       build /c all     - clean up all build directories and binary dirs.
echo       build /c modules - clean up all build directories except for tools.
echo       build /c tools   - clean up only tools directory.
echo.
echo build /f [/a] [/v] [subname]
echo       find all matched modulename with the sub string of module name.
echo       option /a re-processes dsc files to update module name list.
echo       option /v outputs module name and module.inf file name both.
echo.
echo build /h
echo build /?
echo       display help information. 
echo.
goto end

:notfind
echo Warning!!! Your specified module name can't be found.
echo Try to use /f /a options to update module name list.
goto end

:finderror
echo Warning!!! Your specified module name can't be found.
echo Try to use /f to find modules that you want to build.
echo Or use /h to get the helpinfo of this script.
goto end

:builderror
if exist error.log type error.log
echo.
if not exist Tools\ProcessDsc.exe (
  echo.
  echo Error!!! Build tools may not be ready. Try to build tools first.
  echo.
)
del module.* > NUL 2>&1
goto end

:end
if exist error.log del error.log
echo on
