import { memo } from 'react';
import { CELL_KIND_LABEL, CellKind } from '../types.js';
import { Swatch } from './Swatch.js';

interface Props {
  /** The obstacle kinds that are actually on this map. */
  kinds: number[];
  /** Whether a flight line is on screen, so the legend only explains what is drawn. */
  showFlightLine: boolean;
}

export const Legend = memo(function Legend({ kinds, showFlightLine }: Props) {
  return (
    <div className="legend">
      {kinds.map((kind) => (
        <span key={kind}>
          <Swatch kind={kind} size={20} />
          {CELL_KIND_LABEL[kind as CellKind] ?? 'obstacle'}
        </span>
      ))}
      <span>
        <i className="chip" style={{ background: 'var(--seen)' }} /> cells the algorithm checked
      </span>
      <span>
        <i className="chip" style={{ background: 'var(--route)' }} /> shortest route on the grid
      </span>
      {showFlightLine ? (
        <span>
          <i className="chip dashed" /> flight line (waypoints only)
        </span>
      ) : null}
      <span>
        <i className="chip" style={{ background: 'var(--start)' }} /> start
      </span>
      <span>
        <i className="chip" style={{ background: 'var(--end)' }} /> delivery point
      </span>
    </div>
  );
});
