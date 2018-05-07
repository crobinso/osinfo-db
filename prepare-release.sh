#!/bin/sh

set -e
set -v

test -n "$1" && RESULTS=$1 || RESULTS=results.log
INSTALL_ROOT=$HOME/builder

# Make things clean.
make -k clean

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

make
make install DESTDIR="$INSTALL_ROOT" OSINFO_DB_TARGET="--system"

if [ -f /usr/bin/rpmbuild ]; then
  rpmbuild --nodeps \
     --define "_sourcedir `pwd`" \
     -ba --clean osinfo-db.spec
fi

if test -x /usr/bin/i686-w64-mingw32-gcc && test -x /usr/bin/x86_64-w64-mingw32-gcc ; then
  if test -f /usr/bin/rpmbuild ; then
    rpmbuild --nodeps \
       --define "_sourcedir `pwd`" \
       -ba --clean mingw-osinfo-db.spec
  fi
fi
