import { BRUSHES, TOOLS, type Tool } from '../lib/custom.js';
import { Swatch } from './Swatch.js';

interface Props {
  tool: Tool;
  onTool: (t: Tool) => void;
  brush: number;
  onBrush: (b: number) => void;
  onClear: () => void;
  onTown: () => void;
  disabled: boolean;
}

/** The drawing tools for the custom map: what to paint, how wide, and a way to start over. */
export function Editor({ tool, onTool, brush, onBrush, onClear, onTown, disabled }: Props) {
  return (
    <section className="step">
      <h2>
        <i>Step 2</i>&nbsp;&nbsp;Draw your own map
      </h2>
      <p className="note">
        Pick a tool, then drag across the grid below to paint. Place the start and the delivery
        point anywhere, then press <b>Find the shortest route</b>. The map is kept in this
        browser, so it survives a reload.
      </p>

      <div className="tools">
        {TOOLS.map((t) => (
          <button
            key={t.id}
            type="button"
            className="tool"
            aria-pressed={t.id === tool.id}
            onClick={() => onTool(t)}
          >
            {'point' in t ? (
              <i className="dot" style={{ background: `var(--${t.point})` }} />
            ) : (
              <Swatch kind={t.kind} size={18} />
            )}
            {t.label}
          </button>
        ))}
      </div>

      <div className="tools spaced">
        <div className="toolgroup">
          <label htmlFor="brush">brush size</label>
          <div className="brush" id="brush" role="group" aria-label="brush size">
            {BRUSHES.map((b) => (
              <button key={b} type="button" aria-pressed={b === brush} onClick={() => onBrush(b)}>
                {b}
              </button>
            ))}
          </div>
        </div>
        <div className="toolgroup">
          <label>map</label>
          <div className="row">
            <button type="button" className="minor" disabled={disabled} onClick={onClear}>
              Clear everything
            </button>
            <button type="button" className="minor" disabled={disabled} onClick={onTown}>
              Start from a town
            </button>
          </div>
        </div>
      </div>
    </section>
  );
}
