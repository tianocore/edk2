# Microsoft Developer Studio Project File - Name="pcctslib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=pcctslib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "pcctslib50.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "pcctslib50.mak" CFG="pcctslib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "pcctslib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "pcctslib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe

!IF  "$(CFG)" == "pcctslib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"pccts_release.lib"
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Desc=Copy to ..\lib
PostBuild_Cmds=mkdir ..\lib	copy pccts_release.lib ..\lib\pccts_release.lib
# End Special Build Tool

!ELSEIF  "$(CFG)" == "pcctslib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Z7 /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /FD /c
# SUBTRACT CPP /YX
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"pccts_debug.lib"
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Desc=Copy to ..\lib
PostBuild_Cmds=mkdir ..\lib	copy pccts_debug.lib ..\lib\pccts_debug.lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "pcctslib - Win32 Release"
# Name "pcctslib - Win32 Debug"
# Begin Source File

SOURCE=.\AParser.cpp
# End Source File
# Begin Source File

SOURCE=.\ASTBase.cpp
# End Source File
# Begin Source File

SOURCE=.\ATokenBuffer.cpp
# End Source File
# Begin Source File

SOURCE=.\BufFileInput.cpp
# End Source File
# Begin Source File

SOURCE=.\DLexerBase.cpp
# End Source File
# Begin Source File

SOURCE=.\PCCTSAST.cpp
# End Source File
# Begin Source File

SOURCE=.\SList.cpp
# End Source File
# End Target
# End Project
