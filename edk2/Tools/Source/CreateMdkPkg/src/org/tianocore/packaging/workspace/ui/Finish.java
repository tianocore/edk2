/** @file
 
 The file is used to show a Finish page in the last step of setup
 
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
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;

import org.tianocore.packaging.common.ui.IFrame;

/**
 The class is used to show a Finish page in the last step of setup
 
 @since CreateMdkPkg 1.0

 **/
public class Finish extends IFrame implements ActionListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = 9055339173915836187L;

    //
    // Define class members
    //
    private JPanel jContentPane = null;

    private JTextArea jTextAreaTitle = null;

    private JTextArea jTextAreaContent = null;

    private JPanel jPanel = null;

    private JButton jButtonFinish = null;

    private JLabel jLabel = null;

    private JLabel jLabel1 = null;

    private JScrollPane jScrollPane = null;

    private JTextArea jTextAreaComment = null;

    private String strInstallDir = "";

    /**
     This method initializes jTextAreaTitle 
     
     @return javax.swing.JTextArea jTextAreaTitle
     
     **/
    private JTextArea getJTextAreaTitle() {
        if (jTextAreaTitle == null) {
            jTextAreaTitle = new JTextArea();
            jTextAreaTitle.setLocation(new java.awt.Point(0, 0));
            jTextAreaTitle.setText("     Click button \"Install\" to start installation");
            jTextAreaTitle.setFont(new java.awt.Font("Dialog", java.awt.Font.BOLD, 14));
            jTextAreaTitle.setEditable(false);
            jTextAreaTitle.setSize(new java.awt.Dimension(495, 20));
        }
        return jTextAreaTitle;
    }

    /**
     This method initializes jTextAreaContent 
     
     @return javax.swing.JTextArea jTextAreaContent
     
     **/
    private JTextArea getJTextAreaContent() {
        if (jTextAreaContent == null) {
            jTextAreaContent = new JTextArea();
            jTextAreaContent.setLocation(new java.awt.Point(0, 20));
            jTextAreaContent.setText("");
            jTextAreaContent.setEditable(false);
            jTextAreaContent.setSize(new java.awt.Dimension(495, 35));
        }
        return jTextAreaContent;
    }

    /**
     This method initializes jPanel 
     
     @return javax.swing.JPanel jPanel
     
     **/
    private JPanel getJPanel() {
        if (jPanel == null) {
            jLabel1 = new JLabel();
            jLabel1.setText("");
            jLabel1.setLocation(new java.awt.Point(30, 40));
            jLabel1.setSize(new java.awt.Dimension(435, 20));
            jLabel = new JLabel();
            jLabel.setText("");
            jLabel.setLocation(new java.awt.Point(30, 15));
            jLabel.setSize(new java.awt.Dimension(435, 20));
            jPanel = new JPanel();
            jPanel.setLayout(null);
            jPanel.setBorder(javax.swing.BorderFactory.createBevelBorder(javax.swing.border.BevelBorder.LOWERED));
            jPanel.setSize(new java.awt.Dimension(494, 251));
            jPanel.setLocation(new java.awt.Point(0, 55));
            jPanel.add(jLabel, null);
            jPanel.add(jLabel1, null);
            jPanel.add(getJScrollPane(), null);
        }
        return jPanel;
    }

    /**
     This method initializes jButtonFinish 
     
     @return javax.swing.JButton jButtonFinish
     
     **/
    private JButton getJButtonCancel() {
        if (jButtonFinish == null) {
            jButtonFinish = new JButton();
            jButtonFinish.setText("Finish");
            jButtonFinish.setBounds(new java.awt.Rectangle(360, 315, 90, 20));
            jButtonFinish.setEnabled(true);
            jButtonFinish.setSelected(false);
            jButtonFinish.setMnemonic('C');
            jButtonFinish.addActionListener(this);
        }
        return jButtonFinish;
    }

    /**
     This method initializes jScrollPane 
     
     @return javax.swing.JScrollPane jScrollPane
     
     **/
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setLocation(new java.awt.Point(30, 65));
            jScrollPane.setSize(new java.awt.Dimension(435, 180));
            jScrollPane.setViewportView(getJTextAreaComment());
        }
        return jScrollPane;
    }

    /**
     This method initializes jTextAreaComment 
     
     @return javax.swing.JTextArea jTextAreaComment
     
     **/
    private JTextArea getJTextAreaComment() {
        if (jTextAreaComment == null) {
            jTextAreaComment = new JTextArea();
            jTextAreaComment.setEditable(false);
            jTextAreaComment.setLineWrap(true);
            jTextAreaComment.setWrapStyleWord(false);
        }
        return jTextAreaComment;
    }

    /**
     Main class, used for test
     
     @param args
     **/
    public static void main(String[] args) {
        Finish f = new Finish();
        f.setVisible(true);
    }

    /**
     This is the override constructor
     
     @param InstallDir The install target dir
     
     **/
    public Finish(String InstallDir) {
        super();
        this.strInstallDir = InstallDir;
        init();
    }

    /**
     This is the default constructor
     
     **/
    public Finish() {
        super();
        init();
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 390);

        this.setContentPane(getJContentPane());
        this.setTitle("Setup - Installing");
        this.centerWindow();
        this.getRootPane().setDefaultButton(jButtonFinish);
        switchFinish();
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
            jContentPane.add(getJTextAreaTitle(), null);
            jContentPane.add(getJTextAreaContent(), null);
            jContentPane.add(getJPanel(), null);
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

        if (obj == jButtonFinish) {
            this.dispose();
            System.exit(0);
        }
    }

    /**
    Change all message values to Finish contents.
    
    **/
    private void switchFinish() {
        this.setTitle("Setup - Finish");
        jTextAreaTitle.setText("     Congratulations");
        jTextAreaContent.setText("           Your workspace was installed!");
        jLabel.setText("The MDK package was installed successfully");
        jLabel1.setText("Now you can start the trip with EFI");
        jTextAreaComment.setText("Please add \"WORKSPACE=" + this.strInstallDir
                                 + "\" into your system environment variable");
        jButtonFinish.setEnabled(true);
        this.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        jButtonFinish.setText("Finish");
    }

    //    private void switchInstall() {
    //        jTextAreaTitle.setText("     Installation is in process...");
    //        jLabel.setText("The MDK package was being installed...");
    //        jLabel1.setText("Just waiting for a second");
    //        jButtonFinish.setEnabled(false);
    //        jButtonFinish.setText("Finish");
    //    }
    
    /* (non-Javadoc)
     * @see java.awt.event.WindowListener#windowClosing(java.awt.event.WindowEvent)
     * 
     * Override windowClosing to exit directly
     * 
     */
    public void windowClosing(WindowEvent arg0) {
        this.dispose();
        System.exit(0);
    }
}
