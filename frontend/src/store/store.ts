import { configureStore } from '@reduxjs/toolkit';
import raftReducer from './raftSlice';

export const store = configureStore({
  reducer: {
    raft: raftReducer,
  },
});

export type RootState = ReturnType<typeof store.getState>;
export type AppDispatch = typeof store.dispatch;
