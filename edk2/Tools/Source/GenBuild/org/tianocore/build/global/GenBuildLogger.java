/*++

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 Module Name:
 GenBuildLogger.java

 Abstract:

 --*/

package org.tianocore.build.global;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.StringReader;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Vector;

import org.apache.tools.ant.BuildEvent;
import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.DefaultLogger;
import org.apache.tools.ant.Project;
import org.apache.tools.ant.Task;
import org.apache.tools.ant.util.StringUtils;

import org.tianocore.build.id.Identification;
import org.tianocore.common.logger.EdkLog;
import org.tianocore.common.logger.LogMethod;

public class GenBuildLogger extends DefaultLogger implements LogMethod {
    
    private Project project = null;

    ///
    /// flag to present whether cache all msg or not
    /// true means to cache.
    ///
    private static boolean flag = false;
    
    private static boolean enableFlag = true;

    private static Map<Identification, List<String>> map = new HashMap<Identification, List<String> >(256);
    
    private Identification id = null;
    
    public GenBuildLogger () {
        
    }

    public GenBuildLogger (Project project) {
        this.project = project;
    }

    public GenBuildLogger (Project project, Identification id) {
        this.project = project;
        this.id = id;
    }

    /**
      Rules: flag = false: means no cache Action: Print it to console
      
      flag = true: mean cache all msg exception some special Action: loglevel
      is EDK_ALWAYS -- Print but no cache loglevel is EDK_ERROR -- Print and
      cache the msg others -- No print and cache the msg
    **/
    public synchronized void putMessage(Object msgSource, int msgLevel, String msg) {
        if (this.project == null) {
            return;
        }

        //
        // If msgLevel is always print, then print it
        //
        switch (msgLevel) {
        case EdkLog.EDK_ALWAYS:
            //
            // Do some special
            //
            log(msgSource, msg, Project.MSG_ERR);
            break;
        case EdkLog.EDK_ERROR:
            log(msgSource, msg, Project.MSG_ERR);
            break;
        case EdkLog.EDK_WARNING:
            log(msgSource, msg, Project.MSG_WARN);
            break;
        case EdkLog.EDK_INFO:
            log(msgSource, msg, Project.MSG_INFO);
            break;
        case EdkLog.EDK_VERBOSE:
            log(msgSource, msg, Project.MSG_VERBOSE);
            break;
        case EdkLog.EDK_DEBUG:
            log(msgSource, msg, Project.MSG_DEBUG);
            break;
        }
    }

    public void flushToFile(File file) {
        //
        // Put all messages in map to file
        //
        String msg = "Writing log to file [" + file.getPath() + "]";
        log("Logging", msg, Project.MSG_INFO);
        try {
            BufferedWriter bw = new BufferedWriter(new FileWriter(file));
            List<String> allMessages = map.get(null);
            for(int i = 0; i < allMessages.size(); i++) {
                bw.write(allMessages.get(i));
                bw.newLine();
            }
            bw.flush();
            bw.close();
        } catch (IOException e) {
            new BuildException("Writing log error. " + e.getMessage());
        }
        
    }
    
    private void log(Object msgSource, String msg, int level) {
        if (msgSource instanceof Task) {
            this.project.log((Task)msgSource, msg, level);
        } else if (msgSource instanceof String){
            //
            // Pad 12 space to keep message in unify format
            //
            msg = msg.replaceAll("\n", "\n            ");
            this.project.log(String.format("%12s", "[" + msgSource + "] ") + msg, level);
        } else {
            this.project.log(msg, level);
        }
    }
    public void targetStarted(BuildEvent event) {
        if (!flag) {
            super.targetStarted(event);
        }
    }
    
    public void messageLogged(BuildEvent event) {
        if (!enableFlag) {
            return ;
        }
        int currentLevel = event.getPriority();
        //
        // If current level is upper than Ant Level, skip it
        //
        if (currentLevel <= this.msgOutputLevel) {
            String originalMessage = event.getMessage();

            StringBuffer message = new StringBuffer();
            if (!emacsMode && event.getTask() != null) {
                String label = String.format("%12s", "[" + event.getTask().getTaskName() + "] ");
                //
                // Append label first
                //
                message.append(label);
                
                //
                // Format all output message's line separator
                //
                try {
                    BufferedReader r = new BufferedReader(new StringReader(originalMessage));
                    boolean ifFirstLine = true;
                    String line = null;
                    while ((line = r.readLine()) != null) {
                        if (!ifFirstLine) {
                            message.append(StringUtils.LINE_SEP);
                        }
                        ifFirstLine = false;
                        message.append(line);
                    }
                } catch (IOException e) {
                    message.append(originalMessage);
                }
            } else {
                message.append(originalMessage);
            }

            String msg = message.toString();
            if (currentLevel == Project.MSG_ERR) {
                printMessage(msg, err, currentLevel);
            } else if(currentLevel == Project.MSG_WARN) {
                printMessage(msg, out, currentLevel);
            } else if(!flag) {
                printMessage(msg, out, currentLevel);
            } 
            
            log(msg);
        }
    }
    
    public static void setCacheEnable(boolean enable) {
        flag = enable;
    }
    
    public static void maskAllLog(boolean enable) {
        enableFlag = !enable;
    }
    
    protected synchronized void log(String message) {
        //
        // cache log
        //
        if (map.containsKey(this.id)) {
            map.get(this.id).add(message);
        } else {
            List<String> list = new Vector<String>(1024);
            list.add(message);
            map.put(this.id, list);
        }
    }
}