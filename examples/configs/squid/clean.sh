#!/bin/bash

if [ $1 ]
then
  echo $1
  CURR_DIR=`pwd`
  fullpath=$CURR_DIR/$1
  if [ -d $fullpath ];then
    rm -rf $fullpath
    echo "$fullpath is removed"
    mkdir $1
  else 
    echo "the full directory is not valid directory"
  fi
else 
  echo "you have to choose a subdirectory under `pwd`"
fi
