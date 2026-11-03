import { CellKind } from '../types.js';

/**
 * Canvas drawing, kept out of React.
 *
 * Each obstacle kind draws itself through `paintCell`, which the legend calls
 * with the same arguments as the map. That is deliberate: a legend built from
 * separate swatches drifts away from the map the first time a colour changes,
 * and then it is quietly lying about what the picture means.
 */

export type Palette = Record<string, string>;

export function readPalette(root: HTMLElement): Palette {
  const s = getComputedStyle(root);
  const names = [
    '--free',
    '--seen',
    '--front',
    '--bldg',
    '--bldg-roof',
    '--bldg-win',
    '--grass',
    '--tree',
    '--tree-dark',
    '--trunk',
    '--water',
    '--water-line',
    '--rack',
    '--rack-line',
    '--route',
    '--start',
    '--end',
    '--line2',
    '--card',
    '--ink',
  ];
  return Object.fromEntries(names.map((n) => [n, s.getPropertyValue(n).trim()]));
}

export function paintCell(
  g: CanvasRenderingContext2D,
  kind: number,
  x: number,
  y: number,
  s: number,
  p: Palette,
): void {
  if (kind === CellKind.Building) {
    g.fillStyle = p['--bldg']!;
    g.fillRect(x, y, s, s);
    g.fillStyle = p['--bldg-roof']!;
    g.fillRect(x, y, s, Math.max(1.5, s * 0.2));
    if (s >= 9) {
      g.fillStyle = p['--bldg-win']!;
      const w = s * 0.22;
      const gap = s * 0.26;
      for (let r = 0; r < 2; r += 1) {
        for (let c = 0; c < 2; c += 1) {
          g.fillRect(x + gap + c * (w + gap * 0.7), y + s * 0.34 + r * (w + gap * 0.45), w, w);
        }
      }
    }
  } else if (kind === CellKind.Tree) {
    g.fillStyle = p['--grass']!;
    g.fillRect(x, y, s, s);
    g.fillStyle = p['--trunk']!;
    g.fillRect(x + s * 0.44, y + s * 0.58, Math.max(1.2, s * 0.13), s * 0.3);
    g.fillStyle = p['--tree']!;
    g.beginPath();
    g.arc(x + s * 0.5, y + s * 0.44, s * 0.33, 0, Math.PI * 2);
    g.fill();
    g.fillStyle = p['--tree-dark']!;
    g.beginPath();
    g.arc(x + s * 0.62, y + s * 0.36, s * 0.15, 0, Math.PI * 2);
    g.fill();
  } else if (kind === CellKind.Water) {
    g.fillStyle = p['--water']!;
    g.fillRect(x, y, s, s);
    if (s >= 7) {
      g.strokeStyle = p['--water-line']!;
      g.lineWidth = Math.max(1, s * 0.09);
      g.beginPath();
      g.moveTo(x + s * 0.12, y + s * 0.42);
      g.quadraticCurveTo(x + s * 0.34, y + s * 0.26, x + s * 0.56, y + s * 0.42);
      g.moveTo(x + s * 0.3, y + s * 0.72);
      g.quadraticCurveTo(x + s * 0.52, y + s * 0.56, x + s * 0.74, y + s * 0.72);
      g.stroke();
    }
  } else if (kind === CellKind.Rack) {
    g.fillStyle = p['--rack']!;
    g.fillRect(x, y, s, s);
    g.strokeStyle = p['--rack-line']!;
    g.lineWidth = Math.max(1, s * 0.1);
    g.beginPath();
    for (let k = 1; k <= 2; k += 1) {
      const yy = y + (s * k) / 3;
      g.moveTo(x, yy);
      g.lineTo(x + s, yy);
    }
    g.stroke();
  }
}

export interface MapView {
  width: number;
  height: number;
  cells: Uint8Array;
  /** Cells A* has closed so far. */
  closed?: Uint8Array | undefined;
  /** The handful of cells closed most recently — a real wavefront, not an invented one. */
  recent?: Uint8Array | undefined;
  path?: number[] | undefined;
  start: { index: number; label: string };
  end: { index: number; label: string };
}

export function drawMap(canvas: HTMLCanvasElement, view: MapView, p: Palette): void {
  const ctx = canvas.getContext('2d');
  if (!ctx) return;

  const rect = canvas.getBoundingClientRect();
  const dpr = Math.min(2, window.devicePixelRatio || 1);
  const px = Math.max(320, Math.round(rect.width * dpr));
  if (canvas.width !== px) {
    canvas.width = px;
    canvas.height = px;
  }

  const { width: W, height: H, cells } = view;
  const cell = px / W;
  ctx.clearRect(0, 0, px, px);

  for (let y = 0; y < H; y += 1) {
    for (let x = 0; x < W; x += 1) {
      const i = y * W + x;
      const X = Math.floor(x * cell);
      const Y = Math.floor(y * cell);
      const s = Math.ceil(cell);
      const kind = cells[i] ?? 0;
      if (kind === CellKind.Free) {
        ctx.fillStyle = view.recent?.[i]
          ? p['--front']!
          : view.closed?.[i]
            ? p['--seen']!
            : p['--free']!;
        ctx.fillRect(X, Y, s, s);
      } else {
        paintCell(ctx, kind, X, Y, s, p);
      }
    }
  }

  ctx.strokeStyle = p['--line2']!;
  ctx.lineWidth = 1;
  ctx.globalAlpha = 0.55;
  ctx.beginPath();
  for (let k = 0; k <= W; k += 1) {
    const o = Math.round(k * cell) + 0.5;
    ctx.moveTo(o, 0);
    ctx.lineTo(o, px);
    ctx.moveTo(0, o);
    ctx.lineTo(px, o);
  }
  ctx.stroke();
  ctx.globalAlpha = 1;

  const cx = (i: number) => ((i % W) + 0.5) * cell;
  const cy = (i: number) => (Math.floor(i / W) + 0.5) * cell;

  if (view.path && view.path.length > 1) {
    ctx.strokeStyle = p['--route']!;
    ctx.lineWidth = Math.max(3, cell * 0.46);
    ctx.lineJoin = 'round';
    ctx.lineCap = 'round';
    ctx.beginPath();
    ctx.moveTo(cx(view.path[0]!), cy(view.path[0]!));
    for (let i = 1; i < view.path.length; i += 1) ctx.lineTo(cx(view.path[i]!), cy(view.path[i]!));
    ctx.stroke();
  }

  const pin = (index: number, colour: string, label: string) => {
    const rr = Math.max(8, cell * 0.92);
    // Nudge pins on the very edge inward so the whole marker stays visible.
    const x = Math.min(px - rr - 1, Math.max(rr + 1, cx(index)));
    const y = Math.min(px - rr - 1, Math.max(rr + 1, cy(index)));

    ctx.beginPath();
    ctx.arc(x, y, rr, 0, Math.PI * 2);
    ctx.fillStyle = colour;
    ctx.fill();
    ctx.lineWidth = 2.5;
    ctx.strokeStyle = p['--card']!;
    ctx.stroke();

    const fs = Math.max(11, cell * 0.92);
    ctx.font = `600 ${fs}px "IBM Plex Sans", sans-serif`;
    const bw = ctx.measureText(label).width + 12;
    const bh = fs * 1.5;
    const above = y - rr - 6 - bh >= 0;
    const by = above ? y - rr - 6 - bh : y + rr + 6;
    const bx = Math.min(px - bw - 2, Math.max(2, x - bw / 2));

    ctx.fillStyle = p['--card']!;
    ctx.fillRect(bx, by, bw, bh);
    ctx.strokeStyle = colour;
    ctx.lineWidth = 1.5;
    ctx.strokeRect(bx, by, bw, bh);
    ctx.fillStyle = p['--ink']!;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(label, bx + bw / 2, by + bh / 2 + 0.5);
  };

  pin(view.start.index, p['--start']!, view.start.label);
  pin(view.end.index, p['--end']!, view.end.label);
}

/** A 20px swatch drawn by the same code as the map, for the legend. */
export function drawSwatch(canvas: HTMLCanvasElement, kind: number, p: Palette): void {
  const ctx = canvas.getContext('2d');
  if (!ctx) return;
  const dpr = Math.min(2, window.devicePixelRatio || 1);
  const size = Math.round(20 * dpr);
  canvas.width = size;
  canvas.height = size;
  paintCell(ctx, kind, 0, 0, size, p);
}
