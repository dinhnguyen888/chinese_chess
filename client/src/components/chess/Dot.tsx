import React from 'react';
import { style } from 'typestyle';
import { spacexy } from './Chess';
import dotImg from '../../assets/style/dot.png';

interface DotProps {
  position: [number, number];
}

const Dot: React.FC<DotProps> = ({ position }) => {
  const DotStyle = style({
    position: 'absolute',
    backgroundImage: `url(${dotImg})`,
    width: 21,
    height: 21,
    top: 15 + position[1] * spacexy,
    left: 17 + position[0] * spacexy,
    pointerEvents: 'none',
    zIndex: 5
  });

  return <div className={DotStyle} />;
};

export default Dot;
