# Walk — Quadruped Walking Gait via /control Endpoint

You control a 4-legged robot by sending HTTP GET requests to `http://sesame-robot.local/control` with parameters `leg`, `direction`, and `angle`.

## Endpoint

```
GET http://sesame-robot.local/control?leg=<leg>&direction=<dir>&angle=<0-90>
```

- `leg`: `left_front`, `right_front`, `left_back`, `right_back`
- `direction`: `fwd`, `back`, `up`, `down`
- `angle`: `0` (neutral) to `90` (max)

## Instructions

Execute a walking gait by sending sequential HTTP requests using `curl` via the Bash tool. Wait 0.5 seconds between each motion step.

### Walk Cycle (Creep Gait)

The leg order is: `left_front`, `right_back`, `right_front`, `left_back` (diagonal pairs alternating).

For each leg in the sequence:

1. **Lift** the leg: `direction=up`, `angle=40`
2. **Wait 0.5s**
3. **Swing forward**: `direction=fwd`, `angle=25`
4. **Wait 0.5s**
5. **Plant** the leg: `direction=up`, `angle=0`
6. **Wait 0.5s**
7. **Push body forward**: send `direction=back`, `angle=10` to ALL four legs
8. **Wait 0.5s**
9. **Reset stride**: send `direction=fwd`, `angle=0` to ALL four legs
10. **Wait 0.5s**

Then proceed to the next leg.

### Before and After

- Before walking: reset all legs to neutral (`up 0` and `fwd 0` for all four legs)
- After walking: reset all legs to neutral

### Default Parameters

- Number of cycles: 2 (or as specified by user with $ARGUMENTS)
- Step delay: 0.5 seconds
- Lift angle: 40°
- Stride angle: 25°
- Push angle: 10°

### Execution

Use the Bash tool to run `curl` commands sequentially. Chain multiple curl calls for the same step with `&&` and use `sleep 0.5` between motion steps. Use `-s` flag on curl to suppress output noise.

Example single command:
```bash
curl -s "http://sesame-robot.local/control?leg=left_front&direction=up&angle=40" && sleep 0.5
```

Report what you're doing at each phase (e.g., "Lifting left_front", "Pushing body forward") so the user can follow along.
