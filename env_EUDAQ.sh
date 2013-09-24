#!/bin/sh

username=hipex

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

export PATH=/home/${username}/scripts:/usr/sue/bin:/usr/lib64/qt4/bin:/usr/local/bin:/bin:/usr/bin:/usr/local/sbin:/usr/sbin:/sbin:/home/$usernamehipex/bin


#here you may have to add other files depending on version and pc setup
#this is for a local account with slc6 and afs access
source /opt/root/v5.34.09/bin/thisroot.sh


# add local library
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/hipex/itsUp/$DIR/bin

# LCIO
#export LCIO=/data/freidt/lcio/v02-04-02
#export PATH="$LCIO/tools:$LCIO/bin:$PATH"
#export LD_LIBRARY_PATH="$LCIO/lib:$LD_LIBRARY_PATH"
