/** @file
 
 The file is used to provides some useful interfaces 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.common;

import java.awt.Component;
import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.UUID;
import java.util.Vector;

import javax.swing.DefaultListModel;
import javax.swing.JComboBox;
import javax.swing.JList;
import javax.swing.JOptionPane;
import javax.swing.JTable;

import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.MsaHeaderDocument.MsaHeader;
import org.tianocore.PackageSurfaceAreaDocument.PackageSurfaceArea;
import org.tianocore.PlatformHeaderDocument.PlatformHeader;
import org.tianocore.PlatformSurfaceAreaDocument.PlatformSurfaceArea;
import org.tianocore.SpdHeaderDocument.SpdHeader;
import org.tianocore.frameworkwizard.FrameworkWizardUI;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;

/**
 The class is used to provides some useful interfaces  
 
 **/
public class Tools {

    //
    // The dir user selected to create new package in
    //
    public static String dirForNewSpd = null;

    /**
     Get current date and time and format it as "yyyy-MM-dd HH:mm"
     
     @return formatted current date and time
     
     **/
    public static String getCurrentDateTime() {
        Date now = new Date(System.currentTimeMillis());
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm");
        return sdf.format(now);
    }

    /**
     Generate a UUID
     
     @return the created UUID
     
     **/
    public static String generateUuidString() {
        return UUID.randomUUID().toString();
    }

    /**
     Use current file separator in the path
     
     @param strPath
     @return
     
     **/
    public static String convertPathToCurrentOsType(String strPath) {
        strPath = strPath.replace(DataType.DOS_FILE_SEPARATOR, DataType.FILE_SEPARATOR);
        strPath = strPath.replace(DataType.UNIX_FILE_SEPARATOR, DataType.FILE_SEPARATOR);
        return strPath;
    }

    /**
     Use Unix file separator in the path
     
     @param strPath
     @return
     
     **/
    public static String convertPathToUnixType(String strPath) {
        strPath = strPath.replace(DataType.DOS_FILE_SEPARATOR, DataType.UNIX_FILE_SEPARATOR);
        return strPath;
    }

    /**
     Use Dos file separator in the path
     
     @param strPath
     @return
     
     **/
    public static String convertPathToDosType(String strPath) {
        strPath = strPath.replace(DataType.UNIX_FILE_SEPARATOR, DataType.DOS_FILE_SEPARATOR);
        return strPath;
    }

    /**
     Get all system properties and output to the console
     
     **/
    public static void getSystemProperties() {
        System.out.println(System.getProperty("java.class.version"));
        System.out.println(System.getProperty("java.class.path"));
        System.out.println(System.getProperty("java.ext.dirs"));
        System.out.println(System.getProperty("os.name"));
        System.out.println(System.getProperty("os.arch"));
        System.out.println(System.getProperty("os.version"));
        System.out.println(System.getProperty("file.separator"));
        System.out.println(System.getProperty("path.separator"));
        System.out.println(System.getProperty("line.separator"));
        System.out.println(System.getProperty("user.name"));
        System.out.println(System.getProperty("user.home"));
        System.out.println(System.getProperty("user.dir"));
        System.out.println(System.getProperty("PATH"));

        System.out.println(System.getenv("PROCESSOR_REVISION"));
    }

    /**
     Generate selection items for JComboBox by input vector
     
     **/
    public static void generateComboBoxByVector(JComboBox jcb, Vector<String> vector) {
        if (jcb != null) {
            jcb.removeAllItems();
        }
        if (vector != null) {
            for (int index = 0; index < vector.size(); index++) {
                jcb.addItem(vector.elementAt(index));
            }
        }
    }

    /**
     Generate selection items for JList by input vector
     
     **/
    public static void generateListByVector(JList jl, Vector<String> vector) {
        if (jl != null) {
            DefaultListModel listModel = (DefaultListModel) jl.getModel();
            listModel.removeAllElements();

            if (vector != null) {
                for (int index = 0; index < vector.size(); index++) {
                    listModel.addElement(vector.get(index));
                }
            }

            if (listModel.size() > 0) {
                jl.setSelectedIndex(0);
            }
        }
    }

    /**
     Get path only from a path
     
     @param filePath
     @return
     
     **/
    public static String getFilePathOnly(String filePath) {
        String path = filePath.substring(0, filePath.length() - getFileNameOnly(filePath).length());
        if (path.endsWith(DataType.FILE_SEPARATOR)) {
            path = path.substring(0, path.length() - DataType.FILE_SEPARATOR.length());
        }

        return path;
    }

    /**
     Get file name from a path
     
     @param filePath
     @return
     
     **/
    public static String getFileNameOnly(String filePath) {
        File f = new File(filePath);
        return f.getAbsoluteFile().getName();
    }

    public static String getFileNameWithoutExt(String filePath) {
        filePath = getFileNameOnly(filePath);
        filePath = filePath.substring(0, filePath.lastIndexOf(DataType.FILE_EXT_SEPARATOR));
        return filePath;
    }

    /**
     Get relative path
     
     @param wholePath
     @param commonPath
     @return wholePath - commonPath 
     
     **/
    public static String getRelativePath(String wholePath, String commonPath) {
        String path = "";
        int i = 0;
        i = wholePath.indexOf(commonPath);
        if (i > -1) {
            i = i + commonPath.length();
        } else {
            return "";
        }
        path = wholePath.substring(i);
        //
        // remove file separator of head
        //
        if (path.indexOf(DataType.DOS_FILE_SEPARATOR) == 0) {
            path = path.substring(0 + DataType.DOS_FILE_SEPARATOR.length());
        }
        if (path.indexOf(DataType.UNIX_FILE_SEPARATOR) == 0) {
            path = path.substring(0 + DataType.DOS_FILE_SEPARATOR.length());
        }
        //
        // remove file separator of rear
        //
        if (path.length() > 0
            && path.indexOf(DataType.DOS_FILE_SEPARATOR) == path.length() - DataType.DOS_FILE_SEPARATOR.length()) {
            path = path.substring(0, path.length() - DataType.DOS_FILE_SEPARATOR.length());
        }
        if (path.length() > 0
            && path.indexOf(DataType.UNIX_FILE_SEPARATOR) == path.length() - DataType.UNIX_FILE_SEPARATOR.length()) {
            path = path.substring(0, path.length() - DataType.DOS_FILE_SEPARATOR.length());
        }
        //
        // convert to UNIX format
        //
        path = Tools.convertPathToUnixType(path);
        return path;
    }

    /**
     Convert List ot Vector
     
     @param list
     @return
     
     **/
    public static Vector<String> convertListToVector(List list) {
        Vector<String> v = new Vector<String>();
        if (list != null && list.size() > 0) {
            for (int index = 0; index < list.size(); index++) {
                v.addElement(list.get(index).toString());
            }
        }
        return v;
    }

    /**
     Convert a Vector to a String, separator with ", "
     
     @param v
     @return
     
     **/
    public static String convertVectorToString(Vector<String> v) {
        String s = "";
        for (int index = 0; index < v.size(); index++) {
            s = s + v.elementAt(index).toString() + ", ";
        }
        if (s.length() > 0) {
            s = s.substring(0, s.length() - ", ".length());
        }
        return s;
    }

    /**
     Convert a List to a String
     
     @param list
     @return
     
     **/
    public static String convertListToString(List list) {
        return Tools.convertVectorToString(Tools.convertListToVector(list));
    }

    /**
     If the input path missing ext, append the ext to the path
     
     @param path
     @param type
     @return
     
     **/
    public static String addPathExt(String path, int type) {
        String match = "";
        if (type == DataType.RETURN_TYPE_MODULE_SURFACE_AREA) {
            match = DataType.FILE_EXT_SEPARATOR + DataType.MODULE_SURFACE_AREA_EXT;
        }
        if (type == DataType.RETURN_TYPE_PACKAGE_SURFACE_AREA) {
            match = DataType.FILE_EXT_SEPARATOR + DataType.PACKAGE_SURFACE_AREA_EXT;
        }
        if (type == DataType.RETURN_TYPE_PLATFORM_SURFACE_AREA) {
            match = DataType.FILE_EXT_SEPARATOR + DataType.PLATFORM_SURFACE_AREA_EXT;
        }
        if (type == DataType.RETURN_TYPE_TEXT) {
            match = DataType.FILE_EXT_SEPARATOR + DataType.TEXT_FILE_EXT;
        }
        if (type == DataType.RETURN_TYPE_FAR_SURFACE_AREA) {
            match = DataType.FILE_EXT_SEPARATOR + DataType.FAR_SURFACE_AREA_EXT;
        }
        if (path.length() <= match.length()) {
            path = path + match;
            return path;
        }
        if (!(path.substring(path.length() - match.length())).equals(match)) {
            path = path + match;
        }
        return path;
    }

    /**
     Show a message box
     
     @param arg0
     
     **/
    public static void showInformationMessage(String arg0) {
        JOptionPane.showConfirmDialog(FrameworkWizardUI.getInstance(), arg0, "Info", JOptionPane.DEFAULT_OPTION, JOptionPane.INFORMATION_MESSAGE);
    }

    /**
     if the string doesn't end with a file separator, append it to the string
     
     @param arg0
     @return
     
     **/
    public static String addFileSeparator(String arg0) {
        if (!arg0.endsWith(DataType.FILE_SEPARATOR)) {
            arg0 = arg0 + DataType.FILE_SEPARATOR;
        }
        return arg0;
    }

    /**
     Wrap single line long input string to multiple short line string by word
     
     @param arg0 input string
     @return wraped string
     
     **/
    public static String wrapStringByWord(String arg0) {
        int intMaxLength = 40;
        String strReturn = "";
        String strTemp = "";
        boolean isCopied = true;

        if (arg0 == null) {
            return "";
        }
        if (arg0.length() <= 0) {
            return "";
        }

        //
        // Convert string to array by " "
        //
        String s[] = arg0.split(" ");
        if (arg0.indexOf(" ") == -1) {
            s[0] = arg0;
        }

        //
        // Add each string of array one by one
        //
        for (int index = 0; index < s.length; index++) {
            String ss = s[index];
            isCopied = false;
            //
            // The word length > defined line length
            //
            if (ss.length() > intMaxLength) {
                //
                // Finish previous line
                //
                if (!isCopied) {
                    strReturn = strReturn + strTemp + DataType.LINE_SEPARATOR;
                    strTemp = "";
                }
                //
                // Separater to short lines
                //
                while (ss.length() > 0) {
                    if (ss.length() > intMaxLength) {
                        strReturn = strReturn + s[index].substring(0, intMaxLength - 1) + DataType.UNIX_LINE_SEPARATOR;
                        ss = ss.substring(intMaxLength);
                        isCopied = true;
                    } else {
                        strTemp = ss;
                        ss = "";
                        isCopied = false;
                    }
                }
            } else {
                if ((strTemp + " " + ss).length() <= intMaxLength) {
                    strTemp = strTemp + " " + ss;
                    continue;
                } else {
                    strReturn = strReturn + strTemp + DataType.LINE_SEPARATOR;
                    if ((index == s.length - 1) && (!ss.equals(""))) {
                        strReturn = strReturn + ss;
                    } else {
                        strTemp = ss + " ";
                    }
                    isCopied = true;
                }
            }
        }

        if (!isCopied) {
            strReturn = strReturn + strTemp;
        }

        return strReturn;
    }

    public static String convertUnicodeHexStringToString(String str) {
        //
        // Handle if str is null or empty
        //
        if (str == null) {
            return "";
        }
        if (str.equals("")) {
            return "";
        }

        String returnString = "";
        String[] strArray = str.split(" ");
        for (int index = 0; index < strArray.length; index++) {
            String s = strArray[index];
            if (s.length() == 6 && s.indexOf(DataType.HEX_STRING_HEADER) == 0) {
                s = s.substring(DataType.HEX_STRING_HEADER.length());
            } else {
                Log.err("convertUnicodeHexStringToString", "Incorrect input string: " + str);
                continue;
            }
            //
            // Change hex to dec
            //
            int dec = Integer.parseInt(s, 16);

            returnString = returnString + (char) (dec);
        }
        return returnString;
    }

    /**
     Convert input string to unicode hex string
     
     @param str input string
     @return unicode hex string
     
     **/
    public static String convertStringToUnicodeHexString(String str) {
        //
        // Handle if str is null or empty
        //
        if (str == null) {
            return "";
        }
        if (str.equals("")) {
            return "";
        }

        //
        // convert string to hex string
        //
        String hexString = "";
        for (int index = 0; index < str.length(); index++) {
            int codePoint = str.codePointAt(index);
            String s = Integer.toHexString(codePoint);
            //
            // Make the string to four length
            //
            if (s.length() == 3) {
                s = "0" + s;
            } else if (s.length() == 2) {
                s = "00" + s;
            } else if (s.length() == 1) {
                s = "000" + s;
            }

            //
            // Add the string to return hex string
            //
            hexString = hexString + DataType.HEX_STRING_HEADER + s + " ";
        }

        //
        // return hex string
        //
        return hexString.trim();
    }

    public static ModuleIdentification getId(String path, ModuleSurfaceArea msa) {
        MsaHeader head = msa.getMsaHeader();
        String name = head.getModuleName();
        String guid = head.getGuidValue();
        String version = head.getVersion();
        ModuleIdentification id = new ModuleIdentification(name, guid, version, path);
        return id;
    }

    public static PackageIdentification getId(String path, PackageSurfaceArea spd) {
        SpdHeader head = spd.getSpdHeader();
        String name = head.getPackageName();
        String guid = head.getGuidValue();
        String version = head.getVersion();
        PackageIdentification id = new PackageIdentification(name, guid, version, path);
        return id;
    }

    public static PlatformIdentification getId(String path, PlatformSurfaceArea fpd) {
        PlatformHeader head = fpd.getPlatformHeader();
        String name = head.getPlatformName();
        String guid = head.getGuidValue();
        String version = head.getVersion();
        PlatformIdentification id = new PlatformIdentification(name, guid, version, path);
        return id;
    }

    /**
     * To reset the width of input component via container width
     * 
     * @param c
     * @param containerWidth
     * 
     */
    public static void resizeComponentWidth(Component c, int containerWidth, int preferredWidth) {
        int newWidth = c.getPreferredSize().width + (containerWidth - preferredWidth);
        if (newWidth < c.getPreferredSize().width) {
            newWidth = c.getPreferredSize().width;
        }
        c.setSize(new java.awt.Dimension(newWidth, c.getHeight()));
        c.validate();
    }

    /**
     * To reset the height of input component via container height
     * 
     * @param c
     * @param containerHeight
     * 
     */
    public static void resizeComponentHeight(Component c, int containerHeight, int preferredHeight) {
        int newHeight = c.getPreferredSize().height + (containerHeight - preferredHeight);
        if (newHeight < c.getPreferredSize().height) {
            newHeight = c.getPreferredSize().height;
        }
        c.setSize(new java.awt.Dimension(c.getWidth(), newHeight));
        c.validate();
    }

    /**
     * To reset the size of input component via container size
     * 
     * @param c
     * @param containerWidth
     * @param containerHeight
     * 
     */
    public static void resizeComponent(Component c, int containerWidth, int containerHeight, int preferredWidth,
                                       int preferredHeight) {
        resizeComponentWidth(c, containerWidth, preferredWidth);
        resizeComponentHeight(c, containerHeight, preferredHeight);
    }

    /**
     To adjust each column's width to meet the table's size
     
     @param t the table need to be adjusted
     @param width the new width of the table
     
     **/
    public static void resizeTableColumn(JTable t, int width) {
        if (t != null) {
            int columnCount = t.getColumnCount();
            for (int index = 0; index < columnCount; index++) {
                t.getColumn(t.getColumnName(index)).setPreferredWidth(width / columnCount);
            }
        }
    }

    /**
     * To relocate the input component
     * 
     * @param c
     * @param containerWidth
     * @param spaceToRight
     * 
     */
    public static void relocateComponentX(Component c, int containerWidth, int preferredWidth, int spaceToRight) {
        int intGapToRight = spaceToRight + c.getPreferredSize().width;
        int newLocationX = containerWidth - intGapToRight;
        if (newLocationX < preferredWidth - intGapToRight) {
            newLocationX = preferredWidth - intGapToRight;
        }
        c.setLocation(newLocationX, c.getLocation().y);
        c.validate();
    }

    /**
     * To relocate the input component
     * 
     * @param c
     * @param containerHeight
     * @param spaceToBottom
     * 
     */
    public static void relocateComponentY(Component c, int containerHeight, int preferredHeight, int spaceToBottom) {
        int intGapToBottom = spaceToBottom + c.getPreferredSize().height;
        int newLocationY = containerHeight - intGapToBottom;
        if (newLocationY < preferredHeight - spaceToBottom) {
            newLocationY = preferredHeight - spaceToBottom;
        }
        c.setLocation(c.getLocation().x, newLocationY);
        c.validate();
    }

    /**
     * To relocate the input component
     * 
     * @param c
     * @param containerWidth
     * @param containerHeight
     * @param spaceToBottom
     * @param spaceToRight
     * 
     */
    public static void relocateComponent(Component c, int containerWidth, int containerHeight, int preferredWidht,
                                         int preferredHeight, int spaceToRight, int spaceToBottom) {
        relocateComponentX(c, containerWidth, preferredWidht, spaceToRight);
        relocateComponentY(c, containerHeight, preferredHeight, spaceToBottom);
    }

    /**
     Move the component to the center of screen 
     
     @param c
     @param width
     
     **/
    public static void centerComponent(Component c, int width) {
        c.setLocation(width / 2 - c.getWidth() / 2, c.getLocation().y);
        c.validate();
    }

    /**
     Move the component to the center of screen and adjust the y location 
     
     @param c
     @param width
     
     **/
    public static void centerComponent(Component c, int width, int containerHeight, int preferredHeight,
                                       int spaceToBottom) {
        relocateComponentY(c, containerHeight, preferredHeight, spaceToBottom);
        centerComponent(c, width);
    }

    /**
     Find the count of searchString in wholeString
     
     @param wholeString
     @param searchString
     @return

     **/
    public static int getSpecificStringCount(String wholeString, String searchString) {
        int count = 0;
        count = wholeString.split(searchString).length;
        return count;
    }

    /**
     * Check the input data is empty or not
     * 
     * @param strValue
     *            The input data which need be checked
     * 
     * @retval true - The input data is empty
     * @retval fals - The input data is not empty
     * 
     */
    public static boolean isEmpty(String strValue) {
        if (strValue == null) {
            return true;
        }
        if (strValue.length() > 0) {
            return false;
        }
        return true;
    }
}
