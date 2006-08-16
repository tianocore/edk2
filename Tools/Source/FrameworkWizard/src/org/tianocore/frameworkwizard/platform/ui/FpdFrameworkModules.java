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
     * Initialize Globals
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
    
    /**
     * Column settings for displaying all modules in workspace
     */
    private final int modNameColForAllModTable = 0;
    
    private final int pkgNameColForAllModTable = 1;
    
    private final int pathColForAllModTable = 2;
    
    private final int typeColForAllModTable = 3;
    
    private final int pkgVerColForAllModTable = 5;
    
    private final int modVerColForAllModTable = 4;
    
    /**
     * Column settings for display modules in the FPD file
     */
    private final int modNameColForFpdModTable = 0;
    
    private final int pkgNameColForFpdModTable = 1;
    
    private final int pathColForFpdModTable = 2;
    
    private final int archColForFpdModTable = 3;
    
    private final int pkgVerColForFpdModTable = 6;

    private final int modVerColForFpdModTable = 5;
    
    private final int typeColForFpdModTable = 4;
    
    /**
     * FpdFileContents structure
     */
    private final int ffcModGuid = 0;
    
    private final int ffcModVer = 1;
    
    private final int ffcPkgGuid = 2;
    
    private final int ffcPkgVer = 3;
    
    private final int ffcModArch = 4;
    
    /**
     * Set Column Widths, Only the PATH should not have a max width.
     */
    private final int modNameMinWidth = 168;
    
    private final int modNamePrefWidth = 200;
    
    private final int modNameMaxWidth = 350;

    private final int pkgNameMinWidth = 100;
    
    private final int pkgNamePrefWidth = 130;
    
    private final int pkgNameMaxWidth = 150;
    
    private final int verMinWidth = 60;
    
    private final int verMaxWidth = 80;
    
    private final int verPrefWidth = 70;
    
    private final int pathPrefWidth = 600;
    
    private final int pathMinWidth = 280;
    
    private final int archPrefWidth = 80;
    
    private final int archMinWidth = 60;
    
    private final int archMaxWidth = 100;
    
    private final int typePrefWidth = 145;
    
    private final int typeMinWidth = 100;
    
    private final int typeMaxWidth = 155;
    
    /**
     * This method initializes jSplitPane
     * 
     * This is the main edit window
     * 	
     * @return javax.swing.JSplitPane jSplitPane	
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
     * This method initializes jPanelTop
     * 
     * This panel contains the All Modules Table	
     * 	
     * @return javax.swing.JPanel	jPanelTop
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
     * This method initializes jPanelBottom
     * 	
     * This panel contains the FPD Modules Table
     * 
     * @return javax.swing.JPanel	jPanelBottom
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
     * This method initializes jScrollPaneAllModules	
     * 	
     * @return javax.swing.JScrollPane	jScrollPaneAllModules
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
     * This method initializes jTableAllModules
     * 	
     * @return javax.swing.JTable	jTableAllModules
     */
    private JTable getJTableAllModules() {
        if (jTableAllModules == null) {
            modelAllModules = new NonEditableTableModel();
            TableSorter sorter = new TableSorter(modelAllModules);
            jTableAllModules = new JTable(sorter);
            sorter.setTableHeader(jTableAllModules.getTableHeader());
            jTableAllModules.setRowHeight(20);
            modelAllModules.addColumn("<html>Module<br>Name</html>");
            modelAllModules.addColumn("<html>Package<br>Name</html>");
            modelAllModules.addColumn("Path");
            modelAllModules.addColumn("<html>Module<br>Type</html>");
            modelAllModules.addColumn("<html>Module<br>Version</html>");
            modelAllModules.addColumn("<html>Package<br>Version</html>");
            
            javax.swing.table.TableColumn column = null;
            column = jTableAllModules.getColumnModel().getColumn(modNameColForAllModTable);
            column.setPreferredWidth(modNamePrefWidth);
            column.setMinWidth(modNameMinWidth);
            column.setMaxWidth(modNameMaxWidth);
            column = jTableAllModules.getColumnModel().getColumn(modVerColForAllModTable);
            column.setPreferredWidth(verPrefWidth);
            column.setMaxWidth(verMaxWidth);
            column.setMinWidth(verMinWidth);
            column = jTableAllModules.getColumnModel().getColumn(pkgNameColForAllModTable);
            column.setPreferredWidth(pkgNamePrefWidth);
            column.setMinWidth(pkgNameMinWidth);
            column.setMaxWidth(pkgNameMaxWidth);
            column = jTableAllModules.getColumnModel().getColumn(pkgVerColForAllModTable);
            column.setPreferredWidth(verPrefWidth);
            column.setMaxWidth(verMaxWidth);
            column.setMinWidth(verMinWidth);
            column = jTableAllModules.getColumnModel().getColumn(typeColForAllModTable);
            column.setPreferredWidth(typePrefWidth);
            column.setMaxWidth(typeMaxWidth);
            column.setMinWidth(typeMinWidth);
            column = jTableAllModules.getColumnModel().getColumn(pathColForAllModTable);
            column.setPreferredWidth(pathPrefWidth);
            column.setMinWidth(pathMinWidth);

            jTableAllModules.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTableAllModules.setAutoResizeMode(JTable.AUTO_RESIZE_OFF);

        }
        return jTableAllModules;
    }

    /**
     * This method initializes jPanelTopSouth
     * 
     * This panel contains the ADD button
     * 
     * @return javax.swing.JPanel   jPanelTopSouth
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
     * This method initializes jButtonAddModule	
     * 	
     * @return javax.swing.JButton	jButtonAddModule
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
                    String path = modelAllModules.getValueAt(selectedRow, pathColForAllModTable) + "";
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
                    String mType = mi.getModuleType();

                    ArrayList<String> al = fpdMsa.get(mg + mv + pg + pv);
                    if (al == null) {
                        //
                        // if existing ModuleSA does not specify version info.
                        //
                        al = fpdMsa.get(mg + "null" + pg + "null");
                        if (al == null) {
                            al = fpdMsa.get(mg + "null" + pg + pv);
                            if (al == null){
                                al = fpdMsa.get(mg + mv + pg + "null");
                                if (al == null) {
                                    al = new ArrayList<String>();
                                    fpdMsa.put(mg + mv + pg + pv, al);    
                                }
                            }
                        }
                    }
                    //
                    // filter from module SupArchs what archs has been added.
                    //
                    for (int i = 0; i < al.size(); ++i) {
                        vArchs.remove(al.get(i));
                    }
                    //
                    // check whether archs conform to SupArch of platform.
                    //
                    Vector<Object> platformSupArch = new Vector<Object>();
                    ffc.getPlatformDefsSupportedArchs(platformSupArch);
                    vArchs.retainAll(platformSupArch);
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
                        String[] row = { "", "", "", "", "", "", "" };

                        if (mi != null) {
                            row[modNameColForFpdModTable] = mi.getName();
                            row[pkgNameColForFpdModTable] = mi.getPackage().getName();
                            row[pathColForFpdModTable] = path;
                            row[archColForFpdModTable] = arch;
                            row[pkgVerColForFpdModTable] = pv;
                            row[modVerColForFpdModTable] = mv;
                            row[typeColForFpdModTable] = mType;

                        }
                        modelFpdModules.addRow(row);

                        docConsole.setSaved(false);
                        try {
                            //ToDo : specify archs need to add.
                            ffc.addFrameworkModulesPcdBuildDefs(mi, arch, null);
                        } catch (Exception exception) {
                            JOptionPane.showMessageDialog(frame, "Adding " + row[modNameColForFpdModTable] + " with Supporting Architectures: " + arch
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
     * This method initializes jPanelBottomSouth
     * 
     * This panel contains the Settings and Remove Buttons
     * 	
     * @return javax.swing.JPanel	jPanelBottomSouth
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
     * This method initializes jScrollPaneFpdModules
     * 	
     * @return javax.swing.JScrollPane	jScrollPaneFpdModules
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
     * This method initializes jTableFpdModules
     * 	
     * @return javax.swing.JTable	jTableFpdModules
     */
    private JTable getJTableFpdModules() {
        if (jTableFpdModules == null) {
            modelFpdModules = new NonEditableTableModel();
            TableSorter sorter = new TableSorter(modelFpdModules);
            jTableFpdModules = new JTable(sorter);
            sorter.setTableHeader(jTableFpdModules.getTableHeader());
            jTableFpdModules.setRowHeight(20);
            modelFpdModules.addColumn("<html>Module<br>Name</html>");
            modelFpdModules.addColumn("<html>Package<br>Name</html>");
            modelFpdModules.addColumn("Path");
            modelFpdModules.addColumn("<html>Supported<br>Architectures</html>");
            modelFpdModules.addColumn("<html>Module<br>Type</html>");
            modelFpdModules.addColumn("<html>Module<br>Version</html>");
            modelFpdModules.addColumn("<html>Package<br>Version</html>");

            
            javax.swing.table.TableColumn column = null;
            column = jTableFpdModules.getColumnModel().getColumn(modNameColForFpdModTable);
            column.setPreferredWidth(modNamePrefWidth);
            column.setMinWidth(modNameMinWidth);
            column.setMaxWidth(modNameMaxWidth);
            column = jTableFpdModules.getColumnModel().getColumn(modVerColForFpdModTable);
            column.setPreferredWidth(verPrefWidth);
            column.setMaxWidth(verMaxWidth);
            column.setMinWidth(verMinWidth);
            column = jTableFpdModules.getColumnModel().getColumn(pkgNameColForFpdModTable);
            column.setPreferredWidth(pkgNamePrefWidth);
            column.setMinWidth(pkgNameMinWidth);
            column.setMaxWidth(pkgNameMaxWidth);
            column = jTableFpdModules.getColumnModel().getColumn(pkgVerColForFpdModTable);
            column.setPreferredWidth(verPrefWidth);
            column.setMaxWidth(verMaxWidth);
            column.setMinWidth(verMinWidth);
            column = jTableFpdModules.getColumnModel().getColumn(archColForFpdModTable);
            column.setPreferredWidth(archPrefWidth);
            column.setMaxWidth(archMaxWidth);
            column.setMinWidth(archMinWidth);
            column = jTableFpdModules.getColumnModel().getColumn(pathColForFpdModTable);
            column.setPreferredWidth(pathPrefWidth);
            column.setMinWidth(pathMinWidth);
            column = jTableFpdModules.getColumnModel().getColumn(typeColForFpdModTable);
            column.setPreferredWidth(typePrefWidth);
            column.setMaxWidth(typeMaxWidth);
            column.setMinWidth(typeMinWidth);

            jTableFpdModules.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTableFpdModules.setAutoResizeMode(JTable.AUTO_RESIZE_OFF);

        }
        return jTableFpdModules;
    }

    /**
     * This method initializes jButtonSettings
     * 	
     * @return javax.swing.JButton	jButtonSettings
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
                    String mg = sa[ffcModGuid];
                    String mv = sa[ffcModVer];
                    String pg = sa[ffcPkgGuid];
                    String pv = sa[ffcPkgVer];
                    String arch = sa[ffcModArch];
                    settingDlg.setKey(mg + " " + mv + " " + pg + " " + pv + " " + arch, selectedRow, docConsole);
                    settingDlg.setVisible(true);
                }
            });
        }
        return jButtonSettings;
    }

    /**
     * This method initializes jButtonRemoveModule
     * 	
     * @return javax.swing.JButton	jButtonRemoveModule
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
                    String mg = sa[ffcModGuid];
                    String mv = sa[ffcModVer];
                    String pg = sa[ffcPkgGuid];
                    String pv = sa[ffcPkgVer];
                    String arch = sa[ffcModArch];
                    ModuleIdentification mi = GlobalData.getModuleId(mg + " " + mv + " " + pg + " " + pv + " " + arch);
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
     * 
     * @param args
     */
    public static void main(String[] args) {
        // Set the pane visable
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
                ModuleIdentification mi = GlobalData.getModuleId(saa[i][ffcModGuid] + " " + saa[i][ffcModVer] + " "
                                                                 + saa[i][ffcPkgGuid] + " " + saa[i][ffcPkgVer]);
                String[] row = { "", "", "", "", "", "", "" };
                if (mi != null) {
                    row[modNameColForFpdModTable] = mi.getName();
                    row[modVerColForFpdModTable] = mi.getVersion();
                    row[typeColForFpdModTable] = mi.getModuleType();
                    row[pkgNameColForFpdModTable] = mi.getPackage().getName();
                    row[pkgVerColForFpdModTable] = mi.getPackage().getVersion();
                    row[archColForFpdModTable] = saa[i][ffcModArch];
                    try {
                        row[pathColForFpdModTable] = GlobalData.getMsaFile(mi).getPath().substring(
                                                                                      System.getenv("WORKSPACE")
                                                                                            .length() + 1);
                    } catch (Exception e) {
                        JOptionPane.showMessageDialog(frame, "Show FPD Modules:" + e.getMessage());
                    }
                    
                    modelFpdModules.addRow(row);
                    String fpdMsaKey = saa[i][ffcModGuid] + row[modVerColForFpdModTable]
                                                                + saa[i][ffcPkgGuid] + row[pkgVerColForFpdModTable];
                    ArrayList<String> al = fpdMsa.get(fpdMsaKey);
                    if (al == null) {
                        al = new ArrayList<String>();
                        fpdMsa.put(fpdMsaKey, al);
                    }
                    al.add(saa[i][ffcModArch]);
                }
                

            }
            TableSorter sorter = (TableSorter)jTableFpdModules.getModel();
            sorter.setSortingStatus(modNameColForFpdModTable, TableSorter.ASCENDING);
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
            String[] s = { "", "", "", "", "", "" };

            Set<ModuleIdentification> smi = GlobalData.getModules(pi);
            Iterator ismi = smi.iterator();
            while (ismi.hasNext()) {
                ModuleIdentification mi = (ModuleIdentification) ismi.next();
                s[modNameColForAllModTable] = mi.getName();
                s[modVerColForAllModTable] = mi.getVersion();
                s[typeColForAllModTable] = mi.getModuleType();
                s[pkgNameColForAllModTable] = pi.getName();
                s[pkgVerColForAllModTable] = pi.getVersion();
                try {
                    s[pathColForAllModTable] = GlobalData.getMsaFile(mi).getPath()
                                            .substring(System.getenv("WORKSPACE").length() + 1);
                } catch (Exception e) {
                    JOptionPane.showMessageDialog(frame, "Show All Modules:" + e.getMessage());
                }
                modelAllModules.addRow(s);
                miList.add(mi);
            }
        }
        
        TableSorter sorter = (TableSorter)jTableAllModules.getModel();
        sorter.setSortingStatus(modNameColForAllModTable, TableSorter.ASCENDING);
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
