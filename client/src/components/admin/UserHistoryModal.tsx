import React, { useEffect, useState } from 'react';
import { Modal, Table, Button, Tag, message, Space, Tooltip } from 'antd';
import { EyeOutlined, WarningOutlined } from '@ant-design/icons';
import { httpApiUrl } from '../../config/server';
import { useAppDispatch } from '../../hooks';
import { startReplay } from '../../models/chessSlice';

interface MatchRecord {
  id: number;
  opponent: string;
  result: string;
  played_at: string;
  duration_seconds: number;
  has_report: boolean;
}

interface UserHistoryModalProps {
  username: string;
  onClose: () => void;
}

const UserHistoryModal: React.FC<UserHistoryModalProps> = ({ username, onClose }) => {
  const [history, setHistory] = useState<MatchRecord[]>([]);
  const [loading, setLoading] = useState(false);
  const dispatch = useAppDispatch();

  useEffect(() => {
    const fetchHistory = async () => {
      setLoading(true);
      try {
        const token = localStorage.getItem('chess_jwt_token');
        const res = await fetch(httpApiUrl(`/admin/users/history?username=${username}`), {
          headers: { Authorization: `Bearer ${token}` }
        });
        if (res.ok) {
          const data = await res.json();
          setHistory(data);
        } else {
          message.error('Không tải được lịch sử đấu');
        }
      } catch {
        message.error('Lỗi kết nối');
      } finally {
        setLoading(false);
      }
    };
    fetchHistory();
  }, [username]);

  const handleViewReplay = (matchId: number) => {
    window.open(`/replay/${matchId}`, '_blank');
  };

  const columns = [
    { title: 'Thời gian', dataIndex: 'played_at', key: 'played_at', width: 180 },
    { title: 'Đối thủ', dataIndex: 'opponent', key: 'opponent' },
    { 
      title: 'Kết quả', 
      dataIndex: 'result', 
      key: 'result',
      render: (res: string, record: MatchRecord) => {
        const color = res === 'win' ? 'green' : (res === 'loss' ? 'red' : 'orange');
        return (
          <Space>
            <Tag color={color}>{res.toUpperCase()}</Tag>
            {record.has_report && (
              <Tooltip title="Trận đấu này có báo cáo vi phạm">
                <WarningOutlined style={{ color: '#faad14', fontSize: '16px' }} />
              </Tooltip>
            )}
          </Space>
        );
      }
    },
    {
      title: 'Xem',
      key: 'view',
      render: (_: any, record: MatchRecord) => (
        <Button 
          icon={<EyeOutlined />} 
          type="text" 
          onClick={() => handleViewReplay(record.id)} 
        />
      )
    }
  ];

  return (
    <Modal
      title={`Lịch sử đấu của ${username}`}
      open={true}
      onCancel={onClose}
      footer={null}
      width={700}
      style={{ top: 40 }}
    >
      <Table 
        columns={columns} 
        dataSource={history} 
        rowKey="id" 
        loading={loading}
        pagination={{ pageSize: 8 }}
      />
    </Modal>
  );
};

export default UserHistoryModal;
