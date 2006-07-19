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
import java.util.Vector;

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
    private Map<String, ArrayList<String>> fpdMsa = null;
    
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
            model.addColumn("ModuleVersion");
            model.addColumn("PackageName");
            model.addColumn("PackageVersion");
            model.addColumn("Path");
            
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
                    
                    String path = model.getValueAt(selectedRow, 4)+"";
                    ModuleIdentification mi = miList.get(selectedRow);
                    Vector<String> vArchs = null;
                    try {
                        vArchs = GlobalData.getModuleSupArchs(mi);
                    }
                    catch (Exception exp) {
                        JOptionPane.showMessageDialog(frame, exp.getMessage());
                    }
                    
                    if (vArchs == null) {
                        JOptionPane.showMessageDialog(frame, "No supported Archs specified in MSA file.");
                        return;
                    }
                    
                    String archsAdded = "";
                    String mg = mi.getGuid();
                    String mv = mi.getVersion();
                    String pg = mi.getPackage().getGuid();
                    String pv = mi.getPackage().getVersion();
                    
                    ArrayList<String> al = fpdMsa.get(mg + mv + pg + pv);
                    if (al == null) {
                        al = new ArrayList<String>();
                        fpdMsa.put(mg + mv + pg + pv, al);
                    }
                    for (int i = 0; i < al.size(); ++i) {
                        vArchs.remove(al.get(i));
                    }
                    //
                    // Archs this Module supported have already been added.
                    //
                    if (vArchs.size() == 0) {
                        JOptionPane.showMessageDialog(frame, "This Module Already Added.");
                        return;
                    }
                    //ToDo put Arch instead of null
                    for (int i = 0; i < vArchs.size(); ++i) {
                        String arch = vArchs.get(i);
                        al.add(arch);
                        archsAdded += arch + " ";
                        String[] row = {"", mv, "", pv, arch, path};
                        
                       if (mi != null) {
                           row[0] = mi.getName();
                           row[2] = mi.getPackage().getName();
                           
                       }
                       model1.addRow(row);
                       
                       docConsole.setSaved(false);
                       try{
                           //ToDo : specify archs need to add.
                           ffc.addFrameworkModulesPcdBuildDefs(mi, arch, null);
                       }
                       catch (Exception exception) {
                           JOptionPane.showMessageDialog(frame, "PCD Insertion Fail. " + exception.getMessage());
                       }
                    }
                    
                    
                    JOptionPane.showMessageDialog(frame, "This Module with Arch "+ archsAdded +" Added Successfully.");
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
            model1.addColumn("ModuleVersion");            
            model1.addColumn("PackageName");
            model1.addColumn("PackageVersion");
            model1.addColumn("SupportedArch");
            model1.addColumn("Path");
            
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
                    String[] sa = new String[5];
                    ffc.getFrameworkModuleInfo(selectedRow, sa);
                    String mg = sa[0];
                    String mv = sa[1];
                    String pg = sa[2];
                    String pv = sa[3];
                    String arch = sa[4];
                    settingDlg.setKey(mg + " " + mv + " " + pg + " " + pv + " " + arch, selectedRow);
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
                    String[] sa = new String[5];
                    ffc.getFrameworkModuleInfo(selectedRow, sa);
                    String mg = sa[0];
                    String mv = sa[1];
                    String pg = sa[2];
                    String pv = sa[3];
                    String arch = sa[4];
                    ModuleIdentification mi = getModuleId(sa[0] + " " + sa[1] + " " + sa[2] + " " + sa[3] + " " + sa[4]);
                    mv = mi.getVersion();
                    pv = mi.getPackage().getVersion();
                    model1.removeRow(selectedRow);
                    if (arch == null) {
                        // if no arch specified in ModuleSA
                        fpdMsa.remove(mg+mv+pg+pv);
                    }
                    else {
                        ArrayList<String> al = fpdMsa.get(mg+mv+pg+pv);
                        al.remove(arch);
                        if (al.size() == 0) {
                            fpdMsa.remove(mg+mv+pg+pv);
                        }
                    }
                    
                    
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
            JOptionPane.showMessageDialog(frame, "Error occurred when getting module data.");
        }
        
        if (ffc == null){
            ffc = new FpdFileContents(fpd);
            ffc.initDynPcdMap();
        }
        
        if (fpdMsa == null) {
            fpdMsa = new HashMap<String, ArrayList<String>>();
        }
        
        if (ffc.getFrameworkModulesCount() > 0) {
            String[][] saa = new String[ffc.getFrameworkModulesCount()][5];
            ffc.getFrameworkModulesInfo(saa);
            for (int i = 0; i < saa.length; ++i) {
                ModuleIdentification mi = getModuleId(saa[i][0]+ " "+saa[i][1]+" "+saa[i][2]+" "+saa[i][3]);
                String[] row = {"", "", "", "", "", ""};
                if (mi != null) {
                    row[0] = mi.getName();
                    row[1] = mi.getVersion();
                    row[2] = mi.getPackage().getName();
                    row[3] = mi.getPackage().getVersion();
                    row[4] = saa[i][4];
                    try{
                        row[5] = GlobalData.getMsaFile(mi).getPath().substring(System.getenv("WORKSPACE").length() + 1);
                    }
                    catch (Exception e) {
                        JOptionPane.showMessageDialog(frame, "ShowFPDModules:" + e.getMessage());
                    }
                }
                model1.addRow(row);
                ArrayList<String> al = fpdMsa.get(saa[i][0]+row[1]+saa[i][2]+row[3]);
                if (al == null) {
                    al = new ArrayList<String>();
                    fpdMsa.put(saa[i][0]+row[1]+saa[i][2]+row[3], al);
                }
                al.add(saa[i][4]);
                
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
            
            Set<ModuleIdentification> smi = GlobalData.getModules(pi);
            Iterator ismi = smi.iterator();
            while(ismi.hasNext()) {
                ModuleIdentification mi = (ModuleIdentification)ismi.next();
                s[0] = mi.getName();
                s[1] = mi.getVersion();
                s[2] = pi.getName();
                s[3] = pi.getVersion();
                try {
                    s[4] = GlobalData.getMsaFile(mi).getPath().substring(System.getenv("WORKSPACE").length() + 1);
                }
                catch (Exception e) {
                    JOptionPane.showMessageDialog(frame, "ShowAllModules:" + e.getMessage());
                }
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
        // Get ModuleGuid, ModuleVersion, PackageGuid, PackageVersion, Arch into string array.
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
