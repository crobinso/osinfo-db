FROM registry.opensuse.org/opensuse/leap:15.1

RUN zypper update -y && \
    zypper install -y \
           ca-certificates \
           ccache \
           gcc \
           gettext \
           git \
           glib2-devel \
           glibc-locale \
           intltool \
           itstool \
           json-glib-devel \
           libarchive-devel \
           libsoup-devel \
           libxml2-devel \
           libxslt-devel \
           make \
           ninja \
           pkgconfig \
           python3 \
           python3-lxml \
           python3-pip \
           python3-pytest \
           python3-requests \
           python3-setuptools \
           python3-wheel \
           rpm-build \
           xz && \
    zypper clean --all && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/$(basename /usr/bin/gcc)

RUN pip3 install \
         meson==0.54.0

ENV LANG "en_US.UTF-8"
ENV MAKE "/usr/bin/make"
ENV NINJA "/usr/bin/ninja"
ENV PYTHON "/usr/bin/python3"
ENV CCACHE_WRAPPERSDIR "/usr/libexec/ccache-wrappers"
