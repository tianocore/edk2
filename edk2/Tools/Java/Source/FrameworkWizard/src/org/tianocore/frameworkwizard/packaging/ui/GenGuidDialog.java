/** @file
  Java class GenGuidDialog.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.frameworkwizard.packaging.ui;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Toolkit;

import javax.swing.JPanel;
import javax.swing.JDialog;
import java.awt.GridLayout;

import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JTextField;
import javax.swing.JLabel;
import javax.swing.JRadioButton;
import javax.swing.ButtonGroup;
import javax.swing.JButton;
import javax.swing.KeyStroke;

import org.tianocore.frameworkwizard.common.Tools;

import java.awt.FlowLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

/**
 Dialog for GUID generation. 
 @since PackageEditor 1.0
**/
public class GenGuidDialog extends JDialog implements ActionListener{

    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    public static final String guidArrayPat = "0x[a-fA-F0-9]{1,8},( )*0x[a-fA-F0-9]{1,4},( )*0x[a-fA-F0-9]{1,4}(,( )*\\{)?(,?( )*0x[a-fA-F0-9]{1,2}){8}( )*(\\})?";
    
    public static final String guidRegistryPat = "[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}";
    
    static private JFrame frame;
    private JPanel jContentPane = null;
    private JPanel jPanelEast = null;
    private JPanel jPanelCenter = null;
    private JTextField jTextField = null;
    private JLabel jLabel = null;
    private JRadioButton jRadioButton = null;
    private JRadioButton jRadioButtonReg = null;
    private JButton jButtonCancel = null;
    private JButton jButtonNew = null;
    private JButton jButtonOk = null;
    private ActionListener outerListener = null;
    
//    private String guid = null;

    public void actionPerformed(ActionEvent arg0) {
        // TODO Auto-generated method stub
        if (arg0.getSource() == jButtonNew){
            String uuid = Tools.generateUuidString();
            if (jRadioButtonReg.isSelected()) {
                jTextField.setText(uuid);
            }
            else {
                //ToDo: transform to comma-sep guid
                String s = GenGuidDialog.formatGuidString(uuid);
                if (s.equals("0")) {
                    JOptionPane.showMessageDialog(frame, "Check GUID Value, it don't conform to the schema.");
                    return;
                }
                jTextField.setText(s);
            }
        }
        
        if (arg0.getSource() == jRadioButtonReg){
            
            //ToDo: check text field value against RegExp and transform if needed
            if (jTextField.getText().matches(GenGuidDialog.guidRegistryPat)){
                return;
            }
            if (jTextField.getText().matches(GenGuidDialog.guidArrayPat)) {
                jTextField.setText(GenGuidDialog.formatGuidString(jTextField.getText()));
                return;
            }
            if (jTextField.getText().length()>0)
            JOptionPane.showMessageDialog(frame, "Check GUID Value, it don't conform to the schema.");
                    
        }
        
        if (arg0.getSource() == jRadioButton){
            
            //ToDo: check text field value against RegExp and transform if needed
            if (jTextField.getText().matches(GenGuidDialog.guidArrayPat)){
                return;
            }
            if (jTextField.getText().matches(GenGuidDialog.guidRegistryPat)) {
                jTextField.setText(GenGuidDialog.formatGuidString(jTextField.getText()));
                return;
            }
            if (jTextField.getText().length()>0)
            JOptionPane.showMessageDialog(frame, "Check GUID Value, it don't conform to the schema.");
            
        }
        
        if (arg0.getSource() == jButtonOk){
//            if (jTextField.getText().matches(Tools.guidArrayPat) 
//                            || jTextField.getText().matches(Tools.guidRegistryPat)){
//                this.setVisible(false);
//            }
//            else {
//                JOptionPane.showMessageDialog(frame, "Incorrect GUID Value Format.");
//            }
//            this.dispose();
        }
        
        if (arg0.getSource() == jButtonCancel){
            this.dispose();
        }
    }

    /**
     * This method initializes jPanel	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelEast() {
        if (jPanelEast == null) {
            FlowLayout flowLayout = new FlowLayout();
            flowLayout.setVgap(10);
            jPanelEast = new JPanel();
            jPanelEast.setLayout(flowLayout);
            jPanelEast.setPreferredSize(new java.awt.Dimension(100,30));
            jPanelEast.add(getJButtonNew(), null);
            jPanelEast.add(getJButtonOk(), null);
            jPanelEast.add(getJButtonCancel(), null);
        }
        return jPanelEast;
    }

    /**
     * This method initializes jPanel4	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelCenter() {
        if (jPanelCenter == null) {
            jLabel = new JLabel();
            jLabel.setText("GUID Value");
            GridLayout gridLayout = new GridLayout();
            gridLayout.setRows(4);
            jPanelCenter = new JPanel();
            jPanelCenter.setLayout(gridLayout);
            jPanelCenter.add(getJRadioButtonReg(), null);
            jPanelCenter.add(getJRadioButton(), null);
            jPanelCenter.add(jLabel, null);
            jPanelCenter.add(getJTextField(), null);
            ButtonGroup bg = new ButtonGroup();
            bg.add(jRadioButtonReg);
            bg.add(jRadioButton);
        }
        return jPanelCenter;
    }

    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setHorizontalAlignment(JTextField.LEADING);
            jTextField.setPreferredSize(new java.awt.Dimension(100,20));
        }
        return jTextField;
    }

    /**
     * This method initializes jRadioButton	
     * 	
     * @return javax.swing.JRadioButton	
     */
    private JRadioButton getJRadioButton() {
        if (jRadioButton == null) {
            jRadioButton = new JRadioButton();
            jRadioButton.setText("Comma-Seperated Format");
            jRadioButton.setEnabled(false);
            jRadioButton.addActionListener(this);
        }
        return jRadioButton;
    }

    /**
     * This method initializes jRadioButton1	
     * 	
     * @return javax.swing.JRadioButton	
     */
    private JRadioButton getJRadioButtonReg() {
        if (jRadioButtonReg == null) {
            jRadioButtonReg = new JRadioButton();
            jRadioButtonReg.setText("Registry Format");
            jRadioButtonReg.setSelected(true);
            jRadioButtonReg.addActionListener(this);
        }
        return jRadioButtonReg;
    }

    /**
     * This method initializes jButton	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonCancel.setText("Cancel");
            jButtonCancel.addActionListener(this);
            jButtonCancel.registerKeyboardAction(this, KeyStroke.getKeyStroke(KeyEvent.VK_ENTER, 0, false), JComponent.WHEN_FOCUSED);
        }
        return jButtonCancel;
    }

    /**
     * This method initializes jButton1	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonNew() {
        if (jButtonNew == null) {
            jButtonNew = new JButton();
            jButtonNew.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonNew.setHorizontalTextPosition(javax.swing.SwingConstants.LEADING);
            jButtonNew.setText("New");
            jButtonNew.addActionListener(this);
            jButtonNew.registerKeyboardAction(this, KeyStroke.getKeyStroke(KeyEvent.VK_ENTER, 0, false), JComponent.WHEN_FOCUSED);
        }
        return jButtonNew;
    }

    /**
     * This method initializes jButton2	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setPreferredSize(new java.awt.Dimension(80,20));
            jButtonOk.setText("Ok");
            jButtonOk.setActionCommand("GenGuidValue");
//            jButtonOk.addActionListener(this);
            jButtonOk.registerKeyboardAction(outerListener, KeyStroke.getKeyStroke(KeyEvent.VK_ENTER, 0, false), JComponent.WHEN_FOCUSED);
            
        }
        return jButtonOk;
    }

    public String getGuid(){
        return jTextField.getText();
    }
    
    public void setGuid(String s){
        jTextField.setText(s);
    }
    /**
     * This is the default constructor
     */
    public GenGuidDialog() {
        super();
        initialize();
    }
    
    public GenGuidDialog(ActionListener i, JFrame frame){
        super(frame);
        outerListener = i;
        initialize();
        jButtonOk.addActionListener(i);
        this.addWindowListener(new WindowAdapter(){

            @Override
            public void windowActivated(WindowEvent arg0) {
                // TODO Auto-generated method stub
                super.windowActivated(arg0);
                if ((jRadioButtonReg.isSelected() && jTextField.getText().matches(GenGuidDialog.guidArrayPat))
                                || (jRadioButton.isSelected() && jTextField.getText().matches(GenGuidDialog.guidRegistryPat))) {
                    jTextField.setText(GenGuidDialog.formatGuidString(jTextField.getText()));
                }
                
//                if (!jTextField.getText().matches(Tools.guidArrayPat) || !jTextField.getText().matches(Tools.guidRegistryPat)) {
//                  JOptionPane.showMessageDialog(frame, "InitVal: Incorrect GUID Value Format.");
//                  return;
//                }
            }
            
        });
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
    private void initialize() {
        this.setSize(466, 157);
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
        this.setModal(true);
        this.setTitle("Editing GUID Value");
        this.setContentPane(getJContentPane());
        this.centerWindow();
    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jContentPane = new JPanel();
            jContentPane.setLayout(new BorderLayout());
            jContentPane.add(getJPanelEast(), java.awt.BorderLayout.EAST);
            jContentPane.add(getJPanelCenter(), java.awt.BorderLayout.CENTER);
        }
        return jContentPane;
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
    
    public static String formatGuidString (String guidNameConv) {
        String[] strList;
        String guid = "";
        int index = 0;
        if (guidNameConv
                        .matches(GenGuidDialog.guidRegistryPat)) {
            strList = guidNameConv.split("-");
            guid = "0x" + strList[0] + ", ";
            guid = guid + "0x" + strList[1] + ", ";
            guid = guid + "0x" + strList[2] + ", ";

            guid = guid + "0x" + strList[3].substring(0, 2) + ", ";
            guid = guid + "0x" + strList[3].substring(2, 4);

            while (index < strList[4].length()) {
                guid = guid + ", ";
                guid = guid + "0x" + strList[4].substring(index, index + 2);
                index = index + 2;
            }

            return guid;
        }
        else if (guidNameConv
                        .matches(GenGuidDialog.guidArrayPat)) {
            strList = guidNameConv.split(",");
            
            //
            // chang ANSI c form to registry form
            //
            for (int i = 0; i < strList.length; i++){
                strList[i] = strList[i].substring(strList[i].lastIndexOf("x") + 1);
            }
            if (strList[strList.length - 1].endsWith("}")) {
                strList[strList.length -1] = strList[strList.length-1].substring(0, strList[strList.length-1].length()-1); 
            }
            //
            //inserting necessary leading zeros
            //
            
            int segLen = strList[0].length();
            if (segLen < 8){
                for (int i = 0; i < 8 - segLen; ++i){
                    strList[0] = "0" + strList[0];
                }
            }
            
            segLen = strList[1].length();
            if (segLen < 4){
                for (int i = 0; i < 4 - segLen; ++i){
                    strList[1] = "0" + strList[1];
                }
            }
            segLen = strList[2].length();
            if (segLen < 4){
                for (int i = 0; i < 4 - segLen; ++i){
                    strList[2] = "0" + strList[2];
                }
            }
            for (int i = 3; i < 11; ++i) {
                segLen = strList[i].length();
                if (segLen < 2){
                    strList[i] = "0" + strList[i];
                }
            }
            
            for (int i = 0; i < 3; i++){
                guid += strList[i] + "-";
            }
            
            guid += strList[3];
            guid += strList[4] + "-";
            
            for (int i = 5; i < strList.length; ++i){
                guid += strList[i];
            }
            
            
            return guid;
        } else {
            
            return "0";

        }
    }
    
}  //  @jve:decl-index=0:visual-constraint="10,10"
