/** @file
 
 The file is used to create, update Package Definitions of Spd file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.packaging.ui;

import javax.swing.JPanel;
import javax.swing.JLabel;
import javax.swing.JComboBox;
import java.awt.Dimension;

import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPackageType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;

public class SpdPackageDefinitions extends IInternalFrame {

    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    private JPanel jContentPane = null;
    private JLabel jLabel = null;
    private JComboBox jComboBoxReadOnly = null;
    private JLabel jLabel1 = null;
    private JComboBox jComboBoxRePackage = null;
    private StarLabel starLabel = null;
    private StarLabel starLabel1 = null;
    private SpdFileContents sfc = null;
    private OpeningPackageType docConsole = null;

    /**
     * This is the default constructor
     */
    public SpdPackageDefinitions() {
        super();
        initialize();
    }
    
    public SpdPackageDefinitions(PackageSurfaceAreaDocument.PackageSurfaceArea inPsa){
        this();
        sfc = new SpdFileContents(inPsa);
        init(sfc);
    }
    
    public SpdPackageDefinitions(OpeningPackageType opt) {
        this(opt.getXmlSpd());
        docConsole = opt;
    }

    private void init(SpdFileContents sfc) {
        if (sfc.getSpdPkgDefsRdOnly() != null) {
            jComboBoxReadOnly.setSelectedItem(sfc.getSpdPkgDefsRdOnly());
        }
        if (sfc.getSpdPkgDefsRePkg() != null) {
            jComboBoxRePackage.setSelectedItem(sfc.getSpdPkgDefsRePkg());
        }
        this.setVisible(true);
    }
    /**
     * This method initializes this
     * 
     * @return void
     */
    private void initialize() {
        this.setSize(300, 200);
        this.setTitle("Package Definitions");
        this.setContentPane(getJContentPane());
    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            starLabel1 = new StarLabel();
            starLabel1.setBounds(new java.awt.Rectangle(4,62,10,20));
            starLabel1.setVisible(true);
            starLabel = new StarLabel();
            starLabel.setBounds(new java.awt.Rectangle(4,22,10,20));
            jLabel1 = new JLabel();
            jLabel1.setPreferredSize(new java.awt.Dimension(65,20));
            jLabel1.setLocation(new java.awt.Point(22,62));
            jLabel1.setSize(new java.awt.Dimension(65,20));
            jLabel1.setText("RePackage");
            jLabel = new JLabel();
            jLabel.setPreferredSize(new java.awt.Dimension(57,20));
            jLabel.setLocation(new java.awt.Point(22,22));
            jLabel.setSize(new java.awt.Dimension(57,20));
            jLabel.setText("Read Only");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(jLabel, null);
            jContentPane.add(getJComboBox(), null);
            jContentPane.add(jLabel1, null);
            jContentPane.add(getJComboBox1(), null);
            jContentPane.add(starLabel, null);
            jContentPane.add(starLabel1, null);
        }
        return jContentPane;
    }

    /**
     * This method initializes jComboBox	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBox() {
        if (jComboBoxReadOnly == null) {
            jComboBoxReadOnly = new JComboBox();
            jComboBoxReadOnly.setBounds(new java.awt.Rectangle(95,22,117,20));
            jComboBoxReadOnly.setPreferredSize(new Dimension(80, 20));
            jComboBoxReadOnly.addItem("true");
            jComboBoxReadOnly.addItem("false");
            jComboBoxReadOnly.setSelectedIndex(1);
            jComboBoxReadOnly.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    if (jComboBoxReadOnly.getSelectedItem().equals(sfc.getSpdPkgDefsRdOnly())) {
                        return;
                    }
                    if (docConsole != null) {
                        docConsole.setSaved(false);
                    }
                    sfc.setSpdPkgDefsRdOnly(jComboBoxReadOnly.getSelectedItem()+"");
                }
            });
        }
        return jComboBoxReadOnly;
    }

    /**
     * This method initializes jComboBox1	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBox1() {
        if (jComboBoxRePackage == null) {
            jComboBoxRePackage = new JComboBox();
            jComboBoxRePackage.setBounds(new java.awt.Rectangle(95,62,116,20));
            jComboBoxRePackage.setPreferredSize(new Dimension(80, 20));
            jComboBoxRePackage.addItem("false");
            jComboBoxRePackage.addItem("true");
            jComboBoxRePackage.setSelectedIndex(0);
            jComboBoxRePackage.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    if (jComboBoxRePackage.getSelectedItem().equals(sfc.getSpdPkgDefsRePkg())) {
                        return;
                    }
                    if (docConsole != null) {
                        docConsole.setSaved(false);
                    }
                    sfc.setSpdPkgDefsRePkg(jComboBoxRePackage.getSelectedItem()+"");
                }
            });
        }
        return jComboBoxRePackage;
    }

}
