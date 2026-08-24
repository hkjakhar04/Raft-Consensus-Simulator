import React from 'react';
import { useSelector } from 'react-redux';
import { useRaftWebSocket } from '../hooks/useRaftWebSocket';
import { NodeCard } from './NodeCard';
import { ChaosControlPanel } from './ChaosControlPanel';
import { ClientPanel } from './ClientPanel';
import type { RootState } from '../store/store'; 

export const ClusterView: React.FC = () => {
  const wsUrl = import.meta.env.VITE_WS_URL || 'ws://localhost:8080/raft';
  const { sendCommand } = useRaftWebSocket(wsUrl);

  const nodes = useSelector((state: RootState) => state.raft.nodes);
  const nodesArray = Object.values(nodes);

  return (
    <div className="min-h-screen bg-slate-950 text-white p-8 md:p-16 font-sans">
      <div className="max-w-6xl mx-auto">
        <header className="mb-16 text-center">
          <h1 className="text-5xl md:text-6xl font-black mb-4 tracking-tight bg-clip-text text-transparent bg-gradient-to-br from-blue-400 via-teal-300 to-emerald-400 drop-shadow-lg">
            Raft Consensus Cluster
          </h1>
          <p className="text-slate-400 text-lg md:text-xl font-medium tracking-wide">
            Real-time Telemetry & Visualization
          </p>
        </header>

        <div className="flex flex-wrap justify-center items-center gap-4">
          {nodesArray.map((node) => (
            <div key={node.nodeId} className="w-full sm:w-1/2 lg:w-1/3 max-w-sm">
              <NodeCard node={node} />
            </div>
          ))}
        </div>

        <div className="flex flex-col gap-4">
          <ClientPanel onSendCommand={sendCommand} />
          <ChaosControlPanel onSendCommand={sendCommand} />
        </div>
      </div>
    </div>
  );
};
