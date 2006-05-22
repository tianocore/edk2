/** @file
  Java class PackageLibraryClass is GUI for create library definition elements of spd file.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.packaging;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.util.Vector;

import javax.swing.DefaultListModel;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JTextField;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JScrollPane;
import javax.swing.JButton;
import javax.swing.JFrame;

/**
 GUI for create library definition elements of spd file.
  
 @since PackageEditor 1.0
**/
public class PackageLibraryClass extends JFrame implements ActionListener {
    static JFrame frame;
    
    private static String Separator = "::";

    private DefaultListModel listItem = new DefaultListModel();

    private SpdFileContents sfc = null;

    private JPanel jContentPane = null;

    private JRadioButton jRadioButtonAdd = null;

    private JRadioButton jRadioButtonSelect = null;

    private JTextField jTextFieldAdd = null;

    private JComboBox jComboBoxSelect = null;

    private JLabel jLabelUsage = null;

    private JComboBox jComboBoxUsage = null;

    private JScrollPane jScrollPane = null;

    private JList jListLibraryClassDefinitions = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonClearAll = null;

    private JButton jButtonCancel = null;

    private JButton jButtonOk = null;

    private JLabel jLabel = null;

    private JTextField jTextField = null;

    private JButton jButtonBrowse = null;

    /**
      This method initializes this
     
     **/
    private void initialize() {
        this.setTitle("Library Declarations");
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);

    }

    /**
      This method initializes jRadioButtonAdd	
      	
      @return javax.swing.JRadioButton	
     **/
    private JRadioButton getJRadioButtonAdd() {
        if (jRadioButtonAdd == null) {
            jRadioButtonAdd = new JRadioButton();
            jRadioButtonAdd.setBounds(new java.awt.Rectangle(10, 35, 205, 20));
            jRadioButtonAdd.setText("Add a new Library Class");
            jRadioButtonAdd.addActionListener(this);
            jRadioButtonAdd.setSelected(false);
        }
        return jRadioButtonAdd;
    }

    /**
      This method initializes jRadioButtonSelect	
      	
      @return javax.swing.JRadioButton	
     **/
    private JRadioButton getJRadioButtonSelect() {
        if (jRadioButtonSelect == null) {
            jRadioButtonSelect = new JRadioButton();
            jRadioButtonSelect.setBounds(new java.awt.Rectangle(10, 10, 205, 20));
            jRadioButtonSelect.setText("Select Existing Library Class");
            jRadioButtonSelect.addActionListener(this);
            jRadioButtonSelect.setSelected(true);
        }
        return jRadioButtonSelect;
    }

    /**
      This method initializes jTextFieldAdd	
      	
      @return javax.swing.JTextField	
     **/
    private JTextField getJTextFieldAdd() {
        if (jTextFieldAdd == null) {
            jTextFieldAdd = new JTextField();
            jTextFieldAdd.setBounds(new java.awt.Rectangle(220, 35, 260, 20));
            jTextFieldAdd.setEnabled(false);
        }
        return jTextFieldAdd;
    }

    /**
      This method initializes jComboBoxSelect	
      	
      @return javax.swing.JComboBox	
     **/
    private JComboBox getJComboBoxSelect() {
        if (jComboBoxSelect == null) {
            jComboBoxSelect = new JComboBox();
            jComboBoxSelect.setBounds(new java.awt.Rectangle(220, 10, 260, 20));
            jComboBoxSelect.setEnabled(true);
        }
        return jComboBoxSelect;
    }

    /**
      This method initializes jComboBoxUsage	
      	
      @return javax.swing.JComboBox	
     **/
    private JComboBox getJComboBoxUsage() {
        if (jComboBoxUsage == null) {
            jComboBoxUsage = new JComboBox();
            jComboBoxUsage.setBounds(new java.awt.Rectangle(220, 60, 260, 20));
            jComboBoxUsage.setEnabled(false);
        }
        return jComboBoxUsage;
    }

    /**
      This method initializes jScrollPane	
      	
      @return javax.swing.JScrollPane	
     **/
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setBounds(new java.awt.Rectangle(10,149,350,146));
            jScrollPane.setViewportView(getJListLibraryClassDefinitions());
        }
        return jScrollPane;
    }

    /**
      This method initializes jListLibraryClassDefinitions	
      	
      @return javax.swing.JList	
     **/
    private JList getJListLibraryClassDefinitions() {
        if (jListLibraryClassDefinitions == null) {
            jListLibraryClassDefinitions = new JList(listItem);
        }
        return jListLibraryClassDefinitions;
    }

    /**
      This method initializes jButtonAdd	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonAdd() {
        if (jButtonAdd == null) {
            jButtonAdd = new JButton();
            jButtonAdd.setBounds(new java.awt.Rectangle(375,152,90,20));
            jButtonAdd.setText("Add");
            jButtonAdd.addActionListener(this);
        }
        return jButtonAdd;
    }

    /**
      This method initializes jButtonRemove	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonRemove() {
        if (jButtonRemove == null) {
            jButtonRemove = new JButton();
            jButtonRemove.setBounds(new java.awt.Rectangle(375, 230, 90, 20));
            jButtonRemove.setText("Remove");
            jButtonRemove.addActionListener(this);
        }
        return jButtonRemove;
    }

    /**
      This method initializes jButtonRemoveAll	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonClearAll() {
        if (jButtonClearAll == null) {
            jButtonClearAll = new JButton();
            jButtonClearAll.setBounds(new java.awt.Rectangle(375, 260, 90, 20));
            jButtonClearAll.setText("Clear All");
            jButtonClearAll.addActionListener(this);
        }
        return jButtonClearAll;
    }

    /**
      This method initializes jButtonCancel	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setPreferredSize(new java.awt.Dimension(90, 20));
            jButtonCancel.setLocation(new java.awt.Point(390, 305));
            jButtonCancel.setText("Cancel");
            jButtonCancel.setSize(new java.awt.Dimension(90, 20));
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
      This method initializes jButton	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setSize(new java.awt.Dimension(90, 20));
            jButtonOk.setText("OK");
            jButtonOk.setLocation(new java.awt.Point(290, 305));
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
      This is the default constructor
     **/
    public PackageLibraryClass(SpdFileContents sfc) {
        super();
        initialize();
        init();
        this.sfc = sfc;
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
      This method initializes this
      
      @return void
     **/
    private void init() {
        this.setContentPane(getJContentPane());
        this.setTitle("Library Class Declarations");
        this.setBounds(new java.awt.Rectangle(0, 0, 500, 370));
        this.centerWindow();
        initFrame();
    }

    /**
      This method initializes jContentPane
      
      @return javax.swing.JPanel
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(14, 85, 201, 22));
            jLabel.setText("Include Header for Selected Class");
            jLabelUsage = new JLabel();
            jLabelUsage.setBounds(new java.awt.Rectangle(15, 60, 200, 20));
            jLabelUsage.setEnabled(false);
            jLabelUsage.setText("Usage");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJRadioButtonAdd(), null);
            jContentPane.add(getJRadioButtonSelect(), null);
            jContentPane.add(getJTextFieldAdd(), null);
            jContentPane.add(getJComboBoxSelect(), null);
            jContentPane.add(jLabelUsage, null);
            jContentPane.add(getJComboBoxUsage(), null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonClearAll(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(jLabel, null);
            jContentPane.add(getJTextField(), null);
            jContentPane.add(getJButtonBrowse(), null);
        }
        return jContentPane;
    }

    /**
     fill ComboBoxes with pre-defined contents
    **/
    private void initFrame() {
        jComboBoxSelect.addItem("BaseCpuICacheFlush");
        jComboBoxSelect.addItem("BaseDebugLibNull");
        jComboBoxSelect.addItem("BaseDebugLibReportStatusCode");
        jComboBoxSelect.addItem("BaseIoLibIntrinsic");
        jComboBoxSelect.addItem("BaseLib");
        jComboBoxSelect.addItem("BaseMemoryLib");
        jComboBoxSelect.addItem("BaseMemoryLibMmx");
        jComboBoxSelect.addItem("BaseMemoryLibSse2");
        jComboBoxSelect.addItem("BasePeCoffGetEntryPointLib");
        jComboBoxSelect.addItem("BasePeCoffLib");
        jComboBoxSelect.addItem("BasePrintLib");
        jComboBoxSelect.addItem("BaseReportStatusCodeLibNull");
        jComboBoxSelect.addItem("CommonPciCf8Lib");
        jComboBoxSelect.addItem("CommonPciExpressLib");
        jComboBoxSelect.addItem("CommonPciLibCf8");
        jComboBoxSelect.addItem("CommonPciLibPciExpress");
        jComboBoxSelect.addItem("DxeCoreEntryPoint");
        jComboBoxSelect.addItem("DxeHobLib");
        jComboBoxSelect.addItem("DxeIoLibCpuIo");
        jComboBoxSelect.addItem("DxeLib");
        jComboBoxSelect.addItem("DxePcdLib");
        jComboBoxSelect.addItem("DxeReportStatusCodeLib");
        jComboBoxSelect.addItem("DxeServicesTableLib");
        jComboBoxSelect.addItem("PeiCoreEntryPoint");
        jComboBoxSelect.addItem("PeiMemoryLib");
        jComboBoxSelect.addItem("PeimEntryPoint");
        jComboBoxSelect.addItem("PeiReportStatusCodeLib");
        jComboBoxSelect.addItem("PeiServicesTablePointerLib");
        jComboBoxSelect.addItem("PeiServicesTablePointerLibMm7");
        jComboBoxSelect.addItem("UefiDebugLibConOut");
        jComboBoxSelect.addItem("UefiDebugLibStdErr");
        jComboBoxSelect.addItem("UefiDriverEntryPointMultiple");
        jComboBoxSelect.addItem("UefiDriverEntryPointSingle");
        jComboBoxSelect.addItem("UefiDriverEntryPointSingleUnload");
        jComboBoxSelect.addItem("UefiDriverModelLib");
        jComboBoxSelect.addItem("UefiDriverModelLibNoConfigNoDiag");
        jComboBoxSelect.addItem("UefiLib");
        jComboBoxSelect.addItem("UefiMemoryLib");

        jComboBoxUsage.addItem("ALWAYS_CONSUMED");
        jComboBoxUsage.addItem("SOMETIMES_CONSUMED");
        jComboBoxUsage.addItem("ALWAYS_PRODUCED");
        jComboBoxUsage.addItem("SOMETIMES_PRODUCED");
        jComboBoxUsage.addItem("DEFAULT");
        jComboBoxUsage.addItem("PRIVATE");
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     */
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonOk) {
            this.save();
            this.dispose();

        }
        if (arg0.getSource() == jButtonCancel) {
            this.dispose();
        }

        if (arg0.getSource() == jButtonAdd) {
            String strLibClass = "";
            if (jRadioButtonAdd.isSelected()) {
                strLibClass = jTextFieldAdd.getText();
            }
            if (jRadioButtonSelect.isSelected()) {
                strLibClass = jComboBoxSelect.getSelectedItem().toString();
            }
            listItem.addElement(jTextField.getText().replace('\\', '/') + this.Separator + strLibClass);
        }
        //
        // remove selected line
        //
        if (arg0.getSource() == jButtonRemove) {
            int intSelected[] = jListLibraryClassDefinitions.getSelectedIndices();
            if (intSelected.length > 0) {
                for (int index = intSelected.length - 1; index > -1; index--) {
                    listItem.removeElementAt(intSelected[index]);
                }
            }
            jListLibraryClassDefinitions.getSelectionModel().clearSelection();
        }

        if (arg0.getSource() == jButtonClearAll) {
            listItem.removeAllElements();
        }

        if (arg0.getSource() == jRadioButtonAdd) {
            if (jRadioButtonAdd.isSelected()) {
                jRadioButtonSelect.setSelected(false);
                jTextFieldAdd.setEnabled(true);
                jComboBoxSelect.setEnabled(false);
            }
            if (!jRadioButtonSelect.isSelected() && !jRadioButtonAdd.isSelected()) {
                jRadioButtonAdd.setSelected(true);
                jTextFieldAdd.setEnabled(true);
                jComboBoxSelect.setEnabled(false);
            }
        }

        if (arg0.getSource() == jRadioButtonSelect) {
            if (jRadioButtonSelect.isSelected()) {
                jRadioButtonAdd.setSelected(false);
                jTextFieldAdd.setEnabled(false);
                jComboBoxSelect.setEnabled(true);
            }
            if (!jRadioButtonSelect.isSelected() && !jRadioButtonAdd.isSelected()) {
                jRadioButtonSelect.setSelected(true);
                jTextFieldAdd.setEnabled(false);
                jComboBoxSelect.setEnabled(true);
            }
        }
    }

    /**
     Add contents in list to sfc
    **/
    protected void save() {
        try {
            int intLibraryCount = listItem.getSize();

            if (intLibraryCount > 0) {

                for (int index = 0; index < intLibraryCount; index++) {
                    String strAll = listItem.get(index).toString();
                    String strInclude = strAll.substring(0, strAll.indexOf(Separator));
                    String strLibraryClass = strAll.substring(strAll.indexOf(Separator) + Separator.length());
                    sfc.genSpdLibClassDeclarations(strLibraryClass, null, strInclude, null, null, null, null, null,
                                                   null, null);
                }
            } else {

            }

        } catch (Exception e) {
            System.out.println(e.toString());
        }
    }

    /**
      This method initializes jTextField	
      	
      @return javax.swing.JTextField	
     **/
    private JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setBounds(new java.awt.Rectangle(12,112,346,21));
        }
        return jTextField;
    }

    /**
      This method initializes jButtonBrowse	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonBrowse() {
        if (jButtonBrowse == null) {
            jButtonBrowse = new JButton();
            jButtonBrowse.setBounds(new java.awt.Rectangle(374,111,92,21));
            jButtonBrowse.setText("Browse");
            jButtonBrowse.setPreferredSize(new java.awt.Dimension(34,20));
            jButtonBrowse.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    //
                    // Select files from current workspace
                    //
                    JFileChooser chooser = new JFileChooser(System.getenv("WORKSPACE"));
                    File theFile = null;
                    String headerDest = null;
                    
                    chooser.setMultiSelectionEnabled(false);
                    chooser.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
                    int retval = chooser.showOpenDialog(frame);
                    if (retval == JFileChooser.APPROVE_OPTION) {

                        theFile = chooser.getSelectedFile();
                        String file = theFile.getPath();
                        if (!file.startsWith(System.getenv("WORKSPACE"))) {
                            JOptionPane.showMessageDialog(frame, "You can only select files in current workspace!");
                            return;
                        }
                        
                        
                    }
                    else {
                        return;
                    }
                    
                    if (!theFile.getPath().startsWith(PackagingMain.dirForNewSpd)) {
                        //
                        //ToDo: copy elsewhere header file to new pkg dir, prompt user to chooser a location
                        //
                        JOptionPane.showMessageDialog(frame, "You must copy header file into current package directory!");
                        return;
                    }
                    
                    headerDest = theFile.getPath();
                    int fileIndex = headerDest.indexOf(System.getProperty("file.separator"), PackagingMain.dirForNewSpd.length());
                    
                    jTextField.setText(headerDest.substring(fileIndex + 1).replace('\\', '/'));
                        
                }
            });
        }
        return jButtonBrowse;
    }

}
