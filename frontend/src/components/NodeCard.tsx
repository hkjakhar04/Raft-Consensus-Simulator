import React from 'react';
import type { RaftNodeState } from '../store/raftSlice';

interface NodeCardProps {
  node: RaftNodeState;
}

export const NodeCard: React.FC<NodeCardProps> = ({ node }) => {
  const roleStyles = {
    0: 'bg-slate-800 border-slate-600 text-slate-300', // Follower (Gray)
    1: 'bg-amber-600 border-amber-400 text-white', // Candidate (Yellow)
    2: 'bg-blue-600 border-blue-400 text-white shadow-[0_0_20px_rgba(37,99,235,0.7)]' // Leader (Blue)
  };

  const roleNames = {
    0: 'FOLLOWER',
    1: 'CANDIDATE',
    2: 'LEADER'
  };

  const currentStyle = node.isCrashed 
    ? 'bg-red-950 border-red-800 text-red-500 opacity-60'
    : roleStyles[node.role as keyof typeof roleStyles] || roleStyles[0];
  const roleName = node.isCrashed 
    ? 'CRASHED' 
    : roleNames[node.role as keyof typeof roleNames] || 'UNKNOWN';

  return (
    <div className={`flex flex-col items-center justify-center p-6 m-3 rounded-2xl border-2 transition-all duration-300 transform ${node.isCrashed ? '' : 'hover:scale-105'} ${currentStyle}`}>
      <h2 className="text-3xl font-bold mb-2">Node {node.nodeId}</h2>
      
      <div className="text-sm font-extrabold tracking-widest mb-5 px-4 py-1.5 bg-black bg-opacity-40 rounded-full">
        {roleName}
      </div>
      
      <div className="w-full text-left space-y-2 bg-black bg-opacity-25 p-4 rounded-xl font-mono">
        <p className="flex justify-between items-center">
          <span className="opacity-70 text-xs uppercase font-sans font-bold tracking-wider">Term:</span> 
          <span className="text-xl">{node.term}</span>
        </p>
        <p className="flex justify-between items-center">
          <span className="opacity-70 text-xs uppercase font-sans font-bold tracking-wider">Commit Idx:</span> 
          <span className="text-xl">{node.commitIndex}</span>
        </p>
        
        {node.log && node.log.length > 0 && (
          <div className="pt-2 border-t border-white border-opacity-20 mt-2">
            <span className="opacity-70 text-xs uppercase font-sans font-bold tracking-wider block mb-1">Recent Logs:</span>
            <ul className="text-xs space-y-1 max-h-16 overflow-y-auto">
              {node.log.slice(-2).map((entry, idx) => (
                <li key={idx} className="bg-black bg-opacity-30 px-2 py-1 rounded">
                  <span className="opacity-50 mr-2">T{entry.term}</span>
                  <span className="text-emerald-300 font-bold">{entry.command}</span>
                </li>
              ))}
            </ul>
          </div>
        )}
      </div>
    </div>
  );
};
