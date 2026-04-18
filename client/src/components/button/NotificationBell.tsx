import React, { useState, useEffect, useRef } from 'react';
import { Badge, Button, Drawer, List, Tag, Empty } from 'antd';
import { BellOutlined, DeleteOutlined, CheckOutlined } from '@ant-design/icons';

export interface SystemNotification {
  id: string;
  reason: string;
  reporter: string;
  message: string;
  banDays: number;
  canChat: boolean;
  canCreateRoom: boolean;
  createdAt: number; // timestamp ms
  read: boolean;
}

const COOKIE_KEY = 'chess_notifications';
const SEVEN_DAYS_MS = 7 * 24 * 60 * 60 * 1000;

/**
 * Đọc danh sách thông báo từ cookie, tự động loại bỏ thông báo quá 7 ngày.
 */
export function getNotifications(): SystemNotification[] {
  try {
    const match = document.cookie.match(new RegExp('(?:^|; )' + COOKIE_KEY + '=([^;]*)'));
    if (!match) return [];
    const list: SystemNotification[] = JSON.parse(decodeURIComponent(match[1]));
    const now = Date.now();
    // Lọc bỏ thông báo quá 7 ngày
    return list.filter(n => now - n.createdAt < SEVEN_DAYS_MS);
  } catch {
    return [];
  }
}

/**
 * Lưu danh sách thông báo vào cookie (expire 7 ngày kể từ bây giờ).
 */
export function saveNotifications(list: SystemNotification[]) {
  const now = Date.now();
  // Lọc bỏ thông báo quá 7 ngày trước khi lưu
  const filtered = list.filter(n => now - n.createdAt < SEVEN_DAYS_MS);
  const d = new Date();
  d.setTime(d.getTime() + SEVEN_DAYS_MS);
  document.cookie = COOKIE_KEY + '=' + encodeURIComponent(JSON.stringify(filtered)) + ';expires=' + d.toUTCString() + ';path=/';
}

/**
 * Thêm một thông báo mới (gọi từ WebSocket handler).
 */
export function pushNotification(reason: string, reporter: string, msg: string, banDays: number, canChat: boolean, canCreateRoom: boolean) {
  const list = getNotifications();
  const notification: SystemNotification = {
    id: Date.now().toString(36) + Math.random().toString(36).slice(2, 6),
    reason,
    reporter,
    message: msg,
    banDays,
    canChat,
    canCreateRoom,
    createdAt: Date.now(),
    read: false,
  };
  list.unshift(notification); // Mới nhất lên đầu
  saveNotifications(list);
  // Dispatch custom event để component cập nhật real-time
  window.dispatchEvent(new CustomEvent('chess_notification_update'));
}

const NotificationBell: React.FC = () => {
  const [open, setOpen] = useState(false);
  const [notifications, setNotifications] = useState<SystemNotification[]>([]);
  const intervalRef = useRef<ReturnType<typeof setInterval> | null>(null);

  const reload = () => {
    setNotifications(getNotifications());
  };

  useEffect(() => {
    reload();
    // Lắng nghe event real-time khi có thông báo mới
    const handler = () => reload();
    window.addEventListener('chess_notification_update', handler);
    // Mỗi 60s tự dọn thông báo hết hạn
    intervalRef.current = setInterval(reload, 60_000);
    return () => {
      window.removeEventListener('chess_notification_update', handler);
      if (intervalRef.current) clearInterval(intervalRef.current);
    };
  }, []);

  const unreadCount = notifications.filter(n => !n.read).length;

  const markAllRead = () => {
    const list = getNotifications().map(n => ({ ...n, read: true }));
    saveNotifications(list);
    reload();
  };

  const deleteNotification = (id: string) => {
    const list = getNotifications().filter(n => n.id !== id);
    saveNotifications(list);
    reload();
  };

  const clearAll = () => {
    saveNotifications([]);
    reload();
  };

  const formatTime = (ts: number) => {
    const d = new Date(ts);
    return d.toLocaleDateString('vi-VN') + ' ' + d.toLocaleTimeString('vi-VN', { hour: '2-digit', minute: '2-digit' });
  };

  const timeAgo = (ts: number) => {
    const diff = Date.now() - ts;
    const mins = Math.floor(diff / 60000);
    if (mins < 1) return 'Vừa xong';
    if (mins < 60) return `${mins} phút trước`;
    const hours = Math.floor(mins / 60);
    if (hours < 24) return `${hours} giờ trước`;
    const days = Math.floor(hours / 24);
    return `${days} ngày trước`;
  };

  return (
    <>
      <Badge count={unreadCount} size="small" offset={[-2, 2]}>
        <Button
          size="large"
          icon={<BellOutlined />}
          onClick={() => { setOpen(true); reload(); }}
          style={{
            border: unreadCount > 0 ? '1.5px solid #ff4d4f' : undefined,
            animation: unreadCount > 0 ? 'bellShake 0.5s ease-in-out' : undefined,
          }}
        >
          Thông báo
        </Button>
      </Badge>

      <Drawer
        title={
          <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center' }}>
            <span>🔔 Thông báo hệ thống</span>
            <div style={{ display: 'flex', gap: 8 }}>
              {unreadCount > 0 && (
                <Button size="small" icon={<CheckOutlined />} onClick={markAllRead}>
                  Đọc hết
                </Button>
              )}
              {notifications.length > 0 && (
                <Button size="small" danger icon={<DeleteOutlined />} onClick={clearAll}>
                  Xóa hết
                </Button>
              )}
            </div>
          </div>
        }
        placement="right"
        open={open}
        onClose={() => setOpen(false)}
        width={380}
        styles={{
          body: { padding: 0 },
        }}
      >
        {notifications.length === 0 ? (
          <Empty
            description="Không có thông báo"
            style={{ marginTop: 60 }}
          />
        ) : (
          <List
            dataSource={notifications}
            renderItem={(item) => (
              <List.Item
                key={item.id}
                style={{
                  padding: '12px 16px',
                  background: item.read ? 'transparent' : 'rgba(255, 77, 79, 0.04)',
                  borderLeft: item.read ? 'none' : '3px solid #ff4d4f',
                  cursor: 'pointer',
                }}
                onClick={() => {
                  // Đánh dấu đã đọc khi click
                  const list = getNotifications().map(n =>
                    n.id === item.id ? { ...n, read: true } : n
                  );
                  saveNotifications(list);
                  reload();
                }}
                actions={[
                  <Button
                    key="del"
                    type="text"
                    danger
                    size="small"
                    icon={<DeleteOutlined />}
                    onClick={(e) => { e.stopPropagation(); deleteNotification(item.id); }}
                  />
                ]}
              >
                <List.Item.Meta
                  title={
                    <div style={{ display: 'flex', alignItems: 'center', gap: 6 }}>
                      {!item.read && <span style={{ width: 6, height: 6, borderRadius: '50%', background: '#ff4d4f', display: 'inline-block' }} />}
                      <Tag color="red">Xử phạt</Tag>
                      <span style={{ fontSize: 11, color: '#999', marginLeft: 'auto' }}>{timeAgo(item.createdAt)}</span>
                    </div>
                  }
                  description={
                    <div style={{ fontSize: 13 }}>
                      <div><strong>Lý do:</strong> {item.reason}</div>
                      <div><strong>Người tố cáo:</strong> {item.reporter}</div>
                      
                      <div style={{ marginTop: 8 }}>
                        <strong>Hình phạt áp dụng:</strong>
                        <div style={{ display: 'flex', gap: 6, flexWrap: 'wrap', marginTop: 4 }}>
                          {item.banDays > 0 && <Tag color="error">Khóa tài khoản {item.banDays} ngày</Tag>}
                          {!item.canChat && <Tag color="warning">Cấm Chat</Tag>}
                          {!item.canCreateRoom && <Tag color="warning">Cấm Tạo Phòng</Tag>}
                          {item.banDays === 0 && item.canChat && item.canCreateRoom && <Tag color="default">Cảnh cáo</Tag>}
                        </div>
                      </div>

                      <div style={{ color: '#666', marginTop: 8 }}>Chi tiết: {item.message}</div>
                      <div style={{ fontSize: 11, color: '#bbb', marginTop: 4 }}>{formatTime(item.createdAt)}</div>
                    </div>
                  }
                />
              </List.Item>
            )}
          />
        )}
      </Drawer>

      <style>{`
        @keyframes bellShake {
          0%, 100% { transform: rotate(0); }
          20% { transform: rotate(12deg); }
          40% { transform: rotate(-12deg); }
          60% { transform: rotate(8deg); }
          80% { transform: rotate(-8deg); }
        }
      `}</style>
    </>
  );
};

export default NotificationBell;
