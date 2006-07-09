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
# This file must be "sourced" not executed. For example: ". edksetup.sh"

export WORKSPACE=$(pwd)

# In unix-like system, gcc is the compiler for building tools
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
export CLASSPATH=$WORKSPACE/Tools/Jars/SurfaceArea.jar:$WORKSPACE/Tools/Jars/frameworktasks.jar:$WORKSPACE/Tools/Jars/cpptasks.jar:$WORKSPACE/Tools/Jars/GenBuild.jar:$XMLBEANS_HOME/lib/resolver.jar:$XMLBEANS_HOME/lib/xbean.jar:$XMLBEANS_HOME/lib/xmlpublic.jar:$XMLBEANS_HOME/lib/jsr173_1.0_api.jar:$XMLBEANS_HOME/lib/saxon8.jar:$XMLBEANS_HOME/lib/xbean_xpath.jar
export CLASSPATH=$CLASSPATH:$WORKSPACE/Tools/Jars/Common.jar
export FRAMEWORK_TOOLS_PATH=$WORKSPACE/Tools/bin
export PATH=$FRAMEWORK_TOOLS_PATH:$ANT_HOME/bin:$JAVA_HOME/bin:$PATH
# In some unix-like system, following export is to export system's environment to user's environment
export ANT_HOME=$ANT_HOME
export JAVA_HOME=$JAVA_HOME
export XMLBEANS_HOME=$XMLBEANS_HOME

# Handle any particulars down here.
case "`uname`" in
  CYGWIN*) 
    # Convert paths to windows format.
    export WORKSPACE=`cygpath -w $WORKSPACE`
    export CLASSPATH=`cygpath -w -p $CLASSPATH`
    ;;
esac

# Now we need to build the tools.
(cd Tools; ant -noclasspath)
fi
fi
fi
