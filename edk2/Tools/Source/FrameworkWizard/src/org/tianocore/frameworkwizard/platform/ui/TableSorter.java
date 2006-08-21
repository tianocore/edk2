/** @file
 
 The file is used to sort FrameworkModules of Fpd file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.platform.ui;

import java.awt.event.*;
import java.util.*;

import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;
import javax.swing.table.*;


public class TableSorter extends AbstractTableModel {
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    protected DefaultTableModel tableModel;
    private TableRow[] rowInView;
    private int[] viewPos;

    public static final String DESCENDING = "down";
    public static final String NOT_SORTED = "none";
    public static final String ASCENDING = "up";

    private JTableHeader tableHeader;
    private MouseListener mouseListener = new MouseHandler();
    private TableModelListener tableModelListener = new TableModelHandler();
    
    private HashMap<Integer, String> sortingOrders = new HashMap<Integer, String>();

    public TableSorter(DefaultTableModel tableModel) {
        setTableModel(tableModel);
    }


    private void resetSortState() {
        rowInView = null;
        viewPos = null;
    }

    public DefaultTableModel getTableModel() {
        return tableModel;
    }

    public void setTableModel(DefaultTableModel dtm) {
        if (tableModel != null) {
            tableModel.removeTableModelListener(tableModelListener);
        }

        tableModel = dtm;
        if (tableModel != null) {
            tableModel.addTableModelListener(tableModelListener);
        }

        resetSortState();
        fireTableStructureChanged();
    }

    public JTableHeader getTableHeader() {
        return tableHeader;
    }

    public void setTableHeader(JTableHeader th) {
        if (tableHeader != null) {
            tableHeader.removeMouseListener(mouseListener);
        }
        tableHeader = th;
        if (tableHeader != null) {
            tableHeader.addMouseListener(mouseListener);
            
        }
    }

    private String getSortState(int column) {
 
        Integer i = new Integer(column);
        if (sortingOrders.get(i) != null) {
            return sortingOrders.get(i);
        }
        return NOT_SORTED;
    }

    private void sortStateChanged() {
        resetSortState();
        fireTableDataChanged();

    }

    public void setSortState(int column, String status) {
        Integer i = new Integer(column);
        sortingOrders.put(i, status);
        sortStateChanged();
    }

    private TableRow[] getSortedViewRows() {
        if (rowInView == null) {
            int rowCount = tableModel.getRowCount();
            rowInView = new TableRow[rowCount];
            int i = 0;
            while ( i < rowCount ) {
                rowInView[i] = new TableRow(i);
                ++i;
            }

            if (sortingOrders.size() != 0) {
                Arrays.sort(rowInView);
            }
        }
        return rowInView;
    }

    public int getModelRowIndex(int viewIndex) {
        TableRow[] rArray = getSortedViewRows();
        return rArray[viewIndex].modelIndex;
    }

    public int[] getViewIndexArray() {
        if (viewPos == null) {
            int n = getSortedViewRows().length;
            viewPos = new int[n];
            for (int i = 0; i < n; i++) {
                viewPos[getModelRowIndex(i)] = i;
            }
        }
        return viewPos;
    }

  

    public int getRowCount() {
        if (tableModel == null) {
            return 0;
        }
        return tableModel.getRowCount();
    }

    public String getColumnName(int col) {
        return tableModel.getColumnName(col);
    }

    public int getColumnCount() {
        if (tableModel == null) {
            return 0;
        }
        return tableModel.getColumnCount();
    }

    public Class<?> getColumnClass(int col) {
        return tableModel.getColumnClass(col);
    }

    public boolean isCellEditable(int row, int col) {
        int modelIndex = getModelRowIndex(row);
        return tableModel.isCellEditable(modelIndex, col);
    }

    public Object getValueAt(int row, int col) {
        int modelIndex = getModelRowIndex(row);
        return tableModel.getValueAt(modelIndex, col);
    }

    public void setValueAt(Object val, int row, int col) {
        int modelIndex = getModelRowIndex(row);
        tableModel.setValueAt(val, modelIndex, col);
    }

    // Helper classes
    
    private class TableRow implements Comparable {
        private int modelIndex;

        public TableRow(int index) {
            this.modelIndex = index;
        }

        public int compareTo(Object o) {
            int row1 = modelIndex;
            int row2 = ((TableRow) o).modelIndex;
            
            Iterator<Integer> mapIter = sortingOrders.keySet().iterator();

            while (mapIter.hasNext()) {
                
                Integer column = mapIter.next();
                Object o1 = tableModel.getValueAt(row1, column);
                Object o2 = tableModel.getValueAt(row2, column);

                int comparison = 0;
                if (o1 == null && o2 == null) {
                    comparison = 0;
                } else if (o1 == null) {
                    comparison = -1;
                } else if (o2 == null) {
                    comparison = 1;
                } else {
                    comparison = o1.toString().compareTo(o2.toString());;
                }
                if (comparison != 0) {
                    if (getSortState(column.intValue()).equals(DESCENDING)) {
                        return -comparison;
                    }
                    return comparison;
                }
            }
            return 0;
        }
    }

    private class TableModelHandler implements TableModelListener {
        public void tableChanged(TableModelEvent e) {
            if (sortingOrders.size() != 0) {
                resetSortState();
                fireTableChanged(e);
                return;
            }
            if (e.getFirstRow() == TableModelEvent.HEADER_ROW) {
                
                fireTableChanged(e);
                return;
            }
            int column = e.getColumn();
            if (e.getFirstRow() == e.getLastRow()
                    && column != TableModelEvent.ALL_COLUMNS
                    && getSortState(column).equals(NOT_SORTED)
                    && viewPos != null) {
                int viewIndex = getViewIndexArray()[e.getFirstRow()];
                fireTableChanged(new TableModelEvent(TableSorter.this, 
                                                     viewIndex, viewIndex, 
                                                     column, e.getType()));
                return;
            }

            resetSortState();
            fireTableDataChanged();
            return;
        }
    }

    private class MouseHandler extends MouseAdapter {
        public void mouseClicked(MouseEvent e) {
            JTableHeader h = (JTableHeader) e.getSource();
            TableColumnModel columnModel = h.getColumnModel();
            int viewColumn = columnModel.getColumnIndexAtX(e.getX());
            int column = columnModel.getColumn(viewColumn).getModelIndex();
            if (column == 0) {
                String status = getSortState(column);
                

                if (status.equals(ASCENDING)) {
                    status = DESCENDING;
                }
                else if (status.equals(DESCENDING)) {
                    status = NOT_SORTED;
                }
                else if (status.equals(NOT_SORTED)) {
                    status = ASCENDING;
                }
                setSortState(column, status);
            }
        }
    }

}
