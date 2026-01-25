FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    qtbase5-dev \
    qtbase5-dev-tools \
    qt5-qmake \
    libqt5sql5-psql \
    libpq-dev \
    libssl-dev \
    libsodium-dev \
    pkg-config \
    git \
    ca-certificates \
    postgresql-client \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . /app

RUN rm -rf build && mkdir -p build && cd build && cmake .. && make -j$(nproc)

ENV QT_QPA_PLATFORM=xcb

CMD ["./build/EduDesk"]
