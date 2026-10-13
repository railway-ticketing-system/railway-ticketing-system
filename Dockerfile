FROM ubuntu:24.04 AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
      cmake g++ make libssl-dev ca-certificates && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DRTRS_BUILD_TESTS=OFF \
 && cmake --build build -j"$(nproc)" --target rtrs_server

FROM ubuntu:24.04
RUN apt-get update && apt-get install -y --no-install-recommends libssl3t64 curl && rm -rf /var/lib/apt/lists/* \
 && useradd -r -u 10001 rtrs && mkdir /data && chown rtrs /data
WORKDIR /app
COPY --from=build /src/build/rtrs_server /app/rtrs_server
COPY web /app/web
USER rtrs
ENV RTRS_DB=/data/rtrs.db RTRS_PORT=8080 RTRS_WEB_DIR=/app/web
EXPOSE 8080
VOLUME /data
CMD ["/app/rtrs_server"]
