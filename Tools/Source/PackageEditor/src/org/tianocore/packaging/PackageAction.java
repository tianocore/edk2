/** @file
  Java class PackageAction is GUI for create spd file.
 
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

import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JFrame;
import java.awt.GridLayout;
import java.io.File;

import javax.swing.JButton;
import javax.swing.filechooser.FileFilter;

/**
 GUI for create spd file
  
 @since PackageEditor 1.0
**/
public class PackageAction extends JFrame {

    static JFrame frame;

    private JPanel jContentPane = null;

    private JButton jButton = null;

    private JButton jButton1 = null;

    private JButton jButton2 = null;

    private JButton jButton3 = null;

    private JButton jButton4 = null;

    private JButton jButton5 = null;

    private JButton jButton6 = null;

    private JButton jButton7 = null;

    ///
    /// SpdFileContents object passed from main
    ///
    private SpdFileContents sfc = null;

    private JFrame pThis = null;  //  @jve:decl-index=0:visual-constraint="304,10"

    private JButton jButton8 = null;

    private JButton jButton9 = null;  //  @jve:decl-index=0:visual-constraint="116,388"

    /**
      This is the default constructor
     **/
    public PackageAction(SpdFileContents sfc) {
        super();
        initialize();
        this.sfc = sfc;
    }

    /**
      This method initializes this
      
      @return void
     **/
    private void initialize() {
        this.setSize(305, 385);
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
        this.setContentPane(getJContentPane());
        this.setTitle("Please Choose an Action");
        this.centerWindow();
        this.pThis = this;
     
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
      This method initializes jContentPane
      
      @return javax.swing.JPanel
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            GridLayout gridLayout = new GridLayout();
            gridLayout.setRows(10);
            gridLayout.setColumns(1);
            jContentPane = new JPanel();
            jContentPane.setPreferredSize(new java.awt.Dimension(200,300));
            jContentPane.setLayout(gridLayout);
            jContentPane.add(getJButton8(), null);
            jContentPane.add(getJButton7(), null);
            jContentPane.add(getJButton6(), null);
            jContentPane.add(getJButton5(), null);
            jContentPane.add(getJButton4(), null);
            jContentPane.add(getJButton3(), null);
            jContentPane.add(getJButton2(), null);
            jContentPane.add(getJButton1(), null);
            jContentPane.add(getJButton(), null);
            jContentPane.add(getJButton9(), null);
        }
        return jContentPane;
    }

    /**
      This method initializes jButton	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButton() {
        final FileFilter filter = new PkgFileFilter("spd");
        
        if (jButton == null) {
            jButton = new JButton();
            jButton.setText("Save");
            jButton.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    //
                    // save sfc contents to file
                    //
                    JFileChooser chooser = new JFileChooser(PackagingMain.dirForNewSpd);
                    chooser.setMultiSelectionEnabled(false);
                    chooser.setFileFilter(filter);

                    int retval = chooser.showSaveDialog(frame);
                    if (retval == JFileChooser.APPROVE_OPTION) {
                        try {
                            File theFile = chooser.getSelectedFile();
                            if (theFile.exists()) {
                                int retVal = JOptionPane.showConfirmDialog(frame, "Are you sure to replace the exising one?", "File Exists",
                                                                           JOptionPane.YES_NO_OPTION);
                                if (retVal == JOptionPane.NO_OPTION) {
                                    return;
                                } 
                            }
                            sfc.saveAs(theFile);

                        } catch (Exception ee) {
                            System.out.println(ee.toString());
                        }
//                        pThis.dispose();
                    }

                }
            });
        }
        return jButton;
    }

    /**
      This method initializes jButton1	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButton1() {
        if (jButton1 == null) {
            jButton1 = new JButton();
            jButton1.setText("Add PCD Information");
            jButton1.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    //
                    // Add PCD frame show modal
                    //
                    ModalFrameUtil.showAsModal(new PackagePCD(sfc), pThis);
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
            jButton2.setText("Add PPI Declarations");
            jButton2.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    //
                    // Add PPI frame show modal
                    //
                    ModalFrameUtil.showAsModal(new PackagePpi(sfc), pThis);
                }
            });
        }
        return jButton2;
    }

    /**
      This method initializes jButton3	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButton3() {
        if (jButton3 == null) {
            jButton3 = new JButton();
            jButton3.setText("Add Protocol Declarations");
            jButton3.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    ModalFrameUtil.showAsModal(new PackageProtocols(sfc), pThis);
                }
            });
        }
        return jButton3;
    }

    /**
      This method initializes jButton4	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButton4() {
        if (jButton4 == null) {
            jButton4 = new JButton();
            jButton4.setText("Add GUID Declarations");
            jButton4.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    ModalFrameUtil.showAsModal(new PackageGuids(sfc), pThis);
                }
            });
        }
        return jButton4;
    }

    /**
      This method initializes jButton5	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButton5() {
        if (jButton5 == null) {
            jButton5 = new JButton();
            jButton5.setText("Add Package Headers");
            jButton5.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    ModalFrameUtil.showAsModal(new PackagePkgHeader(sfc), pThis);
                }
            });
        }
        return jButton5;
    }

    /**
      This method initializes jButton6	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButton6() {
        if (jButton6 == null) {
            jButton6 = new JButton();
            jButton6.setText("Add MSA Files");
            jButton6.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    ModalFrameUtil.showAsModal(new PackageMsaFile(sfc), pThis);
                }
            });
        }
        return jButton6;
    }

    /**
      This method initializes jButton7	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButton7() {
        if (jButton7 == null) {
            jButton7 = new JButton();
            jButton7.setText("Add Library Classes");
            jButton7.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    ModalFrameUtil.showAsModal(new PackageLibraryClass(sfc), pThis);
                }
            });
        }
        return jButton7;
    }

    /**
      This method initializes jButton8	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButton8() {
        if (jButton8 == null) {
            jButton8 = new JButton();
            jButton8.setText("Add SPD Header");
            jButton8.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    ModalFrameUtil.showAsModal(new PackageNew(sfc), pThis);
                }
            });
        }
        return jButton8;
    }

    /**
      This method initializes jButton9	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButton9() {
        if (jButton9 == null) {
            jButton9 = new JButton();
            jButton9.setText("Done");
            jButton9.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    //
                    // quit current frame
                    //
                    pThis.dispose();
                   
                }
            });
        }
        return jButton9;
    }

} //  @jve:decl-index=0:visual-constraint="104,41"
