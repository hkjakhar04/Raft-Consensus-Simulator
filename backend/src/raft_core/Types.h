#pragma once

#include <string>
#include <vector>
#include <optional>

namespace raft {

enum class NodeState {
    FOLLOWER,
    CANDIDATE,
    LEADER
};

struct LogEntry {
    int term;
    std::string command;
};

// the voter uses lastLogindex and lastLogTerm to determine whether the candidate log is sufficiently up-to-date to grant its vote. 
//If the logs have last entries with different terms, then the log with the later term is more up-to-date.
// If the logs end with the same term, then whichever log is longer is more up-to-date.
struct RequestVoteArgs {
    int term;
    int candidateId;
    int lastLogIndex;
    int lastLogTerm;
};

// if it is higher than the candidate’s current term, then the candidate updates its term and converts to a follower.
struct RequestVoteReply {
    int term;
    bool voteGranted;
};

// entries can contain multiple log entries, each holding a term and command.
// This allows the leader to send several missing entries in one request.
// Example:
// Leader:   [1] [2] [3] [4] [5]
// Follower: [1] [2]
// If prevLogIndex = 4, the follower rejects the request because entry 4
// is missing. The leader backs up nextIndex and retries until it finds
// a matching preceding entry.
// With prevLogIndex = 2 and a matching prevLogTerm, the follower can
// accept entries [3, 4, 5].
// For an empty follower log, replication starts with prevLogIndex = 0
// and prevLogTerm = 0, meaning there is no preceding entry to check.
// Multiple entries can also be batched for efficiency.
// An empty entries list is used for a heartbeat.
struct AppendEntriesArgs {
    int term;
    int leaderId;
    int prevLogIndex;
    int prevLogTerm;
    std::vector<LogEntry> entries;
    int leaderCommit;
};


struct AppendEntriesReply {
    int term;
    bool success;
};

} // namespace raft
