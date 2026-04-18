import React, { useEffect } from 'react';
import { Button, Typography, Alert } from 'antd';
import { TeamOutlined, SettingOutlined, WarningOutlined } from '@ant-design/icons';
import { useLobby } from '../../hooks/useLobby';
import { useAppSelector } from '../../hooks';
import { Link } from 'react-router-dom';
import CreateRoomPanel from './CreateRoomPanel';
import RoomList from './RoomList';

const LobbyScreen: React.FC = () => {
  const { exitLobby } = useLobby();
  const userRole = useAppSelector(s => s.chess.userRole);
  const [punishment, setPunishment] = React.useState<string | null>(null);

  useEffect(() => {
    // Đọc cookie punishment_info
    const cookies = document.cookie.split(';');
    const pCookie = cookies.find(c => c.trim().startsWith('punishment_info='));
    if (pCookie) {
      setPunishment(decodeURIComponent(pCookie.split('=')[1].trim()));
    }
  }, []);

  return (
    <div style={{
      position: 'fixed', inset: 0, zIndex: 1000,
      background: 'linear-gradient(135deg, rgba(20,24,36,0.97) 0%, rgba(36,28,20,0.97) 100%)',
      display: 'flex', alignItems: 'center', justifyContent: 'center',
    }}>
      <div style={{
        background: 'rgba(255,255,255,0.05)', border: '1px solid rgba(255,255,255,0.12)',
        borderRadius: 16, padding: '32px 36px', width: 660, maxHeight: '85vh',
        backdropFilter: 'blur(20px)', boxShadow: '0 20px 60px rgba(0,0,0,0.6)',
        display: 'flex', flexDirection: 'column', gap: 20, overflowY: 'auto',
      }}>
        <Typography.Title level={3} style={{ color: '#f5d76e', margin: 0, textAlign: 'center' }}>
          <TeamOutlined style={{ marginRight: 10 }} />Sảnh Cờ Tướng
        </Typography.Title>
        
        {punishment && (
          <Alert
            message="THÔNG BÁO TỪ ADMIN"
            description={punishment}
            type="error"
            showIcon
            icon={<WarningOutlined />}
            closable
            onClose={() => {
              // Xóa cookie khi đóng alert
              document.cookie = "punishment_info=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;";
            }}
          />
        )}

        <CreateRoomPanel />
        <RoomList />

        <div style={{ textAlign: 'center', display: 'flex', justifyContent: 'center', gap: 15 }}>
          {userRole === 'admin' && (
            <Link to="/admin">
              <Button icon={<SettingOutlined />} type="primary" ghost>
                Admin Dashboard
              </Button>
            </Link>
          )}
          <Button onClick={exitLobby} danger type="text" style={{ color: 'rgba(255,100,100,0.6)' }}>
            Thoát
          </Button>
        </div>
      </div>
    </div>
  );
};

export default LobbyScreen;
