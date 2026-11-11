import { useEffect, useRef } from 'react';
import { drawSwatch, readPalette } from '../lib/render.js';
import { useThemeEpoch } from '../lib/theme.js';

/** An obstacle drawn by the same code as the map, so a legend cannot disagree with it. */
export function Swatch({ kind, size }: { kind: number; size: number }) {
  const ref = useRef<HTMLCanvasElement>(null);
  const theme = useThemeEpoch();

  useEffect(() => {
    const canvas = ref.current;
    if (canvas) drawSwatch(canvas, kind, size, readPalette());
  }, [kind, size, theme]);

  return (
    <canvas ref={ref} className="swatch" style={{ width: size, height: size }} aria-hidden="true" />
  );
}
