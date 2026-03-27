# Compiling the Gamemode

## Prerequisites

The Pawn compiler (`pawncc.exe`) is included in the `qawno/` directory. No additional installation is needed.

## Compile Command

From the `Server/` directory, run:

```bash
./qawno/pawncc.exe gamemodes/mygamemode.pwn -ogamemodes/mygamemode.amx "-i./qawno/include"
```

This compiles `mygamemode.pwn` into `mygamemode.amx` in the `gamemodes/` folder, using the includes from `qawno/include/`.

## After Compiling

Restart the server for changes to take effect:

```bash
./omp-server.exe
```
