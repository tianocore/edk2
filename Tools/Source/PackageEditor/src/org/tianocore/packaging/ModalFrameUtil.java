/** @file
  Java class ModalFrameUtil is used to show modal frame.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.packaging;

import javax.swing.*;
import java.awt.*;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;

/**
 This class is used to show modal frame.
  
 @since PackageEditor 1.0
**/
public class ModalFrameUtil {
    /**
     Invocation handler for event threads
      
     @since PackageEditor 1.0
    **/
    static class EventPump implements InvocationHandler {
        Frame frame;

        public EventPump(Frame frame) {
            this.frame = frame;
        }

        /**
         Invocation handler invoked by Method.invoke
         **/
        public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
            //
            // return frame showing status for Conditional.evaluate()
            //
            return frame.isShowing() ? Boolean.TRUE : Boolean.FALSE;
        }

        public void start() throws Exception {
            Class clazz = Class.forName("java.awt.Conditional");
            //
            // Conditional proxy instance will invoke "this" InvocationHandler.invoke when calling its methods
            //
            Object conditional = Proxy.newProxyInstance(clazz.getClassLoader(), new Class[] { clazz }, this);
            //
            // EventDisaptchThread.pumpEvents will be called under Conditional "conditional"
            //
            Method pumpMethod = Class.forName("java.awt.EventDispatchThread").getDeclaredMethod("pumpEvents",
                                                                                                new Class[] { clazz });
            pumpMethod.setAccessible(true);
            //
            // pumpEvents when conditional.evaluate() == true (frame.isShowing() in EventPump.invoke)
            //
            pumpMethod.invoke(Thread.currentThread(), new Object[] { conditional });
        }
    }

    /**
     Show modal frame, return only when frame closed.
     
     @param frame Frame to be modal
     @param owner Parent Frame
    **/
    public static void showAsModal(final Frame frame, final Frame owner) {
        frame.addWindowListener(new WindowAdapter() {
            public void windowOpened(WindowEvent e) {
                owner.setEnabled(false);
            }

            public void windowClosed(WindowEvent e) {
                owner.setEnabled(true);
                owner.setVisible(true);
                owner.removeWindowListener(this);
            }
        });

        owner.addWindowListener(new WindowAdapter() {
            public void windowActivated(WindowEvent e) {
                if (frame.isShowing()) {
                    frame.setExtendedState(JFrame.NORMAL);
                    frame.toFront();
                } else {
                    owner.removeWindowListener(this);
                }
            }
        });

        frame.setVisible(true);
        try {
            new EventPump(frame).start();
        } catch (Throwable throwable) {
            throw new RuntimeException(throwable);
        }
    }
}
