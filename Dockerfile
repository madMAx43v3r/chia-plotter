# Compile image
# -------------------------------------------------------------------------------------------------
FROM alpine:3.13.5 AS compile-stage

WORKDIR /root

RUN apk update && \
  apk upgrade && \
  apk --update add \
    gcc \
    g++ \
    build-base \
    cmake \
    gmp-dev \
    libsodium-dev \
    libsodium-static \
    git

COPY . .
RUN /bin/sh ./make_devel.sh

# Runtime image
# -------------------------------------------------------------------------------------------------
FROM alpine:3.13.5

WORKDIR /root

RUN apk update && \
  apk upgrade && \
  apk --update add \
    gmp-dev

COPY --from=compile-stage /root/build /usr/lib/chia-plotter
RUN ln -s /usr/lib/chia-plotter/chia_plot /usr/bin/chia_plot

ENTRYPOINT chia_plot
