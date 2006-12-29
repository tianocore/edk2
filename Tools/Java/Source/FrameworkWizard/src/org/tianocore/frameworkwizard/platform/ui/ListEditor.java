/** @file
 Java class GuidEditor.
 
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
import java.util.Vector;

import javax.swing.AbstractCellEditor;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JTable;
import javax.swing.table.TableCellEditor;


/**
 Editor for table cell with GUID value.
 @since PackageEditor 1.0
 **/
public class ListEditor extends AbstractCellEditor implements TableCellEditor, ActionListener {

    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    private boolean canNotBeEmpty = false;
    private JFrame topFrame = null;
    String archs;
    JButton button;
    GenListDialog dialog;
    protected static final String EDIT = "edit";

    public ListEditor(JFrame frame) {
        topFrame = frame;
        button = new JButton();
        button.setActionCommand(EDIT);
        button.addActionListener(this);
        button.setBorderPainted(false);

        
        dialog = new GenListDialog(this, frame);
        
    }
    
    public ListEditor(Vector<String> v, JFrame frame) {
        this(frame);
        dialog.initList(v);
    }

    /* (non-Javadoc)
     * @see javax.swing.table.TableCellEditor#getTableCellEditorComponent(javax.swing.JTable, java.lang.Object, boolean, int, int)
     */
    public Component getTableCellEditorComponent(JTable arg0, Object arg1, boolean arg2, int arg3, int arg4) {
        // TODO Auto-generated method stub
        archs = (String)arg1;
        return button;
    }

    /* (non-Javadoc)
     * @see javax.swing.CellEditor#getCellEditorValue()
     */
    public Object getCellEditorValue() {
        // TODO Auto-generated method stub
        return archs;
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     */
    public void actionPerformed(ActionEvent arg0) {
        // TODO Auto-generated method stub
        if (EDIT.equals(arg0.getActionCommand())) {
            //The user has clicked the cell, so
            //bring up the dialog.
            button.setText(archs);
            dialog.setList(archs);
            dialog.setVisible(true);

            //Make the renderer reappear.
            fireEditingStopped();
        }
        else { //User pressed dialog's "OK" button.
            Vector<String> v = dialog.getList();
            if (canNotBeEmpty && v.size() == 0) {
                JOptionPane.showMessageDialog(topFrame, "You must select at least one item.");
                return;
            }
            String s = " ";
            for (int i = 0; i < v.size(); ++i) {
                s += v.get(i);
                s += " ";
            }
            archs = s.trim();
            dialog.dispose();
        }

    }

    /**
     * @param canNotBeEmpty The canNotBeEmpty to set.
     */
    public void setCanNotBeEmpty(boolean canNotBeEmpty) {
        this.canNotBeEmpty = canNotBeEmpty;
    }

}
