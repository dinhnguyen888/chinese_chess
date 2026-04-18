import { useEffect, useRef } from 'react';
import { message, Modal } from 'antd';
import { useAppDispatch, useAppSelector } from './index';
import {
  applyRemoteMove, onlineMatched, onlineAborted, updateRoomList, addChatMessage,
  hostRoomCreated, playerJoinedRoom, guestJoined, guestLeft, loginSuccess
} from '../models/chessSlice';
import { store } from '../store';
import { wsUrlWithToken } from '../config/server';
import { pushNotification } from '../components/button/NotificationBell';

export let globalWs: WebSocket | null = null;

export function useOnlinePlay() {
  const dispatch = useAppDispatch();
  const mode = useAppSelector((s) => s.chess.mode);
  const isLoggedIn = useAppSelector((s) => s.chess.isLoggedIn);
  const roomStatus = useAppSelector((s) => s.chess.roomStatus);
  const paceHistory = useAppSelector((s) => s.chess.paceHistory);
  const chessChange = useAppSelector((s) => s.chess.chessChange);
  const board = useAppSelector((s) => s.chess.board);
  const winner = useAppSelector((s) => s.chess.winner);


  const wsRef = useRef<WebSocket | null>(null);
  const lastSentPaceLenRef = useRef(0);
  const matchStartTimeRef = useRef<number | null>(null);

  useEffect(() => {
    const token = localStorage.getItem('chess_jwt_token');
    if (!token || !isLoggedIn) {
      wsRef.current?.close();
      wsRef.current = null;
      globalWs = null;
      lastSentPaceLenRef.current = 0;
      return;
    }

    // Đừng tạo mới nếu đã kết nối hoặc đang kết nối
    if (wsRef.current && (wsRef.current.readyState === WebSocket.OPEN || wsRef.current.readyState === WebSocket.CONNECTING)) {
      return;
    }

    lastSentPaceLenRef.current = 0;
    const ws = new WebSocket(wsUrlWithToken(token));
    wsRef.current = ws;
    globalWs = ws;

    ws.onmessage = (ev) => {
      let data: unknown;
      try {
        data = JSON.parse(ev.data as string);
      } catch {
        return;
      }
      const msg = data as Record<string, unknown>;

      if (msg.type === 'auth_success') {
        const username = msg.username as string;
        if (msg.token) {
            localStorage.setItem('chess_jwt_token', msg.token as string);
        }
        if (msg.action !== 'verify') {
            message.success(msg.action === 'register' ? 'Đăng ký thành công!' : 'Đăng nhập thành công!');
        }
        dispatch(loginSuccess({ 
            username, 
            role: (msg.role as string) || 'user',
            canChat: msg.can_chat as boolean,
            canCreateRoom: msg.can_create_room as boolean
        }));
        ws.send(JSON.stringify({ type: 'list_rooms', name: username }));
        return;
      }

      if (msg.type === 'auth_fail') {
        localStorage.removeItem('chess_jwt_token');
        message.warning('Phiên đăng nhập hết hạn, vui lòng đăng nhập lại');
        return;
      }

      if (msg.type === 'room_list') {
        dispatch(updateRoomList(msg.rooms as any));
        return;
      }

      // Host đã tạo phòng thành công → chuyển sang màn hình phòng chờ
      if (msg.type === 'room_created') {
        dispatch(hostRoomCreated({ roomId: msg.roomId as string, roomName: msg.roomName as string }));
        return;
      }

      // Khách đã join phòng → chờ host bắt đầu
      if (msg.type === 'room_joined') {
        message.info(`Đã vào phòng. Chờ chủ phòng bắt đầu...`);
        dispatch(playerJoinedRoom({ roomId: msg.roomId as string, hostName: msg.hostName as string }));
        return;
      }

      // Host nhận notify có khách vào
      if (msg.type === 'guest_joined') {
        message.success(`${msg.guestName} đã vào phòng!`);
        dispatch(guestJoined(msg.guestName as string));
        return;
      }

      // Host nhận notify khách rời phòng
      if (msg.type === 'guest_left') {
        message.warning('Khách đã rời phòng');
        dispatch(guestLeft());
        return;
      }

      if (msg.type === 'chat') {
        dispatch(addChatMessage({ sender: msg.sender as string, message: msg.message as string }));
        return;
      }

      if (msg.type === 'punishment_notify') {
        // Đẩy vào hệ thống thông báo (cookie + badge đỏ)
        pushNotification(
          msg.reason as string,
          msg.reporter as string,
          msg.message as string,
          (msg.ban_days as number) || 0,
          msg.can_chat as boolean !== false, // default true if undefined
          msg.can_create_room as boolean !== false // default true if undefined
        );

        // Vẫn hiện Modal tức thì để user biết ngay
        Modal.warning({
          title: 'THÔNG BÁO XỬ PHẠT',
          content: `Lý do: ${msg.reason}. Người tố cáo: ${msg.reporter}. Chi tiết: ${msg.message}`,
          centered: true,
          okText: 'Tôi đã hiểu',
        });
        return;
      }

      if (msg.type === 'error') {
        message.error(msg.message as string);
        return;
      }

      if (msg.type === 'matched' && typeof msg.color === 'string' && typeof msg.orderSide === 'number') {
        const opponentName = typeof msg.opponentName === 'string' ? msg.opponentName : 'Opponent';
        message.success(`Trận đấu bắt đầu! Đối thủ: ${opponentName}`);
        lastSentPaceLenRef.current = 0;
        matchStartTimeRef.current = Date.now();   // ← ghi nhận giờ bắt đầu
        dispatch(onlineMatched({ color: msg.color, orderSide: msg.orderSide as number, opponentName }));
        return;
      }

      if (msg.type === 'queue') {
        message.info('Đang chờ đối thủ...');
        return;
      }

      if (msg.type === 'opponent_left') {
        message.warning('Đối thủ đã thoát');
        dispatch(onlineAborted());
        ws.send(JSON.stringify({ type: 'list_rooms' }));
        return;
      }

      if (msg.type === 'move') {
        const from = msg.from as unknown;
        const to = msg.to as unknown;
        const piece = msg.piece as unknown;
        const win = msg.winner as unknown;
        if (!Array.isArray(from) || !Array.isArray(to) || from.length !== 2 || to.length !== 2 || typeof piece !== 'string') {
          return;
        }
        dispatch(
          applyRemoteMove({
            from: [Number(from[0]), Number(from[1])],
            to: [Number(to[0]), Number(to[1])],
            piece,
            winner: win === undefined || win === null ? null : Number(win),
          })
        );
        lastSentPaceLenRef.current = store.getState().chess.paceHistory.length;
        return;
      }
    };

    ws.onerror = () => {
      message.error('Lỗi kết nối WebSocket');
    };

    ws.onclose = () => {
      if (wsRef.current === ws) wsRef.current = null;
    };

    return () => {
      ws.close();
      if (wsRef.current === ws) wsRef.current = null;
    };
  }, [dispatch, isLoggedIn]);

  useEffect(() => {
    if (mode !== 5 || roomStatus !== 'playing') return;
    const ws = wsRef.current;
    if (!ws || ws.readyState !== WebSocket.OPEN) return;
    if (paceHistory.length <= lastSentPaceLenRef.current) return;
    if (!chessChange) return;

    const [[i, j], [oldi, oldj]] = chessChange;
    const piece = board[i]?.[j];
    if (!piece) return;

    lastSentPaceLenRef.current = paceHistory.length;

    // Tính thời lượng trận đấu (giây)
    const duration_seconds = matchStartTimeRef.current
      ? Math.floor((Date.now() - matchStartTimeRef.current) / 1000)
      : 0;

    ws.send(
      JSON.stringify({
        type: 'move',
        from: [oldi, oldj],
        to: [i, j],
        piece,
        winner: winner ?? null,
        duration_seconds,   // gửi kèm để server lưu vào DB
        moves: winner !== null ? paceHistory : undefined,
      })
    );
  }, [mode, roomStatus, paceHistory, chessChange, board, winner]);
}
