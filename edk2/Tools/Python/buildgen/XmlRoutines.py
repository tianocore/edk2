#!/usr/bin/env python

# Copyright (c) 2007, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

"""This is an XML API that uses a syntax similar to XPath, but it is written in
   standard python so that no extra python packages are required to use it."""

import xml.dom.minidom

def XmlList(Dom, String):
    """Get a list of XML Elements using XPath style syntax."""
    if String == ""  or Dom == None or not isinstance(Dom, xml.dom.Node):
        return []
  
    if String[0] == "/":
        String = String[1:]

    if Dom.nodeType==Dom.DOCUMENT_NODE:
        Dom = Dom.documentElement

    tagList = String.split('/')
    nodes = [Dom]
    childNodes = []
    index = 0
    end = len(tagList) - 1
    while index <= end:
        for node in nodes:
            if node.nodeType == node.ELEMENT_NODE and node.tagName == tagList[index]:
                if index < end:
                    childNodes.extend(node.childNodes)
                else:
                    childNodes.append(node)

        nodes = childNodes
        childNodes = []
        index += 1
    
    return nodes

def XmlElement (Dom, String):
    """Return a single element that matches the String which is XPath style syntax."""
    if String == ""  or Dom == None or not isinstance(Dom, xml.dom.Node):
        return ""

    if String[0] == "/":
        String = String[1:]

    if Dom.nodeType==Dom.DOCUMENT_NODE:
        Dom = Dom.documentElement

    tagList = String.split('/')
    childNodes = [Dom]
    index = 0
    end = len(tagList) - 1
    while index <= end:
        for node in childNodes:
            if node.nodeType == node.ELEMENT_NODE and node.tagName == tagList[index]:
                if index < end:
                    childNodes = node.childNodes
                else:
                    return node
                break

        index += 1

    return ""

def XmlElementData (Dom):
    """Get the text for this element."""
    if Dom == None or Dom == '' or Dom.firstChild == None:
        return ''

    return Dom.firstChild.data.strip(' ')

def XmlAttribute (Dom, String):
    """Return a single attribute that named by String."""
    if Dom == None or Dom == '':
        return ''

    try:
        return Dom.getAttribute(String).strip(' ')
    except:
        return ''

def XmlTopTag(Dom):
    """Return the name of the Root or top tag in the XML tree."""
    if Dom == None or Dom == '' or Dom.firstChild == None:
        return ''
    return Dom.firstChild.nodeName
  

# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
if __name__ == '__main__':

    # Nothing to do here. Could do some unit tests.
    pass
