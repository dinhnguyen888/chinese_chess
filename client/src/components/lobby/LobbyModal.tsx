import React from 'react';
import { useAppSelector } from '../../hooks';
import LobbyScreen from './LobbyScreen';
import RoomWaitingScreen from './RoomWaitingScreen';
import AuthScreen from './AuthScreen';

/**
 * Entry point cho toàn bộ Lobby UI.
 * Chỉ quyết định hiển thị màn nào dựa vào roomStatus.
 * Mọi logic đã được delegate xuống các component con.
 */
const LobbyModal: React.FC = () => {
  const mode = useAppSelector(s => s.chess.mode);
  const roomStatus = useAppSelector(s => s.chess.roomStatus);
  const isLoggedIn = useAppSelector(s => s.chess.isLoggedIn);

  if (mode !== 5 || roomStatus === 'playing') return null;
  if (!isLoggedIn) return <AuthScreen />;
  if (roomStatus === 'waiting_in_room') return <RoomWaitingScreen />;
  return <LobbyScreen />;
};

export default LobbyModal;
