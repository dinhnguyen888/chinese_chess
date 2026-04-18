import React from 'react';
import { Modal, Button } from 'antd';
import { useAppDispatch } from '../../hooks';
import { startClick, onGameOver } from '../../models/chessSlice';
import { useAppSelector } from '../../hooks';
import ReportModal from '../lobby/ReportModal';

interface WinnerModelProps {
  mode: number;
  color: string;
  winner: number | null;
}

const WinnerModel: React.FC<WinnerModelProps> = ({ mode, color, winner }) => {
  const dispatch = useAppDispatch();
  const { opponentName } = useAppSelector(s => s.chess);
  const [reportOpen, setReportOpen] = React.useState(false);

  if (winner === null) return null;

  const renderText = () => {
    if (mode === 1) {
      return winner === 1 
        ? 'Chúc mừng! Bạn đã chiến thắng máy, hãy thử thách độ khó cao hơn nhé!' 
        : 'Rất tiếc! Bạn đã thua rồi, hãy rèn luyện thêm và thử lại nhé!';
    } else {
      let result: string;
      if (winner === 1) {
        result = color === 'r' ? 'Đỏ' : 'Đen';
      } else {
        result = color === 'r' ? 'Đen' : 'Đỏ';
      }
      return `Bên ${result} đã giành chiến thắng!`;
    }
  };

  return (
    <Modal
      title={<h2><strong>Kết quả trận đấu</strong></h2>}
      open={Math.abs(winner) === 1}
      onCancel={() => dispatch(onGameOver())}
      footer={[
        mode === 5 && (
          <Button key='report' danger onClick={() => setReportOpen(true)}>
            Tố cáo gian lận
          </Button>
        ),
        <Button key='1' size="large" onClick={() => dispatch(startClick())}>Chơi lại</Button>,
        <Button 
          key='2' 
          type="primary" 
          size="large" 
          onClick={() => {
            if (mode === 1) {
              window.location.reload();
            } else {
              dispatch(onGameOver());
            }
          }}
        >
          Đồng ý
        </Button>,
      ]}
    >
      <p style={{ fontSize: '18px', textAlign: 'center' }}>
        {renderText()}
      </p>
      
      {mode === 5 && (
        <ReportModal 
          open={reportOpen} 
          onClose={() => setReportOpen(false)} 
          reportedUser={opponentName} 
        />
      )}
    </Modal>
  );
};

export default WinnerModel;
