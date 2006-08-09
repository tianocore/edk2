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

    private JPanel jPanelTop = null;

    private JPanel jPanelBottom = null;

    private JLabel jLabel = null;

    private JScrollPane jScrollPaneAllModules = null;

    private JTable jTableAllModules = null;

    private JPanel jPanelTopSouth = null;

    private JButton jButtonAddModule = null;

    private JLabel jLabelModulesAdded = null;

    private JPanel jPanelBottomSouth = null;

    private JScrollPane jScrollPaneFpdModules = null;

    private JTable jTableFpdModules = null;

    private JButton jButtonSettings = null;

    private JButton jButtonRemoveModule = null;

    private NonEditableTableModel modelAllModules = null;

    private NonEditableTableModel modelFpdModules = null;

    private FpdModuleSA settingDlg = null;

    private FpdFileContents ffc = null;

    private OpeningPlatformType docConsole = null;

    private Map<String, ArrayList<String>> fpdMsa = null;

    private ArrayList<ModuleIdentification> miList = null;

    private final int ModNameCol = 0;

    private final int ModVerCol = 1;

    private final int PkgNameCol = 2;

    private final int PkgVerCol = 3;

    private final int ArchCol = 4;

    private final int Path4Col = 4;

    private final int Path5Col = 5;
    
    private final int ModNameMinWidth = 168;
    
    private final int ModNamePrefWidth = 200;

    private final int PkgNameMinWidth = 100;
    
    private final int PkgNamePrefWidth = 110;
    
    private final int PkgNameMaxWidth = 150;
    
    private final int VerMinWidth = 50;
    
    private final int VerMaxWidth = 80;
    
    private final int VerPrefWidth = 60;
    
    private final int PathPrefWidth = 320;
    
    private final int PathMinWidth = 280;
    
    private final int ArchPrefWidth = 80;
    
    private final int ArchMinWidth = 60;
    
    private final int ArchMaxWidth = 100;
    
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
            jSplitPane.setBottomComponent(getJPanelBottom());
            jSplitPane.setTopComponent(getJPanelTop());
        }
        return jSplitPane;
    }

    /**
     * This method initializes jPanel	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelTop() {
        if (jPanelTop == null) {
            jLabel = new JLabel();
            jLabel.setText("Modules in Workspace");
            jPanelTop = new JPanel();
            jPanelTop.setLayout(new BorderLayout());
            jPanelTop.add(jLabel, java.awt.BorderLayout.NORTH);
            jPanelTop.add(getJScrollPaneAllModules(), java.awt.BorderLayout.CENTER);
            jPanelTop.add(getJPanelTopSouth(), java.awt.BorderLayout.SOUTH);
        }
        return jPanelTop;
    }

    /**
     * This method initializes jPanel1	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelBottom() {
        if (jPanelBottom == null) {
            jLabelModulesAdded = new JLabel();
            jLabelModulesAdded.setText("Modules Added");
            jPanelBottom = new JPanel();
            jPanelBottom.setLayout(new BorderLayout());
            jPanelBottom.add(jLabelModulesAdded, java.awt.BorderLayout.NORTH);
            jPanelBottom.add(getJPanelBottomSouth(), java.awt.BorderLayout.SOUTH);
            jPanelBottom.add(getJScrollPaneFpdModules(), java.awt.BorderLayout.CENTER);
        }
        return jPanelBottom;
    }

    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneAllModules() {
        if (jScrollPaneAllModules == null) {
            jScrollPaneAllModules = new JScrollPane();
            jScrollPaneAllModules.setPreferredSize(new java.awt.Dimension(600, 200));
            jScrollPaneAllModules.setViewportView(getJTableAllModules());
        }
        return jScrollPaneAllModules;
    }

    /**
     * This method initializes jTable	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableAllModules() {
        if (jTableAllModules == null) {
            modelAllModules = new NonEditableTableModel();
            TableSorter sorter = new TableSorter(modelAllModules);
            jTableAllModules = new JTable(sorter);
            sorter.setTableHeader(jTableAllModules.getTableHeader());
            jTableAllModules.setRowHeight(20);
            modelAllModules.addColumn("<html>Module<br>Name</html>");
            modelAllModules.addColumn("<html>Module<br>Version</html>");
            modelAllModules.addColumn("<html>Package<br>Name</html>");
            modelAllModules.addColumn("<html>Package<br>Version</html>");
            modelAllModules.addColumn("Path");
            javax.swing.table.TableColumn column = null;
            column = jTableAllModules.getColumnModel().getColumn(ModNameCol);
            column.setPreferredWidth(ModNamePrefWidth);
            column.setMinWidth(ModNameMinWidth);
            column = jTableAllModules.getColumnModel().getColumn(ModVerCol);
            column.setPreferredWidth(VerPrefWidth);
            column.setMaxWidth(VerMaxWidth);
            column.setMinWidth(VerMinWidth);
            column = jTableAllModules.getColumnModel().getColumn(PkgNameCol);
            column.setPreferredWidth(PkgNamePrefWidth);
            column.setMinWidth(PkgNameMinWidth);
            column.setMaxWidth(PkgNameMaxWidth);
            column = jTableAllModules.getColumnModel().getColumn(PkgVerCol);
            column.setPreferredWidth(VerPrefWidth);
            column.setMaxWidth(VerMaxWidth);
            column.setMinWidth(VerMinWidth);
            column = jTableAllModules.getColumnModel().getColumn(Path4Col);
            column.setPreferredWidth(PathPrefWidth);
            column.setMinWidth(PathMinWidth);

            jTableAllModules.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        }
        return jTableAllModules;
    }

    /**
     * This method initializes jPanel2	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelTopSouth() {
        if (jPanelTopSouth == null) {
            FlowLayout flowLayout = new FlowLayout();
            flowLayout.setAlignment(java.awt.FlowLayout.RIGHT);
            jPanelTopSouth = new JPanel();
            jPanelTopSouth.setLayout(flowLayout);
            jPanelTopSouth.add(getJButtonAddModule(), null);
        }
        return jPanelTopSouth;
    }

    /**
     * This method initializes jButton	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonAddModule() {
        if (jButtonAddModule == null) {
            jButtonAddModule = new JButton();
            jButtonAddModule.setPreferredSize(new java.awt.Dimension(130, 20));
            jButtonAddModule.setText("Add a Module");
            jButtonAddModule.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    int selectedRow = jTableAllModules.getSelectedRow();
                    if (selectedRow < 0) {
                        return;
                    }

                    TableSorter sorter = (TableSorter) jTableAllModules.getModel();
                    selectedRow = sorter.modelIndex(selectedRow);
                    String path = modelAllModules.getValueAt(selectedRow, Path4Col) + "";
                    ModuleIdentification mi = miList.get(selectedRow);
                    Vector<String> vArchs = null;
                    try {
                        vArchs = GlobalData.getModuleSupArchs(mi);
                    }
                    catch (Exception exp) {
                        JOptionPane.showMessageDialog(frame, exp.getMessage());
                    }

                    if (vArchs == null) {
                        JOptionPane.showMessageDialog(frame, "No Supported Architectures specified in MSA file.");
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
                        JOptionPane.showMessageDialog(frame, "This Module has already been added.");
                        return;
                    }
                    //ToDo put Arch instead of null
                    boolean errorOccurred = false;
                    for (int i = 0; i < vArchs.size(); ++i) {
                        String arch = vArchs.get(i);
                        al.add(arch);
                        archsAdded += arch + " ";
                        String[] row = { "", mv, "", pv, arch, path };

                        if (mi != null) {
                            row[ModNameCol] = mi.getName();
                            row[PkgNameCol] = mi.getPackage().getName();

                        }
                        modelFpdModules.addRow(row);

                        docConsole.setSaved(false);
                        try {
                            //ToDo : specify archs need to add.
                            ffc.addFrameworkModulesPcdBuildDefs(mi, arch, null);
                        } catch (Exception exception) {
                            JOptionPane.showMessageDialog(frame, "Adding " + row[ModNameCol] + " with SupArch " + arch
                                                                 + ": " + exception.getMessage());
                            errorOccurred = true;
                        }
                    }

                    String s = "This Module with Architecture " + archsAdded;
                    if (errorOccurred) {
                        s += " was added with Error. Platform may NOT Build.";
                    } else {
                        s += " was added Successfully.";
                    }
                    JOptionPane.showMessageDialog(frame, s);
                    jTableFpdModules.changeSelection(modelFpdModules.getRowCount() - 1, 0, false, false);
                }
            });
        }
        return jButtonAddModule;
    }

    /**
     * This method initializes jPanel3	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelBottomSouth() {
        if (jPanelBottomSouth == null) {
            FlowLayout flowLayout1 = new FlowLayout();
            flowLayout1.setAlignment(java.awt.FlowLayout.RIGHT);
            jPanelBottomSouth = new JPanel();
            jPanelBottomSouth.setLayout(flowLayout1);
            jPanelBottomSouth.add(getJButtonSettings(), null);
            jPanelBottomSouth.add(getJButtonRemoveModule(), null);
        }
        return jPanelBottomSouth;
    }

    /**
     * This method initializes jScrollPane1	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneFpdModules() {
        if (jScrollPaneFpdModules == null) {
            jScrollPaneFpdModules = new JScrollPane();
            jScrollPaneFpdModules.setPreferredSize(new java.awt.Dimension(453, 200));
            jScrollPaneFpdModules.setViewportView(getJTableFpdModules());
        }
        return jScrollPaneFpdModules;
    }

    /**
     * This method initializes jTable1	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableFpdModules() {
        if (jTableFpdModules == null) {
            modelFpdModules = new NonEditableTableModel();
            TableSorter sorter = new TableSorter(modelFpdModules);
            jTableFpdModules = new JTable(sorter);
            sorter.setTableHeader(jTableFpdModules.getTableHeader());
            jTableFpdModules.setRowHeight(20);
            modelFpdModules.addColumn("<html>Module<br>Name</html>");
            modelFpdModules.addColumn("<html>Module<br>Version</html>");
            modelFpdModules.addColumn("<html>Package<br>Name</html>");
            modelFpdModules.addColumn("<html>Package<br>Version</html>");
            modelFpdModules.addColumn("<html>Supported<br>Architectures</html>");
            modelFpdModules.addColumn("Path");
            javax.swing.table.TableColumn column = null;
            column = jTableFpdModules.getColumnModel().getColumn(ModNameCol);
            column.setPreferredWidth(ModNamePrefWidth);
            column.setMinWidth(ModNameMinWidth);
            column = jTableFpdModules.getColumnModel().getColumn(ModVerCol);
            column.setPreferredWidth(VerPrefWidth);
            column.setMaxWidth(VerMaxWidth);
            column.setMinWidth(VerMinWidth);
            column = jTableFpdModules.getColumnModel().getColumn(PkgNameCol);
            column.setPreferredWidth(PkgNamePrefWidth);
            column.setMinWidth(PkgNameMinWidth);
            column.setMaxWidth(PkgNameMaxWidth);
            column = jTableFpdModules.getColumnModel().getColumn(PkgVerCol);
            column.setPreferredWidth(VerPrefWidth);
            column.setMaxWidth(VerMaxWidth);
            column.setMinWidth(VerMinWidth);
            column = jTableFpdModules.getColumnModel().getColumn(ArchCol);
            column.setPreferredWidth(ArchPrefWidth);
            column.setMaxWidth(ArchMaxWidth);
            column.setMinWidth(ArchMinWidth);
            column = jTableFpdModules.getColumnModel().getColumn(Path5Col);
            column.setPreferredWidth(PathPrefWidth);
            column.setMinWidth(PathMinWidth);

            jTableFpdModules.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        }
        return jTableFpdModules;
    }

    /**
     * This method initializes jButton1	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonSettings() {
        if (jButtonSettings == null) {
            jButtonSettings = new JButton();
            jButtonSettings.setPreferredSize(new java.awt.Dimension(130,20));
            jButtonSettings.setText("Settings");
            jButtonSettings.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    int selectedRow = jTableFpdModules.getSelectedRow();
                    if (selectedRow < 0) {
                        return;
                    }

                    TableSorter sorter = (TableSorter) jTableFpdModules.getModel();
                    selectedRow = sorter.modelIndex(selectedRow);
                    try {
                        if (ffc.adjustPcd(selectedRow)) {
                            docConsole.setSaved(false);
                        }
                    }
                    catch (Exception exp) {
                        JOptionPane.showMessageDialog(frame, exp.getMessage());
                        return;
                    }
                    
                    if (settingDlg == null) {
                        settingDlg = new FpdModuleSA(ffc);
                    }

                    String[] sa = new String[5];
                    ffc.getFrameworkModuleInfo(selectedRow, sa);
                    String mg = sa[ModNameCol];
                    String mv = sa[ModVerCol];
                    String pg = sa[PkgNameCol];
                    String pv = sa[PkgVerCol];
                    String arch = sa[ArchCol];
                    settingDlg.setKey(mg + " " + mv + " " + pg + " " + pv + " " + arch, selectedRow, docConsole);
                    settingDlg.setVisible(true);
                }
            });
        }
        return jButtonSettings;
    }

    /**
     * This method initializes jButton2	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonRemoveModule() {
        if (jButtonRemoveModule == null) {
            jButtonRemoveModule = new JButton();
            jButtonRemoveModule.setPreferredSize(new java.awt.Dimension(130, 20));
            jButtonRemoveModule.setText("Remove Module");
            jButtonRemoveModule.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    int selectedRow = jTableFpdModules.getSelectedRow();
                    if (selectedRow < 0) {
                        return;
                    }

                    TableSorter sorter = (TableSorter) jTableFpdModules.getModel();
                    selectedRow = sorter.modelIndex(selectedRow);

                    String[] sa = new String[5];
                    ffc.getFrameworkModuleInfo(selectedRow, sa);
                    String mg = sa[ModNameCol];
                    String mv = sa[ModVerCol];
                    String pg = sa[PkgNameCol];
                    String pv = sa[PkgVerCol];
                    String arch = sa[ArchCol];
                    ModuleIdentification mi = GlobalData.getModuleId(sa[ModNameCol] + " " + sa[ModVerCol] + " "
                                                                     + sa[PkgNameCol] + " " + sa[PkgVerCol] + " "
                                                                     + sa[ArchCol]);
                    mv = mi.getVersion();
                    pv = mi.getPackage().getVersion();
                    modelFpdModules.removeRow(selectedRow);
                    if (arch == null) {
                        // if no arch specified in ModuleSA
                        fpdMsa.remove(mg + mv + pg + pv);
                    } else {
                        ArrayList<String> al = fpdMsa.get(mg + mv + pg + pv);
                        al.remove(arch);
                        if (al.size() == 0) {
                            fpdMsa.remove(mg + mv + pg + pv);
                        }
                    }

                    docConsole.setSaved(false);
                    ffc.removeModuleSA(selectedRow);
                }
            });
        }
        return jButtonRemoveModule;
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

    public FpdFrameworkModules(PlatformSurfaceAreaDocument.PlatformSurfaceArea fpd) {
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

        if (ffc == null) {
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
                ModuleIdentification mi = GlobalData.getModuleId(saa[i][ModNameCol] + " " + saa[i][ModVerCol] + " "
                                                                 + saa[i][PkgNameCol] + " " + saa[i][PkgVerCol]);
                String[] row = { "", "", "", "", "", "" };
                if (mi != null) {
                    row[ModNameCol] = mi.getName();
                    row[ModVerCol] = mi.getVersion();
                    row[PkgNameCol] = mi.getPackage().getName();
                    row[PkgVerCol] = mi.getPackage().getVersion();
                    row[ArchCol] = saa[i][ArchCol];
                    try {
                        row[Path5Col] = GlobalData.getMsaFile(mi).getPath().substring(
                                                                                      System.getenv("WORKSPACE")
                                                                                            .length() + 1);
                    } catch (Exception e) {
                        JOptionPane.showMessageDialog(frame, "Show FPD Modules:" + e.getMessage());
                    }
                }
                modelFpdModules.addRow(row);
                ArrayList<String> al = fpdMsa.get(saa[i][ModNameCol] + row[ModVerCol] + saa[i][PkgNameCol]
                                                  + row[PkgVerCol]);
                if (al == null) {
                    al = new ArrayList<String>();
                    fpdMsa.put(saa[i][ModNameCol] + row[ModVerCol] + saa[i][PkgNameCol] + row[PkgVerCol], al);
                }
                al.add(saa[i][Path4Col]);

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

        while (ispi.hasNext()) {
            PackageIdentification pi = (PackageIdentification) ispi.next();
            String[] s = { "", "", "", "", "" };

            Set<ModuleIdentification> smi = GlobalData.getModules(pi);
            Iterator ismi = smi.iterator();
            while (ismi.hasNext()) {
                ModuleIdentification mi = (ModuleIdentification) ismi.next();
                s[ModNameCol] = mi.getName();
                s[ModVerCol] = mi.getVersion();
                s[PkgNameCol] = pi.getName();
                s[PkgVerCol] = pi.getVersion();
                try {
                    s[Path4Col] = GlobalData.getMsaFile(mi).getPath()
                                            .substring(System.getenv("WORKSPACE").length() + 1);
                } catch (Exception e) {
                    JOptionPane.showMessageDialog(frame, "Show All Modules:" + e.getMessage());
                }
                modelAllModules.addRow(s);
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

} //  @jve:decl-index=0:visual-constraint="10,10"

class NonEditableTableModel extends DefaultTableModel {
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    public boolean isCellEditable(int row, int col) {
        return false;
    }
}
