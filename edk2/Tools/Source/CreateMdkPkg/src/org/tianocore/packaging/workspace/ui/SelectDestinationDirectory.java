/** @file
 
 The file is used to show a Select Destination Directory page in
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
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JTextArea;
import javax.swing.JTextField;

import org.tianocore.common.Log;
import org.tianocore.packaging.common.ui.IFrame;
import org.tianocore.packaging.workspace.command.InstallWorkspace;

/**
 The class is used to show a Select Destination Directory page in
 the process of setup
 
 @since CreateMdkPkg 1.0

 **/
public class SelectDestinationDirectory extends IFrame implements ActionListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -2924500118774744205L;

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

    private JLabel jLabel1 = null;

    private JTextField jTextFieldInstallDir = null;

    private JButton jButtonBrowse = null;

    private JLabel jLabel2 = null;

    private LicenseAgreement la = null;

    /**
     This method initializes jTextArea 
     
     @return javax.swing.JTextArea jTextArea
     
     **/
    private JTextArea getJTextArea() {
        if (jTextArea == null) {
            jTextArea = new JTextArea();
            jTextArea.setLocation(new java.awt.Point(0, 0));
            jTextArea.setText("    Select Destination Directory");
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
            jTextArea1.setText("           Where should MDK package be installed?");
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
            jLabel2 = new JLabel();
            jLabel2.setText("At least 10 MB of free disk space is required");
            jLabel2.setLocation(new java.awt.Point(30, 225));
            jLabel2.setSize(new java.awt.Dimension(290, 20));
            jLabel1 = new JLabel();
            jLabel1.setText("To continue, click Next. If you wuold like to select different folder, click Browse.");
            jLabel1.setLocation(new java.awt.Point(30, 55));
            jLabel1.setSize(new java.awt.Dimension(435, 20));
            jLabel = new JLabel();
            jLabel.setText("Setup will install MDK package into the following folders:");
            jLabel.setLocation(new java.awt.Point(30, 15));
            jLabel.setSize(new java.awt.Dimension(435, 20));
            jPanel = new JPanel();
            jPanel.setLayout(null);
            jPanel.setBorder(javax.swing.BorderFactory.createBevelBorder(javax.swing.border.BevelBorder.LOWERED));
            jPanel.setSize(new java.awt.Dimension(494, 251));
            jPanel.setLocation(new java.awt.Point(0, 55));
            jPanel.add(jLabel, null);
            jPanel.add(jLabel1, null);
            jPanel.add(getJTextField(), null);
            jPanel.add(getJButtonBrowse(), null);
            jPanel.add(jLabel2, null);
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
            jButtonNext.setEnabled(true);
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
     This method initializes jTextFieldInstallDir 
     
     @return javax.swing.JTextField jTextFieldInstallDir
     
     **/
    private JTextField getJTextField() {
        if (jTextFieldInstallDir == null) {
            jTextFieldInstallDir = new JTextField();
            jTextFieldInstallDir.setLocation(new java.awt.Point(30, 90));
            jTextFieldInstallDir.setSize(new java.awt.Dimension(320, 20));
            jTextFieldInstallDir.setText("C:\\MyWorkspace");
        }
        return jTextFieldInstallDir;
    }

    /**
     This method initializes jButtonBrowse 
     
     @return javax.swing.JButton jButtonBrowse
     
     **/
    private JButton getJButtonBrowse() {
        if (jButtonBrowse == null) {
            jButtonBrowse = new JButton();
            jButtonBrowse.setText("Browse");
            jButtonBrowse.setSize(new java.awt.Dimension(90, 20));
            jButtonBrowse.setLocation(new java.awt.Point(370, 90));
            jButtonBrowse.addActionListener(this);
        }
        return jButtonBrowse;
    }

    /**
     Main class, used for test
     
     @param args
     
     **/
    public static void main(String[] args) {
        SelectDestinationDirectory sdd = new SelectDestinationDirectory();
        sdd.setVisible(true);
    }

    /**
     This is the default constructor
     
     **/
    public SelectDestinationDirectory() {
        super();
        init();
    }

    /**
     This is the override constructor
     
     @param licenseagreement The input data of licenseagreement
     
     **/
    public SelectDestinationDirectory(LicenseAgreement licenseagreement) {
        super();
        init();
        la = licenseagreement;
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(500, 390);
        this.setTitle("Setup - Select Destination Directory");
        this.setContentPane(getJContentPane());
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

        if (obj == jButtonBack) {
            this.setVisible(false);
            la.setVisible(true);
        }

        //
        // Show next page if click button Next
        //
        if (obj == jButtonNext) {
            if (createWorkspace(jTextFieldInstallDir.getText())) {
                if (initWorkspace(jTextFieldInstallDir.getText())) {
                    this.setVisible(false);
                    Finish f = new Finish(jTextFieldInstallDir.getText());
                    f.setVisible(true);
                }
            }
        }

        if (obj == jButtonCancel) {
            this.onExit();
        }

        if (obj == jButtonBrowse) {
            JFileChooser fc = new JFileChooser();
            fc.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
            int result = fc.showOpenDialog(new JPanel());
            if (result == JFileChooser.APPROVE_OPTION) {
                jTextFieldInstallDir.setText(fc.getCurrentDirectory().toString() + System.getProperty("file.separator")
                                             + fc.getSelectedFile().getName());
            }
        }
    }

    /**
     Create workspace to target dir
     
     @param strInstallDir The install target dir
     @retval true - Create success
     @retval false - Create fail
     
     **/
    private boolean createWorkspace(String strInstallDir) {
        boolean bolCreateDirectory = true;
        int intResult;

        //
        //Check if the Install Dir exists
        //
        Log.log("is Exist Install Dir");
        if (InstallWorkspace.isExistInstallDir(strInstallDir)) {
            intResult = JOptionPane.showConfirmDialog(null, strInstallDir + " already exists, continue anyway?",
                                                      "Override", JOptionPane.YES_NO_OPTION);
            if (intResult != JOptionPane.YES_OPTION) {
                return false;
            } else {
                bolCreateDirectory = false;
            }
        }

        //
        //Create the directory
        //
        Log.log("Create Directory");
        if (bolCreateDirectory) {
            if (!InstallWorkspace.createInstallDir(strInstallDir)) {
                intResult = JOptionPane.showConfirmDialog(null, "Cannot create direcotry " + strInstallDir
                                                                + " in system. Click OK to exist.", "Error",
                                                          JOptionPane.DEFAULT_OPTION, JOptionPane.WARNING_MESSAGE);
                return false;
            }
        }
        return true;
    }

    /**
     Init created workspace
     
     @param strInstallDir The dir of workspace
     @retval true - Init Success
     @retval false - Init fail
     
     **/
    private boolean initWorkspace(String strInstallDir) {
        String strJarFile = System.getProperty("user.dir") + System.getProperty("file.separator") + "CreateMdkPkg.jar";

        //
        //Install package
        //
        Log.log("Install Package");
        try {
            if (!InstallWorkspace.installPackage(strInstallDir, strJarFile)) {
                JOptionPane.showConfirmDialog(null, "Cannot intall package in system. Click OK to exist.", "Error",
                                              JOptionPane.DEFAULT_OPTION, JOptionPane.WARNING_MESSAGE);
                return false;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        //
        //Update framework database
        //
        Log.log("Set Framework Database");
        if (!InstallWorkspace.setFrameworkDatabase()) {
            JOptionPane.showConfirmDialog(null, "Cannot create workspace database in system. Click OK to exist.",
                                          "Error", JOptionPane.DEFAULT_OPTION, JOptionPane.WARNING_MESSAGE);
            return false;
        }

        //
        //Set System Environment
        //
        Log.log("Set System Environment");
        if (!InstallWorkspace.setSystemEnvironment()) {
            JOptionPane.showConfirmDialog(null, "Cannot set WORKSPACE variable in system. Click OK to exist.", "Error",
                                          JOptionPane.DEFAULT_OPTION, JOptionPane.WARNING_MESSAGE);
            return false;
        }

        //
        //Set Tool Chain Path
        //
        Log.log("Set Tool Chain Path");
        if (!InstallWorkspace.setToolChainPath()) {
            JOptionPane.showConfirmDialog(null, "Cannot set Tool Chain path variable in system. Click OK to exist.",
                                          "Error", JOptionPane.DEFAULT_OPTION, JOptionPane.WARNING_MESSAGE);
            return false;
        }

        //
        //Install tool chain
        //
        Log.log("Set Tool Chain");
        if (!InstallWorkspace.setToolChain()) {
            JOptionPane.showConfirmDialog(null, "Cannot set Tool Chain in system. Click OK to exist.", "Error",
                                          JOptionPane.DEFAULT_OPTION, JOptionPane.WARNING_MESSAGE);
            return false;
        }

        //
        //Delete setup files
        //
        Log.log("Delete Setup Files");
        try {
            InstallWorkspace.delSetupPackage(strInstallDir + System.getProperty("file.separator") + "org");
        } catch (Exception e) {
            e.printStackTrace();
            Log.log(e.getMessage());
        }

        return true;
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
