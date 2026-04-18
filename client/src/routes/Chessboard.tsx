import React from 'react';
import { style } from 'typestyle';
import { useAppSelector } from '../hooks';
import { useOnlinePlay } from '../hooks/useOnlinePlay';
import { useGameHistoryReporter } from '../hooks/useGameHistoryReporter';
import ChessBoardCanvas from '../components/board/ChessBoardCanvas';
import ButtonGroup from '../components/button/ButtonGroup';
import WinnerModel from '../components/button/WinnerModel';
import LobbyModal from '../components/lobby/LobbyModal';
import ChatBox from '../components/lobby/ChatBox';

import background from '../assets/background.png';

// ── Styles ─────────────────────────────────────────────────────────────────────
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
  position: 'relative',
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  width: '100%',
  height: '100%',
});

const chatPanelStyle = style({
  position: 'absolute',
  left: 0,
  top: 0,
  width: '320px',
  height: '100vh',
  zIndex: 10,
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

// ── Component ─────────────────────────────────────────────────────────────────
/**
 * Trang chính. Chỉ lo bố cục (layout):
 *  - LobbyModal       → quản lý màn Lobby/phòng chờ
 *  - ChatBox          → sidebar chat khi đang chơi online
 *  - ChessBoardCanvas → bàn cờ + quân + AI + labels
 *  - ButtonGroup      → nút điều khiển phía dưới
 *  - WinnerModel      → modal kết quả
 */
const ChessBoard: React.FC = () => {
  const { mode, side, showModel, history, color, winner, roomStatus } = useAppSelector(s => s.chess);
  useOnlinePlay();
  useGameHistoryReporter(); // Tự lưu lịch sử khi ván Người vs Máy kết thúc

  const isOnlinePlaying = mode === 5 && roomStatus === 'playing';

  return (
    <div className={shellStyle}>
      {/* Lobby overlay (sảnh / phòng chờ) */}
      <LobbyModal />

      <div className={layoutStyle}>
        {/* Chat sidebar — chỉ hiện khi đang trong ván online */}
        {isOnlinePlaying && (
          <aside className={chatPanelStyle} aria-label="Khung chat">
            <ChatBox />
          </aside>
        )}

        {/* Khu vực game chính */}
        <main className={gameColumnStyle}>
          <ChessBoardCanvas />

          <div className={buttonBarStyle}>
            <ButtonGroup mode={mode} side={side} showModel={showModel} historyLength={history.length} />
          </div>
        </main>
      </div>

      {/* Modal thắng/thua */}
      <WinnerModel mode={mode} color={color} winner={winner} />
    </div>
  );
};

export default ChessBoard;
