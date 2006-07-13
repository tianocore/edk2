/** @file
 
 The file is used to provid 6 kinds of arch in one jpanel 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.common.ui;

import java.util.Vector;

import javax.swing.JPanel;
import javax.swing.JCheckBox;

import org.tianocore.frameworkwizard.common.DataType;

public class ArchCheckBox extends JPanel {

    ///
    ///
    ///
    private static final long serialVersionUID = 4792669775676953990L;

    private JCheckBox jCheckBoxIa32 = null;

    private JCheckBox jCheckBoxX64 = null;

    private JCheckBox jCheckBoxIpf = null;

    private JCheckBox jCheckBoxEbc = null;

    private JCheckBox jCheckBoxArm = null;

    private JCheckBox jCheckBoxPpc = null;

    /**
     * This method initializes jCheckBoxIa32	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxIa32() {
        if (jCheckBoxIa32 == null) {
            jCheckBoxIa32 = new JCheckBox();
            jCheckBoxIa32.setBounds(new java.awt.Rectangle(0, 0, 50, 20));
            jCheckBoxIa32.setText("IA32");
            jCheckBoxIa32.setToolTipText(DataType.SUP_ARCH_LIST_HELP_TEXT);
        }
        return jCheckBoxIa32;
    }

    /**
     * This method initializes jCheckBoxX64	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxX64() {
        if (jCheckBoxX64 == null) {
            jCheckBoxX64 = new JCheckBox();
            jCheckBoxX64.setBounds(new java.awt.Rectangle(50, 0, 50, 20));
            jCheckBoxX64.setText("X64");
            jCheckBoxX64.setToolTipText(DataType.SUP_ARCH_LIST_HELP_TEXT);
        }
        return jCheckBoxX64;
    }

    /**
     * This method initializes jCheckBoxIpf	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxIpf() {
        if (jCheckBoxIpf == null) {
            jCheckBoxIpf = new JCheckBox();
            jCheckBoxIpf.setBounds(new java.awt.Rectangle(100, 0, 50, 20));
            jCheckBoxIpf.setText("IPF");
            jCheckBoxIpf.setToolTipText(DataType.SUP_ARCH_LIST_HELP_TEXT);
        }
        return jCheckBoxIpf;
    }

    /**
     * This method initializes jCheckBoxEbc	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxEbc() {
        if (jCheckBoxEbc == null) {
            jCheckBoxEbc = new JCheckBox();
            jCheckBoxEbc.setBounds(new java.awt.Rectangle(150, 0, 50, 20));
            jCheckBoxEbc.setText("EBC");
            jCheckBoxEbc.setToolTipText(DataType.SUP_ARCH_LIST_HELP_TEXT);
        }
        return jCheckBoxEbc;
    }

    /**
     * This method initializes jCheckBoxArm	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxArm() {
        if (jCheckBoxArm == null) {
            jCheckBoxArm = new JCheckBox();
            jCheckBoxArm.setBounds(new java.awt.Rectangle(200, 0, 55, 20));
            jCheckBoxArm.setText("ARM");
            jCheckBoxArm.setToolTipText(DataType.SUP_ARCH_LIST_HELP_TEXT);
        }
        return jCheckBoxArm;
    }

    /**
     * This method initializes jCheckBoxPrc	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxPpc() {
        if (jCheckBoxPpc == null) {
            jCheckBoxPpc = new JCheckBox();
            jCheckBoxPpc.setBounds(new java.awt.Rectangle(255, 0, 50, 20));
            jCheckBoxPpc.setText("PPC");
            jCheckBoxPpc.setToolTipText(DataType.SUP_ARCH_LIST_HELP_TEXT);
        }
        return jCheckBoxPpc;
    }

    /**
     
     @param args
     
     **/
    public static void main(String[] args) {
        // TODO Auto-generated method stub

    }

    /**
     * This is the default constructor
     */
    public ArchCheckBox() {
        super();
        initialize();
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
    private void initialize() {
        this.setSize(320, 20);
        this.setLayout(null);
        this.add(getJCheckBoxIa32(), null);
        this.add(getJCheckBoxX64(), null);
        this.add(getJCheckBoxIpf(), null);
        this.add(getJCheckBoxEbc(), null);
        this.add(getJCheckBoxArm(), null);
        this.add(getJCheckBoxPpc(), null);
        this.setToolTipText(DataType.SUP_ARCH_LIST_HELP_TEXT);
    }
    
    public Vector<String> getSelectedItemsVector() {
        Vector<String> v = new Vector<String>();
        if (this.jCheckBoxIa32.isSelected()) {
            v.addElement(jCheckBoxIa32.getText());
        }
        if (this.jCheckBoxX64.isSelected()) {
            v.addElement(jCheckBoxX64.getText());
        }
        if (this.jCheckBoxIpf.isSelected()) {
            v.addElement(jCheckBoxIpf.getText());
        }
        if (this.jCheckBoxEbc.isSelected()) {
            v.addElement(jCheckBoxEbc.getText());
        }
        if (this.jCheckBoxArm.isSelected()) {
            v.addElement(jCheckBoxArm.getText());
        }
        if (this.jCheckBoxPpc.isSelected()) {
            v.addElement(jCheckBoxPpc.getText());
        }
        return v;
    }
    
    public String getSelectedItemsString() {
        String s = "";
        if (this.jCheckBoxIa32.isSelected()) {
            s = s + jCheckBoxIa32.getText() + " ";
        }
        if (this.jCheckBoxX64.isSelected()) {
            s = s + jCheckBoxX64.getText() + " ";
        }
        if (this.jCheckBoxIpf.isSelected()) {
            s = s + jCheckBoxIpf.getText() + " ";
        }
        if (this.jCheckBoxEbc.isSelected()) {
            s = s + jCheckBoxEbc.getText() + " ";
        }
        if (this.jCheckBoxArm.isSelected()) {
            s = s + jCheckBoxArm.getText() + " ";
        }
        if (this.jCheckBoxPpc.isSelected()) {
            s = s + jCheckBoxPpc.getText() + " ";
        }
        return s.trim();
    }
    
    public void setAllItmesSelected(boolean isSelected) {
        this.jCheckBoxIa32.setSelected(isSelected);
        this.jCheckBoxX64.setSelected(isSelected);
        this.jCheckBoxIpf.setSelected(isSelected);
        this.jCheckBoxEbc.setSelected(isSelected);
        this.jCheckBoxArm.setSelected(isSelected);
        this.jCheckBoxPpc.setSelected(isSelected);
    }
    
    public void setSelectedItems(Vector<String> v) {
        setAllItmesSelected(false);
        if (v != null) {
            for (int index = 0; index < v.size(); index++) {
                if (v.get(index).equals(this.jCheckBoxIa32.getText())) {
                    this.jCheckBoxIa32.setSelected(true);
                    continue;
                }
                if (v.get(index).equals(this.jCheckBoxIpf.getText())) {
                    this.jCheckBoxIpf.setSelected(true);
                    continue;
                }
                if (v.get(index).equals(this.jCheckBoxX64.getText())) {
                    this.jCheckBoxX64.setSelected(true);
                    continue;
                }
                if (v.get(index).equals(this.jCheckBoxEbc.getText())) {
                    this.jCheckBoxEbc.setSelected(true);
                    continue;
                }
                if (v.get(index).equals(this.jCheckBoxArm.getText())) {
                    this.jCheckBoxArm.setSelected(true);
                    continue;
                }
                if (v.get(index).equals(this.jCheckBoxPpc.getText())) {
                    this.jCheckBoxPpc.setSelected(true);
                    continue;
                }
            }
        }
    }
}
