# --- Stage 1: The Builder Stage ---
ARG BUILDPLATFORM=linux/amd64
ARG JOBS

FROM --platform=${BUILDPLATFORM} ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
ENV QTSDK_DIR=/opt/Qt
ENV EMSDK_DIR=/opt/emsdk

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    libgl-dev \
    libgl1-mesa-dev \
    libglib2.0-0 \
    libxkbcommon-dev \
    ninja-build \
    python3 \
    python3-pip \
    unzip \
    wget && \
    rm -rf /var/lib/apt/lists/*

RUN pip3 install --break-system-packages aqtinstall && \
    aqt install-qt --outputdir $QTSDK_DIR linux desktop 6.10.3 linux_gcc_64 -m qtwebsockets qtmultimedia && \
    aqt install-qt --outputdir $QTSDK_DIR all_os wasm 6.10.3 wasm_multithread -m qtwebsockets qtmultimedia

WORKDIR $EMSDK_DIR
RUN git clone --depth 1 https://github.com/emscripten-core/emsdk.git . && \
    ./emsdk install 4.0.7

ENV PATH="$EMSDK_DIR:$PATH"
ENV PATH="$QTSDK_DIR/6.10.3/gcc_64/bin:$PATH"
ENV PATH="$QTSDK_DIR/6.10.3/wasm_multithread/bin:$PATH"

COPY . /app

RUN ./emsdk activate 4.0.7 && \
    . $EMSDK_DIR/emsdk_env.sh && \
    qt-cmake -S /app -B /build \
        -DQT_HOST_PATH=$QTSDK_DIR/6.10.3/gcc_64 \
        -DCMAKE_BUILD_TYPE=Release \
        -DWITH_OPENSSL=OFF -DWITH_TESTS=OFF -DWITH_WEBSOCKET=ON -DWITH_UPDATER=OFF -DPACKAGE_TYPE=Wasm && \
    ACTUAL_JOBS=${JOBS:-$(nproc)} && \
    cmake --build /build --parallel ${ACTUAL_JOBS} && \
    ls /build/src && \
    mkdir /dist && \
    cd /build/src && \
    cp mmapper.js /dist/ && \
    cp mmapper.wasm /dist/ && \
    cp qtloader.js /dist/ && \
    cp mmapper.html /dist/index.html && \
    cp /app/src/resources/win32/m-release.ico /dist/favicon.ico && \
    cp /app/src/resources/icons/m-release.png /dist/logo.png && \
    cd /dist && \
    sed -i 's|<head>|<head>\n    <link rel="icon" type="image/x-icon" href="favicon.ico">|' index.html && \
    sed -i 's|src="qtlogo.svg" width="320" height="200"|src="logo.png"|g' index.html && \
    wget https://raw.githubusercontent.com/gzuidhof/coi-serviceworker/refs/heads/master/coi-serviceworker.js && \
    sed -i 's|<head>|<head>\n    <script src="./coi-serviceworker.js"></script>|' index.html

# --- Stage 2: The Final Runtime Stage (Nginx) ---
FROM nginx:alpine

WORKDIR /usr/share/nginx/html
RUN rm -rf ./*
COPY --from=builder /dist/* .

EXPOSE 80

CMD ["nginx", "-g", "daemon off;"]
