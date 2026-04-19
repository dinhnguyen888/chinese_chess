import React, { useEffect, useState } from 'react';
import { Table, Button, Space, Modal, Form, Input, Select, message, Tooltip, Typography, Tabs, Badge, Tag } from 'antd';
import { EditOutlined, DeleteOutlined, HistoryOutlined, PlusOutlined, UserOutlined, ArrowLeftOutlined, LockOutlined, WarningOutlined, EyeOutlined, CheckCircleOutlined } from '@ant-design/icons';
import { httpApiUrl } from '../../config/server';
import { Link } from 'react-router-dom';
import UserHistoryModal from './UserHistoryModal';
import PunishModal from './PunishModal';

const { Title } = Typography;
const { TabPane } = Tabs;

interface ReportItem {
  id: number;
  reporter: string;
  reported: string;
  match_id: number;
  reason: string;
  status: string;
  created_at: string;
}

interface UserItem {
  id: number;
  username: string;
  role: string;
  banned_until?: string;
  can_chat?: boolean;
  can_create_room?: boolean;
}

const UserManager: React.FC = () => {
  const [users, setUsers] = useState<UserItem[]>([]);
  const [userTotal, setUserTotal] = useState(0);
  const [userPage, setUserPage] = useState(1);
  const [reports, setReports] = useState<ReportItem[]>([]);
  const [reportTotal, setReportTotal] = useState(0);
  const [reportPage, setReportPage] = useState(1);
  const [loading, setLoading] = useState(false);
  const [isModalVisible, setIsModalVisible] = useState(false);
  const [editingUser, setEditingUser] = useState<UserItem | null>(null);
  const [historyTarget, setHistoryTarget] = useState<string | null>(null);
  const [punishTarget, setPunishTarget] = useState<string | null>(null);
  const [activeTab, setActiveTab] = useState('1');
  const [highlightedUser, setHighlightedUser] = useState<string | null>(null);
  const [form] = Form.useForm();

  const fetchReports = async (page = 1) => {
    try {
      const token = localStorage.getItem('chess_jwt_token');
      const res = await fetch(httpApiUrl(`/admin/reports?page=${page}&limit=10`), {
        headers: { Authorization: `Bearer ${token}` }
      });
      if (res.ok) {
        const result = await res.json();
        setReports(result.data);
        setReportTotal(result.total);
        setReportPage(page);
      }
    } catch (e) { console.error(e); }
  };

  const fetchUsers = async (page = 1) => {
    setLoading(true);
    try {
      const token = localStorage.getItem('chess_jwt_token');
      const res = await fetch(httpApiUrl(`/admin/users?page=${page}&limit=10`), {
        headers: { Authorization: `Bearer ${token}` }
      });
      if (res.ok) {
        const result = await res.json();
        setUsers(result.data);
        setUserTotal(result.total);
        setUserPage(page);
      } else {
        message.error('Không tải được danh sách người dùng');
      }
    } catch {
      message.error('Lỗi kết nối máy chủ');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchUsers();
    fetchReports();
  }, []);

  // Scroll + highlight user row when navigating from Reports tab
  useEffect(() => {
    if (activeTab === '1' && highlightedUser) {
      const found = users.find(u => u.username === highlightedUser);
      if (!found) {
        fetchUsers(1);
        return;
      }
      setTimeout(() => {
        const el = document.querySelector(`tr.highlighted-user-row`);
        if (el) {
          el.scrollIntoView({ behavior: 'smooth', block: 'center' });
        }
      }, 150);
    }
  }, [activeTab, highlightedUser, users]);

  const handleCreateOrUpdate = async (values: any) => {
    setLoading(true);
    try {
      const token = localStorage.getItem('chess_jwt_token');
      const method = editingUser ? 'PUT' : 'POST';
      const body = editingUser ? { ...values, id: editingUser.id } : values;

      const res = await fetch(httpApiUrl('/admin/users'), {
        method,
        headers: {
          'Content-Type': 'application/json',
          Authorization: `Bearer ${token}`
        },
        body: JSON.stringify(body)
      });

      if (res.ok) {
        message.success(editingUser ? 'Cập nhật thành công' : 'Tạo mới thành công');
        setIsModalVisible(false);
        fetchUsers();
      } else {
        const err = await res.json();
        message.error(err.error || 'Thao tác thất bại');
      }
    } catch {
      message.error('Lỗi hệ thống');
    } finally {
      setLoading(false);
    }
  };

  const handleDelete = (id: number) => {
    Modal.confirm({
      title: 'Xác nhận xóa',
      content: 'Bạn có chắc chắn muốn xóa người dùng này không?',
      okText: 'Xóa',
      okType: 'danger',
      cancelText: 'Hủy',
      onOk: async () => {
        try {
          const token = localStorage.getItem('chess_jwt_token');
          const res = await fetch(httpApiUrl(`/admin/users?id=${id}`), {
            method: 'DELETE',
            headers: { Authorization: `Bearer ${token}` }
          });
          if (res.ok) {
            message.success('Đã xóa người dùng');
            fetchUsers();
          } else {
            message.error('Không thể xóa');
          }
        } catch {
          message.error('Lỗi kết nối');
        }
      }
    });
  };

  const handleUpdateReportStatus = async (id: number, status: string) => {
    try {
      const token = localStorage.getItem('chess_jwt_token');
      const res = await fetch(httpApiUrl('/admin/reports'), {
        method: 'PUT',
        headers: {
          'Content-Type': 'application/json',
          Authorization: `Bearer ${token}`
        },
        body: JSON.stringify({ id, status })
      });
      if (res.ok) {
        message.success('Cập nhật thành công');
        fetchReports();
      }
    } catch (e) { console.error(e); }
  };

  const columns = [
    { title: 'ID', dataIndex: 'id', key: 'id', width: 80 },
    {
      title: 'Username',
      dataIndex: 'username',
      key: 'username',
      render: (text: string) => <span><UserOutlined style={{ marginRight: 8 }} />{text}</span>
    },
    {
      title: 'Quyền hạn',
      dataIndex: 'role',
      key: 'role',
      render: (role: string) => (
        <span style={{
          color: role === 'admin' ? '#f5222d' : '#1890ff',
          fontWeight: 600,
          textTransform: 'uppercase'
        }}>
          {role}
        </span>
      )
    },
    {
      title: 'Thao tác',
      key: 'action',
      render: (_: any, record: UserItem) => (
        <Space size="middle">
          <Tooltip title="Xử phạt">
            <Button
              shape="circle"
              icon={<WarningOutlined />}
              onClick={() => setPunishTarget(record.username)}
              danger
            />
          </Tooltip>
          <Tooltip title="Lịch sử đấu">
            <Button
              shape="circle"
              icon={<HistoryOutlined />}
              onClick={() => setHistoryTarget(record.username)}
              type="primary"
              ghost
            />
          </Tooltip>
          <Tooltip title="Chỉnh sửa">
            <Button
              shape="circle"
              icon={<EditOutlined />}
              onClick={() => {
                setEditingUser(record);
                form.setFieldsValue({
                  username: record.username,
                  role: record.role,
                  password: ''
                });
                setIsModalVisible(true);
              }}
            />
          </Tooltip>
          <Tooltip title="Xóa">
            <Button
              shape="circle"
              icon={<DeleteOutlined />}
              danger
              onClick={() => handleDelete(record.id)}
            />
          </Tooltip>
        </Space>
      ),
    },
  ];

  const reportColumns = [
    { title: 'Thời gian', dataIndex: 'created_at', key: 'created_at', width: 160 },
    { title: 'Người tố cáo', dataIndex: 'reporter', key: 'reporter' },
    {
      title: 'Bị tố cáo',
      dataIndex: 'reported',
      key: 'reported',
      render: (t: string) => (
        <Button
          type="link"
          onClick={() => {
            setHighlightedUser(t);
            setActiveTab('1');
          }}
          style={{ padding: 0, fontWeight: 'bold', color: '#cf1322' }}
        >
          {t}
        </Button>
      )
    },
    { title: 'Lý do', dataIndex: 'reason', key: 'reason' },
    {
      title: 'Trạng thái',
      dataIndex: 'status',
      key: 'status',
      render: (s: string) => {
        let color = 'gold';
        if (s === 'resolved') color = 'green';
        if (s === 'ignored') color = 'gray';
        return <Tag color={color}>{s.toUpperCase()}</Tag>;
      }
    },
    {
      title: 'Hành động',
      key: 'action',
      render: (_: any, record: ReportItem) => (
        <Space size="small">
          {record.match_id > 0 && (
            <Tooltip title="Xem lại trận đấu">
              <Button
                size="small"
                icon={<EyeOutlined />}
                onClick={() => window.open(`/replay/${record.match_id}`, '_blank')}
              >
                Xem trận
              </Button>
            </Tooltip>
          )}
          {record.status === 'pending' && (
            <>
              <Button
                size="small"
                type="primary"
                icon={<CheckCircleOutlined />}
                onClick={() => setPunishTarget(record.reported)}
              >
                Hành quyết
              </Button>
              <Button
                size="small"
                icon={<DeleteOutlined />}
                onClick={() => handleUpdateReportStatus(record.id, 'ignored')}
              >
                Bỏ qua
              </Button>
            </>
          )}
        </Space>
      )
    }
  ];

  return (
    <div style={{ padding: 24, background: '#f0f2f5', minHeight: '100vh' }}>
      <div style={{ background: '#fff', padding: 24, borderRadius: 12, boxShadow: '0 2px 8px rgba(0,0,0,0.06)' }}>
        <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 24 }}>
          <Space align="center" size="large">
            <Link to="/">
              <Button icon={<ArrowLeftOutlined />} shape="circle" />
            </Link>
            <Title level={2} style={{ margin: 0 }}>Quản trị hệ thống</Title>
          </Space>
        </div>

        <Tabs activeKey={activeTab} onChange={setActiveTab} type="card">
          <TabPane
            tab={<span><UserOutlined />Người dùng</span>}
            key="1"
          >
            <div style={{ display: 'flex', justifyContent: 'flex-end', marginBottom: 16 }}>
              <Button
                type="primary"
                icon={<PlusOutlined />}
                onClick={() => {
                  setEditingUser(null);
                  form.resetFields();
                  setIsModalVisible(true);
                }}
              >
                Thêm người dùng
              </Button>
            </div>
            <Table
              columns={columns}
              dataSource={users}
              rowKey="id"
              loading={loading}
              rowClassName={(record) =>
                record.username === highlightedUser ? 'highlighted-user-row' : ''
              }
              onRow={(record) => ({
                style: record.username === highlightedUser ? {
                  background: 'linear-gradient(90deg, #fff1b8, #fffbe6)',
                  boxShadow: 'inset 3px 0 0 #faad14',
                  transition: 'background 0.3s',
                } : {}
              })}
              pagination={{
                current: userPage,
                total: userTotal,
                pageSize: 10,
                onChange: (page) => { fetchUsers(page); setHighlightedUser(highlightedUser); }
              }}
            />
          </TabPane>

          <TabPane
            tab={
              <span>
                <WarningOutlined />
                Tố cáo gian lận
                {reports.filter(r => r.status === 'pending').length > 0 && (
                  <Badge count={reports.filter(r => r.status === 'pending').length} style={{ marginLeft: 8 }} />
                )}
              </span>
            }
            key="2"
          >
            <Table
              columns={reportColumns}
              dataSource={reports}
              rowKey="id"
              loading={loading}
              pagination={{
                current: reportPage,
                total: reportTotal,
                pageSize: 10,
                onChange: (page) => fetchReports(page)
              }}
            />
          </TabPane>
        </Tabs>

        <Modal
          title={editingUser ? "Cập nhật người dùng" : "Thêm người dùng mới"}
          open={isModalVisible}
          onOk={() => form.submit()}
          onCancel={() => setIsModalVisible(false)}
          confirmLoading={loading}
          destroyOnClose
        >
          <Form
            form={form}
            layout="vertical"
            onFinish={handleCreateOrUpdate}
            initialValues={{ role: 'user' }}
          >
            <Form.Item
              name="username"
              label="Tên đăng nhập"
              rules={[{ required: true, message: 'Vui lòng nhập tên đăng nhập!' }]}
            >
              <Input prefix={<UserOutlined />} />
            </Form.Item>
            <Form.Item
              name="password"
              label={editingUser ? "Mật khẩu mới (để trống nếu không đổi)" : "Mật khẩu"}
              rules={[{ required: !editingUser, message: 'Vui lòng nhập mật khẩu!' }]}
            >
              <Input.Password prefix={<LockOutlined />} />
            </Form.Item>
            <Form.Item
              name="role"
              label="Quyền hạn"
              rules={[{ required: true }]}
            >
              <Select>
                <Select.Option value="user">USER</Select.Option>
                <Select.Option value="admin">ADMIN</Select.Option>
              </Select>
            </Form.Item>
          </Form>
        </Modal>

        {historyTarget && (
          <UserHistoryModal
            username={historyTarget}
            onClose={() => setHistoryTarget(null)}
          />
        )}

        {punishTarget && (
          <PunishModal
            username={punishTarget}
            open={!!punishTarget}
            onClose={() => {
              setPunishTarget(null);
              fetchUsers();
              fetchReports();
            }}
          />
        )}
      </div>
    </div>
  );
};

export default UserManager;
