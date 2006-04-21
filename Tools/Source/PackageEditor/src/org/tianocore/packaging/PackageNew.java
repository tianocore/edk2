/** @file
  Java class PackageNew is the top level GUI for create spd file.
 
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
import java.text.SimpleDateFormat;
import java.util.Date;

import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JLabel;
import javax.swing.JTextField;
import javax.swing.JButton;
import javax.swing.JTextArea;
import javax.swing.JScrollPane;
import javax.swing.JComboBox;

import org.tianocore.common.Tools;
import org.tianocore.packaging.common.ui.StarLabel;

/**
 This class contains GUI components to show various GUIs for creating spd file elements
  
 @since PackageEditor 1.0
**/
public class PackageNew extends JFrame implements ActionListener {

    private JPanel jContentPane = null; //  @jve:decl-index=0:visual-constraint="128,4"

    private JLabel jLabelBaseName = null;

    private JTextField jTextFieldBaseName = null;

    private JLabel jLabelGuid = null;

    private JTextField jTextFieldGuid = null;

    private JLabel jLabelVersion = null;

    private JTextField jTextFieldVersion = null;

    private JButton jButtonGenerateGuid = null;

    private JLabel jLabelLicense = null;

    private JTextArea jTextAreaLicense = null;

    private JLabel jLabelCopyright = null;

    private JTextArea jTextAreaCopyright = null;

    private JLabel jLabelDescription = null;

    private JTextArea jTextAreaDescription = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JScrollPane jScrollPaneLicense = null;

    private JScrollPane jScrollPaneDescription = null;

    private JLabel jLabelAbstract = null;

    private JTextField jTextFieldAbstract = null;

    private JLabel jLabelModuleType = null;

    private JLabel jLabelCompontentType = null;

    private JComboBox jComboBox1 = null;

    private JComboBox jComboBoxModuleType = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private StarLabel jStarLabel3 = null;

    private StarLabel jStarLabel4 = null;

    private StarLabel jStarLabel5 = null;

    private StarLabel jStarLabel6 = null;

    private StarLabel jStarLabel7 = null;

    private StarLabel jStarLabel8 = null;

    private JLabel jLabelURL = null;

    private JTextField jTextFieldAbstractURL = null;

    private JLabel jLabel = null;

    private JComboBox jComboBox = null;

    private SpdFileContents sfc = null;

    /**
     This method initializes this
     
     **/
    private void initialize() {
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);

    }

    /**
     This method initializes jTextFieldBaseName	
     	
     @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldBaseName() {
        if (jTextFieldBaseName == null) {
            jTextFieldBaseName = new JTextField();
            jTextFieldBaseName.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
        }
        return jTextFieldBaseName;
    }

    /**
     This method initializes jTextFieldGuid	
     	
     @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldGuid() {
        if (jTextFieldGuid == null) {
            jTextFieldGuid = new JTextField();
            jTextFieldGuid.setBounds(new java.awt.Rectangle(160, 35, 240, 20));
        }
        return jTextFieldGuid;
    }

    /**
     This method initializes jTextFieldVersion	
     	
     @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldVersion() {
        if (jTextFieldVersion == null) {
            jTextFieldVersion = new JTextField();
            jTextFieldVersion.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
        }
        return jTextFieldVersion;
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
     This method initializes jTextAreaLicense	
     	
     @return javax.swing.JTextArea	
     **/
    private JTextArea getJTextAreaLicense() {
        if (jTextAreaLicense == null) {
            jTextAreaLicense = new JTextArea();
            jTextAreaLicense.setText("");
            jTextAreaLicense.setLineWrap(true);
        }
        return jTextAreaLicense;
    }

    /**
     This method initializes jTextAreaCopyright	
     	
     @return javax.swing.JTextArea	
     **/
    private JTextArea getJTextAreaCopyright() {
        if (jTextAreaCopyright == null) {
            jTextAreaCopyright = new JTextArea();
            jTextAreaCopyright.setLineWrap(true);
            jTextAreaCopyright.setBounds(new java.awt.Rectangle(160,172,319,20));
        }
        return jTextAreaCopyright;
    }

    /**
     This method initializes jTextAreaDescription	
     	
     @return javax.swing.JTextArea	
     **/
    private JTextArea getJTextAreaDescription() {
        if (jTextAreaDescription == null) {
            jTextAreaDescription = new JTextArea();
            jTextAreaDescription.setLineWrap(true);
        }
        return jTextAreaDescription;
    }

    /**
     This method initializes jButtonNext	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setText("OK");
            jButtonOk.setBounds(new java.awt.Rectangle(290, 481, 90, 20));
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
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 481, 90, 20));
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jScrollPane	
     	
     @return javax.swing.JScrollPane	
     **/
    private JScrollPane getJScrollPaneLicense() {
        if (jScrollPaneLicense == null) {
            jScrollPaneLicense = new JScrollPane();
            jScrollPaneLicense.setBounds(new java.awt.Rectangle(160, 85, 320, 80));
            jScrollPaneLicense.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jScrollPaneLicense.setViewportView(getJTextAreaLicense());
        }
        return jScrollPaneLicense;
    }

    /**
     This method initializes jScrollPane2	
     	
     @return javax.swing.JScrollPane	
     **/
    private JScrollPane getJScrollPaneDescription() {
        if (jScrollPaneDescription == null) {
            jScrollPaneDescription = new JScrollPane();
            jScrollPaneDescription.setBounds(new java.awt.Rectangle(160, 322, 320, 80));
            jScrollPaneDescription.setViewportView(getJTextAreaDescription());
            jScrollPaneDescription.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
        }
        return jScrollPaneDescription;
    }

    /**
     This method initializes jTextFieldAbstract	
     	
     @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldAbstract() {
        if (jTextFieldAbstract == null) {
            jTextFieldAbstract = new JTextField();
            jTextFieldAbstract.setBounds(new java.awt.Rectangle(159,218,318,70));
        }
        return jTextFieldAbstract;
    }

    /**
     This method initializes jComboBoxCompontentType	
     	
     @return javax.swing.JComboBox	
     **/
    private JComboBox getJComboBox1() {
        if (jComboBox1 == null) {
            jComboBox1 = new JComboBox();
            jComboBox1.setBounds(new java.awt.Rectangle(160, 465, 91, 20));
        }
        return jComboBox1;
    }

    /**
     This method initializes jComboBoxModuleType	
     	
     @return javax.swing.JComboBox	
     **/
    private JComboBox getJComboBoxModuleType() {
        if (jComboBoxModuleType == null) {
            jComboBoxModuleType = new JComboBox();
            jComboBoxModuleType.setBounds(new java.awt.Rectangle(160, 440, 91, 20));
        }
        return jComboBoxModuleType;
    }

    /**
     This method initializes jTextFieldAbstractURL	
     	
     @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldAbstractURL() {
        if (jTextFieldAbstractURL == null) {
            jTextFieldAbstractURL = new JTextField();
            jTextFieldAbstractURL.setBounds(new java.awt.Rectangle(159, 414, 320, 20));
        }
        return jTextFieldAbstractURL;
    }

    public PackageNew(SpdFileContents sfc) {
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
        this.setSize(500, 560);
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
        this.setContentPane(getJContentPane());
        this.setTitle("SPD File Header");
        this.centerWindow();
        //this.getRootPane().setDefaultButton(jButtonOk);
        initFrame();
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(15, 490, 140, 21));
            jLabel.setText("Re-Package");
            jLabelURL = new JLabel();
            jLabelURL.setBounds(new java.awt.Rectangle(16, 414, 25, 20));
            jLabelURL.setText("URL");
            jLabelCompontentType = new JLabel();
            jLabelCompontentType.setBounds(new java.awt.Rectangle(15, 465, 140, 20));
            jLabelCompontentType.setText("Read Only");
            jLabelModuleType = new JLabel();
            jLabelModuleType.setBounds(new java.awt.Rectangle(15, 440, 140, 20));
            jLabelModuleType.setText("Package Type");
            jLabelAbstract = new JLabel();
            jLabelAbstract.setBounds(new java.awt.Rectangle(15,218,140,20));
            jLabelAbstract.setText("Abstract");
            jLabelDescription = new JLabel();
            jLabelDescription.setText("Description");
            jLabelDescription.setBounds(new java.awt.Rectangle(16, 325, 140, 20));
            jLabelCopyright = new JLabel();
            jLabelCopyright.setText("Copyright");
            jLabelCopyright.setBounds(new java.awt.Rectangle(15, 171, 140, 20));
            jLabelLicense = new JLabel();
            jLabelLicense.setText("License");
            jLabelLicense.setBounds(new java.awt.Rectangle(15, 85, 140, 20));
            jLabelVersion = new JLabel();
            jLabelVersion.setText("Version");
            jLabelVersion.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelGuid = new JLabel();
            jLabelGuid.setPreferredSize(new java.awt.Dimension(25, 15));
            jLabelGuid.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelGuid.setText("Guid");
            jLabelBaseName = new JLabel();
            jLabelBaseName.setText("Package Name");
            jLabelBaseName.setBounds(new java.awt.Rectangle(15, 10, 140, 20));
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setLocation(new java.awt.Point(0, 0));
            jContentPane.setSize(new java.awt.Dimension(500, 524));
            jContentPane.add(jLabelBaseName, null);
            jContentPane.add(getJTextFieldBaseName(), null);
            jContentPane.add(jLabelGuid, null);
            jContentPane.add(getJTextFieldGuid(), null);
            jContentPane.add(jLabelVersion, null);
            jContentPane.add(getJTextFieldVersion(), null);
            jContentPane.add(getJButtonGenerateGuid(), null);
            jContentPane.add(jLabelLicense, null);
            jContentPane.add(jLabelCopyright, null);
            jContentPane.add(jLabelDescription, null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJScrollPaneLicense(), null);
            jContentPane.add(getJScrollPaneDescription(), null);
            jContentPane.add(jLabelAbstract, null);
            jContentPane.add(getJTextFieldAbstract(), null);
            jContentPane.add(jLabelModuleType, null);
            jContentPane.add(jLabelCompontentType, null);
            jContentPane.add(getJComboBox1(), null);
            jContentPane.add(getJComboBoxModuleType(), null);

            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(0, 35));
            jStarLabel3 = new StarLabel();
            jStarLabel3.setLocation(new java.awt.Point(0, 60));
            jStarLabel4 = new StarLabel();
            jStarLabel4.setLocation(new java.awt.Point(0, 85));
            jStarLabel5 = new StarLabel();
            jStarLabel5.setLocation(new java.awt.Point(0, 171));
            jStarLabel6 = new StarLabel();
            jStarLabel6.setLocation(new java.awt.Point(1, 325));
            jStarLabel7 = new StarLabel();
            jStarLabel7.setLocation(new java.awt.Point(0,218));
            jStarLabel8 = new StarLabel();
            jStarLabel8.setLocation(new java.awt.Point(0, 440));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jStarLabel3, null);
            jContentPane.add(jStarLabel4, null);
            jContentPane.add(jStarLabel5, null);
            jContentPane.add(jStarLabel6, null);
            jContentPane.add(jStarLabel7, null);
            jContentPane.add(jStarLabel8, null);
            jContentPane.add(jLabelURL, null);
            jContentPane.add(getJTextFieldAbstractURL(), null);
            jContentPane.add(jLabel, null);
            jContentPane.add(getJComboBox(), null);
            jContentPane.add(getJTextAreaCopyright(), null);
        }
        return jContentPane;
    }

    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonOk) {
            this.save();
            this.dispose();
        }

        if (arg0.getSource() == jButtonCancel) {
            this.dispose();
        }

        if (arg0.getSource() == jButtonGenerateGuid) {
            jTextFieldGuid.setText(Tools.generateUuidString());
        }
    }

    /**
     Save all components of Msa Header
     if exist, set the value directly
     if not exist, new instance first
     
     **/
    private void save() {
        SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd HH:mm");
        Date date = new Date();
        sfc.genSpdHeader(jTextFieldBaseName.getText(), jTextFieldGuid.getText(), jTextFieldVersion.getText(),
                         jTextFieldAbstract.getText(), jTextAreaDescription.getText(), jTextAreaCopyright.getText(),
                         jTextAreaLicense.getText(), format.format(date), format.format(date),
                         jTextFieldAbstractURL.getText(), jComboBoxModuleType.getSelectedItem().toString(),
                         jComboBox1.getSelectedItem().toString(), jComboBox.getSelectedItem().toString(), null, null);
    }

    /**
     This method initializes module type and compontent type
     
     **/
    private void initFrame() {
        jComboBoxModuleType.addItem("SOURCE");
        jComboBoxModuleType.addItem("BINARY");
        jComboBoxModuleType.addItem("MIXED");

        jComboBox1.addItem("true");
        jComboBox1.addItem("false");

        jComboBox.addItem("false");
        jComboBox.addItem("true");

    }

    /**
     This method initializes jComboBox	
     	
     @return javax.swing.JComboBox	
     **/
    private JComboBox getJComboBox() {
        if (jComboBox == null) {
            jComboBox = new JComboBox();
            jComboBox.setBounds(new java.awt.Rectangle(160, 490, 90, 20));
        }
        return jComboBox;
    }

} //  @jve:decl-index=0:visual-constraint="38,-22"
