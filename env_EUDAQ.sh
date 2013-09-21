#!/bin/sh

#here you may have to add other files depending on version and pc setup
#this is for a local account with slc6 and afs access
source /opt/root/v5.34.09/bin/thisroot.sh


# add local library
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/data/freidt/git/eudaq-official/bin

# LCIO
export LCIO=/data/freidt/lcio/v02-04-02
export PATH="$LCIO/tools:$LCIO/bin:$PATH"
export LD_LIBRARY_PATH="$LCIO/lib:$LD_LIBRARY_PATH"
