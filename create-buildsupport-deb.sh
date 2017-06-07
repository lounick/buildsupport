#!/bin/bash
mkdir -p deb/buildsupport/DEBIAN
mkdir -p deb/buildsupport/usr/bin
echo 'Package: buildsupport
Version: 1.0
Section: base
Priority: optional
Architecture: all
Depends: libgnat-4.9
Maintainer: Maxime Perrotin
Description: Taste low-level command for code generation
Homepage: http://taste.tuxfamily.org' > deb/buildsupport/DEBIAN/control
cp buildsupport deb/buildsupport/usr/bin
cd deb
dpkg-deb --build buildsupport
mv buildsupport.deb ..
cd ..
rm -rf deb

