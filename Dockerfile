# Base system
FROM ubuntu:24.04

# Install build tools
RUN apt-get update && apt-get install -y build-essential make git

# Create working directory
WORKDIR /engine

# Copy project
COPY . .

# Build engine
RUN make

# Default command
CMD ["bash"]
