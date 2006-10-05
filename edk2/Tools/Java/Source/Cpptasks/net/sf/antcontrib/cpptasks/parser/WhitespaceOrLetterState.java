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

/**
 * This parser state checks consumed characters against a specific character or
 * whitespace.
 *
 * @author Curt Arnold
 */
public final class WhitespaceOrLetterState
    extends AbstractParserState {
  /**
   * Next state if the character is found.
   */
  private final AbstractParserState nextState;

  /**
   * Character to match.
   */
  private final char thisLetter;

  /**
   * Constructor.
   *
   * @param parser
   *            parser
   * @param matchLetter
   *            letter to match
   * @param nextStateArg
   *            next state if a match on the letter
   */
  public WhitespaceOrLetterState(final AbstractParser parser,
                                 final char matchLetter,
                                 final AbstractParserState nextStateArg) {
    super(parser);
    this.thisLetter = matchLetter;
    this.nextState = nextStateArg;
  }

  /**
   * Consumes a character and returns the next state for the parser.
   *
   * @param ch
   *            next character @returns the configured nextState if ch is the
   *            expected character or the configure noMatchState otherwise.
   * @return next state
   */
  public AbstractParserState consume(final char ch) {
    if (ch == thisLetter) {
      return nextState;
    }
    if (ch == ' ' || ch == '\t') {
      return this;
    }
    if (ch == '\n') {
      getParser().getNewLineState();
    }
    return null;
  }
}
