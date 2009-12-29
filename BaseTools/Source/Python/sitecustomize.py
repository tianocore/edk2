import sys
import locale

if sys.platform == "darwin":
  DefaultLocal = locale.getdefaultlocale()[1]
  if DefaultLocal is None:
    DefaultLocal = 'UTF8'  
  sys.setdefaultencoding(DefaultLocal)

