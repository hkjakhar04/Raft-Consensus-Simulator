import React, { useState } from 'react';

interface ClientPanelProps {
  onSendCommand: (command: string, data?: string) => void;
}

export const ClientPanel: React.FC<ClientPanelProps> = ({ onSendCommand }) => {
  const [inputValue, setInputValue] = useState('');

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (inputValue.trim()) {
      onSendCommand('CLIENT_REQUEST', inputValue.trim());
      setInputValue('');
    }
  };

  return (
    <div className="mt-8 p-6 bg-slate-800 rounded-lg shadow-xl border border-slate-700 w-full max-w-4xl mx-auto">
      <h3 className="text-xl font-bold text-slate-200 mb-4 text-center">Client Requests</h3>
      <form onSubmit={handleSubmit} className="flex flex-col sm:flex-row gap-4 justify-center items-center">
        <input 
          type="text" 
          value={inputValue}
          onChange={(e) => setInputValue(e.target.value)}
          placeholder="e.g. SET X=10"
          className="flex-1 px-4 py-2 bg-slate-900 border border-slate-600 rounded text-white focus:outline-none focus:border-blue-500 w-full sm:w-auto"
        />
        <button 
          type="submit"
          disabled={!inputValue.trim()}
          className="px-6 py-2 bg-blue-600 hover:bg-blue-500 disabled:bg-slate-600 disabled:cursor-not-allowed text-white font-bold rounded shadow transition-colors"
        >
          Send to Leader
        </button>
      </form>
    </div>
  );
};
