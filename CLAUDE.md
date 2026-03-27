# Project: OMP RPG Server

## Architecture

- Event bus + module system in `Server/includes/`
- Core framework: `includes/core/` (events, eventbus, module, testing)
- Game modules: `includes/modules/mod_*.inc`
- Thin bridge: `gamemodes/mygamemode.pwn` routes SA-MP callbacks to event bus
- Docs: `Server/docs/`

Compile: `./qawno/pawncc.exe gamemodes/mygamemode.pwn -ogamemodes/mygamemode.amx "-i./qawno/include" "-i./includes"`

## Rules

### If something feels awkward, question the architecture

When writing code that fights the current architecture — workarounds, special cases, coupling between modules that should be independent, reaching into another module's internals — stop and consider whether the architecture itself needs to change. A hack today becomes a pattern tomorrow. If the event bus, module system, or data flow makes something unnecessarily hard, propose a structural fix rather than papering over it.

### Maintain docs on any architectural or workflow change

Every change to how the system works (new merge modes, new conventions, new compile flags, changes to the module template, event data slot conventions, etc.) must be reflected in the relevant doc files in `Server/docs/`. If a doc doesn't exist for the area being changed, create one. Outdated docs are worse than no docs.

Relevant docs:
- `docs/eventbus-guide.md` — event bus mechanics, data slots, merge modes, debug commands
- `docs/module-guide.md` — how to create modules, naming conventions, gotchas
- `docs/compiling.md` — build instructions
