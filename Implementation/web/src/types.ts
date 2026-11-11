/**
 * The HTTP contract.
 *
 * These are the shapes the C++ backend sends. They differ from the engine's own
 * types only where JSON forces it: a typed array of cell indices arrives as a
 * plain array. Nothing is added on the way out, because nothing sits between
 * the engine and the socket.
 *
 * If `server/src/main.cpp` changes a field name, change it here too — this file
 * is the only description of the wire format the browser has.
 */

export const CellKind = {
  Free: 0,
  Building: 1,
  Tree: 2,
  Water: 3,
  Rack: 4,
  /** A restricted zone. Only drawn maps use it; the built-in maps do not. */
  Block: 5,
} as const;
export type CellKind = (typeof CellKind)[keyof typeof CellKind];

export const CELL_KIND_LABEL: Record<CellKind, string> = {
  [CellKind.Free]: 'free',
  [CellKind.Building]: 'building',
  [CellKind.Tree]: 'tree',
  [CellKind.Water]: 'water',
  [CellKind.Rack]: 'warehouse rack',
  [CellKind.Block]: 'blocked zone',
};

/** Movement costs are integers so that route comparisons are exact. */
export const COST_STRAIGHT = 1000;
export const COST_DIAGONAL = 1414;

export const STRUCTURES = [
  'unsorted_array',
  'binary_heap',
  'bucket_queue',
  'four_ary_heap',
  'binomial_heap',
  'fibonacci_heap',
] as const;
export type StructureName = (typeof STRUCTURES)[number];

export interface PointDto {
  name: string;
  x: number;
  y: number;
}

export interface ScenarioDto {
  id: string;
  name: string;
  blurb: string;
  width: number;
  height: number;
  openCells: number;
  /** One digit per cell, row-major; each digit is a {@link CellKind}. */
  cells: string;
  starts: PointDto[];
  ends: PointDto[];
}

export interface ScenariosResponse {
  scenarios: ScenarioDto[];
  structures: string[];
  cellKindLabels: Record<string, string>;
}

export interface PlanQuery {
  scenarioId: string;
  startIndex: number;
  endIndex: number;
  structure?: number;
  reps?: number;
}

export interface PlanResponse {
  engine: 'c++';
  scenarioId: string;
  structure: string;
  found: boolean;
  /** Scaled by 1000. A straight step is 1000, a diagonal step 1414. */
  cost: number;
  expansions: number;
  inserts: number;
  extracts: number;
  stalePops: number;
  peakOpen: number;
  openCells: number;
  /** Median of `reps` timed runs in C++, in milliseconds. */
  ms: number;
  reps: number;
  /** Cell indices, start to goal. */
  path: number[];
  /** The order in which A* closed cells. The UI replays this, it does not re-simulate. */
  order: number[];
  /**
   * The grid route shortened to the waypoints a drone would fly (line-of-sight
   * smoothing). Empty when no route exists.
   */
  smoothPath: number[];
  /** Straight-line length of `path`, 1000 per cell. Same metric as `smoothLength`. */
  gridLength: number;
  /** Straight-line length of `smoothPath`. Never more than `gridLength`. */
  smoothLength: number;
}

/** POST /api/route: a map somebody drew. `cells` is one digit per cell, row-major. */
export interface RouteRequest {
  width: number;
  height: number;
  cells: string;
  start: number;
  end: number;
  structure?: number;
  reps?: number;
}

export interface StructureTimingDto {
  structure: string;
  ms: number;
  cost: number;
  expansions: number;
  inserts: number;
  extracts: number;
}

export interface CompareResponse {
  engine: 'c++';
  scenarioId: string;
  reps: number;
  results: StructureTimingDto[];
  /** False means one of the six is wrong, which is the whole point of the check. */
  allAgreeOnCost: boolean;
}

export interface HealthResponse {
  status: 'ok' | 'degraded';
  engine: { loaded: boolean; scenarios: number; structures: number };
  uptimeSeconds: number;
  version: string;
}

/** Every failure the API returns has this shape. */
export interface ApiErrorBody {
  error: {
    code: string;
    message: string;
    details?: unknown;
    requestId?: string;
  };
}

/** Decodes the packed `cells` string into one byte per cell. */
export function unpackCells(packed: string): Uint8Array {
  const out = new Uint8Array(packed.length);
  for (let i = 0; i < packed.length; i += 1) out[i] = packed.charCodeAt(i) - 48;
  return out;
}

export function isBlocked(kind: number): boolean {
  return kind !== CellKind.Free;
}
