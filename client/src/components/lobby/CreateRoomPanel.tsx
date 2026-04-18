import React, { useState } from 'react';
import { Input, Button, Typography, Space, Checkbox } from 'antd';
import { PlusOutlined } from '@ant-design/icons';
import { useLobby } from '../../hooks/useLobby';
import { useAppSelector } from '../../hooks';

/**
 * Panel tạo phòng mới với tùy chọn "Tự bắt đầu".
 * Dùng trong LobbyScreen.
 */
const CreateRoomPanel: React.FC = () => {
  const [roomName, setRoomName] = useState('');
  const [autoStart, setAutoStart] = useState(false);
  const { createRoom } = useLobby();
  const canCreateRoom = useAppSelector(s => s.chess.canCreateRoom);

  const handleCreate = () => {
    if (!roomName.trim()) return;
    createRoom(roomName.trim(), autoStart);
    setRoomName('');
  };

  return (
    <div style={{
      background: 'rgba(255,255,255,0.04)', border: '1px solid rgba(255,255,255,0.1)',
      borderRadius: 10, padding: '14px 16px',
    }}>
      <Typography.Text style={{ color: 'rgba(255,255,255,0.7)', fontSize: 13, display: 'block', marginBottom: 10 }}>
        <PlusOutlined style={{ marginRight: 6 }} />Tạo phòng mới
      </Typography.Text>
      <Space style={{ width: '100%' }}>
        <Input
          placeholder="Tên phòng..."
          value={roomName}
          onChange={e => setRoomName(e.target.value)}
          onPressEnter={handleCreate}
          style={{ background: 'rgba(0,0,0,0.3)', borderColor: 'rgba(255,255,255,0.15)', color: '#fff', width: 240 }}
        />
        <Checkbox
          checked={autoStart}
          onChange={e => setAutoStart(e.target.checked)}
          style={{ color: 'rgba(255,255,255,0.7)', whiteSpace: 'nowrap' }}
        >
          Tự bắt đầu
        </Checkbox>
        <Button 
          type="primary" 
          icon={<PlusOutlined />} 
          onClick={handleCreate} 
          disabled={!roomName.trim() || !canCreateRoom}
        >
          {canCreateRoom ? "Tạo" : "Bị cấm"}
        </Button>
      </Space>
    </div>
  );
};

export default CreateRoomPanel;
