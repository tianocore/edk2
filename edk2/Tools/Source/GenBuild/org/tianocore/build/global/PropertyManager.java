package org.tianocore.build.global;

import java.util.HashMap;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Set;
import java.util.Stack;

import org.apache.tools.ant.Project;
import org.apache.tools.ant.PropertyHelper;

public class PropertyManager {
    private static Stack<HashMap<String, String>> propertyTableStack = new Stack<HashMap<String, String>>();
    private static HashMap<String, String> orgPropertyTable = null;
    private static HashMap<String, String> oldPropertyTable = null;
    private static HashMap<String, String> bakPropertyTable = null;
    private static Project prj = null;

    public static void save() {
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

        if (bakPropertyTable != null) {
            propertyTableStack.push(bakPropertyTable);
            oldPropertyTable = bakPropertyTable;
        } else {
            oldPropertyTable = orgPropertyTable;
        }
        bakPropertyTable = new HashMap<String, String>();
    }

    public static void restore() {
        if (bakPropertyTable == null) {
            return;
        }
        Set keys = bakPropertyTable.keySet();

        Iterator iter = keys.iterator();
        while (iter.hasNext()) {
            String name = (String)iter.next();
            String value = (String)bakPropertyTable.get(name);
            setProperty(prj, name, value);
        }

        if (propertyTableStack.size() > 0) {
            bakPropertyTable = (HashMap<String, String>)propertyTableStack.pop();
        } else {
            bakPropertyTable = null;
        }

        if (propertyTableStack.size() == 0) {
            oldPropertyTable = orgPropertyTable;
        } else {
            oldPropertyTable = (HashMap<String, String>)propertyTableStack.peek();
        }
    }

    public static void setProject(Project prj) {
        PropertyManager.prj = prj;
    }

    public static void setProperty(String name, String value) {
        if (prj == null) {
            return;
        }

        setProperty(prj, name, value);

        if (oldPropertyTable == null || bakPropertyTable == null) {
            return;
        }

        String oldValue = oldPropertyTable.get(name);
        if (oldValue == null) {
            oldValue = value;
        }
        bakPropertyTable.put(name, oldValue);
    }

    public static void setProperty(Project project, String name, String value) {
        if (project == null) {
            if (prj == null) {
                return;
            }
            project = prj;
        }

        PropertyHelper.getPropertyHelper(project).setProperty(null, name, value, false);
    }
}

