/** @file

 The file is used to show a table with all defined PPIs

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.frameworkwizard.common.find;

import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ComponentEvent;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.WindowEvent;
import java.util.Vector;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.ListSelectionModel;
import javax.swing.SwingConstants;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;
import javax.swing.table.DefaultTableCellRenderer;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.IDefaultTableModel;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.module.Identifications.LibraryClass.LibraryClassVector;
import org.tianocore.frameworkwizard.module.Identifications.PcdCoded.PcdCodedVector;

public class FindResult extends IFrame implements TableModelListener, ListSelectionListener, MouseListener {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -2448484533878401714L;

    //
    // Define class members
    //
    private JTable jTable = null;

    private JPanel jContentPane = null;

    private JScrollPane jScrollPane = null;

    private JScrollPane jScrollPaneTable = null;

    private JButton jButtonClose = null;

    //
    // Not used by UI
    //
    private IDefaultTableModel model = null;

    private String method = "";

    private static FindResult findPpisResult = null;

    private static FindResult findProtocolsResult = null;

    private static FindResult findGuidsResult = null;

    private static FindResult findPcdsResult = null;

    private static FindResult findLibraryClassResult = null;

    private int selectedRow = -1;

    private LibraryClassVector lcv = null;

    private Vector<FindResultId> vLibraryClassFindResult = null;

    private PcdCodedVector pv = null;

    private Vector<PcdFindResultId> vPcdFindResult = null;

    private JButton jButtonReload = null;

    private JPanel jPanelOperation = null;

    /**
     * This is the default constructor
     */
    public FindResult(String method) {
        super();
        init(method);
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
    private void init(String method) {
        this.setSize(600, 500);
        this.setContentPane(getJScrollPane());
        this.setTitle("Find Result");
        this.setResizable(true);
        this.setDefaultCloseOperation(JFrame.HIDE_ON_CLOSE);
        this.centerWindow();

        //
        // max the window
        //
        //this.setExtendedState(JFrame.MAXIMIZED_BOTH);
        this.method = method;
        this.showTable();
    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJScrollPaneTable(), null);
            jContentPane.add(getJPanelOperation(), null);
            jContentPane.setPreferredSize(new java.awt.Dimension(585, 445));
        }
        return jContentPane;
    }

    /**
     This method initializes jScrollPane  
     
     @return javax.swing.JScrollPane  
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setViewportView(getJContentPane());
        }
        return jScrollPane;
    }

    /**
     This method initializes jScrollPaneTable    
     
     @return javax.swing.JScrollPane 
     **/
    private JScrollPane getJScrollPaneTable() {
        if (jScrollPaneTable == null) {
            jScrollPaneTable = new JScrollPane();
            jScrollPaneTable.setBounds(new java.awt.Rectangle(0, 0, 585, 395));
            jScrollPaneTable.setPreferredSize(new Dimension(585, 395));
            jScrollPaneTable.setViewportView(getJTable());
        }
        return jScrollPaneTable;
    }

    /**
     This method initializes jTable  
     
     @return javax.swing.JTable  
     **/
    private JTable getJTable() {
        if (jTable == null) {
            model = new IDefaultTableModel();
            jTable = new JTable(model);
            jTable.setAutoResizeMode(javax.swing.JTable.AUTO_RESIZE_OFF);
            jTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);

            jTable.getSelectionModel().addListSelectionListener(this);
            jTable.getModel().addTableModelListener(this);
            jTable.addMouseListener(this);

            model.addColumn("Name");
            model.addColumn("Type");
            model.addColumn("Produced by");
            model.addColumn("Consumed by");
            model.addColumn("Declared by");

            jTable.getColumn("Name").setCellRenderer(new MyTableCellRenderer());
            jTable.getColumn("Type").setCellRenderer(new MyTableCellRenderer());
            jTable.getColumn("Produced by").setCellRenderer(new MyTableCellRenderer());
            jTable.getColumn("Consumed by").setCellRenderer(new MyTableCellRenderer());
            jTable.getColumn("Declared by").setCellRenderer(new MyTableCellRenderer());

            int columnWidth = (this.getSize().width - 28) / 5;
            jTable.getColumn("Name").setPreferredWidth(columnWidth);
            jTable.getColumn("Type").setPreferredWidth(columnWidth);
            jTable.getColumn("Produced by").setPreferredWidth(columnWidth);
            jTable.getColumn("Consumed by").setPreferredWidth(columnWidth);
            jTable.getColumn("Declared by").setPreferredWidth(columnWidth);
        }
        return jTable;
    }

    /**
     This method initializes jButtonAdd 
     
     @return javax.swing.JButton jButtonAdd
     
     **/
    private JButton getJButtonClose() {
        if (jButtonClose == null) {
            jButtonClose = new JButton();
            jButtonClose.setText("Close");
            jButtonClose.addActionListener(this);
            jButtonClose.setPreferredSize(new java.awt.Dimension(80, 20));
            jButtonClose.setBounds(new java.awt.Rectangle(90, 0, 80, 20));
        }
        return jButtonClose;
    }

    public static FindResult getInstance(String method) {
        if (method.equals("PPI")) {
            if (findPpisResult == null) {
                findPpisResult = new FindResult(method);
            }
            findPpisResult.init(method);
            return findPpisResult;
        }

        if (method.equals("PROTOCOL")) {
            if (findProtocolsResult == null) {
                findProtocolsResult = new FindResult(method);
            }
            findProtocolsResult.init(method);
            return findProtocolsResult;
        }

        if (method.equals("GUID")) {
            if (findGuidsResult == null) {
                findGuidsResult = new FindResult(method);
            }
            findGuidsResult.init(method);
            return findGuidsResult;
        }

        if (method.equals("PCD")) {
            if (findPcdsResult == null) {
                findPcdsResult = new FindResult(method);
            }
            findPcdsResult.init(method);
            return findPcdsResult;
        }

        if (method.equals("LIBRARY_CLASS")) {
            if (findLibraryClassResult == null) {
                findLibraryClassResult = new FindResult(method);
            }
            findLibraryClassResult.init(method);
            return findLibraryClassResult;
        }

        return null;
    }

    /**
     Clear all table rows
     
     **/
    private void clearAll() {
        if (model != null) {
            for (int index = model.getRowCount() - 1; index >= 0; index--) {
                model.removeRow(index);
            }
        }
    }

    /**
     Read content of vector and put then into table
     
     **/
    private void showTable() {
        clearAll();
        this.jTable.repaint();
        this.jTable.updateUI();

        if (this.method.equals("PPI")) {
            Vector<PpiId> vPpi = Find.getAllPpisForFind();

            if (vPpi.size() > 0) {

                for (int index = 0; index < vPpi.size(); index++) {
                    Vector<String> v = new Vector<String>();
                    v.addElement(vPpi.elementAt(index).getName());
                    v.addElement(vPpi.elementAt(index).getType());
                    String strProducedModules = vPpi.elementAt(index).getProducedModules();
                    if (strProducedModules.indexOf("<br>") == 0) {
                        strProducedModules = strProducedModules.substring("<br>".length());
                    }
                    int line1 = Tools.getSpecificStringCount(strProducedModules, "<br>");
                    v.addElement("<html>" + strProducedModules + "</html>");

                    String strConsumedModules = vPpi.elementAt(index).getConsumedModules();
                    if (strConsumedModules.indexOf("<br>") == 0) {
                        strConsumedModules = strConsumedModules.substring("<br>".length());
                    }
                    int line2 = Tools.getSpecificStringCount(strConsumedModules, "<br>");
                    v.addElement("<html>" + strConsumedModules + "</html>");

                    v.addElement(vPpi.elementAt(index).getDeclaredBy());

                    model.addRow(v);
                    jTable.setRowHeight(index, (Math.max(line1, line2) > 1 ? Math.max(line1, line2) : 1) * 18);
                }
            } else {
                Log.wrn("Find PPIs", "No PPI found!!!");
            }
        }

        if (this.method.equals("PROTOCOL")) {
            Vector<ProtocolId> vProtocol = Find.getAllProtocolsForFind();

            if (vProtocol.size() > 0) {

                for (int index = 0; index < vProtocol.size(); index++) {
                    Vector<String> v = new Vector<String>();
                    v.addElement(vProtocol.elementAt(index).getName());
                    v.addElement(vProtocol.elementAt(index).getType());
                    String strProducedModules = vProtocol.elementAt(index).getProducedModules();
                    if (strProducedModules.indexOf("<br>") == 0) {
                        strProducedModules = strProducedModules.substring("<br>".length());
                    }
                    int line1 = Tools.getSpecificStringCount(strProducedModules, "<br>");
                    v.addElement("<html>" + strProducedModules + "</html>");

                    String strConsumedModules = vProtocol.elementAt(index).getConsumedModules();
                    if (strConsumedModules.indexOf("<br>") == 0) {
                        strConsumedModules = strConsumedModules.substring("<br>".length());
                    }
                    int line2 = Tools.getSpecificStringCount(strConsumedModules, "<br>");
                    v.addElement("<html>" + strConsumedModules + "</html>");

                    v.addElement(vProtocol.elementAt(index).getDeclaredBy());

                    model.addRow(v);
                    jTable.setRowHeight(index, (Math.max(line1, line2) > 1 ? Math.max(line1, line2) : 1) * 18);
                }
            } else {
                Log.wrn("Find PROTOCOLs", "No PROTOCOL found!!!");
            }
        }

        if (this.method.equals("GUID")) {
            Vector<GuidId> vGuid = Find.getAllGuidsForFind();

            if (vGuid.size() > 0) {

                for (int index = 0; index < vGuid.size(); index++) {
                    Vector<String> v = new Vector<String>();
                    v.addElement(vGuid.elementAt(index).getName());
                    v.addElement("GUID");
                    String strProducedModules = vGuid.elementAt(index).getProducedModules();
                    if (strProducedModules.indexOf("<br>") == 0) {
                        strProducedModules = strProducedModules.substring("<br>".length());
                    }
                    int line1 = Tools.getSpecificStringCount(strProducedModules, "<br>");
                    v.addElement("<html>" + strProducedModules + "</html>");

                    String strConsumedModules = vGuid.elementAt(index).getConsumedModules();
                    if (strConsumedModules.indexOf("<br>") == 0) {
                        strConsumedModules = strConsumedModules.substring("<br>".length());
                    }
                    int line2 = Tools.getSpecificStringCount(strConsumedModules, "<br>");
                    v.addElement("<html>" + strConsumedModules + "</html>");

                    v.addElement(vGuid.elementAt(index).getDeclaredBy());

                    model.addRow(v);
                    jTable.setRowHeight(index, (Math.max(line1, line2) > 1 ? Math.max(line1, line2) : 1) * 18);
                }
            } else {
                Log.wrn("Find GUIDs", "No GUID found!!!");
            }
        }

        if (this.method.equals("PCD")) {
            pv = Find.getAllPcdCodedVector();
            vPcdFindResult = Find.getAllPcdCodedForFind(pv);

            if (vPcdFindResult.size() > 0) {

                for (int index = 0; index < vPcdFindResult.size(); index++) {
                    Vector<String> v = new Vector<String>();
                    v.addElement(vPcdFindResult.elementAt(index).getName());
                    v.addElement(vPcdFindResult.elementAt(index).getType());

                    //
                    // Generate Produced Modules List
                    //
                    String strProducedModules = "";
                    Vector<ModuleIdentification> vModule = vPcdFindResult.elementAt(index).getProducedModules();
                    for (int indexOfPM = 0; indexOfPM < vModule.size(); indexOfPM++) {
                        strProducedModules = strProducedModules + "<br>"
                                             + vModule.get(indexOfPM).getPackageId().getName() + "."
                                             + vModule.get(indexOfPM).getName();
                    }
                    if (strProducedModules.indexOf("<br>") == 0) {
                        strProducedModules = strProducedModules.substring("<br>".length());
                    }
                    int line1 = Tools.getSpecificStringCount(strProducedModules, "<br>");
                    v.addElement("<html>" + strProducedModules + "</html>");

                    //
                    // Generate Consumed Modules List
                    //
                    String strConsumedModules = "";
                    vModule = vPcdFindResult.elementAt(index).getConsumedModules();
                    for (int indexOfCM = 0; indexOfCM < vModule.size(); indexOfCM++) {
                        strConsumedModules = strConsumedModules + "<br>"
                                             + vModule.get(indexOfCM).getPackageId().getName() + "."
                                             + vModule.get(indexOfCM).getName();
                    }
                    if (strConsumedModules.indexOf("<br>") == 0) {
                        strConsumedModules = strConsumedModules.substring("<br>".length());
                    }
                    int line2 = Tools.getSpecificStringCount(strConsumedModules, "<br>");
                    v.addElement("<html>" + strConsumedModules + "</html>");

                    //
                    // Add declare package name
                    //
                    v.addElement(vPcdFindResult.elementAt(index).getDeclaredBy().getName());

                    model.addRow(v);
                    jTable.setRowHeight(index, (Math.max(line1, line2) > 1 ? Math.max(line1, line2) : 1) * 18);
                }
            } else {
                Log.wrn("Find PCDs", "No PCD found!!!");
            }
        }

        if (this.method.equals("LIBRARY_CLASS")) {
            lcv = Find.getAllLibraryClassVector();
            vLibraryClassFindResult = Find.getAllLibraryClassForFind(lcv);

            if (vLibraryClassFindResult.size() > 0) {
                for (int index = 0; index < vLibraryClassFindResult.size(); index++) {
                    Vector<String> v = new Vector<String>();
                    v.addElement(vLibraryClassFindResult.elementAt(index).getName());
                    v.addElement(vLibraryClassFindResult.elementAt(index).getType());

                    //
                    // Generate Produced Modules List
                    //
                    String strProducedModules = "";
                    Vector<ModuleIdentification> vModule = vLibraryClassFindResult.elementAt(index)
                                                                                  .getProducedModules();
                    for (int indexOfPM = 0; indexOfPM < vModule.size(); indexOfPM++) {
                        strProducedModules = strProducedModules + "<br>"
                                             + vModule.get(indexOfPM).getPackageId().getName() + "."
                                             + vModule.get(indexOfPM).getName();
                    }
                    if (strProducedModules.indexOf("<br>") == 0) {
                        strProducedModules = strProducedModules.substring("<br>".length());
                    }
                    int line1 = Tools.getSpecificStringCount(strProducedModules, "<br>");
                    v.addElement("<html>" + strProducedModules + "</html>");

                    //
                    // Generate Consumed Modules List
                    //
                    String strConsumedModules = "";
                    vModule = vLibraryClassFindResult.elementAt(index).getConsumedModules();
                    for (int indexOfCM = 0; indexOfCM < vModule.size(); indexOfCM++) {
                        strConsumedModules = strConsumedModules + "<br>"
                                             + vModule.get(indexOfCM).getPackageId().getName() + "."
                                             + vModule.get(indexOfCM).getName();
                    }
                    if (strConsumedModules.indexOf("<br>") == 0) {
                        strConsumedModules = strConsumedModules.substring("<br>".length());
                    }
                    int line2 = Tools.getSpecificStringCount(strConsumedModules, "<br>");
                    v.addElement("<html>" + strConsumedModules + "</html>");

                    v.addElement(vLibraryClassFindResult.elementAt(index).getDeclaredBy().getName());

                    model.addRow(v);
                    jTable.setRowHeight(index, (Math.max(line1, line2) > 1 ? Math.max(line1, line2) : 1) * 18);
                }
            } else {
                Log.wrn("Find Library Classes", "No Library Class found!!!");
            }
        }

        this.jTable.repaint();
        this.jTable.updateUI();
    }

    public void tableChanged(TableModelEvent arg0) {
        // TODO Auto-generated method stub

    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     *
     */
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == this.jButtonClose) {
            this.dispose();
        }
        
        if (arg0.getSource() == this.jButtonReload) {
            this.showTable();
        }
    }

    /* (non-Javadoc)
     * @see java.awt.event.ComponentListener#componentResized(java.awt.event.ComponentEvent)
     * 
     * Override componentResized to resize all components when frame's size is changed
     */
    public void componentResized(ComponentEvent arg0) {
        int intCurrentWidth = this.getJContentPane().getWidth();
        int intCurrentHeight = this.getJContentPane().getHeight();
        int intPreferredWidth = this.getJContentPane().getPreferredSize().width;
        int intPreferredHeight = this.getJContentPane().getPreferredSize().height;

        Tools.resizeComponent(this.jScrollPaneTable, intCurrentWidth, intCurrentHeight, intPreferredWidth,
                              intPreferredHeight);
        Tools.centerComponent(this.jPanelOperation, intCurrentWidth, intCurrentHeight, intPreferredHeight,
                              DataType.SPACE_TO_BOTTOM_FOR_CLOSE_BUTTON);
        Tools.resizeTableColumn(this.jTable, this.getSize().width - 28);
    }

    /* (non-Javadoc)
     * @see javax.swing.event.ListSelectionListener#valueChanged(javax.swing.event.ListSelectionEvent)
     *
     */
    public void valueChanged(ListSelectionEvent arg0) {
        if (arg0.getValueIsAdjusting()) {
            return;
        }
        ListSelectionModel lsm = (ListSelectionModel) arg0.getSource();
        if (lsm.isSelectionEmpty()) {
            return;
        } else {
            selectedRow = lsm.getMinSelectionIndex();
        }
    }

    /* (non-Javadoc)
     * @see java.awt.event.MouseListener#mouseClicked(java.awt.event.MouseEvent)
     *
     */
    public void mouseClicked(MouseEvent arg0) {
        if (arg0.getClickCount() == 2) {
            if (this.selectedRow < 0) {
                return;
            } else {
                if (this.method.equals("LIBRARY_CLASS")) {
                    FindResultDetailInfo frdi = new FindResultDetailInfo(vLibraryClassFindResult.elementAt(selectedRow));
                    frdi.setVisible(true);
                }
                if (this.method.equals("PCD")) {
                    FindResultDetailInfo frdi = new FindResultDetailInfo(vPcdFindResult.elementAt(selectedRow));
                    frdi.setVisible(true);
                }
            }
        }
    }

    /* (non-Javadoc)
     * @see java.awt.event.WindowListener#windowClosing(java.awt.event.WindowEvent)
     *
     * Override windowClosing to popup warning message to confirm quit
     * 
     */
    public void windowClosing(WindowEvent arg0) {
        this.dispose();
    }

    class MyTableCellRenderer extends DefaultTableCellRenderer {
        ///
        /// Define Class Serial Version UID
        ///
        private static final long serialVersionUID = -2082787479305255946L;

        public void setValue(Object value) {
            this.setVerticalAlignment(SwingConstants.TOP);
            super.setValue(value);
        }
    }

    public void mousePressed(MouseEvent e) {
        // TODO Auto-generated method stub

    }

    public void mouseReleased(MouseEvent e) {
        // TODO Auto-generated method stub

    }

    public void mouseEntered(MouseEvent e) {
        // TODO Auto-generated method stub

    }

    public void mouseExited(MouseEvent e) {
        // TODO Auto-generated method stub

    }

    /**
     * This method initializes jButtonReload	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonReload() {
        if (jButtonReload == null) {
            jButtonReload = new JButton();
            jButtonReload.setText("Reload");
            jButtonReload.addActionListener(this);
            jButtonReload.setBounds(new java.awt.Rectangle(0, 0, 80, 20));
            jButtonReload.setPreferredSize(new java.awt.Dimension(80, 20));
        }
        return jButtonReload;
    }

    /**
     * This method initializes jPanelOperation	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelOperation() {
        if (jPanelOperation == null) {
            jPanelOperation = new JPanel();
            jPanelOperation.setLayout(null);
            jPanelOperation.setBounds(new java.awt.Rectangle(300, 415, 170, 20));
            jPanelOperation.setPreferredSize(new java.awt.Dimension(170, 20));
            jPanelOperation.add(getJButtonClose(), null);
            jPanelOperation.add(getJButtonReload(), null);
        }
        return jPanelOperation;
    }
}
