#!/bin/sh
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
#  
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http:#opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#


IN=`/usr/bin/cygpath -u $1`
OUT=`/usr/bin/cygpath -u $2`

/usr/bin/sed -e "s/\/cygdrive\/\(.\)/load\/a\/ni\/np \"\1:/g" \
             -e 's:\\:/:g' \
             -e "s/^/load\/a\/ni\/np \"/g" \
             -e "s/dll /dll\" \&/g" \
              $IN | /usr/bin/sort.exe --key=3 --output=$OUT

