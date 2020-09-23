FROM debian:10

LABEL maintainer="jarle@jgaa.com"

RUN DEBIAN_FRONTEND="noninteractive" apt-get -q update &&\
    DEBIAN_FRONTEND="noninteractive" apt-get -y -q --no-install-recommends upgrade &&\
    DEBIAN_FRONTEND="noninteractive" apt-get install -y -q libssl1.1 libgrpc++1

COPY ./rated /usr/bin
CMD rated
