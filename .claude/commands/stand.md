# Stand — Move Robot to Standing Position via /control Endpoint

You control a 4-legged robot by sending HTTP GET requests to `http://sesame-robot.local/control` with parameters `leg`, `direction`, and `angle`.

## Endpoint

```
GET http://sesame-robot.local/control?leg=<leg>&direction=<dir>&angle=<0-90>
```

- `leg`: `left_front`, `right_front`, `left_back`, `right_back`
- `direction`: `fwd`, `back`, `up`, `down`
- `angle`: `0` (neutral) to `90` (max)

## Instructions

Move the robot to a standing position by sending sequential HTTP requests using `curl` via the Bash tool. Wait 0.5 seconds between each motion step.

### Sequence

1. Reset all legs to neutral (fwd 0 and up 0 for all four legs), one command at a time with 0.5s between each:
   - `left_front` fwd 0
   - `right_front` fwd 0
   - `left_back` fwd 0
   - `right_back` fwd 0
   - `left_front` up 0
   - `right_front` up 0
   - `left_back` up 0
   - `right_back` up 0

2. Move each leg down to 45° in sequence with 0.5s between each:
   - `left_front` down 45
   - `right_front` down 45
   - `left_back` down 45
   - `right_back` down 45

### Execution

Use the Bash tool to run `curl` commands sequentially. Use `-s` flag on curl to suppress output noise and `sleep 0.5` between motion steps.

Example single command:
```bash
curl -s "http://sesame-robot.local/control?leg=left_front&direction=down&angle=45" && sleep 0.5
```

Report what you're doing at each phase (e.g., "Resetting to neutral", "Moving left_front down 45°") so the user can follow along.
