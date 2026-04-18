import ws from 'k6/ws';
import http from 'k6/http';
import { check, sleep } from 'k6';
import { randomString } from 'https://jslib.k6.io/k6-utils/1.2.0/index.js';

// ==========================================
// KỊCH BẢN 3: ĐẠI CHIẾN NGHÌN QUÂN (GAMEPLAY)
// ==========================================
// CASE: Kiểm tra khả năng chịu tải của game server khi nhiều session thi đấu song song.
// EXPECT: Move packet gửi đi và nhận lại (broadcast tới đối thủ) ổn định dưới 200ms.
// RESULT (Kì vọng): Không có timeout, không bị rớt mạng, CPU của GameService ổn định.

export const options = {
    // Để bắt đầu nhẹ nhàng, chúng ta test với 100 User đồng thời chơi trong 30 giây.
    // Bạn có thể tăng số này lên 1000 - 5000 ở môi trường thật.
    vus: 100, 
    duration: '30s',
};

const BASE_URL = 'http://localhost:8080';
const WS_URL = 'ws://localhost:8080/ws';

// Bước 1: Setup - Chạy 1 lần trước khi dàn VUs (Virtual Users) vào cuộc
// Chúng ta sẽ bỏ qua bước này vì mỗi VU tự tạo user cho mình.

export default function () {
    // ----------------------------------------------------
    // CASE: Đăng ký & Đăng nhập lấy Token
    // EXPECT: Trả về trạng thái 200/201 và kèm theo JSON có jwt=...
    // ----------------------------------------------------
    const username = `testuser_${randomString(8)}`;
    const password = 'password123';
    
    // Register
    http.post(`${BASE_URL}/auth/register`, JSON.stringify({ username, password }), {
        headers: { 'Content-Type': 'application/json' },
    });

    // Login để lấy JWT
    const loginRes = http.post(`${BASE_URL}/auth/login`, JSON.stringify({ username, password }), {
        headers: { 'Content-Type': 'application/json' },
    });

    check(loginRes, { 'login success': (r) => r.status === 200 });

    const body = loginRes.json();
    if (!body || !body.token) {
        console.error('Không lấy được token, kết thúc VU');
        return; // Dừng VU này
    }

    const token = body.token;
    
    // ----------------------------------------------------
    // CASE: Kết nối WebSocket
    // EXPECT: Nối Socket thành công với Token Query
    // ----------------------------------------------------
    const url = `${WS_URL}?token=${token}`;
    
    ws.connect(url, function (socket) {
        socket.on('open', function () {
            // Khi vừa vào, gửi `find_match` để tìm người chơi khác.
            socket.send(JSON.stringify({ type: 'find_match' }));
        });

        let isInGame = false;
        let isMyTurn = false;
        let startMoveTime = 0;

        socket.on('message', function (msg) {
            const data = JSON.parse(msg);

            // ----------------------------------------------------
            // CASE: Nhận thông báo tìm trận thành công
            // EXPECT: Server trả về match_found
            // ----------------------------------------------------
            if (data.type === 'match_found') {
                isInGame = true;
                // Nếu mình đi trước (màu đỏ)
                if (data.color === 'red') {
                    isMyTurn = true;
                    // Chờ 1 giây rồi đi quân đầu tiên
                    socket.setTimeout(function () {
                        startMoveTime = Date.now();
                        socket.send(JSON.stringify({ type: 'move', data: 'b2b9' })); // Pháo đầu
                        isMyTurn = false; 
                    }, 500);
                }
            }

            // ----------------------------------------------------
            // CASE: Đối thủ vừa đi quân (broadcast)
            // EXPECT: Client nhận thông báo đối thủ đi, trễ < 200ms
            // ----------------------------------------------------
            if (data.type === 'move') {
                if (isInGame) {
                    isMyTurn = true;
                    
                    // Đo lường latency (mang tính tương đối trong test này)
                    // Mỗi lần nhận được move của đối thủ thì phản hồi cực nhanh (Bot)
                    socket.setTimeout(function () {
                        startMoveTime = Date.now();
                        socket.send(JSON.stringify({ type: 'move', data: 'random_move_data' }));
                        isMyTurn = false;
                    }, 100); 
                }
            }

            if (data.type === 'error') {
                console.error(`Socket Error [${username}]:`, data.message);
            }
        });

        socket.on('close', function () {
            // Socket ngắt kết nối
        });
        
        socket.on('error', function (e) {
            if (e.error() != 'websocket: close sent') {
              console.log('An unexpected error occured: ', e.error());
            }
        });

        // Test này kéo dài 30 giây per VU
        socket.setTimeout(function () {
            socket.close();
        }, 30000);
    });
}
