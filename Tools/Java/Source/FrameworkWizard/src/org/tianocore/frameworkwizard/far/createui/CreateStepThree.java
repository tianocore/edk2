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
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.Set;
import java.util.Vector;

import javax.swing.JPanel;
import javax.swing.JTextArea;
import javax.swing.JButton;
import javax.swing.JLabel;

import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;
import org.tianocore.frameworkwizard.far.FarStringDefinition;

import javax.swing.JScrollPane;
import javax.swing.JTextField;

public class CreateStepThree extends IDialog implements MouseListener {

    /**
     * 
     */
    private static final long serialVersionUID = 7559888600474043337L;

    private JPanel jContentPane = null;

    private JTextArea jTextArea = null;

    private JButton jButtonNext = null;

    private JButton jButtonCancel = null;

    private JButton jButtonPrevious = null;

    private JLabel jLabel = null;

    private ICheckBoxList jComboBoxFileFilter = null;

    private JScrollPane jScrollPane = null;

    private JLabel jLabel1 = null;

    private JTextField jTextField = null;

    Vector<String> v = new Vector<String>();

    private CreateStepTwo stepTwo = null;

    private CreateStepFour stepFour = null;

    public CreateStepThree(IDialog iDialog, boolean modal, CreateStepTwo stepTwo) {
        this(iDialog, modal);
        this.stepTwo = stepTwo;
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
            jTextArea.setText("Add additional file filter regular expressions in the text field, separated by space characters.\n");
            jTextArea.append("Note, for additional information about regular expressions, please reference PERL language regular expressions.");
            jTextArea.setEditable(false);
        }
        return jTextArea;
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
     * This method initializes jComboBox	
     * 	
     * @return javax.swing.JComboBox	
     */
    private ICheckBoxList getJComboBoxFileFilter() {
        if (jComboBoxFileFilter == null) {
            jComboBoxFileFilter = new ICheckBoxList();
            v.addElement(".svn");
            v.addElement("CVS");
            jComboBoxFileFilter.setAllItems(v);
            jComboBoxFileFilter.initCheckedItem(true, v);
        }
        return jComboBoxFileFilter;
    }

    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setBounds(new java.awt.Rectangle(30, 85, 640, 130));
            jScrollPane.setViewportView(getJComboBoxFileFilter());
        }
        return jScrollPane;
    }

    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setBounds(new java.awt.Rectangle(30, 250, 640, 20));
        }
        return jTextField;
    }

    /**
     * @param args
     */
    public static void main(String[] args) {
        // TODO Auto-generated method stub
    }

    /**
     * This is the default constructor
     */
    public CreateStepThree(IDialog iDialog, boolean modal) {
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
        this.setTitle(FarStringDefinition.CREATE_STEP_THREE_TITLE);
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
            jLabel1.setBounds(new java.awt.Rectangle(30, 220, 260, 20));
            jLabel1.setText("Input File Filter Pattern (regular expressions)");
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(30, 64, 160, 20));
            jLabel.setText("File Filter Pattern: ");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJTextArea(), null);
            jContentPane.add(getJButtonNext(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonPrevious(), null);
            jContentPane.add(jLabel, null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(jLabel1, null);
            jContentPane.add(getJTextField(), null);
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

            if (stepFour == null) {
                stepFour = new CreateStepFour(this, true, this);
            }
            
            this.setVisible(false);
            stepFour.setVisible(true);
        } else if (e.getSource() == jButtonPrevious) {
            this.setVisible(false);
            stepTwo.setVisible(true);
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

    public Set<String> getFileFilter() {
        Set<String> result = new LinkedHashSet<String>();
        Vector<Integer> selected = jComboBoxFileFilter.getAllCheckedItemsIndex();

        Iterator<Integer> iter = selected.iterator();

        while (iter.hasNext()) {
            result.add(v.get(iter.next().intValue()));
        }

        String[] userdefined = jTextField.getText().split(" ");

        for (int i = 0; i < userdefined.length; i++) {
            if (!userdefined[i].trim().equalsIgnoreCase("")) {
                result.add(userdefined[i]);
            }
        }

        return result;
    }

    public CreateStepTwo getPreviousStep() {
        return stepTwo;
    }
}
