# Project: OMP RPG Server

## Architecture

- Event bus + module system in `Server/includes/`
- Core framework: `includes/core/` (events, eventbus, module, testing)
- Game modules: `includes/modules/mod_*.inc`
- Thin bridge: `gamemodes/mygamemode.pwn` routes SA-MP callbacks to event bus
- E2E test client: `RakClient/` (Node.js + C++ SA-MP protocol client)
- RPC logger component: `Server/src/RpcLogger/` (OMP plugin for server-side packet logging)
- OMP server source (reference): `omp-src/` submodule
- Docs: `Server/docs/`

Compile: `sampctl build dev` (run from `Server/`, requires sampctl in PATH — installed at `C:\sampctl\sampctl.exe`)

Fallback (no sampctl): `./qawno/pawncc.exe gamemodes/mygamemode.pwn -ogamemodes/mygamemode.amx "-i./qawno/include" "-i./includes"`

## Resources

- OMP plugin list / releases: https://forum.open.mp/forumdisplay.php?fid=32

## Rules

### If something feels awkward, question the architecture

When writing code that fights the current architecture — workarounds, special cases, coupling between modules that should be independent, reaching into another module's internals — stop and consider whether the architecture itself needs to change. A hack today becomes a pattern tomorrow. If the event bus, module system, or data flow makes something unnecessarily hard, propose a structural fix rather than papering over it.

### Maintain docs on any architectural or workflow change

Every change to how the system works (new merge modes, new conventions, new compile flags, changes to the module template, event data slot conventions, etc.) must be reflected in the relevant doc files in `Server/docs/`. If a doc doesn't exist for the area being changed, create one. Outdated docs are worse than no docs.

Relevant docs:
- `docs/eventbus-guide.md` — event bus mechanics, data slots, merge modes, debug commands
- `docs/module-guide.md` — how to create modules, naming conventions, gotchas
- `docs/compiling.md` — build instructions
- `docs/testing-guide.md` — E2E testing with RakClient bot, test patterns, OMP quirks
- `docs/debugging-guide.md` — all feedback channels, RPC logging (client + server), debugging recipes
- `RakClient/DEVELOPMENT.md` — RakClient architecture, build, protocol, adding events/actions
