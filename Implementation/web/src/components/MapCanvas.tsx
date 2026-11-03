import { useEffect, useRef } from 'react';
import { CELL_KIND_LABEL, CellKind, unpackCells, type ScenarioDto } from '../types.js';
import { drawMap, drawSwatch, readPalette, type MapView, type Palette } from '../lib/render.js';

interface Props {
  scenario: ScenarioDto;
  startIndex: number;
  endIndex: number;
  /** Cells closed so far, as replayed from the engine's own expansion order. */
  closed?: Uint8Array | undefined;
  recent?: Uint8Array | undefined;
  path?: number[] | undefined;
}

function Legend({ cells, palette }: { cells: Uint8Array; palette: Palette }) {
  const present = [...new Set(Array.from(cells))].filter((k) => k !== CellKind.Free).sort();
  return (
    <div className="legend">
      {present.map((kind) => (
        <span key={kind}>
          <canvas
            className="swatch"
            ref={(el) => {
              if (el) drawSwatch(el, kind, palette);
            }}
          />
          {CELL_KIND_LABEL[kind as CellKind] ?? 'obstacle'}
        </span>
      ))}
      <span>
        <i className="chip" style={{ background: 'var(--seen)' }} /> cells the algorithm checked
      </span>
      <span>
        <i className="chip" style={{ background: 'var(--route)' }} /> route chosen
      </span>
      <span>
        <i className="chip" style={{ background: 'var(--start)' }} /> start
      </span>
      <span>
        <i className="chip" style={{ background: 'var(--end)' }} /> delivery point
      </span>
    </div>
  );
}

export function MapCanvas({ scenario, startIndex, endIndex, closed, recent, path }: Props) {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const cellsRef = useRef<Uint8Array>(unpackCells(scenario.cells));
  const paletteRef = useRef<Palette>({});

  // Re-decoding 1600 characters on every animation frame would be wasteful,
  // and the cells never change within a scenario.
  useEffect(() => {
    cellsRef.current = unpackCells(scenario.cells);
  }, [scenario.cells]);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    paletteRef.current = readPalette(document.documentElement);

    const start = scenario.starts[startIndex];
    const end = scenario.ends[endIndex];
    if (!start || !end) return;

    const view: MapView = {
      width: scenario.width,
      height: scenario.height,
      cells: cellsRef.current,
      closed,
      recent,
      path,
      start: { index: start.y * scenario.width + start.x, label: start.name },
      end: { index: end.y * scenario.width + end.x, label: end.name },
    };
    drawMap(canvas, view, paletteRef.current);
  }, [scenario, startIndex, endIndex, closed, recent, path]);

  // Both a resize and a theme change invalidate what is on the canvas.
  useEffect(() => {
    const redraw = () => {
      const canvas = canvasRef.current;
      const start = scenario.starts[startIndex];
      const end = scenario.ends[endIndex];
      if (!canvas || !start || !end) return;
      paletteRef.current = readPalette(document.documentElement);
      drawMap(
        canvas,
        {
          width: scenario.width,
          height: scenario.height,
          cells: cellsRef.current,
          closed,
          recent,
          path,
          start: { index: start.y * scenario.width + start.x, label: start.name },
          end: { index: end.y * scenario.width + end.x, label: end.name },
        },
        paletteRef.current,
      );
    };
    const media = window.matchMedia('(prefers-color-scheme: dark)');
    window.addEventListener('resize', redraw);
    media.addEventListener('change', redraw);
    return () => {
      window.removeEventListener('resize', redraw);
      media.removeEventListener('change', redraw);
    };
  }, [scenario, startIndex, endIndex, closed, recent, path]);

  return (
    <>
      <div className="canvas-wrap">
        <canvas
          ref={canvasRef}
          aria-label={`${scenario.name}: obstacles, the drone's starting point, the delivery point and the route`}
        />
      </div>
      <Legend cells={cellsRef.current} palette={paletteRef.current} />
    </>
  );
}
