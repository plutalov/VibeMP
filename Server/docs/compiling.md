# Compiling the Gamemode

## Prerequisites

- **sampctl** (recommended): installed at `C:\sampctl\sampctl.exe`, must be in PATH
- **Fallback**: The Pawn compiler (`pawncc.exe`) is included in the `qawno/` directory

## Compile with sampctl (recommended)

From the `Server/` directory:

```bash
sampctl build dev
```

This uses the build configuration in `pawn.json` which sets up include paths and compiler flags automatically.

## Compile manually (fallback)

From the `Server/` directory:

```bash
./qawno/pawncc.exe gamemodes/mygamemode.pwn -ogamemodes/mygamemode.amx "-i./qawno/include" "-i./includes"
```

## After Compiling

Restart the server for changes to take effect:

```bash
./omp-server.exe
```
