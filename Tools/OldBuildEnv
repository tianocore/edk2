#
# Copyright (c) 2006, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
# 
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

# Setup the environment for unix-like systems running a bash-like shell.
# This file must be "sourced" not merely executed. For example: ". edksetup.sh"

# CYGWIN users: Your path and filename related environment variables should be
# set up in the unix style.  This script will make the necessary conversions to
# windows style.

export WORKSPACE=$(pwd)

# In unix-like systems, gcc is the compiler for building tools
export TOOL_CHAIN=gcc

if [ "$JAVA_HOME" == "" ]
then
  echo "Please set JAVA_HOME before sourcing this script."
else
if [ "$ANT_HOME" == "" ]
then
  echo "Please set ANT_HOME before sourcing this script."
else 
if [ "$XMLBEANS_HOME" == "" ]
then
  echo "Please set XMLBEANS_HOME before sourcing this script."
else
  
# These should be ok as they are.
export CLASSPATH=$ANT_HOME/lib/ant-contrib.jar:$WORKSPACE/Tools/Jars/SurfaceArea.jar:$WORKSPACE/Tools/Jars/frameworktasks.jar:$WORKSPACE/Tools/Jars/cpptasks.jar:$WORKSPACE/Tools/Jars/PcdTools.jar:$WORKSPACE/Tools/Jars/GenBuild.jar:$XMLBEANS_HOME/lib/resolver.jar:$XMLBEANS_HOME/lib/xbean.jar:$XMLBEANS_HOME/lib/xmlpublic.jar:$XMLBEANS_HOME/lib/jsr173_1.0_api.jar:$XMLBEANS_HOME/lib/saxon8.jar:$XMLBEANS_HOME/lib/xbean_xpath.jar:$XMLBEANS_HOME/lib/saxon8-dom.jar:$XMLBEANS_HOME/lib/saxon8-xpath.jar
export CLASSPATH=$CLASSPATH:$WORKSPACE/Tools/Jars/Common.jar
export CLASSPATH=$CLASSPATH:$WORKSPACE/Tools/Jars/PcdTools.jar
export CLASSPATH=$CLASSPATH:$WORKSPACE/Tools/bin/FrameworkWizard.jar
export FRAMEWORK_TOOLS_PATH=$WORKSPACE/Tools/bin
export PATH=$FRAMEWORK_TOOLS_PATH:$ANT_HOME/bin:$JAVA_HOME/bin:$PATH

# Handle any particulars down here.
case "`uname`" in
  CYGWIN*) 
    # Convert paths to windows format.
    export WORKSPACE=`cygpath -w $WORKSPACE`
    export ANT_HOME=`cygpath -w $ANT_HOME`
    export XMLBEANS_HOME=`cygpath -w $XMLBEANS_HOME`
    export CLASSPATH=`cygpath -w -p $CLASSPATH`
    export FRAMEWORK_TOOLS_PATH=`cygpath -w -p $FRAMEWORK_TOOLS_PATH`
    ;;
esac

if [ \
  "$1" = Rebuild -o \
  "$1" = ForceRebuild -o \
  ! -e "$WORKSPACE/Tools/Jars/Common.jar" -o \
  ! -e "$WORKSPACE/Tools/Jars/PcdTools.jar" -o \
  ! -e "$WORKSPACE/Tools/Jars/GenBuild.jar" -o \
  ! -e "$WORKSPACE/Tools/Jars/SurfaceArea.jar" -o \
  ! -e "$WORKSPACE/Tools/Jars/cpptasks.jar" -o \
  ! -e "$WORKSPACE/Tools/Jars/frameworktasks.jar" -o \
  ! -e "$WORKSPACE/Tools/bin/FrameworkWizard.jar" -o \
  ! -e "$WORKSPACE/Tools/bin/CompressDll.dll" -o \
  ! -e "$WORKSPACE/Tools/bin/CreateMtFile" -o \
  ! -e "$WORKSPACE/Tools/bin/EfiCompress" -o \
  ! -e "$WORKSPACE/Tools/bin/EfiRom" -o \
  ! -e "$WORKSPACE/Tools/bin/FlashMap" -o \
  ! -e "$WORKSPACE/Tools/bin/FwImage" -o \
  ! -e "$WORKSPACE/Tools/bin/GenAcpiTable" -o \
  ! -e "$WORKSPACE/Tools/bin/GenCRC32Section" -o \
  ! -e "$WORKSPACE/Tools/bin/GenCapsuleHdr" -o \
  ! -e "$WORKSPACE/Tools/bin/GenDepex" -o \
  ! -e "$WORKSPACE/Tools/bin/GenFfsFile" -o \
  ! -e "$WORKSPACE/Tools/bin/GenFvImage" -o \
  ! -e "$WORKSPACE/Tools/bin/GenSection" -o \
  ! -e "$WORKSPACE/Tools/bin/GenTEImage" -o \
  ! -e "$WORKSPACE/Tools/bin/MakeDeps" -o \
  ! -e "$WORKSPACE/Tools/bin/ModifyInf" -o \
  ! -e "$WORKSPACE/Tools/bin/PeiRebase_Ia32" -o \
  ! -e "$WORKSPACE/Tools/bin/PeiRebase_Ipf" -o \
  ! -e "$WORKSPACE/Tools/bin/PeiRebase_X64" -o \
  ! -e "$WORKSPACE/Tools/bin/SecApResetVectorFixup" -o \
  ! -e "$WORKSPACE/Tools/bin/SecFixup" -o \
  ! -e "$WORKSPACE/Tools/bin/SetStamp" -o \
  ! -e "$WORKSPACE/Tools/bin/SplitFile" -o \
  ! -e "$WORKSPACE/Tools/bin/StrGather" -o \
  ! -e "$WORKSPACE/Tools/bin/Strip" -o \
  ! -e "$WORKSPACE/Tools/bin/VfrCompile" -o \
  ! -e "$WORKSPACE/Tools/bin/ZeroDebugData" -o \
  ! -e "$WORKSPACE/Tools/bin/antlr" -o \
  ! -e "$WORKSPACE/Tools/bin/dlg" ]
then
  case "$1" in 
    ForceRebuild)
      ant -noclasspath -f $WORKSPACE/Tools/build.xml cleanall all
      ;;
    *)
      ant -noclasspath -f $WORKSPACE/Tools/build.xml all
      ;;
  esac
fi
fi
fi
fi
