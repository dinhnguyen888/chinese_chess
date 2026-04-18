import React, { useEffect } from 'react';
import { nextPace, chessValue } from '../../models/chessInfo';
import { AIClick } from '../../models/chessSlice';
import { useAppDispatch } from '../../hooks';

interface AIProps {
  treeDepth: number;
  board: (string | undefined)[][];
  side: number;
  mode: number;
  paceHistory: string[];
}

const AI: React.FC<AIProps> = ({ treeDepth, board, side, mode, paceHistory }) => {
  const dispatch = useAppDispatch();
  let searchCount = 0;

  const evaluate = (board: (string | undefined)[][], side: number) => {
    let val = 0;
    board.forEach((row, i) => {
      row.forEach((key, n) => {
        if (key) {
          val += chessValue[key[0]][i][n] * (key[0] >= 'a' ? 1 : -1);
        }
      });
    });
    val += Math.round(Math.random() * 20 - 10);
    searchCount++;
    return val * side;
  };

  const getAllMyChess = (board: (string | undefined)[][], side: number) => {
    let chessArray: Array<{ x: number, y: number, key: string }> = [];
    board.forEach((row, i) => {
      row.forEach((key, n) => {
        if (key && (key[0] >= 'a' ? 1 : -1) === side) {
          chessArray.push({ x: n, y: i, key: key });
        }
      });
    });
    return chessArray;
  };

  const getMoves = (board: (string | undefined)[][], side: number) => {
    let chessArray = getAllMyChess(board, side);
    let moves: [number, number, number, number, string][] = [];
    chessArray.forEach((chess) => {
      let nextMove: number[][] = nextPace[chess.key[0].toLowerCase()](chess.x, chess.y, board, side);
      
      const h = paceHistory;
      const l = h.length;
      if (l >= 8 && chess.key !== 'j0' && chess.key !== 'J0') {
        nextMove = nextMove.filter((pace) => {
          if (h[l - 2] === h[l - 6] && [[pace[1], pace[0]], [chess.y, chess.x]].join() === h[l - 4].substring(0, 7)) {
            return false;
          }
          return true;
        });
      }
      nextMove.forEach((move) => {
        moves.push([chess.x, chess.y, move[0], move[1], chess.key]);
      });
    });
    return moves;
  };

  const getAlphaBeta: any = (A: number, B: number, depth: number, board: (string | undefined)[][], side: number) => {
    if (depth === 0) {
      return { value: evaluate(board, side) };
    }
    let moves = getMoves(board, side);
    let result: any = null;
    for (let move of moves) {
      let [oldX, oldY, newX, newY, key] = move;
      let clearKey = board[newY][newX];
      
      board[newY][newX] = key;
      board[oldY][oldX] = undefined;

      if (clearKey === "j0" || clearKey === "J0") {
        board[oldY][oldX] = key;
        board[newY][newX] = clearKey;
        return { oldx: oldX, oldy: oldY, key: key, x: newX, y: newY, value: 8888 };
      } else {
        let val = -getAlphaBeta(-B, -A, depth - 1, board, -side).value;
        board[oldY][oldX] = key;
        board[newY][newX] = clearKey;

        if (val >= B) {
          return { oldx: oldX, oldy: oldY, key: key, x: newX, y: newY, value: B };
        }
        if (val > A) {
          A = val;
          result = { oldx: oldX, oldy: oldY, key: key, x: newX, y: newY, value: A };
        }
      }
    }
    if (!result) return { value: A };
    return result;
  };

  const initAI = (depth: number) => {
    if (depth === 0) return null;
    let initTime = new Date().getTime();
    searchCount = 0;
    const boardClone = board.map(row => [...row]);
    let result = getAlphaBeta(-9999, 9999, depth, boardClone, side);
    let nowTime = new Date().getTime();
    console.log(`AI Search Result: Move [${result.oldx}, ${result.oldy}] -> [${result.x}, ${result.y}], Depth: ${depth}, Branches: ${searchCount}, Score: ${result.value}, Time: ${nowTime - initTime}ms`);
    return [result.oldx, result.oldy, result.x, result.y];
  };

  useEffect(() => {
    let timer: any;
    const shouldRunAI = () => {
      if (mode === 5 || mode === 6) return false;
      if (mode === 1 && side === -1) return true;
      if (mode === 2 && (side === -1 || side === 1)) return true;
      if (mode > 3) return true; // Hint modes (mode 4, mode 12)
      return false;
    };

    if (shouldRunAI()) {
      timer = setTimeout(() => {
        const result = initAI(treeDepth);
        if (result) dispatch(AIClick(result));
      }, 500);
    }

    return () => clearTimeout(timer);
  }, [side, mode, board, treeDepth]);

  return null;
};

export default AI;
