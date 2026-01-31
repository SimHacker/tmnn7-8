# 🚀 Node.js Webscale Designs

**Faction:** nodejs-webscale  
**Leader:** WebScaleChad  
**Branch:** `nodejs-webscale`

Design documents for the Node.js rewrite of TMNN.

## Philosophy

> "Move fast and `npm install` things."

The C codebase is ancient. Nobody wants to compile anything anymore. JavaScript runs everywhere. Ship it.

## Key Decisions

| Decision | Rationale |
|----------|-----------|
| TypeScript | Types are good actually |
| Express for API | It just works |
| Socket.io for realtime | Websockets made easy |
| MongoDB for storage | Schema? What schema? |
| npm install everything | Why reinvent wheels? |
| Docker deployment | Works on my machine ✅ |

## Architecture Approach

1. **Don't port, reimagine** — The C code is a reference, not a template
2. **API-first design** — Everything is an endpoint
3. **Event-driven** — Node's strength is async
4. **Microservices-ready** — Break it up later

## Tech Stack

```
┌─────────────────────────────────────┐
│           Frontend (React?)         │
├─────────────────────────────────────┤
│     Express API + Socket.io         │
├─────────────────────────────────────┤
│  MongoDB │ Redis │ Message Queue    │
└─────────────────────────────────────┘
```

## Directory Structure

```
nodejs/
├── src/
│   ├── api/           # Express routes
│   ├── services/      # Business logic
│   ├── models/        # Data models
│   └── utils/         # Helpers
├── tests/
├── package.json
├── tsconfig.json
└── Dockerfile
```

## Dependencies (Planned)

```json
{
  "express": "^4.18",
  "socket.io": "^4.6",
  "mongoose": "^7.0",
  "typescript": "^5.0",
  "jest": "^29.0",
  "eslint": "^8.0"
}
```

## Open Questions

- [ ] Frontend framework? (React vs Vue vs Svelte vs HTMX)
- [ ] ORM vs raw queries?
- [ ] Serverless deployment option?
- [ ] How webscale is too webscale?

## Documents

*Add design documents as separate files in this directory.*

| Document | Status |
|----------|--------|
| `api-design.md` | TODO |
| `realtime-architecture.md` | TODO |
| `deployment-strategy.md` | TODO |
