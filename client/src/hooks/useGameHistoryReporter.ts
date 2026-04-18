import { useEffect, useRef } from 'react';
import { useAppSelector } from './index';
import { wsUrlWithToken } from '../config/server';


export function useGameHistoryReporter() {
  const mode = useAppSelector((s) => s.chess.mode);
  const winner = useAppSelector((s) => s.chess.winner);
  const difficulty = useAppSelector((s) => s.chess.difficulty);
  const isLoggedIn = useAppSelector((s) => s.chess.isLoggedIn);
  const paceHistory = useAppSelector((s) => s.chess.paceHistory);

  // Tránh gửi 2 lần cho cùng một kết quả
  const reportedWinnerRef = useRef<number | null>(undefined);
  // Thời điểm ván bắt đầu
  const matchStartRef = useRef<number | null>(null);

  // Ghi nhận lúc ván bắt đầu (side đổi từ 0 → khác 0)
  const side = useAppSelector((s) => s.chess.side);
  useEffect(() => {
    if (mode === 1 && side !== 0) {
      // Ván vừa bắt đầu
      if (matchStartRef.current === null) {
        matchStartRef.current = Date.now();
        reportedWinnerRef.current = undefined;
      }
    }
    if (mode !== 1 || side === 0) {
      // Ván chưa bắt đầu hoặc đã kết thúc → reset timer
      if (winner === null) {
        matchStartRef.current = null;
      }
    }
  }, [mode, side, winner]);

  useEffect(() => {
    // Chỉ xử lý chế độ Người vs Máy
    if (mode !== 1) return;
    // Phải có kết quả và chưa báo cáo lần này
    if (winner === null || winner === reportedWinnerRef.current) return;
    // Phải đã đăng nhập
    if (!isLoggedIn) return;

    const token = localStorage.getItem('chess_jwt_token');
    if (!token) return;

    reportedWinnerRef.current = winner;

    // Tên đối thủ AI tuỳ độ khó
    const diffLabel = difficulty === 2 ? 'AI-GaMo' : difficulty === 3 ? 'AI-NhapMon' : 'AI-BacThay';

    // winner=1 → quân đỏ thắng (người chơi = đỏ nếu color='r')
    // Đơn giản: winner=1 → người thắng (vì AI luôn là side=-1 trong mode 1)
    const result: 'win' | 'lose' = winner === 1 ? 'win' : 'lose';

    const duration = matchStartRef.current
      ? Math.floor((Date.now() - matchStartRef.current) / 1000)
      : 0;

    matchStartRef.current = null; // Reset cho ván tiếp theo

    // Mở WS tạm (JWT trong query) → chờ auth_success → gửi game_result → đóng
    const ws = new WebSocket(wsUrlWithToken(token));

    ws.onmessage = (ev) => {
      try {
        const msg = JSON.parse(ev.data as string);
        if (msg.type === 'auth_success') {
          ws.send(JSON.stringify({
            type: 'game_result',
            opponent: diffLabel,
            result,
            duration_seconds: duration,
            moves: paceHistory,
          }));
          setTimeout(() => ws.close(), 300);
        }
      } catch { /* ignore */ }
    };

    ws.onerror = () => ws.close();
  }, [winner, mode, difficulty, isLoggedIn, paceHistory]);
}
