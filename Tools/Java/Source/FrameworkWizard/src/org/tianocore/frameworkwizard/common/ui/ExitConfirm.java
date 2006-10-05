/** @file
 
 The file is used to popup a exit confirmation window when program exists
 
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
import java.awt.event.WindowEvent;
import java.awt.event.WindowListener;

import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;

/**
 The class is used to popup a exit confirmation window when program exists
 It extends JDialog and implements ActionListener and WindowListener
 
 **/
public class ExitConfirm extends JDialog implements ActionListener, WindowListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -5875921789385911029L;

    private JPanel jContentPane = null;

    private JLabel jLabelMessage = null;

    private JLabel jLabelResume = null;

    private JLabel jLabelExit = null;

    private JButton jButtonResume = null;

    private JButton jButtonExit = null;

    public boolean isCancel = false;

    /**
     This method initializes jButtonResume
     
     @return javax.swing.JButton jButtonResume
     
     **/
    private JButton getJButtonResume() {
        if (jButtonResume == null) {
            jButtonResume = new JButton();
            jButtonResume.setText("Resume");
            jButtonResume.setSize(new java.awt.Dimension(90, 20));
            jButtonResume.setLocation(new java.awt.Point(150, 105));
            jButtonResume.setMnemonic('R');
            jButtonResume.addActionListener(this);
        }
        return jButtonResume;
    }

    /**
     This method initializes jButtonExit
     
     @return javax.swing.JButton jButtonExit
     
     **/
    private JButton getJButtonExit() {
        if (jButtonExit == null) {
            jButtonExit = new JButton();
            jButtonExit.setText("Exit");
            jButtonExit.setSize(new java.awt.Dimension(90, 20));
            jButtonExit.setLocation(new java.awt.Point(260, 105));
            jButtonExit.setMnemonic('x');
            jButtonExit.addActionListener(this);
        }
        return jButtonExit;
    }

    /**
     Main clasee, reserved for test
     
     @param args
     
     **/
    public static void main(String[] args) {
        // TODO Auto-generated method stub

    }

    /**
     This is the default constructor
     
     **/
    public ExitConfirm(IFrame parentFrame, boolean modal) {
        super(parentFrame, modal);
        initialize();
    }

    /**
     This method initializes this
     
     @return void
     
     **/
    private void initialize() {
        this.setSize(500, 170);
        this.setTitle("Exit");
        this.setResizable(false);
        this.setContentPane(getJContentPane());
        this.addWindowListener(this);
        //
        //Set DO_NOTHING_ON_CLOSE when click Close button on title bar
        //
        this.setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
        centerWindow();
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelExit = new JLabel();
            jLabelExit.setSize(new java.awt.Dimension(450, 20));
            jLabelExit.setLocation(new java.awt.Point(25, 70));
            jLabelResume = new JLabel();
            jLabelResume.setSize(new java.awt.Dimension(450, 20));
            jLabelResume.setLocation(new java.awt.Point(25, 40));
            jLabelMessage = new JLabel();
            jLabelMessage.setSize(new java.awt.Dimension(450, 20));
            jLabelMessage.setLocation(new java.awt.Point(25, 10));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(jLabelMessage, null);
            jContentPane.add(jLabelResume, null);
            jContentPane.add(jLabelExit, null);
            jContentPane.add(getJButtonResume(), null);
            jContentPane.add(getJButtonExit(), null);
        }
        return jContentPane;
    }

    /**
     Call setWarningMessage to set messages of frame when it is used for Setup
     
     **/
    public void setSetupMessage() {
        String strTitle = "Exit Setup";
        String strMessage = "Setup is not complete. If you quit now, the program will not be installed.";
        String strResume = "You may run the setup program at a later time to complete the installation.";
        String strExit = "To continue installing, click Resume. To quit the Setup program, click Exit.";
        setWarningMessage(strTitle, strMessage, strResume, strExit);
    }

    /**
     Call setWarningMessage to set messages of frame when it is used for Module Main GUI
     
     **/
    public void setModuleMessage() {
        String strTitle = "Exit";
        String strMessage = "Do you really want to quit now?";
        String strResume = "All unsaved module information will be lost.";
        String strExit = "To continue editing the module, click Resume. To quit the program, click Exit.";
        setWarningMessage(strTitle, strMessage, strResume, strExit);
    }

    /**
     Set message information via input data
     
     @param strTitle The title value
     @param strMessage The main message value
     @param strResume The resume message value
     @param strExit The exit message value
     
     **/
    private void setWarningMessage(String strTitle, String strMessage, String strResume, String strExit) {
        this.setTitle(strTitle);
        jLabelMessage.setText(strMessage);
        jLabelResume.setText(strResume);
        jLabelExit.setText(strExit);
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     * 
     * Override actionPerformed to listern all actions
     * 
     */
    public void actionPerformed(ActionEvent arg0) {
        //
        //Set isCancel true when click button "Exit"
        //
        Object obj = arg0.getSource();
        if (obj == jButtonResume) {
            isCancel = false;
        }
        if (obj == jButtonExit) {
            isCancel = true;
        }
        this.setVisible(false);
    }

    /**
     Make the window in the center of the screen
     
     **/
    private void centerWindow() {
        Dimension d = Toolkit.getDefaultToolkit().getScreenSize();
        this.setLocation((d.width - this.getSize().width) / 2, (d.height - this.getSize().height) / 2);
    }

    public void windowActivated(WindowEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void windowClosed(WindowEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void windowClosing(WindowEvent arg0) {
        isCancel = false;
        this.setVisible(false);
    }

    public void windowDeactivated(WindowEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void windowDeiconified(WindowEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void windowIconified(WindowEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void windowOpened(WindowEvent arg0) {
        // TODO Auto-generated method stub

    }
}
