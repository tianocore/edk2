/** @file
  UIAction class.

  This class is the parent action class of UI wizard.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
 
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

package org.tianocore.build.pcd.action;

import org.tianocore.build.pcd.exception.UIException;

/** This class is the parent class for all UI wizard action.
**/
public abstract class UIAction {
    ///
    /// original message level. when finish this action, original 
    /// message level will be restored.
    ///
    private int originalMessageLevel;

    /**
      Check the parameter for this aciton.
      
      This function will be overrided by child class.
    **/
    abstract void checkParamter() throws UIException;

    /**
      Perform action.
      
      This function will be overrided by child class.
    **/
    abstract void performAction() throws UIException;

    /**
     set the message level for this action.

     @param messageLevel  message level wanted.
    **/
    public void setMessageLevel(int messageLevel) {
        originalMessageLevel        = ActionMessage.messageLevel;
        ActionMessage.messageLevel  = messageLevel;
    }

    /**
     log message for UI wizard aciton.

     @param actionObj  aciton instance object.
     @param logStr     log message string
    **/
    public static void logMsg(Object actionObj, String logStr) {
        System.out.println(logStr);
    }

    /**
     Warning message for UI wizard action.

     @param warningObj action instance object.
     @param warningStr warning message string.
    **/
    public static void warningMsg(Object warningObj, String warningStr) {
        System.out.println(warningStr);
    }

    /**
      Entry function for all UI wizard actions.
    **/
    public void execute() throws UIException {
        checkParamter();
        performAction();

        ActionMessage.messageLevel = originalMessageLevel;
    }
}
