/** @file
  Java class PackageGuids is GUI for create GUID elements of spd file.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.packaging;

import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.DefaultListModel;
import javax.swing.JPanel;
import javax.swing.JLabel;
import javax.swing.JTextField;
import javax.swing.JRadioButton;
import javax.swing.JComboBox;
import javax.swing.JButton;
import javax.swing.JFrame;

import javax.swing.JScrollPane;
import javax.swing.JList;

import org.tianocore.common.Tools;
import org.tianocore.packaging.common.ui.StarLabel;

/**
 GUI for create GUID elements of spd file
  
 @since PackageEditor 1.0
**/
public class PackageGuids extends JFrame implements ActionListener {

    private SpdFileContents sfc = null;

    private static String separator = "::";

    private JPanel jContentPane = null;

    private JLabel jLabelC_Name = null;

    private JTextField jTextFieldC_Name = null;

    private JLabel jLabelGuidValue = null;

    private JTextField jTextFieldGuidValue = null;

    private JLabel jLabelHelpText = null;

    private JTextField jTextFieldName = null;

    private JLabel jLabelEnableFeature = null;

    private JRadioButton jRadioButtonEnableFeature = null;

    private JRadioButton jRadioButtonDisableFeature = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JButton jButtonGenerateGuid = null;

    private StarLabel starLabel = null;

    private StarLabel starLabel1 = null;

    /**
      This method initializes this
      
     **/
    private void initialize() {
        this.setTitle("Guid Declarations");
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);

    }

    /**
      This method initializes jTextFieldC_Name	
      	
      @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldC_Name() {
        if (jTextFieldC_Name == null) {
            jTextFieldC_Name = new JTextField();
            jTextFieldC_Name.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
        }
        return jTextFieldC_Name;
    }

    /**
      This method initializes jTextFieldGuidValsue	
      	
      @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldGuidValsue() {
        if (jTextFieldGuidValue == null) {
            jTextFieldGuidValue = new JTextField();
            jTextFieldGuidValue.setBounds(new java.awt.Rectangle(160, 35, 240, 20));
        }
        return jTextFieldGuidValue;
    }

    /**
      This method initializes jTextFieldName	
      	
      @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldName() {
        if (jTextFieldName == null) {
            jTextFieldName = new JTextField();
            jTextFieldName.setBounds(new java.awt.Rectangle(160, 70, 320, 20));
        }
        return jTextFieldName;
    }

    /**
      This method initializes jRadioButtonEnableFeature	
      	
      @return javax.swing.JRadioButton	
     **/
    private JRadioButton getJRadioButtonEnableFeature() {
        if (jRadioButtonEnableFeature == null) {
            jRadioButtonEnableFeature = new JRadioButton();
            jRadioButtonEnableFeature.setText("Enable");
            jRadioButtonEnableFeature.setBounds(new java.awt.Rectangle(160, 104, 90, 20));
            jRadioButtonEnableFeature.setEnabled(false);
            jRadioButtonEnableFeature.setSelected(true);
        }
        return jRadioButtonEnableFeature;
    }

    /**
      This method initializes jRadioButtonDisableFeature	
      	
      @return javax.swing.JRadioButton	
     **/
    private JRadioButton getJRadioButtonDisableFeature() {
        if (jRadioButtonDisableFeature == null) {
            jRadioButtonDisableFeature = new JRadioButton();
            jRadioButtonDisableFeature.setText("Disable");
            jRadioButtonDisableFeature.setEnabled(false);
            jRadioButtonDisableFeature.setBounds(new java.awt.Rectangle(250, 104, 90, 20));
        }
        return jRadioButtonDisableFeature;
    }

    /**
      This method initializes jButtonOk	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setText("OK");
            jButtonOk.setBounds(new java.awt.Rectangle(300, 240, 75, 20));
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
      This method initializes jButtonCancel	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setText("Cancel");
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 240, 74, 20));
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
      This method initializes jButtonGenerateGuid	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonGenerateGuid() {
        if (jButtonGenerateGuid == null) {
            jButtonGenerateGuid = new JButton();
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(405, 35, 75, 20));
            jButtonGenerateGuid.setText("GEN");
            jButtonGenerateGuid.addActionListener(this);
        }
        return jButtonGenerateGuid;
    }

    /**
      This is the default constructor
     **/
    public PackageGuids(SpdFileContents sfc) {
        super();
        initialize();
        init();
        this.setVisible(true);
        this.sfc = sfc;
    }

    /**
      Start the window at the center of screen
     
     **/
    protected void centerWindow(int intWidth, int intHeight) {
        Dimension d = Toolkit.getDefaultToolkit().getScreenSize();
        this.setLocation((d.width - intWidth) / 2, (d.height - intHeight) / 2);
    }

    /**
      Start the window at the center of screen
     
     **/
    protected void centerWindow() {
        centerWindow(this.getSize().width, this.getSize().height);
    }

    /**
      This method initializes this
      
      @return void
     **/
    private void init() {
        this.setSize(500, 300);
        this.setContentPane(getJContentPane());
        this.setTitle("Add Guids");
        this.centerWindow();
        initFrame();
    }

    /**
      This method initializes jContentPane
      
      @return javax.swing.JPanel
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            starLabel1 = new StarLabel();
            starLabel1.setBounds(new java.awt.Rectangle(5, 34, 10, 20));
            starLabel = new StarLabel();
            starLabel.setBounds(new java.awt.Rectangle(6, 10, 10, 20));
            jLabelEnableFeature = new JLabel();
            jLabelEnableFeature.setText("Enable Feature");
            jLabelEnableFeature.setEnabled(false);
            jLabelEnableFeature.setBounds(new java.awt.Rectangle(15, 104, 140, 20));
            jLabelHelpText = new JLabel();
            jLabelHelpText.setText("Name");
            jLabelHelpText.setBounds(new java.awt.Rectangle(15, 70, 140, 20));
            jLabelGuidValue = new JLabel();
            jLabelGuidValue.setText("Guid Value");
            jLabelGuidValue.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelC_Name = new JLabel();
            jLabelC_Name.setText("C_Name");
            jLabelC_Name.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(jLabelC_Name, null);
            jContentPane.add(getJTextFieldC_Name(), null);
            jContentPane.add(jLabelGuidValue, null);
            jContentPane.add(getJTextFieldGuidValsue(), null);

            jContentPane.add(jLabelHelpText, null);
            jContentPane.add(getJTextFieldName(), null);
            jContentPane.add(jLabelEnableFeature, null);
            jContentPane.add(getJRadioButtonEnableFeature(), null);
            jContentPane.add(getJRadioButtonDisableFeature(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonGenerateGuid(), null);

            jContentPane.add(starLabel, null);
            jContentPane.add(starLabel1, null);

            initFrame();

        }
        return jContentPane;
    }

    /**
      This method initializes events groups and usage type
      
     **/
    private void initFrame() {

    }

    public void actionPerformed(ActionEvent arg0) {
        //
        // save and exit
        //
        if (arg0.getSource() == jButtonOk) {

            this.save();
            this.dispose();
        }
        //
        // exit
        //
        if (arg0.getSource() == jButtonCancel) {
            this.dispose();
        }
        //
        // generate a new GUID
        //
        if (arg0.getSource() == jButtonGenerateGuid) {
            jTextFieldGuidValue.setText(Tools.generateUuidString());
        }
        
        if (arg0.getSource() == jRadioButtonEnableFeature) {
            if (jRadioButtonEnableFeature.isSelected()) {
                jRadioButtonDisableFeature.setSelected(false);
            }
            if (!jRadioButtonDisableFeature.isSelected() && !jRadioButtonEnableFeature.isSelected()) {
                jRadioButtonEnableFeature.setSelected(true);
            }
        }

        if (arg0.getSource() == jRadioButtonDisableFeature) {
            if (jRadioButtonDisableFeature.isSelected()) {
                jRadioButtonEnableFeature.setSelected(false);
            }
            if (!jRadioButtonDisableFeature.isSelected() && !jRadioButtonEnableFeature.isSelected()) {
                jRadioButtonDisableFeature.setSelected(true);
            }
        }
    }

    /**
     Add GUID entry to SpdFileContents object with element values from jTextFields*
    **/
    protected void save() {
        try {
            String strName = jTextFieldName.getText();
            String strCName = jTextFieldC_Name.getText();
            String strGuid = jTextFieldGuidValue.getText();
            sfc.genSpdGuidDeclarations(strName, strCName, strGuid, null);

        } catch (Exception e) {
            System.out.println(e.toString());
        }
    }
} //  @jve:decl-index=0:visual-constraint="10,10"
