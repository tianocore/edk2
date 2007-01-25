/**
 * 
 */
package org.tianocore.frameworkwizard.platform.ui;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Toolkit;

import javax.swing.JPanel;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Vector;

import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JComboBox;
import javax.swing.JTabbedPane;

import org.tianocore.frameworkwizard.common.Identifications.OpeningPlatformType;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.platform.ui.global.WorkspaceProfile;

/**
 * @author jlin16
 *
 */
public class GenAprioriFileDialog extends JDialog implements ActionListener {

    /**
     * 
     */
    private static final long serialVersionUID = 3627991301208644354L;
    private JPanel jContentPane = null;
    private JPanel jPanelN = null;
    private JLabel jLabelFvName = null;
    private JComboBox jComboBoxFvNames = null;
    private JTabbedPane jTabbedPane = null;
    private FpdFileContents ffc = null;
    private OpeningPlatformType docConsole = null;

    /**
     * This is the default constructor
     */
    public GenAprioriFileDialog(FpdFileContents inputFfc, OpeningPlatformType dc) {
        super();
        ffc = inputFfc;
        docConsole = dc;
        initialize();
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
    private void initialize() {
        this.setSize(670, 670);
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
        this.setTitle("Apriori Files");
        this.setContentPane(getJContentPane());
        this.setResizable(false);
        this.setModal(true);
        
        String fvName = jComboBoxFvNames.getSelectedItem()+"";
        if (fvName.length() > 0) {
            jTabbedPane.removeAll();
            AprioriModuleOrderPane peiPane = new AprioriModuleOrderPane(fvName, "", GenAprioriFileDialog.this, true);
            peiPane.showModulesInFv(fvName);
            peiPane.showAllModulesInPlatform();
            jTabbedPane.addTab("PEIMs", peiPane);
            AprioriModuleOrderPane dxePane = new AprioriModuleOrderPane(fvName, "", GenAprioriFileDialog.this, false);
            dxePane.showModulesInFv(fvName);
            dxePane.showAllModulesInPlatform();
            jTabbedPane.addTab("DXE Drivers", dxePane);
        }
        this.centerWindow();
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
            jContentPane.add(getJTabbedPane(), java.awt.BorderLayout.CENTER);
        }
        return jContentPane;
    }

    public void actionPerformed(ActionEvent arg0) {
        // TODO Auto-generated method stub
        if (arg0.getActionCommand().equals("ModuleOrderPaneOk")) {
            docConsole.setSaved(false);
            return;
        }
        if (arg0.getActionCommand().equals("ModuleOrderPaneCancel")) {
            this.dispose();
        }
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
 * This method initializes jPanelN	
 * 	
 * @return javax.swing.JPanel	
 */
private JPanel getJPanelN() {
    if (jPanelN == null) {
        jLabelFvName = new JLabel();
        jLabelFvName.setText("FV Name");
        jPanelN = new JPanel();
        jPanelN.add(jLabelFvName, null);
        jPanelN.add(getJComboBoxFvNames(), null);
    }
    return jPanelN;
}

/**
 * This method initializes jComboBoxFvNames	
 * 	
 * @return javax.swing.JComboBox	
 */
private JComboBox getJComboBoxFvNames() {
    if (jComboBoxFvNames == null) {
        jComboBoxFvNames = new JComboBox();
        jComboBoxFvNames.setPreferredSize(new java.awt.Dimension(200,20));
        Vector<String> vFvNames = new Vector<String>();
        ffc.getFvImagesFvImageFvImageNames(vFvNames);
        for (int i = 0; i < vFvNames.size(); ++i) {
            jComboBoxFvNames.addItem(vFvNames.get(i));
        }
//        if (jComboBoxFvNames.getItemCount() > 0) {
//            jComboBoxFvNames.setSelectedIndex(0);
//            
//        }
        jComboBoxFvNames.addItemListener(new java.awt.event.ItemListener() {
            public void itemStateChanged(java.awt.event.ItemEvent e) {
                String fvName = jComboBoxFvNames.getSelectedItem()+"";
                if (fvName.length() > 0) {
                    jTabbedPane.removeAll();
                    AprioriModuleOrderPane peiPane = new AprioriModuleOrderPane(fvName, "", GenAprioriFileDialog.this, true);
                    peiPane.showModulesInFv(fvName);
                    peiPane.showAllModulesInPlatform();
                    jTabbedPane.addTab("PEIMs", peiPane);
                    AprioriModuleOrderPane dxePane = new AprioriModuleOrderPane(fvName, "", GenAprioriFileDialog.this, false);
                    dxePane.showModulesInFv(fvName);
                    dxePane.showAllModulesInPlatform();
                    jTabbedPane.addTab("DXE Drivers", dxePane);
                }
                
            }
        });
    }
    return jComboBoxFvNames;
}

/**
 * This method initializes jTabbedPane	
 * 	
 * @return javax.swing.JTabbedPane	
 */
private JTabbedPane getJTabbedPane() {
    if (jTabbedPane == null) {
        jTabbedPane = new JTabbedPane();
    }
    return jTabbedPane;
}

private class AprioriModuleOrderPane extends ModuleOrderPane {
    /**
     * 
     */
    private static final long serialVersionUID = -7952853414833230546L;
    private boolean forPEI = false;
    private String fvName = null;

    AprioriModuleOrderPane (String fvName, String file, ActionListener action, boolean b) {
        
        super(fvName, file, ffc, action);
        this.fvName = fvName;
        forPEI = b;
        getJTableModInFv().getColumnModel().getColumn(0).setHeaderValue("Modules in Apriori File");
        getJButtonOk().setText("Save");
        getJButtonCancel().setText("Close");
    }
    
    public void showModulesInFv (String fvName) {
        String id = "1";
        if (forPEI) {
            id = "0";
        }
        int size = ffc.getUserExtsIncModCount(fvName, "APRIORI", id);
        
        if (size != -1) {
            String[][] saa = new String[size][5];
            ffc.getUserExtsIncMods(fvName, "APRIORI", id, saa);

            for (int i = 0; i < size; ++i) {
                String moduleKey = saa[i][0] + " " + saa[i][1] + " " + saa[i][2] + " " + saa[i][3];
                ModuleIdentification mi = WorkspaceProfile.getModuleId(moduleKey);
                String name = "N/A";
                if (mi != null) {
                    name = mi.getName();
                }
                
                String[] row = { name, saa[i][0] , saa[i][1], saa[i][2] , saa[i][3], saa[i][4] };
                getModInFvTableModel().addRow(row);
            }
        }
    }
    
    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getActionCommand().equals("ModuleOrderPaneOk")) {
            String id = "1";
            if (forPEI) {
                id = "0";
            }
            
            Vector<String[]> vModInFv = new Vector<String[]>();
            for (int i = 0; i < getJTableModInFv().getRowCount(); ++i) {
                String moduleName = getModInFvTableModel().getValueAt(i, 0)+"";
                if (moduleName.length() == 0 || moduleName.equals("N/A")) {
                    continue;
                }
                
                String mg = getModInFvTableModel().getValueAt(i, 1)+"";
                String mv = getModInFvTableModel().getValueAt(i, 2)+"";
                String pg = getModInFvTableModel().getValueAt(i, 3)+"";
                String pv = getModInFvTableModel().getValueAt(i, 4)+"";
                String arch = getModInFvTableModel().getValueAt(i, 5)+"";
                    
                String[] sa = { mg, mv, pg, pv, arch};
                vModInFv.add(sa);
                
            }
            
            ffc.removeBuildOptionsUserExtensions(fvName, "APRIORI", id);
            ffc.genBuildOptionsUserExtensions(fvName, "APRIORI", id, "", vModInFv);
            
        }
    }
}
}  //  @jve:decl-index=0:visual-constraint="10,10"
