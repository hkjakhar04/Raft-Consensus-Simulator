import { useEffect, useRef } from 'react';
import { useDispatch } from 'react-redux';
import { updateNodeState } from '../store/raftSlice';

export const useRaftWebSocket = (url: string) => {
  const dispatch = useDispatch();
  const ws = useRef<WebSocket | null>(null);

  useEffect(() => {
    ws.current = new WebSocket(url);

    ws.current.onopen = () => console.log('Connected to Raft Telemetry');
    
    ws.current.onmessage = (event) => {
      try {
        const payload = JSON.parse(event.data);
        dispatch(updateNodeState(payload));
      } catch (error) {
        console.error('Failed to parse WebSocket message', error);
      }
    };

    ws.current.onclose = () => console.log('Disconnected from Raft Telemetry');

    return () => {
      if (ws.current?.readyState === WebSocket.OPEN || ws.current?.readyState === WebSocket.CONNECTING) {
        ws.current.close();
      }
    };
  }, [url, dispatch]);

  const sendCommand = (command: string, payload?: any) => {
    if (ws.current?.readyState === WebSocket.OPEN) {
      if (typeof payload === 'number') {
        ws.current.send(JSON.stringify({ command, nodeId: payload }));
      } else if (typeof payload === 'string') {
        ws.current.send(JSON.stringify({ command, data: payload }));
      } else {
        ws.current.send(JSON.stringify({ command }));
      }
    }
  };

  return { sendCommand };
};
