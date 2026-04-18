import { createSlice, type PayloadAction } from '@reduxjs/toolkit';
import { message } from 'antd';
import { nextPace } from './chessInfo';

export interface ChessProps {
  name: string;
  type: string;
  side: 1 | -1;
  position: [number, number];
}

export interface gameOptions {
  mode: number;
  difficulty: number;
  side: number;
  color: string;
}

export interface RoomInfo {
  id: string;
  name: string;
  host: string;
  guest: string;
  isPlaying: boolean;
  autoStart: boolean;
}

export interface ChatMessage {
  sender: string;
  message: string;
}

export interface gameState {
  side: number;
  click: ChessProps | null;
  board: (string | undefined)[][];
  nextPace: Array<[number, number]> | null;
  chessChange: [[number, number], [number, number], number] | null;
  color: string;
  showModel: boolean;
  mode: number;
  difficulty: number;
  winner: number | null;
  clearChessMode: boolean;
  history: Array<(string | undefined)[][]>;
  paceHistory: string[];
  onlineMatching: boolean;
  roomStatus: 'idle' | 'listing' | 'waiting_in_room' | 'playing';
  isLoggedIn: boolean;
  currentRoomId: string;
  currentRoomName: string;
  guestName: string;
  rooms: RoomInfo[];
  chat: ChatMessage[];
  playerName: string;
  opponentName: string;
  userRole: string;
  canChat: boolean;
  canCreateRoom: boolean;
  replayMoves: string[];
  replayIndex: number;
}

let initIsLoggedIn = false;
let initPlayerName = '';
let initUserRole = 'user';
try {
  const token = localStorage.getItem('chess_jwt_token');
  if (token) {
    const payloadBase64 = token.split('.')[1];
    const base64 = payloadBase64.replace(/-/g, '+').replace(/_/g, '/');
    const pad = base64.length % 4;
    const paddedBase64 = pad ? base64 + '='.repeat(4 - pad) : base64;
    const payloadJson = JSON.parse(atob(paddedBase64));
    if (payloadJson && payloadJson.username) {
      initIsLoggedIn = true;
      initPlayerName = payloadJson.username;
      initUserRole = payloadJson.role || 'user';
    }
  }
} catch (e) {
  // Ignore token parse error
}

const initialState: gameState = {
  side: 0,
  click: null,
  board: [],
  nextPace: null,
  color: 'r',
  chessChange: null,
  showModel: false,
  mode: 1,
  difficulty: 2,
  winner: null,
  clearChessMode: false,
  history: [],
  paceHistory: [],
  onlineMatching: false,
  roomStatus: 'idle',
  isLoggedIn: initIsLoggedIn,
  currentRoomId: '',
  currentRoomName: '',
  guestName: '',
  rooms: [],
  chat: [],
  playerName: initPlayerName,
  opponentName: '',
  userRole: initUserRole,
  canChat: true,
  canCreateRoom: true,
  replayMoves: [],
  replayIndex: 0,
};

/** Lật bàn nhìn đối diện: x' = (cols-1)-x, y' = (rows-1)-y — bàn 9×10 ô quen thuộc tương ứng x'=8-x, y'=9-y. */
function flipBoardPerspective(board: (string | undefined)[][]): (string | undefined)[][] {
  if (!board.length || !board[0]?.length) return board;
  const maxY = board.length - 1;
  const maxX = board[0].length - 1;
  const out: (string | undefined)[][] = Array.from({ length: board.length }, () =>
    Array(board[0].length).fill(undefined)
  );
  for (let y = 0; y <= maxY; y++) {
    for (let x = 0; x <= maxX; x++) {
      const key = board[y][x];
      if (!key) continue;
      const y2 = maxY - y;
      const x2 = maxX - x;
      const newKey = key[0] >= 'a' ? key[0].toUpperCase() : key[0].toLowerCase();
      out[y2][x2] = newKey + key[1];
    }
  }
  return out;
}

function flipSquare(pos: [number, number], maxY: number, maxX: number): [number, number] {
  return [maxY - pos[0], maxX - pos[1]];
}

const chessSlice = createSlice({
  name: 'chess',
  initialState,
  reducers: {
    startClick(state) {
      state.showModel = true;
      if (state.winner) {
        state.side = 0;
      }
      state.winner = null;
    },
    onModelOK(state, action: PayloadAction<gameOptions>) {
      const payload = action.payload;
      state.side = payload.color === 'r' ? payload.side : 0 - payload.side;
      state.difficulty = payload.difficulty;
      state.mode = payload.mode;
      state.color = payload.color;
      const initialBoard = [
        ['C0', 'M0', 'X0', 'S0', 'J0', 'S1', 'X1', 'M1', 'C1'],
        [undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined],
        [undefined, 'P0', undefined, undefined, undefined, undefined, undefined, 'P1', undefined],
        ['Z0', undefined, 'Z1', undefined, 'Z2', undefined, 'Z3', undefined, 'Z4'],
        [undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined],
        [undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined],
        ['z0', undefined, 'z1', undefined, 'z2', undefined, 'z3', undefined, 'z4'],
        [undefined, 'p0', undefined, undefined, undefined, undefined, undefined, 'p1', undefined],
        [undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined],
        ['c0', 'm0', 'x0', 's0', 'j0', 's1', 'x1', 'm1', 'c1']
      ];
      state.board = initialBoard;
      state.click = null;
      state.nextPace = null;
      state.chessChange = null;
      state.showModel = false;
      state.history = state.side === 1 ? [initialBoard.map(row => [...row])] : [];
      state.paceHistory = [];
    },
    onModelCancel(state) {
      state.showModel = false;
    },
    chessClick(state, action: PayloadAction<ChessProps>) {
      if (state.mode === 6) return state; // Blocking in replay mode
      if (state.mode === 2 || (state.mode === 1 && state.side === -1) || (state.mode === 5 && state.side !== 1)) return state;
      
      if (state.clearChessMode) {
        state.clearChessMode = false;
        state.click = null;
        state.nextPace = null;
        state.chessChange = null;
        const [i, j] = action.payload.position;
        if (!state.board[i][j]) return state;
        if (state.board[i][j] === 'J0' || state.board[i][j] === 'j0') {
          message.error('Tướng và Soái không thể chấp được đâu nhé!');
        } else {
          state.board[i][j] = undefined;
        }
        return state;
      }

      if (state.click) {
        const [i, j] = action.payload.position;
        const [oldi, oldj] = state.click.position;
        const canMove = state.nextPace?.some(item => item[0] === j && item[1] === i);
        
        if (canMove) {
          if (state.board[i][j] === 'J0') {
            state.winner = 1;
            state.side = 0;
          } else if (state.board[i][j] === 'j0') {
            state.winner = -1;
            state.side = 0;
          }
          state.board[i][j] = state.click.name;
          state.board[oldi][oldj] = undefined;
          state.chessChange = [[i, j], [oldi, oldj], state.click! .side];
          state.side = -state.side;
          state.history.push(state.board.map(row => [...row]));
          state.paceHistory.push(state.chessChange.join());
        }
        state.nextPace = null;
        state.click = null;
      } else {
        const chess = action.payload;
        if (state.side !== chess.side) return state;
        const [i, j] = chess.position;
        state.click = chess;
        state.nextPace = nextPace[chess.type](j, i, state.board, chess.side);
        
        // 防止长将
        const h = state.paceHistory;
        const l = h.length;
        if (l >= 8 && chess.name !== 'j0' && chess.name !== 'J0') {
          state.nextPace = state.nextPace!.filter(pace => {
             if (h[l-2] === h[l-6] && [[pace[1], pace[0]], [i, j], chess.side].join() === h[l-4]) {
               return false;
             }
             return true;
          });
        }
      }
    },
    boardClick(state, action: PayloadAction<{x: number, y: number, offsetLeft: number, offsetTop: number}>) {
      if (state.mode === 6) return state; // Blocking in replay mode
      if (state.mode === 2 || (state.mode === 1 && state.side === -1) || (state.mode === 5 && state.side !== 1)) return state;
      
      const { x, y, offsetLeft, offsetTop } = action.payload;
      const chessSize = 54;
      const spacexy = 57;
      const j = Math.round((x - offsetLeft - chessSize / 2 + 5) / spacexy);
      const i = Math.round((y - offsetTop - chessSize / 2 + 5) / spacexy);

      if (state.clearChessMode) {
        state.clearChessMode = false;
        state.click = null;
        state.nextPace = null;
        state.chessChange = null;
        if (i < 0 || i > 9 || j < 0 || j > 8) return state;
        if (!state.board[i][j]) return state;
        if (state.board[i][j] === 'J0' || state.board[i][j] === 'j0') {
          message.error('Tướng và Soái không thể chấp được đâu nhé!');
        } else {
          state.board[i][j] = undefined;
        }
        return state;
      }

      if (state.click) {
        const [oldi, oldj] = state.click.position;
        const canMove = state.nextPace?.some(item => item[0] === j && item[1] === i);
        
        if (canMove) {
          if (state.board[i][j] === 'J0') {
            state.winner = 1;
            state.side = 0;
          } else if (state.board[i][j] === 'j0') {
            state.winner = -1;
            state.side = 0;
          }
          state.board[i][j] = state.click.name;
          state.board[oldi][oldj] = undefined;
          state.chessChange = [[i, j], [oldi, oldj], state.click.side];
          state.side = -state.side;
          state.history.push(state.board.map(row => [...row]));
          state.paceHistory.push(state.chessChange.join());
        }
        state.nextPace = null;
        state.click = null;
      }
    },
    AIClick(state, action: PayloadAction<number[]>) {
      const move = action.payload;
      if (move[0] === undefined) return state;
      const [oldx, oldy, x, y] = move;
      const key = state.board[oldy][oldx];
      if (state.board[y][x] === 'j0') {
        state.winner = -1;
        state.side = 0;
      } else if (state.board[y][x] === 'J0') {
        state.winner = 1;
        state.side = 0;
      }
      state.board[y][x] = key;
      state.board[oldy][oldx] = undefined;
      state.chessChange = [[y, x], [oldy, oldx], state.side];
      state.side = -state.side;
      state.history.push(state.board.map(row => [...row]));
      state.paceHistory.push(state.chessChange.join());
      if (state.mode === 4 || state.mode === 12) {
        state.mode = state.mode / 4;
      }
    },
    toggleAI(state) {
      state.side = Math.abs(state.side) === 2 ? state.side / 2 : state.side * 2;
    },
    onGameOver(state) {
      state.winner = null;
      state.side = 0;
      if (state.mode === 5) {
        state.mode = 1;
        state.onlineMatching = false;
        state.board = [];
        state.click = null;
        state.nextPace = null;
        state.chessChange = null;
        state.history = [];
        state.paceHistory = [];
      }
    },
    changeSide(state) {
      const b = state.board;
      const maxY = b.length > 0 ? b.length - 1 : 0;
      const maxX = b[0]?.length ? b[0].length - 1 : 0;
      state.board = flipBoardPerspective(b);
      state.color = state.color === 'r' ? 'b' : 'r';
      state.side = -state.side;
      state.nextPace = null;
      state.click = null;
      const c = state.chessChange;
      if (c) {
        state.chessChange = [flipSquare(c[0], maxY, maxX), flipSquare(c[1], maxY, maxX), -c[2]];
      }
      if (state.mode === 1) {
        state.history = [];
      } else if (state.mode === 3) {
        state.history = [state.board.map(row => [...row])];
      }
    },
    clearChess(state) {
      state.clearChessMode = true;
    },
    showHint(state) {
      state.mode = state.mode * 4;
    },
    regretMove(state) {
      if (state.history.length <= 1) return state;
      if (state.mode === 1) {
        state.history.pop();
        state.history.pop();
        state.board = state.history[state.history.length - 1].map(row => [...row]);
      } else if (state.mode === 3) {
        state.history.pop();
        state.board = state.history[state.history.length - 1].map(row => [...row]);
        state.side = -state.side;
      }
      state.click = null;
      state.nextPace = null;
      state.chessChange = null;
    },
    beginOnlineMatch(state, action: PayloadAction<string>) {
      state.showModel = false;
      state.mode = 5;
      state.roomStatus = 'listing';
      state.playerName = action.payload;
      state.winner = null;
      state.side = 0;
      state.board = [];
      state.click = null;
      state.nextPace = null;
      state.chessChange = null;
      state.history = [];
      state.paceHistory = [];
      state.chat = [];
    },
    updateRoomList(state, action: PayloadAction<RoomInfo[]>) {
      state.rooms = action.payload;
    },
    addChatMessage(state, action: PayloadAction<ChatMessage>) {
      state.chat.push(action.payload);
    },
    // Chủ phòng: đã tạo phòng, chờ người vào
    hostRoomCreated(state, action: PayloadAction<{ roomId: string; roomName: string }>) {
      state.roomStatus = 'waiting_in_room';
      state.currentRoomId = action.payload.roomId;
      state.currentRoomName = action.payload.roomName;
      state.guestName = '';
    },
    // Khách: đã join phòng, chờ host bắt đầu
    playerJoinedRoom(state, action: PayloadAction<{ roomId: string; hostName: string }>) {
      state.roomStatus = 'waiting_in_room';
      state.currentRoomId = action.payload.roomId;
      state.opponentName = action.payload.hostName;
    },
    // Host nhận thông báo có khách vào
    guestJoined(state, action: PayloadAction<string>) {
      state.guestName = action.payload;
    },
    // Khách rời phòng, host trở về chờ
    guestLeft(state) {
      state.guestName = '';
    },
    onlineMatched(state, action: PayloadAction<{ color: string; orderSide: number; opponentName: string }>) {
      const { color, orderSide, opponentName } = action.payload;
      state.roomStatus = 'playing';
      state.onlineMatching = false;
      state.side = color === 'r' ? orderSide : 0 - orderSide;
      state.difficulty = 2;
      state.mode = 5;
      state.color = color;
      state.opponentName = opponentName;
      const initialBoard = [
        ['C0', 'M0', 'X0', 'S0', 'J0', 'S1', 'X1', 'M1', 'C1'],
        [undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined],
        [undefined, 'P0', undefined, undefined, undefined, undefined, undefined, 'P1', undefined],
        ['Z0', undefined, 'Z1', undefined, 'Z2', undefined, 'Z3', undefined, 'Z4'],
        [undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined],
        [undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined],
        ['z0', undefined, 'z1', undefined, 'z2', undefined, 'z3', undefined, 'z4'],
        [undefined, 'p0', undefined, undefined, undefined, undefined, undefined, 'p1', undefined],
        [undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined],
        ['c0', 'm0', 'x0', 's0', 'j0', 's1', 'x1', 'm1', 'c1']
      ];
      state.board = color === 'b' ? flipBoardPerspective(initialBoard) : initialBoard;
      state.click = null;
      state.nextPace = null;
      state.chessChange = null;
      state.history = state.side === 1 ? [state.board.map(row => [...row])] : [];
      state.paceHistory = [];
    },
    onlineAborted(state) {
      state.onlineMatching = false;
      state.roomStatus = 'listing';
      state.side = 0;
      state.board = [];
      state.click = null;
      state.nextPace = null;
      state.chessChange = null;
      state.history = [];
      state.paceHistory = [];
      state.opponentName = '';
      state.chat = [];
    },
    loginSuccess(state, action: PayloadAction<{ username: string, role: string, canChat?: boolean, canCreateRoom?: boolean }>) {
      state.isLoggedIn = true;
      state.playerName = action.payload.username;
      state.userRole = action.payload.role;
      if (action.payload.canChat !== undefined) state.canChat = action.payload.canChat;
      if (action.payload.canCreateRoom !== undefined) state.canCreateRoom = action.payload.canCreateRoom;
    },
    applyRemoteMove(
      state,
      action: PayloadAction<{
        to: [number, number];
        from: [number, number];
        piece: string;
        winner: number | null;
      }>
    ) {
      const { to, from, piece, winner } = action.payload;
      const maxY = state.board.length > 0 ? state.board.length - 1 : 9;
      const maxX = state.board[0]?.length ? state.board[0].length - 1 : 8;

      // Flip coordinates and piece case because the remote player's board is mirrored relative to ours
      const i = maxY - to[0];
      const j = maxX - to[1];
      const oldi = maxY - from[0];
      const oldj = maxX - from[1];
      const flippedPiece = piece[0] >= 'a' ? piece[0].toUpperCase() + piece.slice(1) : piece[0].toLowerCase() + piece.slice(1);

      state.board[i][j] = flippedPiece;
      state.board[oldi][oldj] = undefined;
      const pieceSide = flippedPiece[0] >= 'a' ? 1 : -1;
      state.chessChange = [[i, j], [oldi, oldj], pieceSide];
      if (winner !== null) {
        state.winner = -winner;
        state.side = 0;
      } else {
        state.side = -state.side;
      }
      state.history.push(state.board.map(row => [...row]));
      state.paceHistory.push(state.chessChange.join());
      state.nextPace = null;
      state.click = null;
    },
    startReplay(state, action: PayloadAction<string[]>) {
      state.mode = 6;
      state.replayMoves = action.payload;
      state.replayIndex = 0;
      state.side = 1; // Luôn hiển thị bắt đầu
      state.winner = null;
      state.click = null;
      state.nextPace = null;
      state.chessChange = null;
      state.paceHistory = [];
      const initialBoard = [
        ['C0', 'M0', 'X0', 'S0', 'J0', 'S1', 'X1', 'M1', 'C1'],
        [undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined],
        [undefined, 'P0', undefined, undefined, undefined, undefined, undefined, 'P1', undefined],
        ['Z0', undefined, 'Z1', undefined, 'Z2', undefined, 'Z3', undefined, 'Z4'],
        [undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined],
        [undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined],
        ['z0', undefined, 'z1', undefined, 'z2', undefined, 'z3', undefined, 'z4'],
        [undefined, 'p0', undefined, undefined, undefined, undefined, undefined, 'p1', undefined],
        [undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined, undefined],
        ['c0', 'm0', 'x0', 's0', 'j0', 's1', 'x1', 'm1', 'c1']
      ];
      state.board = initialBoard;
      state.history = [initialBoard.map(row => [...row])];
    },
    nextReplayMove(state) {
      if (state.mode !== 6 || state.replayIndex >= state.replayMoves.length) return state;
      const moveStr = state.replayMoves[state.replayIndex];
      // moveStr format: "i,j,oldi,oldj,pieceSide"
      // or "[[i,j],[oldi,oldj],pieceSide]"
      try {
        const parts = moveStr.replace(/[\[\]]/g, '').split(',').map(Number);
        if (parts.length === 5) {
          const [i, j, oldi, oldj, pieceSide] = parts;
          const piece = state.board[oldi][oldj];
          if (piece) {
            state.board[i][j] = piece;
            state.board[oldi][oldj] = undefined;
            state.chessChange = [[i, j], [oldi, oldj], pieceSide];
          }
        }
      } catch (e) { /* ignore */ }
      state.history.push(state.board.map(row => [...row]));
      state.replayIndex++;
      state.side = -state.side;
    },
    prevReplayMove(state) {
      if (state.mode !== 6 || state.replayIndex <= 0) return state;
      state.history.pop();
      state.board = state.history[state.history.length - 1].map(row => [...row]);
      state.replayIndex--;
      state.side = -state.side;
      state.chessChange = null;
    },
    exitReplay(state) {
      state.mode = 0;
      state.side = 0;
      state.board = [];
      state.click = null;
      state.chessChange = null;
      state.history = [];
      state.replayMoves = [];
      state.replayIndex = 0;
    }
  }
});

export const {
  startClick, onModelOK, onModelCancel, chessClick, boardClick,
  AIClick, toggleAI, onGameOver, changeSide, clearChess, showHint, regretMove,
  beginOnlineMatch, onlineMatched, onlineAborted, applyRemoteMove, updateRoomList, addChatMessage,
  hostRoomCreated, playerJoinedRoom, guestJoined, guestLeft, loginSuccess,
  startReplay, nextReplayMove, prevReplayMove, exitReplay
} = chessSlice.actions;

export default chessSlice.reducer;
