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
import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.Vector;

import javax.swing.JPanel;
import javax.swing.JTextArea;
import javax.swing.JButton;
import javax.swing.JScrollPane;
import javax.swing.JLabel;
import javax.swing.JTable;
import javax.swing.ListSelectionModel;
import javax.swing.table.DefaultTableModel;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.far.AggregationOperation;
import org.tianocore.frameworkwizard.far.DistributeRule;
import org.tianocore.frameworkwizard.far.Far;
import org.tianocore.frameworkwizard.far.FarStringDefinition;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;
import org.tianocore.frameworkwizard.workspace.Workspace;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

public class UpdateStepTwo extends IDialog implements MouseListener {

    /**
     * 
     */
    private static final long serialVersionUID = -4400145363721213110L;

    private JPanel jContentPane = null;

    private JTextArea jTextArea = null;

    private UpdateStepOne stepOne = null;

    private JButton jButtonCancel = null;

    private JButton jButtonFinish = null;

    private JButton jButtonPrevious = null;

    private JScrollPane jScrollPane = null;

    private JLabel jLabel = null;

    private JTable jTablePackage = null;

    private PartialTableModel model = null;

    List<PackageIdentification> updatPkgList = new ArrayList<PackageIdentification>();

    public UpdateStepTwo(IDialog iDialog, boolean modal, UpdateStepOne stepOne) {
        this(iDialog, modal);
        this.stepOne = stepOne;
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
            jTextArea.setText("Step 2: Summary. \n");
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
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setBounds(new java.awt.Rectangle(30, 100, 642, 170));
            jScrollPane.setViewportView(getJTablePackage());
        }
        return jScrollPane;
    }

    /**
     * This method initializes jTablePackage	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTablePackage() {
        if (jTablePackage == null) {
            jTablePackage = new JTable();
            model = new PartialTableModel();
            jTablePackage = new JTable(model);
            jTablePackage.setRowHeight(20);
            jTablePackage.setAutoResizeMode(javax.swing.JTable.AUTO_RESIZE_SUBSEQUENT_COLUMNS);
            model.addColumn("Name");
            model.addColumn("Version");
            model.addColumn("Guid");
            model.addColumn("Path");

            jTablePackage.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        }
        return jTablePackage;
    }

    public void prepareTable() {
        model.setRowCount(0);
        try {
            Far far = stepOne.getFar();
            List<PackageIdentification> packagesInFar = far.manifest.getPackageList();

            WorkspaceTools wt = new WorkspaceTools();
            List<PackageIdentification> packagesInDb = wt.getAllPackages();

            updatPkgList = AggregationOperation.intersection(packagesInDb, packagesInFar);
            //
            // Change here to get packages and platforms from FAR
            //
            Iterator<PackageIdentification> iter = updatPkgList.iterator();//packageList.iterator();
            while (iter.hasNext()) {
                String[] str = new String[4];
                PackageIdentification item = iter.next();
                str[0] = item.getName();
                str[1] = item.getVersion();
                str[2] = item.getGuid();
                str[3] = Tools.getFilePathOnly(Tools.getRelativePath(item.getPath(), Workspace.getCurrentWorkspace()));
                model.addRow(str);
            }
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    /**
     * This is the default constructor
     */
    public UpdateStepTwo(IDialog iDialog, boolean modal) {
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
        this.setTitle(FarStringDefinition.UPDATE_STEP_TWO_TITLE);
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
            jLabel.setBounds(new java.awt.Rectangle(30, 70, 281, 20));
            jLabel.setText("Following packages will be updated: ");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJTextArea(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonFinish(), null);
            jContentPane.add(getJButtonPrevious(), null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(jLabel, null);
        }
        return jContentPane;
    }

    public void mouseClicked(MouseEvent e) {
        if (e.getSource() == jButtonCancel) {
            this.setVisible(false);
        } else if (e.getSource() == jButtonFinish) {
            //
            // Check depedency ?
            //
            WorkspaceTools wsTools = new WorkspaceTools();

            Iterator<PackageIdentification> iter = updatPkgList.iterator();
            List<PackageIdentification> depResultList = new ArrayList<PackageIdentification>();
            while (iter.hasNext()) {
                List<PackageIdentification> depPkgList = stepOne.getFar().getPackageDependencies(iter.next());
                depResultList = AggregationOperation.union(depResultList, depPkgList);
            }

            List<PackageIdentification> dbPkgList = DistributeRule.vectorToList(wsTools.getAllPackages());
            List<PackageIdentification> resultList = AggregationOperation
                                                                         .minus(
                                                                                depResultList,
                                                                                AggregationOperation
                                                                                                    .union(
                                                                                                           this.updatPkgList,
                                                                                                           dbPkgList));
            Iterator resultIter = resultList.iterator();
            while (resultIter.hasNext()) {
                Log.wrn("Update far", "Missing dependency package " + ((PackageIdentification) resultIter.next()).toString()
                        + " in workspace!");
                return;
            }

            //
            // Remove all update packages
            //
            //
            // For all packages, remove all files. 
            // Exception FPD file still in DB
            //
            Vector<PlatformIdentification> allPlatforms = wsTools.getAllPlatforms();
            Set<File> allPlatformFiles = new LinkedHashSet<File>();

            Iterator<PlatformIdentification> allPlfIter = allPlatforms.iterator();
            while (iter.hasNext()) {
                allPlatformFiles.add(allPlfIter.next().getFpdFile());
            }

            Iterator<PackageIdentification> packageIter = this.updatPkgList.iterator();
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

            //
            // Install all update packages
            //
            Iterator<PackageIdentification> updataIter = this.updatPkgList.iterator();
            while (updataIter.hasNext()) {
                PackageIdentification pkgId = updataIter.next();
                try {
                    stepOne.getFar().installPackage(pkgId, new File(pkgId.getSpdFile().getParent()));
                } catch (Exception ex) {
                    Log.wrn("Install " + pkgId.toString(), ex.getMessage());
                    Log.err("Install " + pkgId.toString(), ex.getMessage());
                }

            }
            this.stepOne.returnType = DataType.RETURN_TYPE_OK;

            this.setVisible(false);
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

class PartialTableModel extends DefaultTableModel {
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    public boolean isCellEditable(int row, int col) {
        switch (col) {
        case 3:
            return false;
        default:
            return false;
        }
    }
}
