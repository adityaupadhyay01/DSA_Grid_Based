import { CellKind } from '../types.js';

/**
 * Canvas drawing, kept out of React.
 *
 * Each obstacle kind draws itself through `paintCell`, which the legend and the
 * editor call with the same arguments as the map. That is deliberate: a legend
 * built from separate swatches drifts away from the map the first time a colour
 * changes, and then it is quietly lying about what the picture means.
 *
 * Two offscreen layers keep a 100 x 100 map responsive. The terrain (10,000
 * cells, many of them arcs and strokes) is drawn once and then only the cells
 * that changed are repainted into it. The search layer is one pixel per cell,
 * scaled up with smoothing off, so tinting thousands of explored cells is a
 * single drawImage.
 *
 * The canvas is square, so maps are assumed to be square too.
 */

const NAMES = [
  '--free', '--seen', '--front',
  '--bldg', '--bldg-roof', '--bldg-win',
  '--grass', '--tree', '--tree-dark', '--trunk',
  '--water', '--water-line',
  '--rack', '--rack-line',
  '--block', '--block-line',
  '--route', '--start', '--end',
  '--line2', '--card', '--ink',
] as const;

export type Palette = Record<(typeof NAMES)[number], string>;

export function readPalette(root: HTMLElement = document.documentElement): Palette {
  const s = getComputedStyle(root);
  return Object.fromEntries(NAMES.map((n) => [n, s.getPropertyValue(n).trim()])) as Palette;
}

function rgb(hex: string): [number, number, number] {
  const h = hex.replace('#', '');
  const f = h.length === 3 ? [...h].map((c) => c + c).join('') : h;
  return [parseInt(f.slice(0, 2), 16), parseInt(f.slice(2, 4), 16), parseInt(f.slice(4, 6), 16)];
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
    g.fillStyle = p['--bldg'];
    g.fillRect(x, y, s, s);
    g.fillStyle = p['--bldg-roof'];
    g.fillRect(x, y, s, Math.max(1.5, s * 0.2));
    if (s >= 9) {
      g.fillStyle = p['--bldg-win'];
      const w = s * 0.22;
      const gap = s * 0.26;
      for (let r = 0; r < 2; r += 1) {
        for (let c = 0; c < 2; c += 1) {
          g.fillRect(x + gap + c * (w + gap * 0.7), y + s * 0.34 + r * (w + gap * 0.45), w, w);
        }
      }
    }
  } else if (kind === CellKind.Tree) {
    g.fillStyle = p['--grass'];
    g.fillRect(x, y, s, s);
    g.fillStyle = p['--trunk'];
    g.fillRect(x + s * 0.44, y + s * 0.58, Math.max(1.2, s * 0.13), s * 0.3);
    g.fillStyle = p['--tree'];
    g.beginPath();
    g.arc(x + s * 0.5, y + s * 0.44, s * 0.33, 0, Math.PI * 2);
    g.fill();
    g.fillStyle = p['--tree-dark'];
    g.beginPath();
    g.arc(x + s * 0.62, y + s * 0.36, s * 0.15, 0, Math.PI * 2);
    g.fill();
  } else if (kind === CellKind.Water) {
    g.fillStyle = p['--water'];
    g.fillRect(x, y, s, s);
    if (s >= 7) {
      g.strokeStyle = p['--water-line'];
      g.lineWidth = Math.max(1, s * 0.09);
      g.beginPath();
      g.moveTo(x + s * 0.12, y + s * 0.42);
      g.quadraticCurveTo(x + s * 0.34, y + s * 0.26, x + s * 0.56, y + s * 0.42);
      g.moveTo(x + s * 0.3, y + s * 0.72);
      g.quadraticCurveTo(x + s * 0.52, y + s * 0.56, x + s * 0.74, y + s * 0.72);
      g.stroke();
    }
  } else if (kind === CellKind.Rack) {
    g.fillStyle = p['--rack'];
    g.fillRect(x, y, s, s);
    g.strokeStyle = p['--rack-line'];
    g.lineWidth = Math.max(1, s * 0.1);
    g.beginPath();
    for (let k = 1; k <= 2; k += 1) {
      const yy = y + (s * k) / 3;
      g.moveTo(x, yy);
      g.lineTo(x + s, yy);
    }
    g.stroke();
  } else if (kind === CellKind.Block) {
    g.fillStyle = p['--block'];
    g.fillRect(x, y, s, s);
    g.strokeStyle = p['--block-line'];
    g.lineWidth = Math.max(1, s * 0.11);
    g.beginPath(); // a hatch, so it reads as concrete
    g.moveTo(x, y + s);
    g.lineTo(x + s, y);
    g.moveTo(x, y + s * 0.5);
    g.lineTo(x + s * 0.5, y);
    g.moveTo(x + s * 0.5, y + s);
    g.lineTo(x + s, y + s * 0.5);
    g.stroke();
  } else {
    g.fillStyle = p['--free'];
    g.fillRect(x, y, s, s);
  }
}

export interface Marker {
  index: number;
  label: string;
}

export interface MapView {
  width: number;
  height: number;
  /** Mutated in place while a map is being drawn; the renderer diffs it each frame. */
  cells: Uint8Array;
  start: Marker;
  end: Marker;
  /** The grid route A* found. */
  path?: readonly number[] | undefined;
  /** The same route as a flight line: waypoints only. */
  smoothPath?: readonly number[] | undefined;
}

/** How much of the engine's recorded search to show. */
export interface Replay {
  /** The order the C++ engine really closed cells in. */
  order: readonly number[];
  upto: number;
  /** Highlight the last few cells closed, the moving edge of the search. */
  front: boolean;
}

export class MapRenderer {
  private readonly terrain = document.createElement('canvas');
  private readonly search = document.createElement('canvas');
  private palette: Palette;

  private terrainOk = false;
  private snapshot = new Uint8Array(0);
  private gridW = 0;

  private order: readonly number[] | null = null;
  private upto = 0;
  private closed = new Uint8Array(0);
  private recent = new Uint8Array(0);
  private recentCells: number[] = [];

  constructor(private readonly canvas: HTMLCanvasElement) {
    this.palette = readPalette();
  }

  /** The colour scheme changed: re-read the palette and repaint everything. */
  setPalette(): void {
    this.palette = readPalette();
    this.terrainOk = false;
  }

  /** The canvas was resized, or something else invalidated the terrain layer. */
  invalidate(): void {
    this.terrainOk = false;
  }

  draw(view: MapView, replay?: Replay): void {
    const ctx = this.canvas.getContext('2d');
    if (!ctx) return;

    const px = this.fit();
    const { width: W, cells } = view;
    const cell = px / W;

    if (!this.terrainOk || this.gridW !== W || this.snapshot.length !== cells.length) {
      this.rebuild(view, px);
    } else {
      this.sync(view, px);
    }

    ctx.clearRect(0, 0, px, px);
    ctx.drawImage(this.terrain, 0, 0);
    this.drawSearch(ctx, view, replay, px);

    const cx = (i: number) => ((i % W) + 0.5) * cell;
    const cy = (i: number) => (Math.floor(i / W) + 0.5) * cell;
    const line = (pts: readonly number[]) => {
      ctx.beginPath();
      ctx.moveTo(cx(pts[0]!), cy(pts[0]!));
      for (let i = 1; i < pts.length; i += 1) ctx.lineTo(cx(pts[i]!), cy(pts[i]!));
      ctx.stroke();
    };

    const p = this.palette;
    ctx.lineJoin = 'round';
    ctx.lineCap = 'round';

    if (view.path && view.path.length > 1) {
      ctx.strokeStyle = p['--route'];
      ctx.lineWidth = Math.max(2.5, cell * 0.46);
      line(view.path);
    }

    // The flight line sits on top of the grid route: dashed and thin, with a
    // dot at every waypoint, so the two read as "what A* found" and "what the
    // drone would actually fly".
    if (view.smoothPath && view.smoothPath.length > 1) {
      ctx.strokeStyle = p['--ink'];
      ctx.lineWidth = Math.max(1.6, cell * 0.2);
      ctx.setLineDash([Math.max(4, cell * 0.9), Math.max(3, cell * 0.6)]);
      line(view.smoothPath);
      ctx.setLineDash([]);

      ctx.fillStyle = p['--card'];
      ctx.lineWidth = Math.max(1.4, cell * 0.16);
      for (const i of view.smoothPath) {
        ctx.beginPath();
        ctx.arc(cx(i), cy(i), Math.max(2.6, cell * 0.3), 0, Math.PI * 2);
        ctx.fill();
        ctx.stroke();
      }
    }

    this.pin(ctx, px, cell, cx(view.start.index), cy(view.start.index), p['--start'], view.start.label);
    this.pin(ctx, px, cell, cx(view.end.index), cy(view.end.index), p['--end'], view.end.label);
  }

  // ------------------------------------------------------------------ private

  private fit(): number {
    const r = this.canvas.getBoundingClientRect();
    const dpr = Math.min(2, window.devicePixelRatio || 1);
    const px = Math.max(320, Math.round(r.width * dpr));
    if (this.canvas.width !== px || this.canvas.height !== px) {
      this.canvas.width = px;
      this.canvas.height = px;
      this.terrainOk = false;
    }
    return px;
  }

  private rebuild(view: MapView, px: number): void {
    const { width: W, cells } = view;
    const g = this.terrain.getContext('2d');
    if (!g) return;
    this.terrain.width = px;
    this.terrain.height = px;
    g.clearRect(0, 0, px, px);

    const s = px / W;
    for (let y = 0; y < W; y += 1) {
      for (let x = 0; x < W; x += 1) {
        paintCell(g, cells[y * W + x] ?? 0, Math.floor(x * s), Math.floor(y * s), Math.ceil(s), this.palette);
      }
    }

    // Grid lines go on once, with the terrain. At 100 x 100 every line would
    // be a smear, so only every fifth is drawn.
    g.strokeStyle = this.palette['--line2'];
    g.lineWidth = 1;
    g.globalAlpha = 0.55;
    g.beginPath();
    const step = W <= 60 ? 1 : 5;
    for (let k = 0; k <= W; k += step) {
      const o = Math.round(k * s) + 0.5;
      g.moveTo(o, 0);
      g.lineTo(o, px);
      g.moveTo(0, o);
      g.lineTo(px, o);
    }
    g.stroke();
    g.globalAlpha = 1;

    this.snapshot = cells.slice();
    this.gridW = W;
    this.terrainOk = true;
  }

  /** Repaints only the cells that changed since the last frame. */
  private sync(view: MapView, px: number): void {
    const g = this.terrain.getContext('2d');
    if (!g) return;
    const { width: W, cells } = view;
    const s = px / W;
    for (let i = 0; i < cells.length; i += 1) {
      const k = cells[i] ?? 0;
      if (k === this.snapshot[i]) continue;
      this.snapshot[i] = k;
      const X = Math.floor((i % W) * s);
      const Y = Math.floor(Math.floor(i / W) * s);
      const S = Math.ceil(s);
      paintCell(g, k, X, Y, S, this.palette);
      if (W <= 60) {
        g.strokeStyle = this.palette['--line2'];
        g.lineWidth = 1;
        g.globalAlpha = 0.55;
        g.strokeRect(Math.round(X) + 0.5, Math.round(Y) + 0.5, S, S);
        g.globalAlpha = 1;
      }
    }
  }

  private drawSearch(
    ctx: CanvasRenderingContext2D,
    view: MapView,
    replay: Replay | undefined,
    px: number,
  ): void {
    if (!replay) {
      this.order = null;
      this.upto = 0;
      return;
    }

    const { width: W, height: H } = view;
    const total = W * H;
    if (replay.order !== this.order || replay.upto < this.upto || this.closed.length !== total) {
      this.order = replay.order;
      this.upto = 0;
      this.closed = new Uint8Array(total);
      this.recent = new Uint8Array(total);
      this.recentCells = [];
    }

    for (const c of this.recentCells) this.recent[c] = 0;
    this.recentCells.length = 0;
    for (let k = this.upto; k < replay.upto; k += 1) this.closed[replay.order[k]!] = 1;
    this.upto = replay.upto;
    if (replay.front) {
      for (let k = Math.max(0, replay.upto - 20); k < replay.upto; k += 1) {
        const c = replay.order[k]!;
        this.recent[c] = 1;
        this.recentCells.push(c);
      }
    }

    if (this.search.width !== W) this.search.width = W;
    if (this.search.height !== H) this.search.height = H;
    const sctx = this.search.getContext('2d');
    if (!sctx) return;
    const img = sctx.createImageData(W, H);
    const d = img.data;
    const seen = rgb(this.palette['--seen']);
    const front = rgb(this.palette['--front']);
    for (let i = 0; i < total; i += 1) {
      const c = this.recent[i] ? front : this.closed[i] ? seen : null;
      if (!c) continue;
      const o = i * 4;
      d[o] = c[0];
      d[o + 1] = c[1];
      d[o + 2] = c[2];
      d[o + 3] = 255;
    }
    sctx.putImageData(img, 0, 0);

    ctx.imageSmoothingEnabled = false;
    ctx.drawImage(this.search, 0, 0, px, px);
    ctx.imageSmoothingEnabled = true;
  }

  private pin(
    ctx: CanvasRenderingContext2D,
    px: number,
    cell: number,
    cx: number,
    cy: number,
    colour: string,
    label: string,
  ): void {
    const p = this.palette;
    const rr = Math.max(8, cell * 0.92);
    // Nudge pins on the very edge inward so the whole marker stays visible.
    const x = Math.min(px - rr - 1, Math.max(rr + 1, cx));
    const y = Math.min(px - rr - 1, Math.max(rr + 1, cy));

    ctx.beginPath();
    ctx.arc(x, y, rr, 0, Math.PI * 2);
    ctx.fillStyle = colour;
    ctx.fill();
    ctx.lineWidth = 2.5;
    ctx.strokeStyle = p['--card'];
    ctx.stroke();

    const fs = Math.max(11, Math.min(18, cell * 0.92));
    ctx.font = `600 ${fs}px "IBM Plex Sans", sans-serif`;
    const bw = ctx.measureText(label).width + 12;
    const bh = fs * 1.5;
    // Flip the label below the pin when there is no room above it.
    const above = y - rr - 6 - bh >= 0;
    const by = above ? y - rr - 6 - bh : y + rr + 6;
    const bx = Math.min(px - bw - 2, Math.max(2, x - bw / 2));

    ctx.fillStyle = p['--card'];
    ctx.fillRect(bx, by, bw, bh);
    ctx.strokeStyle = colour;
    ctx.lineWidth = 1.5;
    ctx.strokeRect(bx, by, bw, bh);
    ctx.fillStyle = p['--ink'];
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(label, bx + bw / 2, by + bh / 2 + 0.5);
  }
}

/** A swatch drawn by the same code as the map, for the legend and the editor. */
export function drawSwatch(canvas: HTMLCanvasElement, kind: number, size: number, p: Palette): void {
  const ctx = canvas.getContext('2d');
  if (!ctx) return;
  const dpr = Math.min(2, window.devicePixelRatio || 1);
  const px = Math.round(size * dpr);
  canvas.width = px;
  canvas.height = px;
  paintCell(ctx, kind, 0, 0, px, p);
}
