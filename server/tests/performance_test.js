import ws from 'k6/ws';
import http from 'k6/http';
import { check, sleep } from 'k6';

const BASE_URL = 'http://localhost:8081';
const WS_URL = 'ws://localhost:8080/ws';

export const options = {
    vus: 50, // Adjust this to scale test (e.g. 100, 500...)
    duration: '1m',
};

// Simplified pawn coordinates
const RED_PAWNS = [[3, 0], [3, 2], [3, 4], [3, 6], [3, 8]];
const BLACK_PAWNS = [[6, 0], [6, 2], [6, 4], [6, 6], [6, 8]];

export default function () {
    const userId = Math.floor(Math.random() * 100000) + 1;
    const username = `user_${userId}`;
    const password = `pass_${userId}`;

    // 1. Login
    const loginRes = http.post(`${BASE_URL}/login`, JSON.stringify({ username, password }), {
        headers: { 'Content-Type': 'application/json' },
    });

    if (loginRes.status !== 200) return;
    const token = loginRes.json().token;
    const url = `${WS_URL}?token=${token}`;

    ws.connect(url, function (socket) {
        socket.on('open', function () {
            // 2. Find Match
            socket.send(JSON.stringify({ type: 'find_match' }));
        });

        let myColor = '';
        let pawnIndex = 0;
        let isMyTurn = false;

        socket.on('message', function (msg) {
            const data = JSON.parse(msg);

            // Handle matching
            if (data.type === 'matched') {
                myColor = data.color; // 'r' or 'b'
                isMyTurn = (myColor === 'r'); // Red goes first
                
                if (isMyTurn) sendPawnMove(socket);
            }

            // Handle opponent move
            if (data.type === 'move' && !data.winner) {
                isMyTurn = true;
                sendPawnMove(socket);
            }
        });

        function sendPawnMove(s) {
            if (pawnIndex >= 5) {
                // Done with 5 pawns, close after a bit
                s.setTimeout(() => s.close(), 1000);
                return;
            }

            // Simulating "thinking" time to avoid move-flood anti-cheat (must be > 0.5s)
            s.setTimeout(() => {
                const pawn = (myColor === 'r') ? RED_PAWNS[pawnIndex] : BLACK_PAWNS[pawnIndex];
                const direction = (myColor === 'r') ? 1 : -1;
                
                const movePacket = {
                    type: 'move',
                    from: pawn,
                    to: [pawn[0] + direction, pawn[1]],
                    moves: [] // Simplified for stress test
                };

                s.send(JSON.stringify(movePacket));
                pawnIndex++;
                isMyTurn = false;
            }, 800); 
        }

        socket.setTimeout(() => socket.close(), 40000);
    });
}
