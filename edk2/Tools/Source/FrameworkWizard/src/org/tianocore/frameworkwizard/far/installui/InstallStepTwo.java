/** @file

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.far.installui;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.io.File;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.swing.JPanel;
import javax.swing.JTextArea;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JScrollPane;
import javax.swing.ListSelectionModel;
import javax.swing.table.DefaultTableModel;
import javax.swing.JTable;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.far.Far;
import org.tianocore.frameworkwizard.far.FarStringDefinition;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;
import org.tianocore.frameworkwizard.workspace.Workspace;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

public class InstallStepTwo extends IDialog implements MouseListener {

    /**
     * 
     */
    private static final long serialVersionUID = 4583090421587036969L;

    private JPanel jContentPane = null;

    private JTextArea jTextArea = null;

    private PartialEditableTableModel packageModel = null;

    private PartialEditableTableModel platformModel = null;

    private InstallStepOne stepOne = null;

    private JButton jButtonCancel = null;

    private JButton jButtonFinish = null;

    private JButton jButtonPrevious = null;

    private JLabel jLabel = null;

    private JScrollPane jScrollPane = null;

    private JTable jTablePackage = null;

    private JLabel jLabel1 = null;

    private JScrollPane jScrollPane1 = null;

    private JTable jTablePlatform = null;

    List<PlatformIdentification> platformVector = null;

    List<PackageIdentification> packageVector = null;

    public InstallStepTwo(IDialog iDialog, boolean modal, InstallStepOne stepOne) {
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
            jTextArea.setText("Step 2: Set Install Path for Packages and/or Platforms.\n");
            jTextArea.setCaretColor(Color.RED);
            jTextArea.append("Note that the Install Path is Relative to WORKSPACE. ");
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
            jScrollPane.setBounds(new java.awt.Rectangle(30, 80, 642, 110));
            jScrollPane.setViewportView(getJTablePackage());
        }
        return jScrollPane;
    }

    /**
     * This method initializes jTable	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTablePackage() {
        if (jTablePackage == null) {
            jTablePackage = new JTable();
            packageModel = new PartialEditableTableModel();
            jTablePackage = new JTable(packageModel);
            jTablePackage.setRowHeight(20);
            jTablePackage.setAutoResizeMode(javax.swing.JTable.AUTO_RESIZE_SUBSEQUENT_COLUMNS);
            packageModel.addColumn("Name");
            packageModel.addColumn("Version");
            packageModel.addColumn("Default Path");
            packageModel.addColumn("Install To");

            jTablePackage.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        }
        return jTablePackage;
    }

    public void preparePackageTable() {
        packageModel.setRowCount(0);
        //
        // Change here to get packages and platforms from FAR
        //
        try {
            Far far = stepOne.getFar();

            packageVector = far.manifest.getPackageList();
            Iterator<PackageIdentification> iter = packageVector.iterator();
            while (iter.hasNext()) {
                String[] str = new String[4];
                PackageIdentification item = iter.next();
                str[0] = item.getName();
                str[1] = item.getVersion();
                str[2] = Tools.getFilePathOnly(Tools.getRelativePath(item.getPath(), Workspace.getCurrentWorkspace()));
                str[3] = Tools.getFilePathOnly(Tools.getRelativePath(item.getPath(), Workspace.getCurrentWorkspace()));
                packageModel.addRow(str);
            }
        } catch (Exception e) {
        }
    }

    /**
     * This method initializes jScrollPane1	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane1() {
        if (jScrollPane1 == null) {
            jScrollPane1 = new JScrollPane();
            jScrollPane1.setBounds(new java.awt.Rectangle(30, 215, 642, 110));
            jScrollPane1.setViewportView(getJTablePlatform());
        }
        return jScrollPane1;
    }

    /**
     * This method initializes jTablePlatform	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTablePlatform() {
        if (jTablePlatform == null) {
            jTablePlatform = new JTable();
            platformModel = new PartialEditableTableModel();
            jTablePlatform = new JTable(platformModel);
            jTablePlatform.setRowHeight(20);
            jTablePlatform.setAutoResizeMode(javax.swing.JTable.AUTO_RESIZE_SUBSEQUENT_COLUMNS);
            platformModel.addColumn("Name");
            platformModel.addColumn("Version");
            platformModel.addColumn("Default Path");
            platformModel.addColumn("Install To");

            jTablePlatform.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            preparePlatformTable();
        }
        return jTablePlatform;
    }

    public void preparePlatformTable() {
        platformModel.setRowCount(0);
        //
        // Change here to get packages and platforms from FAR
        //
        try {
            Far far = stepOne.getFar();

            platformVector = far.manifest.getPlatformList();
            Iterator<PlatformIdentification> iter = platformVector.iterator();
            while (iter.hasNext()) {
                String[] str = new String[4];
                PlatformIdentification item = iter.next();
                str[0] = item.getName();
                str[1] = item.getVersion();
                str[2] = Tools.getFilePathOnly(Tools.getRelativePath(item.getPath(), Workspace.getCurrentWorkspace()));
                str[3] = Tools.getFilePathOnly(Tools.getRelativePath(item.getPath(), Workspace.getCurrentWorkspace()));
                platformModel.addRow(str);
            }
        } catch (Exception e) {
        }
    }

    /**
     * This is the default constructor
     */
    public InstallStepTwo(IDialog iDialog, boolean modal) {
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
        this.setTitle(FarStringDefinition.INSTALL_STEP_TWO_TITLE);
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
            jLabel1.setBounds(new java.awt.Rectangle(30, 195, 348, 18));
            jLabel1.setText("Edit \"Install To\" paths for platforms: ");
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(29, 60, 366, 20));
            jLabel.setText("Edit \"Install To\" paths for packages");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJTextArea(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonFinish(), null);
            jContentPane.add(getJButtonPrevious(), null);
            jContentPane.add(jLabel, null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(jLabel1, null);
            jContentPane.add(getJScrollPane1(), null);
        }
        return jContentPane;
    }

    public void mouseClicked(MouseEvent e) {
        if (e.getSource() == jButtonCancel) {
            this.setVisible(false);
            this.dispose();
        } else if (e.getSource() == jButtonFinish) {

            if (jTablePackage.isEditing()) {
                jTablePackage.getCellEditor().stopCellEditing();
            }

            if (jTablePlatform.isEditing()) {
                jTablePlatform.getCellEditor().stopCellEditing();
            }

            List<String> packageList = new ArrayList<String>();
            List<String> platformList = new ArrayList<String>();
            //
            // Add some logic process here
            // Guid Check, File Check etc.
            //
            Set<File> allNewPath = new LinkedHashSet<File>();
            Map<PackageIdentification, File> packageMap = new LinkedHashMap<PackageIdentification, File>();
            for (int i = 0; i < packageModel.getRowCount(); i++) {
                File toFile = new File(Workspace.getCurrentWorkspace() + File.separatorChar
                                       + packageModel.getValueAt(i, 3));
                if (!isPackagePathValid(toFile)) {
                    Log.wrn("Install far", packageVector.get(i) + " path already contains a package.");
                    return;
                }
                if (allNewPath.contains(toFile)) {
                    Log.wrn("Install far", "Path " + packageModel.getValueAt(i, 3) + " is specified twice.");
                    return;
                }
                allNewPath.add(toFile);
                File spdFile = new File((String) packageModel.getValueAt(i, 3) + File.separatorChar
                                        + packageVector.get(i).getSpdFile().getName());
                packageList.add(spdFile.getPath());
                packageMap.put(packageVector.get(i), toFile);
            }

            Map<PlatformIdentification, File> platformMap = new LinkedHashMap<PlatformIdentification, File>();
            for (int i = 0; i < platformModel.getRowCount(); i++) {
                File toFile = new File(Workspace.getCurrentWorkspace() + File.separatorChar
                                       + platformModel.getValueAt(i, 3));
                if (!isPlatformPathValid(toFile)) {
                    Log.wrn("Install far", platformVector.get(i) + " path already contains a platform.");
                    return;
                }
                File fpdFile = new File((String) platformModel.getValueAt(i, 3) + File.separatorChar
                                        + platformVector.get(i).getFpdFile().getName());
                platformList.add(fpdFile.getPath());
                platformMap.put(platformVector.get(i), toFile);
            }

            //
            //
            //
            Far far = stepOne.getFar();
            try {
                far.InstallFar(platformMap, packageMap);
                //
                // Add to database
                //
                WorkspaceTools wt = new WorkspaceTools();
                wt.addFarToDb(packageList, platformList, far.manifest.getHeader());
            } catch (Exception ex) {
                Log.wrn("Install far", ex.getMessage());
                Log.err("Install far", ex.getMessage());
                return;
            }

            this.setVisible(false);
            this.stepOne.returnType = DataType.RETURN_TYPE_OK;
            this.dispose();
        } else if (e.getSource() == jButtonPrevious) {
            this.setVisible(false);
            stepOne.setVisible(true);
        }
    }

    private boolean isPackagePathValid(File spdFile) {
        WorkspaceTools wt = new WorkspaceTools();
        List<PackageIdentification> allPackages = wt.getAllPackages();
        Iterator<PackageIdentification> iter = allPackages.iterator();

        while (iter.hasNext()) {
            PackageIdentification item = iter.next();
            if (isPathContainMutual(spdFile, item.getSpdFile())) {
                return false;
            }
        }
        return true;
    }

    private boolean isPlatformPathValid(File fpdFile) {
        WorkspaceTools wt = new WorkspaceTools();
        List<PlatformIdentification> allPlatforms = wt.getAllPlatforms();
        Iterator<PlatformIdentification> iter = allPlatforms.iterator();

        while (iter.hasNext()) {
            PlatformIdentification item = iter.next();
            if (isPathContainMutual(fpdFile, item.getFpdFile())) {
                return false;
            }
        }
        return true;
    }

    private boolean isPathContainMutual(File path1, File path2) {
        String s1 = Tools.addFileSeparator(path1.getPath());
        String s2 = Tools.addFileSeparator(path2.getParent());

        if (s1.length() > s2.length()) {
            if (s1.substring(0, s2.length()).equalsIgnoreCase(s2)) {
                return true;
            }
        } else {
            if (s2.substring(0, s1.length()).equalsIgnoreCase(s1)) {
                return true;
            }
        }
        return false;
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

}

class PartialEditableTableModel extends DefaultTableModel {
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    public boolean isCellEditable(int row, int col) {
        switch (col) {
        case 3:
            return true;
        default:
            return false;
        }
    }
}
