/*
 * 
 * Copyright 2002-2004 The Ant-Contrib project
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
package net.sf.antcontrib.cpptasks.borland;
import net.sf.antcontrib.cpptasks.parser.AbstractParser;
import net.sf.antcontrib.cpptasks.parser.AbstractParserState;
public class ConsumeToSpaceOrNewLine extends AbstractParserState {
    public ConsumeToSpaceOrNewLine(AbstractParser parser) {
        super(parser);
    }
    public AbstractParserState consume(char ch) {
        if (ch == ' ' || ch == '\t' || ch == '\n') {
            return getParser().getNewLineState();
        }
        return this;
    }
}
