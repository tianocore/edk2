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
package net.sf.antcontrib.cpptasks.parser;
public class BranchState extends AbstractParserState {
    private char[] branchChars;
    private AbstractParserState[] branchStates;
    private AbstractParserState noMatchState;
    public BranchState(AbstractParser parser, char[] branchChars,
            AbstractParserState[] branchStates, AbstractParserState noMatchState) {
        super(parser);
        this.branchChars = (char[]) branchChars.clone();
        this.branchStates = (AbstractParserState[]) branchStates.clone();
        this.noMatchState = noMatchState;
    }
    public AbstractParserState consume(char ch) {
        AbstractParserState state;
        for (int i = 0; i < branchChars.length; i++) {
            if (ch == branchChars[i]) {
                state = branchStates[i];
                return state.consume(ch);
            }
        }
        state = getNoMatchState();
        if (state != null) {
            return state.consume(ch);
        }
        return state;
    }
    protected AbstractParserState getNoMatchState() {
        return noMatchState;
    }
}
