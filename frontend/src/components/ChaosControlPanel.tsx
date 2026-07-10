import React from 'react';
import { useSelector } from 'react-redux';
import type { RootState } from '../store/store';

interface ChaosControlPanelProps {
  onSendCommand: (command: string, nodeId: number) => void;
}

export const ChaosControlPanel: React.FC<ChaosControlPanelProps> = ({ onSendCommand }) => {
  const nodes = useSelector((state: RootState) => state.raft.nodes);
  
  return (
    <div className="mt-8 p-6 bg-slate-800 rounded-lg shadow-xl border border-slate-700 w-full max-w-4xl mx-auto">
      <h3 className="text-xl font-bold text-slate-200 mb-4 text-center">Chaos Control Panel</h3>
      <div className="flex justify-center gap-4 flex-wrap">
        {Object.values(nodes).map(node => (
          <button
            key={node.nodeId}
            onClick={() => {
              const command = node.isCrashed ? 'RECOVER_NODE' : 'CRASH_NODE';
              onSendCommand(command, node.nodeId);
            }}
            className={`px-4 py-2 font-bold rounded shadow-lg transition-colors duration-200 ${
              node.isCrashed 
                ? 'bg-red-600 hover:bg-red-500 text-white' 
                : 'bg-green-600 hover:bg-green-500 text-white'
            }`}
          >
            {node.isCrashed ? `Revive Node ${node.nodeId}` : `Crash Node ${node.nodeId}`}
          </button>
        ))}
      </div>
    </div>
  );
};
