cmake_minimum_required(VERSION 2.8) # Needed for file(COPY ...)

project(Memorycoin)

enable_testing()

set(MAJOR_VERSION 3)
set(MINOR_VERSION 0)
set(RELEASE_VERSION 000)
set(BUILD_VERSION 1)

set(PACKAGE memorycoin)
set(VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${RELEASE_VERSION}.${BUILD_VERSION})
set(host ${CMAKE_SYSTEM} ${CMAKE_SYSTEM_PROCESSOR})
set(COPYRIGHT "Copyright (c) 2009-2013 Bitcoin Developers, 2013-2014 Memorycoin Dev Team.  All Rights Reserved.")
set(prefix ${CMAKE_INSTALL_PREFIX})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake#!/bin/bash

if [ -d "$1" ]; then
  cd "$1"
else
  echo "Usage: $0 <datadir>" >&2
  echo "Removes obsolete Memorycoin database files" >&2
  exit 1
fi

LEVEL=0
if [ -f wallet.dat -a -f addr.dat -a -f blkindex.dat -a -f blk0001.dat ]; then LEVEL=1; fi
if [ -f wallet.dat -a -f peers.dat -a -f blkindex.dat -a -f blk0001.dat ]; then LEVEL=2; fi
if [ -f wallet.dat -a -f peers.dat -a -f coins/CURRENT -a -f blktree/CURRENT -a -f blocks/blk00000.dat ]; then LEVEL=3; fi
if [ -f wallet.dat -a -f peers.dat -a -f chainstate/CURRENT -a -f blocks/index/CURRENT -a -f blocks/blk00000.dat ]; then LEVEL=4; fi

case $LEVEL in
  0)
    echo "Error: no Memorycoin datadir detected."
    exit 1
    ;;
  1)
    echo "Detected old Memorycoin datadir (before 0.7)."
    echo "Nothing to do."
    exit 0
    ;;
  2)
    echo "Detected Memorycoin 0.7 datadir."
    ;;
  3)
    echo "Detected Memorycoin pre-0.8 datadir."
    ;;
  4)
    echo "Detected Memorycoin 0.8 datadir."
    ;;
esac

FILES=""
DIRS=""

if [ $LEVEL -ge 3 ]; then FILES=$(echo $FILES blk????.dat blkindex.dat); fi
if [ $LEVEL -ge 2 ]; then FILES=$(echo $FILES addr.dat); fi
if [ $LEVEL -ge 4 ]; then DIRS=$(echo $DIRS coins blktree); fi

for FILE in $FILES; do
  if [ -f $FILE ]; then
    echo "Deleting: $FILE"
    rm -f $FILE
  fi
done

for DIR in $DIRS; do
  if [ -d $DIR ]; then
    echo "Deleting: $DIR/"
    rm -rf $DIR
  fi
done

echo "Done."
