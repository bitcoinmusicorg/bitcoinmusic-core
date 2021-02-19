# Get Ubuntu 16.04
FROM phusion/baseimage:0.10.1 AS build

# Set Variables
ENV DATADIR "/data"
ENV LANG=en_US.UTF-8

# Make Ports Available
EXPOSE 1027
EXPOSE 1028
EXPOSE 1029

RUN mkdir -p "${DATADIR}" /usr/local/bin

# EntryPoint for Config
CMD [ "/entrypoint.sh" ]

RUN \
    apt-get update -y && \
    apt-get install -y \
      g++ \
      autoconf \
      cmake \
      git \
      libbz2-dev \
      libcurl4-openssl-dev \
      libssl-dev \
      libncurses-dev \
      libboost-thread-dev \
      libboost-iostreams-dev \
      libboost-date-time-dev \
      libboost-system-dev \
      libboost-filesystem-dev \
      libboost-program-options-dev \
      libboost-chrono-dev \
      libboost-test-dev \
      libboost-context-dev \
      libboost-regex-dev \
      libboost-coroutine-dev \
      libtool \
      doxygen \
      ca-certificates \
      fish \
    && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

ADD . /btcm-Source
WORKDIR /btcm-Source

# Compile
RUN \
    ( git submodule sync --recursive || \
      find `pwd`  -type f -name .git | \
        while read f; do \
          rel="$(echo "${f#$PWD/}" | sed 's=[^/]*/=../=g')"; \
          sed -i "s=: .*/.git/=: $rel/=" "$f"; \
        done && \
      git submodule sync --recursive ) && \
    git submodule update --init --recursive && \
    mkdir -p _build && \
    cd _build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        .. && \
    make btcmd cli_wallet get_dev_key && \
    install -s programs/btcmd/btcmd programs/cli_wallet/cli_wallet programs/util/get_dev_key /usr/local/bin && \
    cd .. && \
    install -d /etc/BTCM && \
    install -m 0644 Docker/config.ini /etc/BTCM/ && \
    install -m 0755 Docker/entrypoint.sh / && \
    #
    # Obtain version
    git rev-parse --short HEAD > /etc/BTCM/version && \
    rm -rf _build

#
# Build smaller runtime image
#
FROM phusion/baseimage:0.10.1

# Set Variables
ENV DATADIR "/data"
ENV LANG=en_US.UTF-8

# Make Ports Available
EXPOSE 1027
EXPOSE 1028
EXPOSE 1029

RUN mkdir -p "${DATADIR}" /usr/local/bin /etc/BTCM

# EntryPoint for Config
CMD [ "/entrypoint.sh" ]

COPY --from=build /entrypoint.sh /entrypoint.sh
COPY --from=build /etc/BTCM/config.ini /etc/BTCM/config.ini
COPY --from=build /etc/BTCM/version /etc/BTCM/version
COPY --from=build /usr/local/bin/cli_wallet /usr/local/bin/cli_wallet
COPY --from=build /usr/local/bin/get_dev_key /usr/local/bin/get_dev_key
COPY --from=build /usr/local/bin/btcmd /usr/local/bin/btcmd
