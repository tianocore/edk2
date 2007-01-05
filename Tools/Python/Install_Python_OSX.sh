echo "You proxy is currently set to \"$http_proxy\"."
echo "It needs to set in order to download through a firewall."
echo "For example: \"export http_proxy=proxy.company.com:911\""
set -e
echo "After the download, get ready to enter the root password."
curl -C - --remote-name http://www.python.org/ftp/python/2.4.4/python-2.4.4-macosx2006-10-18.dmg --progress-bar
hdiutil attach python-2.4.4-macosx2006-10-18.dmg
sudo installer -pkg "/Volumes/Univeral MacPython 2.4.4/MacPython.mpkg/" -target /
sudo ln -fs /usr/local/bin/python2.4 /usr/bin/python
hdiutil unmount "/Volumes/Univeral MacPython 2.4.4/"

# Get the wxpython installer.
curl -C - --remote-name http://superb-west.dl.sourceforge.net/sourceforge/wxpython/wxPython2.8-osx-unicode-2.8.0.1-universal10.4-py2.4.dmg --progress-bar
hdiutil attach wxPython2.8-osx-unicode-2.8.0.1-universal10.4-py2.4.dmg 
sudo installer -pkg /Volumes/wxPython2.8-osx-unicode-2.8.0.1-universal10.4-py2.4/wxPython2.8-osx-unicode-universal10.4-py2.4.pkg -target /
hdiutil unmount /Volumes/wxPython2.8-osx-unicode-2.8.0.1-universal10.4-py2.4/
