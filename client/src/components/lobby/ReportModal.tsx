import React, { useState } from 'react';
import { Modal, Form, Select, Input, message } from 'antd';
import { httpApiUrl } from '../../config/server';

interface ReportModalProps {
  open: boolean;
  onClose: () => void;
  reportedUser: string;
  matchId?: number;
}

const ReportModal: React.FC<ReportModalProps> = ({ open, onClose, reportedUser, matchId }) => {
  const [form] = Form.useForm();
  const [submitting, setSubmitting] = useState(false);

  const handleOk = async () => {
    try {
      const values = await form.validateFields();
      setSubmitting(true);
      const token = localStorage.getItem('chess_jwt_token');
      const res = await fetch(httpApiUrl('/report'), {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          token,
          reported: reportedUser,
          reason: values.reason === 'khác' ? values.otherReason : values.reason,
          match_id: matchId || 0
        })
      });

      if (res.ok) {
        message.success('Đã gửi báo cáo. Cảm ơn bạn đã đóng góp!');
        form.resetFields();
        onClose();
      } else {
        message.error('Gửi báo cáo thất bại');
      }
      setSubmitting(false);

    } catch (e) {
      console.log('Validate Failed:', e);
    }
  };

  return (
    <Modal
      title="Tố cáo người chơi"
      open={open}
      onOk={handleOk}
      onCancel={onClose}
      confirmLoading={submitting}
      okText="Gửi báo cáo"
      cancelText="Hủy"
    >
      <div style={{ marginBottom: 16 }}>
        Đang tố cáo người chơi: <strong style={{ color: '#f5222d' }}>{reportedUser}</strong>
      </div>
      <Form form={form} layout="vertical">
        <Form.Item
          name="reason"
          label="Lý do tố cáo"
          rules={[{ required: true, message: 'Vui lòng chọn lý do' }]}
        >
          <Select placeholder="Chọn lý do">
            <Select.Option value="sử dụng phần mềm can thiệp">Sử dụng phần mềm can thiệp (Cheat)</Select.Option>
            <Select.Option value="câu giờ / không đánh">Câu giờ / Không đánh</Select.Option>
            <Select.Option value="ngôn từ thô tục">Ngôn từ thô tục / Xúc phạm</Select.Option>
            <Select.Option value="khác">Khác</Select.Option>
          </Select>
        </Form.Item>

        <Form.Item
          noStyle
          shouldUpdate={(prevValues, currentValues) => prevValues.reason !== currentValues.reason}
        >
          {({ getFieldValue }) =>
            getFieldValue('reason') === 'khác' ? (
              <Form.Item
                name="otherReason"
                label="Chi tiết lý do"
                rules={[{ required: true, message: 'Vui lòng nhập chi tiết' }]}
              >
                <Input.TextArea rows={3} placeholder="Mô tả thêm về hành vi vi phạm..." />
              </Form.Item>
            ) : null
          }
        </Form.Item>
      </Form>
    </Modal>
  );
};

export default ReportModal;
