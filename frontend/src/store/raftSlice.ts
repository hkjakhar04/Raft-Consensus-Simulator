import { createSlice } from '@reduxjs/toolkit';
import type { PayloadAction } from '@reduxjs/toolkit';

export interface LogEntry {
  term: number;
  command: string;
}

export interface RaftNodeState {
  nodeId: number;
  term: number;
  role: number; // 0 = Follower, 1 = Candidate, 2 = Leader
  commitIndex: number;
  isCrashed: boolean;
  log: LogEntry[];
}

interface ClusterState {
  nodes: Record<number, RaftNodeState>;
}

const initialState: ClusterState = {
  nodes: {
    1: { nodeId: 1, term: 0, role: 0, commitIndex: 0, isCrashed: false, log: [] },
    2: { nodeId: 2, term: 0, role: 0, commitIndex: 0, isCrashed: false, log: [] },
    3: { nodeId: 3, term: 0, role: 0, commitIndex: 0, isCrashed: false, log: [] },
    4: { nodeId: 4, term: 0, role: 0, commitIndex: 0, isCrashed: false, log: [] },
    5: { nodeId: 5, term: 0, role: 0, commitIndex: 0, isCrashed: false, log: [] },
  }
};

interface RaftPayload {
  nodeId: number;
  type: string;
  term: number;
  role: number;
  commitIndex: number;
  message?: string;
  isCrashed?: boolean;
  log?: LogEntry[];
}

export const raftSlice = createSlice({
  name: 'raft',
  initialState,
  reducers: {
    updateNodeState: (state, action: PayloadAction<RaftPayload>) => {
      const { nodeId, term, role, commitIndex, isCrashed, log } = action.payload;
      if (state.nodes[nodeId]) {
        state.nodes[nodeId].term = term;
        state.nodes[nodeId].role = role;
        state.nodes[nodeId].commitIndex = commitIndex;
        if (isCrashed !== undefined) {
          state.nodes[nodeId].isCrashed = isCrashed;
        }
        if (log !== undefined) {
          state.nodes[nodeId].log = log;
        }
      }
    }
  }
});

export const { updateNodeState } = raftSlice.actions;
export default raftSlice.reducer;
