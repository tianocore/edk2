/** @file
  Java class FpdFvOptions is GUI for FV options in FPD file.
 
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
import java.awt.event.MouseEvent;
import java.awt.event.WindowEvent;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Set;

import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JDialog;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.ListSelectionModel;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableModel;
import javax.swing.JButton;

import org.tianocore.frameworkwizard.FrameworkWizardUI;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPlatformType;

/**
 * 
 *
 */
public class FpdFvOptions extends JDialog {

    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    private static JFrame frame;
    private JPanel jContentPane = null;
    private JPanel jPanelN = null;
    private JPanel jPanelS = null;
    private JPanel jPanelC = null;
    private JScrollPane jScrollPaneFvOptions = null;
    private JTable jTableFvOptions = null;
    private DefaultTableModel tableModel = null;
    private String fvName = null;
    private FpdFileContents ffc = null;
    private OpeningPlatformType docConsole = null;
    private JButton jButtonNew = null;
    private JButton jButtonDelete = null;
    private String oldOptionName = "";
    private int selectedRow = -1;
    private TableModelListener tableModelListener = null;

    /**
     * This is the default constructor
     */
    public FpdFvOptions(String name, DefaultTableModel tm, FpdFileContents ffc, OpeningPlatformType dc) {
        super(FrameworkWizardUI.getInstance());
        fvName = name;
        this.ffc = ffc;
        this.docConsole = dc;
        setTableModel(tm);
        initOptions();
        initialize();
        
    }
    
    protected void processWindowEvent (WindowEvent e) {
        if (e.getID() == WindowEvent.WINDOW_CLOSING) {
            if (jTableFvOptions.isEditing()) {
                jTableFvOptions.getCellEditor().stopCellEditing();
            }
            tableModel.removeTableModelListener(tableModelListener);
            this.dispose();
        }
    }

    private void initOptions() {
        tableModel.setRowCount(0);
        LinkedHashMap<String, String> mOpts = new LinkedHashMap<String, String>();
        ffc.getFvImagesFvImageOptions(fvName, mOpts);
        Set<String> sKey = mOpts.keySet();
        Iterator<String> iter = sKey.iterator();
        while (iter.hasNext()) {
            String name = iter.next();
            String value = mOpts.get(name);
            tableModel.addRow(new String[]{name, value});
        }
    }
    
    private boolean fvOptionNameExists (String name) {
        int count = 0;
        for (int i = 0; i < jTableFvOptions.getRowCount(); ++i) {
            if (getTableModel().getValueAt(i, 0).equals(name)) {
                ++count;
            }
        }
        if (count > 1) {
            return true;
        }
        return false;
    }
    /**
     * This method initializes this
     * 
     * @return void
     */
    private void initialize() {
        this.setSize(650, 400);
        this.setModal(true);
        this.setTitle("FV Options");
        this.setDefaultCloseOperation(JDialog.DO_NOTHING_ON_CLOSE);
        this.setContentPane(getJContentPane());
        this.centerWindow();
        this.setVisible(true);
        
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
            jContentPane.add(getJPanelN(), java.awt.BorderLayout.NORTH);
            jContentPane.add(getJPanelS(), java.awt.BorderLayout.SOUTH);
            jContentPane.add(getJPanelC(), java.awt.BorderLayout.CENTER);
        }
        return jContentPane;
    }

    /**
     * This method initializes jPanelN	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelN() {
        if (jPanelN == null) {
            jPanelN = new JPanel();
        }
        return jPanelN;
    }

    /**
     * This method initializes jPanelS	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelS() {
        if (jPanelS == null) {
            jPanelS = new JPanel();
            jPanelS.add(getJButtonNew(), null);
            jPanelS.add(getJButtonDelete(), null);
        }
        return jPanelS;
    }

    /**
     * This method initializes jPanelC	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelC() {
        if (jPanelC == null) {
            jPanelC = new JPanel();
            jPanelC.add(getJScrollPaneFvOptions(), null);
        }
        return jPanelC;
    }

    /**
     * This method initializes jScrollPaneFvOptions	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneFvOptions() {
        if (jScrollPaneFvOptions == null) {
            jScrollPaneFvOptions = new JScrollPane();
            jScrollPaneFvOptions.setPreferredSize(new java.awt.Dimension(600,320));
            jScrollPaneFvOptions.setViewportView(getJTableFvOptions());
        }
        return jScrollPaneFvOptions;
    }

    /**
     * This method initializes jTableFvOptions	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableFvOptions() {
        if (jTableFvOptions == null) {
            jTableFvOptions = new JTable(getTableModel()) {
                /**
                 * 
                 */
                private static final long serialVersionUID = -1941328952828651192L;

                public String getToolTipText(MouseEvent e) {
                    String tip = null;
                    java.awt.Point p = e.getPoint();
                    int rowIndex = rowAtPoint(p);
//                    int colIndex = columnAtPoint(p);
//                    int realColumnIndex = convertColumnIndexToModel(colIndex);

                    TableModel model = getModel();
                    String optName = (String) model.getValueAt(rowIndex, 0);
                    if (((FvOptsTableModel)model).getVKeyWords().contains(optName)){
                        tip = optName + " is from Flash Definition File and it is NOT editable.";
                    }
                         
                    return tip;
                }

            };
 
            jTableFvOptions.setRowHeight(20);
            
            jTableFvOptions.getSelectionModel().addListSelectionListener(new ListSelectionListener() {
                public void valueChanged(ListSelectionEvent e) {
                    if (e.getValueIsAdjusting()) {
                        return;
                    }
                    ListSelectionModel lsm = (ListSelectionModel) e.getSource();
                    if (lsm.isSelectionEmpty()) {
                        return;
                    } else {
                        selectedRow = lsm.getMinSelectionIndex();
                        oldOptionName = getTableModel().getValueAt(selectedRow, 0)+"";
                    }
                }
            });
            
            tableModelListener = new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    int col = arg0.getColumn();
                    TableModel m = (TableModel) arg0.getSource();
                    
                    if (arg0.getType() == TableModelEvent.UPDATE) {
                        String newOptionName = m.getValueAt(row, 0) + "";
                        if (col == 0) {
                            if (newOptionName.equals(oldOptionName)) {
                                return;
                            }
                            if (fvOptionNameExists(newOptionName)) {
                                JOptionPane.showMessageDialog(frame, "This Option already exists. Please choose another Option name.");
                                m.setValueAt(oldOptionName, row, 0);
                                return;
                            }
                            
                            ffc.setTypedNamedFvImageNameValue(fvName, "Options", oldOptionName, m.getValueAt(row, 1)+"", newOptionName);
                            docConsole.setSaved(false);
                            oldOptionName = newOptionName;
                        }
                        
                        if (col == 1) {
                            ffc.setTypedNamedFvImageNameValue(fvName, "Options", oldOptionName, m.getValueAt(row, 1)+"", newOptionName);
                            docConsole.setSaved(false);    
                        }
                                            
                    }
                }
            };
            
            jTableFvOptions.getModel().addTableModelListener(tableModelListener);
        }
        return jTableFvOptions;
    }

    protected DefaultTableModel getTableModel() {
        return tableModel;
    }

    protected void setTableModel(DefaultTableModel tableModel) {
        
        this.tableModel = tableModel;
        
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

/**
 * This method initializes jButtonNew	
 * 	
 * @return javax.swing.JButton	
 */
private JButton getJButtonNew() {
    if (jButtonNew == null) {
        jButtonNew = new JButton();
        jButtonNew.setPreferredSize(new java.awt.Dimension(80,20));
        jButtonNew.setText("New");
        jButtonNew.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent e) {
                tableModel.addRow(new String[]{"", ""});
                oldOptionName = "";
            }
        });
    }
    return jButtonNew;
}

/**
 * This method initializes jButtonDelete	
 * 	
 * @return javax.swing.JButton	
 */
private JButton getJButtonDelete() {
    if (jButtonDelete == null) {
        jButtonDelete = new JButton();
        jButtonDelete.setPreferredSize(new java.awt.Dimension(80,20));
        jButtonDelete.setText("Delete");
        jButtonDelete.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent e) {
                int selectedRow = jTableFvOptions.getSelectedRow();
                if (selectedRow < 0) {
                    return;
                }
                String optName = tableModel.getValueAt(selectedRow, 0)+"";
                if (((FvOptsTableModel)tableModel).getVKeyWords().contains(optName)){
                    return;
                }
                if (((FvOptsTableModel)tableModel).getVNonEditableName().contains(optName)){
                    return;
                }
                
                ffc.removeTypedNamedFvImageNameValue(fvName, "Options", optName);
                tableModel.removeRow(selectedRow);
                docConsole.setSaved(false);
            }
        });
    }
    return jButtonDelete;
}
}
