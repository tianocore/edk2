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
import java.util.Iterator;
import java.util.List;

import javax.swing.JPanel;
import javax.swing.JTextArea;
import javax.swing.JButton;
import javax.swing.JScrollPane;
import javax.swing.JLabel;
import javax.swing.JTable;
import javax.swing.ListSelectionModel;
import javax.swing.table.DefaultTableModel;

import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.far.AggregationOperation;
import org.tianocore.frameworkwizard.far.Far;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
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
            jScrollPane.setBounds(new java.awt.Rectangle(30, 98, 570, 170));
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
            List<PackageIdentification> packagesInFar = far.mainfest.getPackageList();

            WorkspaceTools wt = new WorkspaceTools();
            List<PackageIdentification> packagesInDb = wt.getAllPackages();

            List<PackageIdentification> result = AggregationOperation.intersection(packagesInDb, packagesInFar);
            //
            // Change here to get packages and platforms from FAR
            //
            Iterator<PackageIdentification> iter = result.iterator();//packageList.iterator();
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
        this.setTitle("Update Framework Archive(FAR) - Step 2: Summary");
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

            //
            // Remove all update packages
            //

            //
            // Install all update packages
            //

            this.setVisible(false);
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
