/** @file

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.far.updateui;

import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.io.File;
import java.util.Vector;
import java.util.jar.JarFile;

import javax.swing.JFileChooser;
import javax.swing.JPanel;
import javax.swing.JTextArea;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JTextField;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.IFileFilter;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.far.Far;
import org.tianocore.frameworkwizard.far.FarIdentification;
import org.tianocore.frameworkwizard.far.FarStringDefinition;
import org.tianocore.frameworkwizard.workspace.Workspace;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

import javax.swing.JScrollPane;
import javax.swing.JList;

public class UpdateStepOne extends IDialog implements MouseListener {

    /**
     * 
     */
    private static final long serialVersionUID = 735554907464539931L;

    private JPanel jContentPane = null;

    private JTextArea jTextArea = null;

    private JButton jButtonCancel = null;

    private JButton jButtonNext = null;

    private JLabel jLabel = null;

    private JTextField jTextFieldFarFile = null;

    private JButton jButtonBrowser = null;

    private UpdateStepTwo stepTwo = null;

    private Far far = null;

    private Vector<FarIdentification> farVector = null;

    private JLabel jLabel1 = null;

    private JScrollPane jScrollPane = null;

    private JList jListFarFromDb = null;

    private File farFile = null;

    public File getFarFile() {
        return farFile;
    }

    /**
     * This method initializes jTextArea	
     * 	
     * @return javax.swing.JTextArea	
     */
    private JTextArea getJTextArea() {
        if (jTextArea == null) {
            jTextArea = new JTextArea();
            jTextArea.setBounds(new java.awt.Rectangle(30, 7, 642, 50));
            jTextArea.setText("Step 1: Choose framework archive (FAR) file. \n");
            jTextArea.setEditable(false);
        }
        return jTextArea;
    }

    /**
     * This method initializes jButtonCancel	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setBounds(new java.awt.Rectangle(570, 330, 90, 20));
            jButtonCancel.setText("Cancel");
            jButtonCancel.addMouseListener(this);
        }
        return jButtonCancel;
    }

    /**
     * This method initializes jButtonNext	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonNext() {
        if (jButtonNext == null) {
            jButtonNext = new JButton();
            jButtonNext.setBounds(new java.awt.Rectangle(470, 330, 90, 20));
            jButtonNext.setText("Next");
            jButtonNext.addMouseListener(this);
        }
        return jButtonNext;
    }

    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldFarFile() {
        if (jTextFieldFarFile == null) {
            jTextFieldFarFile = new JTextField();
            jTextFieldFarFile.setBounds(new java.awt.Rectangle(130, 80, 436, 20));
        }
        return jTextFieldFarFile;
    }

    /**
     * This method initializes jButton	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonBrowser() {
        if (jButtonBrowser == null) {
            jButtonBrowser = new JButton();
            jButtonBrowser.setBounds(new java.awt.Rectangle(570, 80, 100, 20));
            jButtonBrowser.setText("Browser...");
            jButtonBrowser.addMouseListener(this);
        }
        return jButtonBrowser;
    }

    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setBounds(new java.awt.Rectangle(30, 135, 642, 160));
            jScrollPane.setViewportView(getJListFarFromDb());
        }
        return jScrollPane;
    }

    /**
     * This method initializes jListFarFromDb	
     * 	
     * @return javax.swing.JList	
     */
    private JList getJListFarFromDb() {
        if (jListFarFromDb == null) {
            jListFarFromDb = new JList();
            WorkspaceTools wt = new WorkspaceTools();
            farVector = wt.getAllFars();
            jListFarFromDb.setListData(farVector);
            jListFarFromDb.setSelectionMode(0);
        }
        return jListFarFromDb;
    }

    /**
     * This is the default constructor
     */
    public UpdateStepOne(IFrame iFrame, boolean modal) {
        super(iFrame, modal);
        initialize();
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
    private void initialize() {
        this.setSize(700, 400);
        this.setContentPane(getJContentPane());
        this.setTitle(FarStringDefinition.UPDATE_STEP_ONE_TITLE);
        Dimension d = Toolkit.getDefaultToolkit().getScreenSize();
        this.setLocation((d.width - this.getSize().width) / 2, (d.height - this.getSize().height) / 2);
    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabel1 = new JLabel();
            jLabel1.setBounds(new java.awt.Rectangle(30, 110, 355, 18));
            jLabel1.setText("Choose FAR from current WORKSPACE.");
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(30, 80, 97, 20));
            jLabel.setText("Choose FAR file: ");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJTextArea(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonNext(), null);
            jContentPane.add(jLabel, null);
            jContentPane.add(getJTextFieldFarFile(), null);
            jContentPane.add(getJButtonBrowser(), null);
            jContentPane.add(jLabel1, null);
            jContentPane.add(getJScrollPane(), null);
        }
        return jContentPane;
    }

    public void mouseClicked(MouseEvent e) {
        if (e.getSource() == jButtonCancel) {
            this.setVisible(false);
        } else if (e.getSource() == jButtonNext) {
            //
            // Judge if FAR file is existed
            //
            farFile = new File(jTextFieldFarFile.getText());
            if (!farFile.exists() || !farFile.isFile()) {
                Log.wrn("Update far", "Please choose a FAR file that already exists.");
                return;
            }

            //
            // Judge FAR is valid
            //
            try {
                JarFile file = new JarFile(farFile);
                this.far = new Far(file);
            } catch (Exception ex) {
                Log.wrn("Update far", ex.getMessage());
                Log.err("Update far", ex.getMessage());
            }

            //
            // Add more logic process here
            //
            if (jListFarFromDb.getSelectedValue() == null) {
                Log.wrn("Update far", "Please choose a FAR from current WORKSPACE.");
                return;
            }

            if (stepTwo == null) {
                stepTwo = new UpdateStepTwo(this, true, this);
            }
            this.setVisible(false);
            stepTwo.prepareTable();
            stepTwo.setVisible(true);
        } else if (e.getSource() == jButtonBrowser) {
            JFileChooser fc = new JFileChooser();
            fc.setAcceptAllFileFilterUsed(false);
            fc.addChoosableFileFilter(new IFileFilter(DataType.FAR_SURFACE_AREA_EXT));
            fc.setCurrentDirectory(new File(Workspace.getCurrentWorkspace()));

            int result = fc.showOpenDialog(new JPanel());
            if (result == JFileChooser.APPROVE_OPTION) {
                this.jTextFieldFarFile.setText(Tools.addPathExt(fc.getSelectedFile().getPath(),
                                                                DataType.RETURN_TYPE_FAR_SURFACE_AREA));
            }
        }
    }

    public void mousePressed(MouseEvent e) {
        // TODO Auto-generated method stub

    }

    public void mouseReleased(MouseEvent e) {
        // TODO Auto-generated method stub

    }

    public void mouseEntered(MouseEvent e) {
        // TODO Auto-generated method stub

    }

    public void mouseExited(MouseEvent e) {
        // TODO Auto-generated method stub

    }

    public FarIdentification getSelecedDbFar() {
        return (FarIdentification) jListFarFromDb.getSelectedValue();
    }

    public Far getFar() {
        return far;
    }
}
