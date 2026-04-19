import ws from 'k6/ws';
import http from 'k6/http';
import { check, sleep } from 'k6';
import { randomString } from 'https://jslib.k6.io/k6-utils/1.2.0/index.js';

// Configuration
const BASE_URL = 'http://localhost:8081'; // HTTP Port
const WS_URL = 'ws://localhost:8080/ws'; // WS Port

export const options = {
    vus: 5, 
    iterations: 5,
};

function setupUser() {
    const username = `cheat_test_${randomString(5)}`;
    const password = 'password123';

    // Register
    http.post(`${BASE_URL}/register`, JSON.stringify({ username, password }), {
        headers: { 'Content-Type': 'application/json' },
    });

    // Login
    const loginRes = http.post(`${BASE_URL}/login`, JSON.stringify({ username, password }), {
        headers: { 'Content-Type': 'application/json' },
    });

    if (loginRes.status !== 200) {
        throw new Error(`Login failed: ${loginRes.body}`);
    }

    return { token: loginRes.json().token, username };
}

export default function () {
    const user = setupUser();
    const url = `${WS_URL}?token=${user.token}`;

    ws.connect(url, function (socket) {
        socket.on('open', function () {
            console.log(`[${user.username}] Connected - Starting Security Siege...`);
            
            // --- CASE 1: Same type spam (search_room > 8 times) ---
            console.log(`[${user.username}] Test: Same-Type Spam (search_room)...`);
            for (let i = 0; i < 12; i++) {
                socket.send(JSON.stringify({ type: 'search_room', query: 'test' }));
            }

            // --- CASE 2: Instant Win Fraud ---
            console.log(`[${user.username}] Test: Instant Win Claim (0 server-side moves)...`);
            socket.send(JSON.stringify({ 
                type: 'move', 
                winner: 1, 
                moves: ["fabricated_history_1"] 
            }));

            // --- CASE 3: Move Flooding (> 2 moves/s) ---
            console.log(`[${user.username}] Test: Move Flooding (10 moves/s)...`);
            for (let i = 0; i < 5; i++) {
                socket.send(JSON.stringify({ type: 'move', from: [0,0], to: [0,1] }));
            }

            // --- CASE 4: Payload Sanity (Chat length > 300) ---
            console.log(`[${user.username}] Test: Chat Payload Overflow...`);
            socket.send(JSON.stringify({ 
                type: 'chat', 
                message: "A".repeat(500) 
            }));

            // --- CASE 5: Invalid Game Result Value ---
            console.log(`[${user.username}] Test: Invalid Result String Injection...`);
            socket.send(JSON.stringify({ 
                type: 'game_result', 
                result: "i_always_win", 
                opponent: "victim" 
            }));

            // --- CASE 6: Type Confusion (RoomID as Object) ---
            console.log(`[${user.username}] Test: Type Confusion (roomId object)...`);
            socket.send(JSON.stringify({ 
                type: 'join_room', 
                roomId: { id: 123 } 
            }));

            // Stay open a bit to receive error responses
            socket.setTimeout(() => {
                socket.close();
            }, 3000);
        });

        socket.on('message', function (msg) {
            const data = JSON.parse(msg);
            if (data.type === 'error') {
                console.warn(`[Anti-Cheat Caught] ${user.username}: ${data.message}`);
            }
        });

        socket.on('close', () => console.log(`[${user.username}] Security test finished.`));
    });
}
