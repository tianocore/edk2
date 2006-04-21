/** @file
 
 The file is used to show a welcome page in the process of setup 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.packaging.workspace.ui;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowEvent;

import javax.swing.JButton;
import javax.swing.JPanel;
import javax.swing.JTextArea;

import org.tianocore.packaging.common.ui.IFrame;

/**
 The class is used to show a welcome page in the process of setup
 
 @since CreateMdkPkg 1.0

 **/
public class Welcome extends IFrame implements ActionListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = 8160041311175680637L;

    //
    // Define class members
    //
    private JPanel jContentPane = null;

    private JPanel jPanel = null;

    private JTextArea jTextArea = null;

    private JTextArea jTextArea1 = null;

    private JTextArea jTextArea2 = null;

    private JTextArea jTextArea3 = null;

    private JButton jButtonNext = null;

    private JButton jButtonCancel = null;

    private LicenseAgreement la = null;

    /**
     This method initializes jPanel 
     
     @return javax.swing.JPanel jPanel
     
     **/
    private JPanel getJPanel() {
        if (jPanel == null) {
            jPanel = new JPanel();
            jPanel.setLayout(null);
            jPanel.setSize(new java.awt.Dimension(495, 355));
            jPanel.setLocation(new java.awt.Point(0, 0));
            jPanel.add(getJTextArea(), null);
            jPanel.add(getJTextArea1(), null);
            jPanel.add(getJTextArea2(), null);
            jPanel.add(getJTextArea3(), null);
            jPanel.add(getJButtonNext(), null);
            jPanel.add(getJButtonCancel(), null);
        }
        return jPanel;
    }

    /**
     This method initializes jTextArea 
     
     @return javax.swing.JTextArea jTextArea
     
     **/
    private JTextArea getJTextArea() {
        if (jTextArea == null) {
            jTextArea = new JTextArea();
            jTextArea.setFont(new java.awt.Font("Times New Roman", java.awt.Font.BOLD, 24));
            jTextArea.setSize(new java.awt.Dimension(495, 70));
            jTextArea.setLocation(new java.awt.Point(0, 0));
            jTextArea.setEnabled(true);
            jTextArea.setEditable(false);
            jTextArea.setText("Welcome to the MDK Package Setup Wizard");
        }
        return jTextArea;
    }

    /**
     This method initializes jTextArea1 
     
     @return javax.swing.JTextArea jTextArea1
     
     **/
    private JTextArea getJTextArea1() {
        if (jTextArea1 == null) {
            jTextArea1 = new JTextArea();
            jTextArea1.setText("This will install MDK Package on your computer. ");
            jTextArea1.setSize(new java.awt.Dimension(495, 40));
            jTextArea1.setEnabled(true);
            jTextArea1.setEditable(false);
            jTextArea1.setLocation(new java.awt.Point(0, 70));
        }
        return jTextArea1;
    }

    /**
     This method initializes jTextArea2 
     
     @return javax.swing.JTextArea jTextArea2
     
     **/
    private JTextArea getJTextArea2() {
        if (jTextArea2 == null) {
            jTextArea2 = new JTextArea();
            jTextArea2.setSize(new java.awt.Dimension(495, 50));
            jTextArea2
                      .setText("It is strongly recommended that you exit all other programs before running this installation program.");
            jTextArea2.setLineWrap(true);
            jTextArea2.setEnabled(true);
            jTextArea2.setEditable(false);
            jTextArea2.setLocation(new java.awt.Point(0, 110));
        }
        return jTextArea2;
    }

    /**
     This method initializes jTextArea3 
     
     @return javax.swing.JTextArea jTextArea3
     
     **/
    private JTextArea getJTextArea3() {
        if (jTextArea3 == null) {
            jTextArea3 = new JTextArea();
            jTextArea3.setBounds(new java.awt.Rectangle(0, 160, 495, 150));
            jTextArea3.setEnabled(true);
            jTextArea3.setEditable(false);
            jTextArea3.setText("Click Nex to continue. Or click Cancel to exit Setup");
        }
        return jTextArea3;
    }

    /**
     This method initializes jButtonNext 
     
     @return javax.swing.JButton jButtonNext
     
     **/
    private JButton getJButtonNext() {
        if (jButtonNext == null) {
            jButtonNext = new JButton();
            jButtonNext.setText("Next");
            jButtonNext.setSize(new java.awt.Dimension(90, 20));
            jButtonNext.setLocation(new java.awt.Point(290, 320));
            jButtonNext.setMnemonic('N');
            jButtonNext.addActionListener(this);
        }
        return jButtonNext;
    }

    /**
     This method initializes jButtonCancel 
     
     @return javax.swing.JButton jButtonCancel
     
     **/
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setText("Cancel");
            jButtonCancel.setSize(new java.awt.Dimension(90, 20));
            jButtonCancel.setLocation(new java.awt.Point(390, 320));
            jButtonCancel.setMnemonic('C');
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     Main class, used for test
     
     @param args
     **/
    public static void main(String[] args) {
        Welcome w = new Welcome();
        w.setVisible(true);
    }

    /**
     This is the default constructor
     
     **/
    public Welcome() {
        super();
        init();
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 390);
        this.setContentPane(getJContentPane());
        this.setTitle("Welcome");
        this.centerWindow();
        this.getRootPane().setDefaultButton(jButtonNext);
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJPanel(), null);
        }
        return jContentPane;
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     * 
     * Override actionPerformed to listen all actions
     * 
     */
    public void actionPerformed(ActionEvent arg0) {
        Object obj = arg0.getSource();
        //
        // Show next page if click button Next
        //
        if (obj == jButtonNext) {
            if (la == null) {
                la = new LicenseAgreement(this);
            }
            this.setVisible(false);
            la.setVisible(true);
        }
        if (obj == jButtonCancel) {
            this.onExit();
        }
    }

    /* (non-Javadoc)
     * @see java.awt.event.WindowListener#windowClosing(java.awt.event.WindowEvent)
     * 
     * Override windowClosing to show confirm quit dialog
     * 
     */
    public void windowClosing(WindowEvent arg0) {
        this.onExit();
    }
}
