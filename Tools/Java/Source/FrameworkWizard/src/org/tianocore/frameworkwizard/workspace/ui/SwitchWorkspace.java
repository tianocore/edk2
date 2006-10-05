package org.tianocore.frameworkwizard.workspace.ui;

import java.awt.event.ActionEvent;

import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;

import org.tianocore.frameworkwizard.common.DataType;
import org.tianocore.frameworkwizard.common.Log;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.ui.IDialog;
import org.tianocore.frameworkwizard.common.ui.IFrame;
import org.tianocore.frameworkwizard.workspace.Workspace;

public class SwitchWorkspace extends IDialog {

    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = 2184556370155608202L;

    //
    // Define class members
    //
    private JPanel jContentPane = null;

    private JLabel jLabel = null;

    private JLabel jLabel1 = null;

    private JTextField jTextFieldOld = null;

    private JTextField jTextFieldNew = null;

    private JButton jButtonOk = null;

    private JButton jButtonCancel = null;

    private JButton jButtonBrowse = null;

    /**
     * This method initializes jTextFieldOld	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldOld() {
        if (jTextFieldOld == null) {
            jTextFieldOld = new JTextField();
            jTextFieldOld.setBounds(new java.awt.Rectangle(140, 10, 320, 20));
            jTextFieldOld.setEditable(false);
            jTextFieldOld.setText(Workspace.getCurrentWorkspace() == null ? "Not Defined"
                                                                         : Workspace.getCurrentWorkspace());
        }
        return jTextFieldOld;
    }

    /**
     * This method initializes jTextFieldNew	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldNew() {
        if (jTextFieldNew == null) {
            jTextFieldNew = new JTextField();
            jTextFieldNew.setBounds(new java.awt.Rectangle(140, 35, 220, 20));
        }
        return jTextFieldNew;
    }

    /**
     * This method initializes jButtonOk	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setBounds(new java.awt.Rectangle(290, 70, 80, 20));
            jButtonOk.setText("Ok");
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
     * This method initializes jButtonCancel	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setBounds(new java.awt.Rectangle(380, 70, 80, 20));
            jButtonCancel.setText("Cancel");
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     * This method initializes jButtonBrowse	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonBrowse() {
        if (jButtonBrowse == null) {
            jButtonBrowse = new JButton();
            jButtonBrowse.setBounds(new java.awt.Rectangle(370, 35, 90, 20));
            jButtonBrowse.setText("Browse");
            jButtonBrowse.addActionListener(this);
        }
        return jButtonBrowse;
    }

    /**
     * This is the default constructor
     * 
     */
    public SwitchWorkspace() {
        super();
        initialize();
    }

    /**
     * This is the override constructor
     * 
     */
    public SwitchWorkspace(IFrame parentFrame, boolean modal) {
        super(parentFrame, modal);
        initialize();
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
    private void initialize() {
        this.setSize(472, 132);
        this.setContentPane(getJContentPane());
        this.setTitle("Select workspace");
        this.centerWindow();
    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabel1 = new JLabel();
            jLabel1.setBounds(new java.awt.Rectangle(15, 10, 120, 20));
            jLabel1.setText("Current Workspace");
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(15, 35, 120, 20));
            jLabel.setText("Change to ");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(jLabel, null);
            jContentPane.add(jLabel1, null);
            jContentPane.add(getJTextFieldOld(), null);
            jContentPane.add(getJTextFieldNew(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonBrowse(), null);
        }
        return jContentPane;
    }

    private boolean check() {
        if (isEmpty(this.jTextFieldNew.getText())) {
            Log.wrn("Switch Workspace", "New workspace must be entered!");
            return false;
        }
        if (Workspace.checkWorkspace(this.jTextFieldNew.getText()) != Workspace.WORKSPACE_VALID) {
            Log.wrn("Switch Workspace", "Please select a valid workspace!");
            return false;
        }
        return true;
    }

    /* (non-Javadoc)
     * @see java.awt.event.ActionListener#actionPerformed(java.awt.event.ActionEvent)
     * 
     * Override actionPerformed to listen all actions
     */
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonCancel) {
            this.setVisible(false);
            returnType = DataType.RETURN_TYPE_CANCEL;
        }

        if (arg0.getSource() == jButtonOk) {
            if (!check()) {
                return;
            } else {
                Workspace.setCurrentWorkspace(this.jTextFieldNew.getText());
                returnType = DataType.RETURN_TYPE_OK;
                this.setVisible(false);
            }
        }

        if (arg0.getSource() == jButtonBrowse) {
            JFileChooser fc = new JFileChooser();
            fc.setAcceptAllFileFilterUsed(false);
            fc.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
            int result = fc.showOpenDialog(new JPanel());
            if (result == JFileChooser.APPROVE_OPTION) {
                this.jTextFieldNew.setText(Tools.convertPathToCurrentOsType(fc.getSelectedFile().getPath()));
            }
        }
    }
}
