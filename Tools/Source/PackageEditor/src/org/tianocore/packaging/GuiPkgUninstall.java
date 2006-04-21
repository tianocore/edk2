/** @file
  Java class GuiPkgUninstall is GUI for package installation.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.packaging;

import javax.swing.JPanel;
import javax.swing.JFrame;
import java.awt.FlowLayout;
//import java.awt.GridLayout;
import javax.swing.JLabel;
import javax.swing.JTextField;
import java.awt.Dimension;
import javax.swing.JButton;
import java.awt.ComponentOrientation;
import java.awt.Font;
import java.awt.Toolkit;
import java.io.File;

import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.SwingConstants;
import javax.swing.JList;
import javax.swing.JTextPane;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.table.DefaultTableModel;

/**
 GUI for package uninstallation.
 
 @since PackageEditor 1.0
**/
public class GuiPkgUninstall extends JFrame {

    final static long serialVersionUID = 0;
    
    static JFrame frame;

    private JPanel jPanel = null;

    private JLabel jLabel = null;

    private JTextField jTextField = null;

    private JButton jButton = null;

    private JLabel jLabel1 = null;

    private JPanel jPanel1 = null;

    private JButton jButton1 = null;

    private JButton jButton2 = null;

    private JScrollPane jScrollPane = null;

    private JTable jTable = null;

    private JButton jButton3 = null;
    
    private PkgRemoveTableModel model = null;
    
    private DbFileContents dfc = null;
    
    private JFrame pThis = null;

    
    public GuiPkgUninstall() {
        super();
        initialize();
    }

   
    private void initialize() {
        this.setSize(481, 404);
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
        this.setContentPane(getJPanel());
        this.setTitle("Package Uninstallation");
        this.centerWindow();
        pThis = this;
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
     initialize table contents from db file
     
     @param f FrameworkDatabase.db file under workspace
    **/
    protected void loadDB(File f) {
        if (!f.exists()) {
            JOptionPane.showMessageDialog(frame,
                                          "No FrameworkDatabase.db File!");
            return;
        }
        dfc = new DbFileContents(f);
        if (dfc.getPackageCount() == 0) {
            return;
        }
        //
        // Get package list info. and add them one by one into table
        //
        String[][] saa = new String[dfc.getPackageCount()][5];
        dfc.getPackageList(saa);
        int i = 0;
        while (i < saa.length) {
            model.addRow(saa[i]);
            i++;
        }
 
    }
    /**
     save package info. from table to db file
    **/
    protected void save() {
        dfc.removePackageList();
        int rowCount = jTable.getRowCount();
        int i = 0;
        while (i < rowCount) {
            
            dfc.genPackage(jTable.getValueAt(i, 0).toString(), jTable.getValueAt(i, 1).toString(),
                           jTable.getValueAt(i, 2).toString(), jTable.getValueAt(i, 3).toString(),
                           jTable.getValueAt(i, 4).toString());
            i++;
        }
        dfc.saveAs();
    }
 
    private JPanel getJPanel() {
        if (jPanel == null) {
            jLabel1 = new JLabel();
            jLabel1.setBounds(new java.awt.Rectangle(20, 83, 141, 16));
            jLabel1.setText("  Packages Installed");
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(17, 16, 171, 16));
            jLabel.setText(" Enter Workspace Location");
            jPanel = new JPanel();
            jPanel.setLayout(null);
            jPanel.add(jLabel, null);
            jPanel.add(getJTextField(), null);
            jPanel.add(getJButton(), null);
            jPanel.add(jLabel1, null);
            jPanel.add(getJPanel1(), null);
            jPanel.add(getJScrollPane(), null);
        }
        return jPanel;
    }

    /**
      This method initializes jTextField	
      	
      @return javax.swing.JTextField	
     **/
    private JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setBounds(new java.awt.Rectangle(16, 41, 350, 20));
            jTextField.setHorizontalAlignment(JTextField.LEFT);
            jTextField.setEditable(false);
            jTextField.setText(System.getenv("WORKSPACE"));
            jTextField.setPreferredSize(new Dimension(350, 20));
        }
        return jTextField;
    }

    /**
      This method initializes jButton	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButton() {
        if (jButton == null) {
            jButton = new JButton();
            jButton.setBounds(new java.awt.Rectangle(372,40,78,20));
            jButton.setFont(new Font("Dialog", Font.BOLD, 12));
            jButton.setPreferredSize(new Dimension(80, 20));
            jButton.setToolTipText("Where is the package?");
            jButton.setHorizontalAlignment(SwingConstants.LEFT);
            jButton.setHorizontalTextPosition(SwingConstants.CENTER);
            jButton.setText("Browse");
            jButton.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    //
                    // user can select another workspace directory
                    //
                    JFileChooser chooser = new JFileChooser();
                    chooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
                    chooser.setMultiSelectionEnabled(false);
                    int retval = chooser.showOpenDialog(frame);
                    if (retval == JFileChooser.APPROVE_OPTION) {
                        //
                        // update table when user selects a new workspace directory
                        //
                        jTextField.setText(chooser.getSelectedFile().getPath());
                        File f = new File(chooser.getSelectedFile(), FrameworkPkg.dbConfigFile);
                        loadDB(f);
                    }
                }
            });
        }
        return jButton;
    }

    /**
      This method initializes jPanel1	
      	
      @return javax.swing.JPanel	
     **/
    private JPanel getJPanel1() {
        if (jPanel1 == null) {
            FlowLayout flowLayout = new FlowLayout();
            flowLayout.setAlignment(java.awt.FlowLayout.LEFT);
            flowLayout.setHgap(20);
            jPanel1 = new JPanel();
            jPanel1.setLayout(flowLayout);
            jPanel1.setBounds(new java.awt.Rectangle(133,310,318,53));
            jPanel1.add(getJButton3(), null);
            jPanel1.add(getJButton1(), null);
            jPanel1.add(getJButton2(), null);
        }
        return jPanel1;
    }

    /**
      This method initializes jButton1	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButton1() {
        if (jButton1 == null) {
            jButton1 = new JButton();
            jButton1.setPreferredSize(new java.awt.Dimension(85, 20));
            jButton1.setText("Ok");
            jButton1.setHorizontalTextPosition(javax.swing.SwingConstants.LEFT);
            jButton1.setEnabled(true);
            jButton1.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    if (dfc != null) {
                        //
                        // save package info. to file before exit
                        //
                        save();
                    }
                    pThis.dispose();
                }
            });
        }
        return jButton1;
    }

    /**
      This method initializes jButton2	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButton2() {
        if (jButton2 == null) {
            jButton2 = new JButton();
            jButton2.setPreferredSize(new java.awt.Dimension(85, 20));
            jButton2.setText("Cancel");
            jButton2.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    pThis.dispose();
                }
            });
        }
        return jButton2;
    }

    /**
      This method initializes jScrollPane	
      	
      @return javax.swing.JScrollPane	
     **/
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setBounds(new java.awt.Rectangle(20,108,431,194));
            jScrollPane.setViewportView(getJTable());
        }
        return jScrollPane;
    }

    /**
      This method initializes jTable	
      	
      @return javax.swing.JTable	
     **/
    private JTable getJTable() {
        if (jTable == null) {
            model = new PkgRemoveTableModel();
            jTable = new JTable(model);
            jTable.setRowHeight(20);
            jTable.setSelectionMode(javax.swing.ListSelectionModel.SINGLE_SELECTION);
            jTable.setRowSelectionAllowed(true);
            model.addColumn("PackageName");
            model.addColumn("Version");
            model.addColumn("GUID");
            model.addColumn("Path");
            model.addColumn("InstallDate");
            File f = new File(jTextField.getText(), FrameworkPkg.dbConfigFile);
            loadDB(f);
        }
        return jTable;
    }

    /**
      This method initializes jButton3	
      	
      @return javax.swing.JButton	
    **/
    private JButton getJButton3() {
        if (jButton3 == null) {
            jButton3 = new JButton();
            jButton3.setText("Remove");
            jButton3.setPreferredSize(new java.awt.Dimension(85,20));
            jButton3.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    if (model != null){
                        int index = jTable.getSelectedRow();
                        if (index > -1) {
                            model.removeRow(index);
                        }
                    }
                }
            });
        }
        return jButton3;
    }

} //  @jve:decl-index=0:visual-constraint="10,10"

/**
 Derived table model which disables table edit
  
 @since PackageEditor 1.0
**/
class PkgRemoveTableModel extends DefaultTableModel {
    PkgRemoveTableModel() {
        super();
    }
    
    public boolean isCellEditable (int row, int col) {
        return false;
    }
}
