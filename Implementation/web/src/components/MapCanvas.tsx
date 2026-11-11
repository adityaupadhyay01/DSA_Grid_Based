import { useEffect, useRef } from 'react';
import { MapRenderer, type MapView, type Replay } from '../lib/render.js';
import { useThemeEpoch } from '../lib/theme.js';

interface Props {
  view: MapView;
  /** The engine's recorded search, replayed up to `upto` cells. */
  replay?: Replay | undefined;
  /** True on the custom map when nothing is running: pointer events paint. */
  editable: boolean;
  label: string;
  onPaintStart: (cell: number) => void;
  onPaintMove: (from: number, to: number) => void;
  onPaintEnd: () => void;
}

export function MapCanvas({
  view,
  replay,
  editable,
  label,
  onPaintStart,
  onPaintMove,
  onPaintEnd,
}: Props) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const renderer = useRef<MapRenderer | null>(null);
  const theme = useThemeEpoch();

  // The resize observer outlives any one render, so it reads the latest props
  // from here instead of capturing stale ones.
  const latest = useRef({ view, replay });
  latest.current = { view, replay };

  const painting = useRef(false);
  const lastCell = useRef(-1);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    const r = new MapRenderer(canvas);
    renderer.current = r;
    const observer = new ResizeObserver(() => {
      r.invalidate();
      r.draw(latest.current.view, latest.current.replay);
    });
    observer.observe(canvas);
    return () => {
      observer.disconnect();
      renderer.current = null;
    };
  }, []);

  useEffect(() => {
    renderer.current?.setPalette();
  }, [theme]);

  // Every render repaints. The renderer diffs the cells itself, so this is
  // cheap, and it means the picture cannot fall out of step with the state.
  useEffect(() => {
    renderer.current?.draw(view, replay);
  });

  const cellAt = (e: React.PointerEvent<HTMLCanvasElement>): number => {
    const r = e.currentTarget.getBoundingClientRect();
    const x = Math.floor(((e.clientX - r.left) / r.width) * view.width);
    const y = Math.floor(((e.clientY - r.top) / r.height) * view.height);
    if (x < 0 || y < 0 || x >= view.width || y >= view.height) return -1;
    return y * view.width + x;
  };

  return (
    <div className="canvas-wrap">
      <canvas
        ref={canvasRef}
        className={editable ? 'editing' : undefined}
        aria-label={label}
        onPointerDown={(e) => {
          if (!editable) return;
          painting.current = true;
          e.currentTarget.setPointerCapture(e.pointerId);
          lastCell.current = cellAt(e);
          onPaintStart(lastCell.current);
        }}
        onPointerMove={(e) => {
          if (!painting.current) return;
          const c = cellAt(e);
          if (c < 0 || c === lastCell.current) return;
          onPaintMove(lastCell.current, c);
          lastCell.current = c;
        }}
        onPointerUp={() => {
          if (!painting.current) return;
          painting.current = false;
          lastCell.current = -1;
          onPaintEnd();
        }}
        onPointerCancel={() => {
          painting.current = false;
          lastCell.current = -1;
        }}
      />
    </div>
  );
}
