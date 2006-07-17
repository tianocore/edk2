/** @file
 
 The file is used to create, update spdHeader of Spd file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.packaging.ui;

import java.awt.event.ActionEvent;
import java.awt.event.ComponentEvent;
import java.awt.event.FocusAdapter;
import java.awt.event.FocusEvent;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;


import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPackageType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;

/**
 The class is used to create, update spdHeader of Spd file
 It extends IInternalFrame
 
 @since PackageEditor 1.0

 **/
public class SpdHeader extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -8152099582923006900L;

    //
    //Define class members
    //
    static JFrame frame;
    
    private JPanel jContentPane = null;

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

    private JLabel jLabelDescription = null;

    private JTextArea jTextAreaDescription = null;

    private JLabel jLabelSpecification = null;

    private JTextField jTextFieldSpecification = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JScrollPane jScrollPaneLicense = null;

    private JScrollPane jScrollPaneDescription = null;

    private JLabel jLabelAbstract = null;

    private JTextField jTextFieldAbstract = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private StarLabel jStarLabel3 = null;

    private StarLabel jStarLabel4 = null;

    private StarLabel jStarLabel5 = null;

    private StarLabel jStarLabel6 = null;

    private StarLabel jStarLabel7 = null;

    private StarLabel jStarLabel9 = null;
    
    private SpdFileContents sfc = null;
    
    private OpeningPackageType docConsole = null;

    private JTextField jTextFieldCopyright = null;
    
    private JScrollPane topScrollPane = null;

    private JLabel jLabel = null;

    private JTextField jTextField = null;

    /**
     This method initializes jTextFieldBaseName 
     
     @return javax.swing.JTextField jTextFieldBaseName
     
     **/
    private JTextField getJTextFieldBaseName() {
        if (jTextFieldBaseName == null) {
            jTextFieldBaseName = new JTextField();
            jTextFieldBaseName.setBounds(new java.awt.Rectangle(160, 10, 320, 20));
            jTextFieldBaseName.setPreferredSize(new java.awt.Dimension(320,20));
            jTextFieldBaseName.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e){
                    if (!DataValidation.isUiNameType(jTextFieldBaseName.getText())) {
                        JOptionPane.showMessageDialog(frame, "Package Name is NOT UiNameType.");
                        return;
                    }
                    docConsole.setSaved(false);
                    sfc.setSpdHdrPkgName(jTextFieldBaseName.getText());
                }
            });
        }
        return jTextFieldBaseName;
    }

    /**
     This method initializes jTextFieldGuid 
     
     @return javax.swing.JTextField jTextFieldGuid
     
     **/
    private JTextField getJTextFieldGuid() {
        if (jTextFieldGuid == null) {
            jTextFieldGuid = new JTextField();
            jTextFieldGuid.setBounds(new java.awt.Rectangle(160, 35, 250, 20));
            jTextFieldGuid.setPreferredSize(new java.awt.Dimension(250,20));
            jTextFieldGuid.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e){
                    if (!DataValidation.isGuid(jTextFieldGuid.getText())) {
                        JOptionPane.showMessageDialog(frame, "Guid is NOT GuidType.");
                        return;
                    }
                    docConsole.setSaved(false);
                    sfc.setSpdHdrGuidValue(jTextFieldGuid.getText());
                }
            });
        }
        return jTextFieldGuid;
    }

    /**
     This method initializes jTextFieldVersion 
     
     @return javax.swing.JTextField jTextFieldVersion
     
     **/
    private JTextField getJTextFieldVersion() {
        if (jTextFieldVersion == null) {
            jTextFieldVersion = new JTextField();
            jTextFieldVersion.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
            jTextFieldVersion.setPreferredSize(new java.awt.Dimension(320,20));
            jTextFieldVersion.addFocusListener(new FocusAdapter(){
               public void focusLost(FocusEvent e){
                   if (!DataValidation.isVersion(jTextFieldVersion.getText())) {
                       JOptionPane.showMessageDialog(frame, "Version is NOT version type.");
                       return;
                   }
                   docConsole.setSaved(false);
                   sfc.setSpdHdrVer(jTextFieldVersion.getText());
               } 
            });
        }
        return jTextFieldVersion;
    }

    /**
     This method initializes jButtonGenerateGuid 
     
     @return javax.swing.JButton jButtonGenerateGuid
     
     **/
    private JButton getJButtonGenerateGuid() {
        if (jButtonGenerateGuid == null) {
            jButtonGenerateGuid = new JButton();
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(415, 35, 65, 20));
            jButtonGenerateGuid.setText("GEN");
            jButtonGenerateGuid.addActionListener(this);
        }
        return jButtonGenerateGuid;
    }

    /**
     This method initializes jTextAreaLicense 
     
     @return javax.swing.JTextArea jTextAreaLicense
     
     **/
    private JTextArea getJTextAreaLicense() {
        if (jTextAreaLicense == null) {
            jTextAreaLicense = new JTextArea();
            jTextAreaLicense.setText("");
//            jTextAreaLicense.setPreferredSize(new java.awt.Dimension(317,77));
            jTextAreaLicense.setLineWrap(true);
            jTextAreaLicense.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e){
                    if (jTextAreaLicense.getText().length() == 0) {
                        JOptionPane.showMessageDialog(frame, "License contents could NOT be empty.");
                        return;
                    }
                    docConsole.setSaved(false);
                    sfc.setSpdHdrLicense(jTextAreaLicense.getText());
                }
            });
        }
        return jTextAreaLicense;
    }

    /**
     This method initializes jTextAreaDescription 
     
     @return javax.swing.JTextArea jTextAreaDescription
     
     **/
    private JTextArea getJTextAreaDescription() {
        if (jTextAreaDescription == null) {
            jTextAreaDescription = new JTextArea();
            jTextAreaDescription.setLineWrap(true);
//            jTextAreaDescription.setPreferredSize(new java.awt.Dimension(317,77));
            jTextAreaDescription.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e){
                    if (jTextAreaDescription.getText().length() == 0) {
                        JOptionPane.showMessageDialog(frame, "Description contents could NOT be empty.");
                        return;
                    }
                    docConsole.setSaved(false);
                    sfc.setSpdHdrDescription(jTextAreaDescription.getText());
                }
            });
        }
        return jTextAreaDescription;
    }

    /**
     This method initializes jTextFieldSpecification 
     
     @return javax.swing.JTextField jTextFieldSpecification
     
     **/
    private JTextField getJTextFieldSpecification() {
        if (jTextFieldSpecification == null) {
            jTextFieldSpecification = new JTextField();
            jTextFieldSpecification.setBounds(new java.awt.Rectangle(161,330,320,20));
            jTextFieldSpecification.setEditable(false);
            jTextFieldSpecification.setPreferredSize(new java.awt.Dimension(320,20));
            jTextFieldSpecification.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e){
                    sfc.setSpdHdrSpec(jTextFieldSpecification.getText());
                }
            });
        }
        return jTextFieldSpecification;
    }

    /**
     This method initializes jButtonOk 
     
     @return javax.swing.JButton jButtonOk
     
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setText("OK");
            jButtonOk.setBounds(new java.awt.Rectangle(290, 445, 90, 20));
            jButtonOk.setVisible(false);
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
     This method initializes jButtonCancel 
     
     @return javax.swing.JButton jButtonCancel
     
     **/
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setText("Cancel");
            jButtonCancel.setBounds(new java.awt.Rectangle(390, 445, 90, 20));
            jButtonCancel.setVisible(false);
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jScrollPaneLicense 
     
     @return javax.swing.JScrollPane jScrollPaneLicense
     
     **/
    private JScrollPane getJScrollPaneLicense() {
        if (jScrollPaneLicense == null) {
            jScrollPaneLicense = new JScrollPane();
            jScrollPaneLicense.setBounds(new java.awt.Rectangle(161,108,320,80));
            jScrollPaneLicense.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jScrollPaneLicense.setPreferredSize(new java.awt.Dimension(320,80));
            jScrollPaneLicense.setViewportView(getJTextAreaLicense());
        }
        return jScrollPaneLicense;
    }

    /**
     This method initializes jScrollPaneDescription 
     
     @return javax.swing.JScrollPane jScrollPaneDescription
     
     **/
    private JScrollPane getJScrollPaneDescription() {
        if (jScrollPaneDescription == null) {
            jScrollPaneDescription = new JScrollPane();
            jScrollPaneDescription.setBounds(new java.awt.Rectangle(160,244,320,80));
            jScrollPaneDescription.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jScrollPaneDescription.setViewportView(getJTextAreaDescription());
        }
        return jScrollPaneDescription;
    }

    /**
     This method initializes jTextFieldAbstract 
     
     @return javax.swing.JTextField jTextFieldAbstract
     
     **/
    private JTextField getJTextFieldAbstract() {
        if (jTextFieldAbstract == null) {
            jTextFieldAbstract = new JTextField();
            jTextFieldAbstract.setBounds(new java.awt.Rectangle(161,220,320,20));
            jTextFieldAbstract.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextFieldAbstract.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e){
                    if (!DataValidation.isAbstract(jTextFieldAbstract.getText())) {
                        JOptionPane.showMessageDialog(frame, "Abstract could NOT be empty.");
                        return;
                    }
                    docConsole.setSaved(false);
                    sfc.setSpdHdrAbs(jTextFieldAbstract.getText());
                }
            });
        }
        return jTextFieldAbstract;
    }

    /**
      This method initializes jTextFieldCopyright	
      	
      @return javax.swing.JTextField jTextFieldCopyright
     
     **/
    private JTextField getJTextFieldCopyright() {
        if (jTextFieldCopyright == null) {
            jTextFieldCopyright = new JTextField();
            jTextFieldCopyright.setBounds(new java.awt.Rectangle(160,86,320,20));
            jTextFieldCopyright.setPreferredSize(new java.awt.Dimension(320,20));
            jTextFieldCopyright.addFocusListener(new FocusAdapter(){
               public void focusLost(FocusEvent e){
                   if (!DataValidation.isCopyright(jTextFieldCopyright.getText())) {
                       JOptionPane.showMessageDialog(frame, "Copyright contents could not be empty.");
                       return;
                   }
                   docConsole.setSaved(false);
                   sfc.setSpdHdrCopyright(jTextFieldCopyright.getText());
               } 
            });
        }
        return jTextFieldCopyright;
    }

    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setBounds(new java.awt.Rectangle(161,195,320,20));
            jTextField.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextField.addFocusListener(new FocusAdapter(){
               public void focusLost(FocusEvent e){
                   sfc.setSpdHdrLicense(jTextAreaLicense.getText());
                   sfc.setSpdHdrUrl(jTextField.getText());
                   docConsole.setSaved(false);
               } 
            });
        }
        return jTextField;
    }

    public static void main(String[] args) {
        new SpdHeader().setVisible(true);
    }

    /**
     This is the default constructor
     
     **/
    public SpdHeader() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inspdHeader The input data of spdHeaderDocument.spdHeader
     
     **/
    public SpdHeader(PackageSurfaceAreaDocument.PackageSurfaceArea inPsa) {
        this();
        sfc = new SpdFileContents(inPsa);
        init(sfc);
    }
    
    public SpdHeader(OpeningPackageType opt){
        this(opt.getXmlSpd());
        docConsole = opt;
    }
    /**
     This method initializes this
     
     **/
    private void init() {
        this.setContentPane(getTopScrollPane());
        this.setTitle("Package Surface Area Header");
        initFrame();
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inspdHeader  The input data of spdHeaderDocument.spdHeader
     
     **/
    private void init(SpdFileContents sfc) {
            if (sfc.getSpdHdrPkgName() != null) {
                jTextFieldBaseName.setText(sfc.getSpdHdrPkgName());
            }
            if (sfc.getSpdHdrGuidValue() != null) {
                jTextFieldGuid.setText(sfc.getSpdHdrGuidValue());
            }
            if (sfc.getSpdHdrVer() != null) {
               jTextFieldVersion.setText(sfc.getSpdHdrVer());
            }
            if (sfc.getSpdHdrLicense() != null) {
                jTextAreaLicense.setText(sfc.getSpdHdrLicense());
            }
            if (sfc.getSpdHdrUrl() != null) {
                jTextField.setText(sfc.getSpdHdrUrl());
            }
            if (sfc.getSpdHdrCopyright() != null) {
                jTextFieldCopyright.setText(sfc.getSpdHdrCopyright());
            }
            if (sfc.getSpdHdrAbs() != null) {
                jTextFieldAbstract.setText(sfc.getSpdHdrAbs());
            }
            if (sfc.getSpdHdrDescription() != null) {
                jTextAreaDescription.setText(sfc.getSpdHdrDescription());
            }
            if (sfc.getSpdHdrSpec() != null) {
                jTextFieldSpecification.setText(sfc.getSpdHdrSpec());
            }
            sfc.setSpdHdrSpec(jTextFieldSpecification.getText());
            
            if (!sfc.getSpdPkgDefsRdOnly().equals("true")) {
                sfc.setSpdPkgDefsRdOnly("false");
            }
            if (!sfc.getSpdPkgDefsRePkg().equals("true")) {
                sfc.setSpdPkgDefsRePkg("false");
            }
            
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
        	jLabel = new JLabel();
        	jLabel.setBounds(new java.awt.Rectangle(16,195,140,20));
        	jLabel.setText("URL");
        	jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setLocation(new java.awt.Point(0, 0));
            jContentPane.setPreferredSize(new java.awt.Dimension(500, 524));
            jLabelAbstract = new JLabel();
            jLabelAbstract.setBounds(new java.awt.Rectangle(16,245,140,20));
            jLabelAbstract.setText("Description");
            jLabelSpecification = new JLabel();
            jLabelSpecification.setText("Specification");
            jLabelSpecification.setBounds(new java.awt.Rectangle(16,330,140,20));
            jLabelDescription = new JLabel();
            jLabelDescription.setText("Abstract");
            jLabelDescription.setBounds(new java.awt.Rectangle(16,220,140,20));
            jLabelCopyright = new JLabel();
            jLabelCopyright.setText("Copyright");
            jLabelCopyright.setBounds(new java.awt.Rectangle(15,86,140,20));
            jLabelLicense = new JLabel();
            jLabelLicense.setText("License");
            jLabelLicense.setBounds(new java.awt.Rectangle(16,110,140,20));
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
            jContentPane.add(jLabelSpecification, null);
            jContentPane.add(getJTextFieldSpecification(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJScrollPaneLicense(), null);
            jContentPane.add(getJScrollPaneDescription(), null);
            jContentPane.add(jLabelAbstract, null);
            jContentPane.add(getJTextFieldAbstract(), null);
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, 10));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(0, 35));
            jStarLabel3 = new StarLabel();
            jStarLabel3.setLocation(new java.awt.Point(0, 60));
            jStarLabel4 = new StarLabel();
            jStarLabel4.setLocation(new java.awt.Point(1,110));
            jStarLabel5 = new StarLabel();
            jStarLabel5.setLocation(new java.awt.Point(0,86));
            jStarLabel6 = new StarLabel();
            jStarLabel6.setLocation(new java.awt.Point(1,220));
            jStarLabel7 = new StarLabel();
            jStarLabel7.setLocation(new java.awt.Point(1,330));
            jStarLabel7.setEnabled(false);
            jStarLabel9 = new StarLabel();
            jStarLabel9.setLocation(new java.awt.Point(1,245));
            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jStarLabel3, null);
            jContentPane.add(jStarLabel4, null);
            jContentPane.add(jStarLabel5, null);
            jContentPane.add(jStarLabel6, null);
            jContentPane.add(jStarLabel7, null);
            jContentPane.add(jStarLabel9, null);
            jContentPane.add(getJTextFieldCopyright(), null);

            jContentPane.add(jLabel, null);
            jContentPane.add(getJTextField(), null);
        }
        return jContentPane;
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     *
     * Override actionPerformed to listen all actions
     *
     */
    public void actionPerformed(ActionEvent arg0) {
        docConsole.setSaved(false);
        if (arg0.getSource() == jButtonOk) {
            this.save();
            this.setEdited(true);
        }
        if (arg0.getSource() == jButtonCancel) {
            this.setEdited(false);
        }
        if (arg0.getSource() == jButtonGenerateGuid) {
            //ToDo: invoke GuidValueEditor
            jTextFieldGuid.setText(Tools.generateUuidString());
            sfc.setSpdHdrGuidValue(jTextFieldGuid.getText());
        }
    }

    /**
     Data validation for all fields
     
     @retval true - All datas are valid
     @retval false - At least one data is invalid
     
     **/
    public boolean check() {
        //
        // Check if all required fields are not empty
        //
        if (isEmpty(this.jTextFieldBaseName.getText())) {
            Log.err("Base Name couldn't be empty");
            return false;
        }
        if (isEmpty(this.jTextFieldGuid.getText())) {
            Log.err("Guid couldn't be empty");
            return false;
        }
        if (isEmpty(this.jTextFieldVersion.getText())) {
            Log.err("Version couldn't be empty");
            return false;
        }
        if (isEmpty(this.jTextAreaLicense.getText())) {
            Log.err("License couldn't be empty");
            return false;
        }
        if (isEmpty(this.jTextFieldCopyright.getText())) {
            Log.err("Copyright couldn't be empty");
            return false;
        }
        if (isEmpty(this.jTextAreaDescription.getText())) {
            Log.err("Description couldn't be empty");
            return false;
        }
        if (isEmpty(this.jTextFieldAbstract.getText())) {
            Log.err("Abstract couldn't be empty");
            return false;
        }

        //
        // Check if all fields have correct data types 
        //
        if (!DataValidation.isBaseName(this.jTextFieldBaseName.getText())) {
            Log.err("Incorrect data type for Base Name");
            return false;
        }
        if (!DataValidation.isGuid((this.jTextFieldGuid).getText())) {
            Log.err("Incorrect data type for Guid");
            return false;
        }
        if (!DataValidation.isAbstract(this.jTextFieldAbstract.getText())) {
            Log.err("Incorrect data type for Abstract");
            return false;
        }
        if (!DataValidation.isCopyright(this.jTextFieldCopyright.getText())) {
            Log.err("Incorrect data type for Copyright");
            return false;
        }
        return true;
    }

    /**
     Save all components of Spd Header
     if exists spdHeader, set the value directly
     if not exists spdHeader, new an instance first
     
     **/
    public void save() {
        try {
            
        } catch (Exception e) {
            Log.err("Save Package", e.getMessage());
        }
    }

    /**
     This method initializes Package type and Compontent type
     
     **/
    private void initFrame() {
        
    }

	/* (non-Javadoc)
	 * @see java.awt.event.ComponentListener#componentResized(java.awt.event.ComponentEvent)
	 * 
	 * Override componentResized to resize all components when frame's size is changed
	 */
	public void componentResized(ComponentEvent arg0) {
        int intPreferredWidth = 500;
        int intCurrentWidth = this.getJContentPane().getWidth();
        
		resizeComponentWidth(this.jTextFieldBaseName, intCurrentWidth,intPreferredWidth);
		resizeComponentWidth(this.jTextFieldGuid, intCurrentWidth,intPreferredWidth);
		resizeComponentWidth(this.jTextFieldVersion, intCurrentWidth,intPreferredWidth);
        resizeComponentWidth(this.jTextField, intCurrentWidth,intPreferredWidth);
		resizeComponentWidth(this.jScrollPaneLicense, intCurrentWidth,intPreferredWidth);
		resizeComponentWidth(this.jTextFieldCopyright, intCurrentWidth,intPreferredWidth);
		resizeComponentWidth(this.jScrollPaneDescription, intCurrentWidth,intPreferredWidth);
		resizeComponentWidth(this.jTextFieldSpecification, intCurrentWidth,intPreferredWidth);
		resizeComponentWidth(this.jTextFieldAbstract, intCurrentWidth,intPreferredWidth);
		relocateComponentX(this.jButtonGenerateGuid, intCurrentWidth, jButtonGenerateGuid.getWidth(),30);
	}

    private JScrollPane getTopScrollPane() {
        if (topScrollPane == null){
            topScrollPane = new JScrollPane();
            topScrollPane.setViewportView(getJContentPane());
        }
        return topScrollPane;
    }
}
