/** @file
 
 The file is used to override JComboBox to provides customized interfaces 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.common.ui;

import java.awt.event.FocusEvent;
import java.awt.event.FocusListener;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;

import javax.swing.JComboBox;
import javax.swing.JFrame;
import javax.swing.JPanel;

/**
 The class is used to override JComboBox to provides customized interfaces
 It extends JComboBox implements KeyListener, MouseListener and FocusListener
 

 
 **/
public class IComboBox extends JComboBox implements KeyListener, MouseListener, FocusListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -1940262568168458911L;

    public void focusGained(FocusEvent arg0) {
        // TODO Auto-generated method stub

    }

    /* (non-Javadoc)
     * @see java.awt.event.FocusListener#focusLost(java.awt.event.FocusEvent)
     * 
     * Override focusLost to exit edit mode
     * 
     */
    public void focusLost(FocusEvent arg0) {
        this.closeEdit();
    }

    /**
     Main class, used for test
     
     @param args
     
     **/
    public static void main(String[] args) {
        JFrame jf = new JFrame();
        jf.setSize(500, 200);
        JPanel jp = new JPanel();
        jp.setLayout(null);
        IComboBox icb = new IComboBox();
        jp.add(icb, null);
        jf.setContentPane(jp);
        jf.setVisible(true);
    }

    /**
     This is the default constructor
     
     **/
    public IComboBox() {
        super();
        init();
    }

    /**
     This method initializes this
     
     **/
    private void init() {
        this.setSize(320, 20);
        this.setEditable(false);
        this.editor.addActionListener(this);
        this.addMouseListener(this);
        this.addKeyListener(this);
        this.getEditor().getEditorComponent().addKeyListener(this);
        this.getEditor().getEditorComponent().addFocusListener(this);
        this.setToolTipText("<html>Double Click to add an entry, then finish by press ENTER.<br>"
                            + "Selecting DELETE will remove selected entry.</html>");
    }

    public void keyPressed(KeyEvent arg0) {
        // TODO Auto-generated method stub
    }

    /* (non-Javadoc)
     * @see java.awt.event.KeyListener#keyReleased(java.awt.event.KeyEvent)
     * 
     * Override keyReleased to listen key action
     * 
     */
    public void keyReleased(KeyEvent arg0) {
        //
        //Add new item to list when press ENTER
        //
        if (arg0.getSource() == this.getEditor().getEditorComponent()) {
            if (arg0.getKeyCode() == KeyEvent.VK_ENTER) {
                String strCurrentText = this.getEditor().getItem().toString().trim();
                if (strCurrentText.length() == 0) {
                    if (this.getItemCount() > 0) {
                        this.setSelectedIndex(0);
                    }
                } else {
                    this.addItem(strCurrentText);
                    this.setSelectedItem(strCurrentText);
                }
                this.setEditable(false);
            }

            if (arg0.getKeyCode() == KeyEvent.VK_ESCAPE) {
                closeEdit();
            }
        }

        if (arg0.getSource() == this) {
            //
            //Remove item from the list when press DEL
            //
            if (arg0.getKeyCode() == KeyEvent.VK_DELETE) {
                int intSelected = this.getSelectedIndex();
                if (intSelected > -1) {
                    this.removeItemAt(this.getSelectedIndex());
                    if (this.getItemCount() > 0) {
                        this.setSelectedIndex(0);
                    } else {
                        this.removeAllItems();
                    }
                }
            }
        }
    }

    public void keyTyped(KeyEvent arg0) {
        // TODO Auto-generated method stub
    }

    /* (non-Javadoc)
     * @see java.awt.event.MouseListener#mouseClicked(java.awt.event.MouseEvent)
     * 
     * Override mouseClicked to enter edit mode when double click mouse
     * 
     */
    public void mouseClicked(MouseEvent arg0) {
        if (arg0.getClickCount() == 2) {
            this.setEditable(true);
            this.getEditor().setItem("");
        }
    }

    public void mouseEntered(MouseEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void mouseExited(MouseEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void mousePressed(MouseEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void mouseReleased(MouseEvent arg0) {
        // TODO Auto-generated method stub

    }

    /**
     Exit edit mode
     
     **/
    private void closeEdit() {
        this.setEditable(false);
        this.getEditor().setItem("");
    }

    /**
     Set the input item as selected
     
     @param item the item which is needed to be set selected
     
     **/
    public void setSelectedItem(Object item) {
        boolean isFind = false;
        //
        // If the input value is not in the default list, add it to the list
        //
        if (item != null) {
            for (int index = 0; index < this.getItemCount(); index++) {
                if (this.getItemAt(index).equals(item)) {
                    isFind = true;
                    break;
                }
            }
            //
            // Add this item to IComboBox if not found
            //
            if (!isFind && !item.toString().equals("")) {
                super.addItem(item);
            }
        }
        
        //
        // Call super function to set the item selected.
        //
        super.setSelectedItem(item);
    }
}
