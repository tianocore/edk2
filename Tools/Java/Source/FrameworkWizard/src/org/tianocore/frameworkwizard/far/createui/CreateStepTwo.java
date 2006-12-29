/** @file

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.far.createui;

import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;

import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.far.FarStringDefinition;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;
import org.tianocore.frameworkwizard.workspace.WorkspaceTools;

public class CreateStepTwo extends IDialog implements MouseListener {

    /**
     * 
     */
    private static final long serialVersionUID = 3003841865197005528L;

    private JPanel jContentPane = null;

    private JTextArea jTextArea = null;

    private JLabel jLabel = null;

    private JLabel jLabel1 = null;

    private ICheckBoxList jComboBoxPackage = null;

    private ICheckBoxList jComboBoxPlatform = null;

    private JButton jButtonNext = null;

    private JButton jButtonCancel = null;

    private JScrollPane jScrollPanePackage = null;

    private JScrollPane jScrollPanePlatform = null;

    private CreateStepThree stepThree = null;

    private Vector<PlatformIdentification> platformVector = null;

    private Vector<PackageIdentification> packageVector = null;

    private CreateStepOne stepOne = null;

    private JButton jButtonPrevious = null;

    public CreateStepTwo(IDialog iDialog, boolean modal, CreateStepOne stepOne) {
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
            jTextArea.setText("Choose at least one package or platform. ");
            jTextArea.setEditable(false);
        }
        return jTextArea;
    }

    /**
     * This method initializes jComboBox	
     * 	
     * @return javax.swing.JComboBox	
     */
    private ICheckBoxList getJComboBoxPackage() {
        if (jComboBoxPackage == null) {
            jComboBoxPackage = new ICheckBoxList();
            WorkspaceTools wt = new WorkspaceTools();
            Vector<String> v = new Vector<String>();
            packageVector = wt.getAllPackages();
            Iterator<PackageIdentification> iter = packageVector.iterator();
            while (iter.hasNext()) {
                PackageIdentification item = iter.next();
                String str = item.getName() + " " + item.getVersion() + " [" + item.getPath() + "]";
                v.addElement(str);
            }
            jComboBoxPackage.setAllItems(v);
        }
        return jComboBoxPackage;
    }

    /**
     * This method initializes jComboBox1	
     * 	
     * @return javax.swing.JComboBox	
     */
    private ICheckBoxList getJComboBoxPlatform() {
        if (jComboBoxPlatform == null) {
            jComboBoxPlatform = new ICheckBoxList();
            WorkspaceTools wt = new WorkspaceTools();
            Vector<String> v = new Vector<String>();
            platformVector = wt.getAllPlatforms();
            Iterator<PlatformIdentification> iter = platformVector.iterator();
            while (iter.hasNext()) {
                PlatformIdentification item = iter.next();
                String str = item.getName() + " " + item.getVersion() + " [" + item.getPath() + "]";
                v.addElement(str);
            }
            jComboBoxPlatform.setAllItems(v);
        }
        return jComboBoxPlatform;
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
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPanePackage() {
        if (jScrollPanePackage == null) {
            jScrollPanePackage = new JScrollPane();
            jScrollPanePackage.setBounds(new java.awt.Rectangle(140,65,535,130));
            jScrollPanePackage.setViewportView(getJComboBoxPackage());
        }
        return jScrollPanePackage;
    }

    /**
     * This method initializes jScrollPane1	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPanePlatform() {
        if (jScrollPanePlatform == null) {
            jScrollPanePlatform = new JScrollPane();
            jScrollPanePlatform.setBounds(new java.awt.Rectangle(140,200,535,110));
            jScrollPanePlatform.setViewportView(getJComboBoxPlatform());
        }
        return jScrollPanePlatform;
    }

    /**
     * This is the default constructor
     */
    public CreateStepTwo(IDialog iDialog, boolean modal) {
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
        this.setTitle(FarStringDefinition.CREATE_STEP_TWO_TITLE);
        this.centerWindow();
    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabel1 = new JLabel();
            jLabel1.setBounds(new java.awt.Rectangle(30, 200, 100, 20));
            jLabel1.setText("Platforms: ");
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(30, 64, 100, 20));
            jLabel.setText("Packages:");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJTextArea(), null);
            jContentPane.add(jLabel, null);
            jContentPane.add(jLabel1, null);
            jContentPane.add(getJButtonNext(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJScrollPanePackage(), null);
            jContentPane.add(getJScrollPanePlatform(), null);
            jContentPane.add(getJButtonPrevious(), null);
        }
        return jContentPane;
    }

    public void mouseClicked(MouseEvent e) {
        if (e.getSource() == jButtonCancel) {
            this.setVisible(false);
        } else if (e.getSource() == jButtonPrevious) {
            this.setVisible(false);
            stepOne.setVisible(true);
        } else if (e.getSource() == jButtonNext) {
            //
            // Add some logic process here
            //
            if (jComboBoxPlatform.getAllCheckedItemsIndex().size() == 0
                && jComboBoxPackage.getAllCheckedItemsIndex().size() == 0) {
                Log.wrn("Create far", "Choose at least one package and/or platform.");
                return;
            }
            
            //
            // If some packages a Repackage=false, give a warning message
            //
            List<PackageIdentification> selectedPackages = getSelectedPackages();
            WorkspaceTools wt = new WorkspaceTools();
            List<PackageIdentification> allRepackablePackages = wt.getAllRepackagablePackages();
            
            List<PackageIdentification> unRepackablePackages = new Vector<PackageIdentification>();
            String msg = "Following selected packages: \n";
            Iterator<PackageIdentification> iter = selectedPackages.iterator();
            while (iter.hasNext()) {
                PackageIdentification item = iter.next();
                if (!allRepackablePackages.contains(item)) {
                    unRepackablePackages.add(item);
                    msg += item.getName() + "\n";
                }
            }
            msg += "is un-Repackagable. Do you want to continue? ";
            
            if (unRepackablePackages.size() > 0) {
                if(JOptionPane.showConfirmDialog(this, msg, "Warning", JOptionPane.YES_NO_OPTION) == JOptionPane.NO_OPTION) {
                    return ;
                }
            }
            
            if (stepThree == null) {
                stepThree = new CreateStepThree(this, true, this);
            }
            this.setVisible(false);
            stepThree.setVisible(true);
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

    public List<PackageIdentification> getSelectedPackages() {
        Vector<Integer> v = jComboBoxPackage.getAllCheckedItemsIndex();
        List<PackageIdentification> result = new ArrayList<PackageIdentification>();
        for (int i = 0; i < v.size(); i++) {
            result.add(packageVector.get(v.get(i).intValue()));
        }
        return result;
    }

    public List<PlatformIdentification> getSelectedPlatforms() {
        Vector<Integer> v = jComboBoxPlatform.getAllCheckedItemsIndex();
        List<PlatformIdentification> result = new ArrayList<PlatformIdentification>();
        for (int i = 0; i < v.size(); i++) {
            result.add(platformVector.get(v.get(i).intValue()));
        }
        return result;
    }

    public CreateStepOne getPreviousStep() {
        return stepOne;
    }
}
