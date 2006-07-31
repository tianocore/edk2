/** @file
  BuildAction class.

  BuildAction is the parent class for all action related to ant Task. This class will
  define some common utility functionality, such as logMsg, warningMsg..etc.
 
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
import org.apache.tools.ant.Project;
import org.tianocore.pcd.exception.BuildActionException;

/** BuildAction is the parent class for all action related to ant Task. This class will
    define some common utility functionality, such as logMsg, warningMsg..etc.
**/
public abstract class BuildAction extends Task {
    ///
    /// Original message level before this action. This value will 
    /// be restored when quit this action.
    ///
    private int originalMessageLevel;

    /**
      checkParameter function check all parameter valid.

      This function will be overrided by child class.
    **/
    public abstract void checkParameter() throws BuildActionException;

    /**
     performAction is to execute the detail action.
      
     This function will be overrided by child class.
    **/
    public abstract void performAction() throws BuildActionException;

    /**
      setMessageLevel function set current message for task instance object.

      The message should be restored when this action exit.

      @param messageLevel The message level for this action.
    **/
    public void setMessageLevel(int messageLevel) {
        originalMessageLevel        = ActionMessage.messageLevel;
        ActionMessage.messageLevel  = messageLevel;
    }

    /**
      logMsg function provide common log information functionality for all 
      PCD tool extends from ANT task class.

      This function will use the log function in Ant task class.
     
      @param action     The class object who want log information.
      @param logStr     The string contains log information.
    **/
    public static void logMsg(Object action, String logStr) {
        ((Task) action).log(logStr, Project.MSG_INFO);
    }

    /**
      warningMsg function provide common warning information functionality for all 
      PCD tool.

      This function will dispatch message to special class such as BuildAction
      Class, Entity Class etc.
     
      @param action      The class object who want warn information.
      @param warningStr  The string contains warning information.
    **/  
    public static void warningMsg(Object action, String warningStr) {
        ((Task) action).log(warningStr, Project.MSG_WARN);
    }

    /**
      execute function is the main flow for all build action class.

      This workflow will be:
      1) Check paramet of this action.
      2) Perform the child class action function.
      3) Restore the message level.
     
      @throws BuildActionException
    **/  
    public void execute() throws BuildActionException {
        checkParameter();
        performAction();

        //
        // Restore orignal message level when exist the action.
        //
        ActionMessage.messageLevel = originalMessageLevel;
    }
}
