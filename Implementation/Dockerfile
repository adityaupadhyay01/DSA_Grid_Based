# syntax=docker/dockerfile:1
# ---------------------------------------------------------------------------
# Two build stages, one runtime. Node builds the frontend and then goes away:
# the running container is a single statically-linked C++ binary and a folder
# of static files, with no JavaScript runtime in it at all.
# ---------------------------------------------------------------------------
FROM node:22-bookworm-slim AS web
WORKDIR /src/web
COPY web/package.json web/package-lock.json* ./
RUN npm ci || npm install
COPY web/ ./
RUN npm run build

FROM debian:bookworm-slim AS backend
WORKDIR /src
RUN apt-get update \
 && apt-get install -y --no-install-recommends g++ make libstdc++-12-dev \
 && rm -rf /var/lib/apt/lists/*
COPY Makefile ./
COPY engine/ engine/
COPY server/ server/
# -static-libstdc++ so the runtime image needs no C++ runtime package.
RUN make bin/server LDFLAGS="-pthread -static-libstdc++ -static-libgcc" \
 && make bin/tests \
 && ./bin/tests

FROM debian:bookworm-slim AS runtime
WORKDIR /app
RUN useradd --system --uid 10001 --no-create-home planner
COPY --from=backend /src/bin/server ./bin/server
COPY --from=web     /src/web/dist   ./web/dist

USER planner
EXPOSE 8080
ENV HOST=0.0.0.0 PORT=8080 WEB_DIST=/app/web/dist WORKERS=4

# The engine is what the service exists to expose, so readiness asks about it.
HEALTHCHECK --interval=30s --timeout=5s --start-period=5s --retries=3 \
  CMD ["/app/bin/server", "--healthcheck"]

ENTRYPOINT ["/app/bin/server"]
