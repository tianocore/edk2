/** @file
  Java class GenListDialog.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.frameworkwizard.platform.ui;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Toolkit;

import javax.swing.JPanel;
import javax.swing.JDialog;

import javax.swing.JButton;


import java.awt.FlowLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Vector;

import javax.swing.JScrollPane;

import org.tianocore.frameworkwizard.common.ui.iCheckBoxList.ICheckBoxList;


/**
 Dialog for List generation. 
 @since PackageEditor 1.0
**/
public class GenListDialog extends JDialog implements ActionListener{

    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    private JPanel jContentPane = null;
    private JPanel jPanel = null;
    private JPanel jPanel1 = null;
    private JPanel jPanel2 = null;
    private JPanel jPanel3 = null;
    private JPanel jPanel4 = null;
    private JButton jButton = null;
    private JButton jButton1 = null;
    private JButton jButton2 = null;
    private ICheckBoxList checkBoxList = null;


    private JScrollPane jScrollPane = null;



    public void actionPerformed(ActionEvent arg0) {
        // TODO Auto-generated method stub
        if (arg0.getSource() == jButton1){
            checkBoxList.setAllItemsUnchecked();
        }
        
        if (arg0.getSource() == jButton2){

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
            jPanel4 = new JPanel();
            jPanel4.setLayout(new FlowLayout());
            jPanel4.add(getJScrollPane(), null);
            
        }
        return jPanel4;
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
            jButton1.setText("Clear");
            jButton1.setVisible(false);
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
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setPreferredSize(new java.awt.Dimension(300,100));
            jScrollPane.setViewportView(getICheckBoxList());
        }
        return jScrollPane;
    }

    private ICheckBoxList getICheckBoxList() {
        if (checkBoxList == null) {
            checkBoxList = new ICheckBoxList();
            checkBoxList.setBounds(new java.awt.Rectangle(40,20,177,74));
//            Vector<String> v = new Vector<String>();
//            v.add("DATA_HUB_RECORD");
//            v.add("EFI_EVENT");
//            v.add("EFI_SYSTEM_CONFIGURATION_TABLE");
//            v.add("EFI_VARIABLE");
//            v.add("GUID");
//            v.add("HII_PACKAGE_LIST");
//            v.add("HOB");
//            v.add("TOKEN_SPACE_GUID");
//          
//            checkBoxList.setAllItems(v);
        }
        return checkBoxList;
    }

    public void initList(Vector<String> v){
        checkBoxList.setAllItems(v);
    }
    public Vector<String> getList(){
        Vector<String> v = checkBoxList.getAllCheckedItemsString();
        return v;
    }
    
    public void setList(String s){
        Vector<String> v = new Vector<String>();
        if (s == null) {
            checkBoxList.setAllItemsUnchecked();
            return;
        }
        String[] sArray = s.split(" ");
        for (int i = 0; i < sArray.length; ++i){
            v.add(sArray[i]);
        }
        checkBoxList.initCheckedItem(true, v);
    }
    /**
     * This is the default constructor
     */
    public GenListDialog() {
        super();
        initialize();
    }
    
    public GenListDialog(ActionListener i){
        this();
        jButton2.addActionListener(i);
        
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
        this.setTitle("List");
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
    
    
    
}  //  @jve:decl-index=0:visual-constraint="10,10"
