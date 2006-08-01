/** @file
  ActionMessage class.

  ActionMessage class take over all message for loging and waning. This class should
  dispatch message into different class according to instance class type.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.pcd.action;

import org.apache.tools.ant.Task;
import org.tianocore.logger.EdkLog;

/** ActionMessage class take over all message for loging and waning. This class
    should dispatch message into different Action class according to instance
    class type.
**/
public class ActionMessage {
    ///
    /// Macro definition for NULL messge level.
    /// In this meessage level, all message will be hidden.
    ///
    public final static int NULL_MESSAGE_LEVEL    = 0;

    ///
    /// Macro definition for Log messge level.
    /// In this message level, Only log information will be shown.
    ///
    public final static int LOG_MESSAGE_LEVEL     = 1;

    ///
    /// Macro definition for Warning message level.
    /// In this message level, log and waning message will be shown.
    ///
    public final static int WARNING_MESSAGE_LEVEL = 2;

    ///
    /// Macro definition for Debug mesage level.
    /// In this message level, log, warning, debug message will be shown.
    ///
    public final static int DEBUG_MESSAGE_LEVEL   = 3;

    ///
    /// Macor definition for MAX message level.
    /// In this message level, all message will be shown.
    ///
    public final static int MAX_MESSAGE_LEVEL     = 4;

    ///
    /// Current message level. It will control all message output for PCD tool.
    ///
    public       static int messageLevel          = NULL_MESSAGE_LEVEL;

    /**
      Log() function provide common log information functionality for all
      PCD tool includes all function

      This function will dispatch message to special class such as BuildAction
      Class, Entity Class etc.

      @param thisClass   The class object who want log information.
      @param logStr      The string contains log information.
    **/
    public static void log(Object thisClass, String logStr) {
        if(messageLevel < LOG_MESSAGE_LEVEL) {
            return;
        }

        if(thisClass instanceof Task) {
            BuildAction.logMsg(thisClass, "$$LOG$$:" + logStr);
        } else {
            System.out.println("$$LOG$$:" + logStr);
        }
    }

    /**
      Warning() function provide common warning information functionality for all
      PCD tool.

      This function will dispatch message to special class such as BuildAction
      Class, Entity Class etc.

      @param thisClass   The class object who want warn information.
      @param warningStr  The string contains warning information.
    **/
    public static void warning(Object thisClass, String warningStr) {
        if(messageLevel < WARNING_MESSAGE_LEVEL) {
            return;
        }

        if(thisClass instanceof Task) {
            BuildAction.warningMsg(thisClass, "**WARNING**:" + warningStr);
        } else {
            System.out.println("**WARNING**:" + warningStr);
        }
    }

    /**
      Debug() function provide common Debug information functionality for all
      PCD tool.

      This function will dispatch message to special class such as BuildAction
      Class, Entity Class etc.

      @param thisClass   The class object who want Debug information.
      @param debugStr    The string contains Debug information.
    **/
    public static void debug(Object thisClass, String debugStr) {
        if(messageLevel < DEBUG_MESSAGE_LEVEL) {
            return;
        }

        if(thisClass instanceof Task) {
            BuildAction.logMsg(thisClass, "%%DEBUG%%:" + debugStr);
        } else {
            System.out.println("%%DEBUG%%:" + debugStr);
        }
    }
}
