/** @file
 
 The file is used to create, update Module Definitions of MSA file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.module.ui;

import java.awt.event.ComponentEvent;
import java.awt.event.FocusEvent;

import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JLabel;
import javax.swing.JTextField;
import javax.swing.JComboBox;

import org.tianocore.ModuleDefinitionsDocument;
import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.ModuleDefinitionsDocument.ModuleDefinitions.ClonedFrom;
import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.EnumerationData;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningModuleType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;

public class ModuleDefinitions extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = 5860378543553036323L;

    private JScrollPane jScrollPane = null;

    private JPanel jContentPane = null;

    private JLabel jLabelArch = null;

    private ICheckBoxList iCheckBoxListArch = null;

    private JLabel jLabelBinaryModule = null;

    private JComboBox jComboBoxBinaryModule = null;

    private JLabel jLabelOutputFileBasename = null;

    private JTextField jTextFieldOutputFileBasename = null;

    private JScrollPane jScrollPaneArch = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private StarLabel jStarLabel3 = null;
    
    private OpeningModuleType omt = null;
    
    private ClonedFrom cf = null;

    private ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = null;

    private ModuleDefinitionsDocument.ModuleDefinitions md = null;

    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setViewportView(getJContentPane());
        }
        return jScrollPane;
    }

    /**
     * This method initializes jPanel	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelOutputFileBasename = new JLabel();
            jLabelOutputFileBasename.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jLabelOutputFileBasename.setText("Output File Basename");
            jLabelBinaryModule = new JLabel();
            jLabelBinaryModule.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelBinaryModule.setText("Binary Module");
            jLabelArch = new JLabel();
            jLabelArch.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelArch.setText("Supported Architectures");
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(0, 35));
            jStarLabel3 = new StarLabel();
            jStarLabel3.setLocation(new java.awt.Point(0, 60));

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setPreferredSize(new java.awt.Dimension(490, 150));

            jContentPane.add(jLabelArch, null);
            jContentPane.add(getJScrollPaneArch(), null);
            jContentPane.add(jLabelBinaryModule, null);
            jContentPane.add(getJComboBoxBinaryModule(), null);
            jContentPane.add(jLabelOutputFileBasename, null);
            jContentPane.add(getJTextFieldOutputFileBasename(), null);
            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jStarLabel3, null);
        }
        return jContentPane;
    }

    /**
     This method initializes iCheckBoxListArch	
     
     @return ICheckBoxList	
     **/
    private ICheckBoxList getICheckBoxListSupportedArchitectures() {
        if (iCheckBoxListArch == null) {
            iCheckBoxListArch = new ICheckBoxList();
            iCheckBoxListArch.addFocusListener(this);
            iCheckBoxListArch.setToolTipText(DataType.SUP_ARCH_LIST_HELP_TEXT);
        }
        return iCheckBoxListArch;
    }

    /**
     * This method initializes jComboBoxBinaryModule	
     * 	
     * @return javax.swing.JComboBox	
     */
    private JComboBox getJComboBoxBinaryModule() {
        if (jComboBoxBinaryModule == null) {
            jComboBoxBinaryModule = new JComboBox();
            jComboBoxBinaryModule.setBounds(new java.awt.Rectangle(160, 35, 320, 20));
            jComboBoxBinaryModule.setPreferredSize(new java.awt.Dimension(320, 20));
            jComboBoxBinaryModule.addFocusListener(this);
            jComboBoxBinaryModule.setToolTipText("Modules are either source modules which can be compiled or binary modules which are linked.  A module cannot contain both.  The GUID numbers should be identical for a binary and source MSA, but the BINARY MSA should have a higher version number.");
        }
        return jComboBoxBinaryModule;
    }

    /**
     * This method initializes jTextFieldOutputFileBasename	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldOutputFileBasename() {
        if (jTextFieldOutputFileBasename == null) {
            jTextFieldOutputFileBasename = new JTextField();
            jTextFieldOutputFileBasename.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
            jTextFieldOutputFileBasename.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextFieldOutputFileBasename.addFocusListener(this);
            jTextFieldOutputFileBasename.setToolTipText("Enter a single word for generated output file names");
        }
        return jTextFieldOutputFileBasename;
    }

    /**
     This method initializes jScrollPaneArch	
     
     @return javax.swing.JScrollPane	
     
     **/
    private JScrollPane getJScrollPaneArch() {
        if (jScrollPaneArch == null) {
            jScrollPaneArch = new JScrollPane();
            jScrollPaneArch.setBounds(new java.awt.Rectangle(160, 60, 320, 80));
            jScrollPaneArch.setPreferredSize(new java.awt.Dimension(320, 60));
            jScrollPaneArch.setViewportView(getICheckBoxListSupportedArchitectures());
        }
        return jScrollPaneArch;
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
    public ModuleDefinitions() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inMsa
     
     **/
    public ModuleDefinitions(OpeningModuleType inOmt) {
        super();
        this.omt = inOmt;
        this.msa = omt.getXmlMsa();
        if (msa.getModuleDefinitions() != null) {
            this.cf = msa.getModuleDefinitions().getClonedFrom();    
        }
        init(msa.getModuleDefinitions());
        this.setVisible(true);
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
    private void init() {
        this.setContentPane(getJScrollPane());
        this.setTitle("Module Definitions");
        initFrame();
        this.setPreferredSize(new java.awt.Dimension(490, 520));
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inMsaHeader  The input data of MsaHeaderDocument.MsaHeader
     
     **/
    private void init(ModuleDefinitionsDocument.ModuleDefinitions inMd) {
        init();
        if (inMd != null) {
            this.md = inMd;
            if (md.getSupportedArchitectures() != null) {
                this.iCheckBoxListArch.initCheckedItem(true, Tools.convertListToVector(md.getSupportedArchitectures()));
            }
            if (md.getBinaryModule()) {
                this.jComboBoxBinaryModule.setSelectedIndex(1);
            } else {
                this.jComboBoxBinaryModule.setSelectedIndex(0);
            }
            if (md.getOutputFileBasename() != null) {
                this.jTextFieldOutputFileBasename.setText(md.getOutputFileBasename());
            }
        }
    }

    /**
     This method initializes Module type and Compontent type
     
     **/
    private void initFrame() {
        EnumerationData ed = new EnumerationData();
        this.iCheckBoxListArch.setAllItems(ed.getVSupportedArchitectures());
        Tools.generateComboBoxByVector(jComboBoxBinaryModule, ed.getVBoolean());
    }
    
    private boolean check() {
        if (isEmpty(this.jTextFieldOutputFileBasename.getText())) {
            Log.err("Output File Basename couldn't be empty!");
            return false;
        }
        if (!DataValidation.isOutputFileBasename(this.jTextFieldOutputFileBasename.getText())) {
            Log.err("Incorrect data type for Output File Basename");
            return false;
        }
        return true;
    }

    /* (non-Javadoc)
     * @see java.awt.event.ComponentListener#componentResized(java.awt.event.ComponentEvent)
     * 
     * Override componentResized to resize all components when frame's size is changed
     */
    public void componentResized(ComponentEvent arg0) {
        int intCurrentWidth = this.getJContentPane().getWidth();
        int intPreferredWidth = this.getJContentPane().getPreferredSize().width;

        resizeComponentWidth(this.jScrollPaneArch, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jComboBoxBinaryModule, intCurrentWidth, intPreferredWidth);
        resizeComponentWidth(this.jTextFieldOutputFileBasename, intCurrentWidth, intPreferredWidth);
    }

    /**
     Save all components of Module Definitions
     if exists Module Definitions, set the value directly
     if not exists Module Definitions, new an instance first
     
     **/
    public void save() {
        check();
        try {
            if (this.md == null) {
                md = ModuleDefinitionsDocument.ModuleDefinitions.Factory.newInstance();
            }

            if (!isEmpty(this.jTextFieldOutputFileBasename.getText())) {
                md.setOutputFileBasename(this.jTextFieldOutputFileBasename.getText());
            }

            if (this.jComboBoxBinaryModule.getSelectedIndex() == 0) {
                md.setBinaryModule(false);
            } else {
                md.setBinaryModule(true);
            }
            
            //
            // Set ClonedFrom field
            //
            if (this.cf != null) {
                md.setClonedFrom(this.cf);
            }

            //
            // Save Arch list
            //
            md.setSupportedArchitectures(this.iCheckBoxListArch.getAllCheckedItemsString());
            
            msa.setModuleDefinitions(md);
            
            this.omt.setSaved(false);

        } catch (Exception e) {
            Log.err("Save Module Definitions", e.getMessage());
        }
    }

    public void focusLost(FocusEvent arg0) {
        this.save();
        if (arg0.getSource() == this.jTextFieldOutputFileBasename) {
        }
    }
}
