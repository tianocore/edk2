/** @file
 Java class LongTextEditor.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
**/
package org.tianocore.frameworkwizard.platform.ui;

import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.AbstractCellEditor;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JTable;
import javax.swing.table.TableCellEditor;


/**
 Editor for table cell with Long Text.
 @since PackageEditor 1.0
 **/
public class LongTextEditor extends AbstractCellEditor implements TableCellEditor, ActionListener {

    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    String text;
    JButton button;
    GenLongTextDialog dialog;
    protected static final String EDIT = "editLongText";

    public LongTextEditor(JFrame frame) {
        
        button = new JButton();
        button.setActionCommand(EDIT);
        button.addActionListener(this);
        button.setBorderPainted(false);
        
        dialog = new GenLongTextDialog(this, frame);
        
    }

    /* (non-Javadoc)
     * @see javax.swing.table.TableCellEditor#getTableCellEditorComponent(javax.swing.JTable, java.lang.Object, boolean, int, int)
     */
    public Component getTableCellEditorComponent(JTable arg0, Object arg1, boolean arg2, int arg3, int arg4) {
        // TODO Auto-generated method stub
        text = (String)arg1;
        return button;
    }

    /* (non-Javadoc)
     * @see javax.swing.CellEditor#getCellEditorValue()
     */
    public Object getCellEditorValue() {
        // TODO Auto-generated method stub
        return text;
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     */
    public void actionPerformed(ActionEvent arg0) {
        // TODO Auto-generated method stub
        if (EDIT.equals(arg0.getActionCommand())) {
            //The user has clicked the cell, so
            //bring up the dialog.
            button.setText(text);
            dialog.setText(text);
            dialog.setVisible(true);

            //Make the renderer reappear.
            fireEditingStopped();
        }
        else { //User pressed dialog's "OK" button.
            
            text = dialog.getText().trim();
            dialog.dispose();
        }

    }

}
