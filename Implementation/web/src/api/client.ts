import type {
  ApiErrorBody,
  CompareResponse,
  HealthResponse,
  PlanResponse,
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

async function get<T>(path: string, params?: Record<string, string | number>): Promise<T> {
  const url = new URL(path, window.location.origin);
  for (const [k, v] of Object.entries(params ?? {})) url.searchParams.set(k, String(v));

  const res = await fetch(url, { headers: { accept: 'application/json' } });
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

export const api = {
  scenarios: () => get<ScenariosResponse>('/api/scenarios'),
  plan: (p: { scenarioId: string; startIndex: number; endIndex: number; structure?: number }) =>
    get<PlanResponse>('/api/plan', {
      scenarioId: p.scenarioId,
      startIndex: p.startIndex,
      endIndex: p.endIndex,
      ...(p.structure !== undefined ? { structure: p.structure } : {}),
    }),
  compare: (p: { scenarioId: string; startIndex: number; endIndex: number }) =>
    get<CompareResponse>('/api/compare', p),
  health: () => get<HealthResponse>('/health/ready'),
};
