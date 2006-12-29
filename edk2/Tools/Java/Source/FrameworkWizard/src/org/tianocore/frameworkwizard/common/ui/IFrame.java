/** @file
 
 The file is used to override Frame to provides customized interfaces 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.common.ui;

import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ComponentEvent;
import java.awt.event.ComponentListener;
import java.awt.event.WindowEvent;
import java.awt.event.WindowListener;

import javax.swing.JFrame;
import javax.swing.JOptionPane;

import org.tianocore.frameworkwizard.common.Tools;

/**
 The class is used to override Frame to provides customized interfaces 
 It extends JFrame implements ActionListener and WindowListener
 
 **/
public class IFrame extends JFrame implements ActionListener, WindowListener, ComponentListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -3324138961029300427L;

    //
    //Define class members
    //
    private ExitConfirm ec = null;

    //
    // To indicate the status while quit
    // 0 - When setup (Default)
    // 1 - Whne editing module
    //
    private int intExitType = 0;

    /**
     Main class, used for test
     
     @param args
     
     **/
    public static void main(String[] args) {
        IFrame i = new IFrame();
        i.setVisible(true);
    }

    /**
     This is the default constructor
     
     **/
    public IFrame() {
        super();
        initialize();
    }

    /**
     This method initializes this
     
     **/
    public void initialize() {
        this.setResizable(false);
        this.setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
        this.addWindowListener(this);
        this.addComponentListener(this);
    }

    /**
     Start the dialog at the center of screen
     
     @param intWidth The width of the dialog
     @param intHeight The height of the dialog
     
     **/
    protected void centerWindow(int intWidth, int intHeight) {
        Dimension d = Toolkit.getDefaultToolkit().getScreenSize();
        this.setLocation((d.width - intWidth) / 2, (d.height - intHeight) / 2);
    }

    /**
     Start the window full of the screen
     
     **/
    protected void maxWindow() {
        Dimension d = Toolkit.getDefaultToolkit().getScreenSize();
        this.setLocation(0, 0);
        this.setSize(d);
    }

    /**
     Start the dialog at the center of screen
     
     **/
    protected void centerWindow() {
        centerWindow(this.getSize().width, this.getSize().height);
    }

    /**
     Set the exit window type
     
     @param ExitType The input data of ExitType
     
     **/
    protected void setExitType(int ExitType) {
        this.intExitType = ExitType;
    }

    /* (non-Javadoc)
     * @see java.awt.event.WindowListener#windowClosing(java.awt.event.WindowEvent)
     *
     * Override windowClosing to call this.onDisvisible()
     * 
     */
    public void windowClosing(WindowEvent arg0) {
        //this.onDisvisible();
    }

    public void windowOpened(WindowEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void windowClosed(WindowEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void windowIconified(WindowEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void windowDeiconified(WindowEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void windowActivated(WindowEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void windowDeactivated(WindowEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void actionPerformed(ActionEvent arg0) {
        // TODO Auto-generated method stub

    }

    /**
     Define the actions when exit
     
     **/
    public void onExit() {
        ec = new ExitConfirm(this, true);
        //
        //Show different warning message via different ExitType
        //
        switch (intExitType) {
        case 0:
            ec.setSetupMessage();
            break;
        case 1:
            ec.setModuleMessage();
            break;
        }
        ec.setVisible(true);
        if (ec.isCancel) {
            this.dispose();
            System.exit(0);
        }
    }

    /**
     Define the actions when disvisible
     
     **/
    public void onDisvisible() {
        ec = new ExitConfirm(this, true);
        //
        //Show different warning message via different ExitType
        //
        switch (intExitType) {
        case 0:
            ec.setSetupMessage();
            break;
        case 1:
            ec.setModuleMessage();
            break;
        }
        ec.setVisible(true);
        if (ec.isCancel) {
            this.dispose();
        }
    }

    public int showSaveDialog() {
        return JOptionPane.showConfirmDialog(this, "Save all changed files?", "Save", JOptionPane.YES_NO_CANCEL_OPTION,
                                             JOptionPane.WARNING_MESSAGE);
    }

    /**
     Check the input data is empty or not
     
     @param strValue The input data which need be checked
     
     @retval true - The input data is empty
     @retval fals - The input data is not empty
     
     **/
    public boolean isEmpty(String strValue) {
        return Tools.isEmpty(strValue);
    }

    /**
     Display the dialog
     
     **/
    public void showDialog() {
        this.setVisible(true);
    }

    public void componentResized(ComponentEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void componentMoved(ComponentEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void componentShown(ComponentEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void componentHidden(ComponentEvent arg0) {
        // TODO Auto-generated method stub

    }
}
