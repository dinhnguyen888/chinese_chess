import React, { useEffect, useState } from 'react';
import { useParams, useNavigate } from 'react-router-dom';
import { Spin, Empty, Button, message } from 'antd';
import { style } from 'typestyle';
import { useAppDispatch, useAppSelector } from '../hooks';
import { startReplay } from '../models/chessSlice';
import ChessBoardCanvas from '../components/board/ChessBoardCanvas';
import ButtonGroup from '../components/button/ButtonGroup';
import { wsUrlWithToken } from '../config/server';

import background from '../assets/background.png';

const shellStyle = style({
  height: '100vh',
  width: '100vw',
  overflow: 'hidden',
  backgroundImage: `url(${background})`,
  backgroundSize: 'cover',
  backgroundPosition: 'center',
  backgroundAttachment: 'fixed',
});

const layoutStyle = style({
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  width: '100%',
  height: '100%',
});

const gameColumnStyle = style({
  display: 'flex',
  flexDirection: 'column',
  alignItems: 'center',
});

const buttonBarStyle = style({
  marginTop: '24px',
  width: '507px',
});

const Replay: React.FC = () => {
  const { matchId } = useParams<{ matchId: string }>();
  const navigate = useNavigate();
  const dispatch = useAppDispatch();
  const { mode, side, showModel, history } = useAppSelector(s => s.chess);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  useEffect(() => {
    if (!matchId) return;

    const token = localStorage.getItem('chess_jwt_token');
    if (!token) {
      setError('Bạn cần đăng nhập để xem lại trận đấu');
      setLoading(false);
      return;
    }

    const ws = new WebSocket(wsUrlWithToken(token));
    
    ws.onopen = () => {
      ws.send(JSON.stringify({ type: 'get_replay', match_id: parseInt(matchId) }));
    };

    ws.onmessage = (ev) => {
      try {
        const msg = JSON.parse(ev.data as string);
        if (msg.type === 'replay_data') {
          dispatch(startReplay(msg.moves as string[]));
          setLoading(false);
          ws.close();
        } else if (msg.type === 'error') {
          setError(msg.message || 'Lỗi khi tải dữ liệu trận đấu');
          setLoading(false);
        }
      } catch (e) {
        setError('Lỗi kết nối server');
        setLoading(false);
      }
    };

    ws.onerror = () => {
      setError('Lỗi kết nối WebSocket');
      setLoading(false);
    };

    return () => {
      if (ws.readyState === WebSocket.OPEN) ws.close();
    };
  }, [matchId, dispatch]);

  if (loading) {
    return (
      <div className={shellStyle}>
        <div style={{ display: 'flex', height: '100%', alignItems: 'center', justifyContent: 'center', flexDirection: 'column' }}>
          <Spin size="large" />
          <div style={{ marginTop: 20, color: '#fff' }}>Đang tải kỳ phổ...</div>
        </div>
      </div>
    );
  }

  if (error) {
    return (
      <div className={shellStyle}>
        <div style={{ display: 'flex', height: '100%', alignItems: 'center', justifyContent: 'center' }}>
          <Empty description={<span style={{ color: '#fff' }}>{error}</span>}>
            <Button type="primary" onClick={() => window.close()}>Đóng tab</Button>
          </Empty>
        </div>
      </div>
    );
  }

  return (
    <div className={shellStyle}>
      <div className={layoutStyle}>
        <main className={gameColumnStyle}>
          <ChessBoardCanvas />
          <div className={buttonBarStyle}>
            <ButtonGroup mode={mode} side={side} showModel={showModel} historyLength={history.length} />
          </div>
        </main>
      </div>
    </div>
  );
};

export default Replay;
