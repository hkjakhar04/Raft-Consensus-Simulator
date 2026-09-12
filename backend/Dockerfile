FROM ubuntu:22.04

# Install build dependencies
RUN apt-get update && apt-get install -y g++ make libasio-dev

# Set working directory
WORKDIR /app

# Copy the backend source code
COPY . .

# Compile the C++ backend
RUN g++ -I./src -o raft_simulator src/main.cpp src/raft_core/Node.cpp -lpthread

# Expose the WebSocket port
EXPOSE 8080

# Run the server
CMD ["./raft_simulator"]
