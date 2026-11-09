FROM python:3.12-slim-bookworm

ENV PYTHONBUFFERED=1

# Install required packages and tools
RUN apt-get update && apt-get install -y --no-install-recommends \
    curl \
    bash \
    ca-certificates \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=/usr/bin sh

RUN pip3 install --no-cache --upgrade pip setuptools pyserial esptool

# Older x86 toolchains need the 32-bit loader. arm64 images use the native toolchain.
RUN if [ "$(dpkg --print-architecture)" = "amd64" ]; then \
      apt-get update && apt-get install -y --no-install-recommends lib32stdc++6 \
      && apt-get clean && rm -rf /var/lib/apt/lists/*; \
    fi

# Install m5stack boards
COPY ./package_m5stack_index.json /package_m5stack_index.json
RUN arduino-cli core install m5stack:esp32 --additional-urls "file:///package_m5stack_index.json" --log-level warn --verbose

RUN arduino-cli core search m5stack
RUN arduino-cli board listall

ARG LIBRARIES 

# Install required libraries
RUN arduino-cli lib install $LIBRARIES --log-level warn --verbose

COPY ./scripts/compile.sh /compile.sh
RUN chmod +x /compile.sh

COPY ./scripts/entrypoint.sh ./entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
