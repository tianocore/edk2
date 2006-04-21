/** @file
 
 The class is used to show a License Agreement page in
 the process of setup
 
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
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;

import org.tianocore.packaging.common.ui.IFrame;

/**
 The class is used to show a License Agreement page in
 the process of setup
 
 @since CreateMdkPkg 1.0

 **/
public class LicenseAgreement extends IFrame implements ActionListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = 5507683268692334188L;

    //
    // Define class members
    //
    private JPanel jContentPane = null;

    private JTextArea jTextArea = null;

    private JTextArea jTextArea1 = null;

    private JPanel jPanel = null;

    private JButton jButtonBack = null;

    private JButton jButtonNext = null;

    private JButton jButtonCancel = null;

    private JLabel jLabel = null;

    private JRadioButton jRadioButtonAgree = null;

    private JRadioButton jRadioButtonDisagree = null;

    private JScrollPane jScrollPane = null;

    private JTextArea jTextArea2 = null;

    private JLabel jLabel1 = null;

    private Welcome w = null;

    private SelectDestinationDirectory sdd = null;

    /**
     This method initializes jTextArea 
     
     @return javax.swing.JTextArea jTextArea
     
     **/
    private JTextArea getJTextArea() {
        if (jTextArea == null) {
            jTextArea = new JTextArea();
            jTextArea.setLocation(new java.awt.Point(0, 0));
            jTextArea.setText("    License Agreement");
            jTextArea.setFont(new java.awt.Font("Dialog", java.awt.Font.BOLD, 14));
            jTextArea.setEditable(false);
            jTextArea.setSize(new java.awt.Dimension(495, 20));
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
            jTextArea1.setLocation(new java.awt.Point(0, 20));
            jTextArea1.setText("           Please read the following important information before continuing.");
            jTextArea1.setEditable(false);
            jTextArea1.setSize(new java.awt.Dimension(495, 35));
        }
        return jTextArea1;
    }

    /**
     This method initializes jPanel 
     
     @return javax.swing.JPanel jPanel
     
     **/
    private JPanel getJPanel() {
        if (jPanel == null) {
            jLabel1 = new JLabel();
            jLabel1.setText(" this agreement before continuing with the installation.");
            jLabel1.setLocation(new java.awt.Point(30, 35));
            jLabel1.setSize(new java.awt.Dimension(435, 20));
            jLabel = new JLabel();
            jLabel.setText("Please read the following License Agreement. You must accept the terms of");
            jLabel.setLocation(new java.awt.Point(30, 15));
            jLabel.setSize(new java.awt.Dimension(435, 20));
            jPanel = new JPanel();
            jPanel.setLayout(null);
            jPanel.setBorder(javax.swing.BorderFactory.createBevelBorder(javax.swing.border.BevelBorder.LOWERED));
            jPanel.setSize(new java.awt.Dimension(494, 251));
            jPanel.setLocation(new java.awt.Point(0, 55));
            jPanel.add(jLabel, null);
            jPanel.add(getJRadioButtonAgree(), null);
            jPanel.add(getJRadioButtonDisagree(), null);
            jPanel.add(getJScrollPane(), null);
            jPanel.add(jLabel1, null);
        }
        return jPanel;
    }

    /**
     This method initializes jButtonBack 
     
     @return javax.swing.JButton jButtonBack
     
     **/
    private JButton getJButtonBack() {
        if (jButtonBack == null) {
            jButtonBack = new JButton();
            jButtonBack.setText("Back");
            jButtonBack.setSize(new java.awt.Dimension(90, 20));
            jButtonBack.setLocation(new java.awt.Point(200, 315));
            jButtonBack.setMnemonic('B');
            jButtonBack.addActionListener(this);
        }
        return jButtonBack;
    }

    /**
     This method initializes jButtonNext 
     
     @return javax.swing.JButton jButtonNext
     
     **/
    private JButton getJButtonNext() {
        if (jButtonNext == null) {
            jButtonNext = new JButton();
            jButtonNext.setText("Next");
            jButtonNext.setBounds(new java.awt.Rectangle(292, 315, 90, 20));
            jButtonNext.setEnabled(false);
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
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 315, 90, 20));
            jButtonCancel.setMnemonic('C');
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jRadioButtonAgree 
     
     @return javax.swing.JRadioButton jRadioButtonAgree
     
     **/
    private JRadioButton getJRadioButtonAgree() {
        if (jRadioButtonAgree == null) {
            jRadioButtonAgree = new JRadioButton();
            jRadioButtonAgree.setText("I accept the agreement");
            jRadioButtonAgree.setLocation(new java.awt.Point(30, 200));
            jRadioButtonAgree.setSize(new java.awt.Dimension(156, 19));
            jRadioButtonAgree.addActionListener(this);
        }
        return jRadioButtonAgree;
    }

    /**
     This method initializes jRadioButtonDisagree 
     
     @return javax.swing.JRadioButton jRadioButtonDisagree
     
     **/
    private JRadioButton getJRadioButtonDisagree() {
        if (jRadioButtonDisagree == null) {
            jRadioButtonDisagree = new JRadioButton();
            jRadioButtonDisagree.setText("I do not accept the agreement");
            jRadioButtonDisagree.setLocation(new java.awt.Point(30, 220));
            jRadioButtonDisagree.setSize(new java.awt.Dimension(248, 19));
            jRadioButtonDisagree.addActionListener(this);
        }
        return jRadioButtonDisagree;
    }

    /**
     This method initializes jScrollPane 
     
     @return javax.swing.JScrollPane jScrollPane
     
     **/
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setSize(new java.awt.Dimension(435, 140));
            jScrollPane.setVerticalScrollBarPolicy(javax.swing.JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);
            jScrollPane.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jScrollPane.setBorder(javax.swing.BorderFactory.createBevelBorder(javax.swing.border.BevelBorder.LOWERED));
            jScrollPane.setViewportView(getJTextArea2());
            jScrollPane.setLocation(new java.awt.Point(30, 55));
        }
        return jScrollPane;
    }

    /**
     This method initializes jTextArea2 
     
     @return javax.swing.JTextArea jTextArea2
     
     **/
    private JTextArea getJTextArea2() {
        if (jTextArea2 == null) {
            jTextArea2 = new JTextArea();
            jTextArea2.setEditable(false);
            jTextArea2.setWrapStyleWord(false);
            jTextArea2.setLineWrap(true);
            jTextArea2.setText("Copyright (c) 2006, Intel Corp.\n"
                               + "All rights reserved. This program and the accompanying materials "
                               + "are licensed and made available under the terms and conditions of the BSD License "
                               + "which may be found at http://opensource.org/licenses/bsd-license.php\n\n\n"
                               + "THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN \"AS IS\" BASIS, "
                               + "WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.");
        }
        return jTextArea2;
    }

    /**
     Main class, used for test
     
     @param args
     
     **/
    public static void main(String[] args) {
        LicenseAgreement la = new LicenseAgreement();
        la.setVisible(true);
    }

    /**
     This is the default constructor
     
     **/
    public LicenseAgreement() {
        super();
        init();
    }

    /**
     This is the override constructor

     @param welcome The input data of Welcome
     
     **/
    public LicenseAgreement(Welcome welcome) {
        super();
        init();
        w = welcome;
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 390);
        this.setContentPane(getJContentPane());
        this.setTitle("Setup - License Agreement");
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
            jContentPane.setBorder(javax.swing.BorderFactory.createBevelBorder(javax.swing.border.BevelBorder.RAISED));
            jContentPane.add(getJTextArea(), null);
            jContentPane.add(getJTextArea1(), null);
            jContentPane.add(getJPanel(), null);
            jContentPane.add(getJButtonBack(), null);
            jContentPane.add(getJButtonNext(), null);
            jContentPane.add(getJButtonCancel(), null);
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
        // Disable button next when select jRadioButtonDisagree
        //
        if (obj == jRadioButtonDisagree) {
            if (jRadioButtonDisagree.isSelected()) {
                jRadioButtonAgree.setSelected(false);
                jButtonNext.setEnabled(false);
                jButtonNext.setFocusable(false);
            }
            if (!jRadioButtonAgree.isSelected() && !jRadioButtonDisagree.isSelected()) {
                jRadioButtonDisagree.setSelected(true);
            }
        }
        
        //
        // Enable button next when select jRadioButtonAgree
        //
        if (obj == jRadioButtonAgree) {
            if (jRadioButtonAgree.isSelected()) {
                jRadioButtonDisagree.setSelected(false);
                jButtonNext.setEnabled(true);
                jButtonNext.setFocusable(true);
            }
            if (!jRadioButtonAgree.isSelected() && !jRadioButtonDisagree.isSelected()) {
                jRadioButtonAgree.setSelected(true);
            }
        }

        if (obj == jButtonBack) {
            this.setVisible(false);
            w.setVisible(true);
        }

        //
        // Show next page when click button Next
        //
        if (obj == jButtonNext) {
            if (sdd == null) {
                sdd = new SelectDestinationDirectory(this);
            }
            this.setVisible(false);
            sdd.setVisible(true);
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
