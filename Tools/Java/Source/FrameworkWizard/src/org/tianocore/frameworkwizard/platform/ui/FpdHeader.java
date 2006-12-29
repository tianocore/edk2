/** @file
 
 The file is used to create, update FpdHeader of Fpd file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.platform.ui;

import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ComponentEvent;
import java.awt.event.FocusAdapter;
import java.awt.event.FocusEvent;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;



import org.tianocore.PlatformSurfaceAreaDocument;

import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPlatformType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;

/**
 The class is used to create, update FpdHeader of Fpd file
 It extends IInternalFrame
 
 @since PackageEditor 1.0

 **/
public class FpdHeader extends IInternalFrame implements DocumentListener{

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

    private final int rowNine = rowEight + fourRowHeight +  threeRowHeight +rowSep;

    private int dialogHeight = rowNine + threeRowHeight;
    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -8152099582923006900L;

    //
    //Define class members
    //
    private JPanel jContentPane = null;  //  @jve:decl-index=0:visual-constraint="10,53"

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

    private JTextField jTextFieldSpecification = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JScrollPane jScrollPaneLicense = null;

    private JScrollPane jScrollPaneDescription = null;
    
    private JScrollPane jCopyrightScrollPane = null;

    private JLabel jLabelAbstract = null;

    private JTextField jTextFieldAbstract = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel2 = null;

    private StarLabel jStarLabel3 = null;

    private StarLabel jStarLabel4 = null;

    private StarLabel jStarLabel5 = null;

    private StarLabel jStarLabel7 = null;

    private StarLabel jStarLabel8 = null;
    
    private JTextArea jCopyrightTextArea = null;

    private JLabel jLabel = null;

    private JTextField jTextFieldUrl = null;
    
    private FpdFileContents ffc = null;
    
    private OpeningPlatformType docConsole = null;
    
    private boolean amended = false; 

    /**
     This method initializes jTextFieldBaseName 
     
     @return javax.swing.JTextField jTextFieldBaseName
     
     **/
    private JTextField getJTextFieldBaseName() {
        if (jTextFieldBaseName == null) {
            jTextFieldBaseName = new JTextField();
            jTextFieldBaseName.setBounds(new java.awt.Rectangle(valueColumn, rowOne, valueWidth, oneRowHeight));
            jTextFieldBaseName.setPreferredSize(new java.awt.Dimension(valueWidth,oneRowHeight));
            jTextFieldBaseName.getDocument().addDocumentListener(this);
            jTextFieldBaseName.addFocusListener(new FocusAdapter(){
               public void focusLost(FocusEvent e) {
                   if (!DataValidation.isUiNameType(jTextFieldBaseName.getText())) {
                       JOptionPane.showMessageDialog(FpdHeader.this, "Package Name does not match the UiNameType datatype.");
                       return;
                   }
                   if (jTextFieldBaseName.getText().equals(ffc.getFpdHdrPlatformName())) {
                       return;
                   }
                   ffc.setFpdHdrPlatformName(jTextFieldBaseName.getText());
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
            jTextFieldGuid.setBounds(new java.awt.Rectangle(valueColumn, rowTwo, shortValueWidth, oneRowHeight));
            jTextFieldGuid.setPreferredSize(new java.awt.Dimension(shortValueWidth,oneRowHeight));
            jTextFieldGuid.getDocument().addDocumentListener(this);
            jTextFieldGuid.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e) {
                    if (!DataValidation.isGuid(jTextFieldGuid.getText())) {
                        JOptionPane.showMessageDialog(FpdHeader.this, "Guid must be in registry (8-4-4-4-12) format.");
                        return;
                    }
                    if (jTextFieldGuid.getText().equals(ffc.getFpdHdrGuidValue())) {
                        return;
                    }
                    ffc.setFpdHdrGuidValue(jTextFieldGuid.getText());
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
            jTextFieldVersion.setBounds(new java.awt.Rectangle(valueColumn, rowThree, valueWidth, oneRowHeight));
            jTextFieldVersion.setPreferredSize(new java.awt.Dimension(valueWidth,oneRowHeight));
            jTextFieldVersion.getDocument().addDocumentListener(this);
            jTextFieldVersion.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e) {
                    if (!DataValidation.isVersion(jTextFieldVersion.getText())) {
                        JOptionPane.showMessageDialog(FpdHeader.this, "Version does not match the Version datatype.");
                        return;
                    }
                    if (jTextFieldVersion.getText().equals(ffc.getFpdHdrVer())) {
                        return;
                    }
                    ffc.setFpdHdrVer(jTextFieldVersion.getText());
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
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(valueColumn + shortValueWidth + 5, rowTwo, buttonWidth, oneRowHeight));
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
            jTextAreaLicense.setLineWrap(true);
            jTextAreaLicense.getDocument().addDocumentListener(this);
            jTextAreaLicense.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e) {
                    if (jTextAreaLicense.getText().length() == 0) {
                        JOptionPane.showMessageDialog(FpdHeader.this, "License must be entered!");
                        return;
                    }
                    if (jTextAreaLicense.getText().equals(ffc.getFpdHdrLicense())) {
                        return;
                    }
                    ffc.setFpdHdrLicense(jTextAreaLicense.getText());
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
            jTextAreaDescription.getDocument().addDocumentListener(this);
            jTextAreaDescription.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e) {
                    if (jTextAreaDescription.getText().length() == 0) {
                        JOptionPane.showMessageDialog(FpdHeader.this, "Description must be entered.");
                        return;
                    }
                    if (jTextAreaDescription.getText().equals(ffc.getFpdHdrDescription())) {
                        return;
                    }
                    ffc.setFpdHdrDescription(jTextAreaDescription.getText());
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
            jTextFieldSpecification.setBounds(new java.awt.Rectangle(labelColumn,rowNine,specWidth,oneRowHeight));
            jTextFieldSpecification.setEditable(false);
            jTextFieldSpecification.setPreferredSize(new java.awt.Dimension(specWidth,oneRowHeight));
            jTextFieldSpecification.setBorder(null);
//            jTextFieldSpecification.addFocusListener(new FocusAdapter(){
//                public void focusLost(FocusEvent e) {
//                    ffc.setFpdHdrSpec(jTextFieldSpecification.getText());
//                } 
//             });
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
            jButtonOk.setBounds(new java.awt.Rectangle(290,351,90,20));
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
            jButtonCancel.setBounds(new java.awt.Rectangle(390,351,90,20));
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
            jScrollPaneLicense.setBounds(new java.awt.Rectangle(valueColumn,rowFive,valueWidth,fourRowHeight));
            jScrollPaneLicense.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jScrollPaneLicense.setPreferredSize(new java.awt.Dimension(valueWidth,fourRowHeight));
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
            jScrollPaneDescription.setBounds(new java.awt.Rectangle(valueColumn,rowEight,valueWidth,fourRowHeight));
            jScrollPaneDescription.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jScrollPaneDescription.setPreferredSize(new java.awt.Dimension(valueWidth, fourRowHeight));
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
            jTextFieldAbstract.setBounds(new java.awt.Rectangle(valueColumn,rowSeven,valueWidth,oneRowHeight));
            jTextFieldAbstract.setPreferredSize(new java.awt.Dimension(valueWidth, oneRowHeight));
            jTextFieldAbstract.getDocument().addDocumentListener(this);
            jTextFieldAbstract.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e) {
                    if (!DataValidation.isAbstract(jTextFieldAbstract.getText())) {
                        JOptionPane.showMessageDialog(FpdHeader.this, "Abstract must be entered.");
                        return;
                    }
                    if (jTextFieldAbstract.getText().equals(ffc.getFpdHdrAbs())) {
                        return;
                    }
                    ffc.setFpdHdrAbs(jTextFieldAbstract.getText());
                } 
             });
        }
        return jTextFieldAbstract;
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
            jCopyrightTextArea.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e) {
                    if (!DataValidation.isCopyright(jCopyrightTextArea.getText())) {
                        JOptionPane.showMessageDialog(FpdHeader.this, "Copyright must be entered.");
                        return;
                    }
                    if (jCopyrightTextArea.getText().equals(ffc.getFpdHdrCopyright())) {
                        return;
                    }
                    ffc.setFpdHdrCopyright(jCopyrightTextArea.getText());
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
    private JTextField getJTextFieldUrl() {
        if (jTextFieldUrl == null) {
            jTextFieldUrl = new JTextField();
            jTextFieldUrl.setBounds(new java.awt.Rectangle(valueColumn,rowSix,valueWidth,oneRowHeight));
            jTextFieldUrl.setPreferredSize(new Dimension(valueWidth, oneRowHeight));
            jTextFieldUrl.getDocument().addDocumentListener(this);
            jTextFieldUrl.addFocusListener(new FocusAdapter(){
               public void focusLost(FocusEvent e){
                   if (jTextFieldUrl.getText().length() == 0 && ffc.getFpdHdrUrl() == null) {
                       return;
                   }
                   if (jTextFieldUrl.getText().equals(ffc.getFpdHdrUrl())) {
                       return;
                   }
                   ffc.setFpdHdrLicense(jTextAreaLicense.getText());
                   ffc.setFpdHdrUrl(jTextFieldUrl.getText());
               } 
            });
        }
        return jTextFieldUrl;
    }

    public static void main(String[] args) {
        new FpdHeader().setVisible(true);
    }

    /**
     This is the default constructor
     
     **/
    public FpdHeader() {
        super();
        init();
        this.setVisible(true);
    }

    /**
     This is the override edit constructor
     
     @param inFpdHeader The input data of FpdHeaderDocument.FpdHeader
     
     **/
    public FpdHeader(PlatformSurfaceAreaDocument.PlatformSurfaceArea inFpd) {
        this();
        ffc = new FpdFileContents(inFpd);
        init(ffc);
        
    }
    
    public FpdHeader(OpeningPlatformType opt) {
        this(opt.getXmlFpd());
        docConsole = opt;
        if (amended) {
            docConsole.setSaved(false);
            amended = false;
        }
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        //this.setSize(500, 515);
        this.setContentPane(getJContentPane());
        this.setTitle("Platform Surface Area Header");
        initFrame();
      
    }

    /**
     This method initializes this
     Fill values to all fields if these values are not empty
     
     @param inFpdHeader  The input data of FpdHeaderDocument.FpdHeader
     
     **/
    private void init(FpdFileContents ffc) {

        if (ffc.getFpdHdrPlatformName() != null) {
            jTextFieldBaseName.setText(ffc.getFpdHdrPlatformName());
        }
        if (ffc.getFpdHdrGuidValue() != null) {
            jTextFieldGuid.setText(ffc.getFpdHdrGuidValue());
        }
        if (ffc.getFpdHdrVer() != null) {
            jTextFieldVersion.setText(ffc.getFpdHdrVer());
        }
        if (ffc.getFpdHdrLicense() != null) {
            jTextAreaLicense.setText(ffc.getFpdHdrLicense());
        }
        if (ffc.getFpdHdrAbs() != null) {
            jTextFieldAbstract.setText(ffc.getFpdHdrAbs());
        }
        if (ffc.getFpdHdrUrl() != null) {
            jTextFieldUrl.setText(ffc.getFpdHdrUrl());
        }
        if (ffc.getFpdHdrCopyright() != null) {
            jCopyrightTextArea.setText(ffc.getFpdHdrCopyright());
        }
        if (ffc.getFpdHdrDescription() != null) {
            jTextAreaDescription.setText(ffc.getFpdHdrDescription());
        }
        if (ffc.getFpdHdrSpec() != null) {
            jTextFieldSpecification.setText(ffc.getFpdHdrSpec());
        }
        ffc.setFpdHdrSpec(jTextFieldSpecification.getText());
        
        if (ffc.getPlatformDefsSkuInfoCount() == 0) {
            ffc.genPlatformDefsSkuInfo("0", "DEFAULT");
            amended = true;
            JOptionPane.showMessageDialog(this, "Default SKU set for this platform.");
        }
        Vector<Object> v = new Vector<Object>();
        ffc.getPlatformDefsSupportedArchs(v);
        if (v.size() == 0) {
            v.add("IA32");
            ffc.setPlatformDefsSupportedArchs(v);
            amended = true;
            JOptionPane.showMessageDialog(this, "Supported Arch. IA32 added for this platform.");
        }
        v.removeAllElements();
        ffc.getPlatformDefsBuildTargets(v);
        if (v.size() == 0) {
            v.add("DEBUG");
            ffc.setPlatformDefsBuildTargets(v);
            amended = true;
            JOptionPane.showMessageDialog(this, "Build target IA32 added for this platform.");
        }
        if (ffc.getPlatformDefsInterDir() == null) {
            ffc.setPlatformDefsInterDir("UNIFIED");
            amended = true;
            JOptionPane.showMessageDialog(this, "UNIFIED Intermediate Directory set for this platform.");
        }
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setLocation(new java.awt.Point(0, 0));
            jContentPane.setSize(new java.awt.Dimension(dialogWidth - 20 ,dialogHeight - 20));

            jLabel = new JLabel();
        	jLabel.setBounds(new java.awt.Rectangle(labelColumn,rowSix,labelWidth,oneRowHeight));
        	jLabel.setText("URL");
            jLabelAbstract = new JLabel();
            jLabelAbstract.setBounds(new java.awt.Rectangle(labelColumn,rowSeven,labelWidth,oneRowHeight));
            jLabelAbstract.setText("Abstract");
            jLabelDescription = new JLabel();
            jLabelDescription.setText("Description");
            jLabelDescription.setBounds(new java.awt.Rectangle(labelColumn,rowEight,labelWidth,oneRowHeight));
            jLabelCopyright = new JLabel();
            jLabelCopyright.setText("Copyright");
            jLabelCopyright.setBounds(new java.awt.Rectangle(labelColumn,rowFour,labelWidth,oneRowHeight));
            jLabelLicense = new JLabel();
            jLabelLicense.setText("License");
            jLabelLicense.setBounds(new java.awt.Rectangle(labelColumn,rowFive,labelWidth,oneRowHeight));
            jLabelVersion = new JLabel();
            jLabelVersion.setText("Version");
            jLabelVersion.setBounds(new java.awt.Rectangle(labelColumn, rowThree, labelWidth, oneRowHeight));
            jLabelGuid = new JLabel();
            jLabelGuid.setPreferredSize(new java.awt.Dimension(labelWidth, oneRowHeight));
            jLabelGuid.setBounds(new java.awt.Rectangle(labelColumn, rowTwo, labelWidth, oneRowHeight));
            jLabelGuid.setText("Guid");
            jLabelBaseName = new JLabel();
            jLabelBaseName.setText("Platform Name");
            jLabelBaseName.setBounds(new java.awt.Rectangle(labelColumn, rowOne, labelWidth, oneRowHeight));
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(0, rowOne));
            jStarLabel2 = new StarLabel();
            jStarLabel2.setLocation(new java.awt.Point(0, rowTwo));
            jStarLabel3 = new StarLabel();
            jStarLabel3.setLocation(new java.awt.Point(0, rowThree));
            jStarLabel4 = new StarLabel();
            jStarLabel4.setLocation(new java.awt.Point(0,rowFour));
            jStarLabel5 = new StarLabel();
            jStarLabel5.setLocation(new java.awt.Point(0,rowFive));
            jStarLabel7 = new StarLabel();
            jStarLabel7.setLocation(new java.awt.Point(0,rowSeven));
            jStarLabel8 = new StarLabel();
            jStarLabel8.setLocation(new java.awt.Point(0,rowEight));
            
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
            jContentPane.add(getJTextFieldSpecification(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJScrollPaneLicense(), null);
            jContentPane.add(getJScrollPaneDescription(), null);
            jContentPane.add(jLabelAbstract, null);
            jContentPane.add(getJTextFieldAbstract(), null);
 
            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jStarLabel3, null);
            jContentPane.add(jStarLabel4, null);
            jContentPane.add(jStarLabel5, null);
            jContentPane.add(jStarLabel7, null);
            jContentPane.add(jStarLabel8, null);
            jContentPane.add(getCopyrightScrollPane(), null);

            jContentPane.add(jLabel, null);
            jContentPane.add(getJTextFieldUrl(), null);
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
        
        if (arg0.getSource() == jButtonGenerateGuid) {
            jTextFieldGuid.setText(Tools.generateUuidString());
            ffc.setFpdHdrGuidValue(jTextFieldGuid.getText());
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
        int intPreferredWidth = dialogWidth;
        int intCurrentWidth = this.getJContentPane().getWidth();
        
        // Tools.resizeComponentWidth(this.jTextFieldBaseName, this.getWidth(), intPreferredWidth);
        // Tools.resizeComponentWidth(this.jTextFieldGuid, this.getWidth(), intPreferredWidth);
//      Tools.relocateComponentX(this.jButtonGenerateGuid, this.getWidth(), jButtonGenerateGuid.getWidth(), 25);
        // Tools.resizeComponentWidth(this.jTextFieldVersion, this.getWidth(), intPreferredWidth);
        // Tools.resizeComponentWidth(this.jTextFieldCopyright, this.getWidth(), intPreferredWidth);
        Tools.resizeComponentWidth(this.jScrollPaneLicense, intCurrentWidth, intPreferredWidth);
        Tools.resizeComponentWidth(this.jTextFieldUrl, intCurrentWidth, intPreferredWidth);
        Tools.resizeComponentWidth(this.jTextFieldAbstract, intCurrentWidth, intPreferredWidth);        
        Tools.resizeComponentWidth(this.jScrollPaneDescription, intCurrentWidth, intPreferredWidth);
        // Tools.resizeComponentWidth(this.jTextFieldSpecification, this.getWidth(), intPreferredWidth);
        
		
        
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
