# ========================
# Stage 1: Build
# ========================
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential cmake git pkg-config \
    libboost-system-dev \
    libssl-dev \
    libpq-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /deps

# --- libpqxx (C++ PostgreSQL client) ---
RUN git clone --branch 7.9.2 --depth 1 https://github.com/jtv/libpqxx.git && \
    cd libpqxx && mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DSKIP_BUILD_TEST=ON -DBUILD_SHARED_LIBS=ON && \
    cmake --build . -j$(nproc) && cmake --install . && \
    cd /deps && rm -rf libpqxx

# --- jwt-cpp ---
RUN git clone --branch v0.7.0 --depth 1 https://github.com/Thalhammer/jwt-cpp.git && \
    cd jwt-cpp && mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DJWT_BUILD_EXAMPLES=OFF -DJWT_BUILD_TESTS=OFF -DJWT_DISABLE_PICOJSON=ON && \
    cmake --build . -j$(nproc) && cmake --install . && \
    cd /deps && rm -rf jwt-cpp

WORKDIR /app
COPY . .

RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . -j$(nproc)

# Generate a baseline migration from models so fresh databases can bootstrap tables.
RUN mkdir -p /app/migrations && /app/build/server_chinese_chess add-migration auto_init

# ========================
# Stage 2: Runtime
# ========================
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libboost-system1.74.0 \
    libssl3 \
    libpq5 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Copy libpqxx shared library from builder
COPY --from=builder /usr/local/lib/libpqxx* /usr/local/lib/
RUN ldconfig

WORKDIR /app

COPY --from=builder /app/build/server_chinese_chess .
COPY --from=builder /app/migrations ./migrations/

# Default environment
ENV DB_HOST=db \
    DB_PORT=5432 \
    DB_USER=postgres \
    DB_PASS=postgres \
    DB_NAME=chess_db \
    WS_PORT=8080 \
    HTTP_PORT=8081

EXPOSE 8080 8081

CMD ["./server_chinese_chess"]
