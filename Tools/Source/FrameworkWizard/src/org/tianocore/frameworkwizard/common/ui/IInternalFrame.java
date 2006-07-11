/** @file
 
 The file is used to override JInternalFrame to provides customized interfaces 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.common.ui;

import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ComponentEvent;
import java.awt.event.ComponentListener;
import java.awt.event.FocusEvent;
import java.awt.event.FocusListener;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;

import javax.swing.JDesktopPane;
import javax.swing.JFrame;
import javax.swing.JInternalFrame;
import javax.swing.JPanel;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;

/**
 * The class is used to override JInternalFrame to provides customized
 * interfaces It extends JInternalFrame implements ActionListener
 * 
 *
 * 
 */
public class IInternalFrame extends JInternalFrame implements ActionListener, ComponentListener, ItemListener,
                                                  FocusListener, ListSelectionListener, TableModelListener, MouseListener {

    // /
    // / Define class Serial Version UID
    // /
    private static final long serialVersionUID = -609841772384875886L;

    //
    // Define class members
    //
    private boolean isEdited = false;

    /**
     * Main class, used for test
     * 
     * @param args
     * 
     */
    public static void main(String[] args) {
        JFrame jf = new JFrame();
        JPanel jp = new JPanel();
        JDesktopPane jdp = new JDesktopPane();
        IInternalFrame itf = new IInternalFrame();
        jdp.add(itf, 1);
        jf.setContentPane(jp);
        jf.setVisible(true);
    }

    /**
     * This is the default constructor
     * 
     */
    public IInternalFrame() {
        super();
        initialize();
    }

    /**
     * This method initializes this
     * 
     */
    private void initialize() {
        this.setBounds(new java.awt.Rectangle(0, 0, 520, 545));
        this.setMinimumSize(new java.awt.Dimension(520, 545));
        this.addComponentListener(this);
    }

    /**
     * Get if the InternalFrame has been edited
     * 
     * @retval true - The InternalFrame has been edited
     * @retval false - The InternalFrame hasn't been edited
     * 
     */
    public boolean isEdited() {
        return isEdited;
    }

    /**
     * Set if the InternalFrame has been edited
     * 
     * @param isEdited
     *            The input data which identify if the InternalFrame has been
     *            edited
     * 
     */
    public void setEdited(boolean isEdited) {
        this.isEdited = isEdited;
    }

    /**
     * Check the input data is empty or not
     * 
     * @param strValue
     *            The input data which need be checked
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

    public void actionPerformed(ActionEvent arg0) {
        // TODO Auto-generated method stub
    }

    public void componentHidden(ComponentEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void componentMoved(ComponentEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void componentResized(ComponentEvent arg0) {
        // TODO Auto-generated method stub
    }

    public void componentShown(ComponentEvent arg0) {
        // TODO Auto-generated method stub

    }

    /**
     * To reset the width of input component via container width
     * 
     * @param c
     * @param containerWidth
     * 
     */
    public void resizeComponentWidth(Component c, int containerWidth, int preferredWidth) {
        int newWidth = c.getPreferredSize().width + (containerWidth - preferredWidth);
        if (newWidth < c.getPreferredSize().width) {
            newWidth = c.getPreferredSize().width;
        }
        c.setSize(new java.awt.Dimension(newWidth, c.getHeight()));
        c.validate();
    }

    /**
     * To reset the height of input component via container height
     * 
     * @param c
     * @param containerHeight
     * 
     */
    public void resizeComponentHeight(Component c, int containerHeight, int preferredHeight) {
        int newHeight = c.getPreferredSize().height + (containerHeight - preferredHeight);
        if (newHeight < c.getPreferredSize().height) {
            newHeight = c.getPreferredSize().height;
        }
        c.setSize(new java.awt.Dimension(c.getWidth(), newHeight));
        c.validate();
    }

    /**
     * To reset the size of input component via container size
     * 
     * @param c
     * @param containerWidth
     * @param containerHeight
     * 
     */
    public void resizeComponent(Component c, int containerWidth, int containerHeight, int preferredWidth,
                                int preferredHeight) {
        resizeComponentWidth(c, containerWidth, preferredWidth);
        resizeComponentHeight(c, containerHeight, preferredHeight);
    }

    /**
     * To relocate the input component
     * 
     * @param c
     * @param containerWidth
     * @param spaceToRight
     * 
     */
    public void relocateComponentX(Component c, int containerWidth, int preferredWidth, int spaceToRight) {
        int intGapToRight = spaceToRight + c.getPreferredSize().width;
        int newLocationX = containerWidth - intGapToRight;
        if (newLocationX < preferredWidth - intGapToRight) {
            newLocationX = preferredWidth - intGapToRight;
        }
        c.setLocation(newLocationX, c.getLocation().y);
        c.validate();
    }

    /**
     * To relocate the input component
     * 
     * @param c
     * @param containerHeight
     * @param spaceToBottom
     * 
     */
    public void relocateComponentY(Component c, int containerHeight, int preferredHeight, int spaceToBottom) {
        int intGapToBottom = spaceToBottom + c.getPreferredSize().height;
        int newLocationY = containerHeight - intGapToBottom;
        if (newLocationY < preferredHeight - spaceToBottom) {
            newLocationY = preferredHeight - spaceToBottom;
        }
        c.setLocation(c.getLocation().x, newLocationY);
        c.validate();
    }

    /**
     * To relocate the input component
     * 
     * @param c
     * @param containerWidth
     * @param containerHeight
     * @param spaceToBottom
     * @param spaceToRight
     * 
     */
    public void relocateComponent(Component c, int containerWidth, int containerHeight, int preferredWidht,
                                  int preferredHeight, int spaceToRight, int spaceToBottom) {
        relocateComponentX(c, containerWidth, preferredWidht, spaceToRight);
        relocateComponentY(c, containerHeight, preferredHeight, spaceToBottom);
    }

    public void showStandard() {

    }

    public void showAdvanced() {

    }

    public void showXML() {

    }

    public void itemStateChanged(ItemEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void focusGained(FocusEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void focusLost(FocusEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void valueChanged(ListSelectionEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void tableChanged(TableModelEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void mouseClicked(MouseEvent arg0) {
        // TODO Auto-generated method stub
        
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
}
