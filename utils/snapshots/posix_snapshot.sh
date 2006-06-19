#!/bin/bash
set -e # abort immediately if something fails
DATE=`date +'%Y%m%d'`
SYSTEM=`uname -s | sed -e 's/MINGW.*/Mingw/g'`

get_source() {
    cvs co freecnc++
    cd freecnc++
    find -name CVS -exec rm -r {} \;
    find -iname .cvsignore -exec rm {} \;
    cd ..
}

binary_snap() {
    make -C freecnc++ RELEASE=1
    echo "Packaging binary snapshot"
    tar czf freecnc-$DATE-$SYSTEM.tgz --exclude doc/tech freecnc++/freecnc freecnc++/*.txt freecnc++/doc freecnc++/data
    make -C freecnc++ cleaner
    echo -n "Binary snapshot finished.  NOTE: "
    g++ --version | head -1
}

source_snap() {
    echo "Packaging source snapshot:"
    tar czf freecnc-$DATE-source.tgz freecnc++
    echo done
}

if [ ! -d freecnc++ ]; then
    get_source
fi

source_snap

binary_snap

rm -rf freecnc++
