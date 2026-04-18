import React from 'react';
import { style } from 'typestyle';
import { useAppDispatch } from '../../hooks';
import { chessClick, type ChessProps as ChessModelProps } from '../../models/chessSlice';

export interface ChessProps extends ChessModelProps {
  color: string;
  control: ChessModelProps | null;
}

export const chessSize = 54;
export const spacexy = 57;

const Chess: React.FC<ChessProps> = (props) => {
  const dispatch = useAppDispatch();

  const getImagePath = (type: string, side: number, color: string) => {
    const c = side === 1 ? (color === 'r' ? 'r' : 'b') : (color === 'b' ? 'r' : 'b');
    // Using dynamic import or URL
    return new URL(`../../assets/style/${c}_${type}.png`, import.meta.url).href;
  };

  const ChessStyle = style({
    position: 'absolute',
    backgroundImage: `url(${getImagePath(props.type, props.side, props.color)})`,
    width: chessSize,
    height: chessSize,
    top: -3 + props.position[0] * spacexy,
    left: -3 + props.position[1] * spacexy,
    opacity: props.control && props.control.name === props.name ? 0.8 : 1,
    cursor: 'pointer',
    backgroundSize: 'contain',
    zIndex: 10
  });

  return (
    <div
      className={ChessStyle}
      onClick={(e) => {
        e.stopPropagation();
        dispatch(chessClick(props));
      }}
    />
  );
};

export default Chess;
