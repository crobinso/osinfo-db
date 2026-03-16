# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

FROM quay.io/centos/centos:stream9

RUN dnf --quiet distro-sync -y && \
    dnf --quiet install 'dnf-command(config-manager)' -y && \
    dnf --quiet config-manager --set-enabled -y crb && \
    dnf --quiet install -y epel-release && \
    dnf --quiet install -y epel-next-release && \
    dnf --quiet install -y \
                ca-certificates \
                ccache \
                gcc \
                gettext \
                git \
                glib2-devel \
                glibc-devel \
                glibc-langpack-en \
                json-glib-devel \
                libarchive-devel \
                libsoup-devel \
                libxml2-devel \
                make \
                meson \
                ninja-build \
                perl-podlators \
                pkgconfig \
                python3 \
                python3-lxml \
                python3-pytest \
                python3-requests \
                rpm-build \
                xz && \
    dnf --quiet autoremove -y && \
    dnf --quiet clean all -y && \
    rm -f /usr/lib*/python3*/EXTERNALLY-MANAGED && \
    rpm -qa | sort > /packages.txt && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/gcc

ENV CCACHE_WRAPPERSDIR="/usr/libexec/ccache-wrappers"
ENV LANG="en_US.UTF-8"
ENV MAKE="/usr/bin/make"
ENV NINJA="/usr/bin/ninja"
ENV PYTHON="/usr/bin/python3"
