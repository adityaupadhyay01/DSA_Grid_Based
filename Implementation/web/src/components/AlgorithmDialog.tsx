import { useEffect, useRef, type ReactNode } from 'react';
import { CELL_KIND_LABEL, type CellKind, type PlanResponse } from '../types.js';

/** Everything the dialog says is taken from one finished run. */
export interface RunSummary {
  result: PlanResponse;
  from: string;
  to: string;
  mapName: string;
  width: number;
  height: number;
  /** [cell kind, how many] for every obstacle kind on the map at the time of the run. */
  counts: [number, number][];
}

interface Props {
  open: boolean;
  onClose: () => void;
  run: RunSummary;
}

interface Step {
  topic: string;
  name: string;
  detail: ReactNode;
}

const OPEN_LIST: Record<string, { name: string; how: string }> = {
  binary_heap: { name: 'Binary min-heap', how: 'so the cheapest comes out in O(log n) instead of a scan' },
  four_ary_heap: { name: '4-ary min-heap', how: 'a shallower heap with four children per node' },
  bucket_queue: { name: 'Bucket queue', how: 'costs are whole numbers, so cells are filed into buckets by score and the cheapest bucket is read directly' },
  binomial_heap: { name: 'Binomial heap', how: 'a forest of trees merged like binary addition' },
  fibonacci_heap: { name: 'Fibonacci heap', how: 'lazily linked trees, consolidated on removal' },
  unsorted_array: { name: 'Unsorted array', how: 'a plain list scanned for the cheapest each time' },
};

const n = (v: number) => v.toLocaleString();
const cells = (scaled: number) => (scaled / 1000).toFixed(1);

/**
 * The answer to "which algorithms ran?", built from the numbers this run
 * actually produced rather than from a fixed script. Nothing here is a claim
 * the engine did not measure.
 */
export function AlgorithmDialog({ open, onClose, run }: Props) {
  const closeRef = useRef<HTMLButtonElement>(null);

  useEffect(() => {
    if (!open) return;
    closeRef.current?.focus();
    const onKey = (e: KeyboardEvent) => {
      if (e.key === 'Escape') onClose();
    };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
  }, [open, onClose]);

  if (!open) return null;

  const { result, from, to, mapName, width, height, counts } = run;
  const total = width * height;
  const obstacles = counts.length
    ? counts.map(([k, c]) => `${n(c)} ${CELL_KIND_LABEL[k as CellKind] ?? 'obstacle'} cells`).join(', ')
    : 'no obstacles at all';

  const list = OPEN_LIST[result.structure] ?? { name: result.structure.replace(/_/g, ' '), how: '' };
  const saved =
    result.gridLength > 0 ? (100 * (result.gridLength - result.smoothLength)) / result.gridLength : 0;

  const model: Step = {
    topic: 'Map model',
    name: 'The map became a graph',
    detail: (
      <>
        Every open cell became a <b>vertex</b> and every legal move a weighted <b>edge</b>, in
        eight directions. Obstacles are cells with no edges into them — <b>{obstacles}</b> here,
        leaving <b>{n(result.openCells)} open cells</b>. Neighbours are worked out by arithmetic
        when needed, so the {n(total)}-vertex graph is never stored as a matrix.
      </>
    ),
  };

  const steps: Step[] = result.found
    ? [
        model,
        {
          topic: 'Search',
          name: 'A* search',
          detail: (
            <>
              A* repeatedly takes the most promising cell and looks at its neighbours, scoring each
              as <b>f = g + h</b>: the distance already travelled plus an estimate of what is left.
              It checked <b>{n(result.expansions)} cells</b> out of {n(result.openCells)} and never
              had to look at the rest.
            </>
          ),
        },
        {
          topic: 'Priority queue',
          name: `${list.name} holds the waiting cells`,
          detail: (
            <>
              The cells waiting to be examined sit in a <b>{list.name.toLowerCase()}</b>
              {list.how ? `, ${list.how}` : ''}. It took <b>{n(result.inserts)} insertions</b> and{' '}
              <b>{n(result.extracts)} removals</b>, grew to <b>{n(result.peakOpen)} cells</b>, and
              discarded <b>{n(result.stalePops)} stale entries</b> when they surfaced. That is lazy
              deletion, and it is sound because the estimate below is consistent.
            </>
          ),
        },
        {
          topic: 'Heuristic',
          name: 'Octile distance as the estimate',
          detail: (
            <>
              The estimate never overstates the real remaining distance, because it assumes a clear
              run with nothing in the way. An estimate that never overstates is what guarantees the
              grid route is the <b>shortest one that exists</b>, not merely a good one.
            </>
          ),
        },
        {
          topic: 'Path',
          name: 'Route rebuilt from parent links',
          detail: (
            <>
              Each cell remembered which cell it was reached from. Following those links back from{' '}
              {to} and reversing gives the grid route, <b>{n(result.path.length)} cells long</b>.
            </>
          ),
        },
        {
          topic: 'Flight line',
          name: 'Line-of-sight smoothing',
          detail: (
            <>
              A grid route turns at every cell. From each waypoint the planner jumps to the farthest
              later cell it can see in a straight line without touching an obstacle or clipping a
              corner. That cut <b>{n(result.path.length)} cells to {n(result.smoothPath.length)}{' '}
              waypoints</b> and the straight-line distance from <b>{cells(result.gridLength)}</b> to{' '}
              <b>{cells(result.smoothLength)} cells</b>
              {saved > 0.05 ? <> ({saved.toFixed(1)}% shorter)</> : null}.
            </>
          ),
        },
      ]
    : [
        model,
        {
          topic: 'Search',
          name: 'A* searched everything reachable',
          detail: (
            <>
              A* checked <b>{n(result.expansions)} cells</b> and the waiting list emptied before{' '}
              {to} was reached, which proves the two points are not connected.
            </>
          ),
        },
      ];

  const summary: [string, string][] = result.found
    ? [
        ['Grid route cost (1000 per straight step)', n(result.cost)],
        ['Steps along the grid route', n(result.path.length - 1)],
        ['Cells the algorithm checked', n(result.expansions)],
        ['Cells it never had to check', n(result.openCells - result.expansions)],
        ['Waypoints in the flight line', `${n(result.smoothPath.length)} (from ${n(result.path.length)})`],
        ['Flight distance, in cells', `${cells(result.smoothLength)} (grid route ${cells(result.gridLength)})`],
        [`Search time (median of ${result.reps})`, `${result.ms.toFixed(3)} ms`],
        ['Computed by', 'the C++ backend, compiled at -O2'],
      ]
    : [];

  return (
    <div
      className="scrim"
      role="dialog"
      aria-modal="true"
      aria-labelledby="dialog-title"
      onClick={(e) => {
        if (e.target === e.currentTarget) onClose();
      }}
    >
      <div className="modal">
        <header>
          <h3 id="dialog-title">
            {result.found ? `How the drone got from ${from} to ${to}` : 'Why there is no route'}
          </h3>
          <p>
            {mapName} · {width} × {height} grid · planned by the C++ backend, replayed slowly on
            screen so it can be followed
          </p>
        </header>
        <div className="body">
          <ol className="algos">
            {steps.map((s) => (
              <li key={s.name}>
                <div>
                  <div className="unit">{s.topic}</div>
                  <div className="name">{s.name}</div>
                  <div className="detail">{s.detail}</div>
                </div>
              </li>
            ))}
          </ol>

          {summary.length > 0 ? (
            <dl className="summary">
              {summary.map(([k, v]) => (
                <div key={k}>
                  <dt>{k}</dt>
                  <dd>{v}</dd>
                </div>
              ))}
            </dl>
          ) : null}

          <div className="verdict">
            {result.found ? (
              <>
                <b>Result:</b> the orange line is the shortest route that exists on this grid when
                the drone moves in eight directions — no other path from {from} to {to} is shorter.
                A* examined {((100 * result.expansions) / result.openCells).toFixed(0)}% of the open
                cells to prove it. The dashed flight line is a straight-line shortcut of that route;
                it is safe and never longer, but it is not claimed to be the shortest any-angle path
                there could be.
              </>
            ) : (
              <>
                <b>Result:</b> {from} and {to} are walled off from each other here. The algorithm is
                correct to report no route.
              </>
            )}
          </div>

          <button type="button" className="close" ref={closeRef} onClick={onClose}>
            Close
          </button>
        </div>
      </div>
    </div>
  );
}
