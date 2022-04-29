FROM ubuntu:latest

COPY . /plotter
WORKDIR /plotter

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Chicago
RUN apt-get update -y
RUN apt-get install -y libsodium-dev cmake g++ git build-essential

RUN ./make_devel.sh
RUN cp -r build/ /usr/lib/chia-plotter
RUN ln -s /usr/lib/chia-plotter/chia_plot /usr/bin/chia_plot

ENTRYPOINT ["/usr/bin/chia_plot"]


