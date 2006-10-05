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
 * This parser state checks consumed characters against a specific character
 * (case insensitive) or whitespace.
 *
 * @author Curt Arnold
 */
public final class WhitespaceOrCaseInsensitiveLetterState
    extends
    AbstractParserState {
  /**
   * Next state if the character is found.
   */
  private final AbstractParserState nextState;

  /**
   * Character to match (lower case).
   */
  private final char lowerLetter;

  /**
   * Character to match (upper case).
   */
  private final char upperLetter;

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
  public WhitespaceOrCaseInsensitiveLetterState(final AbstractParser parser,
                                                final char matchLetter,
                                                final AbstractParserState
                                                nextStateArg) {
    super(parser);
    this.lowerLetter = Character.toLowerCase(matchLetter);
    this.upperLetter = Character.toUpperCase(matchLetter);
    this.nextState = nextStateArg;
  }

  /**
   * Consumes a character and returns the next state for the parser.
   *
   * @param ch
   *            next character
   * @return the configured nextState if ch is the expected character or the
   *         configure noMatchState otherwise.
   */
  public AbstractParserState consume(final char ch) {
    if (ch == lowerLetter || ch == upperLetter) {
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
