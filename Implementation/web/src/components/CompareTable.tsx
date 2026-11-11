import type { CompareResponse } from '../types.js';

interface Props {
  data?: CompareResponse | undefined;
  loading: boolean;
  error?: string | undefined;
  onRun: () => void;
  disabled: boolean;
}

const pretty = (s: string) => s.replace(/_/g, ' ');

/**
 * The project's actual research question, on whatever route is selected.
 * A* is fixed; only the OPEN list changes, so any difference in time belongs
 * to the structure.
 */
export function CompareTable({ data, loading, error, onRun, disabled }: Props) {
  const rows = data ? [...data.results].sort((a, b) => a.ms - b.ms) : [];
  const fastest = rows[0];
  const slowest = rows[rows.length - 1];

  return (
    <section className="compare">
      <h2>Same route, six OPEN lists</h2>
      <p>
        A* is held fixed and only the structure holding its waiting list is swapped. All six must
        return the same route; if they do not, one of them is wrong.
      </p>
      <button type="button" className="secondary" onClick={onRun} disabled={disabled || loading}>
        {loading ? 'Running six searches in C++…' : 'Run this route with all six OPEN lists'}
      </button>

      {error ? <p className="verdict bad">{error}</p> : null}

      {rows.length > 0 && fastest && slowest ? (
        <>
          <div className="scroll">
            <table>
              <thead>
                <tr>
                  <th>OPEN list</th>
                  <th>time</th>
                  <th>vs fastest</th>
                  <th>cells expanded</th>
                  <th>insertions</th>
                  <th>route cost</th>
                </tr>
              </thead>
              <tbody>
                {rows.map((r) => (
                  <tr key={r.structure} className={r === fastest ? 'best' : undefined}>
                    <td>{pretty(r.structure)}</td>
                    <td>{r.ms.toFixed(3)} ms</td>
                    <td>{r === fastest ? 'fastest' : `${(r.ms / fastest.ms).toFixed(1)}x`}</td>
                    <td>{r.expansions.toLocaleString()}</td>
                    <td>{r.inserts.toLocaleString()}</td>
                    <td>{r.cost.toLocaleString()}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
          <p className={data?.allAgreeOnCost ? 'verdict' : 'verdict bad'}>
            {data?.allAgreeOnCost ? (
              <>
                <strong>All six returned the same route</strong> — cost{' '}
                {fastest.cost.toLocaleString()} — so the structure changed only the time:{' '}
                {pretty(fastest.structure)} at {fastest.ms.toFixed(3)} ms against{' '}
                {pretty(slowest.structure)} at {slowest.ms.toFixed(3)} ms, a factor of{' '}
                {(slowest.ms / fastest.ms).toFixed(1)}. Median of {data.reps} runs each, after a
                discarded warm-up.
              </>
            ) : (
              <>
                <strong>The structures disagreed on the route cost.</strong> One of them is wrong.
                This is exactly the check that caught the bucket queue sized at C+1.
              </>
            )}
          </p>
        </>
      ) : null}
    </section>
  );
}
