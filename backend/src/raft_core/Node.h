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

    // Telemetry
    void setTelemetryCallback(std::function<void(const telemetry::RaftStateEvent&)> cb);
    void forceSyncTelemetry();
    
    void crash();
    void recover();
    void clientRequest(const std::string& command);

private:
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
    int m_currentTerm = 0;
    std::optional<int> m_votedFor;
    std::vector<LogEntry> m_log;

    // Volatile state
    int m_commitIndex = 0;
    int m_lastApplied = 0;
    NodeState m_state = NodeState::FOLLOWER;
    
    // Volatile state for leaders
    std::map<int, int> m_nextIndex;
    std::map<int, int> m_matchIndex;

    // Election timeout tracking
    std::chrono::time_point<std::chrono::steady_clock> m_lastHeartbeat;
    std::chrono::milliseconds m_electionTimeout;

    // RPC callbacks to other nodes
    std::function<RequestVoteReply(int, const RequestVoteArgs&)> sendRequestVote;
    std::function<AppendEntriesReply(int, const AppendEntriesArgs&)> sendAppendEntries;

    // Telemetry callback
    std::function<void(const telemetry::RaftStateEvent&)> m_telemetryCallback;
};

} // namespace raft
