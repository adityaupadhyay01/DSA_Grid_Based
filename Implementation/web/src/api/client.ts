import type {
  ApiErrorBody,
  CompareResponse,
  HealthResponse,
  PlanResponse,
  RouteRequest,
  ScenariosResponse,
} from '../types.js';

/** Thrown for anything the API refuses, carrying the id it logged it under. */
export class ApiError extends Error {
  constructor(
    readonly status: number,
    readonly code: string,
    message: string,
    readonly requestId?: string,
  ) {
    super(message);
    this.name = 'ApiError';
  }
}

async function decode<T>(res: Response): Promise<T> {
  if (!res.ok) {
    let body: ApiErrorBody | undefined;
    try {
      body = (await res.json()) as ApiErrorBody;
    } catch {
      /* a non-JSON failure is still a failure; fall through to the status */
    }
    throw new ApiError(
      res.status,
      body?.error.code ?? 'HTTP_ERROR',
      body?.error.message ?? `request failed with ${res.status}`,
      body?.error.requestId,
    );
  }
  return (await res.json()) as T;
}

async function get<T>(path: string, params?: Record<string, string | number>): Promise<T> {
  const url = new URL(path, window.location.origin);
  for (const [k, v] of Object.entries(params ?? {})) url.searchParams.set(k, String(v));
  return decode<T>(await fetch(url, { headers: { accept: 'application/json' } }));
}

async function post<T>(path: string, body: unknown): Promise<T> {
  return decode<T>(
    await fetch(path, {
      method: 'POST',
      headers: { 'content-type': 'application/json', accept: 'application/json' },
      body: JSON.stringify(body),
    }),
  );
}

export const api = {
  scenarios: () => get<ScenariosResponse>('/api/scenarios'),
  plan: (p: { scenarioId: string; startIndex: number; endIndex: number; structure?: number }) =>
    get<PlanResponse>('/api/plan', {
      scenarioId: p.scenarioId,
      startIndex: p.startIndex,
      endIndex: p.endIndex,
      ...(p.structure !== undefined ? { structure: p.structure } : {}),
    }),
  /** A map the user drew; it is too big for a query string, so it is POSTed. */
  route: (r: RouteRequest) => post<PlanResponse>('/api/route', r),
  compare: (p: { scenarioId: string; startIndex: number; endIndex: number }) =>
    get<CompareResponse>('/api/compare', p),
  health: () => get<HealthResponse>('/health/ready'),
};
