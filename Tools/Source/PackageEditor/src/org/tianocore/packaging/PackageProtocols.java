/** @file
  Java class PackageProtocols is GUI for create Protocol definition elements of spd file.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.packaging;

import javax.swing.JPanel;
import javax.swing.JLabel;
import javax.swing.JTextField;
import javax.swing.JButton;
import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JComboBox;
import javax.swing.JRadioButton;
import javax.swing.JFrame;

import org.tianocore.common.Tools;
import org.tianocore.packaging.common.ui.StarLabel;

/**
 GUI for create Protocol definition elements of spd file.
  
 @since PackageEditor 1.0
**/
public class PackageProtocols extends JFrame implements ActionListener {

    private int location = -1;

    private SpdFileContents sfc = null;

    private JPanel jContentPane = null;

    private JLabel jLabelC_Name = null;

    private JTextField jTextFieldC_Name = null;

    private JLabel jLabelGuid = null;

    private JTextField jTextFieldGuid = null;

    private JLabel jLabelFeatureFlag = null;

    private JTextField jTextFieldFeatureFlag = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JLabel jLabelEnableFeature = null;

    private JRadioButton jRadioButtonEnableFeature = null;

    private JRadioButton jRadioButtonDisableFeature = null;

    private JButton jButtonGenerateGuid = null;

    private StarLabel jStarLabel2 = null;

    private StarLabel starLabel = null;

    private JLabel jLabel = null;

    private JTextField jTextField = null;

    /**
     This method initializes this
     
     **/
    private void initialize() {
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);

    }

    /**
     This method initializes jTextFieldProtocolName	
     	
     @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldProtocolName() {
        if (jTextFieldC_Name == null) {
            jTextFieldC_Name = new JTextField();
            jTextFieldC_Name.setBounds(new java.awt.Rectangle(160, 35, 320, 20));
        }
        return jTextFieldC_Name;
    }

    /**
     This method initializes jTextFieldGuid	
     	
     @return javax.swing.JTextField	
     **/
    public JTextField getJTextFieldGuid() {
        if (jTextFieldGuid == null) {
            jTextFieldGuid = new JTextField();
            jTextFieldGuid.setBounds(new java.awt.Rectangle(160, 60, 240, 20));
        }
        return jTextFieldGuid;
    }

    /**
     This method initializes jTextFieldFeatureFlag	
     	
     @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldFeatureFlag() {
        if (jTextFieldFeatureFlag == null) {
            jTextFieldFeatureFlag = new JTextField();
            jTextFieldFeatureFlag.setBounds(new java.awt.Rectangle(160, 135, 320, 20));
            jTextFieldFeatureFlag.setEnabled(false);
        }
        return jTextFieldFeatureFlag;
    }

    /**
     This method initializes jButton	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setText("OK");
            jButtonOk.setBounds(new java.awt.Rectangle(290, 190, 90, 20));
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
     This method initializes jButton1	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setText("Cancel");
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 190, 90, 20));
            jButtonCancel.setPreferredSize(new Dimension(90, 20));
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jRadioButtonEnableFeature	
     	
     @return javax.swing.JRadioButton	
     **/
    private JRadioButton getJRadioButtonEnableFeature() {
        if (jRadioButtonEnableFeature == null) {
            jRadioButtonEnableFeature = new JRadioButton();
            jRadioButtonEnableFeature.setText("Enable");
            jRadioButtonEnableFeature.setBounds(new java.awt.Rectangle(160, 110, 90, 20));
            jRadioButtonEnableFeature.setEnabled(false);
            jRadioButtonEnableFeature.addActionListener(this);
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
            jRadioButtonDisableFeature.setBounds(new java.awt.Rectangle(320, 110, 90, 20));
            jRadioButtonDisableFeature.addActionListener(this);
        }
        return jRadioButtonDisableFeature;
    }

    /**
     This method initializes jButtonGenerateGuid	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButtonGenerateGuid() {
        if (jButtonGenerateGuid == null) {
            jButtonGenerateGuid = new JButton();
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(405, 60, 75, 20));
            jButtonGenerateGuid.setText("GEN");
            jButtonGenerateGuid.addActionListener(this);
        }
        return jButtonGenerateGuid;
    }

    /**
     This is the default constructor
     **/
    public PackageProtocols(SpdFileContents sfc) {
        super();
        initialize();
        init();
        this.setVisible(true);
        this.sfc = sfc;
    }

    /**
     This method initializes this
     
     @return void
     **/
    private void init() {
        this.setSize(500, 250);
        this.setName("JFrame");
        this.setContentPane(getJContentPane());
        this.setTitle("Add Protocols");
        this.centerWindow();
        //this.getRootPane().setDefaultButton(jButtonOk);
        initFrame();
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
     This method initializes jContentPane
     
     @return javax.swing.JPanel
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(16, 10, 138, 16));
            jLabel.setText("Name");
            starLabel = new StarLabel();
            starLabel.setBounds(new java.awt.Rectangle(0, 9, 10, 20));
            jLabelEnableFeature = new JLabel();
            jLabelEnableFeature.setText("Enable Feature");
            jLabelEnableFeature.setEnabled(false);
            jLabelEnableFeature.setBounds(new java.awt.Rectangle(15, 110, 140, 20));
            jLabelFeatureFlag = new JLabel();
            jLabelFeatureFlag.setText("Feature Flag");
            jLabelFeatureFlag.setEnabled(false);
            jLabelFeatureFlag.setBounds(new java.awt.Rectangle(15, 135, 140, 20));
            jLabelGuid = new JLabel();
            jLabelGuid.setText("Guid");
            jLabelGuid.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelC_Name = new JLabel();
            jLabelC_Name.setText("C_Name");
            jLabelC_Name.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(jLabelC_Name, null);
            jContentPane.add(getJTextFieldProtocolName(), null);
            jContentPane.add(jLabelGuid, null);
            jContentPane.add(getJTextFieldGuid(), null);
            jContentPane.add(jLabelFeatureFlag, null);
            jContentPane.add(getJTextFieldFeatureFlag(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(jLabelEnableFeature, null);
            jContentPane.add(getJRadioButtonEnableFeature(), null);
            jContentPane.add(getJRadioButtonDisableFeature(), null);
            jContentPane.add(getJButtonGenerateGuid(), null);

            jStarLabel2 = new StarLabel();
            jStarLabel2.setBounds(new java.awt.Rectangle(0, 35, 10, 20));

            jContentPane.add(jStarLabel2, null);
            jContentPane.add(starLabel, null);
            jContentPane.add(jLabel, null);
            jContentPane.add(getJTextField(), null);
        }
        return jContentPane;
    }

    /**
     This method initializes protocol usage type
     
     **/
    private void initFrame() {

    }

    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonOk) {

            this.save();
            this.dispose();
        }
        if (arg0.getSource() == jButtonCancel) {
            this.dispose();
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

        if (arg0.getSource() == jButtonGenerateGuid) {
            jTextFieldGuid.setText(Tools.generateUuidString());
        }
    }

    protected void save() {
        try {
            sfc.genSpdProtocolDeclarations(jTextField.getText(), jTextFieldC_Name.getText(), jTextFieldGuid.getText(),
                                           null);
        } catch (Exception e) {
            System.out.println(e.toString());
        }
    }

    /**
     This method initializes jTextField	
     	
     @return javax.swing.JTextField	
     **/
    public JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setBounds(new java.awt.Rectangle(160, 8, 319, 23));
        }
        return jTextField;
    }

    public JTextField getJTextFieldC_Name() {
        return jTextFieldC_Name;
    }

    public void setJTextFieldC_Name(JTextField textFieldC_Name) {
        jTextFieldC_Name = textFieldC_Name;
    }

    public void setJTextField(JTextField textField) {
        jTextField = textField;
    }

    public void setJTextFieldGuid(JTextField textFieldGuid) {
        jTextFieldGuid = textFieldGuid;
    }
} //  @jve:decl-index=0:visual-constraint="10,10"
