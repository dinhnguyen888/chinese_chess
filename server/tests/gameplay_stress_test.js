import ws from 'k6/ws';
import http from 'k6/http';
import { check, sleep } from 'k6';
import { randomString } from 'https://jslib.k6.io/k6-utils/1.2.0/index.js';

export const options = {
    vus: 100,
    duration: '30s',
};

const BASE_URL = 'http://localhost:8080';
const WS_URL = 'ws://localhost:8080/ws';

export default function () {
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


    const url = `${WS_URL}?token=${token}`;

    ws.connect(url, function (socket) {
        socket.on('open', function () {
            socket.send(JSON.stringify({ type: 'find_match' }));
        });

        let isInGame = false;
        let isMyTurn = false;
        let startMoveTime = 0;

        socket.on('message', function (msg) {
            const data = JSON.parse(msg);

            if (data.type === 'match_found') {
                isInGame = true;

                if (data.color === 'red') {
                    isMyTurn = true;

                    socket.setTimeout(function () {
                        startMoveTime = Date.now();
                        socket.send(JSON.stringify({ type: 'move', data: 'b2b9' })); // Pháo đầu
                        isMyTurn = false;
                    }, 500);
                }
            }


            if (data.type === 'move') {
                if (isInGame) {
                    isMyTurn = true;


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
        });

        socket.on('error', function (e) {
            if (e.error() != 'websocket: close sent') {
                console.log('An unexpected error occured: ', e.error());
            }
        });

        socket.setTimeout(function () {
            socket.close();
        }, 30000);
    });
}
