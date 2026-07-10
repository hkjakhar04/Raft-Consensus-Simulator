#include "Node.h"
#include <iostream>
#include <random>

namespace raft {

Node::Node(int id, const std::vector<int>& peerIds)
    : m_id(id), m_peerIds(peerIds) {
    resetElectionTimer();
}

Node::~Node() {
    stop();
}

void Node::crash() {
    m_isCrashed = true;
    std::cout << "Node " << m_id << " CRASHED.\n";
    emitTelemetry(telemetry::EventType::STATE_TRANSITION, "Node Crashed");
}

void Node::recover() {
    m_isCrashed = false;
    std::cout << "Node " << m_id << " RECOVERED.\n";
    resetElectionTimer();
    emitTelemetry(telemetry::EventType::STATE_TRANSITION, "Node Recovered");
}

void Node::clientRequest(const std::string& command) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_state != NodeState::LEADER || m_isCrashed) {
        return; // Only leaders process client requests
    }
    
    m_log.push_back({m_currentTerm, command});
    std::cout << "Node " << m_id << " appended CLIENT_REQUEST: " << command << "\n";
    emitTelemetry(telemetry::EventType::STATE_TRANSITION, "Appended client request");
}

void Node::forceSyncTelemetry() {
    std::lock_guard<std::mutex> lock(m_mutex);
    emitTelemetry(telemetry::EventType::STATE_TRANSITION, "Sync state");
}

void Node::start() {
    if (m_running) return;
    m_running = true;
    m_workerThread = std::thread(&Node::run, this);
}

void Node::stop() {
    if (!m_running) return;
    m_running = false;
    m_cv.notify_all();
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

void Node::setRpcHandler(std::function<RequestVoteReply(int, const RequestVoteArgs&)> requestVoteFunc,
                         std::function<AppendEntriesReply(int, const AppendEntriesArgs&)> appendEntriesFunc) {
    sendRequestVote = std::move(requestVoteFunc);
    sendAppendEntries = std::move(appendEntriesFunc);
}

void Node::setTelemetryCallback(std::function<void(const telemetry::RaftStateEvent&)> cb) {
    m_telemetryCallback = std::move(cb);
}

void Node::emitTelemetry(telemetry::EventType type, const std::string& message) {
    if (m_telemetryCallback) {
        telemetry::RaftStateEvent event;
        event.nodeId = m_id;
        event.type = type;
        event.currentTerm = m_currentTerm;
        event.role = m_state;
        event.commitIndex = m_commitIndex;
        event.message = message;
        event.isCrashed = m_isCrashed;
        event.logEntries = m_log;
        m_telemetryCallback(event);
    }
}

void Node::run() {
    while (m_running) {
        std::unique_lock<std::mutex> lock(m_mutex);
        
        auto now = std::chrono::steady_clock::now();
        if (!m_isCrashed && m_state != NodeState::LEADER && now - m_lastHeartbeat >= m_electionTimeout) {
            startElection();
        }

        m_cv.wait_for(lock, std::chrono::milliseconds(50), [this]() {
            return !m_running;
        });
    }
}

void Node::resetElectionTimer() {
    // Randomized election timeout between 150ms and 300ms
    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> distribution(150, 300);
    
    m_electionTimeout = std::chrono::milliseconds(distribution(generator));
    m_lastHeartbeat = std::chrono::steady_clock::now();
}

void Node::startElection() {
    m_state = NodeState::CANDIDATE;
    m_currentTerm++;
    m_votedFor = m_id;
    resetElectionTimer();

    std::cout << "Node " << m_id << " starting election for term " << m_currentTerm << "\n";
    emitTelemetry(telemetry::EventType::ELECTION_STARTED, "Started election for term " + std::to_string(m_currentTerm));

    RequestVoteArgs args;
    args.term = m_currentTerm;
    args.candidateId = m_id;
    args.lastLogIndex = m_log.empty() ? 0 : m_log.size();
    args.lastLogTerm = m_log.empty() ? 0 : m_log.back().term;

    auto votesGranted = std::make_shared<int>(1); // Voted for self
    int majority = (m_peerIds.size() + 1) / 2;

    for (int peerId : m_peerIds) {
        // Fire-and-forget RPCs
        std::thread([this, peerId, args, votesGranted, majority]() {
            if (!sendRequestVote) return;

            // Block during RPC call (without holding the lock)
            RequestVoteReply reply = sendRequestVote(peerId, args);

            // Acquire lock to process the reply safely
            std::lock_guard<std::mutex> lock(m_mutex);

            // Stale State Protection: discard if we are no longer candidates for this term
            if (m_state != NodeState::CANDIDATE || m_currentTerm != args.term) {
                return;
            }

            if (reply.term > m_currentTerm) {
                becomeFollower(reply.term);
                return;
            }

            if (reply.voteGranted) {
                (*votesGranted)++;
                if (*votesGranted > majority) {
                    becomeLeader();
                }
            }
        }).detach();
    }
}

void Node::becomeFollower(int term) {
    m_state = NodeState::FOLLOWER;
    m_currentTerm = term;
    m_votedFor = std::nullopt;
    resetElectionTimer();
    emitTelemetry(telemetry::EventType::STATE_TRANSITION, "Became FOLLOWER for term " + std::to_string(term));
}

void Node::becomeLeader() {
    m_state = NodeState::LEADER;
    std::cout << "Node " << m_id << " became LEADER for term " << m_currentTerm << "\n";
    emitTelemetry(telemetry::EventType::STATE_TRANSITION, "Became LEADER for term " + std::to_string(m_currentTerm));
    
    // Initialize volatile state for leaders
    for (int peerId : m_peerIds) {
        m_nextIndex[peerId] = m_log.size() + 1; // 1-based log indexing
        m_matchIndex[peerId] = 0;
    }

    // Start background heartbeat loop
    std::thread([this, termAtElection = m_currentTerm]() {
        while (m_running) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_state != NodeState::LEADER || m_currentTerm != termAtElection) {
                    break;
                }

                if (m_isCrashed) {
                    continue; // Skip sending heartbeats if crashed
                }

                for (int peerId : m_peerIds) {
                    AppendEntriesArgs args;
                    args.term = m_currentTerm;
                    args.leaderId = m_id;
                    args.leaderCommit = m_commitIndex;

                    int nextIdx = m_nextIndex[peerId];
                    args.prevLogIndex = nextIdx - 1;
                    args.prevLogTerm = (args.prevLogIndex > 0 && args.prevLogIndex <= m_log.size()) 
                                        ? m_log[args.prevLogIndex - 1].term : 0;
                    
                    if (nextIdx > 0 && nextIdx <= m_log.size()) {
                        args.entries = std::vector<LogEntry>(m_log.begin() + nextIdx - 1, m_log.end());
                    } else {
                        args.entries = {}; 
                    } 

                    // Fire-and-forget concurrent RPC
                    std::thread([this, peerId, args]() {
                        if (!sendAppendEntries) return;
                        
                        // Block during RPC call
                        AppendEntriesReply reply = sendAppendEntries(peerId, args);

                        // Process reply
                        std::lock_guard<std::mutex> lock(m_mutex);
                        if (m_state != NodeState::LEADER || m_currentTerm != args.term) {
                            return; // Stale reply
                        }

                        if (reply.term > m_currentTerm) {
                            becomeFollower(reply.term);
                            return;
                        }

                        // Log matching processing
                        if (reply.success) {
                            int matchIdx = args.prevLogIndex + args.entries.size();
                            if (matchIdx > m_matchIndex[peerId]) {
                                m_matchIndex[peerId] = matchIdx;
                                m_nextIndex[peerId] = matchIdx + 1;
                                
                                // Update commitIndex
                                for (int n = m_log.size(); n > m_commitIndex; --n) {
                                    if (m_log[n - 1].term == m_currentTerm) {
                                        int count = 1; // self
                                        for (int p : m_peerIds) {
                                            if (m_matchIndex[p] >= n) count++;
                                        }
                                        if (count > (m_peerIds.size() + 1) / 2) {
                                            m_commitIndex = n;
                                            emitTelemetry(telemetry::EventType::STATE_TRANSITION, "Committed log idx " + std::to_string(n));
                                            break;
                                        }
                                    }
                                }
                            }
                        } else {
                            // Decrement nextIndex to retry
                            m_nextIndex[peerId] = std::max(1, m_nextIndex[peerId] - 1);
                        }
                    }).detach();
                }
            } // Unlock before sleeping

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }).detach();
}

RequestVoteReply Node::requestVote(const RequestVoteArgs& args) {
    std::lock_guard<std::mutex> lock(m_mutex);
    RequestVoteReply reply;
    reply.term = m_currentTerm;
    reply.voteGranted = false;

    if (m_isCrashed) return reply;

    if (args.term < m_currentTerm) {
        return reply;
    }

    if (args.term > m_currentTerm) {
        becomeFollower(args.term);
    }

    if (!m_votedFor.has_value() || m_votedFor.value() == args.candidateId) {
        int lastLogTerm = m_log.empty() ? 0 : m_log.back().term;
        int lastLogIndex = m_log.empty() ? 0 : m_log.size();

        bool logIsUpToDate = (args.lastLogTerm > lastLogTerm) || 
                             (args.lastLogTerm == lastLogTerm && args.lastLogIndex >= lastLogIndex);
        
        if (logIsUpToDate) {
            m_votedFor = args.candidateId;
            reply.voteGranted = true;
            resetElectionTimer();
        }
    }

    reply.term = m_currentTerm;
    return reply;
}

AppendEntriesReply Node::appendEntries(const AppendEntriesArgs& args) {
    std::lock_guard<std::mutex> lock(m_mutex);
    AppendEntriesReply reply;
    reply.term = m_currentTerm;
    reply.success = false;

    if (m_isCrashed) return reply;

    if (args.term < m_currentTerm) {
        return reply;
    }

    if (args.term > m_currentTerm) {
        becomeFollower(args.term);
    } else if (m_state != NodeState::FOLLOWER) {
        becomeFollower(args.term); // Fallback if candidate receives valid heartbeat
    }

    // Acknowledge leader authority
    resetElectionTimer();

    // Log Consistency Check (The Raft Safety Property)
    if (args.prevLogIndex > m_log.size()) {
        reply.success = false; // Log is too short
        return reply;
    }

    if (args.prevLogIndex > 0) {
        if (m_log[args.prevLogIndex - 1].term != args.prevLogTerm) {
            reply.success = false; // Term mismatch at prevLogIndex
            return reply;
        }
    }

    // Otherwise, log matches
    reply.success = true;
    reply.term = m_currentTerm;

    int index = args.prevLogIndex;
    for (const auto& entry : args.entries) {
        index++;
        if (m_log.size() >= index) {
            if (m_log[index - 1].term != entry.term) {
                m_log.erase(m_log.begin() + index - 1, m_log.end());
                m_log.push_back(entry);
            }
        } else {
            m_log.push_back(entry);
        }
    }

    if (args.leaderCommit > m_commitIndex) {
        m_commitIndex = std::min(args.leaderCommit, (int)m_log.size());
        emitTelemetry(telemetry::EventType::STATE_TRANSITION, "Updated commit index");
    }

    return reply;
}

} // namespace raft
