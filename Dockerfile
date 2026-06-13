FROM python:3.11.9-slim-bookworm

ARG PLATFORMIO_VERSION=6.1.19

ENV DEBIAN_FRONTEND=noninteractive
ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8
ENV PLATFORMIO_CORE_DIR=/opt/platformio
ENV PLATFORMIO_CORE_VERSION=${PLATFORMIO_VERSION}

RUN apt-get update \
  && apt-get install -y --no-install-recommends ca-certificates git openssh-client \
  && rm -rf /var/lib/apt/lists/*

RUN pip install --no-cache-dir platformio==${PLATFORMIO_VERSION}

WORKDIR /work
