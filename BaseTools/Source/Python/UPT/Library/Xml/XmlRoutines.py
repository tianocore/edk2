## @file
# This is an XML API that uses a syntax similar to XPath, but it is written in
# standard python so that no extra python packages are required to use it.
#
# Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available 
# under the terms and conditions of the BSD License which accompanies this 
# distribution. The full text of the license may be found at 
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

'''
XmlRoutines
'''

##
# Import Modules
#
import xml.dom.minidom
import re
import codecs
from Logger.ToolError import PARSER_ERROR
import Logger.Log as Logger

## Create a element of XML
#
# @param Name
# @param String
# @param NodeList
# @param AttributeList
#
def CreateXmlElement(Name, String, NodeList, AttributeList):
    Doc = xml.dom.minidom.Document()
    Element = Doc.createElement(Name)
    if String != '' and String != None:
        Element.appendChild(Doc.createTextNode(String))

    for Item in NodeList:
        if type(Item) == type([]):
            Key = Item[0]
            Value = Item[1]
            if Key != '' and Key != None and Value != '' and Value != None:
                Node = Doc.createElement(Key)
                Node.appendChild(Doc.createTextNode(Value))
                Element.appendChild(Node)
        else:
            Element.appendChild(Item)
    for Item in AttributeList:
        Key = Item[0]
        Value = Item[1]
        if Key != '' and Key != None and Value != '' and Value != None:
            Element.setAttribute(Key, Value)

    return Element

## Get a list of XML nodes using XPath style syntax.
#
# Return a list of XML DOM nodes from the root Dom specified by XPath String.
# If the input Dom or String is not valid, then an empty list is returned.
#
# @param  Dom                The root XML DOM node.
# @param  String             A XPath style path.
#
def XmlList(Dom, String):
    if String == None or String == "" or Dom == None or Dom == "":
        return []
    if Dom.nodeType == Dom.DOCUMENT_NODE:
        Dom = Dom.documentElement
    if String[0] == "/":
        String = String[1:]
    TagList = String.split('/')
    Nodes = [Dom]
    Index = 0
    End = len(TagList) - 1
    while Index <= End:
        ChildNodes = []
        for Node in Nodes:
            if Node.nodeType == Node.ELEMENT_NODE and Node.tagName == \
            TagList[Index]:
                if Index < End:
                    ChildNodes.extend(Node.childNodes)
                else:
                    ChildNodes.append(Node)
        Nodes = ChildNodes
        ChildNodes = []
        Index += 1

    return Nodes


## Get a single XML node using XPath style syntax.
#
# Return a single XML DOM node from the root Dom specified by XPath String.
# If the input Dom or String is not valid, then an empty string is returned.
#
# @param  Dom                The root XML DOM node.
# @param  String             A XPath style path.
#
def XmlNode(Dom, String):
    if String == None or String == ""  or Dom == None or Dom == "":
        return None
    if Dom.nodeType == Dom.DOCUMENT_NODE:
        Dom = Dom.documentElement
    if String[0] == "/":
        String = String[1:]
    TagList = String.split('/')
    Index = 0
    End = len(TagList) - 1
    ChildNodes = [Dom]
    while Index <= End:
        for Node in ChildNodes:
            if Node.nodeType == Node.ELEMENT_NODE and \
               Node.tagName == TagList[Index]:
                if Index < End:
                    ChildNodes = Node.childNodes
                else:
                    return Node
                break
        Index += 1
    return None


## Get a single XML element using XPath style syntax.
#
# Return a single XML element from the root Dom specified by XPath String.
# If the input Dom or String is not valid, then an empty string is returned.
#
# @param  Dom                The root XML DOM object.
# @param  Strin              A XPath style path.
#
def XmlElement(Dom, String):
    try:
        return XmlNode(Dom, String).firstChild.data.strip()
    except BaseException:
        return ""

## Get a single XML element using XPath style syntax.
#
# Similar with XmlElement, but do not strip all the leading and tailing space
# and newline, instead just remove the newline and spaces introduced by 
# toprettyxml() 
#
# @param  Dom                The root XML DOM object.
# @param  Strin              A XPath style path.
#
def XmlElement2(Dom, String):
    try:
        HelpStr = XmlNode(Dom, String).firstChild.data
        gRemovePrettyRe = re.compile(r"""(?:(\n *)  )(.*)\1""", re.DOTALL)
        HelpStr = re.sub(gRemovePrettyRe, r"\2", HelpStr)
        return HelpStr
    except BaseException:
        return ""


## Get a single XML element of the current node.
#
# Return a single XML element specified by the current root Dom.
# If the input Dom is not valid, then an empty string is returned.
#
# @param  Dom                The root XML DOM object.
#
def XmlElementData(Dom):
    try:
        return Dom.firstChild.data.strip()
    except BaseException:
        return ""


## Get a list of XML elements using XPath style syntax.
#
# Return a list of XML elements from the root Dom specified by XPath String.
# If the input Dom or String is not valid, then an empty list is returned.
#
# @param  Dom                The root XML DOM object.
# @param  String             A XPath style path.
#
def XmlElementList(Dom, String):
    return map(XmlElementData, XmlList(Dom, String))


## Get the XML attribute of the current node.
#
# Return a single XML attribute named Attribute from the current root Dom.
# If the input Dom or Attribute is not valid, then an empty string is returned.
#
# @param  Dom                The root XML DOM object.
# @param  Attribute          The name of Attribute.
#
def XmlAttribute(Dom, Attribute):
    try:
        return Dom.getAttribute(Attribute)
    except BaseException:
        return ''


## Get the XML node name of the current node.
#
# Return a single XML node name from the current root Dom.
# If the input Dom is not valid, then an empty string is returned.
#
# @param  Dom                The root XML DOM object.
#
def XmlNodeName(Dom):
    try:
        return Dom.nodeName.strip()
    except BaseException:
        return ''

## Parse an XML file.
#
# Parse the input XML file named FileName and return a XML DOM it stands for.
# If the input File is not a valid XML file, then an empty string is returned.
#
# @param  FileName           The XML file name.
#
def XmlParseFile(FileName):
    try:
        XmlFile = codecs.open(FileName, 'rb')
        Dom = xml.dom.minidom.parse(XmlFile)
        XmlFile.close()
        return Dom
    except BaseException, XExcept:
        XmlFile.close()
        Logger.Error('\nUPT', PARSER_ERROR, XExcept, File=FileName, RaiseError=True)
