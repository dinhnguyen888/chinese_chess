import React from 'react';
import { style } from 'typestyle';
import { UserOutlined } from '@ant-design/icons';
import { useAppSelector, useAppDispatch } from '../../hooks';
import { boardClick } from '../../models/chessSlice';
import Chess from '../chess/Chess';
import Dot from '../chess/Dot';
import Box from '../chess/Box';
import AI from '../AI/AI';

import bgImg from '../../assets/style/bg.png';

const namePillStyle = style({
  display: 'inline-flex',
  alignItems: 'center',
  gap: 8,
  padding: '8px 16px',
  borderRadius: 999,
  background: 'rgba(0,0,0,0.45)',
  border: '1px solid rgba(255,255,255,0.12)',
  color: 'rgba(255,255,255,0.92)',
  fontSize: 15,
  backdropFilter: 'blur(8px)',
});

const nameAccentStyle = style({ color: '#f5d76e', fontWeight: 600 });

const boardCanvasStyle = style({
  backgroundImage: `url(${bgImg})`,
  width: '507px',
  height: '567px',
  position: 'relative',
  boxShadow: '0 16px 48px rgba(0,0,0,0.45), 0 0 0 1px rgba(255,255,255,0.06) inset',
  borderRadius: '8px',
});

/**
 * Toàn bộ khu vực bàn cờ: label đối thủ → bản cờ → label người chơi.
 * Tự đọc state từ Redux, không cần props.
 */
const ChessBoardCanvas: React.FC = () => {
  const chessState = useAppSelector(state => state.chess);
  const dispatch = useAppDispatch();

  const isOnlinePlaying = chessState.mode === 5 && chessState.roomStatus === 'playing';

  const handleBoardClick = (e: React.MouseEvent<HTMLDivElement>) => {
    const rect = e.currentTarget.getBoundingClientRect();
    dispatch(boardClick({ x: e.clientX, y: e.clientY, offsetLeft: rect.left, offsetTop: rect.top }));
  };

  return (
    <>
      {/* Label đối thủ (trên) */}
      {isOnlinePlaying && (
        <div className={namePillStyle} style={{ marginBottom: 12 }}>
          <UserOutlined />
          <span>Đối thủ:</span>
          <span className={nameAccentStyle}>{chessState.opponentName || 'Đối thủ'}</span>
        </div>
      )}

      {/* Bàn cờ */}
      <div className={boardCanvasStyle} onClick={handleBoardClick}>
        {chessState.board.map((row, i) =>
          row.map((item, j) => {
            if (!item) return null;
            return (
              <Chess
                key={`${item}-${i}-${j}`}
                name={item}
                type={item[0].toLowerCase()}
                side={item[0] >= 'a' ? 1 : -1}
                position={[i, j]}
                color={chessState.color}
                control={chessState.click}
              />
            );
          })
        )}
        {chessState.nextPace?.map((position, idx) => (
          <Dot position={position} key={`dot-${idx}`} />
        ))}
        {chessState.chessChange && (
          <Box chessChange={chessState.chessChange} color={chessState.color} />
        )}
        <AI
          treeDepth={chessState.difficulty}
          mode={chessState.mode}
          board={chessState.board}
          side={chessState.side}
          paceHistory={chessState.paceHistory}
        />
      </div>

      {/* Label người chơi (dưới) */}
      {isOnlinePlaying && (
        <div className={namePillStyle} style={{ marginTop: 12 }}>
          <UserOutlined />
          <span>Bạn:</span>
          <span className={nameAccentStyle}>{chessState.playerName || 'Bạn'}</span>
        </div>
      )}
    </>
  );
};

export default ChessBoardCanvas;
