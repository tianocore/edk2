## @file
#  Small test script generator for OrderedCollectionTest.
#
#  Usage:
#  - generate script:        sh gentest.sh >input.txt
#  - run script with tester: OrderedCollectionTest -i input.txt -o output.txt
#
#  Copyright (C) 2014, Red Hat, Inc.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

set -e -u -C

RANGE_START=0
RANGE_STOP=9999

gen_rnd_insert()
{
  shuf --input-range=$RANGE_START-$RANGE_STOP | sed 's/^/insert /'
}

gen_rnd_delete()
{
  shuf --input-range=$RANGE_START-$RANGE_STOP | sed 's/^/delete /'
}

gen_mon_inc_insert()
{
  seq $RANGE_START $RANGE_STOP | sed 's/^/insert /'
}

gen_mon_inc_delete()
{
  seq $RANGE_START $RANGE_STOP | sed 's/^/delete /'
}

gen_mon_dec_delete()
{
  seq $RANGE_START $RANGE_STOP | tac | sed 's/^/delete /'
}

{
  echo '# populate the tree in random order and empty it iterating forward'
  gen_rnd_insert
  echo forward-empty

  echo
  echo '# populate the tree in random order and empty it iterating backward'
  gen_rnd_insert
  echo backward-empty

  echo
  echo '# populate the tree in random order, list it in increasing and'
  echo '# decreasing order, then empty it in random order'
  gen_rnd_insert
  echo forward-list
  echo backward-list
  gen_rnd_delete

  echo
  echo '# populate the tree in monotonically increasing order, then undo it'
  echo '# piecewise in the same order'
  gen_mon_inc_insert
  gen_mon_inc_delete

  echo
  echo '# populate the tree in monotonically increasing order, then undo it'
  echo '# piecewise in reverse order'
  gen_mon_inc_insert
  gen_mon_dec_delete

  echo
  echo '# populate the tree randomly, trigger a run of collisions, then exit'
  echo '# and let CmdForwardEmpty() empty the tree'
  gen_rnd_insert
  gen_mon_inc_insert
} \
| unix2dos
