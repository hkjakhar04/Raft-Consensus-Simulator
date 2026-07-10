```text
Raft Consensus Cluster
├── README.md
├── backend
│   ├── CMakeLists.txt
│   └── src
│       ├── crow.h
│       ├── main.cpp                 # Orchestrator & WS Server
│       ├── raft_core
│       │   ├── Node.cpp             # Core Raft Logic & State Machine
│       │   ├── Node.h
│       │   └── Types.h
│       └── telemetry
│           ├── TelemetryTypes.h
│           └── ThreadSafeQueue.h    # Thread-Safe Event Bus
└── frontend
    ├── index.html
    ├── package.json
    ├── src
    │   ├── App.css
    │   ├── App.tsx
    │   ├── components
    │   │   ├── ChaosControlPanel.tsx
    │   │   ├── ClientPanel.tsx
    │   │   ├── ClusterView.tsx      # Main Visualization Canvas
    │   │   └── NodeCard.tsx
    │   ├── hooks
    │   │   └── useRaftWebSocket.ts  # React WebSocket Integration
    │   ├── index.css
    │   ├── main.tsx
    │   └── store
    │       ├── raftSlice.ts         # Redux Immutable State Reducer
    │       └── store.ts
    ├── tsconfig.json
    └── vite.config.ts
```
