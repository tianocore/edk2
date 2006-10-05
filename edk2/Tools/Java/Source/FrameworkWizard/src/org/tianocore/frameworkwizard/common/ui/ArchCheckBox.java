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

import javax.swing.JCheckBox;
import javax.swing.JPanel;

import org.tianocore.frameworkwizard.common.DataType;

public class ArchCheckBox extends JPanel {

    ///
    /// Define class members
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
            jCheckBoxIa32.setBounds(new java.awt.Rectangle(0, 0, 55, 20));
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
            jCheckBoxX64.setBounds(new java.awt.Rectangle(55, 0, 53, 20));
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
            jCheckBoxIpf.setBounds(new java.awt.Rectangle(108, 0, 52, 20));
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
            jCheckBoxEbc.setBounds(new java.awt.Rectangle(160, 0, 53, 20));
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
            jCheckBoxArm.setBounds(new java.awt.Rectangle(213, 0, 54, 20));
            jCheckBoxArm.setText("ARM");
            jCheckBoxArm.setToolTipText(DataType.SUP_ARCH_LIST_HELP_TEXT);
        }
        return jCheckBoxArm;
    }

    /**
     * This method initializes jCheckBoxPpc	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxPpc() {
        if (jCheckBoxPpc == null) {
            jCheckBoxPpc = new JCheckBox();
            jCheckBoxPpc.setBounds(new java.awt.Rectangle(267, 0, 53, 20));
            jCheckBoxPpc.setText("PPC");
            jCheckBoxPpc.setToolTipText(DataType.SUP_ARCH_LIST_HELP_TEXT);
        }
        return jCheckBoxPpc;
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
        if (this.jCheckBoxIa32.isSelected() && this.jCheckBoxIa32.isEnabled()) {
            v.addElement(jCheckBoxIa32.getText());
        }
        if (this.jCheckBoxX64.isSelected() && this.jCheckBoxX64.isEnabled()) {
            v.addElement(jCheckBoxX64.getText());
        }
        if (this.jCheckBoxIpf.isSelected() && this.jCheckBoxIpf.isEnabled()) {
            v.addElement(jCheckBoxIpf.getText());
        }
        if (this.jCheckBoxEbc.isSelected() && this.jCheckBoxEbc.isEnabled()) {
            v.addElement(jCheckBoxEbc.getText());
        }
        if (this.jCheckBoxArm.isSelected() && this.jCheckBoxArm.isEnabled()) {
            v.addElement(jCheckBoxArm.getText());
        }
        if (this.jCheckBoxPpc.isSelected() && this.jCheckBoxPpc.isEnabled()) {
            v.addElement(jCheckBoxPpc.getText());
        }
        return v;
    }

    public String getSelectedItemsString() {
        String s = "";
        if (this.jCheckBoxIa32.isSelected() && this.jCheckBoxIa32.isEnabled()) {
            s = s + jCheckBoxIa32.getText() + " ";
        }
        if (this.jCheckBoxX64.isSelected() && this.jCheckBoxX64.isEnabled()) {
            s = s + jCheckBoxX64.getText() + " ";
        }
        if (this.jCheckBoxIpf.isSelected() && this.jCheckBoxIpf.isEnabled()) {
            s = s + jCheckBoxIpf.getText() + " ";
        }
        if (this.jCheckBoxEbc.isSelected() && this.jCheckBoxEbc.isEnabled()) {
            s = s + jCheckBoxEbc.getText() + " ";
        }
        if (this.jCheckBoxArm.isSelected() && this.jCheckBoxArm.isEnabled()) {
            s = s + jCheckBoxArm.getText() + " ";
        }
        if (this.jCheckBoxPpc.isSelected() && this.jCheckBoxPpc.isEnabled()) {
            s = s + jCheckBoxPpc.getText() + " ";
        }
        return s.trim();
    }

    public void setAllItemsSelected(boolean isSelected) {
        this.jCheckBoxIa32.setSelected(isSelected);
        this.jCheckBoxX64.setSelected(isSelected);
        this.jCheckBoxIpf.setSelected(isSelected);
        this.jCheckBoxEbc.setSelected(isSelected);
        this.jCheckBoxArm.setSelected(isSelected);
        this.jCheckBoxPpc.setSelected(isSelected);
    }

    public void setSelectedItems(Vector<String> v) {
        setAllItemsSelected(false);
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

    public void setAllItemsEnabled(boolean isEnabled) {
        this.jCheckBoxIa32.setEnabled(isEnabled);
        this.jCheckBoxX64.setEnabled(isEnabled);
        this.jCheckBoxIpf.setEnabled(isEnabled);
        this.jCheckBoxEbc.setEnabled(isEnabled);
        this.jCheckBoxArm.setEnabled(isEnabled);
        this.jCheckBoxPpc.setEnabled(isEnabled);
    }

    public void setEnabledItems(Vector<String> v) {
        setAllItemsEnabled(false);
        if (v != null) {
            for (int index = 0; index < v.size(); index++) {
                if (v.get(index).equals(this.jCheckBoxIa32.getText())) {
                    this.jCheckBoxIa32.setEnabled(true);
                    continue;
                }
                if (v.get(index).equals(this.jCheckBoxIpf.getText())) {
                    this.jCheckBoxIpf.setEnabled(true);
                    continue;
                }
                if (v.get(index).equals(this.jCheckBoxX64.getText())) {
                    this.jCheckBoxX64.setEnabled(true);
                    continue;
                }
                if (v.get(index).equals(this.jCheckBoxEbc.getText())) {
                    this.jCheckBoxEbc.setEnabled(true);
                    continue;
                }
                if (v.get(index).equals(this.jCheckBoxArm.getText())) {
                    this.jCheckBoxArm.setEnabled(true);
                    continue;
                }
                if (v.get(index).equals(this.jCheckBoxPpc.getText())) {
                    this.jCheckBoxPpc.setEnabled(true);
                    continue;
                }
            }
        }
    }

    public void setDisabledItems(Vector<String> v) {
        setAllItemsEnabled(true);
        if (v != null) {
            for (int index = 0; index < v.size(); index++) {
                if (v.get(index).equals(this.jCheckBoxIa32.getText())) {
                    this.jCheckBoxIa32.setEnabled(false);
                    continue;
                }
                if (v.get(index).equals(this.jCheckBoxIpf.getText())) {
                    this.jCheckBoxIpf.setEnabled(false);
                    continue;
                }
                if (v.get(index).equals(this.jCheckBoxX64.getText())) {
                    this.jCheckBoxX64.setEnabled(false);
                    continue;
                }
                if (v.get(index).equals(this.jCheckBoxEbc.getText())) {
                    this.jCheckBoxEbc.setEnabled(false);
                    continue;
                }
                if (v.get(index).equals(this.jCheckBoxArm.getText())) {
                    this.jCheckBoxArm.setEnabled(false);
                    continue;
                }
                if (v.get(index).equals(this.jCheckBoxPpc.getText())) {
                    this.jCheckBoxPpc.setEnabled(false);
                    continue;
                }
            }
        }
    }
}
