FROM debian:12-slim

WORKDIR /app

RUN apt-get update && apt-get install -y gcc-multilib make

COPY . .

RUN CONFIG_X86_32=1 CONFIG_SOFTFLOAT=1 make mqjs_compile
