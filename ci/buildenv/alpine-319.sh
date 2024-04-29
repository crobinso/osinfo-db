# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

function install_buildenv() {
    apk update
    apk upgrade
    apk add \
        ca-certificates \
        ccache \
        gcc \
        gettext \
        git \
        glib-dev \
        json-glib-dev \
        libarchive-dev \
        libsoup-dev \
        libxml2-dev \
        libxslt-dev \
        make \
        meson \
        musl-dev \
        perl \
        pkgconf \
        py3-lxml \
        py3-pytest \
        py3-requests \
        python3 \
        samurai \
        xz
    rm -f /usr/lib*/python3*/EXTERNALLY-MANAGED
    apk list --installed | sort > /packages.txt
    mkdir -p /usr/libexec/ccache-wrappers
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/gcc
}

export CCACHE_WRAPPERSDIR="/usr/libexec/ccache-wrappers"
export LANG="en_US.UTF-8"
export MAKE="/usr/bin/make"
export NINJA="/usr/bin/ninja"
export PYTHON="/usr/bin/python3"
