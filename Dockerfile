# Stage 1: Build C++ Sovereign Solver CLI Binary
FROM ubuntu:22.04 AS builder

# Prevent interactive prompts during apt install
ENV DEBIAN_FRONTEND=noninteractive

# Install C++20 compiler tools and CMake
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build_src

# Copy C++ engine source files and CMake build configuration
COPY CMakeLists.txt .
COPY include/ ./include/
COPY src/ ./src/
COPY apps/ ./apps/
COPY tests/ ./tests/

# Configure and build the workbench_cli executable target in Release mode
RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --target workbench_cli -j$(nproc)

# Stage 2: Production Runtime Image (Node.js Express + Sovereign Solver Engine)
FROM node:20-slim

# Install minimal C++ runtime libraries and curl for health check
RUN apt-get update && apt-get install -y \
    libstdc++6 \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Create directory structure for C++ binary
RUN mkdir -p /app/bin

# Copy compiled C++ solver executable from builder stage
COPY --from=builder /build_src/build/workbench_cli /app/bin/workbench_cli
RUN chmod +x /app/bin/workbench_cli

# Setup Express backend working directory
WORKDIR /app/workbench/backend
COPY workbench/backend/package*.json ./
RUN npm ci --only=production || npm install --production

# Copy backend application source code
COPY workbench/backend/ ./

# Environment defaults for container execution
ENV PORT=3001
ENV HOST=0.0.0.0
ENV SOVEREIGN_SOLVER_PATH=/app/bin/workbench_cli

EXPOSE 3001

# Health check instruction
HEALTHCHECK --interval=10s --timeout=3s --start-period=5s --retries=3 \
  CMD curl -f http://localhost:3001/api/health || exit 1

# Start Node backend REST API
CMD ["node", "server.js"]
