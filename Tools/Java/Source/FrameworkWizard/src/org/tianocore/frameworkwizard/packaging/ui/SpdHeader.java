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
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;

import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPackageType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import javax.swing.JCheckBox;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;

/**
 The class is used to create, update spdHeader of Spd file
 It extends IInternalFrame
 
 @since PackageEditor 1.0

 **/
public class SpdHeader extends IInternalFrame implements DocumentListener{

    private int dialogWidth = 560;

    private int labelColumn = 12;

    private int labelWidth = 155;

    private int buttonWidth = 60;

    private final int valueColumn = 168;

    private final int valueWidth = 320;
    
    private final int specWidth = 420;
    
    private int shortValueWidth = valueWidth - (buttonWidth + 5);

    private final int oneRowHeight = 20;

    private final int threeRowHeight = 60;

    private final int fourRowHeight = 80;

    private final int rowSep = 5;

    private final int rowOne = 12;

    private final int rowTwo = rowOne + oneRowHeight + rowSep;

    private final int rowThree = rowTwo + oneRowHeight + rowSep;

    private final int rowFour = rowThree + oneRowHeight + rowSep;

    private final int rowFive = rowFour + threeRowHeight + rowSep;

    private final int rowSix = rowFive + fourRowHeight + rowSep;

    private final int rowSeven = rowSix + oneRowHeight + rowSep;

    private final int rowEight = rowSeven + oneRowHeight + rowSep;

    private final int rowNine = rowEight + fourRowHeight + rowSep;

    private final int rowTen = rowNine + oneRowHeight + rowSep;

    private int dialogHeight = rowTen + threeRowHeight;

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -8152099582923006900L;

    //
    //Define class members
    //
    private IInternalFrame topFrame;

    private JPanel jContentPane = null;

    private JLabel jPackageNameLabel = null;

    private JTextField jPackageNameTextField = null;

    private JLabel jGuidLabel = null;

    private JTextField jGuidTextField = null;

    private JLabel jVersionLabel = null;

    private JTextField jVersionTextField = null;

    private JButton jGenerateGuidButton = null;

    private JLabel jLicenseLabel = null;

    private JTextArea jLicenseTextArea = null;

    private JLabel jLabelCopyright = null;

    private JLabel jDescriptionLabel = null;

    private JTextArea jDescriptionTextArea = null;

    private JTextField jSpecificationTextField = null;

    private JScrollPane jLicenseScrollPane = null;

    private JScrollPane jDescriptionScrollPane = null;
    
    private JScrollPane jCopyrightScrollPane = null;

    private JLabel jAbstractLabel = null;

    private JTextField jAbstractTextField = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private StarLabel jStarLabel3 = null;

    private StarLabel jStarLabel4 = null;

    private StarLabel jStarLabel5 = null;

    private StarLabel jStarLabel6 = null;

    private StarLabel jStarLabel7 = null;
    
    private StarLabel jStarLabel8 = null;

    private StarLabel jStarLabel9 = null;

    private SpdFileContents sfc = null;

    private OpeningPackageType docConsole = null;

    private JTextArea jCopyrightTextArea = null;

    private JScrollPane topScrollPane = null;

    private JLabel jUrlLabel = null;

    private JTextField jUrlTextField = null;

    private JCheckBox jCheckBoxRdOnly = null;

    private JCheckBox jCheckBoxRePkg = null;

    /**
     This method initializes jPackageNameTextField 
     
     @return javax.swing.JTextField jPackageNameTextField
     
     **/
    private JTextField getJPackageNameTextField() {
        if (jPackageNameTextField == null) {
            jPackageNameTextField = new JTextField();
            jPackageNameTextField.setBounds(new java.awt.Rectangle(valueColumn, rowOne, valueWidth, oneRowHeight));
            jPackageNameTextField.setPreferredSize(new java.awt.Dimension(valueWidth, oneRowHeight));
            jPackageNameTextField.getDocument().addDocumentListener(this);
            jPackageNameTextField.addFocusListener(new FocusAdapter() {
                public void focusLost(FocusEvent e) {
                    if (!DataValidation.isUiNameType(jPackageNameTextField.getText())) {
                        JOptionPane.showMessageDialog(topFrame, "Package Name must start with a letter.");
                        return;
                    }
                    if (jPackageNameTextField.getText().equals(sfc.getSpdHdrPkgName())) {
                        return;
                    }
                    sfc.setSpdHdrPkgName(jPackageNameTextField.getText());
                }
            });
        }
        return jPackageNameTextField;
    }

    /**
     This method initializes jGuidTextField 
     
     @return javax.swing.JTextField jGuidTextField
     
     **/
    private JTextField getJGuidTextField() {
        if (jGuidTextField == null) {
            jGuidTextField = new JTextField();
            jGuidTextField.setBounds(new java.awt.Rectangle(valueColumn, rowTwo, shortValueWidth, oneRowHeight));
            jGuidTextField.setPreferredSize(new java.awt.Dimension(shortValueWidth, oneRowHeight));
            jGuidTextField.getDocument().addDocumentListener(this);
            jGuidTextField.addFocusListener(new FocusAdapter() {
                public void focusLost(FocusEvent e) {
                    if (!DataValidation.isGuid(jGuidTextField.getText())) {
                        JOptionPane.showMessageDialog(topFrame, "Guid must be in registry (8-4-4-4-12) format.");
                        return;
                    }
                    if (jGuidTextField.getText().equals(sfc.getSpdHdrGuidValue())) {
                        return;
                    }
                    sfc.setSpdHdrGuidValue(jGuidTextField.getText());
                }
            });
        }
        return jGuidTextField;
    }

    /**
     This method initializes jVersionTextField 
     
     @return javax.swing.JTextField jVersionTextField
     
     **/
    private JTextField getJVersionTextField() {
        if (jVersionTextField == null) {
            jVersionTextField = new JTextField();
            jVersionTextField.setBounds(new java.awt.Rectangle(valueColumn, rowThree, valueWidth, oneRowHeight));
            jVersionTextField.setPreferredSize(new java.awt.Dimension(valueWidth, oneRowHeight));
            jVersionTextField.getDocument().addDocumentListener(this);
            jVersionTextField.addFocusListener(new FocusAdapter() {
                public void focusLost(FocusEvent e) {
                    if (!DataValidation.isVersion(jVersionTextField.getText())) {
                        JOptionPane.showMessageDialog(topFrame, "Version must start with a number.");
                        return;
                    }
                    if (jVersionTextField.getText().equals(sfc.getSpdHdrVer())) {
                        return;
                    }
                    sfc.setSpdHdrVer(jVersionTextField.getText());
                }
            });
        }
        return jVersionTextField;
    }

    /**
     This method initializes jGenerateGuidButton 
     
     @return javax.swing.JButton jGenerateGuidButton
     
     **/
    private JButton getJGenerateGuidButton() {
        if (jGenerateGuidButton == null) {
            jGenerateGuidButton = new JButton();
            jGenerateGuidButton.setBounds(new java.awt.Rectangle(valueColumn + shortValueWidth + 5, rowTwo, buttonWidth, oneRowHeight));
            jGenerateGuidButton.setText("GEN");
            jGenerateGuidButton.addActionListener(this);
        }
        return jGenerateGuidButton;
    }

    /**
     This method initializes jLicenseTextArea 
     
     @return javax.swing.JTextArea jLicenseTextArea
     
     **/
    private JTextArea getJLicenseTextArea() {
        if (jLicenseTextArea == null) {
            jLicenseTextArea = new JTextArea();
            jLicenseTextArea.setLineWrap(true);
            jLicenseTextArea.getDocument().addDocumentListener(this);
            jLicenseTextArea.addFocusListener(new FocusAdapter() {
                public void focusLost(FocusEvent e) {
                    if (jLicenseTextArea.getText().length() == 0) {
                        JOptionPane.showMessageDialog(topFrame, "License is a required field.");
                        return;
                    }
                    if (jLicenseTextArea.getText().equals(sfc.getSpdHdrLicense())) {
                        return;
                    }
                    sfc.setSpdHdrLicense(jLicenseTextArea.getText());
                }
            });
        }
        return jLicenseTextArea;
    }

    /**
     This method initializes jDescriptionTextArea 
     
     @return javax.swing.JTextArea jDescriptionTextArea
     
     **/
    private JTextArea getJDescriptionTextArea() {
        if (jDescriptionTextArea == null) {
            jDescriptionTextArea = new JTextArea();
            jDescriptionTextArea.setLineWrap(true);
            jDescriptionTextArea.getDocument().addDocumentListener(this);
            jDescriptionTextArea.addFocusListener(new FocusAdapter() {
                public void focusLost(FocusEvent e) {
                    if (jDescriptionTextArea.getText().length() == 0) {
                        JOptionPane
                                   .showMessageDialog(topFrame,
                                                      "Description is a required field, and should reflect the contents of the package.");
                        return;
                    }
                    if (jDescriptionTextArea.getText().equals(sfc.getSpdHdrDescription())) {
                        return;
                    }
                    sfc.setSpdHdrDescription(jDescriptionTextArea.getText());
                }
            });
        }
        return jDescriptionTextArea;
    }

    /**
     This method initializes jSpecificationTextField 
     
     @return javax.swing.JTextField jSpecificationTextField
     
     **/
    private JTextField getJSpecificationTextField() {
        if (jSpecificationTextField == null) {
            jSpecificationTextField = new JTextField();
            jSpecificationTextField.setBounds(new java.awt.Rectangle(labelColumn, dialogHeight - 40, specWidth, oneRowHeight));
            jSpecificationTextField.setEditable(false);
            jSpecificationTextField.setPreferredSize(new java.awt.Dimension(specWidth, oneRowHeight));
            jSpecificationTextField.setLocation(new java.awt.Point(2, dialogHeight - oneRowHeight));
            jSpecificationTextField.setBorder(null);

            //            jSpecificationTextField.addFocusListener(new FocusAdapter(){
            //                public void focusLost(FocusEvent e){
            //                    sfc.setSpdHdrSpec(jSpecificationTextField.getText());
            //                }
            //            });
        }
        return jSpecificationTextField;
    }

    /**
     This method initializes jLicenseScrollPane 
     
     @return javax.swing.JScrollPane jLicenseScrollPane
     
     **/
    private JScrollPane getJLicenseScrollPane() {
        if (jLicenseScrollPane == null) {
            jLicenseScrollPane = new JScrollPane();
            jLicenseScrollPane.setBounds(new java.awt.Rectangle(valueColumn, rowFive, valueWidth, fourRowHeight));
            jLicenseScrollPane.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jLicenseScrollPane.setPreferredSize(new java.awt.Dimension(valueWidth, fourRowHeight));
            jLicenseScrollPane.setViewportView(getJLicenseTextArea());
        }
        return jLicenseScrollPane;
    }

    /**
     This method initializes jDescriptionScrollPane 
     
     @return javax.swing.JScrollPane jDescriptionScrollPane
     
     **/
    private JScrollPane getJDescriptionScrollPane() {
        if (jDescriptionScrollPane == null) {
            jDescriptionScrollPane = new JScrollPane();
            jDescriptionScrollPane.setBounds(new java.awt.Rectangle(valueColumn, rowEight, valueWidth, fourRowHeight));
            jDescriptionScrollPane.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jDescriptionScrollPane.setPreferredSize(new java.awt.Dimension(valueWidth, fourRowHeight));
            jDescriptionScrollPane.setViewportView(getJDescriptionTextArea());
        }
        return jDescriptionScrollPane;
    }

    /**
     This method initializes jAbstractTextField 
     
     @return javax.swing.JTextField jAbstractTextField
     
     **/
    private JTextField getJAbstractTextField() {
        if (jAbstractTextField == null) {
            jAbstractTextField = new JTextField();
            jAbstractTextField.setBounds(new java.awt.Rectangle(valueColumn, rowSeven, valueWidth, oneRowHeight));
            jAbstractTextField.setPreferredSize(new java.awt.Dimension(valueWidth, oneRowHeight));
            jAbstractTextField.getDocument().addDocumentListener(this);
            jAbstractTextField.addFocusListener(new FocusAdapter() {
                public void focusLost(FocusEvent e) {
                    if (!DataValidation.isAbstract(jAbstractTextField.getText())) {
                        JOptionPane.showMessageDialog(topFrame, "Abstract could NOT be empty.");
                        return;
                    }
                    if (jAbstractTextField.getText().equals(sfc.getSpdHdrAbs())) {
                        return;
                    }
                    sfc.setSpdHdrAbs(jAbstractTextField.getText());
                }
            });
        }
        return jAbstractTextField;
    }

    private JScrollPane getCopyrightScrollPane() {
        if (jCopyrightScrollPane == null) {
            jCopyrightScrollPane = new JScrollPane();
            jCopyrightScrollPane.setBounds(new java.awt.Rectangle(valueColumn, rowFour, valueWidth, threeRowHeight));
            jCopyrightScrollPane.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jCopyrightScrollPane.setPreferredSize(new java.awt.Dimension(valueWidth, threeRowHeight));
            jCopyrightScrollPane.setViewportView(getJCopyrightTextArea());
        }
        return jCopyrightScrollPane;
    }
    /**
     This method initializes jTextFieldCopyright	
     
     @return javax.swing.JTextField jTextFieldCopyright
     
     **/
    private JTextArea getJCopyrightTextArea() {
        if (jCopyrightTextArea == null) {
            jCopyrightTextArea = new JTextArea();
            jCopyrightTextArea.setWrapStyleWord(true);
            jCopyrightTextArea.setLineWrap(true);
            jCopyrightTextArea.getDocument().addDocumentListener(this);
            jCopyrightTextArea.addFocusListener(new FocusAdapter() {
                public void focusLost(FocusEvent e) {
                    if (!DataValidation.isCopyright(jCopyrightTextArea.getText())) {
                        JOptionPane.showMessageDialog(topFrame, "Copyright must be entered.");
                        return;
                    }
                    if (jCopyrightTextArea.getText().equals(sfc.getSpdHdrCopyright())) {
                        return;
                    }
                    sfc.setSpdHdrCopyright(jCopyrightTextArea.getText());
                }
            });
        }
        return jCopyrightTextArea;
    }

    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJUrlTextField() {
        if (jUrlTextField == null) {
            jUrlTextField = new JTextField();
            jUrlTextField.setBounds(new java.awt.Rectangle(valueColumn, rowSix, valueWidth, oneRowHeight));
            jUrlTextField.setPreferredSize(new java.awt.Dimension(valueWidth, oneRowHeight));
            jUrlTextField.getDocument().addDocumentListener(this);
            jUrlTextField.addFocusListener(new FocusAdapter() {
                public void focusLost(FocusEvent e) {
                    if (jUrlTextField.getText().length() == 0 && sfc.getSpdHdrUrl() == null) {
                        return;
                    }
                    if (jUrlTextField.getText().equals(sfc.getSpdHdrUrl())) {
                        return;
                    }
                    sfc.setSpdHdrLicense(jLicenseTextArea.getText());
                    sfc.setSpdHdrUrl(jUrlTextField.getText());
                }
            });
        }
        return jUrlTextField;
    }

    /**
     * This method initializes jCheckBoxRdOnly	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxRdOnly() {
        if (jCheckBoxRdOnly == null) {
            jCheckBoxRdOnly = new JCheckBox();
            jCheckBoxRdOnly.setText("Read Only");
            jCheckBoxRdOnly.setLocation(new java.awt.Point(labelColumn, rowNine));
            jCheckBoxRdOnly.setSize(new java.awt.Dimension(labelWidth, oneRowHeight));
            jCheckBoxRdOnly.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    if (docConsole != null) {
                        docConsole.setSaved(false);
                    }
                    sfc.setSpdPkgDefsRdOnly(jCheckBoxRdOnly.isSelected()+"");
                    initFrame();
                }
            });
        }
        return jCheckBoxRdOnly;
    }

    /**
     * This method initializes jCheckBoxRePkg	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxRePkg() {
        if (jCheckBoxRePkg == null) {
            jCheckBoxRePkg = new JCheckBox();
            jCheckBoxRePkg.setLocation(new java.awt.Point(labelColumn, rowTen));
            jCheckBoxRePkg.setText("RePackagable");
            jCheckBoxRePkg.setSize(new java.awt.Dimension(labelWidth, oneRowHeight));
            jCheckBoxRePkg.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    if (docConsole != null) {
                        docConsole.setSaved(false);
                    }
                    sfc.setSpdPkgDefsRePkg(jCheckBoxRePkg.isSelected()+"");
                }
            });
        }
        return jCheckBoxRePkg;
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

    public SpdHeader(OpeningPackageType opt) {
        this(opt.getXmlSpd());
        docConsole = opt;
        initFrame();
        topFrame = this;
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setContentPane(getTopScrollPane());
        this.setTitle("Package Surface Area Header");
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inspdHeader  The input data of spdHeaderDocument.spdHeader
     
     **/
    private void init(SpdFileContents sfc) {
        if (sfc.getSpdHdrPkgName() != null) {
            jPackageNameTextField.setText(sfc.getSpdHdrPkgName());
        }
        if (sfc.getSpdHdrGuidValue() != null) {
            jGuidTextField.setText(sfc.getSpdHdrGuidValue());
        }
        if (sfc.getSpdHdrVer() != null) {
            jVersionTextField.setText(sfc.getSpdHdrVer());
        }
        if (sfc.getSpdHdrLicense() != null) {
            jLicenseTextArea.setText(sfc.getSpdHdrLicense());
        }
        if (sfc.getSpdHdrUrl() != null) {
            jUrlTextField.setText(sfc.getSpdHdrUrl());
        }
        if (sfc.getSpdHdrCopyright() != null) {
            jCopyrightTextArea.setText(sfc.getSpdHdrCopyright());
        }
        if (sfc.getSpdHdrAbs() != null) {
            jAbstractTextField.setText(sfc.getSpdHdrAbs());
        }
        if (sfc.getSpdHdrDescription() != null) {
            jDescriptionTextArea.setText(sfc.getSpdHdrDescription());
        }
        if (sfc.getSpdHdrSpec() != null) {
            jSpecificationTextField.setText(sfc.getSpdHdrSpec());
        }
        sfc.setSpdHdrSpec(jSpecificationTextField.getText());

        if (!sfc.getSpdPkgDefsRdOnly().equals("true")) {
            sfc.setSpdPkgDefsRdOnly("false");
            jCheckBoxRdOnly.setSelected(false);
        } else {
            jCheckBoxRdOnly.setSelected(true);
        }
        if (!sfc.getSpdPkgDefsRePkg().equals("true")) {
            sfc.setSpdPkgDefsRePkg("false");
            jCheckBoxRePkg.setSelected(false);
        } else {
            jCheckBoxRePkg.setSelected(true);
        }

    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(2, rowOne));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(2, rowTwo));
            jStarLabel3 = new StarLabel();
            jStarLabel3.setLocation(new java.awt.Point(2, rowThree));
            jStarLabel4 = new StarLabel();
            jStarLabel4.setLocation(new java.awt.Point(2, rowFour));
            jStarLabel5 = new StarLabel();
            jStarLabel5.setLocation(new java.awt.Point(2, rowFive));
            jStarLabel6 = new StarLabel();
            jStarLabel6.setLocation(new java.awt.Point(2, rowSeven));
            jStarLabel7 = new StarLabel();
            jStarLabel7.setLocation(new java.awt.Point(2, rowEight));
            jStarLabel8 = new StarLabel();
            jStarLabel8.setLocation(new java.awt.Point(2, rowNine));
            jStarLabel8.setVisible(false);
            jStarLabel9 = new StarLabel();
            jStarLabel9.setLocation(new java.awt.Point(2, rowTen));

            jStarLabel9.setVisible(false);
            jPackageNameLabel = new JLabel();
            jPackageNameLabel.setText("Package Name");
            jPackageNameLabel.setBounds(new java.awt.Rectangle(labelColumn, rowOne, labelWidth, oneRowHeight));
            jGuidLabel = new JLabel();
            jGuidLabel.setBounds(new java.awt.Rectangle(labelColumn, rowTwo, labelWidth, oneRowHeight));
            jGuidLabel.setText("Guid");
            jVersionLabel = new JLabel();
            jVersionLabel.setText("Version");
            jVersionLabel.setBounds(new java.awt.Rectangle(labelColumn, rowThree, labelWidth, oneRowHeight));
            jLabelCopyright = new JLabel();
            jLabelCopyright.setText("Copyright");
            jLabelCopyright.setBounds(new java.awt.Rectangle(labelColumn, rowFour, labelWidth, oneRowHeight));
            jLicenseLabel = new JLabel();
            jLicenseLabel.setText("License");
            jLicenseLabel.setBounds(new java.awt.Rectangle(labelColumn, rowFive, labelWidth, oneRowHeight));
            jUrlLabel = new JLabel();
            jUrlLabel.setBounds(new java.awt.Rectangle(labelColumn, rowSix, labelWidth, oneRowHeight));
            jUrlLabel.setText("URL");
            jAbstractLabel = new JLabel();
            jAbstractLabel.setBounds(new java.awt.Rectangle(labelColumn, rowSeven, labelWidth, oneRowHeight));
            jAbstractLabel.setText("Abstract");
            jDescriptionLabel = new JLabel();
            jDescriptionLabel.setText("Description");
            jDescriptionLabel.setBounds(new java.awt.Rectangle(labelColumn, rowEight, labelWidth, oneRowHeight));

            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setLocation(new java.awt.Point(0, 0));
            jContentPane.setPreferredSize(new java.awt.Dimension(dialogWidth - 20, dialogHeight - 20));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jStarLabel3, null);
            jContentPane.add(jStarLabel4, null);
            jContentPane.add(jStarLabel5, null);
            jContentPane.add(jStarLabel6, null);
            jContentPane.add(jStarLabel7, null);
            jContentPane.add(jStarLabel8, null);
            jContentPane.add(jStarLabel9, null);

            jContentPane.add(jPackageNameLabel, null);
            jContentPane.add(getJPackageNameTextField(), null);
            jContentPane.add(jGuidLabel, null);
            jContentPane.add(getJGuidTextField(), null);
            jContentPane.add(jVersionLabel, null);
            jContentPane.add(getJVersionTextField(), null);
            jContentPane.add(getJGenerateGuidButton(), null);
            jContentPane.add(jLabelCopyright, null);
            jContentPane.add(getCopyrightScrollPane(), null);
            jContentPane.add(jLicenseLabel, null);
            jContentPane.add(getJLicenseScrollPane(), null);
            jContentPane.add(jUrlLabel, null);
            jContentPane.add(getJUrlTextField(), null);
            jContentPane.add(jAbstractLabel, null);
            jContentPane.add(getJAbstractTextField(), null);
            jContentPane.add(jDescriptionLabel, null);
            jContentPane.add(getJDescriptionScrollPane(), null);

//            jContentPane.add(getJButtonOk(), null);
//            jContentPane.add(getJButtonCancel(), null);

            jContentPane.add(getJCheckBoxRdOnly(), null);
            jContentPane.add(getJCheckBoxRePkg(), null);
            jContentPane.add(getJSpecificationTextField(), null);

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

        if (arg0.getSource() == jGenerateGuidButton) {
            //ToDo: invoke GuidValueEditor
            jGuidTextField.setText(Tools.generateUuidString());
            sfc.setSpdHdrGuidValue(jGuidTextField.getText());
        }
    }

    /**
     This method initializes Package type and Compontent type
     
     **/
    private void initFrame() {
        boolean editable = true;
        if (sfc.getSpdPkgDefsRdOnly().equals("true")) {
            editable = false;
        }
        jPackageNameTextField.setEditable(editable);
        jGuidTextField.setEditable(editable);
        jGenerateGuidButton.setEnabled(editable);
        jVersionTextField.setEditable(editable);
        jCopyrightTextArea.setEditable(editable);
        jLicenseTextArea.setEditable(editable);
        jUrlTextField.setEditable(editable);
        jAbstractTextField.setEditable(editable);
        jDescriptionTextArea.setEditable(editable);
        jCheckBoxRePkg.setEnabled(editable);
    }

    /* (non-Javadoc)
     * @see java.awt.event.ComponentListener#componentResized(java.awt.event.ComponentEvent)
     * 
     * Override componentResized to resize all components when frame's size is changed
     */
    public void componentResized(ComponentEvent arg0) {
        int intPreferredWidth = dialogWidth;
        int intCurrentWidth = this.getJContentPane().getWidth();

//        Tools.resizeComponentWidth(this.jPackageNameTextField, intCurrentWidth, intPreferredWidth);
//        Tools.resizeComponentWidth(this.jGuidTextField, intCurrentWidth, intPreferredWidth);
//        Tools.resizeComponentWidth(this.jVersionTextField, intCurrentWidth, intPreferredWidth);
        Tools.resizeComponentWidth(this.jUrlTextField, intCurrentWidth, intPreferredWidth);
        Tools.resizeComponentWidth(this.jLicenseScrollPane, intCurrentWidth, intPreferredWidth);
        Tools.resizeComponentWidth(this.jCopyrightTextArea, intCurrentWidth, intPreferredWidth);
        Tools.resizeComponentWidth(this.jDescriptionScrollPane, intCurrentWidth, intPreferredWidth);
        //        Tools.resizeComponentWidth(this.jSpecificationTextField, intCurrentWidth,intPreferredWidth);
        Tools.resizeComponentWidth(this.jAbstractTextField, intCurrentWidth, intPreferredWidth);
//        Tools.relocateComponentX(this.jGenerateGuidButton, intCurrentWidth, jGenerateGuidButton.getWidth(), 30);
    }

    private JScrollPane getTopScrollPane() {
        if (topScrollPane == null) {
            topScrollPane = new JScrollPane();
            topScrollPane.setViewportView(getJContentPane());
        }
        return topScrollPane;
    }

    /* (non-Javadoc)
     * @see javax.swing.event.DocumentListener#changedUpdate(javax.swing.event.DocumentEvent)
     */
    public void changedUpdate(DocumentEvent arg0) {
        // TODO Auto-generated method stub
        
    }

    /* (non-Javadoc)
     * @see javax.swing.event.DocumentListener#insertUpdate(javax.swing.event.DocumentEvent)
     */
    public void insertUpdate(DocumentEvent arg0) {
        // TODO Auto-generated method stub
        if (docConsole != null) {
            docConsole.setSaved(false);
        }
    }

    /* (non-Javadoc)
     * @see javax.swing.event.DocumentListener#removeUpdate(javax.swing.event.DocumentEvent)
     */
    public void removeUpdate(DocumentEvent arg0) {
        // TODO Auto-generated method stub
        if (docConsole != null) {
            docConsole.setSaved(false);    
        }
    }
}
