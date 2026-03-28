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

## Building the RpcLogger component

The RPC logger is an OMP component (DLL) for server-side packet logging. Requires VS2019 Build Tools and CMake.

```bash
# Init the OMP SDK submodule (one-time)
git submodule update --init --recursive omp-src

# Build (must be 32-bit — OMP server is x86)
cd Server/src/RpcLogger
mkdir build && cd build
cmake .. -G "Visual Studio 16 2019" -A Win32
cmake --build . --config Release

# Deploy
cp Release/RpcLogger.dll ../../components/
```

## Building the RakClient test framework

Requires Node 18 LTS and VS2019 Build Tools.

```bash
cd RakClient/app
npm install
npx node-gyp rebuild
```

Run tests: `node tests/test-debug.js`
