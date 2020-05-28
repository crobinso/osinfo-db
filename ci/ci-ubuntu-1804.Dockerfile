FROM ubuntu:18.04

RUN export DEBIAN_FRONTEND=noninteractive && \
    apt-get update && \
    apt-get dist-upgrade -y && \
    apt-get install --no-install-recommends -y \
            autoconf \
            automake \
            autopoint \
            bash \
            bash-completion \
            ca-certificates \
            ccache \
            chrony \
            gcc \
            gdb \
            gettext \
            git \
            intltool \
            itstool \
            libarchive-dev \
            libc6-dev \
            libglib2.0-dev \
            libjson-glib-dev \
            libsoup2.4-dev \
            libtool \
            libtool-bin \
            libxml2-dev \
            libxslt1-dev \
            locales \
            lsof \
            make \
            net-tools \
            ninja-build \
            patch \
            perl \
            pkgconf \
            python3 \
            python3-lxml \
            python3-pip \
            python3-pytest \
            python3-requests \
            python3-setuptools \
            python3-wheel \
            screen \
            strace \
            sudo \
            vim && \
    apt-get autoremove -y && \
    apt-get autoclean -y && \
    sed -Ei 's,^# (en_US\.UTF-8 .*)$,\1,' /etc/locale.gen && \
    dpkg-reconfigure locales && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/$(basename /usr/bin/gcc)

RUN pip3 install \
         meson==0.49.0

ENV LANG "en_US.UTF-8"

ENV MAKE "/usr/bin/make"
ENV NINJA "/usr/bin/ninja"
ENV PYTHON "/usr/bin/python3"

ENV CCACHE_WRAPPERSDIR "/usr/libexec/ccache-wrappers"
