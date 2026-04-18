import React from 'react';
import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';
import ChessBoard from './routes/Chessboard';
import Replay from './routes/Replay';
import UserManager from './components/admin/UserManager';
import { useAppSelector } from './hooks';
import 'antd/dist/reset.css';

const AdminRoute: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const { isLoggedIn, userRole } = useAppSelector(s => s.chess);
  if (!isLoggedIn || userRole !== 'admin') {
    return <Navigate to="/" replace />;
  }
  return <>{children}</>;
};

function App() {
  return (
    <BrowserRouter>
      <div className="App">
        <Routes>
          <Route path="/" element={<ChessBoard />} />
          <Route path="/replay/:matchId" element={<Replay />} />
          <Route 
            path="/admin" 
            element={
              <AdminRoute>
                <UserManager />
              </AdminRoute>
            } 
          />
        </Routes>
      </div>
    </BrowserRouter>
  );
}

export default App;
