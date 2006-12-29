/** @file
  Java class GenLongTextDialog.
 
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

import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JDialog;
import javax.swing.JTextArea;
import javax.swing.JButton;
import javax.swing.KeyStroke;

import java.awt.FlowLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyEvent;

import javax.swing.JScrollPane;



/**
 Dialog for Long Text generation. 
 @since PackageEditor 1.0
**/
public class GenLongTextDialog extends JDialog implements ActionListener{

    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    private JPanel jContentPane = null;
    private JPanel jPanelContentEast = null;
    private JPanel jPanelContentCenter = null;
    private JButton jButtonCancel = null;
    private JButton jButtonOk = null;
    private JTextArea jTextArea = null;


    private JScrollPane jScrollPane = null;

    public void actionPerformed(ActionEvent arg0) {
        
        if (arg0.getSource() == jButtonOk){

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
    private JPanel getJPanelContentEast() {
        if (jPanelContentEast == null) {
            FlowLayout flowLayout = new FlowLayout();
            flowLayout.setVgap(5);
            flowLayout.setAlignment(java.awt.FlowLayout.RIGHT);
            jPanelContentEast = new JPanel();
            jPanelContentEast.setLayout(flowLayout);
            jPanelContentEast.setPreferredSize(new java.awt.Dimension(100,30));
            jPanelContentEast.add(getJButtonOk(), null);
            jPanelContentEast.add(getJButtonCancel(), null);
        }
        return jPanelContentEast;
    }

    /**
     * This method initializes jPanel4	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelContentCenter() {
        if (jPanelContentCenter == null) {
            jPanelContentCenter = new JPanel();
            jPanelContentCenter.setLayout(new FlowLayout());
            jPanelContentCenter.add(getJScrollPane(), null);
            
        }
        return jPanelContentCenter;
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
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setPreferredSize(new java.awt.Dimension(600,40));
            jScrollPane.setViewportView(getJTextArea());
        }
        return jScrollPane;
    }

    private JTextArea getJTextArea() {
        if (jTextArea == null) {
            jTextArea = new JTextArea();
//            jTextArea.setBounds(new java.awt.Rectangle(40,20,300,54));

        }
        return jTextArea;
    }

    
    public String getText(){
       
        return jTextArea.getText();
    }
    
    public void setText(String s){
        jTextArea.setText(s);
    }
    /**
     * This is the default constructor
     */
    public GenLongTextDialog(JFrame frame) {
        super(frame);
        initialize();
    }
    
    public GenLongTextDialog(ActionListener i, JFrame frame){
        this(frame);
        jButtonOk.addActionListener(i);
        jButtonOk.registerKeyboardAction(i, KeyStroke.getKeyStroke(KeyEvent.VK_ENTER, 0, false), JComponent.WHEN_FOCUSED);
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
    private void initialize() {
        this.setSize(620, 120);
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
        this.setModal(true);
        this.setTitle("Text Content");
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
            jContentPane.add(getJPanelContentEast(), java.awt.BorderLayout.SOUTH);
            jContentPane.add(getJPanelContentCenter(), java.awt.BorderLayout.CENTER);
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
