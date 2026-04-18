import React from 'react';
import { Button, Typography, Tag, Spin, Divider } from 'antd';
import { UserOutlined, PlayCircleOutlined } from '@ant-design/icons';
import { useAppSelector } from '../../hooks';
import { useLobby } from '../../hooks/useLobby';

/** Slot avatar của một người chơi */
const PlayerSlot: React.FC<{
  label: string;
  tagColor: string;
  tagText: string;
  name: string;
  filled: boolean;
  isRed?: boolean;
}> = ({ tagColor, tagText, name, filled, isRed }) => (
  <div style={{ textAlign: 'center' }}>
    <div style={{
      width: 80, height: 80, borderRadius: '50%',
      background: filled
        ? isRed
          ? 'linear-gradient(135deg, #e74c3c, #c0392b)'
          : 'linear-gradient(135deg, #2c3e50, #1a252f)'
        : 'rgba(255,255,255,0.06)',
      display: 'flex', alignItems: 'center', justifyContent: 'center', margin: '0 auto 12px',
      border: filled ? '2px solid rgba(255,255,255,0.2)' : '2px dashed rgba(255,255,255,0.15)',
      boxShadow: filled ? `0 4px 16px ${isRed ? 'rgba(231,76,60,0.4)' : 'rgba(0,0,0,0.3)'}` : 'none',
    }}>
      {filled ? <UserOutlined style={{ fontSize: 32, color: '#fff' }} /> : <Spin size="default" />}
    </div>
    <Tag color={tagColor} style={{ fontSize: 13, padding: '2px 12px' }}>{tagText}</Tag>
    <div style={{ marginTop: 8, fontWeight: 600, fontSize: 15, color: filled ? (isRed ? '#f5d76e' : '#ccc') : 'rgba(255,255,255,0.3)' }}>
      {name}
    </div>
  </div>
);

/**
 * Màn hình phòng chờ sau khi tạo/join phòng.
 * Host thấy nút Bắt đầu; Guest thấy spinner chờ.
 */
const RoomWaitingScreen: React.FC = () => {
  const playerName = useAppSelector(s => s.chess.playerName);
  const currentRoomId = useAppSelector(s => s.chess.currentRoomId);
  const currentRoomName = useAppSelector(s => s.chess.currentRoomName);
  const guestName = useAppSelector(s => s.chess.guestName);
  const opponentName = useAppSelector(s => s.chess.opponentName);
  const { startGame, exitLobby } = useLobby();

  // Host: biết currentRoomName (do tạo phòng), chưa có opponentName
  const isHost = !!currentRoomName && !opponentName;
  const hasGuest = !!guestName;

  return (
    <div style={{
      position: 'fixed', inset: 0, zIndex: 1000,
      background: 'linear-gradient(135deg, rgba(20,24,36,0.97) 0%, rgba(36,28,20,0.97) 100%)',
      display: 'flex', alignItems: 'center', justifyContent: 'center',
    }}>
      <div style={{
        background: 'rgba(255,255,255,0.05)', border: '1px solid rgba(255,255,255,0.12)',
        borderRadius: 16, padding: '40px 48px', width: 520, backdropFilter: 'blur(20px)',
        boxShadow: '0 20px 60px rgba(0,0,0,0.6)', textAlign: 'center',
      }}>
        {/* Tiêu đề */}
        <Typography.Title level={3} style={{ color: '#f5d76e', marginBottom: 4 }}>
          {isHost ? `🏠 Phòng: ${currentRoomName}` : `⚔️ Phòng của ${opponentName}`}
        </Typography.Title>
        <Typography.Text style={{ color: 'rgba(255,255,255,0.5)', fontSize: 13, display: 'block', marginBottom: 32 }}>
          ID Phòng: <strong style={{ color: '#fff' }}>#{currentRoomId}</strong>
        </Typography.Text>

        {/* Slot 2 người */}
        <div style={{ display: 'flex', justifyContent: 'center', gap: 32, marginBottom: 36 }}>
          <PlayerSlot
            label="host"
            tagColor="red" tagText="Đỏ (Chủ phòng)"
            name={isHost ? playerName : opponentName}
            filled isRed
          />
          <div style={{ display: 'flex', alignItems: 'center', color: 'rgba(255,255,255,0.3)', fontSize: 24, fontWeight: 700 }}>
            VS
          </div>
          <PlayerSlot
            label="guest"
            tagColor="default" tagText="Đen (Khách)"
            name={isHost ? (hasGuest ? guestName : 'Đang chờ...') : playerName}
            filled={hasGuest || !isHost}
          />
        </div>

        {/* Nút điều khiển */}
        {isHost ? (
          <div>
            <Button
              type="primary" size="large" icon={<PlayCircleOutlined />}
              disabled={!hasGuest}
              onClick={startGame}
              style={{
                width: '100%', height: 48, fontSize: 16, borderRadius: 8,
                background: hasGuest ? 'linear-gradient(90deg, #e74c3c, #c0392b)' : undefined,
                border: 'none', marginBottom: 12,
              }}
            >
              {hasGuest ? 'Bắt đầu trận đấu!' : 'Chờ người chơi vào...'}
            </Button>
            <Typography.Text style={{ color: 'rgba(255,255,255,0.4)', fontSize: 12 }}>
              Chia sẻ ID phòng <strong style={{ color: '#f5d76e' }}>#{currentRoomId}</strong> cho bạn bè
            </Typography.Text>
          </div>
        ) : (
          <div style={{ color: 'rgba(255,255,255,0.6)', fontSize: 15 }}>
            <Spin size="small" style={{ marginRight: 10 }} />
            Chờ chủ phòng bắt đầu trận đấu...
          </div>
        )}

        <Divider style={{ borderColor: 'rgba(255,255,255,0.1)', margin: '28px 0 16px' }} />
        <Button onClick={exitLobby} danger type="text" style={{ color: 'rgba(255,100,100,0.7)' }}>
          Rời phòng
        </Button>
      </div>
    </div>
  );
};

export default RoomWaitingScreen;
