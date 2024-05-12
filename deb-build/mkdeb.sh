#!/bin/bash

VERSION=$(date +%Y%m%d%H%M)

rsync -avpx --delete $(dirname $0)/ deb-build/
cp -f bin/* deb-build/build/usr/bin/

if [ "$(which dpkg-architecture)" ==  "" ]; then
    echo "dpkg-architekture is missing ... probably build-essential package"
    exit 1
fi

ARCH=$(dpkg-architecture | grep DEB_BUILD_ARCH= | cut -f2 -d=)
UBUNTU=$(lsb_release -c | cut -f2 -d: | tr -d " \t")

echo $(pwd)

pushd deb-build
echo START BUILDING DEB PACKAGE
unset LD_PRELOAD
unset LD_LIBRARY_PATH

echo GENERATING REPENDENCY
DEPENDS=$(./libs.sh)
echo $DEPENDS
echo PREPARING FILES
find build/ -type f | grep -v DEBIAN | xargs md5sum | sed "s/deb.build.//" >build/DEBIAN/md5sums
cat templates/control | sed "s/#VERSION#/$VERSION/" | sed "s/#SIZE#/$(du -s build | cut -f1)/" | sed "s/#ARCH#/$ARCH/" | sed "s/#DEPENDS#/$DEPENDS/" >build/DEBIAN/control
echo BUILDING PACKAGE
dpkg-deb --build -Zzstd -z7 build

NAME=$(cat build/DEBIAN/control | grep Package | cut -f2 -d: | tr -d " \t")
echo REMOVE OLD FILES
rm -f ../*deb
echo put new files to ther places
mv build.deb "../$NAME-$ARCH-$VERSION.deb"
popd
