import React, { useState } from 'react';
import { Input, Button, Typography, message, Segmented } from 'antd';
import { UserOutlined, LockOutlined, LoginOutlined, UserAddOutlined } from '@ant-design/icons';
import { useAppSelector, useAppDispatch } from '../../hooks';
import { loginSuccess } from '../../models/chessSlice';
import { httpApiUrl } from '../../config/server';

const AuthScreen: React.FC = () => {
  const dispatch = useAppDispatch();
  const [mode, setMode] = useState<string | number>('login');
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [loading, setLoading] = useState(false);

  const { roomStatus } = useAppSelector(s => s.chess);

  // Chưa đăng nhập trong sảnh online (chờ HTTP login để lấy JWT rồi mới mở WebSocket)
  if (roomStatus !== 'listing') return null;

  const handleSubmit = async () => {
    if (!username.trim() || !password.trim()) {
      message.error("Vui lòng nhập đầy đủ tên đăng nhập và mật khẩu");
      return;
    }
    if (username.trim().length > 10) {
      message.error("Tên đăng nhập không được vượt quá 10 ký tự");
      return;
    }
    if (username.trim().includes(" ") || username.trim().includes("@")) {
      message.warning("Vui lòng sử dụng Tên đăng nhập ngắn gọn, không dùng Email hay chứa dấu cách");
    }

    setLoading(true);
    try {
      const path = mode === 'login' ? '/login' : '/register';
      const res = await fetch(httpApiUrl(path), {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ username: username.trim(), password: password.trim() }),
      });
      const data = (await res.json()) as { 
        type?: string; 
        message?: string; 
        token?: string; 
        username?: string;
        can_chat?: boolean;
        can_create_room?: boolean;
      };

      if (!res.ok) {
        message.error(data.message || 'Có lỗi xảy ra');
        return;
      }
      
      let role = 'user';
      if (data.token) {
        localStorage.setItem('chess_jwt_token', data.token);
        try {
          const payload = JSON.parse(atob(data.token.split('.')[1]));
          role = payload.role || 'user';
        } catch (e) { /* ignore */ }
      }
      
      if (data.username) {
        dispatch(loginSuccess({ 
          username: data.username, 
          role,
          canChat: data.can_chat,
          canCreateRoom: data.can_create_room
        }));
      }
      message.success(mode === 'register' ? 'Đăng ký thành công!' : 'Đăng nhập thành công!');
    } catch {
      message.error('Không kết nối được máy chủ');
    } finally {
      setLoading(false);
    }
  };

  return (
    <div style={{
      position: 'fixed', inset: 0, zIndex: 1000,
      background: 'rgba(255, 255, 255, 0.6)',
      backdropFilter: 'blur(10px)',
      display: 'flex', alignItems: 'center', justifyContent: 'center',
    }}>
      <div style={{
        background: '#ffffff', border: '1px solid #e8e8e8',
        borderRadius: 16, padding: '40px 48px', width: 420,
        boxShadow: '0 20px 60px rgba(0,0,0,0.1)', textAlign: 'center',
        display: 'flex', flexDirection: 'column', gap: 20
      }}>
        <Typography.Title level={3} style={{ color: '#1890ff', margin: 0 }}>
          {mode === 'login' ? 'Đăng nhập' : 'Tạo tài khoản'}
        </Typography.Title>
        <Typography.Text style={{ color: '#666', fontSize: 13 }}>
          Sử dụng tên ngắn gọn (ví dụ: Dinh123), tối đa 10 ký tự
        </Typography.Text>

        <Segmented 
          options={[
            { label: 'Đăng nhập', value: 'login', icon: <LoginOutlined /> },
            { label: 'Đăng ký', value: 'register', icon: <UserAddOutlined /> }
          ]} 
          value={mode}
          onChange={setMode}
          block
          size="large"
        />

        <div style={{ display: 'flex', flexDirection: 'column', gap: 16, marginTop: 8 }}>
          <Input 
            size="large" 
            placeholder="Tên đăng nhập (Tối đa 10 ký tự)" 
            prefix={<UserOutlined style={{ color: '#bfbfbf' }} />} 
            value={username}
            onChange={e => setUsername(e.target.value)}
            maxLength={10}
          />
          <Input.Password 
            size="large" 
            placeholder="Mật khẩu" 
            prefix={<LockOutlined style={{ color: '#bfbfbf' }} />} 
            value={password}
            onChange={e => setPassword(e.target.value)}
            onPressEnter={() => void handleSubmit()}
          />
          
          <Button 
            type="primary" 
            size="large" 
            onClick={() => void handleSubmit()} 
            loading={loading}
            style={{ 
              marginTop: 12, 
              background: mode === 'login' ? 'linear-gradient(90deg, #1890ff, #096dd9)' : 'linear-gradient(90deg, #52c41a, #389e0d)',
              border: 'none',
              fontWeight: 600
            }}
          >
            {mode === 'login' ? 'Vào game' : 'Đăng ký & Vào sảnh'}
          </Button>
        </div>

        <Button onClick={() => window.location.reload()} danger type="text" style={{ marginTop: 8 }}>
          Hủy & Quay lại
        </Button>
      </div>
    </div>
  );
};

export default AuthScreen;
