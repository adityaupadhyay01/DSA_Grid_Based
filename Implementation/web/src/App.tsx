import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { useMutation, useQuery } from '@tanstack/react-query';
import { unpackCells, type CompareResponse, type PlanResponse, type ScenarioDto } from './types.js';
import { api, ApiError } from './api/client.js';
import {
  CUSTOM_SIZE,
  TOOLS,
  applyTool,
  defaultCustom,
  loadCustom,
  packCells,
  paintLine,
  saveCustom,
  townLayout,
  type CustomMap,
  type Tool,
} from './lib/custom.js';
import { prefersReducedMotion } from './lib/theme.js';
import type { MapView, Replay } from './lib/render.js';
import { MapCanvas } from './components/MapCanvas.js';
import { Legend } from './components/Legend.js';
import { Editor } from './components/Editor.js';
import { StepPicker } from './components/StepPicker.js';
import { CompareTable } from './components/CompareTable.js';
import { AlgorithmDialog, type RunSummary } from './components/AlgorithmDialog.js';

const sleep = (ms: number) => new Promise<void>((r) => setTimeout(r, ms));

/** Where on a map something can stand: a name for the label, a cell for the pin. */
interface Spot {
  name: string;
  index: number;
}

/** A map, whether the server supplied it or the user drew it. */
interface MapModel {
  id: string;
  name: string;
  blurb: string;
  width: number;
  height: number;
  cells: Uint8Array;
  starts: Spot[];
  ends: Spot[];
  custom: boolean;
}

function fromScenario(s: ScenarioDto): MapModel {
  const spot = (p: { name: string; x: number; y: number }): Spot => ({
    name: p.name,
    index: p.y * s.width + p.x,
  });
  return {
    id: s.id,
    name: s.name,
    blurb: s.blurb,
    width: s.width,
    height: s.height,
    cells: unpackCells(s.cells),
    starts: s.starts.map(spot),
    ends: s.ends.map(spot),
    custom: false,
  };
}

export function App() {
  const scenariosQuery = useQuery({ queryKey: ['scenarios'], queryFn: api.scenarios });
  const healthQuery = useQuery({ queryKey: ['health'], queryFn: api.health, retry: 0 });
  const serverMaps = useMemo(
    () => (scenariosQuery.data?.scenarios ?? []).map(fromScenario),
    [scenariosQuery.data],
  );

  const [mapIx, setMapIx] = useState(0);
  const [startIx, setStartIx] = useState(0);
  const [endIx, setEndIx] = useState(0);

  // The drawn map is edited in place, a stroke at a time, so it lives in a
  // mutable holder and `revision` tells React when to look at it again.
  const [holder] = useState(() => ({ map: loadCustom() ?? defaultCustom() }));
  const [revision, setRevision] = useState(0);
  const [tool, setTool] = useState<Tool>(TOOLS[0]!);
  const [brush, setBrush] = useState(1);

  const [replay, setReplay] = useState<Replay | undefined>();
  const [path, setPath] = useState<number[] | undefined>();
  const [smoothPath, setSmoothPath] = useState<number[] | undefined>();
  const [status, setStatus] = useState('Waiting to start');
  const [hint, setHint] = useState({ text: 'Ready.', bad: false });
  const [busy, setBusy] = useState(false);
  const [lastRun, setLastRun] = useState<RunSummary | undefined>();
  const [dialogOpen, setDialogOpen] = useState(false);
  const [comparison, setComparison] = useState<CompareResponse | undefined>();

  // A replay in flight must stop when the user changes anything underneath it.
  const runToken = useRef(0);

  const customModel = useMemo<MapModel>(
    () => ({
      id: 'custom',
      name: 'Custom map',
      blurb: `draw it yourself · ${CUSTOM_SIZE} × ${CUSTOM_SIZE}`,
      width: CUSTOM_SIZE,
      height: CUSTOM_SIZE,
      cells: holder.map.cells,
      starts: [{ name: 'Start', index: holder.map.start }],
      ends: [{ name: 'Delivery', index: holder.map.end }],
      custom: true,
    }),
    // `revision` is the signal that the holder's contents changed.
    [holder, revision],
  );

  const maps = useMemo(() => [...serverMaps, customModel], [serverMaps, customModel]);
  const current = maps[Math.min(mapIx, maps.length - 1)] ?? customModel;
  const isCustom = current.custom;

  const resetRoute = useCallback(() => {
    runToken.current += 1;
    setBusy(false);
    setReplay(undefined);
    setPath(undefined);
    setSmoothPath(undefined);
    setLastRun(undefined);
    setComparison(undefined);
    setDialogOpen(false);
    setStatus('Waiting to start');
    setHint({ text: 'Ready.', bad: false });
  }, []);

  useEffect(() => {
    resetRoute();
  }, [mapIx, startIx, endIx, resetRoute]);

  // ---------------------------------------------------------------- editing
  const renderPending = useRef(false);
  const saveTimer = useRef(0);

  const touched = (changed: boolean) => {
    if (!changed) return;
    resetRoute();
    // One repaint per animation frame however many cells a stroke touched.
    if (!renderPending.current) {
      renderPending.current = true;
      requestAnimationFrame(() => {
        renderPending.current = false;
        setRevision((r) => r + 1);
      });
    }
    window.clearTimeout(saveTimer.current);
    saveTimer.current = window.setTimeout(() => saveCustom(holder.map), 400);
  };

  const replaceCustom = (next: CustomMap) => {
    holder.map = next;
    saveCustom(next);
    resetRoute();
    setRevision((r) => r + 1);
  };

  // ------------------------------------------------------------------ running
  const run = async () => {
    if (busy) return;
    const map = current;
    const from = map.starts[isCustom ? 0 : startIx];
    const to = map.ends[isCustom ? 0 : endIx];
    if (!from || !to) return;

    if (from.index === to.index) {
      setHint({ text: 'The start and the delivery point are the same cell.', bad: true });
      return;
    }

    const token = (runToken.current += 1);
    setBusy(true);
    setReplay(undefined);
    setPath(undefined);
    setSmoothPath(undefined);
    setLastRun(undefined);
    setStatus('Asking the C++ engine…');
    setHint({ text: 'Searching for the shortest route.', bad: false });

    let data: PlanResponse;
    try {
      data = map.custom
        ? await api.route({
            width: map.width,
            height: map.height,
            cells: packCells(map.cells),
            start: from.index,
            end: to.index,
          })
        : await api.plan({ scenarioId: map.id, startIndex: startIx, endIndex: endIx });
    } catch (err) {
      if (token !== runToken.current) return;
      setBusy(false);
      setStatus('The planner could not be reached');
      setHint({ text: err instanceof Error ? err.message : String(err), bad: true });
      return;
    }
    if (token !== runToken.current) return;

    // The engine returned the order in which it really closed cells. This
    // replays that array; it is not a second search running in the browser.
    // About four seconds whatever the map size, and instant for anyone who has
    // asked their system for reduced motion.
    const { order } = data;
    const duration = prefersReducedMotion() ? 0 : Math.min(4200, Math.max(1100, order.length * 5));
    const t0 = performance.now();
    await new Promise<void>((resolve) => {
      const frame = (now: number) => {
        if (runToken.current !== token) return resolve();
        const k = duration === 0 ? 1 : Math.min(1, (now - t0) / duration);
        const upto = Math.ceil(k * order.length);
        setReplay({ order, upto, front: k < 1 });
        setStatus(`A* is checking cells… ${upto.toLocaleString()} checked`);
        if (k < 1) requestAnimationFrame(frame);
        else resolve();
      };
      requestAnimationFrame(frame);
    });
    if (token !== runToken.current) return;

    const counts = new Map<number, number>();
    for (const k of map.cells) if (k !== 0) counts.set(k, (counts.get(k) ?? 0) + 1);

    setPath(data.found ? data.path : undefined);
    setSmoothPath(data.found ? data.smoothPath : undefined);
    setLastRun({
      result: data,
      from: from.name,
      to: to.name,
      mapName: map.name,
      width: map.width,
      height: map.height,
      counts: [...counts.entries()].sort((a, b) => a[0] - b[0]),
    });
    setStatus(
      data.found
        ? `Route found · length ${data.cost.toLocaleString()} · ${data.expansions.toLocaleString()} cells checked`
        : 'No route exists between these two points',
    );
    setHint({ text: data.found ? 'Done.' : 'Try another pair of points.', bad: false });
    setBusy(false);

    await sleep(380);
    if (token === runToken.current) setDialogOpen(true);
  };

  const compareMutation = useMutation({
    mutationFn: async () =>
      api.compare({ scenarioId: current.id, startIndex: startIx, endIndex: endIx }),
    onSuccess: setComparison,
  });

  // -------------------------------------------------------------- derived view
  const present = useMemo(() => {
    const kinds = new Set<number>();
    for (const k of current.cells) if (k !== 0) kinds.add(k);
    return [...kinds].sort((a, b) => a - b);
    // `revision` covers edits made in place to the custom map's cells.
  }, [current, revision]);

  if (scenariosQuery.isPending) {
    return (
      <main className="shell">
        <p className="loading">Loading the maps from the engine…</p>
      </main>
    );
  }

  if (scenariosQuery.isError) {
    const err = scenariosQuery.error;
    return (
      <main className="shell">
        <h1>Drone Path Planning</h1>
        <p className="verdict bad">
          Could not reach the API.{' '}
          {err instanceof ApiError ? `${err.code}: ${err.message}` : String(err)}
          <br />
          Start the backend with <code>make serve</code>, or <code>./bin/server</code> while
          developing with <code>npm run dev</code>.
        </p>
      </main>
    );
  }

  const start = current.starts[isCustom ? 0 : startIx] ?? current.starts[0]!;
  const end = current.ends[isCustom ? 0 : endIx] ?? current.ends[0]!;
  const view: MapView = {
    width: current.width,
    height: current.height,
    cells: current.cells,
    start: { index: start.index, label: start.name },
    end: { index: end.index, label: end.name },
    path,
    smoothPath,
  };

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
          {healthQuery.data?.engine.loaded ? 'C++ engine connected' : 'engine status unknown'}
        </span>
      </header>

      <p className="intro">
        Pick a <b>map</b>, pick where the drone <b>starts</b>, pick where it must <b>deliver</b> —
        or draw your own map and place the two points yourself. The drone cannot fly through
        buildings, trees, water, racks or blocked zones. The C++ engine finds the shortest safe
        route, and then this page shows exactly which algorithms and data structures it used.
      </p>

      <StepPicker
        step={1}
        title="Choose a map"
        options={maps.map((m) => ({ name: m.name, blurb: m.blurb }))}
        selected={maps.indexOf(current)}
        disabled={busy}
        onSelect={(i) => {
          setMapIx(i);
          setStartIx(0);
          setEndIx(0);
        }}
      />

      {isCustom ? (
        <Editor
          tool={tool}
          onTool={setTool}
          brush={brush}
          onBrush={setBrush}
          disabled={busy}
          onClear={() => replaceCustom(defaultCustom())}
          onTown={() => replaceCustom(townLayout())}
        />
      ) : (
        <>
          <StepPicker
            step={2}
            title="Choose the starting point"
            options={current.starts.map((p) => ({ name: p.name }))}
            selected={startIx}
            disabled={busy}
            onSelect={setStartIx}
          />
          <StepPicker
            step={3}
            title="Choose the delivery point"
            options={current.ends.map((p) => ({ name: p.name }))}
            selected={endIx}
            disabled={busy}
            onSelect={setEndIx}
          />
        </>
      )}

      <div className="go-row">
        <button type="button" className="primary" disabled={busy} onClick={() => void run()}>
          {busy ? 'Planning…' : 'Find the shortest route'}
        </button>
        <span className={hint.bad ? 'hint bad' : 'hint'} role="status">
          {hint.text}
        </span>
        {lastRun ? (
          <button type="button" className="secondary" onClick={() => setDialogOpen(true)}>
            Which algorithms ran?
          </button>
        ) : null}
      </div>

      <section className="map-card">
        <div className="map-bar">
          <span>
            {current.name} · {current.width} × {current.height} cells
          </span>
          <span>{status}</span>
        </div>
        <MapCanvas
          view={view}
          replay={replay}
          editable={isCustom && !busy}
          label={`${current.name}: obstacles, the drone's starting point, the delivery point and the route`}
          onPaintStart={(cell) => touched(applyTool(holder.map, tool, brush, cell))}
          onPaintMove={(from, to) =>
            touched(
              'point' in tool
                ? applyTool(holder.map, tool, brush, to)
                : paintLine(holder.map, tool, brush, from, to),
            )
          }
          onPaintEnd={() => saveCustom(holder.map)}
        />
        <Legend kinds={present} showFlightLine={Boolean(smoothPath && smoothPath.length > 1)} />
      </section>

      {isCustom ? null : (
        <details className="bench">
          <summary>Engine benchmark — run this route with all six OPEN lists</summary>
          <CompareTable
            data={comparison}
            loading={compareMutation.isPending}
            error={compareMutation.isError ? (compareMutation.error as Error).message : undefined}
            onRun={() => compareMutation.mutate()}
            disabled={busy}
          />
        </details>
      )}

      <footer className="foot">
        Distances are whole numbers — a straight step costs 1000, a diagonal step 1414. Nothing
        here is a recording: the C++ engine runs the search for each request, and the map replays
        the order in which it really closed its cells.
      </footer>

      {lastRun ? (
        <AlgorithmDialog open={dialogOpen} onClose={() => setDialogOpen(false)} run={lastRun} />
      ) : null}
    </main>
  );
}
