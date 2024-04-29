# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

FROM docker.io/library/alpine:3.18

RUN apk update && \
    apk upgrade && \
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
        xz && \
    rm -f /usr/lib*/python3*/EXTERNALLY-MANAGED && \
    apk list --installed | sort > /packages.txt && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/gcc

ENV CCACHE_WRAPPERSDIR "/usr/libexec/ccache-wrappers"
ENV LANG "en_US.UTF-8"
ENV MAKE "/usr/bin/make"
ENV NINJA "/usr/bin/ninja"
ENV PYTHON "/usr/bin/python3"
