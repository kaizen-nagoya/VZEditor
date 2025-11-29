FROM --platform=linux/arm64 debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential clang git cmake curl ca-certificates pkg-config \
    libncurses5-dev libncursesw5-dev lld llvm && \
    apt-get clean && rm -rf /var/lib/apt/lists/*

# Install Zig (used as an easy cross-compiler for macOS aarch64 target)
RUN curl -L https://ziglang.org/download/0.11.0/zig-linux-aarch64-0.11.0.tar.xz | tar xJf - -C /opt && \
    ln -s /opt/zig-linux-aarch64-0.11.0/zig /usr/local/bin/zig

WORKDIR /workspace

CMD ["/bin/bash"]
