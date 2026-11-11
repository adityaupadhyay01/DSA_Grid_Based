import { CellKind } from '../types.js';

/**
 * The map a person draws themselves.
 *
 * It is kept in localStorage so a reload does not throw away a map someone has
 * just spent two minutes drawing. Every access is guarded: storage can be
 * absent, full or disabled, and none of those should stop the page working.
 */

export const CUSTOM_SIZE = 100;
const SAVE_KEY = 'drone-planner-custom-v1';

export interface CustomMap {
  /** CUSTOM_SIZE x CUSTOM_SIZE, row-major, one CellKind per cell. Mutated in place. */
  cells: Uint8Array;
  start: number;
  end: number;
}

const DEFAULT_START = 3 * CUSTOM_SIZE + 3;
const DEFAULT_END = 96 * CUSTOM_SIZE + 96;

export function defaultCustom(): CustomMap {
  return {
    cells: new Uint8Array(CUSTOM_SIZE * CUSTOM_SIZE),
    start: DEFAULT_START,
    end: DEFAULT_END,
  };
}

export function loadCustom(): CustomMap | null {
  try {
    const raw = localStorage.getItem(SAVE_KEY);
    if (!raw) return null;
    const saved = JSON.parse(raw) as {
      size?: number;
      cells?: unknown;
      start?: unknown;
      end?: unknown;
    };
    const total = CUSTOM_SIZE * CUSTOM_SIZE;
    if (saved.size !== CUSTOM_SIZE || typeof saved.cells !== 'string') return null;
    if (saved.cells.length !== total) return null;

    const cells = new Uint8Array(total);
    for (let i = 0; i < total; i += 1) {
      const k = saved.cells.charCodeAt(i) - 48;
      cells[i] = k >= 0 && k <= CellKind.Block ? k : 0;
    }
    const inside = (v: unknown): v is number =>
      typeof v === 'number' && Number.isInteger(v) && v >= 0 && v < total;
    const start = inside(saved.start) ? saved.start : DEFAULT_START;
    const end = inside(saved.end) ? saved.end : DEFAULT_END;
    if (start === end) return null;
    // A saved point standing inside an obstacle would make the search fail for
    // a reason nobody can see.
    cells[start] = CellKind.Free;
    cells[end] = CellKind.Free;
    return { cells, start, end };
  } catch {
    return null;
  }
}

export function saveCustom(m: CustomMap): void {
  try {
    let s = '';
    for (let i = 0; i < m.cells.length; i += 1) s += String.fromCharCode(48 + (m.cells[i] ?? 0));
    localStorage.setItem(SAVE_KEY, JSON.stringify({ size: CUSTOM_SIZE, cells: s, start: m.start, end: m.end }));
  } catch {
    /* private window, quota, blocked storage — the map still works */
  }
}

/** A starting point someone can edit, rather than a blank 10,000-cell grid. */
export function townLayout(): CustomMap {
  const m = defaultCustom();
  const put = (x0: number, y0: number, w: number, h: number, k: number) => {
    for (let y = Math.max(0, y0); y < Math.min(CUSTOM_SIZE, y0 + h); y += 1) {
      for (let x = Math.max(0, x0); x < Math.min(CUSTOM_SIZE, x0 + w); x += 1) {
        m.cells[y * CUSTOM_SIZE + x] = k;
      }
    }
  };
  for (let by = 0; by < 6; by += 1) {
    for (let bx = 0; bx < 6; bx += 1) {
      const x = 6 + bx * 15;
      const y = 6 + by * 15;
      if ((bx + by) % 4 === 3) {
        for (let j = 0; j < 9; j += 1) {
          for (let i = 0; i < 9; i += 1) {
            if ((i + j) % 2 === 0) m.cells[(y + j) * CUSTOM_SIZE + x + i] = CellKind.Tree;
          }
        }
      } else {
        put(x, y, 9, 9, CellKind.Building);
      }
    }
  }
  put(0, 49, 100, 2, CellKind.Block); // a wall across the middle
  put(30, 49, 8, 2, CellKind.Free); //   with two gaps in it
  put(72, 49, 8, 2, CellKind.Free);
  return m;
}

// ---------------------------------------------------------------------------
// Tools
// ---------------------------------------------------------------------------
export type Tool =
  | { id: string; label: string; kind: number }
  | { id: string; label: string; point: 'start' | 'end' };

export const TOOLS: Tool[] = [
  { id: 'bldg', label: 'Building', kind: CellKind.Building },
  { id: 'tree', label: 'Tree', kind: CellKind.Tree },
  { id: 'block', label: 'Block', kind: CellKind.Block },
  { id: 'erase', label: 'Erase', kind: CellKind.Free },
  { id: 'start', label: 'Start point', point: 'start' },
  { id: 'end', label: 'Delivery point', point: 'end' },
];

export const BRUSHES = [1, 2, 4, 8] as const;

/** Returns true when the map actually changed. */
export function applyTool(m: CustomMap, tool: Tool, brush: number, i: number): boolean {
  if (i < 0 || i >= m.cells.length) return false;
  let changed = false;

  if ('point' in tool) {
    // Dropping a point onto an obstacle would make the search fail for a reason
    // nobody can see, so the cell under it is cleared first.
    if (m.cells[i] !== CellKind.Free) {
      m.cells[i] = CellKind.Free;
      changed = true;
    }
    if (tool.point === 'start') {
      if (i !== m.end && m.start !== i) {
        m.start = i;
        changed = true;
      }
    } else if (i !== m.start && m.end !== i) {
      m.end = i;
      changed = true;
    }
    return changed;
  }

  const x = i % CUSTOM_SIZE;
  const y = Math.floor(i / CUSTOM_SIZE);
  const r = brush - 1;
  const half = Math.floor(r / 2);
  for (let dy = -half; dy <= r - half; dy += 1) {
    for (let dx = -half; dx <= r - half; dx += 1) {
      const nx = x + dx;
      const ny = y + dy;
      if (nx < 0 || ny < 0 || nx >= CUSTOM_SIZE || ny >= CUSTOM_SIZE) continue;
      const j = ny * CUSTOM_SIZE + nx;
      // Never paint over the start or the delivery point.
      if (j === m.start || j === m.end) continue;
      if (m.cells[j] === tool.kind) continue;
      m.cells[j] = tool.kind;
      changed = true;
    }
  }
  return changed;
}

/**
 * Pointer events do not arrive once per cell: drag quickly and the cursor jumps
 * ten cells between two events, which would leave a dotted line instead of a
 * wall. So each move paints every cell on the segment from the previous
 * position, by Bresenham.
 */
export function paintLine(m: CustomMap, tool: Tool, brush: number, from: number, to: number): boolean {
  if (from < 0) return applyTool(m, tool, brush, to);
  let x0 = from % CUSTOM_SIZE;
  let y0 = Math.floor(from / CUSTOM_SIZE);
  const x1 = to % CUSTOM_SIZE;
  const y1 = Math.floor(to / CUSTOM_SIZE);
  const dx = Math.abs(x1 - x0);
  const sx = x0 < x1 ? 1 : -1;
  const dy = -Math.abs(y1 - y0);
  const sy = y0 < y1 ? 1 : -1;
  let err = dx + dy;
  let changed = false;
  for (let guard = 0; guard <= CUSTOM_SIZE * 2; guard += 1) {
    if (applyTool(m, tool, brush, y0 * CUSTOM_SIZE + x0)) changed = true;
    if (x0 === x1 && y0 === y1) break;
    const e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
  return changed;
}

/** One digit per cell, the way the API wants a drawn map. */
export function packCells(cells: Uint8Array): string {
  return new TextDecoder().decode(Uint8Array.from(cells, (c) => c + 48));
}
