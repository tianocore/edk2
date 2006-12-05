#!/usr/bin/env python

# This is an XML API that uses a syntax similar to XPath, but it is written in
# standard python so that no extra python packages are required to use it.

import xml.dom.minidom

def XmlList(Dom, String):
  """Get a list of XML Elements using XPath style syntax."""
  if Dom.nodeType==Dom.DOCUMENT_NODE:
    return XmlList(Dom.documentElement, String)
  if String[0] == "/":
    return XmlList(Dom, String[1:])
  if String == "" :
    return []
  TagList = String.split('/')
  nodes = []
  if Dom.nodeType == Dom.ELEMENT_NODE and Dom.tagName.strip() == TagList[0]:
    if len(TagList) == 1:
      nodes = [Dom]
    else:
      restOfPath = "/".join(TagList[1:])
      for child in Dom.childNodes:
        nodes = nodes + XmlList(child, restOfPath)
  return nodes

def XmlElement (Dom, String):
  """Return a single element that matches the String which is XPath style syntax."""
  try:
    return XmlList (Dom, String)[0].firstChild.data.strip(' ')
  except:
    return ''

def XmlElementData (Dom):
  """Get the text for this element."""
  return Dom.firstChild.data.strip(' ')

def XmlAttribute (Dom, String):
  """Return a single attribute that named by String."""
  try:
    return Dom.getAttribute(String)
  except:
    return ''

# This acts like the main() function for the script, unless it is 'import'ed into another
# script.
if __name__ == '__main__':

  # Nothing to do here. Could do some unit tests.
  pass
