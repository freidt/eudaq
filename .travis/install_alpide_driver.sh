#!/bin/bash

# This package is necessary for the alpide producer

echo "Entering install_alpide_driver"
echo "Installing alpide driver:"

export temporary_path=`pwd`

echo $temporary_path

cd $TRAVIS_BUILD_DIR/extern/

if [ $TRAVIS_OS_NAME == linux ]; then

	sudo apt-get update && sudo apt-get install -y libtinyxml-dev expect-dev libusb-1.0-0-dev;

	if [ -d "$TRAVIS_BUILD_DIR/extern/new-alpide-software" ]; then

		echo "new-alpide-software source restored from cache as path exists:"

		zip -r new-alpide-software.zip new-alpide-software

	else
		echo "new-alpide-software source not restored from cache - downloading from CERNBOX and unpacking"

        mkdir new-alpide-software

		wget -O new-alpide-software.zip https://cernbox.cern.ch/index.php/s/TN3BQ8FsOtocpP6/download

		unzip new-alpide-software.zip

		mv new-alpide-software.zip ../
	fi

	echo `ls `

	cd new-alpide-software
	make lib

fi

pwd

cd $temporary_path

echo "Installed ALPIDE driver"
