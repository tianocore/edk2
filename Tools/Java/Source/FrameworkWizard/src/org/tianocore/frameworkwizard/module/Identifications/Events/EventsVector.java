/** @file
 
 The file is used to define Package Dependencies Vector
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.module.Identifications.Events;

import java.util.Vector;

public class EventsVector {

    private Vector<EventsIdentification> vEvents = new Vector<EventsIdentification>();

    public int findEvents(EventsIdentification sfi) {
        for (int index = 0; index < vEvents.size(); index++) {
            if (vEvents.elementAt(index).equals(sfi)) {
                return index;
            }
        }
        return -1;
    }

    public int findEvents(String name) {
        for (int index = 0; index < vEvents.size(); index++) {
            if (vEvents.elementAt(index).getName().equals(name)) {
                return index;
            }
        }
        return -1;
    }

    public EventsIdentification getEvents(int index) {
        if (index > -1) {
            return vEvents.elementAt(index);
        } else {
            return null;
        }
    }

    public void addEvents(EventsIdentification arg0) {
        vEvents.addElement(arg0);
    }

    public void setEvents(EventsIdentification arg0, int arg1) {
        vEvents.setElementAt(arg0, arg1);
    }

    public void removeEvents(EventsIdentification arg0) {
        int index = findEvents(arg0);
        if (index > -1) {
            vEvents.removeElementAt(index);
        }
    }

    public void removeEvents(int index) {
        if (index > -1 && index < this.size()) {
            vEvents.removeElementAt(index);
        }
    }

    public Vector<EventsIdentification> getvEvents() {
        return vEvents;
    }

    public void setvEvents(Vector<EventsIdentification> Events) {
        vEvents = Events;
    }

    public Vector<String> getEventsName() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vEvents.size(); index++) {
            v.addElement(vEvents.get(index).getName());
        }
        return v;
    }

    public int size() {
        return this.vEvents.size();
    }
    
    public Vector<String> toStringVector(int index) {
        Vector<String> v = new Vector<String>();
        v.addElement(getEvents(index).getName());
        v.addElement(getEvents(index).getType());
        v.addElement(getEvents(index).getUsage());
        return v;
    }
}
