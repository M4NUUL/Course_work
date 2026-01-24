# Базовый образ
FROM ubuntu:24.04

# Установка зависимостей
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
    xdg-utils \
    gedit \
    postgresql-client \
    && rm -rf /var/lib/apt/lists/*


# Рабочая директория
WORKDIR /app

# Копируем проект внутрь контейнера
COPY . /app

# Чистая сборка
RUN rm -rf build && mkdir -p build && cd build && cmake .. && make -j$(nproc)

# Настройки Qt для работы через X11
ENV QT_QPA_PLATFORM=xcb

# Ожидание готовности БД и запуск GUI
CMD bash -lc 'until pg_isready -h db -p 5432 -U jumandgi; do echo "Waiting for db..."; sleep 1; done; ./build/Jumandgi'
