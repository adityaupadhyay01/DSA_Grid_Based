import { useEffect, useRef } from 'react';
import { CELL_KIND_LABEL, CellKind, type PlanResponse, type ScenarioDto } from '../types.js';

interface Props {
  open: boolean;
  onClose: () => void;
  result: PlanResponse;
  scenario: ScenarioDto;
  from: string;
  to: string;
  obstacleCounts: Map<number, number>;
}

interface Step {
  unit: string;
  name: string;
  detail: React.ReactNode;
}

/**
 * The answer to "which algorithms ran?", built from the numbers this run
 * actually produced rather than from a fixed script.
 */
export function AlgorithmDialog({
  open,
  onClose,
  result,
  scenario,
  from,
  to,
  obstacleCounts,
}: Props) {
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

  const obstacles = [...obstacleCounts.entries()]
    .map(([kind, n]) => `${n} ${CELL_KIND_LABEL[kind as CellKind] ?? 'obstacle'} cells`)
    .join(', ');

  const totalCells = scenario.width * scenario.height;
  const steps: Step[] = result.found
    ? [
        {
          unit: 'Unit 2 — Graphs',
          name: 'The map became a graph',
          detail: (
            <>
              Every open cell became a <b>vertex</b> and every legal move a weighted <b>edge</b>.
              Buildings, trees, water and racks are cells with no edges into them — <b>{obstacles}</b>{' '}
              here, leaving <b>{result.openCells.toLocaleString()} open cells</b>. Neighbours are
              worked out by arithmetic when needed, so the {totalCells.toLocaleString()}-vertex
              graph is never stored as a matrix.
            </>
          ),
        },
        {
          unit: 'Unit 2 — Shortest path',
          name: 'A* search',
          detail: (
            <>
              A* repeatedly takes the most promising cell and looks at its neighbours, scoring each
              as <b>f = g + h</b>. It checked <b>{result.expansions.toLocaleString()} cells</b> out
              of {result.openCells.toLocaleString()} and never had to look at the rest.
            </>
          ),
        },
        {
          unit: 'Unit 1 — Trees and heaps',
          name: `Binary min-heap (${result.structure.replace(/_/g, ' ')})`,
          detail: (
            <>
              The cells waiting to be examined sit in a <b>min-heap</b>, so the cheapest comes out
              in O(log n) instead of a scan. It took <b>{result.inserts.toLocaleString()} insertions</b>{' '}
              and <b>{result.extracts.toLocaleString()} removals</b>, grew to{' '}
              <b>{result.peakOpen.toLocaleString()} cells</b>, and discarded{' '}
              <b>{result.stalePops.toLocaleString()} stale entries</b> on pop — that is lazy
              deletion, and it is sound because the heuristic is consistent.
            </>
          ),
        },
        {
          unit: 'Why the answer is trustworthy',
          name: 'Octile distance as the estimate',
          detail: (
            <>
              The estimate never overstates the real remaining distance, because it assumes a clear
              run. An estimate that never overstates is what guarantees the route is the{' '}
              <b>shortest one that exists</b>, not merely a good one.
            </>
          ),
        },
        {
          unit: 'Path reconstruction',
          name: 'Route rebuilt from parent links',
          detail: (
            <>
              Each cell remembered which cell it was reached from. Following those links back from{' '}
              {to} and reversing gives the route, <b>{result.path.length} cells long</b>.
            </>
          ),
        },
      ]
    : [
        {
          unit: 'Unit 2 — Graphs',
          name: 'The map became a graph',
          detail: (
            <>
              Open cells became vertices: <b>{obstacles}</b> and{' '}
              <b>{result.openCells.toLocaleString()} open cells</b>.
            </>
          ),
        },
        {
          unit: 'Unit 2 — Shortest path',
          name: 'A* searched everything reachable',
          detail: (
            <>
              A* checked <b>{result.expansions.toLocaleString()} cells</b> and the waiting list
              emptied before {to} was reached, which proves the two points are not connected.
            </>
          ),
        },
      ];

  const summary: [string, string][] = result.found
    ? [
        ['Route length (1000 per straight step)', result.cost.toLocaleString()],
        ['Steps taken by the drone', (result.path.length - 1).toLocaleString()],
        ['Cells the algorithm checked', result.expansions.toLocaleString()],
        ['Cells it never had to check', (result.openCells - result.expansions).toLocaleString()],
        [`Time the algorithm took (median of ${result.reps})`, `${result.ms.toFixed(3)} ms`],
        ['Computed by', 'the C++ backend, compiled at -O2'],
      ]
    : [];

  return (
    <div className="scrim" role="dialog" aria-modal="true" aria-labelledby="dialog-title" onClick={(e) => { if (e.target === e.currentTarget) onClose(); }}>
      <div className="modal">
        <header>
          <h3 id="dialog-title">
            {result.found ? `How the drone got from ${from} to ${to}` : 'Why there is no route'}
          </h3>
          <p>
            {scenario.name} · {scenario.width} × {scenario.height} grid · planned by the C++
            backend, replayed slowly on screen
          </p>
        </header>
        <div className="body">
          <ol className="algos">
            {steps.map((s) => (
              <li key={s.name}>
                <div>
                  <div className="unit">{s.unit}</div>
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
                <b>Result:</b> this is the shortest route that exists on this map — no other path
                from {from} to {to} can be shorter. A* examined{' '}
                {((100 * result.expansions) / result.openCells).toFixed(0)}% of the open cells to
                prove it.
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
