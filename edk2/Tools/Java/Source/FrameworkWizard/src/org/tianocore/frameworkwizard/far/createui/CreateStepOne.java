/** @file

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.far.createui;

import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;

import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;

import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.common.ui.StarLabel;
import org.tianocore.frameworkwizard.far.FarHeader;
import org.tianocore.frameworkwizard.far.FarStringDefinition;

public class CreateStepOne extends IDialog implements MouseListener {

    // /
    // / Define class Serial Version UID
    // /
    private static final long serialVersionUID = -8152099582923006900L;

    //
    // Define class members
    //
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

    private JScrollPane jScrollPaneLicense = null;

    private JScrollPane jScrollPaneDescription = null;

    private JLabel jLabelAbstract = null;

    private JTextField jTextFieldAbstract = null;

    private StarLabel jStarLabel1 = null;

    private StarLabel jStarLabel4 = null;

    private StarLabel jStarLabel5 = null;

    private StarLabel jStarLabel6 = null;

    private StarLabel jStarLabel7 = null;

    private StarLabel jStarLabel8 = null;

    private StarLabel jStarLabel10 = null;

    private StarLabel jStarLabel12 = null;

    private JLabel jLabelURL = null;

    private JTextField jTextFieldURL = null;

    private JScrollPane jScrollPane = null;

    private CreateStepTwo stepTwo = null;

    private JButton jButtonCancel = null;

    private JButton jButtonNext = null;

    private FarHeader farHeader = new FarHeader();

    private JScrollPane jScrollPaneCopyright = null;

    private JTextArea jTextAreaCopyright = null;

    /**
     * This method initializes jTextFieldBaseName
     * 
     * @return javax.swing.JTextField jTextFieldBaseName
     * 
     */
    private JTextField getJTextFieldBaseName() {
        if (jTextFieldBaseName == null) {
            jTextFieldBaseName = new JTextField();
            jTextFieldBaseName.setBounds(new java.awt.Rectangle(160, 10, 520, 20));
            jTextFieldBaseName.setToolTipText("A brief Identifier, such as USB I/O Drivers, of the Framework Archive.");
        }
        return jTextFieldBaseName;
    }

    /**
     * This method initializes jTextFieldGuid
     * 
     * @return javax.swing.JTextField jTextFieldGuid
     * 
     */
    private JTextField getJTextFieldGuid() {
        if (jTextFieldGuid == null) {
            jTextFieldGuid = new JTextField();
            jTextFieldGuid.setBounds(new java.awt.Rectangle(160, 35, 410, 20));
            jTextFieldGuid.setToolTipText("Guaranteed Unique Identification Number (8-4-4-4-12)");
        }
        return jTextFieldGuid;
    }

    /**
     * This method initializes jTextFieldVersion
     * 
     * @return javax.swing.JTextField jTextFieldVersion
     * 
     */
    private JTextField getJTextFieldVersion() {
        if (jTextFieldVersion == null) {
            jTextFieldVersion = new JTextField();
            jTextFieldVersion.setBounds(new java.awt.Rectangle(160, 60, 520, 20));
            jTextFieldVersion.setToolTipText("A Version Number, 1.0, 1, 1.01");
        }
        return jTextFieldVersion;
    }

    /**
     * This method initializes jButtonGenerateGuid
     * 
     * @return javax.swing.JButton jButtonGenerateGuid
     * 
     */
    private JButton getJButtonGenerateGuid() {
        if (jButtonGenerateGuid == null) {
            jButtonGenerateGuid = new JButton();
            jButtonGenerateGuid.setBounds(new java.awt.Rectangle(590, 35, 90, 20));
            jButtonGenerateGuid.setText("Generate");
            jButtonGenerateGuid.addMouseListener(this);
        }
        return jButtonGenerateGuid;
    }

    /**
     * This method initializes jTextAreaLicense
     * 
     * @return javax.swing.JTextArea jTextAreaLicense
     * 
     */
    private JTextArea getJTextAreaLicense() {
        if (jTextAreaLicense == null) {
            jTextAreaLicense = new JTextArea();
            jTextAreaLicense.setText("");
            jTextAreaLicense.setLineWrap(true);
            jTextAreaLicense.setWrapStyleWord(true);
            jTextAreaLicense.setToolTipText("The License for this FAR file.");
        }
        return jTextAreaLicense;
    }

    /**
     * This method initializes jTextAreaDescription
     * 
     * @return javax.swing.JTextArea jTextAreaDescription
     * 
     */
    private JTextArea getJTextAreaDescription() {
        if (jTextAreaDescription == null) {
            jTextAreaDescription = new JTextArea();
            jTextAreaDescription.setLineWrap(true);
            jTextAreaDescription.setWrapStyleWord(true);
            jTextAreaDescription.setToolTipText("A verbose description of the FAR contents.");
        }
        return jTextAreaDescription;
    }

    /**
     * This method initializes jTextFieldSpecification
     * 
     * @return javax.swing.JTextField jTextFieldSpecification
     * 
     */
    private JTextField getJTextFieldSpecification() {
        if (jTextFieldSpecification == null) {
            jTextFieldSpecification = new JTextField();
            jTextFieldSpecification.setText("FRAMEWORK_BUILD_PACKAGING_SPECIFICATION   0x00000052");
            jTextFieldSpecification.setBounds(new java.awt.Rectangle(160, 310, 520, 20));
            jTextFieldSpecification.setEditable(false);
        }
        return jTextFieldSpecification;
    }

    /**
     * This method initializes jScrollPaneLicense
     * 
     * @return javax.swing.JScrollPane jScrollPaneLicense
     * 
     */
    private JScrollPane getJScrollPaneLicense() {
        if (jScrollPaneLicense == null) {
            jScrollPaneLicense = new JScrollPane();
            jScrollPaneLicense.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jScrollPaneLicense.setBounds(new java.awt.Rectangle(160, 220, 520, 60));
            jScrollPaneLicense.setViewportView(getJTextAreaLicense());
        }
        return jScrollPaneLicense;
    }

    /**
     * This method initializes jScrollPaneDescription
     * 
     * @return javax.swing.JScrollPane jScrollPaneDescription
     * 
     */
    private JScrollPane getJScrollPaneDescription() {
        if (jScrollPaneDescription == null) {
            jScrollPaneDescription = new JScrollPane();
            jScrollPaneDescription.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jScrollPaneDescription.setBounds(new java.awt.Rectangle(160, 110, 520, 60));
            jScrollPaneDescription.setViewportView(getJTextAreaDescription());
        }
        return jScrollPaneDescription;
    }

    /**
     * This method initializes jTextFieldAbstract
     * 
     * @return javax.swing.JTextField jTextFieldAbstract
     * 
     */
    private JTextField getJTextFieldAbstract() {
        if (jTextFieldAbstract == null) {
            jTextFieldAbstract = new JTextField();
            jTextFieldAbstract.setBounds(new java.awt.Rectangle(160, 85, 520, 20));
            jTextFieldAbstract.setToolTipText("A one sentence description of this FAR package.");
        }
        return jTextFieldAbstract;
    }

    /**
     * This method initializes jTextFieldURL
     * 
     * @return javax.swing.JTextField
     */
    private JTextField getJTextFieldURL() {
        if (jTextFieldURL == null) {
            jTextFieldURL = new JTextField();
            jTextFieldURL.setBounds(new java.awt.Rectangle(160, 285, 520, 20));
            jTextFieldURL.setToolTipText("A URL for the latest version of the license");
        }
        return jTextFieldURL;
    }

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
     * This method initializes jButtonCancel1
     * 
     * @return javax.swing.JButton
     */
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setBounds(new java.awt.Rectangle(590, 350, 90, 20));
            jButtonCancel.setText("Cancel");
            jButtonCancel.addMouseListener(this);
        }
        return jButtonCancel;
    }

    /**
     * This method initializes jButtonNext
     * 
     * @return javax.swing.JButton
     */
    private JButton getJButtonNext() {
        if (jButtonNext == null) {
            jButtonNext = new JButton();
            jButtonNext.setBounds(new java.awt.Rectangle(480, 350, 90, 20));
            jButtonNext.setText("Next");
            jButtonNext.addMouseListener(this);
        }
        return jButtonNext;
    }

    /**
     * This method initializes jScrollPaneCopyright	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneCopyright() {
        if (jScrollPaneCopyright == null) {
            jScrollPaneCopyright = new JScrollPane();
            jScrollPaneCopyright.setBounds(new java.awt.Rectangle(160, 175, 520, 40));
            jScrollPaneCopyright.setViewportView(getJTextAreaCopyright());
        }
        return jScrollPaneCopyright;
    }

    /**
     * This method initializes jTextAreaCopyright	
     * 	
     * @return javax.swing.JTextArea	
     */
    private JTextArea getJTextAreaCopyright() {
        if (jTextAreaCopyright == null) {
            jTextAreaCopyright = new JTextArea();
        }
        return jTextAreaCopyright;
    }

    public static void main(String[] args) {
        CreateStepOne c = new CreateStepOne(new IFrame(), true);
        c.setVisible(true);
    }

    /**
     * This is the default constructor
     * 
     */
    public CreateStepOne(IFrame iFrame, boolean modal) {
        super(iFrame, modal);
        initialize();
    }

    /**
     * Disable all components when the mode is view
     * 
     * @param isView
     *          true - The view mode; false - The non-view mode
     * 
     */
    public void setViewMode(boolean isView) {
        if (isView) {
            this.jTextFieldBaseName.setEnabled(!isView);
            this.jTextFieldGuid.setEnabled(!isView);
            this.jTextFieldVersion.setEnabled(!isView);
            this.jTextAreaLicense.setEnabled(!isView);
            this.jScrollPaneCopyright.setEnabled(!isView);
            this.jTextAreaCopyright.setEnabled(!isView);
            this.jTextAreaDescription.setEnabled(!isView);
            this.jTextFieldSpecification.setEnabled(!isView);
            this.jTextFieldAbstract.setEnabled(!isView);
            this.jButtonGenerateGuid.setEnabled(!isView);
            this.jButtonOk.setEnabled(!isView);
        }
    }

    /**
     * This method initializes this
     * 
     */
    private void initialize() {
        this.setSize(700, 425);
        this.setContentPane(getJScrollPane());
        this.setTitle(FarStringDefinition.CREATE_STEP_ONE_TITLE);
        this.centerWindow();
    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel jContentPane
     * 
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {

            jLabelURL = new JLabel();
            jLabelURL.setText("License URL");
            jLabelURL.setBounds(new java.awt.Rectangle(35, 285, 120, 20));
            jLabelBaseName = new JLabel();
            jLabelBaseName.setText("FAR Name");
            jLabelBaseName.setBounds(new java.awt.Rectangle(35, 10, 120, 20));
            jLabelGuid = new JLabel();
            jLabelGuid.setText("Guid Value");
            jLabelGuid.setBounds(new java.awt.Rectangle(35, 35, 120, 20));
            jLabelVersion = new JLabel();
            jLabelVersion.setText("Version");
            jLabelVersion.setBounds(new java.awt.Rectangle(35, 60, 120, 20));
            jLabelAbstract = new JLabel();
            jLabelAbstract.setText("Abstract");
            jLabelAbstract.setBounds(new java.awt.Rectangle(35, 85, 120, 20));
            jLabelDescription = new JLabel();
            jLabelDescription.setText("Description");
            jLabelDescription.setBounds(new java.awt.Rectangle(35, 110, 120, 20));
            jLabelCopyright = new JLabel();
            jLabelCopyright.setText("Copyright");
            jLabelCopyright.setBounds(new java.awt.Rectangle(35, 175, 120, 20));
            jLabelLicense = new JLabel();
            jLabelLicense.setText("License");
            jLabelLicense.setBounds(new java.awt.Rectangle(35, 220, 120, 20));
            jLabelSpecification = new JLabel();
            jLabelSpecification.setText("Specification");
            jLabelSpecification.setBounds(new java.awt.Rectangle(35, 310, 120, 20));

            jContentPane = new JPanel();
            jContentPane.setLayout(null);

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
            jContentPane.add(getJScrollPaneLicense(), null);
            jContentPane.add(getJScrollPaneDescription(), null);
            jContentPane.add(jLabelAbstract, null);
            jContentPane.add(getJTextFieldAbstract(), null);
            jContentPane.add(jLabelURL, null);
            jContentPane.add(getJTextFieldURL(), null);
            jStarLabel1 = new StarLabel();
            jStarLabel1.setLocation(new java.awt.Point(20, 10));
            jStarLabel4 = new StarLabel();
            jStarLabel4.setLocation(new java.awt.Point(20, 35));
            jStarLabel5 = new StarLabel();
            jStarLabel5.setLocation(new java.awt.Point(20, 60));
            jStarLabel6 = new StarLabel();
            jStarLabel6.setLocation(new java.awt.Point(20, 110));
            jStarLabel7 = new StarLabel();
            jStarLabel7.setLocation(new java.awt.Point(20, 175));
            jStarLabel8 = new StarLabel();
            jStarLabel8.setLocation(new java.awt.Point(20, 220));
            jStarLabel10 = new StarLabel();
            jStarLabel10.setLocation(new java.awt.Point(20, 85));
            jStarLabel12 = new StarLabel();
            jStarLabel12.setLocation(new java.awt.Point(20, 310));

            jContentPane.add(jStarLabel1, null);
            jContentPane.add(jStarLabel4, null);
            jContentPane.add(jStarLabel5, null);
            jContentPane.add(jStarLabel6, null);
            jContentPane.add(jStarLabel7, null);
            jContentPane.add(jStarLabel8, null);
            jContentPane.add(jStarLabel10, null);
            jContentPane.add(jStarLabel12, null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonNext(), null);
            jContentPane.add(getJScrollPaneCopyright(), null);
        }
        return jContentPane;
    }

    public boolean valid() {
        //
        // Check BaseName
        //
        if (isEmpty(this.jTextFieldBaseName.getText())) {
            Log.wrn("Create far", "FAR Name must be entered.");
            return false;
        }
        if (!DataValidation.isBaseName(this.jTextFieldBaseName.getText())) {
            Log.wrn("Create far", "Incorrect data type for FAR Name");
            return false;
        }
        farHeader.setFarName(this.jTextFieldBaseName.getText());

        //
        // Check Guid
        //
        if (isEmpty(this.jTextFieldGuid.getText())) {
            Log.wrn("Create far", "A GUID must be entered.");
            return false;
        }
        if (!DataValidation.isGuid((this.jTextFieldGuid).getText())) {
            Log.wrn("Create far", "Incorrect data type for Guid");
            return false;
        }
        farHeader.setGuidValue(this.jTextFieldGuid.getText());

        //
        // Check Version
        //
        if (isEmpty(this.jTextFieldVersion.getText())) {
            Log.wrn("Create far", "A Version must be entered.");
            return false;
        }
        if (!DataValidation.isVersion(this.jTextFieldVersion.getText())) {
            Log.wrn("Create far", "Incorrect data type for Version");
            return false;
        }
        farHeader.setVersion(this.jTextFieldVersion.getText());

        //
        // Check Abstact
        //
        if (isEmpty(this.jTextFieldAbstract.getText())) {
            Log.wrn("Create far", "An Abstract must be entered.");
            return false;
        }
        if (!DataValidation.isAbstract(this.jTextFieldAbstract.getText())) {
            Log.wrn("Create far", "Incorrect data type for Abstract");
            return false;
        }
        farHeader.setAbstractStr(this.jTextFieldAbstract.getText());

        //
        // Check Description
        //
        if (isEmpty(this.jTextAreaDescription.getText())) {
            Log.wrn("Create far", "A Description must be entered.");
            return false;
        }
        farHeader.setDescription(this.jTextAreaDescription.getText());

        //
        // Check Copyright
        //
        if (isEmpty(this.jTextAreaCopyright.getText())) {
            Log.wrn("Create far", "The Copyright must be entered.");
            return false;
        }
        farHeader.setCopyright(this.jTextAreaCopyright.getText());

        //
        // Check License
        //
        if (isEmpty(this.jTextAreaLicense.getText())) {
            Log.wrn("Create far", "The License must be entered.");
            return false;
        }
        farHeader.setLicense(this.jTextAreaLicense.getText());

        farHeader.setSpecification(this.jTextFieldSpecification.getText());
        return true;
    }

    /**
     * Check the input data is empty or not
     * 
     * @param strValue
     *          The input data which need be checked
     * 
     * @retval true - The input data is empty
     * @retval fals - The input data is not empty
     * 
     */
    public boolean isEmpty(String strValue) {
        if (strValue.length() > 0) {
            return false;
        }
        return true;
    }

    public void mouseClicked(MouseEvent e) {
        if (e.getSource() == jButtonCancel) {
            this.setVisible(false);
        } else if (e.getSource() == jButtonNext) {
            //
            // Add some logic process here
            //
            if (!valid()) {
                return ;
            }
            if (stepTwo == null) {
                stepTwo = new CreateStepTwo(this, true, this);
            }
            this.setVisible(false);
            stepTwo.setVisible(true);
        } else if (e.getSource() == jButtonGenerateGuid) {
            this.jTextFieldGuid.setText(Tools.generateUuidString());
        }
    }

    public void mousePressed(MouseEvent e) {
        // TODO Auto-generated method stub

    }

    public void mouseReleased(MouseEvent e) {
        // TODO Auto-generated method stub

    }

    public void mouseEntered(MouseEvent e) {
        // TODO Auto-generated method stub

    }

    public void mouseExited(MouseEvent e) {
        // TODO Auto-generated method stub

    }

    public FarHeader getFarHeader() {
        return farHeader;
    }
}
