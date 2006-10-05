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

import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.io.File;
import java.util.List;
import java.util.jar.JarFile;
import java.util.Iterator;

import javax.swing.JFileChooser;
import javax.swing.JPanel;
import javax.swing.JButton;
import javax.swing.JTextArea;
import javax.swing.JLabel;
import javax.swing.JTextField;
import javax.swing.ListSelectionModel;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.IFileFilter;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.far.DistributeRule;
import org.tianocore.frameworkwizard.far.Far;
import org.tianocore.frameworkwizard.far.FarStringDefinition;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.workspace.Workspace;

import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.table.DefaultTableModel;

public class InstallStepOne extends IDialog implements MouseListener {

    /**
     * 
     */
    private static final long serialVersionUID = -8821906198791949544L;

    private JPanel jContentPane = null;

    private JButton jButtonCancel = null;

    private JButton jButtonNext = null;

    private JTextArea jTextArea = null;

    private JLabel jLabel = null;

    private JTextField jTextFieldFarFile = null;

    private JButton jButtonBrowser = null;

    private InstallStepTwo stepTwo = null;

    Far far = null;

    private JLabel jLabelWarning = null;

    private JScrollPane jScrollPane = null;

    private JTable jTable = null;

    private PartialTableModel model = null;

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
     * This method initializes jTextArea	
     * 	
     * @return javax.swing.JTextArea	
     */
    private JTextArea getJTextArea() {
        if (jTextArea == null) {
            jTextArea = new JTextArea();
            jTextArea.setBounds(new java.awt.Rectangle(30, 7, 642, 50));
            jTextArea.setText("Step 1: Choose a framework archive(FAR) file. \n");
            jTextArea.setEditable(false);
        }
        return jTextArea;
    }

    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldFarFile() {
        if (jTextFieldFarFile == null) {
            jTextFieldFarFile = new JTextField();
            jTextFieldFarFile.setBounds(new java.awt.Rectangle(140, 80, 423, 20));
        }
        return jTextFieldFarFile;
    }

    /**
     * This method initializes jButtonBrowser	
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
            jScrollPane.setBounds(new java.awt.Rectangle(30, 165, 642, 140));
            jScrollPane.setViewportView(getJTable());
        }
        jScrollPane.setVisible(false);
        return jScrollPane;
    }

    /**
     * This method initializes jTable	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable() {
        if (jTable == null) {
            jTable = new JTable();
            model = new PartialTableModel();
            jTable = new JTable(model);
            jTable.setRowHeight(20);
            jTable.setAutoResizeMode(javax.swing.JTable.AUTO_RESIZE_SUBSEQUENT_COLUMNS);
            model.addColumn("Name");
            model.addColumn("Version");
            model.addColumn("GUID");

            jTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        }
        jTable.setVisible(false);
        return jTable;
    }

    public void prepareTable(List<PackageIdentification> packageList) {
        model.setRowCount(0);
        //
        // Change here to get packages and platforms from FAR
        //
        Iterator<PackageIdentification> iter = packageList.iterator();
        while (iter.hasNext()) {
            String[] str = new String[3];
            PackageIdentification item = iter.next();
            str[0] = item.getName();
            str[1] = item.getVersion();
            str[2] = item.getGuid();
            model.addRow(str);
        }
    }

    /**
     * This is the default constructor
     */
    public InstallStepOne(IFrame iFrame, boolean modal) {
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
        this.setTitle(FarStringDefinition.INSTALL_STEP_ONE_TITLE);
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
            jLabelWarning = new JLabel();
            jLabelWarning.setBounds(new java.awt.Rectangle(30, 125, 510, 20));
            jLabelWarning.setText("Cannot install this FAR, the WORKSPACE is missing the following required packages.");
            jLabelWarning.setVisible(false);
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(30, 80, 97, 20));
            jLabel.setText("Choose FAR file: ");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonNext(), null);
            jContentPane.add(getJTextArea(), null);
            jContentPane.add(jLabel, null);
            jContentPane.add(getJTextFieldFarFile(), null);
            jContentPane.add(getJButtonBrowser(), null);
            jContentPane.add(jLabelWarning, null);
            jContentPane.add(getJScrollPane(), null);
        }
        return jContentPane;
    }

    public void mouseClicked(MouseEvent e) {
        if (e.getSource() == jButtonCancel) {
            this.setVisible(false);
        } else if (e.getSource() == jButtonNext) {
            //
            // Add some logic process here
            //
            File farFile = new File(jTextFieldFarFile.getText());
            if (!farFile.exists() || !farFile.isFile()) {
                Log.wrn("Install far", "Please choose an existing FAR file.");
                return;
            }

            //
            // Verify Far
            //
            JarFile jarFar;
            try {
                jarFar = new JarFile(farFile);
                far = new Far(jarFar);

                //
                // Far dependency check
                //
                List<PackageIdentification> pkgIdList = DistributeRule.installFarCheck(far);

                if (pkgIdList.size() > 0) {
                    prepareTable(pkgIdList);
                    jLabelWarning.setVisible(true);
                    jTable.setVisible(true);
                    jScrollPane.setVisible(true);
                    return;
                }

            } catch (Exception exp) {
                Log.wrn("Install far" + exp.getMessage());
                Log.err("Install far" + exp.getMessage());
            }

            if (stepTwo == null) {
                stepTwo = new InstallStepTwo(this, true, this);
            }
            this.setVisible(false);

            //
            // Refresh table
            //
            stepTwo.preparePackageTable();
            stepTwo.preparePlatformTable();
            stepTwo.setVisible(true);
        } else if (e.getSource() == jButtonBrowser) {
            JFileChooser fc = new JFileChooser();
            fc.setAcceptAllFileFilterUsed(false);
            fc.addChoosableFileFilter(new IFileFilter(DataType.FAR_SURFACE_AREA_EXT));
            fc.setCurrentDirectory(new File(Workspace.getCurrentWorkspace()));

            int result = fc.showOpenDialog(new JPanel());
            if (result == JFileChooser.APPROVE_OPTION) {
                jLabelWarning.setVisible(false);
                jTable.setVisible(false);
                jScrollPane.setVisible(false);
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

    public Far getFar() {
        return far;
    }

}

class PartialTableModel extends DefaultTableModel {
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    public boolean isCellEditable(int row, int col) {
        switch (col) {
        default:
            return false;
        }
    }
}
