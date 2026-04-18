import React, { useEffect, useState } from 'react';
import { Modal, Table, Tag, Typography, Empty, Spin, Statistic, Row, Col, Button } from 'antd';
import { TrophyOutlined, CloseCircleOutlined, MinusCircleOutlined, PlayCircleOutlined } from '@ant-design/icons';
import { globalWs } from '../../hooks/useOnlinePlay';
import { wsUrlWithToken } from '../../config/server';
import { useAppSelector, useAppDispatch } from '../../hooks';
import { startReplay } from '../../models/chessSlice';

interface MatchRecord {
  id: number;
  opponent: string;
  result: 'win' | 'lose' | 'draw';
  played_at: string;
  duration_seconds: number;
}

interface MatchHistoryModalProps {
  open: boolean;
  onClose: () => void;
}

function formatDuration(seconds: number): string {
  if (!seconds) return '—';
  const m = Math.floor(seconds / 60);
  const s = seconds % 60;
  return `${m}p ${s}s`;
}

const MatchHistoryModal: React.FC<MatchHistoryModalProps> = ({ open, onClose }) => {
  const dispatch = useAppDispatch();
  const [records, setRecords] = useState<MatchRecord[]>([]);
  const [loading, setLoading] = useState(false);
  const [replayLoading, setReplayLoading] = useState<number | null>(null);
  const [wsRef, setWsRef] = useState<WebSocket | null>(null);
  const playerName = useAppSelector(s => s.chess.playerName);
  const isLoggedIn = useAppSelector(s => s.chess.isLoggedIn);

  useEffect(() => {
    if (!open || !isLoggedIn) return;

    setLoading(true);
    setRecords([]);

    // Mở kết nối WS tạm thời nếu globalWs không có sẵn
    const fetchViaWs = (ws: WebSocket) => {
      const handler = (ev: MessageEvent) => {
        try {
          const msg = JSON.parse(ev.data as string);
          if (msg.type === 'history_list') {
            setRecords(msg.records ?? []);
            setLoading(false);
            ws.removeEventListener('message', handler);
          }
        } catch { /* ignore */ }
      };
      ws.addEventListener('message', handler);
      ws.send(JSON.stringify({ type: 'get_history' }));
    };

    if (globalWs && globalWs.readyState === WebSocket.OPEN) {
      fetchViaWs(globalWs);
      setWsRef(globalWs);
    } else {
      const token = localStorage.getItem('chess_jwt_token');
      if (!token) {
        setLoading(false);
        return;
      }
      const tempWs = new WebSocket(wsUrlWithToken(token));
      setWsRef(tempWs);
      tempWs.onopen = () => {
        fetchViaWs(tempWs);
      };
      tempWs.onerror = () => setLoading(false);
    }
  }, [open, isLoggedIn]);

  const handleReplay = (matchId: number) => {
    window.open(`/replay/${matchId}`, '_blank');
    onClose();
  };

  const wins  = records.filter(r => r.result === 'win').length;
  const loses = records.filter(r => r.result === 'lose').length;
  const draws = records.filter(r => r.result === 'draw').length;

  const columns = [
    {
      title: '#',
      key: 'index',
      width: 50,
      render: (_: unknown, __: unknown, idx: number) => idx + 1,
    },
    {
      title: 'Đối thủ',
      dataIndex: 'opponent',
      key: 'opponent',
      render: (v: string) => <Typography.Text strong>{v}</Typography.Text>,
    },
    {
      title: 'Kết quả',
      dataIndex: 'result',
      key: 'result',
      render: (v: string) => {
        if (v === 'win')  return <Tag icon={<TrophyOutlined />} color="gold">Thắng</Tag>;
        if (v === 'lose') return <Tag icon={<CloseCircleOutlined />} color="red">Thua</Tag>;
        return <Tag icon={<MinusCircleOutlined />} color="default">Hòa</Tag>;
      },
    },
    {
      title: 'Thời lượng',
      dataIndex: 'duration_seconds',
      key: 'duration',
      render: (v: number) => formatDuration(v),
    },
    {
      title: 'Thời gian',
      dataIndex: 'played_at',
      key: 'played_at',
    },
    {
      title: '',
      key: 'action',
      width: 95,
      render: (_: unknown, record: MatchRecord) => (
        <Button
          type="link"
          size="small"
          icon={<PlayCircleOutlined />}
          loading={replayLoading === record.id}
          onClick={() => handleReplay(record.id)}
        >
          Xem lại
        </Button>
      ),
    },
  ];

  return (
    <Modal
      title={
        <div style={{ display: 'flex', alignItems: 'center', gap: 10 }}>
          <TrophyOutlined style={{ color: '#f5d76e', fontSize: 20 }} />
          <span>Lịch sử đấu của <strong style={{ color: '#1890ff' }}>{playerName}</strong></span>
        </div>
      }
      open={open}
      onCancel={onClose}
      footer={null}
      width={700}
      styles={{ body: { padding: '16px 24px' } }}
    >
      {!isLoggedIn ? (
        <Empty description="Bạn cần đăng nhập để xem lịch sử đấu" />
      ) : loading ? (
        <div style={{ textAlign: 'center', padding: 40 }}>
          <Spin size="large" />
          <div style={{ marginTop: 12, color: '#999' }}>Đang tải lịch sử...</div>
        </div>
      ) : (
        <>
          {/* Thống kê tổng quan */}
          <Row gutter={16} style={{ marginBottom: 20 }}>
            <Col span={6}>
              <Statistic title="Tổng trận" value={records.length} />
            </Col>
            <Col span={6}>
              <Statistic
                title="Thắng" value={wins}
                valueStyle={{ color: '#d4a017' }}
                prefix={<TrophyOutlined />}
              />
            </Col>
            <Col span={6}>
              <Statistic
                title="Thua" value={loses}
                valueStyle={{ color: '#cf1322' }}
                prefix={<CloseCircleOutlined />}
              />
            </Col>
            <Col span={6}>
              <Statistic title="Hòa" value={draws} valueStyle={{ color: '#666' }} />
            </Col>
          </Row>

          {records.length === 0 ? (
            <Empty description="Bạn chưa có trận đấu nào được lưu lại" />
          ) : (
            <Table
              dataSource={records}
              columns={columns}
              rowKey={(r, i) => `${r.played_at}-${i}`}
              pagination={{ pageSize: 10, size: 'small' }}
              size="middle"
              rowClassName={(r) =>
                r.result === 'win' ? 'history-win' : r.result === 'lose' ? 'history-lose' : ''
              }
            />
          )}
        </>
      )}
    </Modal>
  );
};

export default MatchHistoryModal;
