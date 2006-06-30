/** @file
 
 The file is used to create cell renderer for CheckBoxList Item 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.common.ui.iCheckBoxList;

import java.awt.*;
import javax.swing.*;
import javax.swing.border.*;

class ICheckBoxListCellRenderer extends JCheckBox implements ListCellRenderer {
    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -1718072217181674870L;

    protected static Border noFocusBorder = new EmptyBorder(1, 1, 1, 1);

    /**
     This is the default Constructor
     
     **/
    public ICheckBoxListCellRenderer() {
        super();
        setOpaque(true);
        setBorder(noFocusBorder);
    }

    /* (non-Javadoc)
     * @see javax.swing.ListCellRenderer#getListCellRendererComponent(javax.swing.JList, java.lang.Object, int, boolean, boolean)
     * Override to get attribute of the ICheckListCellRenderer
     * 
     */
    public Component getListCellRendererComponent(JList list, Object value, int index, boolean isSelected,
                                                  boolean cellHasFocus) {
        ICheckBoxListItem item = (ICheckBoxListItem) value;

        setComponentOrientation(list.getComponentOrientation());
        
        if (item.isChecked()) {
            setBackground(list.getSelectionBackground());
            setForeground(list.getSelectionForeground());
        } else {
            if (isSelected) {
                setBackground(Color.LIGHT_GRAY);    
                setForeground(list.getForeground());                
            } else {
                setBackground(list.getBackground());
                setForeground(list.getForeground());
            }
        }

        if (value instanceof ICheckBoxListItem) {
            setText(item.getText());
            setSelected(item.isChecked());
        } else {
            setIcon(null);
            setText((value == null) ? "" : value.toString());
        }

        setEnabled(list.isEnabled());
        setFont(list.getFont());

        return this;
    }
}
