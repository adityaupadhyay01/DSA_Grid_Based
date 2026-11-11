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
import net from 'node:net';
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

async function post(path, body, headers = { 'content-type': 'application/json' }) {
  const res = await fetch(BASE + path, {
    method: 'POST',
    headers,
    body: typeof body === 'string' ? body : JSON.stringify(body),
  });
  const text = await res.text();
  let json;
  try {
    json = JSON.parse(text);
  } catch {
    /* not JSON */
  }
  return { status: res.status, headers: res.headers, text, json };
}

/**
 * A request fetch() will not send: wrong or missing Content-Length, a body that
 * stops early. Writes the bytes itself and reads whatever the server answers.
 */
function raw(headerLines, body = '', half_close = false) {
  return new Promise((resolve) => {
    const sock = net.connect(PORT, '127.0.0.1');
    let data = '';
    sock.setTimeout(4000, () => sock.destroy());
    sock.on('data', (d) => { data += d.toString(); });
    sock.on('close', () => {
      const status = Number(data.match(/^HTTP\/1\.1 (\d+)/)?.[1] ?? 0);
      const json = (() => { try { return JSON.parse(data.slice(data.indexOf('\r\n\r\n') + 4)); } catch { return undefined; } })();
      resolve({ status, json });
    });
    sock.on('error', () => {});
    sock.write(`${headerLines.join('\r\n')}\r\n\r\n${body}`);
    if (half_close) sock.end();
  });
}

/** Deterministic pseudo-random obstacles, so a failure reproduces. */
function randomMap(side, density, seed) {
  let s = seed >>> 0;
  const next = () => { s = (Math.imul(s, 1664525) + 1013904223) >>> 0; return s / 2 ** 32; };
  const cells = [];
  for (let i = 0; i < side * side; i += 1) cells.push(next() < density ? '1' : '0');
  cells[0] = '0';
  cells[side * side - 1] = '0';
  return cells.join('');
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
    check(Object.keys(sc.json?.cellKindLabels ?? {}).length === 6 &&
      sc.json.cellKindLabels['5'] === 'blocked zone', 'the blocked-zone cell kind is described');

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
      check(r.json.smoothPath[0] === r.json.path[0] &&
        r.json.smoothPath.at(-1) === r.json.path.at(-1) &&
        r.json.smoothPath.length <= r.json.path.length,
      `plan ${id}: the flight line keeps both ends and never adds waypoints`);
      check(r.json.smoothLength <= r.json.gridLength,
        `plan ${id}: the flight line is no longer than the grid route`,
        `${r.json.smoothLength} vs ${r.json.gridLength}`);
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

    // ---- a map the caller drew: POST /api/route ----
    const empty = await post('/api/route', {
      width: 10, height: 10, cells: '0'.repeat(100), start: 0, end: 99,
    });
    check(empty.status === 200 && empty.json.found === true, 'route on an empty drawn map succeeds');
    check(empty.json?.cost === 12_726, 'empty 10x10 corner to corner costs 9 diagonal steps',
      `got ${empty.json?.cost}`);
    check(empty.json?.scenarioId === 'custom' && empty.json?.order.length === empty.json?.expansions,
      'drawn map: scenarioId is custom and order length equals expansions');
    check(empty.json?.smoothPath.length === 2, 'drawn map: a clear diagonal is flown as one straight line');

    // A wall of blocked zone at x = 6 with a single gap at y = 9.
    const wall = Array.from({ length: 144 }, (_, i) => (i % 12 === 6 && Math.floor(i / 12) !== 9 ? '5' : '0')).join('');
    const walled = await post('/api/route', { width: 12, height: 12, cells: wall, start: 1, end: 2 * 12 + 10 });
    check(walled.status === 200 && walled.json.found === true, 'route goes around a blocked zone through the gap');
    check(walled.json.path.every((i) => wall[i] === '0'), 'the drawn-map route never enters an obstacle');
    check(walled.json.path.includes(9 * 12 + 6), 'and it passes through the only gap');

    const sealed = Array.from({ length: 144 }, (_, i) => (i % 12 === 6 ? '5' : '0')).join('');
    const none = await post('/api/route', { width: 12, height: 12, cells: sealed, start: 1, end: 2 * 12 + 10 });
    check(none.status === 200 && none.json.found === false && none.json.path.length === 0,
      'a sealed wall reports no route, as a normal 200 answer');

    // The same drawn map through five OPEN lists must give one cost.
    const side = 100;
    const big = randomMap(side, 0.22, 7);
    const costs = [];
    for (const structure of [1, 2, 3, 4, 5]) {
      const r = await post('/api/route', {
        width: side, height: side, cells: big, start: 0, end: side * side - 1, structure, reps: 1,
      });
      costs.push(r.json?.found ? r.json.cost : `error:${r.status}`);
    }
    check(new Set(costs).size === 1 && typeof costs[0] === 'number',
      'five OPEN lists agree on a 100x100 drawn map', costs.join(', '));

    const bigRoute = await post('/api/route', {
      width: side, height: side, cells: big, start: 0, end: side * side - 1,
    });
    check(bigRoute.json?.smoothLength <= bigRoute.json?.gridLength &&
      bigRoute.json.smoothPath.length < bigRoute.json.path.length,
    '100x100: smoothing shortens the route and removes waypoints');

    // ---- the drawn-map endpoint refuses what it should ----
    const cells100 = '0'.repeat(100);
    const base = { width: 10, height: 10, cells: cells100, start: 0, end: 99 };
    const refusals = [
      ['not JSON', () => post('/api/route', '{nope'), 400, 'BAD_JSON'],
      ['an array body', () => post('/api/route', '[]'), 400, 'BAD_JSON'],
      ['nested values', () => post('/api/route', '{"width":{"a":1}}'), 400, 'BAD_JSON'],
      ['wrong content type', () => post('/api/route', JSON.stringify(base), { 'content-type': 'text/plain' }), 415, 'UNSUPPORTED_MEDIA_TYPE'],
      ['no content type', () => post('/api/route', JSON.stringify(base), {}), 415, 'UNSUPPORTED_MEDIA_TYPE'],
      ['an unknown field', () => post('/api/route', { ...base, bogus: 1 }), 400, 'BAD_REQUEST'],
      ['missing width', () => post('/api/route', { ...base, width: undefined }), 400, 'BAD_REQUEST'],
      ['a fractional start', () => post('/api/route', { ...base, start: 1.5 }), 400, 'BAD_REQUEST'],
      ['cells of the wrong length', () => post('/api/route', { ...base, cells: '0'.repeat(99) }), 400, 'BAD_REQUEST'],
      ['a cell kind above 5', () => post('/api/route', { ...base, cells: `6${'0'.repeat(99)}` , start: 1}), 400, 'BAD_REQUEST'],
      ['a start inside an obstacle', () => post('/api/route', { ...base, cells: `1${'0'.repeat(99)}` }), 400, 'BAD_REQUEST'],
      ['start equal to end', () => post('/api/route', { ...base, end: 0 }), 400, 'BAD_REQUEST'],
      ['an end outside the map', () => post('/api/route', { ...base, end: 100 }), 400, 'BAD_REQUEST'],
      ['an oversized map', () => post('/api/route', { width: 300, height: 300, cells: '0'.repeat(90_000), start: 0, end: 1 }), 400, 'BAD_REQUEST'],
      ['the unsorted array on a drawn map', () => post('/api/route', { ...base, structure: 0 }), 400, 'BAD_REQUEST'],
      ['too many repetitions', () => post('/api/route', { ...base, reps: 99 }), 400, 'BAD_REQUEST'],
    ];
    for (const [what, call, status, code] of refusals) {
      const r = await call();
      check(r.status === status && r.json?.error?.code === code,
        `drawn map: ${what} -> ${status} ${code}`, `got ${r.status} ${r.json?.error?.code}`);
    }

    const getRoute = await get('/api/route');
    check(getRoute.status === 405 && getRoute.headers.get('allow') === 'POST',
      'GET /api/route -> 405 with Allow: POST');
    const postPlan = await post('/api/plan', {});
    check(postPlan.status === 405 && (postPlan.headers.get('allow') ?? '').includes('GET'),
      'POST /api/plan -> 405 with Allow: GET');

    const head = ['POST /api/route HTTP/1.1', 'Host: x', 'Content-Type: application/json', 'Connection: close'];
    const huge = await raw([...head, 'Content-Length: 10000000']);
    check(huge.status === 413, 'a 10 MB Content-Length is refused before any of it is read', `got ${huge.status}`);
    const noLen = await raw(head);
    check(noLen.status === 411, 'POST without Content-Length -> 411', `got ${noLen.status}`);
    const badLen = await raw([...head, 'Content-Length: -5']);
    check(badLen.status === 400, 'a negative Content-Length -> 400', `got ${badLen.status}`);
    const chunked = await raw([...head, 'Transfer-Encoding: chunked']);
    check(chunked.status === 400, 'a chunked body -> 400', `got ${chunked.status}`);
    const short = await raw([...head, 'Content-Length: 100'], '{"width":', true);
    check(short.status === 400, 'a body that ends before Content-Length -> 400', `got ${short.status}`);

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
