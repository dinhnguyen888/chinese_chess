import React, { useState } from 'react';
import { Modal, Form, Checkbox, Radio, message, Divider, Input } from 'antd';
import { httpApiUrl } from '../../config/server';

interface PunishModalProps {
  open: boolean;
  onClose: () => void;
  username: string;
}

const PunishModal: React.FC<PunishModalProps> = ({ open, onClose, username }) => {
  const [form] = Form.useForm();
  const [loading, setLoading] = useState(false);

  const handleOk = async () => {
    try {
      const values = await form.validateFields();
      setLoading(true);
      const token = localStorage.getItem('chess_jwt_token');
      
      const res = await fetch(httpApiUrl('/admin/punish'), {
        method: 'POST',
        headers: { 
          'Content-Type': 'application/json',
          Authorization: `Bearer ${token}` 
        },
        body: JSON.stringify({
          username,
          ban_days: values.ban_type === 'none' ? 0 : parseInt(values.ban_type),
          can_chat: !values.actions.includes('mute'),
          can_create_room: !values.actions.includes('no_room'),
          reason: values.reason,
          reporter: values.reporter
        })
      });

      if (res.ok) {
        message.success('Đã áp dụng hình phạt');
        onClose();
      } else {
        message.error('Thao tác thất bại');
      }
    } catch (e) {
      console.log(e);
    } finally {
      setLoading(false);
    }
  };

  return (
    <Modal
      title={<span>Xử lý vi phạm: <strong style={{ color: '#f5222d' }}>{username}</strong></span>}
      open={open}
      onOk={handleOk}
      onCancel={onClose}
      confirmLoading={loading}
      okText="Chấp hành"
      cancelText="Hủy"
    >
      <Form form={form} layout="vertical" initialValues={{ actions: [], ban_type: '0' }}>
        <Form.Item name="actions" label="Hình thức xử phạt (Có thể chọn nhiều)">
          <Checkbox.Group style={{ width: '100%' }}>
            <div style={{ display: 'flex', flexDirection: 'column', gap: '8px' }}>
              <Checkbox value="mute">Cấm chat (Mute)</Checkbox>
              <Checkbox value="no_room">Cấm tạo phòng</Checkbox>
            </div>
          </Checkbox.Group>
        </Form.Item>

        <Divider />

        <Form.Item name="ban_type" label="Khóa tài khoản (Ban Account)">
          <Radio.Group>
            <div style={{ display: 'flex', flexDirection: 'column', gap: '8px' }}>
              <Radio value="0">Không khóa</Radio>
              <Radio value="3">Khóa 3 ngày</Radio>
              <Radio value="5">Khóa 5 ngày</Radio>
              <Radio value="-1">Khóa vĩnh viễn</Radio>
            </div>
          </Radio.Group>
        </Form.Item>

        <Divider />

        <Form.Item name="reason" label="Nguyên do vi phạm" rules={[{ required: true, message: 'Vui lòng nhập lý do' }]}>
          <Input.TextArea rows={2} placeholder="Ví dụ: Gian lận trong trận đấu, xúc phạm người chơi..." />
        </Form.Item>

        <Form.Item name="reporter" label="Người tố cáo" initialValue="Hệ thống">
          <Input placeholder="Tên người tố cáo hoặc 'Hệ thống'" />
        </Form.Item>
      </Form>
    </Modal>
  );
};

export default PunishModal;
