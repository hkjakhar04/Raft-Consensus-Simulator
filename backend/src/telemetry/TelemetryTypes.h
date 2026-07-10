#pragma once
#include "../raft_core/Types.h"
#include <string>

namespace telemetry {

enum class EventType {
    STATE_TRANSITION,
    ELECTION_STARTED,
    VOTE_CAST
};

struct RaftStateEvent {
    int nodeId;
    EventType type;
    int currentTerm;
    raft::NodeState role;
    int commitIndex;
    std::string message;
    bool isCrashed = false;
    std::vector<raft::LogEntry> logEntries;
};

} // namespace telemetry
