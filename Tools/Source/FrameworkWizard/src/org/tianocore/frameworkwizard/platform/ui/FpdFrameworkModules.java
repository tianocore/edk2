package org.tianocore.frameworkwizard.platform.ui;

import java.awt.BorderLayout;

import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JSplitPane;
import javax.swing.JLabel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JButton;
import javax.swing.ListSelectionModel;
import javax.swing.table.DefaultTableModel;

import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPlatformType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import org.tianocore.frameworkwizard.platform.ui.global.GlobalData;
import org.tianocore.frameworkwizard.platform.ui.id.ModuleIdentification;
import org.tianocore.frameworkwizard.platform.ui.id.PackageIdentification;

import java.awt.FlowLayout;
import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

public class FpdFrameworkModules extends IInternalFrame {

    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    static JFrame frame;
    private JSplitPane jSplitPane = null;
    private JPanel jPanel = null;
    private JPanel jPanel1 = null;
    private JLabel jLabel = null;
    private JScrollPane jScrollPane = null;
    private JTable jTable = null;
    private JPanel jPanel2 = null;
    private JButton jButton = null;
    private JLabel jLabel1 = null;
    private JPanel jPanel3 = null;
    private JScrollPane jScrollPane1 = null;
    private JTable jTable1 = null;
    private JButton jButton1 = null;
    private JButton jButton2 = null;
    private NonEditableTableModel model = null;
    private NonEditableTableModel model1 = null;
    
    private FpdModuleSA settingDlg = null;
    
    private FpdFileContents ffc = null;
    private OpeningPlatformType docConsole = null;
    private Map<String, String> fpdMsa = null;
    
    private ArrayList<ModuleIdentification> miList = null;

    /**
     * This method initializes jSplitPane	
     * 	
     * @return javax.swing.JSplitPane	
     */
    private JSplitPane getJSplitPane() {
        if (jSplitPane == null) {
            jSplitPane = new JSplitPane();
            jSplitPane.setOrientation(javax.swing.JSplitPane.VERTICAL_SPLIT);
            jSplitPane.setDividerLocation(250);
            jSplitPane.setBottomComponent(getJPanel1());
            jSplitPane.setTopComponent(getJPanel());
        }
        return jSplitPane;
    }

    /**
     * This method initializes jPanel	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel() {
        if (jPanel == null) {
            jLabel = new JLabel();
            jLabel.setText("Modules in Workspace");
            jPanel = new JPanel();
            jPanel.setLayout(new BorderLayout());
            jPanel.add(jLabel, java.awt.BorderLayout.NORTH);
            jPanel.add(getJScrollPane(), java.awt.BorderLayout.CENTER);
            jPanel.add(getJPanel2(), java.awt.BorderLayout.SOUTH);
        }
        return jPanel;
    }

    /**
     * This method initializes jPanel1	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel1() {
        if (jPanel1 == null) {
            jLabel1 = new JLabel();
            jLabel1.setText("Modules Added");
            jPanel1 = new JPanel();
            jPanel1.setLayout(new BorderLayout());
            jPanel1.add(jLabel1, java.awt.BorderLayout.NORTH);
            jPanel1.add(getJPanel3(), java.awt.BorderLayout.SOUTH);
            jPanel1.add(getJScrollPane1(), java.awt.BorderLayout.CENTER);
        }
        return jPanel1;
    }

    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setPreferredSize(new java.awt.Dimension(600,200));
            jScrollPane.setViewportView(getJTable());
        }
        return jScrollPane;
    }

    /**
     * This method initializes jTable	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable() {
        if (jTable == null) {
            model = new NonEditableTableModel();
            jTable = new JTable(model);
            jTable.setRowHeight(20);
            model.addColumn("ModuleName");
            model.addColumn("ModuleGUID");
            model.addColumn("ModuleVersion");
            model.addColumn("PackageGUID");
            model.addColumn("PackageVersion");
            
            jTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        }
        return jTable;
    }

    /**
     * This method initializes jPanel2	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel2() {
        if (jPanel2 == null) {
            FlowLayout flowLayout = new FlowLayout();
            flowLayout.setAlignment(java.awt.FlowLayout.RIGHT);
            jPanel2 = new JPanel();
            jPanel2.setLayout(flowLayout);
            jPanel2.add(getJButton(), null);
        }
        return jPanel2;
    }

    /**
     * This method initializes jButton	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton() {
        if (jButton == null) {
            jButton = new JButton();
            jButton.setPreferredSize(new java.awt.Dimension(130,20));
            jButton.setText("Add a Module");
            jButton.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    int selectedRow = jTable.getSelectedRow();
                    if (selectedRow < 0){
                        return;
                    }
                    
                    String mg = (String)model.getValueAt(selectedRow, 1);
                    String mv = (String)model.getValueAt(selectedRow, 2);
                    String pg = (String)model.getValueAt(selectedRow, 3);
                    String pv = (String)model.getValueAt(selectedRow, 4);
                    if (fpdMsa.containsKey(mg + mv + pg + pv)) {
                        JOptionPane.showMessageDialog(frame, "This Module Already Added.");
                        return;
                    }
                    //ToDo put Arch instead of null
                    fpdMsa.put(mg + mv + pg + pv, null);
                    
                    String[] row = {" ", mg, mv, pg, pv};
                    ModuleIdentification mi = getModuleId(mg + " " + mv + " " + pg + " " + pv);
                    if (mi != null) {
                        row[0] = mi.getName();
                        row[2] = mi.getVersion();
                        row[4] = mi.getPackage().getVersion();
                    }
                    model1.addRow(row);
                    
                    docConsole.setSaved(false);
                    try{
                        ffc.addFrameworkModulesPcdBuildDefs(miList.get(selectedRow), null);
                    }
                    catch (Exception exception) {
                        JOptionPane.showMessageDialog(frame, "PCD Insertion Fail. " + exception.getMessage());
                    }
                    JOptionPane.showMessageDialog(frame, "This Module Added Successfully.");
                    jTable1.changeSelection(model1.getRowCount()-1, 0, false, false);
                }
            });
        }
        return jButton;
    }

    /**
     * This method initializes jPanel3	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel3() {
        if (jPanel3 == null) {
            FlowLayout flowLayout1 = new FlowLayout();
            flowLayout1.setAlignment(java.awt.FlowLayout.RIGHT);
            jPanel3 = new JPanel();
            jPanel3.setLayout(flowLayout1);
            jPanel3.add(getJButton1(), null);
            jPanel3.add(getJButton2(), null);
        }
        return jPanel3;
    }

    /**
     * This method initializes jScrollPane1	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane1() {
        if (jScrollPane1 == null) {
            jScrollPane1 = new JScrollPane();
            jScrollPane1.setPreferredSize(new java.awt.Dimension(453,200));
            jScrollPane1.setViewportView(getJTable1());
        }
        return jScrollPane1;
    }

    /**
     * This method initializes jTable1	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable1() {
        if (jTable1 == null) {
            model1 = new NonEditableTableModel();
            jTable1 = new JTable(model1);
            jTable1.setRowHeight(20);
            model1.addColumn("ModuleName");
            model1.addColumn("ModuleGUID");
            model1.addColumn("ModuleVersion");            
            model1.addColumn("PackageGUID");
            model1.addColumn("PackageVersion");
//            model1.addColumn("SupportedArch");
            
            jTable1.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        }
        return jTable1;
    }

    /**
     * This method initializes jButton1	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton1() {
        if (jButton1 == null) {
            jButton1 = new JButton();
            jButton1.setPreferredSize(new java.awt.Dimension(130,20));
            jButton1.setText("Settings");
            jButton1.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    int selectedRow = jTable1.getSelectedRow();
                    if (selectedRow < 0){
                        return;
                    }
                    if (settingDlg == null) {
                        settingDlg = new FpdModuleSA(ffc);
                    }
                    docConsole.setSaved(false);
                    String mg = model1.getValueAt(selectedRow, 1)+"";
                    String mv = model1.getValueAt(selectedRow, 2)+"";
                    String pg = model1.getValueAt(selectedRow, 3)+"";
                    String pv = model1.getValueAt(selectedRow, 4)+"";
                    settingDlg.setKey(mg + " " + mv + " " + pg + " " + pv);
                    settingDlg.setVisible(true);
                }
            });
        }
        return jButton1;
    }

    /**
     * This method initializes jButton2	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton2() {
        if (jButton2 == null) {
            jButton2 = new JButton();
            jButton2.setPreferredSize(new java.awt.Dimension(130,20));
            jButton2.setText("Remove Module");
            jButton2.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    int selectedRow = jTable1.getSelectedRow();
                    if (selectedRow < 0){
                        return;
                    }
                    String mg = model1.getValueAt(selectedRow, 1).toString();
                    String mv = model1.getValueAt(selectedRow, 2).toString();
                    String pg = model1.getValueAt(selectedRow, 3).toString();
                    String pv = model1.getValueAt(selectedRow, 4).toString();
                    model1.removeRow(selectedRow);
                    fpdMsa.remove(mg+mv+pg+pv);
                    docConsole.setSaved(false);
                    ffc.removeModuleSA(selectedRow);
                }
            });
        }
        return jButton2;
    }

    /**
     * @param args
     */
    public static void main(String[] args) {
        // TODO Auto-generated method stub
        new FpdFrameworkModules().setVisible(true);
    }

    /**
     * This is the default constructor
     */
    public FpdFrameworkModules() {
        super();
        initialize();
    }

    public FpdFrameworkModules(PlatformSurfaceAreaDocument.PlatformSurfaceArea fpd){
        this();
        init(fpd);
        
    }
    
    public FpdFrameworkModules(OpeningPlatformType opt) {
        this(opt.getXmlFpd());
        docConsole = opt;
    }
    
    private void init(PlatformSurfaceAreaDocument.PlatformSurfaceArea fpd) {
        try {
            GlobalData.initInfo("Tools" + File.separator + "Conf" + File.separator + "FrameworkDatabase.db", System.getenv("WORKSPACE"));
        }
        catch(Exception e){
            JOptionPane.showMessageDialog(frame, "FrameworkDatabase Corrupted.");
        }
        
        if (ffc == null){
            ffc = new FpdFileContents(fpd);
            ffc.initDynPcdMap();
        }
        
        if (fpdMsa == null) {
            fpdMsa = new HashMap<String, String>();
        }
        
        if (ffc.getFrameworkModulesCount() > 0) {
            String[][] saa = new String[ffc.getFrameworkModulesCount()][5];
            ffc.getFrameworkModulesInfo(saa);
            for (int i = 0; i < saa.length; ++i) {
                ModuleIdentification mi = getModuleId(saa[i][1]+ " "+saa[i][2]+" "+saa[i][3]+" "+saa[i][4]);
                if (mi != null) {
                    saa[i][0] = mi.getName();
                    saa[i][2] = mi.getVersion();
                    saa[i][4] = mi.getPackage().getVersion();
                }
                model1.addRow(saa[i]);
                fpdMsa.put(saa[i][1]+saa[i][2]+saa[i][3]+saa[i][4], saa[i][0]);
            }
        }
        
        showAllModules();
        
    }
    
    private void showAllModules() {
        
        if (miList == null) {
            miList = new ArrayList<ModuleIdentification>();
        }
        Set<PackageIdentification> spi = GlobalData.getPackageList();
        Iterator ispi = spi.iterator();
        
        while(ispi.hasNext()) {
            PackageIdentification pi = (PackageIdentification)ispi.next();
            String[] s = {"", "", "", "", ""};
            s[3] = pi.getGuid();
            s[4] = pi.getVersion();
            Set<ModuleIdentification> smi = GlobalData.getModules(pi);
            Iterator ismi = smi.iterator();
            while(ismi.hasNext()) {
                ModuleIdentification mi = (ModuleIdentification)ismi.next();
                s[0] = mi.getName();
                s[1] = mi.getGuid();
                s[2] = mi.getVersion();
                model.addRow(s);
                miList.add(mi);
            }
        }
    }
    /**
     * This method initializes this
     * 
     * @return void
     */
    private void initialize() {
        this.setSize(633, 533);
        this.setTitle("Framework Modules");
        this.setContentPane(getJSplitPane());
        this.setVisible(true);
        
    }
    
    private ModuleIdentification getModuleId(String key){
        //
        // Get ModuleGuid, ModuleVersion, PackageGuid, PackageVersion into string array.
        //
        String[] keyPart = key.split(" ");
        Set<PackageIdentification> spi = GlobalData.getPackageList();
        Iterator ispi = spi.iterator();
        
        while(ispi.hasNext()) {
            PackageIdentification pi = (PackageIdentification)ispi.next();
            if ( !pi.getGuid().equals(keyPart[2])){ 

                continue;
            }
            if (keyPart[3] != null && keyPart[3].length() > 0 && !keyPart[3].equals("null")){
                if(!pi.getVersion().equals(keyPart[3])){
                continue;
            }
            }
            Set<ModuleIdentification> smi = GlobalData.getModules(pi);
            Iterator ismi = smi.iterator();
            while(ismi.hasNext()) {
                ModuleIdentification mi = (ModuleIdentification)ismi.next();
                if (mi.getGuid().equals(keyPart[0])){
                    if (keyPart[1] != null && keyPart[1].length() > 0 && !keyPart[1].equals("null")){
                        if(!mi.getVersion().equals(keyPart[1])){
                            continue;
                        }
                    }

                    return mi;
                }
            }
        }
        return null;
    }

}  //  @jve:decl-index=0:visual-constraint="10,10"

class NonEditableTableModel extends DefaultTableModel {
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    public boolean isCellEditable(int row, int col) {
        return false;
    }
}
