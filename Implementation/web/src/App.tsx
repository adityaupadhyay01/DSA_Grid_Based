import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { useMutation, useQuery } from '@tanstack/react-query';
import {
  CellKind,
  unpackCells,
  type CompareResponse,
  type PlanResponse,
  type ScenarioDto,
} from './types.js';
import { api, ApiError } from './api/client.js';
import { MapCanvas } from './components/MapCanvas.js';
import { StepPicker } from './components/StepPicker.js';
import { CompareTable } from './components/CompareTable.js';
import { AlgorithmDialog } from './components/AlgorithmDialog.js';

const sleep = (ms: number) => new Promise<void>((r) => setTimeout(r, ms));

export function App() {
  const [scenarioIndex, setScenarioIndex] = useState(0);
  const [startIndex, setStartIndex] = useState(0);
  const [endIndex, setEndIndex] = useState(0);

  const [closed, setClosed] = useState<Uint8Array | undefined>();
  const [recent, setRecent] = useState<Uint8Array | undefined>();
  const [path, setPath] = useState<number[] | undefined>();
  const [status, setStatus] = useState('Waiting to start');
  const [result, setResult] = useState<PlanResponse | undefined>();
  const [dialogOpen, setDialogOpen] = useState(false);
  const [comparison, setComparison] = useState<CompareResponse | undefined>();

  // A replay in flight must stop when the user changes the map underneath it.
  const runToken = useRef(0);

  const scenariosQuery = useQuery({ queryKey: ['scenarios'], queryFn: api.scenarios });
  const scenarios = scenariosQuery.data?.scenarios ?? [];
  const scenario: ScenarioDto | undefined = scenarios[scenarioIndex];

  const resetRoute = useCallback(() => {
    runToken.current += 1;
    setClosed(undefined);
    setRecent(undefined);
    setPath(undefined);
    setResult(undefined);
    setComparison(undefined);
    setStatus('Waiting to start');
  }, []);

  const obstacleCounts = useMemo(() => {
    const counts = new Map<number, number>();
    if (!scenario) return counts;
    for (const k of unpackCells(scenario.cells)) {
      if (k !== CellKind.Free) counts.set(k, (counts.get(k) ?? 0) + 1);
    }
    return counts;
  }, [scenario]);

  const planMutation = useMutation({
    mutationFn: async () => {
      if (!scenario) throw new Error('no scenario loaded');
      return api.plan({ scenarioId: scenario.id, startIndex, endIndex });
    },
    onSuccess: async (data) => {
      const token = (runToken.current += 1);
      const total = scenario ? scenario.width * scenario.height : 0;

      // The engine returns the order in which it actually closed cells. This
      // replays that; it is not a second search running in the browser.
      const closedSet = new Uint8Array(total);
      const recentSet = new Uint8Array(total);
      const step = Math.max(1, Math.round(data.order.length / 200));

      for (let i = 0; i < data.order.length; i += step) {
        if (runToken.current !== token) return;
        const upto = Math.min(data.order.length, i + step);
        for (let k = i; k < upto; k += 1) closedSet[data.order[k]!] = 1;
        recentSet.fill(0);
        for (let k = Math.max(0, upto - 20); k < upto; k += 1) recentSet[data.order[k]!] = 1;

        setClosed(closedSet.slice());
        setRecent(recentSet.slice());
        setStatus(`A* is checking cells… ${upto} checked`);
        await sleep(16);
      }
      if (runToken.current !== token) return;

      setRecent(undefined);
      setPath(data.path);
      setResult(data);
      setStatus(
        data.found
          ? `Route found · length ${data.cost.toLocaleString()} · ${data.expansions} cells checked`
          : 'No route exists between these two points',
      );
      await sleep(350);
      if (runToken.current === token) setDialogOpen(true);
    },
  });

  const compareMutation = useMutation({
    mutationFn: async () => {
      if (!scenario) throw new Error('no scenario loaded');
      return api.compare({ scenarioId: scenario.id, startIndex, endIndex });
    },
    onSuccess: setComparison,
  });

  useEffect(() => {
    resetRoute();
  }, [scenarioIndex, startIndex, endIndex, resetRoute]);

  const healthQuery = useQuery({ queryKey: ['health'], queryFn: api.health, retry: 0 });

  if (scenariosQuery.isPending) {
    return (
      <main className="shell">
        <p className="loading">Loading the maps from the engine…</p>
      </main>
    );
  }

  if (scenariosQuery.isError || !scenario) {
    const err = scenariosQuery.error;
    return (
      <main className="shell">
        <h1>Drone Path Planning</h1>
        <p className="verdict bad">
          Could not reach the API.{' '}
          {err instanceof ApiError ? `${err.code}: ${err.message}` : String(err)}
          <br />
          Start it with <code>npm run dev -w @drone-planner/server</code>.
        </p>
      </main>
    );
  }

  const busy = planMutation.isPending || compareMutation.isPending;

  return (
    <main className="shell">
      <header className="masthead">
        <div>
          <h1>Drone Path Planning — what the drone does, step by step</h1>
          <div className="byline">
            Aditya Upadhyay · 2501330100038 · B.Tech CSE-A Sem III · DSA II (CCSE0301) · NIET
            Greater Noida
          </div>
        </div>
        <span className="engine" data-ok={healthQuery.data?.engine.loaded ? 'yes' : 'no'}>
          {healthQuery.data?.engine.loaded
            ? `C++ engine · ${healthQuery.data.engine.structures} OPEN lists`
            : 'engine status unknown'}
        </span>
      </header>

      <p className="intro">
        Pick a <b>map</b>, pick where the drone <b>starts</b>, pick where it must <b>deliver</b>.
        The drone cannot fly through buildings, trees, water or warehouse racks. The C++ engine
        finds the shortest safe route, and then this page shows which algorithms and data
        structures it used.
      </p>

      <StepPicker
        step={1}
        title="Choose a map"
        options={scenarios.map((s) => ({ name: s.name, blurb: s.blurb }))}
        selected={scenarioIndex}
        disabled={busy}
        onSelect={(i) => {
          setScenarioIndex(i);
          setStartIndex(0);
          setEndIndex(0);
        }}
      />
      <StepPicker
        step={2}
        title="Choose the starting point"
        options={scenario.starts.map((p) => ({ name: p.name }))}
        selected={startIndex}
        disabled={busy}
        onSelect={setStartIndex}
      />
      <StepPicker
        step={3}
        title="Choose the delivery point"
        options={scenario.ends.map((p) => ({ name: p.name }))}
        selected={endIndex}
        disabled={busy}
        onSelect={setEndIndex}
      />

      <div className="go-row">
        <button
          type="button"
          className="primary"
          disabled={busy}
          onClick={() => planMutation.mutate()}
        >
          {planMutation.isPending ? 'Planning…' : 'Find the shortest route'}
        </button>
        <span className="hint">
          {planMutation.isError
            ? `Failed: ${(planMutation.error as Error).message}`
            : result
              ? 'Done.'
              : 'Ready.'}
        </span>
        {result ? (
          <button type="button" className="secondary" onClick={() => setDialogOpen(true)}>
            Which algorithms ran?
          </button>
        ) : null}
      </div>

      <CompareTable
        data={comparison}
        loading={compareMutation.isPending}
        error={
          compareMutation.isError ? (compareMutation.error as Error).message : undefined
        }
        onRun={() => compareMutation.mutate()}
        disabled={busy}
      />

      <section className="map-card">
        <div className="map-bar">
          <span>
            {scenario.name} · {scenario.width} × {scenario.height} cells
          </span>
          <span>{status}</span>
        </div>
        <MapCanvas
          scenario={scenario}
          startIndex={startIndex}
          endIndex={endIndex}
          closed={closed}
          recent={recent}
          path={path}
        />
      </section>

      <footer className="foot">
        Distances are whole numbers — a straight step costs 1000, a diagonal step 1414. Nothing
        here is a recording: the C++ engine runs the search for each request, and the map replays
        the order in which it really closed its cells.
      </footer>

      {result ? (
        <AlgorithmDialog
          open={dialogOpen}
          onClose={() => setDialogOpen(false)}
          result={result}
          scenario={scenario}
          from={scenario.starts[startIndex]?.name ?? 'start'}
          to={scenario.ends[endIndex]?.name ?? 'destination'}
          obstacleCounts={obstacleCounts}
        />
      ) : null}
    </main>
  );
}
