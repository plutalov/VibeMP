# Compiling the Gamemode

## Prerequisites

- **sampctl** (recommended): installed at `C:\sampctl\sampctl.exe`, must be in PATH
- **Fallback**: The Pawn compiler (`pawncc.exe`) is included in the `qawno/` directory
- **Docker** (for MySQL): Docker Desktop with WSL2 backend
- **Plugins**: MySQL and bcrypt plugins (installed via `sampctl install`)

## First-time setup

```bash
cd Server/

# Install plugin dependencies (downloads DLLs + includes)
sampctl install pBlueG/SA-MP-MySQL
sampctl install Sreyas-Sreelal/samp-bcrypt

# Start MySQL (via WSL)
wsl docker compose up -d mysql
wsl docker compose up flyway     # run migrations
```

## Compile with sampctl (recommended)

From the `Server/` directory:

```bash
sampctl build dev
```

This uses the build configuration in `pawn.json` which sets up include paths and compiler flags automatically.

## Compile manually (fallback)

From the `Server/` directory:

```bash
./qawno/pawncc.exe gamemodes/mygamemode.pwn -ogamemodes/mygamemode.amx "-i./qawno/include" "-i./includes" "-i./dependencies/samp-bcrypt/include" "-i./dependencies/SA-MP-MySQL"
```

## After Compiling

Make sure MySQL is running, then start the server:

```bash
# Start MySQL if not running:
wsl docker compose up -d mysql

# Start server:
./omp-server.exe
```

## Plugin Configuration

The `config.json` must have the plugins listed:

```json
"legacy_plugins": ["mysql", "samp_bcrypt"]
```

Plugin DLLs must be in `Server/plugins/`:
- `mysql.dll` — BlueG's MySQL R41
- `samp_bcrypt.dll` — samp-bcrypt v2.2.5
