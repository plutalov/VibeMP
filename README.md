# VibeMP — OMP RPG Server

An open.mp RPG server built with a modular architecture designed for **vibe coding with AI agents**. The codebase includes comprehensive documentation, an E2E testing framework, and full RPC logging on both client and server — giving AI agents (and humans) rich feedback loops for building and debugging game systems.

## For Developers: How to Add Features with Claude

### Step 1: Orient the agent

Ask Claude to read the project documentation first:

> "Read all the docs in Server/docs/ and RakClient/DEVELOPMENT.md and CLAUDE.md. Understand the architecture before making changes."

Claude will learn the event bus, module system, database patterns, testing framework, and debugging tools.

### Step 2: Ask for what you want

Describe the system you want in plain language:

> "Add a fishing system. Players use /fish near water, wait a random time, catch a fish, and get money. Fish types vary by location."

> "Add a vehicle ownership system. Players can buy vehicles from a dealership, lock them, and they persist across sessions."

> "Add a faction system with police, medics, and gangs. Each faction has ranks and a shared radio channel."

### Step 3: What to expect

The agent should:

- **Create a new module** (`includes/modules/mod_fishing.inc`) following the module template
- **Subscribe to events** via the event bus — not by editing other modules' internals
- **Add database migrations** (`sql/V<next>__<description>.sql`) for persistent data
- **Add commands** using the ZCMD-style `cmd_<name>` pattern
- **Wire the module** into `mygamemode.pwn` (include, init, destroy)
- **Compile and verify** the build succeeds
- **Write E2E tests** using the RakClient bot framework

### Step 4: Expect the agent to use the feedback systems

The project has rich debugging and testing infrastructure. The agent should use it:

- **Server-side RPC logging** — The RpcLogger OMP component (`Server/src/RpcLogger/`) logs every incoming and outgoing RPC on the server. The agent can read `Server/log.txt` to see exactly what the server receives and sends.
- **Client-side RPC logging** — The RakClient logs every incoming and outgoing RPC to stdout (`[RPC-IN]` / `[RPC-OUT]`). The agent can cross-reference with server logs.
- **RakClient E2E tests** — Automated bot scripts (`RakClient/app/tests/`) that connect, perform actions, and assert on server responses. The agent should write tests for new features and run them.
- **Record-and-replay workflow** — For complex flows, connect with a real SA-MP client while RpcLogger records the session, then reconstruct the exact RPC sequence in a bot test.
- **OMP server source** — The `omp-src/` submodule contains the open.mp C++ source. The agent can read it to understand protocol details (like dialog response format, spawn sequence, etc.).
- **RakClient source** — The agent can read and extend the C++ protocol client to add missing event handlers or outbound actions when needed for testing new features.

---

## PR Requirements

Every pull request will be assessed against these criteria:

### 1. Exhaustive description

The PR must describe what was added, why, and how it integrates with existing systems. Include the module's public API, events emitted/consumed, commands added, and database schema changes.

### 2. Regression tests

New functionality must have E2E test coverage using the RakClient bot framework. The test should exercise the happy path end-to-end: connect, authenticate, perform the action, verify the server response, disconnect.

### 3. No existing tests break

All existing tests must continue to pass. Run `node tests/test-register.js` (and any other test files) before submitting.

### 4. Architecture compliance

- **No existing module rewrites.** Small backward-compatible changes to existing modules are acceptable (e.g., emitting a new event, exposing a new API function). Rewriting another module's logic is not.
- **Use the event bus.** Modules communicate through events, not by calling each other's internal functions. If you need data from another module, use its public API (`stock` functions).
- **Use the module system.** New functionality goes in a new `mod_<name>.inc` file with proper init/destroy lifecycle, static encapsulation, and event subscriptions.
- **Use ZCMD-style commands.** Define `cmd_<name>` functions. Don't modify the command processor.
- **Database changes use Flyway migrations.** Never modify existing migration files. Add new `V<next>__<description>.sql` files.

---

## Project Structure

```
Server/
  gamemodes/mygamemode.pwn    — thin bridge routing callbacks to event bus
  includes/
    core/                     — event bus, events, commands, module system
    modules/                  — game modules (mod_auth, mod_spawn, etc.)
  sql/                        — Flyway database migrations
  src/RpcLogger/              — OMP component for server-side RPC logging
  docs/                       — architecture and usage documentation

RakClient/                    — E2E testing bot (Node.js + C++ SA-MP protocol)
  app/tests/                  — test scripts
  app/lib/rak-client.js       — JS EventEmitter wrapper
  src/                        — C++ SA-MP 0.3.7 protocol implementation

omp-src/                      — open.mp server source (reference submodule)
```

## Quick Start

```bash
# 1. Start MySQL
wsl docker compose -f Server/docker-compose.yml up -d

# 2. Compile gamemode
cd Server && sampctl build dev

# 3. Start server
./omp-server.exe

# 4. Run tests
cd ../RakClient/app
npm install && npx node-gyp rebuild
node tests/test-register.js
```

## Documentation

| Document | What it covers |
|----------|---------------|
| [CLAUDE.md](CLAUDE.md) | Project architecture and rules for AI agents |
| [Module Guide](Server/docs/module-guide.md) | How to create modules, templates, gotchas |
| [Module Reference](Server/docs/modules/) | Per-module docs: API, events, dependencies, commands |
| [Event Bus Guide](Server/docs/eventbus-guide.md) | Event dispatch, data slots, merge modes |
| [Database Guide](Server/docs/database.md) | MySQL setup, migrations, async query patterns |
| [Testing Guide](Server/docs/testing-guide.md) | E2E testing with RakClient, test patterns |
| [Debugging Guide](Server/docs/debugging-guide.md) | All feedback channels, RPC logging, recipes |
| [Compiling](Server/docs/compiling.md) | Build instructions for gamemode, RpcLogger, RakClient |
| [RPC Trace Reference](Server/docs/rpc-trace-reference.md) | Annotated real-client session recording |
| [RakClient Development](RakClient/DEVELOPMENT.md) | Bot architecture, protocol, adding events |
