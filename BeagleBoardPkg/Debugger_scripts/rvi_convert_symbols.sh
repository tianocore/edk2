#!/bin/sh
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
#  
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#


IN=`/usr/bin/cygpath -u $1`
OUT=`/usr/bin/cygpath -u $2`

/usr/bin/sed -e "s/\/cygdrive\/\(.\)/load\/a\/ni\/np \"\1:/g" \
             -e 's:\\:/:g' \
             -e "s/^/load\/a\/ni\/np \"/g" \
             -e "s/dll /dll\" \&/g" \
              $IN | /usr/bin/sort.exe --key=3 --output=$OUT

