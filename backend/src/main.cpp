#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include <unordered_set>
#include <mutex>
#include <random>
#include <sstream>

// If Crow is not installed globally, you will need to add it to your CMakeLists.txt via FetchContent.
#include "crow.h"

#include "raft_core/Node.h"
#include "telemetry/ThreadSafeQueue.h"
#include "telemetry/TelemetryTypes.h"

using namespace raft;

// Helper to simulate network latency (10-25ms)
void simulateNetworkLatency() {
    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> distribution(10, 25);
    std::this_thread::sleep_for(std::chrono::milliseconds(distribution(generator)));
}

// Convert EventType to String for JSON formatting
std::string eventTypeToString(telemetry::EventType type) {
    switch (type) {
        case telemetry::EventType::STATE_TRANSITION: return "STATE_TRANSITION";
        case telemetry::EventType::ELECTION_STARTED: return "ELECTION_STARTED";
        case telemetry::EventType::VOTE_CAST: return "VOTE_CAST";
        default: return "UNKNOWN";
    }
}

int main() {
    std::cout << "Initializing Raft Cluster Orchestrator...\n";

    // 1. Instantiate the Global Telemetry Queue
    telemetry::ThreadSafeQueue<telemetry::RaftStateEvent> telemetryQueue;

    // 2. Instantiate 5 Nodes (IDs 1 to 5)
    const int numNodes = 5;
    std::vector<std::shared_ptr<Node>> nodes;
    
    // We create IDs 1 through 5.
    for (int i = 1; i <= numNodes; ++i) {
        std::vector<int> peers;
        for (int j = 1; j <= numNodes; ++j) {
            if (i != j) peers.push_back(j);
        }
        nodes.push_back(std::make_shared<Node>(i, peers));
    }

    // 3. Wire RPC handlers and Telemetry Callbacks
    for (int i = 0; i < numNodes; ++i) {
        auto requestVoteHandler = [&nodes](int targetId, const RequestVoteArgs& args) {
            simulateNetworkLatency(); // Simulating real-world TCP delay
            // TargetId is 1-based, our vector is 0-based
            return nodes[targetId - 1]->requestVote(args);
        };
        
        auto appendEntriesHandler = [&nodes](int targetId, const AppendEntriesArgs& args) {
            simulateNetworkLatency(); // Simulating real-world TCP delay
            return nodes[targetId - 1]->appendEntries(args);
        };
        
        nodes[i]->setRpcHandler(requestVoteHandler, appendEntriesHandler);

        // Wire the node to push events into our global queue
        nodes[i]->setTelemetryCallback([&telemetryQueue](const telemetry::RaftStateEvent& event) {
            telemetryQueue.push(event);
        });
    }

    // 4. WebSocket Connected Clients Set & Mutex
    std::mutex clientsMutex;
    std::unordered_set<crow::websocket::connection*> connectedClients;

    // 5. The Consumer Loop (Background Thread)
    std::thread consumerThread([&telemetryQueue, &clientsMutex, &connectedClients]() {
        while (true) {
            telemetry::RaftStateEvent event;
            // Blocks efficiently until an event is pushed
            telemetryQueue.wait_and_pop(event);

            // Format into JSON manually
            std::ostringstream json;
            json << "{\"nodeId\": " << event.nodeId 
                 << ", \"type\": \"" << eventTypeToString(event.type) << "\""
                 << ", \"term\": " << event.currentTerm 
                 << ", \"role\": " << static_cast<int>(event.role) 
                 << ", \"commitIndex\": " << event.commitIndex
                 << ", \"isCrashed\": " << (event.isCrashed ? "true" : "false")
                 << ", \"message\": \"" << event.message << "\"";
                 
            json << ", \"log\": [";
            for (size_t i = 0; i < event.logEntries.size(); ++i) {
                json << "{\"term\": " << event.logEntries[i].term << ", \"command\": \"" << event.logEntries[i].command << "\"}";
                if (i < event.logEntries.size() - 1) json << ",";
            }
            json << "]}";
            
            std::string payload = json.str();

            // Broadcast to all connected clients
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (auto* client : connectedClients) {
                client->send_text(payload);
            }
        }
    });
    // Detach so it runs continuously in the background
    consumerThread.detach();

    // 6. Start the Raft Nodes
    for (auto& node : nodes) {
        node->start();
    }

    // 7. Initialize Crow App and WebSocket Route
    crow::SimpleApp app;

    CROW_WEBSOCKET_ROUTE(app, "/raft")
        .onopen([&](crow::websocket::connection& conn) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            connectedClients.insert(&conn);
            std::cout << "Client connected to telemetry stream!" << std::endl;
            // Force the nodes to broadcast their current state to the new client
            for (auto& node : nodes) {
                node->forceSyncTelemetry();
            }
        })
        .onclose([&](crow::websocket::connection& conn, const std::string& reason) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            connectedClients.erase(&conn);
            std::cout << "Client disconnected." << std::endl;
        })
        .onmessage([&nodes](crow::websocket::connection& /*conn*/, const std::string& data, bool is_binary) {
            auto parsed = crow::json::load(data);
            if (!parsed) return;
            
            if (parsed.has("command")) {
                std::string command = parsed["command"].s();
                
                if (command == "CLIENT_REQUEST" && parsed.has("data")) {
                    std::string dataStr = parsed["data"].s();
                    for (auto& node : nodes) {
                        node->clientRequest(dataStr);
                    }
                } else if (parsed.has("nodeId")) {
                    int nodeId = parsed["nodeId"].i();
                    if (nodeId >= 1 && nodeId <= nodes.size()) {
                        if (command == "CRASH_NODE") {
                            nodes[nodeId - 1]->crash();
                        } else if (command == "RECOVER_NODE") {
                            nodes[nodeId - 1]->recover();
                        }
                    }
                }
            }
        });

    std::cout << "Starting Telemetry Server on ws://localhost:8080/raft\n";
    std::cout << "Press Ctrl+C to terminate the cluster..." << std::endl;
    // Block the main thread and run the web server
    app.port(8080).multithreaded().run();
    
    // Fallback blocking mechanism in case app.run() returns asynchronously
    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(1));
    }
    
    return 0;
}
