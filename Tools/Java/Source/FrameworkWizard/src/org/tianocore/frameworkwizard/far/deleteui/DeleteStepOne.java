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

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import javax.swing.ImageIcon;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;

import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.far.AggregationOperation;
import org.tianocore.frameworkwizard.far.FarIdentification;
import org.tianocore.frameworkwizard.far.FarStringDefinition;
import org.tianocore.frameworkwizard.far.PackageQuery;
import org.tianocore.frameworkwizard.far.PackageQueryInterface;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

public class DeleteStepOne extends IDialog implements ListSelectionListener {

    /**
     * 
     */
    private static final long serialVersionUID = 636773964435618476L;

    private JPanel jContentPane = null;

    private JButton jButtonCancel = null;

    private JButton jButtonNext = null;

    private JTextArea jTextAreaInstruction = null;

    private JLabel jLabel = null;

    private JScrollPane jScrollPane = null;

    private JLabel jLabel2 = null;

    private JLabel jLabel3 = null;

    private JScrollPane jScrollPane1 = null;

    private JScrollPane jScrollPane2 = null;

    private JList jListPlatform = null;

    private JList jListPackage = null;

    private JLabel jLabel4 = null;

    private JButton jButtonDetail = null;

    private JList jListFar = null;

    private JLabel jLabelImage = null;

    private Vector<FarIdentification> farVector = null;

    Vector<PackageIdentification> removePackages = null;

    Vector<PlatformIdentification> removePlatforms = null;

    private DeleteStepTwo stepTwo = null;

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
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     * This method initializes jButtonFinish
     * 
     * @return javax.swing.JButton
     */
    private JButton getJButtonNext() {
        if (jButtonNext == null) {
            jButtonNext = new JButton();
            jButtonNext.setBounds(new java.awt.Rectangle(470, 330, 90, 20));
            jButtonNext.setText("Next");
            jButtonNext.setEnabled(false);
            jButtonNext.addActionListener(this);
        }
        return jButtonNext;
    }

    /**
     * This method initializes jTextArea1
     * 
     * @return javax.swing.JTextArea
     */
    private JTextArea getJTextArea1() {
        if (jTextAreaInstruction == null) {
            jTextAreaInstruction = new JTextArea();
            jTextAreaInstruction.setBounds(new java.awt.Rectangle(30, 7, 662, 50));
            jTextAreaInstruction.setText("Step 1: Select FAR to remove.\n");
            jTextAreaInstruction.setCaretColor(Color.RED);
            jTextAreaInstruction
                                .append("After choosing the FAR, the packages and/or platforms that belong to the FAR will displayed.\n");
            jTextAreaInstruction.append("Icon \"OK\" or \"NO\" indicates whether the FAR can be safely removed.");
            jTextAreaInstruction.setEditable(false);
        }
        return jTextAreaInstruction;
    }

    /**
     * This method initializes jScrollPane
     * 
     * @return javax.swing.JScrollPane
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setBounds(new java.awt.Rectangle(140, 65, 530, 100));
            jScrollPane.setViewportView(getJListFar());
        }
        return jScrollPane;
    }

    /**
     * This method initializes jScrollPane1
     * 
     * @return javax.swing.JScrollPane
     */
    private JScrollPane getJScrollPane1() {
        if (jScrollPane1 == null) {
            jScrollPane1 = new JScrollPane();
            jScrollPane1.setBounds(new java.awt.Rectangle(30, 195, 300, 115));
            jScrollPane1.setViewportView(getJListPackage());
        }
        return jScrollPane1;
    }

    /**
     * This method initializes jScrollPane2
     * 
     * @return javax.swing.JScrollPane
     */
    private JScrollPane getJScrollPane2() {
        if (jScrollPane2 == null) {
            jScrollPane2 = new JScrollPane();
            jScrollPane2.setBounds(new java.awt.Rectangle(360, 195, 310, 115));
            jScrollPane2.setViewportView(getJListPlatform());
        }
        return jScrollPane2;
    }

    /**
     * This method initializes jList
     * 
     * @return javax.swing.JList
     */
    private JList getJListPlatform() {
        if (jListPlatform == null) {
            jListPlatform = new JList();
            jListPlatform.setEnabled(false);
        }
        return jListPlatform;
    }

    /**
     * This method initializes jList1
     * 
     * @return javax.swing.JList
     */
    private JList getJListPackage() {
        if (jListPackage == null) {
            jListPackage = new JList();
            jListPackage.setEnabled(false);
        }
        return jListPackage;
    }

    /**
     * This method initializes jButtonDetail
     * 
     * @return javax.swing.JButton
     */
    private JButton getJButtonDetail() {
        if (jButtonDetail == null) {
            jButtonDetail = new JButton();
            jButtonDetail.setBounds(new java.awt.Rectangle(367, 325, 69, 20));
            jButtonDetail.setText("Detail");
            jButtonDetail.setVisible(false);
        }
        return jButtonDetail;
    }

    /**
     * This method initializes jListFar
     * 
     * @return javax.swing.JList
     */
    private JList getJListFar() {
        if (jListFar == null) {
            jListFar = new JList();
            WorkspaceTools wt = new WorkspaceTools();
            farVector = wt.getAllFars();
            jListFar.setListData(farVector);
            jListFar.addListSelectionListener(this);
        }
        return jListFar;
    }

    /**
     * This is the default constructor
     */
    public DeleteStepOne(IFrame iFrame, boolean modal) {
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
        this.setTitle(FarStringDefinition.DELETE_STEP_ONE_TITLE);
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
            jLabelImage = new JLabel();
            jLabelImage.setBounds(new java.awt.Rectangle(30, 319, 36, 36));
            jLabel4 = new JLabel();
            jLabel4.setBounds(new java.awt.Rectangle(71, 325, 320, 20));
            jLabel3 = new JLabel();
            jLabel3.setBounds(new java.awt.Rectangle(360, 170, 113, 20));
            jLabel3.setText("FAR's Platforms");
            jLabel2 = new JLabel();
            jLabel2.setBounds(new java.awt.Rectangle(30, 170, 113, 20));
            jLabel2.setText("FAR's Packages");
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(30, 65, 100, 20));
            jLabel.setText("Select one FAR: ");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonNext(), null);
            jContentPane.add(getJTextArea1(), null);
            jContentPane.add(jLabel, null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(jLabel2, null);
            jContentPane.add(jLabel3, null);
            jContentPane.add(getJScrollPane1(), null);
            jContentPane.add(getJScrollPane2(), null);
            jContentPane.add(jLabel4, null);
            jContentPane.add(getJButtonDetail(), null);
            jContentPane.add(jLabelImage, null);
        }
        return jContentPane;
    }

    public void valueChanged(ListSelectionEvent e) {
        //
        // Add logic for FAR list value changed
        //
        if (e.getSource() == jListFar) {
            boolean flag = true;
            FarIdentification far = (FarIdentification) jListFar.getSelectedValue();
            WorkspaceTools wt = new WorkspaceTools();

            removePackages = wt.getPackagesByFar(far);
            jListPackage.setListData(removePackages);
            removePlatforms = wt.getPlatformsByFar(far);
            jListPlatform.setListData(removePlatforms);

            //
            // Get Dependencies Info for current FAR
            //
            List<PackageIdentification> allPackages = wt.getAllPackages();

            //
            // Remain packages
            //
            allPackages.removeAll(removePackages);

            Iterator<PackageIdentification> iter = allPackages.iterator();

            PackageQueryInterface pq = new PackageQuery();
            while (iter.hasNext()) {
                PackageIdentification item = iter.next();
                List<PackageIdentification> list = pq.getPackageDependencies(item.getSpdFile());
                List<PackageIdentification> result = AggregationOperation.minus(list, allPackages);
                if (result.size() > 0) {
                    if (AggregationOperation.intersection(result, removePackages).size() > 0) {
                        flag = false;
                        break;
                    }
                }
            }

            if (flag) {
                jLabelImage.setIcon(new ImageIcon(getClass().getResource("/resources/images/Yes.JPG")));
                jLabel4.setText("None of the remaining packages depend on this FAR. ");
                jButtonDetail.setVisible(false);
                jButtonNext.setEnabled(true);
            } else {
                jLabelImage.setIcon(new ImageIcon(getClass().getResource("/resources/images/No.JPG")));
                jLabel4.setText("Some of the remaining packages still depend on this FAR. ");
                //        jButtonDetail.setVisible(true);
                jButtonNext.setEnabled(false);
            }
        }
    }
    
    public void actionPerformed(ActionEvent e) {
        if (e.getSource() == jButtonCancel) {
            this.setVisible(false);
        } else if (e.getSource() == jButtonNext) {
            //
            // Add some logic process here
            //

            if (stepTwo == null) {
                stepTwo = new DeleteStepTwo(this, true, this);
            }
            this.setVisible(false);
            stepTwo.setVisible(true);
        }

    }

    public FarIdentification getSelecedFar() {
        return (FarIdentification) jListFar.getSelectedValue();
    }

}
