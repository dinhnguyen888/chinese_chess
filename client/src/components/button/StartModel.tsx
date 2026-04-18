import React, { useState } from 'react';
import { Modal, Radio } from 'antd';
import { useAppDispatch } from '../../hooks';
import { onModelOK, onModelCancel, beginOnlineMatch } from '../../models/chessSlice';

const RadioButton = Radio.Button;
const RadioGroup = Radio.Group;

interface StartModelProps {
  visible: boolean;
}

export interface GameOptions {
  mode: number;
  difficulty: number;
  side: number;
  color: string;
}

const StartModel: React.FC<StartModelProps> = ({ visible }) => {
  const dispatch = useAppDispatch();
  const [confirmLoading, setConfirmLoading] = useState(false);
  const [options, setOptions] = useState<GameOptions>({
    mode: 1,
    difficulty: 2,
    side: 1,
    color: 'r'
  });

  const handleOk = () => {
    setConfirmLoading(true);
    setTimeout(() => {
      setConfirmLoading(false);
      if (options.mode === 5) {
        // Just trigger online mode without specifying a name 
        dispatch(beginOnlineMatch(""));
      } else {
        dispatch(onModelOK(options));
      }
    }, 1000);
  };

  const handleCancel = () => {
    dispatch(onModelCancel());
  };

  const radioButtonStyle = { margin: '0 15px' };

  return (
    <Modal
      title={<h2><strong>Tùy chọn trò chơi</strong></h2>}
      open={visible}
      onOk={handleOk}
      onCancel={handleCancel}
      confirmLoading={confirmLoading}
      okText="Xác nhận"
      cancelText="Hủy"
    >
      <div style={{ fontSize: 16 }}>
        Chế độ chơi:
        <RadioGroup 
          onChange={(e) => setOptions({ ...options, mode: e.target.value })} 
          value={options.mode} 
          size='large'
        >
          <RadioButton value={1} style={radioButtonStyle}>Người vs Máy</RadioButton>
          <RadioButton value={2} style={radioButtonStyle}>Máy vs Máy</RadioButton>
          <RadioButton value={3} style={radioButtonStyle}>Người vs Người</RadioButton>
          <RadioButton value={5} style={radioButtonStyle}>Chơi online</RadioButton>
        </RadioGroup>
      </div>
      {(options.mode !== 5 && options.mode !== 3) && (
      <div style={{ fontSize: 16, marginTop: 16 }}>
        Độ khó:
        <RadioGroup 
          onChange={(e) => setOptions({ ...options, difficulty: e.target.value })} 
          value={options.difficulty} 
          size='large'
        >
          <RadioButton value={2} style={radioButtonStyle}>Gà mờ</RadioButton>
          <RadioButton value={3} style={radioButtonStyle}>Nhập môn</RadioButton>
          <RadioButton value={4} style={radioButtonStyle}>Bậc thầy</RadioButton>
        </RadioGroup>
      </div>
      )}
      {options.mode !== 5 && (
      <div style={{ fontSize: 16, marginTop: 16 }}>
        Thứ tự đi:
        <RadioGroup 
          onChange={(e) => setOptions({ ...options, side: e.target.value })} 
          value={options.side} 
          size='large'
        >
          <RadioButton value={1} style={radioButtonStyle}>Đỏ đi trước</RadioButton>
          <RadioButton value={-1} style={radioButtonStyle}>Đen đi trước</RadioButton>
        </RadioGroup>
      </div>
      )}
      {options.mode !== 5 && (
      <div style={{ fontSize: 16, marginTop: 16 }}>
        Màu quân:
        <RadioGroup 
          onChange={(e) => setOptions({ ...options, color: e.target.value })} 
          value={options.color} 
          size='large'
        >
          <RadioButton value={'r'} style={radioButtonStyle}>Quân Đỏ</RadioButton>
          <RadioButton value={'b'} style={radioButtonStyle}>Quân Đen</RadioButton>
        </RadioGroup>
      </div>
      )}
      {options.mode === 5 && (
        <div style={{ marginTop: 16 }}>
          <p style={{ marginTop: 16, color: '#666', fontSize: 16 }}>
            Hệ thống yêu cầu bạn đăng nhập (hoặc Đăng ký) để lưu lại tiến trình và tham gia các phòng online.
          </p>
        </div>
      )}
    </Modal>
  );
};

export default StartModel;
