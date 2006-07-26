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

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;



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
public class FpdHeader extends IInternalFrame {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -8152099582923006900L;

    static JFrame frame;
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

    private StarLabel jStarLabel8 = null;

    private StarLabel jStarLabel9 = null;
    
    private JTextField jTextFieldCopyright = null;

    private JLabel jLabel = null;

    private JTextField jTextField = null;
    
    private FpdFileContents ffc = null;
    
    private OpeningPlatformType docConsole = null;

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
               public void focusLost(FocusEvent e) {
                   if (!DataValidation.isUiNameType(jTextFieldBaseName.getText())) {
                       JOptionPane.showMessageDialog(frame, "Package Name is NOT UiNameType.");
                       return;
                   }
                   if (jTextFieldBaseName.getText().equals(ffc.getFpdHdrPlatformName())) {
                       return;
                   }
                   docConsole.setSaved(false);
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
            jTextFieldGuid.setBounds(new java.awt.Rectangle(160, 35, 250, 20));
            jTextFieldGuid.setPreferredSize(new java.awt.Dimension(250,20));
            jTextFieldGuid.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e) {
                    if (!DataValidation.isGuid(jTextFieldGuid.getText())) {
                        JOptionPane.showMessageDialog(frame, "Guid is NOT GuidType.");
                        return;
                    }
                    if (jTextFieldGuid.getText().equals(ffc.getFpdHdrGuidValue())) {
                        return;
                    }
                    docConsole.setSaved(false);
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
            jTextFieldVersion.setBounds(new java.awt.Rectangle(160, 60, 320, 20));
            jTextFieldVersion.setPreferredSize(new java.awt.Dimension(320,20));
            jTextFieldVersion.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e) {
                    if (!DataValidation.isVersion(jTextFieldVersion.getText())) {
                        JOptionPane.showMessageDialog(frame, "Version is NOT version type.");
                        return;
                    }
                    if (jTextFieldVersion.getText().equals(ffc.getFpdHdrVer())) {
                        return;
                    }
                    docConsole.setSaved(false);
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
            jTextAreaLicense.setLineWrap(true);
            jTextAreaLicense.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e) {
                    if (jTextAreaLicense.getText().length() == 0) {
                        JOptionPane.showMessageDialog(frame, "License contents could NOT be empty.");
                        return;
                    }
                    if (jTextAreaLicense.getText().equals(ffc.getFpdHdrLicense())) {
                        return;
                    }
                    docConsole.setSaved(false);
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
            jTextAreaDescription.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e) {
                    if (jTextAreaDescription.getText().length() == 0) {
                        JOptionPane.showMessageDialog(frame, "Description contents could NOT be empty.");
                        return;
                    }
                    if (jTextAreaDescription.getText().equals(ffc.getFpdHdrDescription())) {
                        return;
                    }
                    docConsole.setSaved(false);
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
            jTextFieldSpecification.setBounds(new java.awt.Rectangle(160,328,320,20));
            jTextFieldSpecification.setEditable(false);
            jTextFieldSpecification.setPreferredSize(new java.awt.Dimension(320,20));
            jTextFieldSpecification.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e) {
                    ffc.setFpdHdrSpec(jTextFieldSpecification.getText());
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
            jScrollPaneLicense.setBounds(new java.awt.Rectangle(160,107,320,80));
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
            jScrollPaneDescription.setBounds(new java.awt.Rectangle(160,243,320,80));
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
            jTextFieldAbstract.setBounds(new java.awt.Rectangle(160,218,320,20));
            jTextFieldAbstract.setPreferredSize(new java.awt.Dimension(320, 20));
            jTextFieldAbstract.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e) {
                    if (!DataValidation.isAbstract(jTextFieldAbstract.getText())) {
                        JOptionPane.showMessageDialog(frame, "Abstract could NOT be empty.");
                        return;
                    }
                    if (jTextFieldAbstract.getText().equals(ffc.getFpdHdrAbs())) {
                        return;
                    }
                    docConsole.setSaved(false);
                    ffc.setFpdHdrAbs(jTextFieldAbstract.getText());
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
            jTextFieldCopyright.setBounds(new java.awt.Rectangle(160,85,320,20));
            jTextFieldCopyright.setPreferredSize(new java.awt.Dimension(320,20));
            jTextFieldCopyright.addFocusListener(new FocusAdapter(){
                public void focusLost(FocusEvent e) {
                    if (!DataValidation.isCopyright(jTextFieldCopyright.getText())) {
                        JOptionPane.showMessageDialog(frame, "Copyright contents could not be empty.");
                        return;
                    }
                    if (jTextFieldCopyright.getText().equals(ffc.getFpdHdrCopyright())) {
                        return;
                    }
                    docConsole.setSaved(false);
                    ffc.setFpdHdrCopyright(jTextFieldCopyright.getText());
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
            jTextField.setBounds(new java.awt.Rectangle(160,193,320,20));
            jTextField.setPreferredSize(new Dimension(320, 20));
            jTextField.addFocusListener(new FocusAdapter(){
               public void focusLost(FocusEvent e){
                   if (jTextField.getText().length() == 0 && ffc.getFpdHdrUrl() == null) {
                       return;
                   }
                   if (jTextField.getText().equals(ffc.getFpdHdrUrl())) {
                       return;
                   }
                   ffc.setFpdHdrLicense(jTextAreaLicense.getText());
                   ffc.setFpdHdrUrl(jTextField.getText());
                   docConsole.setSaved(false);
               } 
            });
        }
        return jTextField;
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
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        //this.setSize(500, 515);
        this.setContentPane(getJContentPane());
        this.setTitle("Package Surface Area Header");
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
            jTextField.setText(ffc.getFpdHdrUrl());
        }
        if (ffc.getFpdHdrCopyright() != null) {
            jTextFieldCopyright.setText(ffc.getFpdHdrCopyright());
        }
        if (ffc.getFpdHdrDescription() != null) {
            jTextAreaDescription.setText(ffc.getFpdHdrDescription());
        }
        if (ffc.getFpdHdrSpec() != null) {
            jTextFieldSpecification.setText(ffc.getFpdHdrSpec());
        }
        ffc.setFpdHdrSpec(jTextFieldSpecification.getText());    
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel jContentPane
     
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
        	jLabel = new JLabel();
        	jLabel.setBounds(new java.awt.Rectangle(15,193,140,20));
        	jLabel.setText("URL");
        	jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.setLocation(new java.awt.Point(0, 0));
            jContentPane.setSize(new java.awt.Dimension(500,423));
            jLabelAbstract = new JLabel();
            jLabelAbstract.setBounds(new java.awt.Rectangle(15,218,140,20));
            jLabelAbstract.setText("Abstract");
            jLabelSpecification = new JLabel();
            jLabelSpecification.setText("Specification");
            jLabelSpecification.setBounds(new java.awt.Rectangle(15,328,140,20));
            jLabelDescription = new JLabel();
            jLabelDescription.setText("Description");
            jLabelDescription.setBounds(new java.awt.Rectangle(15,243,140,20));
            jLabelCopyright = new JLabel();
            jLabelCopyright.setText("Copyright");
            jLabelCopyright.setBounds(new java.awt.Rectangle(15,85,140,20));
            jLabelLicense = new JLabel();
            jLabelLicense.setText("License");
            jLabelLicense.setBounds(new java.awt.Rectangle(15,108,140,20));
            jLabelVersion = new JLabel();
            jLabelVersion.setText("Version");
            jLabelVersion.setBounds(new java.awt.Rectangle(15, 60, 140, 20));
            jLabelGuid = new JLabel();
            jLabelGuid.setPreferredSize(new java.awt.Dimension(25, 15));
            jLabelGuid.setBounds(new java.awt.Rectangle(15, 35, 140, 20));
            jLabelGuid.setText("Guid");
            jLabelBaseName = new JLabel();
            jLabelBaseName.setText("Platform Name");
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
            jStarLabel4.setLocation(new java.awt.Point(0,108));
            jStarLabel5 = new StarLabel();
            jStarLabel5.setLocation(new java.awt.Point(0,85));
            jStarLabel6 = new StarLabel();
            jStarLabel6.setLocation(new java.awt.Point(0,218));
            jStarLabel7 = new StarLabel();
            jStarLabel7.setLocation(new java.awt.Point(0,328));
            jStarLabel7.setEnabled(false);
            jStarLabel8 = new StarLabel();
            jStarLabel8.setLocation(new java.awt.Point(0,243));
            jStarLabel9 = new StarLabel();
            jStarLabel9.setLocation(new java.awt.Point(0,303));
            jStarLabel9.setVisible(false);
            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel2, null);
            jContentPane.add(jStarLabel3, null);
            jContentPane.add(jStarLabel4, null);
            jContentPane.add(jStarLabel5, null);
            jContentPane.add(jStarLabel6, null);
            jContentPane.add(jStarLabel7, null);
            jContentPane.add(jStarLabel8, null);
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
        
        if (arg0.getSource() == jButtonGenerateGuid) {
            docConsole.setSaved(false);
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
        int intPreferredWidth = 500;
        
        resizeComponentWidth(this.jTextFieldBaseName, this.getWidth(), intPreferredWidth);
		resizeComponentWidth(this.jTextFieldGuid, this.getWidth(), intPreferredWidth);
		resizeComponentWidth(this.jTextFieldVersion, this.getWidth(), intPreferredWidth);
        resizeComponentWidth(this.jTextField, this.getWidth(), intPreferredWidth);
		resizeComponentWidth(this.jScrollPaneLicense, this.getWidth(), intPreferredWidth);
		resizeComponentWidth(this.jTextFieldCopyright, this.getWidth(), intPreferredWidth);
		resizeComponentWidth(this.jScrollPaneDescription, this.getWidth(), intPreferredWidth);
		resizeComponentWidth(this.jTextFieldSpecification, this.getWidth(), intPreferredWidth);
		resizeComponentWidth(this.jTextFieldAbstract, this.getWidth(), intPreferredWidth);
		
		relocateComponentX(this.jButtonGenerateGuid, this.getWidth(), jButtonGenerateGuid.getWidth(), 25);
	}
}
