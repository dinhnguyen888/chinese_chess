import { globalWs } from './useOnlinePlay';
import { useAppSelector } from './index';

/**
 * Tất cả các action gửi lên server liên quan tới Lobby:
 * tạo phòng, join phòng, bắt đầu, tìm kiếm, làm mới.
 */
export function useLobby() {
  const playerName = useAppSelector(s => s.chess.playerName);
  const currentRoomId = useAppSelector(s => s.chess.currentRoomId);

  const send = (payload: object) => {
    if (globalWs && globalWs.readyState === WebSocket.OPEN) {
      globalWs.send(JSON.stringify(payload));
    }
  };

  const createRoom = (roomName: string, autoStart: boolean) => {
    send({ type: 'create_room', roomName, name: playerName, autoStart });
  };

  const joinRoom = (roomId: string) => {
    send({ type: 'join_room', roomId, name: playerName });
  };

  const startGame = () => {
    send({ type: 'start_game', roomId: currentRoomId, name: playerName });
  };

  const searchRoom = (query: string) => {
    send({ type: 'search_room', query, name: playerName });
  };

  const refreshRooms = () => {
    send({ type: 'list_rooms', name: playerName });
  };

  const exitLobby = () => {
    window.location.reload();
  };

  return { createRoom, joinRoom, startGame, searchRoom, refreshRooms, exitLobby };
}
