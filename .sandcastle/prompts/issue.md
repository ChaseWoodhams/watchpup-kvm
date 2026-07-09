# Issue Worker Instructions

Work only on the assigned issue.

Inspect repository conventions before editing. Make the smallest safe change that solves the issue.

Do not create or modify secrets, `.env` files, `node_modules`, generated files, build outputs, or logs. Do not modify `agent-loop-kit` or Sandcastle.

Run:

```sh
npm run typecheck
npm run build
```

Commit your changes with a clear message. Stop when the issue is solved.
