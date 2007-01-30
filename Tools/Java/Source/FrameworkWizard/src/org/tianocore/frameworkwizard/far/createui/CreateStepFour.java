/** @file

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.far.createui;

import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.io.File;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextArea;
import javax.swing.JTextField;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.IDefaultTableModel;
import org.tianocore.frameworkwizard.common.IFileFilter;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.far.AggregationOperation;
import org.tianocore.frameworkwizard.far.Far;
import org.tianocore.frameworkwizard.far.FarStringDefinition;
import org.tianocore.frameworkwizard.far.PackageQuery;
import org.tianocore.frameworkwizard.far.PackageQueryInterface;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.workspace.Workspace;

public class CreateStepFour extends IDialog implements MouseListener {

    /**
     * 
     */
    private static final long serialVersionUID = -7397213364965470902L;

    private JPanel jContentPane = null;

    private JTextArea jTextAreaInstruction = null;

    private JLabel jLabel = null;

    private JLabel jLabel2 = null;

    private JTextField jTextFieldSaveToFile = null;

    private JButton jButtonBrowser = null;

    //  private JScrollPane jScrollPane = null;
    private JButton jButtonCancel = null;

    private JButton jButtonFinish = null;

    private JButton jButtonPrevious = null;

    private IDefaultTableModel model = null;

    private CreateStepThree stepThree = null;

    //  private JTable jTable = null;
    public CreateStepFour(IDialog iDialog, boolean modal, CreateStepThree stepThree) {
        this(iDialog, modal);
        this.stepThree = stepThree;
    }

    /**
     * This method initializes jTextArea	
     * 	
     * @return javax.swing.JTextArea	
     */
    private JTextArea getJTextArea() {
        if (jTextAreaInstruction == null) {
            jTextAreaInstruction = new JTextArea();
            jTextAreaInstruction.setBounds(new java.awt.Rectangle(30, 7, 642, 50));
            jTextAreaInstruction.setText("Step 4: Choose a file \n");
            jTextAreaInstruction.setEditable(false);
        }
        return jTextAreaInstruction;
    }

    /**
     * This method initializes jTextField1	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField1() {
        if (jTextFieldSaveToFile == null) {
            jTextFieldSaveToFile = new JTextField();
            jTextFieldSaveToFile.setBounds(new java.awt.Rectangle(147,70,412,20));
        }
        return jTextFieldSaveToFile;
    }

    /**
     * This method initializes jButtonBrowser	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonBrower() {
        if (jButtonBrowser == null) {
            jButtonBrowser = new JButton();
            jButtonBrowser.setBounds(new java.awt.Rectangle(570, 70, 100, 20));
            jButtonBrowser.setText("Browser...");
            jButtonBrowser.addMouseListener(this);
        }
        return jButtonBrowser;
    }

    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    //  private JScrollPane getJScrollPane() {
    //    if (jScrollPane == null) {
    //      jScrollPane = new JScrollPane();
    //      jScrollPane.setBounds(new java.awt.Rectangle(139,85,500,100));
    //      jScrollPane.setViewportView(getJTable());
    //    }
    //    return jScrollPane;
    //  }
    /**
     * This method initializes jButtonCancel	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setBounds(new java.awt.Rectangle(570, 330, 90, 20));
            jButtonCancel.setText("Cancel");
            jButtonCancel.addMouseListener(this);
        }
        return jButtonCancel;
    }

    /**
     * This method initializes jButtonFinish	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonFinish() {
        if (jButtonFinish == null) {
            jButtonFinish = new JButton();
            jButtonFinish.setBounds(new java.awt.Rectangle(470, 330, 90, 20));
            jButtonFinish.setText("Finish");
            jButtonFinish.addMouseListener(this);
        }
        return jButtonFinish;
    }

    /**
     * This method initializes jButtonPrevious	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonPrevious() {
        if (jButtonPrevious == null) {
            jButtonPrevious = new JButton();
            jButtonPrevious.setBounds(new java.awt.Rectangle(370, 330, 90, 20));
            jButtonPrevious.setText("Previous");
            jButtonPrevious.addMouseListener(this);
            jButtonPrevious.setVisible(true);
        }
        return jButtonPrevious;
    }

    /**
     * This is the default constructor
     */
    public CreateStepFour(IDialog iDialog, boolean modal) {
        super(iDialog, modal);
        initialize();
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
    private void initialize() {
        this.setSize(700, 400);
        this.setContentPane(getJContentPane());
        this.setTitle(FarStringDefinition.CREATE_STEP_FOUR_TITLE);
        this.centerWindow();
    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabel2 = new JLabel();
            jLabel2.setBounds(new java.awt.Rectangle(30,70,111,18));
            jLabel2.setText("File to Save:  ");
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(29,108,320,20));
            jLabel.setText("This FAR will depend on the following packages: ");
            jLabel.setVisible(false);
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJTextArea(), null);
            jContentPane.add(jLabel, null);
            //      jContentPane.add(getJScrollPane(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonFinish(), null);
            jContentPane.add(getJButtonPrevious(), null);
            jContentPane.add(jLabel2, null);
            jContentPane.add(getJTextField1(), null);
            jContentPane.add(getJButtonBrower(), null);
        }
        return jContentPane;
    }

    public void mouseClicked(MouseEvent e) {
        if (e.getSource() == jButtonCancel) {
            this.setVisible(false);
        } else if (e.getSource() == jButtonFinish) {
            //
            // Add some logic process here
            // Guid Check, File Check etc.
            //
            if (this.jTextFieldSaveToFile.getText() == null) {
                Log.wrn("Create far", "Please input the Far name!");
            }
            try {
                //
                // Create an output stream for JAR
                //

                Far far = new Far(new File(this.jTextFieldSaveToFile.getText()));

                far.creatFar(this.getPreviousStep().getPreviousStep().getSelectedPackages(),
                             this.getPreviousStep().getPreviousStep().getSelectedPlatforms(), this.getPreviousStep()
                                                                                                  .getFileFilter(),
                             this.getPreviousStep().getPreviousStep().getPreviousStep().getFarHeader());
            } catch (Exception exp) {
                Log.wrn("Create far", exp.getMessage());
                Log.err("Create far", exp.getMessage());
                return;
            }
            getPreviousStep().getPreviousStep().getPreviousStep().returnType = DataType.RETURN_TYPE_OK;
            getPreviousStep().getPreviousStep().dispose();
            getPreviousStep().dispose();
            this.setVisible(false);
            this.dispose();
        } else if (e.getSource() == jButtonBrowser) {
            JFileChooser fc = new JFileChooser();
            fc.setAcceptAllFileFilterUsed(false);
            fc.addChoosableFileFilter(new IFileFilter(DataType.FAR_SURFACE_AREA_EXT));
            fc.setCurrentDirectory(new File(Workspace.getCurrentWorkspace()));

            int result = fc.showSaveDialog(new JPanel());
            if (result == JFileChooser.APPROVE_OPTION) {
                this.jTextFieldSaveToFile.setText(Tools.addPathExt(fc.getSelectedFile().getPath(),
                                                                   DataType.RETURN_TYPE_FAR_SURFACE_AREA));
            }
        } else if (e.getSource() == jButtonPrevious) {
            this.setVisible(false);
            stepThree.setVisible(true);
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

    public CreateStepThree getPreviousStep() {
        return stepThree;
    }

    public void prepareTable() {
        model.setRowCount(0);

        List<PackageIdentification> packageList = new ArrayList<PackageIdentification>();
        //
        // Change here to get packages and platforms from FAR
        //
        List<PackageIdentification> selectedPackages = getPreviousStep().getPreviousStep().getSelectedPackages();
        PackageQueryInterface pq = new PackageQuery();

        Iterator<PackageIdentification> iter = selectedPackages.iterator();
        while (iter.hasNext()) {
            PackageIdentification item = iter.next();
            List<PackageIdentification> list = pq.getPackageDependencies(item.getSpdFile());
            packageList = AggregationOperation.union(list, packageList);
        }

        packageList = AggregationOperation.minus(packageList, selectedPackages);

        iter = packageList.iterator();
        while (iter.hasNext()) {
            String[] str = new String[3];
            PackageIdentification item = iter.next();
            str[2] = item.getName();
            str[1] = item.getVersion();
            str[0] = item.getGuid();
            model.addRow(str);
        }
    }
}

