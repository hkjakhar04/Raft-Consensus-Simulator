#pragma once
#include "Types.h"
#include "../telemetry/TelemetryTypes.h"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <functional>
#include <map>

namespace raft {

class Node {
public:
    Node(int id, const std::vector<int>& peerIds);
    ~Node();

    void start();
    void stop();

    // RPC endpoints (called by other nodes)
    RequestVoteReply requestVote(const RequestVoteArgs& args);
    AppendEntriesReply appendEntries(const AppendEntriesArgs& args);

    // Provide a way to link nodes together for in-memory RPC
    void setRpcHandler(std::function<RequestVoteReply(int, const RequestVoteArgs&)> requestVoteFunc,
                       std::function<AppendEntriesReply(int, const AppendEntriesArgs&)> appendEntriesFunc);

    // Telemetry means information about what the node is doing, its state, and events that occur. 
    // setTelemetryCallback sets up where state updates should go.
    // forceSyncTelemetry forces the node to send its current state to the telemetry callback.
    void setTelemetryCallback(std::function<void(const telemetry::RaftStateEvent&)> cb);
    void forceSyncTelemetry();
    
    void crash();
    void recover();
    void clientRequest(const std::string& command);

private:
// creates s snapshot of the node's state and sends it to the telemetry callback
    void emitTelemetry(telemetry::EventType type, const std::string& message = "");
    void run();
    void resetElectionTimer();
    void startElection();
    void becomeFollower(int term);
    void becomeLeader();
    
    // Core state
    int m_id;
    std::vector<int> m_peerIds;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_isCrashed{false};
    std::thread m_workerThread;

    std::mutex m_mutex;
    std::condition_variable m_cv;

    // Persistent state
    // the highest term the node has seen
    int m_currentTerm = 0;
    // the candidate id this node voted for in the current term
    std::optional<int> m_votedFor;
    // the log entries
    std::vector<LogEntry> m_log;

    // Volatile state
    // the highest log entry known to be committed
    int m_commitIndex = 0;
    // the highest log entry applied to the state machine
    int m_lastApplied = 0;
    // stores the current role of the node (follower, candidate, leader)
    NodeState m_state = NodeState::FOLLOWER;
    
    // Volatile state for leaders
    // maps a follower node ID to the next log index the leader should attempt to send
    std::map<int, int> m_nextIndex;
    // maps a follower node ID to the highest matching log index acknowledged by that follower
    std::map<int, int> m_matchIndex;

    // Election timeout tracking
    // stores the time point of the last heartbeat received from a leader
    std::chrono::time_point<std::chrono::steady_clock> m_lastHeartbeat;
    // stores how long the node should wait before starting an election if no heartbeat is received
    std::chrono::milliseconds m_electionTimeout;

    // RPC callbacks to other nodes
    std::function<RequestVoteReply(int, const RequestVoteArgs&)> sendRequestVote;
    std::function<AppendEntriesReply(int, const AppendEntriesArgs&)> sendAppendEntries;

    // Telemetry callback
    std::function<void(const telemetry::RaftStateEvent&)> m_telemetryCallback;
};

} // namespace raft
