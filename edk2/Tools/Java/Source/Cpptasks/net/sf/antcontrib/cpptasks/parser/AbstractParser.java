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
package net.sf.antcontrib.cpptasks.parser;
import java.io.IOException;
import java.io.Reader;
/**
 * An abstract base class for simple parsers
 * 
 * @author Curt Arnold
 */
public abstract class AbstractParser {
    /**
     * 
     *  
     */
    protected AbstractParser() {
    }
    protected abstract void addFilename(String filename);
    public abstract AbstractParserState getNewLineState();
    protected void parse(Reader reader) throws IOException {
        char[] buf = new char[4096];
        AbstractParserState newLineState = getNewLineState();
        AbstractParserState state = newLineState;
        int charsRead = -1;
        do {
            charsRead = reader.read(buf, 0, buf.length);
            if (state == null) {
                for (int i = 0; i < charsRead; i++) {
                    if (buf[i] == '\n') {
                        state = newLineState;
                        break;
                    }
                }
            }
            if (state != null) {
                for (int i = 0; i < charsRead; i++) {
                    state = state.consume(buf[i]);
                    //
                    //  didn't match a production, skip to a new line
                    //
                    if (state == null) {
                        for (; i < charsRead; i++) {
                            if (buf[i] == '\n') {
                                state = newLineState;
                                break;
                            }
                        }
                    }
                }
            }
        } while (charsRead >= 0);
    }
}
