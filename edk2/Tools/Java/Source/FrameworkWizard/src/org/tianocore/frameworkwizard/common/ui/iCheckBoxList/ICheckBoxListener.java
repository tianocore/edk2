/** @file
 
 The file is used to create listener for Checkbox List
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.common.ui.iCheckBoxList;

import java.awt.event.*;

class ICheckBoxListener implements MouseListener, KeyListener {

    protected ICheckBoxList iCheckboxlist;

    /**
     This is the default constructor
     
     @param parent
     
     **/
    public ICheckBoxListener(ICheckBoxList parent) {
        iCheckboxlist = parent;
    }

    /* (non-Javadoc)
     * @see java.awt.event.KeyListener#keyReleased(java.awt.event.KeyEvent)
     * Override to deal with keyReleased event
     * 
     *
     */
    public void keyReleased(KeyEvent e) {
        Object[] selectedValues = iCheckboxlist.getSelectedValues();
        int[] selectedIndices = iCheckboxlist.getSelectedIndices();

        for (int index = 0; index < selectedValues.length; index++) {
            ICheckBoxListItem item = (ICheckBoxListItem) selectedValues[index];

            if (iCheckboxlist.isEnabled()) {
                if (e.getKeyCode() == KeyEvent.VK_SPACE) {
                    //
                    //if press space key, then reverse all selected item.
                    //
                    item.invertChecked();
                }
                ((ICheckBoxListModel) iCheckboxlist.getModel()).setElementAt(item, selectedIndices[index]);
            }
        }
    }


    /* (non-Javadoc)
     * @see java.awt.event.MouseListener#mouseClicked(java.awt.event.MouseEvent)
     * Override to deal with mouse clicked event
     * 
     */
    public void mouseClicked(MouseEvent e) {
        int index = iCheckboxlist.locationToIndex(e.getPoint());
        ICheckBoxListItem item = null;
        item = (ICheckBoxListItem) iCheckboxlist.getModel().getElementAt(index);
        
        if (item != null && iCheckboxlist.isEnabled()) {
            item.invertChecked();
            ((ICheckBoxListModel) iCheckboxlist.getModel()).setElementAt(item, index);
        }
    }

    public void mousePressed(MouseEvent arg0) {
        // TODO Auto-generated method stub
        
    }

    public void mouseReleased(MouseEvent arg0) {
        // TODO Auto-generated method stub
        
    }

    public void mouseEntered(MouseEvent arg0) {
        // TODO Auto-generated method stub
        
    }

    public void mouseExited(MouseEvent arg0) {
        // TODO Auto-generated method stub
        
    }

    public void keyPressed(KeyEvent arg0) {
        // TODO Auto-generated method stub
        
    }

    public void keyTyped(KeyEvent arg0) {
        // TODO Auto-generated method stub
        
    }
}
