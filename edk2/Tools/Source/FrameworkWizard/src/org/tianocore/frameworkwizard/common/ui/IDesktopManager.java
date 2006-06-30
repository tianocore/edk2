/** @file
 
 The file is used to override DefaultDesktopManager to provides customized interfaces 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.common.ui;

import javax.swing.DefaultDesktopManager;
import javax.swing.JComponent;

/**
 The class is used to override DefaultDesktopManager to provides customized interfaces
 It extends DefaultDesktopManager
 

 
 **/
public class IDesktopManager extends DefaultDesktopManager {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -4596986878722011062L;

    /**
     Main class, reserved for test
     
     @param args
     
     **/
    public static void main(String[] args) {
        // TODO Auto-generated method stub

    }

    /* (non-Javadoc)
     * @see javax.swing.DesktopManager#dragFrame(javax.swing.JComponent, int, int)
     * 
     * Override dragFrame to do nothing to forbid internalframe to be draged 
     * 
     */
    public void dragFrame(JComponent f, int newX, int newY) {

    }

    /* (non-Javadoc)
     * @see javax.swing.DesktopManager#endDraggingFrame(javax.swing.JComponent)
     * 
     * Override endDraggingFrame to do nothing to forbid internalframe to be draged
     * 
     */
    public void endDraggingFrame(JComponent f) {

    }

    /* (non-Javadoc)
     * @see javax.swing.DesktopManager#beginResizingFrame(javax.swing.JComponent, int)
     * 
     * Override beginResizingFrame to do nothing to forbid internalframe to be draged
     * 
     */
    public void beginResizingFrame(JComponent f, int direction) {

    }

}
