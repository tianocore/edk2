/** @file
  Java class UpdateAction is GUI for update spd file.
 
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

/**
GUI for update spd file
 
@since PackageEditor 1.0
**/
public class UpdateAction extends JFrame {

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

    private SpdFileContents sfc = null;

    private JFrame pThis = null;  //  @jve:decl-index=0:visual-constraint="322,10"

    private JButton jButton8 = null;
    
    private JButton jButton9 = null;

    /**
     This is the default constructor
     **/
    public UpdateAction(SpdFileContents sfc) {
        super();
        initialize();
        this.sfc = sfc;
    }

    /**
     This method initializes this
     
     @return void
     **/
    private void initialize() {
        this.setSize(300, 333);
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
        this.setContentPane(getJContentPane());
        this.setTitle("Please Choose an Action");
        this.centerWindow();
        this.pThis = this;
        pThis.setSize(new java.awt.Dimension(316,399));
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
        if (jButton == null) {
            jButton = new JButton();
            jButton.setText("Save");
            jButton.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    JFileChooser chooser = new JFileChooser(sfc.getFile());
                    chooser.setMultiSelectionEnabled(false);

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
            jButton1.setText("Update PCD Information");
            jButton1.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    ModalFrameUtil.showAsModal(new UpdatePCD(sfc), pThis);
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
            jButton2.setText("Update PPI Declarations");
            jButton2.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    ModalFrameUtil.showAsModal(new UpdatePpi(sfc), pThis);
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
            jButton3.setText("Update Protocol Declarations");
            jButton3.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    ModalFrameUtil.showAsModal(new UpdateProtocols(sfc), pThis);
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
            jButton4.setText("Update GUID Declarations");
            jButton4.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    ModalFrameUtil.showAsModal(new UpdateGuids(sfc), pThis);
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
            jButton5.setText("Update Package Headers");
            jButton5.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    ModalFrameUtil.showAsModal(new UpdatePkgHeader(sfc), pThis);
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
            jButton6.setText("Update MSA Files");
            jButton6.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    ModalFrameUtil.showAsModal(new UpdateMsaFile(sfc), pThis);
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
            jButton7.setText("Update Library Classes");
            jButton7.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    ModalFrameUtil.showAsModal(new UpdateLibraryClass(sfc), pThis);
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
            jButton8.setText("Update SPD Header");
            jButton8.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    ModalFrameUtil.showAsModal(new UpdateNew(sfc), pThis);
                }
            });
        }
        return jButton8;
    }

    private JButton getJButton9() {
        if (jButton9 == null) {
            jButton9 = new JButton();
            jButton9.setText("Done");
            jButton9.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    
                        pThis.dispose();
               
                }
            });
        }
        return jButton9;
    }

} //  @jve:decl-index=0:visual-constraint="104,41"
