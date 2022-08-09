#!/bin/sh

set -e
set -v

INSTALL_ROOT="$PWD/install"
RPMBUILD_ROOT="$PWD/rpmbuild"

# If the MAKEFLAGS envvar does not yet include a -j option,
# add -jN where N depends on the number of processors.
case $MAKEFLAGS in
  *-j*) ;;
  *) n=$(getconf _NPROCESSORS_ONLN 2> /dev/null)
    test "$n" -gt 0 || n=1
    n=$(expr $n + 1)
    MAKEFLAGS="$MAKEFLAGS -j$n"
    export MAKEFLAGS
    ;;
esac

# Make things clean.
make clean
rm -rf "$INSTALL_ROOT"

make
OSINFO_DB_NETWORK_TESTS=1 make check
make install DESTDIR="$INSTALL_ROOT" OSINFO_DB_TARGET="--system"

if test -x /usr/bin/rpmbuild; then
  rpmbuild --nodeps \
     --define "_topdir $RPMBUILD_ROOT" \
     --define "_sourcedir `pwd`" \
     -ba --clean osinfo-db.spec
fi

