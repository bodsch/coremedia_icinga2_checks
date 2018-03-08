FROM alpine:latest

MAINTAINER Bodo Schulz <bodo@boone-schulz.de>

ENV \
  TERM=xterm \
  BUILD_DATE="2018-03-06"

RUN \
  apk --no-cache update && \
  apk --no-cache upgrade

RUN \
  apk add --quiet --no-cache \
    bash \
    build-base \
    bison \
    cmake \
    curl \
    flex \
    git \
    hiredis-dev \
    libev-dev

RUN \
  cd /tmp/ && \
  git clone https://github.com/bodsch/coremedia_icinga2_checks.git && \
  git clone https://github.com/hmartiro/redox.git && \
  git clone https://github.com/nlohmann/json

RUN \
  cd /tmp/redox && \
  cmake -H. -Bbuild && \
  cmake --build build -- -j3 && \
  cd build && \
  make install

RUN \
  cd /tmp/json && \
  cp -ar /tmp/json/include/nlohmann /usr/local/include/

RUN \
  cd /tmp/coremedia_icinga2_checks && \
  cmake -H. -Bbuild && \
  cmake --build build -- -j3

RUN \
  apk del --quiet \
    build-base \
    bison \
    cmake \
    curl \
    flex \
    git \
    hiredis-dev \
    libev-dev

CMD [ "/bin/bash" ]

# EOF
