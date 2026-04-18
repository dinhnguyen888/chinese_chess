import React, { useState, useEffect, useRef } from 'react';
import { Input, Button, Typography, Space } from 'antd';
import { useAppSelector } from '../../hooks';
import { globalWs } from '../../hooks/useOnlinePlay';

const ChatBox: React.FC = () => {
  const chat = useAppSelector(s => s.chess.chat);
  const mode = useAppSelector(s => s.chess.mode);
  const roomStatus = useAppSelector(s => s.chess.roomStatus);
  const canChat = useAppSelector(s => s.chess.canChat);
  const [msg, setMsg] = useState('');
  const messagesEndRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [chat]);

  if (mode !== 5 || roomStatus !== 'playing') return null;

  const handleSend = () => {
    if (!msg.trim()) return;
    if (globalWs && globalWs.readyState === WebSocket.OPEN) {
      globalWs.send(JSON.stringify({ type: 'chat', message: msg.trim() }));
      setMsg('');
    }
  };

  return (
    <div style={{
      width: '100%', height: '100%', background: 'rgba(0,0,0,0.6)', 
      padding: 12, display: 'flex', flexDirection: 'column', 
      borderRight: '1px solid #444',
    }}>
      <div style={{ flex: 1, overflowY: 'auto', marginBottom: 8, paddingRight: 8 }}>
        {chat.map((c, i) => (
          <div key={i} style={{ marginBottom: 4 }}>
            <Typography.Text style={{ color: '#aaa', fontSize: 13 }}>[{c.sender}]: </Typography.Text>
            <Typography.Text style={{ color: '#fff', fontSize: 14 }}>{c.message}</Typography.Text>
          </div>
        ))}
        <div ref={messagesEndRef} />
      </div>
      <Space.Compact style={{ width: '100%' }}>
        <Input 
          value={msg} 
          onChange={e => setMsg(e.target.value)} 
          placeholder={canChat ? "Nhập tin nhắn..." : "Bạn đã bị cấm chat"} 
          onPressEnter={handleSend}
          disabled={!canChat}
          style={{ background: '#333', color: '#fff', borderColor: '#444' }}
        />
        <Button type="primary" onClick={handleSend} disabled={!canChat}>Gửi</Button>
      </Space.Compact>
    </div>
  );
};
export default ChatBox;
