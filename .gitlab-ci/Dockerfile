FROM fedora:42

# NOTE: We set tsflags to empty so docs actually install...
RUN dnf update -y \
    && dnf install --setopt=tsflags='' -y \
                      gcc \
                      gcovr \
                      gettext \
                      gi-docgen \
                      git \
                      glib2-devel \
                      gobject-introspection-devel \
                      graphviz \
                      gtksourceview5-devel \
                      json-glib-devel \
                      lcov \
                      libabigail \
                      libadwaita-devel \
                      libasan \
                      libsoup3-devel \
                      libxml2-devel \
                      meson \
                      ninja-build \
                      python3 \
                      python3-jinja2 \
                      python3-packaging \
                      python3-pip \
                      python3-pygments \
                      python3-toml \
                      python3-typogrify \
                      python3-wheel \
                      redhat-rpm-config \
                      vala \
                      valgrind \
    && dnf clean all

RUN git clone https://gitlab.freedesktop.org/hadess/check-abi \
    && pushd check-abi \
    && meson . _build --prefix=/usr \
    && ninja -C _build install \
    && popd \
    && rm -rf check-abi

ARG HOST_USER_ID=5555
ENV HOST_USER_ID ${HOST_USER_ID}
RUN useradd -u $HOST_USER_ID -ms /bin/bash user

USER user
WORKDIR /home/user

ENV LANG C.UTF-8
