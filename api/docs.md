# SnackYou Turret API

Controls a networked snack-launching turret: yaw/pitch positioning, a lock-based ownership model, a fire trigger, and a polling interface for an ESP32 controller. State lives in memory on the Go (Gin) backend.

**Base URL:** `http://<server>:8080/api`

## Endpoints

| Method | Path | Description |
|--------|------|-------------|
| POST | `/api/lock` | Acquire or extend the control lock |
| DELETE | `/api/lock` | Release the control lock |
| GET | `/api/lock` | Get current lock status |
| GET | `/api/state` | Get turret state and lock info |
| PUT | `/api/state` | Update yaw/pitch (requires lock) |
| POST | `/api/fire` | Trigger fire (requires lock) |
| GET | `/api/camera/stream` | Browser MJPEG camera stream relay |
| GET | `/api/esp/state` | ESP32 polls for current commands |
| POST | `/api/esp/ack` | ESP32 acknowledges fire execution |
| POST | `/api/esp/camera` | ESP32 uploads latest JPEG camera frame |

## Core concepts

**Lock system.** Only one user controls the turret at a time. A lock must be acquired before moving the turret or firing. It expires automatically after 30 seconds unless refreshed.

**State.**
```json
{
  "yaw": 0-180,
  "pitch": 0-90,
  "fire": false,
  "version": int
}
```

**User identity.** Every request that modifies state includes a `user` string, which is checked against the current lock owner.

## Lock endpoints

### POST /api/lock

Acquires or extends control of the turret.

Request:
```json
{ "user": "ritam" }
```

Behavior:
- No existing lock → grants it
- Requesting user already owns it → extends by 30s
- A different user owns it → 409 Conflict

Response (success):
```json
{ "success": true, "expires": "2026-07-04T02:40:00Z" }
```

Response (conflict):
```json
{ "success": false, "owner": "other_user" }
```

### DELETE /api/lock

Releases the lock. Only the current owner can release it.

Request (query param or body):
```json
{ "user": "ritam" }
```

Behavior: if the requester owns the lock, it's cleared and ownership/expiry reset.

Response:
```json
{ "released": true }
```

### GET /api/lock

Returns current lock ownership.

Response:
```json
{ "owner": "ritam", "expires": "2026-07-04T02:40:00Z" }
```

## Turret state endpoints

### GET /api/state

Returns full turret state plus lock info.

Response:
```json
{
  "turret": {
    "yaw": 90,
    "pitch": 20,
    "fire": false,
    "version": 12
  },
  "lock": {
    "owner": "ritam",
    "expires": "2026-07-04T02:40:00Z"
  }
}
```

### PUT /api/state

Updates turret orientation. Caller must own the lock, or the request returns 403 Forbidden.

Request:
```json
{ "user": "ritam", "yaw": 120, "pitch": 30 }
```

Response:
```json
{ "yaw": 120, "pitch": 30, "fire": false, "version": 13 }
```

## Fire endpoint

### POST /api/fire

Triggers a snack launch. Caller must own the lock. Sets `fire = true`; the ESP resets it after acknowledging.

Request:
```json
{ "user": "ritam" }
```

Response:
```json
{ "ok": true, "version": 14 }
```

## ESP32 endpoints

### POST /api/esp/camera

Uploads one JPEG camera frame from the ESP32 which the hosted web app reads through `/api/camera/stream`

Request:
```http
Content-Type: image/jpeg

<jpeg bytes>
```

Response:
```json
{ "ok": true, "seq": 42 }
```

## Camera endpoints

### GET /api/camera/stream

Streams the most recently uploaded ESP32 JPEG frames as an MJPEG response:

```http
Content-Type: multipart/x-mixed-replace;boundary=123456789000000000000987654321
```

This endpoint is intended for the browser app's `<img>` tag.

### GET /api/esp/state

Polled by the ESP32 to read the desired state. The ESP compares `version` against its last-seen value to detect changes, then executes movement or firing.

Response:
```json
{ "yaw": 120, "pitch": 30, "fire": false, "version": 14 }
```

### POST /api/esp/ack

Acknowledges that a fire command executed.

Request:
```json
{ "version": 14 }
```

Behavior: if `version` matches the current state, `fire` resets to `false`.

Response:
```json
{ "ok": true }
```

## System rules

- Only the lock owner can move or fire the turret.
- `version` increments on every state change or fire, which prevents duplicate execution on the ESP side.
- `fire` is a transient flag and must be acknowledged by the ESP before it resets.

## Example session

1. `POST /api/lock` — acquire the lock
2. `PUT /api/state` — move the turret
3. `POST /api/fire` — fire
4. `POST /api/esp/ack` — ESP acknowledges execution
5. `DELETE /api/lock` — release the lock
