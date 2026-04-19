import React, { useState, useRef, useEffect } from 'react';
import { Button } from 'antd';
import { useAppDispatch, useAppSelector } from '../../hooks';
import {
  startClick, changeSide, toggleAI, clearChess, showHint, regretMove, beginOnlineMatch, onGameOver,
  prevReplayMove, nextReplayMove, exitReplay
} from '../../models/chessSlice';
import StartModel from './StartModel';
import MatchHistoryModal from '../lobby/MatchHistoryModal';

interface ButtonGroupProps {
  mode: number;
  side: number;
  showModel: boolean;
  historyLength: number;
}

const ButtonGroup: React.FC<ButtonGroupProps> = ({ mode, side, showModel, historyLength }) => {
  const dispatch = useAppDispatch();
  const isLoggedIn = useAppSelector(s => s.chess.isLoggedIn);
  const replayIndex = useAppSelector(s => s.chess.replayIndex);
  const replayTotal = useAppSelector(s => s.chess.replayMoves.length);
  const [historyOpen, setHistoryOpen] = useState(false);
  const [autoPlay, setAutoPlay] = useState(false);
  const autoPlayRef = useRef<ReturnType<typeof setInterval> | null>(null);

  // Tự động phát: mỗi 1.2s dispatch nextReplayMove
  useEffect(() => {
    if (autoPlay) {
      autoPlayRef.current = setInterval(() => {
        dispatch(nextReplayMove());
      }, 1200);
    } else {
      if (autoPlayRef.current) clearInterval(autoPlayRef.current);
    }
    return () => { if (autoPlayRef.current) clearInterval(autoPlayRef.current); };
  }, [autoPlay, dispatch]);

  // Tự dừng khi đến nước cuối
  useEffect(() => {
    if (autoPlay && replayIndex >= replayTotal) {
      setAutoPlay(false);
    }
  }, [replayIndex, replayTotal, autoPlay]);

  const containerStyle = {
    height: '100%',
    width: '100%',
    display: 'flex',
    justifyContent: 'center',
    gap: '16px', // Khoảng cách đều giữa các nút
  };

  // MÀN HÌNH CHƯA CHƠI (side === 0)
  if (side === 0) {
    return (
      <div>
        <div style={containerStyle}>
          <Button
            type='primary'
            size='large'
            onClick={() => dispatch(startClick())}
          >
            Bắt đầu
          </Button>

          {!isLoggedIn ? (
            <>
              <Button size='large' onClick={() => dispatch(beginOnlineMatch(''))}>Đăng nhập</Button>
              <Button size='large' onClick={() => dispatch(beginOnlineMatch(''))}>Đăng ký</Button>
            </>
          ) : (
            <>
              <Button size='large' onClick={() => setHistoryOpen(true)}>Lịch sử đấu</Button>
              <Button size='large' onClick={() => {
                localStorage.removeItem('chess_jwt_token');
                window.location.reload();
              }}>
                Đăng xuất
              </Button>
            </>
          )}

          {/* <Button 
            size='large' 
            style={{
              background: 'linear-gradient(45deg, #f1c40f, #e67e22)',
              color: '#fff',
              border: 'none',
              fontWeight: 'bold',
              textTransform: 'uppercase',
              letterSpacing: '1px',
              boxShadow: '0 4px 15px rgba(230, 126, 34, 0.5)',
              animation: 'pulse 2s infinite'
            }}
            onClick={() => dispatch(beginOnlineMatch(''))}
          >
            🏆 Đấu hạng
          </Button> */}
        </div>
        <StartModel visible={showModel} />
        <MatchHistoryModal open={historyOpen} onClose={() => setHistoryOpen(false)} />
      </div>
    );
  }

  if (mode === 6) {
    const isEnd = replayIndex >= replayTotal;
    return (
      <div style={containerStyle}>
        <Button
          size='large'
          onClick={() => { setAutoPlay(false); dispatch(prevReplayMove()); }}
          disabled={replayIndex === 0}
        >
          ◄ Trước
        </Button>

        <Button
          size='large'
          type={autoPlay ? 'default' : 'primary'}
          onClick={() => setAutoPlay(p => !p)}
          disabled={isEnd}
          style={autoPlay ? { borderColor: '#faad14', color: '#faad14' } : {}}
        >
          {autoPlay ? '⏸ Dừng' : '▶ Tự động'}
        </Button>

        <Button
          size='large'
          onClick={() => { setAutoPlay(false); dispatch(nextReplayMove()); }}
          disabled={isEnd}
        >
          Sau ►
        </Button>

        {/* Bước hiện tại */}
        <span style={{ lineHeight: '40px', color: '#aaa', fontSize: 13 }}>
          {replayIndex} / {replayTotal}
        </span>

        <Button danger size="large" onClick={() => { setAutoPlay(false); dispatch(exitReplay()); window.close(); }}>
          Thoát xem
        </Button>
      </div>
    );
  }

  // MÀN HÌNH ĐANG CHƠI (side !== 0)
  const renderThirdButton = () => {
    if (mode === 5) {
      return null;
    }
    if (mode === 2) {
      return (
        <Button
          size='large'
          onClick={() => dispatch(toggleAI())}
        >
          {Math.abs(side) === 2 ? 'Tiếp tục' : 'Tạm dừng'}
        </Button>
      );
    } else {
      return (
        <Button
          size='large'
          disabled={historyLength <= 1}
          onClick={() => dispatch(regretMove())}
        >
          Đi lại
        </Button>
      );
    }
  };

  return (
    <div>
      <div style={containerStyle}>
        {/* Nút gợi ý (ẩn khi không dùng được) */}
        {(mode !== 2 && mode !== 5) && (
          <Button
            size='large'
            onClick={() => dispatch(showHint())}
          >
            Gợi ý
          </Button>
        )}

        {renderThirdButton()}

        {/* Nút đổi bên (ẩn ở chế độ Online hoặc khi là chế độ Máy tự đánh) */}
        {(mode !== 5 && !(mode === 2 && Math.abs(side) === 1)) && (
          <Button
            size='large'
            onClick={() => dispatch(changeSide())}
          >
            Đổi bên
          </Button>
        )}

        {/* Nút chấp quân (ẩn ở Online và Máy đánh) */}
        {(mode !== 2 && mode !== 5) && (
          <Button
            size='large'
            onClick={() => dispatch(clearChess())}
          >
            Chấp quân
          </Button>
        )}

        {/* Nút Đầu Hàng hoặc Hòa cờ có thể thêm tại đây cho chế độ Online */}
        <Button
          danger
          size="large"
          onClick={() => {
            if (mode === 5) {
              window.location.reload();
            } else {
              dispatch(onGameOver());
            }
          }}
        >
          {mode === 5 ? 'Đầu hàng' : 'Thoát trận'}
        </Button>
      </div>
      <StartModel visible={showModel} />
    </div>
  );
};

export default ButtonGroup;
