/** @file
  PropertyManager class.
  
  PropertyManager class wraps Project.setProperty and tracks overrided properties.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.build.global;

import java.util.HashMap;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Set;
import java.util.Stack;

import org.apache.tools.ant.Project;
import org.apache.tools.ant.PropertyHelper;

/**
   PropertyManager uses a incremental way to to track overrided properties when 
   setProperty. This is useful for property recovery in nestly calling build files.
   Another functionality of this class is to prevent warning message printed when
   building with "verbose" mode.
 **/
public class PropertyManager {
    //
    // Property table stack, keeps track the history of properties changes
    // 
    private static Stack<HashMap<String, String>> propertyTableStack = new Stack<HashMap<String, String>>();
    //
    // The very original properties
    // 
    private static HashMap<String, String> orgPropertyTable = null;
    //
    // The last changes of properties
    // 
    private static HashMap<String, String> oldPropertyTable = null;
    //
    // The current changes of properties
    // 
    private static HashMap<String, String> bakPropertyTable = null;
    //
    // The Project of tracking properties
    // 
    private static Project prj = null;
    //
    // PropertyHelper of this project for setting property quietly
    // 
    private static PropertyHelper ph = null;

    /**
       Backup properties that have been overrided onto the stack for later recovery.
     **/
    public static void save() {
        //
        // If this is the first time to save properties changes, keep all properties
        // of this project as the original property table.
        // 
        if (orgPropertyTable == null) {
            Hashtable prjProperties = prj.getProperties();
            orgPropertyTable = new HashMap<String, String>();

            Set keys = prjProperties.keySet();
            Iterator iter = keys.iterator();
            while (iter.hasNext()) {
                String item = (String)iter.next();
                orgPropertyTable.put(item, (String)prjProperties.get(item));
            }
        }

        //
        // If there're already overrided properties, push it onto stack; otherwise
        // prepare taking new overrided property by allocating space for it.
        // 
        if (bakPropertyTable != null) {
            propertyTableStack.push(bakPropertyTable);
            oldPropertyTable = bakPropertyTable;
        } else {
            oldPropertyTable = orgPropertyTable;
        }
        bakPropertyTable = new HashMap<String, String>();
    }

    /**
       Restore the properties backup
     **/
    public static void restore() {
        if (bakPropertyTable == null) {
            //
            // No properties backup, do nothing 
            // 
            return;
        }
        Set keys = bakPropertyTable.keySet();

        //
        // Re-set properties in backup
        // 
        Iterator iter = keys.iterator();
        while (iter.hasNext()) {
            String name = (String)iter.next();
            String value = (String)bakPropertyTable.get(name);
            ph.setProperty(null, name, value, false);
        }

        //
        // If there's backup history, get top one for next recovery
        // 
        if (propertyTableStack.size() > 0) {
            bakPropertyTable = (HashMap<String, String>)propertyTableStack.pop();
        } else {
            bakPropertyTable = null;    // no recovery any more
        }

        //
        // Determine last overrided properties for incremental judgement
        // 
        if (propertyTableStack.size() == 0) {
            oldPropertyTable = orgPropertyTable;
        } else {
            oldPropertyTable = (HashMap<String, String>)propertyTableStack.peek();
        }
    }

    /**
       Set current Project for save() and restore() use.

       @param prj
     **/
    public static void setProject(Project prj) {
        PropertyManager.prj = prj;
        PropertyManager.ph  = PropertyHelper.getPropertyHelper(prj);
    }

    /**
       Set a property for current project. It will also be put into property
       history record if the record table has been setup.

       @param name      Property name
       @param value     Property value
     **/
    public static void setProperty(String name, String value) {
        if (prj == null) {
            return;
        }

        setProperty(prj, name, value);
    }

    /**
       Set a property for current project. It will also be put into property
       history record if the record table has been setup.

       @param project   The Project for which the property will be set
       @param name      Property name
       @param value     Property value
     **/
    public static void setProperty(Project project, String name, String value) {
        if (project == null) {
            if (prj == null) {
                return; // a Project must be given; otherwise nothing can be set
            }
            project = prj;
        }

        //
        // Using PropertyHelper to set a property can be quiet (no override
        // warning preset).
        // 
        PropertyHelper.getPropertyHelper(project).setProperty(null, name, value, false);

        //
        // If no property override history record is found, do nothing further
        // 
        if (oldPropertyTable == null || bakPropertyTable == null) {
            return;
        }

        //
        // Put a copy of given property in history record.
        // 
        String oldValue = oldPropertyTable.get(name);
        if (oldValue == null) {
            oldValue = value;
        }
        bakPropertyTable.put(name, oldValue);
    }
}

