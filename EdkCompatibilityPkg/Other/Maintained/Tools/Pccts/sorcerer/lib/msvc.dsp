# Microsoft Developer Studio Project File - Name="lib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=lib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "lib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "lib.mak" CFG="lib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "lib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "lib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""$/pccts/Sorcerer/lib", EPABAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe

!IF  "$(CFG)" == "lib - Win32 Release"

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
# ADD CPP /nologo /W3 /GX /O2 /I "h" /I "..\h" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "PC" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "lib - Win32 Debug"

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
# ADD CPP /nologo /W3 /GX /Z7 /Od /I "h" /I "..\h" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "PC" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "lib - Win32 Release"
# Name "lib - Win32 Debug"
# Begin Group "Header files"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\CASTBase.h
# End Source File
# End Group
# Begin Group "Source files"

# PROP Default_Filter "c,cpp"
# Begin Source File

SOURCE=.\astlib.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\errsupport.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\PCCTSAST.cpp
# End Source File
# Begin Source File

SOURCE=.\sintstack.c
# End Source File
# Begin Source File

SOURCE=.\SList.cpp
# End Source File
# Begin Source File

SOURCE=.\sorcerer.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\sorlist.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\sstack.c
# End Source File
# Begin Source File

SOURCE=.\STreeParser.cpp
# End Source File
# End Group
# End Target
# End Project
