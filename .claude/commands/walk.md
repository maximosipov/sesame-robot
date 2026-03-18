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

The robot starts from a **standing position** (all legs at `down 45`). Use `/stand` first if the robot is not already standing.

### Walk Cycle (Diagonal Gait)

Each cycle moves two diagonal pairs in sequence: first `left_front` + `right_back`, then `right_front` + `left_back`.

For each diagonal pair (leg A, then leg B):

1. **Lift leg A**: `direction=down`, `angle=0` (raise from standing)
2. **Wait 0.5s**
3. **Swing leg A forward**: `direction=fwd`, `angle=25`
4. **Wait 0.5s**
5. **Plant leg A**: `direction=down`, `angle=45` (return to standing height)
6. **Wait 0.5s**
7. **Lift leg B**: `direction=down`, `angle=0`
8. **Wait 0.5s**
9. **Swing leg B forward**: `direction=fwd`, `angle=25`
10. **Wait 0.5s**
11. **Plant leg B**: `direction=down`, `angle=45`
12. **Wait 0.5s**
13. **Push body forward**: send `direction=back`, `angle=10` to ALL four legs simultaneously
14. **Wait 0.5s**
15. **Reset stride for each leg** (one at a time, with 0.5s between each):
    - Lift: `direction=down`, `angle=0`
    - Reset: `direction=fwd`, `angle=0`
    - Plant: `direction=down`, `angle=45`

Then proceed to the next diagonal pair.

### Before and After

- Before walking: ensure robot is in standing position (all legs `down 45`, `fwd 0`)
- After walking: return to standing position (all legs `down 45`, `fwd 0`)

### Default Parameters

- Number of cycles: 2 (or as specified by user with $ARGUMENTS)
- Step delay: 0.5 seconds
- Standing angle: 45° (down)
- Stride angle: 25°
- Push angle: 10°

### Execution

Use the Bash tool to run **one `curl` command per Bash call**, followed by a separate `sleep 0.5` Bash call. Do NOT chain commands with `&&` or `;`. Use `-s` flag on curl to suppress output noise.

Example — each of these is a separate Bash tool call:
```bash
curl -s "http://sesame-robot.local/control?leg=left_front&direction=down&angle=0"
```
```bash
sleep 0.5
```

Report what you're doing at each phase (e.g., "Lifting left_front", "Pushing body forward") so the user can follow along.
