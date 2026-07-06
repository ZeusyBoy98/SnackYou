# Sn\*ck You!

Do you ever feel lazy to get yourself a snack?

Well, worry no more! Sn\*ck You! lets you aim + launch a snack at yourself.

Or just launch it at your friends, lol.

**Live app:** [snackyou.zeusyboy.com](https://snackyou.zeusyboy.com/App)

## Architecture

SnackYou has three parts:

| Part         | Tech                                         | Role                                       |
| ------------ | -------------------------------------------- | ------------------------------------------ |
| **App**      | Static HTML/CSS/JS (`App/`)                  | Control panel + live camera feed           |
| **API**      | Go + Gin (`api/`)                            | Turret state, lock ownership, camera relay |
| **Firmware** | ESP-IDF on XIAO ESP32-S3 Sense (`firmware/`) | Camera capture + WiFi upload               |

The hosted app at `https://snackyou.zeusyboy.com` talks to the API at `https://snackapi.zeusyboy.com`. The ESP32 pushes JPEG frames to the API, and the app reads them back over HTTPS as an MJPEG stream.

## Prerequisites

- [Nix](https://nixos.org/download/) with flakes enabled
- [direnv](https://direnv.net/) (optional, recommended)
- For firmware flashing: USB cable + serial port access to the XIAO ESP32-S3 Sense

## Development environment

All project tooling lives in the root Nix flake (Go, ESP-IDF, Docker, etc.).

```bash
# One-off shell
nix develop

# Or, with direnv (auto-activates when you cd into the repo)
direnv allow
```

## Web app

The app is a static site in `App/`. No build step.

### Run locally

```bash
python3 -m http.server 8080 --directory App
```

Open [http://localhost:8080](http://localhost:8080). By default the app points at the production API (`https://snackapi.zeusyboy.com/api`). To test against a local API, change `BASEURL` in `App/api.js`.

The camera feed auto-connects to `https://snackapi.zeusyboy.com/api/camera/stream`. The ESP32 must be running and uploading frames for the feed to show anything.

### Deploy

The app is served via GitHub Pages. The `CNAME` file points the site at `snackyou.zeusyboy.com`.

Push changes on `main` and GitHub Pages will serve the `App/` folder. No separate build or deploy command is needed beyond keeping `CNAME` and the static files in the repo.

## API

Go backend in `api/`. State is in-memory (single instance). See [api/docs.md](api/docs.md) for the full endpoint reference.

### Run locally

```bash
cd api
go run .
```

The server listens on `:8080` by default. Health check:

```bash
curl http://localhost:8080/api/health
```

Hot reload during development:

```bash
cd api
air --build.cmd "go build -o bin/api main.go" --build.bin "./bin/api"
```

### Deploy with Docker

```bash
cd api
docker build -f DOCKERFILE -t snackyou-api .
docker run --rm -p 8080:8080 snackyou-api
```

Production runs at `https://snackapi.zeusyboy.com`. Deploy however you host that service (the repo includes the Dockerfile; the live deployment is outside this repo).

### Camera relay endpoints

| Endpoint                 | Purpose                                       |
| ------------------------ | --------------------------------------------- |
| `POST /api/esp/camera`   | ESP32 uploads JPEG frames here                |
| `GET /api/camera/stream` | Browser MJPEG stream (used by the hosted app) |

## Firmware

ESP-IDF project for the **Seeed Studio XIAO ESP32-S3 Sense** with OV2640 camera module.

### Configure

Set WiFi credentials and camera upload settings before building:

```bash
cd firmware
idf.py menuconfig
```

Under **SnackYou Configuration**:

- `WiFi SSID` / `WiFi Password` — your network
- `Upload camera frames to SnackYou API` — leave enabled for the hosted app
- `Camera frame upload URL` — defaults to `https://snackapi.zeusyboy.com/api/esp/camera`

### Build

From the repo root (with the Nix dev shell active):

```bash
cd firmware
idf.py build
```

### Flash

Connect the board over USB, then:

```bash
cd firmware
idf.py -p /dev/ttyACM0 flash monitor
```

Replace `/dev/ttyACM0` with your serial port (`/dev/ttyUSB0`, `COM3`, etc.).

On success the serial log prints the device IP. The firmware also:

- serves a local MJPEG stream at `http://<device-ip>/stream` (LAN debugging)
- uploads frames to the hosted API for the web app

### Troubleshooting

- **Camera init fails** — confirm the Sense camera module is attached and PSRAM is enabled (defaults in `sdkconfig.defaults` handle this).
- **WiFi won't connect** — double-check SSID/password in menuconfig.
- **No feed in the hosted app** — the ESP32 must reach the internet and successfully POST to `/api/esp/camera`. Check serial logs for upload errors.
- **Build too large** — the custom partition table in `firmware/partitions.csv` allocates a 3 MB app slot for the HTTPS uploader.

## Quick start checklist

1. `nix develop` (or `direnv allow`)
2. `cd api && go run .` — start the API locally, or use production
3. `python3 -m http.server 8080 --directory App` — open the control panel
4. Configure WiFi in `firmware` menuconfig, then `idf.py build flash`
5. Open the app — camera feed should appear once the ESP32 is uploading
