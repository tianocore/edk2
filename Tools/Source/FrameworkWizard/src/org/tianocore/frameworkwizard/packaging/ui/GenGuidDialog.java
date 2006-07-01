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

import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JTextField;
import javax.swing.JLabel;
import javax.swing.JRadioButton;
import javax.swing.ButtonGroup;
import javax.swing.JButton;

import org.tianocore.frameworkwizard.common.Tools;

import java.awt.FlowLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
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
    private JPanel jPanel = null;
    private JPanel jPanel1 = null;
    private JPanel jPanel2 = null;
    private JPanel jPanel3 = null;
    private JPanel jPanel4 = null;
    private JTextField jTextField = null;
    private JLabel jLabel = null;
    private JRadioButton jRadioButton = null;
    private JRadioButton jRadioButton1 = null;
    private JButton jButton = null;
    private JButton jButton1 = null;
    private JButton jButton2 = null;
    
//    private String guid = null;

    public void actionPerformed(ActionEvent arg0) {
        // TODO Auto-generated method stub
        if (arg0.getSource() == jButton1){
            String uuid = Tools.generateUuidString();
            if (jRadioButton1.isSelected()) {
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
        
        if (arg0.getSource() == jRadioButton1){
            
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
        
        if (arg0.getSource() == jButton2){
//            if (jTextField.getText().matches(Tools.guidArrayPat) 
//                            || jTextField.getText().matches(Tools.guidRegistryPat)){
//                this.setVisible(false);
//            }
//            else {
//                JOptionPane.showMessageDialog(frame, "Incorrect GUID Value Format.");
//            }
            this.dispose();
        }
        
        if (arg0.getSource() == jButton){
            this.dispose();
        }
    }

    /**
     * This method initializes jPanel	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel() {
        if (jPanel == null) {
            FlowLayout flowLayout = new FlowLayout();
            flowLayout.setVgap(10);
            jPanel = new JPanel();
            jPanel.setLayout(flowLayout);
            jPanel.setPreferredSize(new java.awt.Dimension(100,30));
            jPanel.add(getJButton1(), null);
            jPanel.add(getJButton2(), null);
            jPanel.add(getJButton(), null);
        }
        return jPanel;
    }

    /**
     * This method initializes jPanel1	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel1() {
        if (jPanel1 == null) {
            jPanel1 = new JPanel();
        }
        return jPanel1;
    }

    /**
     * This method initializes jPanel2	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel2() {
        if (jPanel2 == null) {
            jPanel2 = new JPanel();
        }
        return jPanel2;
    }

    /**
     * This method initializes jPanel3	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel3() {
        if (jPanel3 == null) {
            jPanel3 = new JPanel();
        }
        return jPanel3;
    }

    /**
     * This method initializes jPanel4	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel4() {
        if (jPanel4 == null) {
            jLabel = new JLabel();
            jLabel.setText("GUID Value");
            GridLayout gridLayout = new GridLayout();
            gridLayout.setRows(4);
            jPanel4 = new JPanel();
            jPanel4.setLayout(gridLayout);
            jPanel4.add(getJRadioButton1(), null);
            jPanel4.add(getJRadioButton(), null);
            jPanel4.add(jLabel, null);
            jPanel4.add(getJTextField(), null);
            ButtonGroup bg = new ButtonGroup();
            bg.add(jRadioButton1);
            bg.add(jRadioButton);
        }
        return jPanel4;
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
    private JRadioButton getJRadioButton1() {
        if (jRadioButton1 == null) {
            jRadioButton1 = new JRadioButton();
            jRadioButton1.setText("Registry Format");
            jRadioButton1.setSelected(true);
            jRadioButton1.addActionListener(this);
        }
        return jRadioButton1;
    }

    /**
     * This method initializes jButton	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton() {
        if (jButton == null) {
            jButton = new JButton();
            jButton.setPreferredSize(new java.awt.Dimension(80,20));
            jButton.setText("Cancel");
            jButton.addActionListener(this);
        }
        return jButton;
    }

    /**
     * This method initializes jButton1	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton1() {
        if (jButton1 == null) {
            jButton1 = new JButton();
            jButton1.setPreferredSize(new java.awt.Dimension(80,20));
            jButton1.setHorizontalTextPosition(javax.swing.SwingConstants.LEADING);
            jButton1.setText("New");
            jButton1.addActionListener(this);
        }
        return jButton1;
    }

    /**
     * This method initializes jButton2	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton2() {
        if (jButton2 == null) {
            jButton2 = new JButton();
            jButton2.setPreferredSize(new java.awt.Dimension(80,20));
            jButton2.setText("Ok");
            jButton2.setActionCommand("GenGuidValue");
            jButton2.addActionListener(this);
        }
        return jButton2;
    }

    /**
     
     @param args
     **/
    public static void main(String[] args) {
        // TODO Auto-generated method stub
        new GenGuidDialog().setVisible(true);
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
    
    public GenGuidDialog(ActionListener i){
        super();
        initialize();
        jButton2.addActionListener(i);
        this.addWindowListener(new WindowAdapter(){

            @Override
            public void windowActivated(WindowEvent arg0) {
                // TODO Auto-generated method stub
                super.windowActivated(arg0);
                if ((jRadioButton1.isSelected() && jTextField.getText().matches(GenGuidDialog.guidArrayPat))
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
            jContentPane.add(getJPanel(), java.awt.BorderLayout.EAST);
            jContentPane.add(getJPanel1(), java.awt.BorderLayout.WEST);
            jContentPane.add(getJPanel2(), java.awt.BorderLayout.NORTH);
            jContentPane.add(getJPanel3(), java.awt.BorderLayout.SOUTH);
            jContentPane.add(getJPanel4(), java.awt.BorderLayout.CENTER);
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
