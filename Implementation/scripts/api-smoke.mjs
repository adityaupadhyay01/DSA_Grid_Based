#!/usr/bin/env node
/**
 * End-to-end check of the C++ backend.
 *
 * Starts ./bin/server on a spare port, exercises every endpoint, and stops it.
 * Node is already needed to build the frontend, so using it here adds no new
 * dependency to the project — and the C++ test suite still runs on its own
 * with `make test`, which is the one that must never need anything installed.
 */
import { spawn } from 'node:child_process';
import { existsSync } from 'node:fs';
import { setTimeout as sleep } from 'node:timers/promises';

// A fixed port invites a stale server from an earlier run to answer instead of
// the one this test started — which looks like a failing feature rather than a
// port clash. Pick a random high port, and refuse to proceed unless the child
// itself is the thing listening.
const PORT = Number(process.env.SMOKE_PORT ?? 20000 + Math.floor(Math.random() * 20000));
const BASE = `http://127.0.0.1:${PORT}`;
const WEB = existsSync('web/dist/index.html') ? ['--web', 'web/dist'] : [];

let failures = 0;
const results = [];

function check(ok, what, detail = '') {
  results.push(`  [${ok ? 'PASS' : 'FAIL'}] ${what}${detail ? ` — ${detail}` : ''}`);
  if (!ok) failures += 1;
}

async function get(path) {
  const res = await fetch(BASE + path);
  const text = await res.text();
  let json;
  try {
    json = JSON.parse(text);
  } catch {
    /* static files are not JSON, which is fine */
  }
  return { status: res.status, headers: res.headers, text, json };
}

/** Costs cross-checked against an independent implementation of the same algorithm. */
const KNOWN = { city: 61_006, warehouse: 65_968, river: 55_452 };

async function main() {
  if (!existsSync('bin/server')) {
    console.error('bin/server is missing — run `make` first');
    process.exit(1);
  }

  const server = spawn('./bin/server', ['--port', String(PORT), ...WEB], {
    stdio: ['ignore', 'pipe', 'pipe'],
  });

  let stdout = '';
  let stderr = '';
  server.stdout.on('data', (d) => { stdout += d.toString(); });
  server.stderr.on('data', (d) => { stderr += d.toString(); });

  let exited = false;
  server.on('exit', (code) => { exited = true; if (code) stderr += `\nexited with ${code}`; });

  // Wait for this server to announce its own port, not merely for something to
  // answer on it.
  const banner = `http://127.0.0.1:${PORT}/`;
  let up = false;
  for (let i = 0; i < 100 && !up && !exited; i += 1) {
    await sleep(50);
    up = stdout.includes(banner);
  }
  if (!up) {
    server.kill('SIGKILL');
    console.error(`the server this test started never listened on ${PORT}`);
    if (stderr.trim()) console.error(stderr.trim());
    process.exit(1);
  }

  try {
    // ---- scenarios ----
    const sc = await get('/api/scenarios');
    check(sc.status === 200, 'GET /api/scenarios returns 200');
    check(sc.json?.scenarios?.length === 3, 'three maps are served');
    check(sc.json?.structures?.length === 6, 'six OPEN lists are listed');

    for (const s of sc.json.scenarios) {
      check(s.cells.length === s.width * s.height, `${s.id}: one cell per grid square`);
      const buried = [...s.starts, ...s.ends].filter(
        (p) => s.cells[p.y * s.width + p.x] !== '0',
      );
      check(buried.length === 0, `${s.id}: no start or delivery point inside an obstacle`);
    }

    // ---- plan ----
    for (const [id, cost] of Object.entries(KNOWN)) {
      const r = await get(`/api/plan?scenarioId=${id}&startIndex=0&endIndex=0`);
      check(r.status === 200 && r.json.found === true, `plan ${id} succeeds`);
      check(r.json.cost === cost, `plan ${id} returns the known optimal cost`,
        `got ${r.json.cost}, expected ${cost}`);
      // The browser replays `order`; a mismatch would silently truncate the animation.
      check(r.json.order.length === r.json.expansions,
        `plan ${id}: order length equals expansions`);
      check(r.json.path.length > 1 && r.json.ms > 0, `plan ${id}: path and timing present`);
    }

    // ---- every pair on every map ----
    let unreachable = 0;
    for (const s of sc.json.scenarios) {
      for (let i = 0; i < s.starts.length; i += 1) {
        for (let j = 0; j < s.ends.length; j += 1) {
          const r = await get(`/api/plan?scenarioId=${s.id}&startIndex=${i}&endIndex=${j}&reps=1`);
          if (r.json?.found !== true) unreachable += 1;
        }
      }
    }
    check(unreachable === 0, 'all 27 start/end pairs are reachable');

    // ---- compare ----
    const cmp = await get('/api/compare?scenarioId=warehouse&startIndex=0&endIndex=0&reps=1');
    check(cmp.status === 200 && cmp.json.results.length === 6, 'compare runs all six structures');
    check(cmp.json.allAgreeOnCost === true, 'all six agree on the route cost');
    check(new Set(cmp.json.results.map((r) => r.cost)).size === 1,
      'the six reported costs are identical');

    // ---- validation ----
    const bad = [
      ['/api/plan?scenarioId=nope', 404, 'NOT_FOUND'],
      ['/api/plan?scenarioId=city&startIndex=9', 400, 'BAD_REQUEST'],
      ['/api/plan?scenarioId=city&structure=42', 400, 'BAD_REQUEST'],
      ['/api/plan?scenarioId=city&reps=abc', 400, 'BAD_REQUEST'],
      ['/api/plan?scenarioId=city&reps=9999', 400, 'BAD_REQUEST'],
      ['/api/plan', 400, 'BAD_REQUEST'],
      ['/api/nope', 404, 'NOT_FOUND'],
    ];
    for (const [path, status, code] of bad) {
      const r = await get(path);
      check(r.status === status && r.json?.error?.code === code,
        `${path} -> ${status} ${code}`, `got ${r.status} ${r.json?.error?.code}`);
      check(typeof r.json?.error?.requestId === 'string' || status === 404,
        `${path} carries a request id`);
    }

    // ---- static files and the traversal guard ----
    if (WEB.length) {
      const index = await get('/');
      check(index.status === 200 && index.text.includes('<div id="root">'),
        'the built frontend is served at /');

      const asset = index.text.match(/\/assets\/[^"]+\.js/)?.[0];
      const js = asset ? await get(asset) : { status: 0, headers: new Headers() };
      check(js.status === 200, 'hashed assets are served');
      check((js.headers.get('content-type') ?? '').includes('javascript'),
        'assets are served with the right content type');

      // A static server that can be talked out of its own directory is the
      // classic way a demo leaks source. Each of these must fall through to
      // the SPA page, never to the file named.
      for (const attack of [
        '/../Makefile',
        '/assets/../../Makefile',
        '/%2e%2e/%2e%2e/etc/passwd',
        '/../engine/results.csv',
      ]) {
        const r = await get(attack);
        const leaked = r.text.includes('CXXFLAGS') || r.text.includes('root:') ||
          r.text.includes('structure,grid,density');
        check(!leaked, `path traversal refused: ${attack}`);
      }

      const spa = await get('/some/client/route');
      check(spa.status === 200 && spa.text.includes('<div id="root">'),
        'unknown paths fall back to the app, so client routing works');
    }

    // ---- health ----
    const ready = await get('/health/ready');
    check(ready.status === 200 && ready.json.engine.loaded === true,
      '/health/ready reports the engine as loaded');
    check(ready.json.engine.scenarios === 3 && ready.json.engine.structures === 6,
      '/health/ready counts maps and structures');
  } finally {
    server.kill('SIGTERM');
    await sleep(250);
    if (!server.killed) server.kill('SIGKILL');
  }

  console.log(results.join('\n'));
  console.log(
    failures === 0
      ? `\nALL API CHECKS PASSED (${results.length} checks)`
      : `\nAPI CHECKS FAILED (${failures} of ${results.length})`,
  );
  process.exit(failures === 0 ? 0 : 1);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
