/** @file
  Java class PackagePCD is GUI for create PCD definition elements of spd file.
 
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

import javax.swing.ButtonGroup;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JLabel;
import javax.swing.JTextField;
import javax.swing.JComboBox;
import javax.swing.JButton;
import javax.swing.JFrame;

import org.tianocore.common.Tools;
import org.tianocore.packaging.common.ui.StarLabel;

/**
 GUI for create PCD definition elements of spd file
  
 @since PackageEditor 1.0
**/
public class PackagePCD extends JFrame implements ActionListener {

    private JPanel jContentPane = null;

    private JLabel jLabelItemType = null;

    private JLabel jLabelC_Name = null;

    private JComboBox jComboBoxItemType = null;

    private JTextField jTextFieldC_Name = null;

    private JLabel jLabelToken = null;

    private JTextField jTextFieldToken = null;

    private JLabel jLabelDataType = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JComboBox jComboBoxDataType = null;

    private JLabel jLabelOverrideID = null;

    private JTextField jTextFieldOverrideID = null;

    private ButtonGroup bg1 = null;

    private ButtonGroup bg2 = null;

    private ButtonGroup bg3 = null;

    private StarLabel jStarLabel2 = null;

    private StarLabel jStarLabel3 = null;

    private SpdFileContents sfc = null;

    private StarLabel jStarLabel = null;

    private StarLabel jStarLabel1 = null;

    /**
     This method initializes this
     
     **/
    private void initialize() {
        this.setTitle("PCD Definition");
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);

    }

    /**
     This method initializes jComboBoxItemType	
     	
     @return javax.swing.JComboBox	
     **/
    private JComboBox getJComboBoxItemType() {
        if (jComboBoxItemType == null) {
            jComboBoxItemType = new JComboBox();
            jComboBoxItemType.setBounds(new java.awt.Rectangle(160, 110, 320, 20));
        }
        return jComboBoxItemType;
    }

    /**
     This method initializes jTextFieldC_Name	
     	
     @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldC_Name() {
        if (jTextFieldC_Name == null) {
            jTextFieldC_Name = new JTextField();
            jTextFieldC_Name.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
        }
        return jTextFieldC_Name;
    }

    /**
     This method initializes jTextFieldToken	
     	
     @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldToken() {
        if (jTextFieldToken == null) {
            jTextFieldToken = new JTextField();
            jTextFieldToken.setBounds(new java.awt.Rectangle(160, 135, 320, 20));
        }
        return jTextFieldToken;
    }

    /**
     This method initializes jButtonOk	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setText("OK");
            jButtonOk.setBounds(new java.awt.Rectangle(279,247,90,20));
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
            jButtonCancel.setBounds(new java.awt.Rectangle(389,247,90,20));
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jComboBoxDataType	
     	
     @return javax.swing.JComboBox	
     **/
    private JComboBox getJComboBoxDataType() {
        if (jComboBoxDataType == null) {
            jComboBoxDataType = new JComboBox();
            jComboBoxDataType.setBounds(new java.awt.Rectangle(160, 160, 320, 20));
        }
        return jComboBoxDataType;
    }

    /**
     This method initializes jTextFieldOverrideID	
     	
     @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldOverrideID() {
        if (jTextFieldOverrideID == null) {
            jTextFieldOverrideID = new JTextField();
            jTextFieldOverrideID.setBounds(new java.awt.Rectangle(159,198,320,20));
        }
        return jTextFieldOverrideID;
    }

    /**
     This is the default constructor
     **/
    public PackagePCD(SpdFileContents sfc) {
        super();
        init();
        initialize();
        this.sfc = sfc;
    }

    /**
     This method initializes this
     
     @return void
     **/
    private void init() {
        this.setSize(500, 450);
        this.setContentPane(getJContentPane());
        this.setTitle("Add PCDs");
        this.centerWindow();
        this.getRootPane().setDefaultButton(jButtonOk);
        initFrame();
        this.setVisible(true);
    }

    /**
     Start the window at the center of screen
     *
     **/
    protected void centerWindow(int intWidth, int intHeight) {
        Dimension d = Toolkit.getDefaultToolkit().getScreenSize();
        this.setLocation((d.width - intWidth) / 2, (d.height - intHeight) / 2);
    }

    /**
     Start the window at the center of screen
     *
     **/
    protected void centerWindow() {
        centerWindow(this.getSize().width, this.getSize().height);
    }

    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabelOverrideID = new JLabel();
            jLabelOverrideID.setBounds(new java.awt.Rectangle(14,197,140,20));
            jLabelOverrideID.setText("Default Value");
            jLabelC_Name = new JLabel();
            jLabelC_Name.setText("C_Name");
            jLabelC_Name.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelDataType = new JLabel();
            jLabelDataType.setText("Data Type");
            jLabelDataType.setBounds(new java.awt.Rectangle(15, 160, 140, 20));
            jLabelToken = new JLabel();
            jLabelToken.setText("Token");
            jLabelToken.setBounds(new java.awt.Rectangle(15, 135, 140, 20));
            jLabelItemType = new JLabel();
            jLabelItemType.setText("Item Type");
            jLabelItemType.setBounds(new java.awt.Rectangle(15, 110, 140, 20));
            bg1 = new ButtonGroup();
            bg2 = new ButtonGroup();
            bg3 = new ButtonGroup();
            //bg1.add(getJRadioButtonPCData());
            //bg2.add(getJRadioButtonPcdBuildData());
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            //jContentPane.add(bg1);
            jContentPane.add(jLabelItemType, null);
            jContentPane.add(jLabelC_Name, null);
            jContentPane.add(getJTextFieldC_Name(), null);
            jContentPane.add(jLabelToken, null);
            jContentPane.add(getJTextFieldToken(), null);
            jContentPane.add(jLabelDataType, null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJComboBoxItemType(), null);
            jContentPane.add(getJComboBoxDataType(), null);
            jContentPane.add(jLabelOverrideID, null);
            jContentPane.add(getJTextFieldOverrideID(), null);

            jStarLabel = new StarLabel();
            jStarLabel1 = new StarLabel();
            jStarLabel1.setBounds(new java.awt.Rectangle(6, 59, 10, 20));
            jStarLabel2 = new StarLabel();
            jStarLabel3 = new StarLabel();
            jStarLabel.setLocation(new java.awt.Point(6, 110));
            jStarLabel.setLocation(new java.awt.Point(5, 85));
            jStarLabel2.setLocation(new java.awt.Point(5, 134));
            jStarLabel3.setLocation(new java.awt.Point(5, 159));
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jStarLabel3, null);
            jContentPane.add(jStarLabel, null);
            jContentPane.add(jStarLabel1, null);
        }
        return jContentPane;
    }

    /**
     This method initializes comboboxes			
     
     **/
    private void initFrame() {

        jComboBoxItemType.addItem("FEATURE_FLAG");
        jComboBoxItemType.addItem("FIXED_AT_BUILD");
        jComboBoxItemType.addItem("PATCHABLE_IN_MODULE");
        jComboBoxItemType.addItem("DYNAMIC");
        jComboBoxItemType.addItem("DYNAMIC_EX");

        jComboBoxDataType.addItem("UINT8");
        jComboBoxDataType.addItem("UINT16");
        jComboBoxDataType.addItem("UINT32");
        jComboBoxDataType.addItem("UINT64");
        jComboBoxDataType.addItem("VOID*");
        jComboBoxDataType.addItem("BOOLEAN");
    }

    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonOk) {
            save();
            this.dispose();
        }
        if (arg0.getSource() == jButtonCancel) {
            this.dispose();
        }
        

    }

    protected void save() {
        
        sfc.genSpdPcdDefinitions(jComboBoxItemType.getSelectedItem().toString(), jTextFieldC_Name.getText(),
                                 jTextFieldToken.getText(), jComboBoxDataType.getSelectedItem().toString(), null,
                                 null, null, null, null, null, jTextFieldOverrideID.getText());
    }
} //  @jve:decl-index=0:visual-constraint="22,11"
