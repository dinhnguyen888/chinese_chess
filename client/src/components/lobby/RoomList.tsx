import React, { useState } from 'react';
import { Input, Button, List, Tag } from 'antd';
import { ReloadOutlined, LoginOutlined } from '@ant-design/icons';
import { useAppSelector } from '../../hooks';
import { useLobby } from '../../hooks/useLobby';
import type { RoomInfo } from '../../models/chessSlice';

/**
 * Danh sách phòng hiện có, kèm tìm kiếm và làm mới.
 * Dùng trong LobbyScreen.
 */
const RoomList: React.FC = () => {
  const rooms = useAppSelector(s => s.chess.rooms);
  const playerName = useAppSelector(s => s.chess.playerName);
  const [search, setSearch] = useState('');
  const { joinRoom, searchRoom, refreshRooms } = useLobby();

  const handleSearch = () => searchRoom(search.trim());

  return (
    <div style={{ flex: 1 }}>
      <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 12 }}>
        <Input.Search
          placeholder="Tìm ID hoặc tên phòng..."
          value={search}
          onChange={e => setSearch(e.target.value)}
          onSearch={handleSearch}
          style={{ width: 280 }}
        />
        <Button
          icon={<ReloadOutlined />}
          onClick={refreshRooms}
          style={{ color: 'rgba(255,255,255,0.7)', borderColor: 'rgba(255,255,255,0.15)', background: 'transparent' }}
        >
          Làm mới
        </Button>
      </div>

      <List<RoomInfo>
        dataSource={rooms}
        locale={{ emptyText: <span style={{ color: 'rgba(255,255,255,0.3)' }}>Chưa có phòng nào. Hãy tạo phòng mới!</span> }}
        renderItem={item => (
          <List.Item
            style={{
              background: 'rgba(255,255,255,0.04)', borderRadius: 8, marginBottom: 8,
              border: '1px solid rgba(255,255,255,0.08)', padding: '10px 16px',
            }}
            actions={[
              <Button
                key="join"
                disabled={item.isPlaying || item.host === playerName}
                type="primary"
                icon={<LoginOutlined />}
                onClick={() => joinRoom(item.id)}
                size="small"
                danger={item.isPlaying}
              >
                {item.host === playerName ? 'Phòng bạn' : item.isPlaying ? 'Đang chơi' : 'Vào phòng'}
              </Button>
            ]}
          >
            <List.Item.Meta
              title={<span style={{ color: '#f5d76e', fontWeight: 600 }}>#{item.id} · {item.name}</span>}
              description={
                <span style={{ color: 'rgba(255,255,255,0.5)', fontSize: 13 }}>
                  🔴 {item.host}
                  {item.guest
                    ? <span style={{ marginLeft: 12 }}>⚫ {item.guest}</span>
                    : <span style={{ marginLeft: 12, color: 'rgba(255,255,255,0.25)' }}>⚫ Chờ khách...</span>
                  }
                  {item.autoStart && <Tag color="blue" style={{ marginLeft: 12, fontSize: 11 }}>Tự bắt đầu</Tag>}
                </span>
              }
            />
          </List.Item>
        )}
        style={{ maxHeight: 320, overflowY: 'auto' }}
      />
    </div>
  );
};

export default RoomList;
