/** @file

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.far.deleteui;

import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.io.File;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.Set;
import java.util.Vector;

import javax.swing.ButtonGroup;
import javax.swing.JPanel;
import javax.swing.JButton;
import javax.swing.JTextArea;
import javax.swing.JLabel;
import javax.swing.JRadioButton;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.far.FarIdentification;
import org.tianocore.frameworkwizard.far.FarStringDefinition;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

public class DeleteStepTwo extends IDialog implements MouseListener {

    /**
     * 
     */
    private static final long serialVersionUID = -1333748185798962746L;

    private JPanel jContentPane = null;

    private JButton jButtonCancel = null;

    private JButton jButtonFinish = null;

    private JButton jButtonPrevious = null;

    private JTextArea jTextArea = null;

    private JLabel jLabel = null;

    private JRadioButton jRadioButton = null;

    private JRadioButton jRadioButton1 = null;

    private DeleteStepOne stepOne = null;

    public DeleteStepTwo(IDialog iDialog, boolean modal, DeleteStepOne stepOne) {
        this(iDialog, modal);
        this.stepOne = stepOne;
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
     * This method initializes jButtonFinish	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFinish() {
        if (jButtonFinish == null) {
            jButtonFinish = new JButton();
            jButtonFinish.setBounds(new java.awt.Rectangle(470, 330, 90, 20));
            jButtonFinish.setText("Finish");
            jButtonFinish.addMouseListener(this);
        }
        return jButtonFinish;
    }

    /**
     * This method initializes jButtonPrevious	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonPrevious() {
        if (jButtonPrevious == null) {
            jButtonPrevious = new JButton();
            jButtonPrevious.setBounds(new java.awt.Rectangle(370, 330, 90, 20));
            jButtonPrevious.setText("Previous");
            jButtonPrevious.addMouseListener(this);
        }
        return jButtonPrevious;
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
            jTextArea.setText("Step 2: Choose Delete Mode. \n");
            jTextArea.append("Mode 1 Only remove registation information from the WORKSPACE. \n");
            jTextArea.append("Mode 2 Also delete all files and directories from file system. ");
            jTextArea.setEditable(false);
        }
        return jTextArea;
    }

    /**
     * This method initializes jRadioButton	
     * 	
     * @return javax.swing.JRadioButton	
     */
    private JRadioButton getJRadioButton() {
        if (jRadioButton == null) {
            jRadioButton = new JRadioButton();
            jRadioButton.setBounds(new java.awt.Rectangle(40,100,440,20));
            jRadioButton.setSelected(true);
            jRadioButton.setText("Mode 1: Only remove registration information from the WORKSPACE.");
        }
        return jRadioButton;
    }

    /**
     * This method initializes jRadioButton1	
     * 	
     * @return javax.swing.JRadioButton	
     */
    private JRadioButton getJRadioButton1() {
        if (jRadioButton1 == null) {
            jRadioButton1 = new JRadioButton();
            jRadioButton1.setBounds(new java.awt.Rectangle(40,140,440,20));
            jRadioButton1.setText("Mode 2: Delete ALL related files and directories from the WORKSPACE.");
        }
        return jRadioButton1;
    }

    /**
     * This is the default constructor
     */
    public DeleteStepTwo(IDialog iDialog, boolean modal) {
        super(iDialog, modal);
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
        this.setTitle(FarStringDefinition.DELETE_STEP_TWO_TITLE);
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
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(30, 70, 200, 20));
            jLabel.setText("Select delete mode: ");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonFinish(), null);
            jContentPane.add(getJButtonPrevious(), null);
            jContentPane.add(getJTextArea(), null);
            jContentPane.add(jLabel, null);
            ButtonGroup group = new ButtonGroup();
            group.add(getJRadioButton());
            group.add(getJRadioButton1());
            jContentPane.add(getJRadioButton(), null);
            jContentPane.add(getJRadioButton1(), null);
        }
        return jContentPane;
    }

    public void mouseClicked(MouseEvent e) {
        if (e.getSource() == jButtonCancel) {
            this.setVisible(false);
            this.dispose();
        } else if (e.getSource() == jButtonFinish) {
            FarIdentification far = stepOne.getSelecedFar();
            WorkspaceTools wt = new WorkspaceTools();
            //
            // If remove all source files
            //
            if (jRadioButton1.isSelected()) {

                Vector<PackageIdentification> removePackages = wt.getPackagesByFar(far);
                Vector<PlatformIdentification> removePlatforms = wt.getPlatformsByFar(far);

                Vector<PlatformIdentification> allPlatforms = wt.getAllPlatforms();
                Set<File> allPlatformFiles = new LinkedHashSet<File>();

                Iterator<PlatformIdentification> iter = allPlatforms.iterator();
                while (iter.hasNext()) {
                    allPlatformFiles.add(iter.next().getFpdFile());
                }

                //
                // For all platforms, only remove its FPD file
                //
                Iterator<PlatformIdentification> platfomrIter = removePlatforms.iterator();
                while (platfomrIter.hasNext()) {
                    PlatformIdentification item = platfomrIter.next();
                    allPlatformFiles.remove(item.getFpdFile());
                    File parentDir = item.getFpdFile().getParentFile();
                    item.getFpdFile().delete();
                    //
                    // Remove all empty parent dir
                    //
                    while (parentDir.listFiles().length == 0) {
                        File tempFile = parentDir;
                        parentDir = parentDir.getParentFile();
                        tempFile.delete();
                    }
                }

                //
                // For all packages, remove all files. 
                // Exception FPD file still in DB
                //

                Iterator<PackageIdentification> packageIter = removePackages.iterator();
                while (packageIter.hasNext()) {
                    PackageIdentification item = packageIter.next();
                    Set<File> deleteFiles = new LinkedHashSet<File>();
                    recursiveDir(deleteFiles, item.getSpdFile().getParentFile(), allPlatformFiles);
                    Iterator<File> iterDeleteFile = deleteFiles.iterator();
                    while (iterDeleteFile.hasNext()) {
                        deleteFiles(iterDeleteFile.next());
                    }
                    //
                    // Remove all empty parent dir
                    //
                    File parentDir = item.getSpdFile().getParentFile();
                    while (parentDir.listFiles().length == 0) {
                        File tempFile = parentDir;
                        parentDir = parentDir.getParentFile();
                        tempFile.delete();
                    }
                }
            }

            //
            // Update DB file
            //
            wt.removeFarFromDb(far);

            this.setVisible(false);
            this.stepOne.returnType = DataType.RETURN_TYPE_OK;
            this.dispose();
        } else if (e.getSource() == jButtonPrevious) {
            this.setVisible(false);
            stepOne.setVisible(true);
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

    private void recursiveDir(Set<File> files, File dir, Set<File> platformFiles) {
        File[] fileList = dir.listFiles();
        for (int i = 0; i < fileList.length; i++) {
            if (fileList[i].isFile()) {
                if (!platformFiles.contains(fileList[i])) {
                    files.add(fileList[i]);
                }
            } else {
                if (isContain(fileList[i], platformFiles)) {
                    recursiveDir(files, fileList[i], platformFiles);
                } else {
                    files.add(fileList[i]);
                }
            }
        }
    }

    private void deleteFiles(File file) {
        if (file.isDirectory()) {
            File[] files = file.listFiles();
            for (int i = 0; i < files.length; i++) {
                deleteFiles(files[i]);
            }
        }
        file.delete();
    }

    private boolean isContain(File dir, Set<File> platformFiles) {
        Iterator<File> iter = platformFiles.iterator();
        while (iter.hasNext()) {
            File file = iter.next();
            if (file.getPath().startsWith(dir.getPath())) {
                //
                // continue this FPD file
                //
                return true;
            }
        }
        return false;
    }
}
