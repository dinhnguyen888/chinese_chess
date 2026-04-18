import { configureStore } from '@reduxjs/toolkit';
import chessReducer from '../models/chessSlice';

export const store = configureStore({
  reducer: {
    chess: chessReducer,
  },
});

export type RootState = ReturnType<typeof store.getState>;
export type AppDispatch = typeof store.dispatch;
