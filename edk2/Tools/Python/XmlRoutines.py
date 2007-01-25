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
  if String == None or String == "" or Dom == None or Dom == "":
    return []
  if Dom.nodeType==Dom.DOCUMENT_NODE:
    Dom = Dom.documentElement
  if String[0] == "/":
    String = String[1:]
  tagList = String.split('/')
  nodes = [Dom]
  index = 0
  end = len(tagList) - 1
  while index <= end:
    childNodes = []
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

def XmlNode (Dom, String):
  """Return a single node that matches the String which is XPath style syntax."""
  if String == None or String == ""  or Dom == None or Dom == "":
    return ""
  if Dom.nodeType==Dom.DOCUMENT_NODE:
    Dom = Dom.documentElement
  if String[0] == "/":
    String = String[1:]
  tagList = String.split('/')
  index = 0
  end = len(tagList) - 1
  childNodes = [Dom]
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

def XmlElement (Dom, String):
  """Return a single element that matches the String which is XPath style syntax."""
  try:
    return XmlNode (Dom, String).firstChild.data.strip()
  except:
    return ''

def XmlElementData (Dom):
  """Get the text for this element."""
  if Dom == None or Dom == '' or Dom.firstChild == None:
    return ''
  return Dom.firstChild.data.strip()

def XmlAttribute (Dom, AttName):
  """Return a single attribute named AttName."""
  if Dom == None or Dom == '':
    return ''
  try:
    return Dom.getAttribute(AttName)
  except:
    return ''

def XmlTopTag(Dom):
  """Return the name of the Root or top tag in the XML tree."""
  return Dom.firstChild.nodeName
  
def XmlParseFile (FileName):
  """Parse an XML file into a DOM and return the DOM."""
  try:
    f = open(FileName, 'r')
    Dom = xml.dom.minidom.parse(f)
    f.close()
    return Dom
  except:
    return xml.dom.minidom.parseString('<empty/>')

def XmlParseString (Str):
  """Parse an XML string into a DOM and return the DOM."""
  try:
    return xml.dom.minidom.parseString(Str)
  except:
    return xml.dom.minidom.parseString('<empty/>')

def XmlParseFileSection (FileName, Tag):
  """Parse a section of an XML file into a DOM(Document Object Model) and return the DOM."""
  try:
    f = open(FileName, 'r')
  except:
    return xml.dom.minidom.parseString('<empty/>')
  Start = '<' + Tag
  End = '</' + Tag + '>'
  File = ''
  while File.find(Start) < 0 or File.find(End) < 0:
    Section = f.read(0x1000)
    if Section == '':
      break
    File += Section
  f.close()
  if File.find(Start) < 0 or File.find(End) < 0:
    return xml.dom.minidom.parseString('<empty/>')
  File = File[File.find(Start):File.find(End)+len(End)]
  try:
    return xml.dom.minidom.parseString(File)
  except:
    return xml.dom.minidom.parseString('<empty/>')

def XmlParseStringSection (XmlString, Tag):
  """Parse a section of an XML string into a DOM(Document Object Model) and return the DOM."""
  Start = '<' + Tag
  End = '</' + Tag + '>'
  File = XmlString
  if File.find(Start) < 0 or File.find(End) < 0:
    return xml.dom.minidom.parseString('<empty/>')
  File = File[File.find(Start):File.find(End)+len(End)]
  try:
    return xml.dom.minidom.parseString(File)
  except:
    return xml.dom.minidom.parseString('<empty/>')

def XmlSaveFile (Dom, FileName, Encoding='UTF-8'):
  """Save a DOM(Document Object Model) into an XML file."""
  try:
    f = open(FileName, 'w')
    f.write(Dom.toxml(Encoding).replace('&quot;','"').replace('&gt;','>'))
    f.close()
    return True
  except:
    return False

def XmlRemoveElement(Node):
  """Remove an element node from DOM(Document Object Model) tree."""
  ParentNode = Node.parentNode
  if ParentNode == None:
    return False
  PreviousSibling = Node.previousSibling
  while PreviousSibling != None and PreviousSibling.nodeType == PreviousSibling.TEXT_NODE and PreviousSibling.data.strip() == '':
    Temp = PreviousSibling
    PreviousSibling = PreviousSibling.previousSibling
    ParentNode.removeChild(Temp)
  ParentNode.removeChild(Node)
  return True

def XmlAppendChildElement(ParentNode, TagName, ElementText='', AttributeDictionary = {}):
  """Add a child element to a DOM(Document Object Model) tree with optional Attributes."""
  TagName = TagName.strip()
  if TagName == '':
    return None
  Depth = 0
  Dom = ParentNode
  while Dom != None and Dom.nodeType != Dom.DOCUMENT_NODE:
    Dom = Dom.parentNode
    Depth += 1
  if Dom == None:
    return None
  ParentNode.appendChild(Dom.createTextNode('\n%*s' % (Depth * 2, '')))
  ElementNode = Dom.createElement(TagName)
  if ElementText != '':
    ElementNode.appendChild(Dom.createTextNode(ElementText))
  for Item in AttributeDictionary:
    ElementNode.setAttribute(Item, AttributeDictionary[Item])
  ParentNode.appendChild(ElementNode)
  return ElementNode
  

# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
if __name__ == '__main__':

  # Nothing to do here. Could do some unit tests.
  pass
