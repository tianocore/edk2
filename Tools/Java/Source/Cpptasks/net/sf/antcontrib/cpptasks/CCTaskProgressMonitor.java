/*
 * 
 * Copyright 2001-2004 The Ant-Contrib project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
package net.sf.antcontrib.cpptasks;
import java.io.IOException;

import net.sf.antcontrib.cpptasks.compiler.ProcessorConfiguration;
import net.sf.antcontrib.cpptasks.compiler.ProgressMonitor;
public class CCTaskProgressMonitor implements ProgressMonitor {
    private ProcessorConfiguration config;
    private TargetHistoryTable history;
    private long lastCommit = -1;
    public CCTaskProgressMonitor(TargetHistoryTable history) {
        this.history = history;
    }
    public void finish(ProcessorConfiguration config, boolean normal) {
        long current = System.currentTimeMillis();
        if ((current - lastCommit) > 120000) {
            try {
                history.commit();
                lastCommit = System.currentTimeMillis();
            } catch (IOException ex) {
            }
        }
    }
    public void progress(String[] sources) {
        history.update(config, sources);
        long current = System.currentTimeMillis();
        if ((current - lastCommit) > 120000) {
            try {
                history.commit();
                lastCommit = current;
            } catch (IOException ex) {
            }
        }
    }
    public void start(ProcessorConfiguration config) {
        if (lastCommit < 0) {
            lastCommit = System.currentTimeMillis();
        }
        this.config = config;
    }
}
