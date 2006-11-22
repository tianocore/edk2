set -e
echo "After the download, get ready to enter the root password. You should make sure your proxy is set."
# export http_proxy=http://proxy.dp.intel.com:911
# curl --remote-name http://www.python.org/ftp/python/2.4.4/python-2.4.4-macosx2006-10-18.dmg --progress-bar
hdiutil attach python-2.4.4-macosx2006-10-18.dmg
sudo installer -pkg "/Volumes/Univeral MacPython 2.4.4/MacPython.mpkg/" -target /
sudo ln -fs /usr/local/bin/python2.4 /usr/bin/python
hdiutil unmount "/Volumes/Univeral MacPython 2.4.4/"
