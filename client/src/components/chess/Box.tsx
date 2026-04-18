import React from 'react';
import { style } from 'typestyle';
import { chessSize, spacexy } from './Chess';

interface BoxProps {
  chessChange: [[number, number], [number, number], number];
  color: string;
}

const Box: React.FC<BoxProps> = ({ chessChange, color }) => {
  let boxColorPrefix: string;
  if (color === 'r') {
    boxColorPrefix = chessChange[2] === 1 ? 'r' : 'b';
  } else {
    boxColorPrefix = chessChange[2] === 1 ? 'b' : 'r';
  }

  const boxImg = new URL(`../../assets/style/${boxColorPrefix}_box.png`, import.meta.url).href;

  const BoxStylePrev = style({
    position: 'absolute',
    backgroundImage: `url(${boxImg})`,
    width: chessSize,
    height: chessSize,
    top: -3 + chessChange[0][0] * spacexy,
    left: -3 + chessChange[0][1] * spacexy,
    pointerEvents: 'none',
    zIndex: 2,
    backgroundSize: 'contain'
  });

  const BoxStyleNext = style({
    position: 'absolute',
    backgroundImage: `url(${boxImg})`,
    width: chessSize,
    height: chessSize,
    top: -3 + chessChange[1][0] * spacexy,
    left: -3 + chessChange[1][1] * spacexy,
    pointerEvents: 'none',
    zIndex: 2,
    backgroundSize: 'contain'
  });

  return (
    <div>
      <div className={BoxStylePrev} />
      <div className={BoxStyleNext} />
    </div>
  );
};

export default Box;
