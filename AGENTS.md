# Working on XCloud360

- The repository root is the only active project. Start with AI_CONTEXT.md.
- Read REFERENCE_BASE/AI_REFERENCE.md only when references are relevant.
- Use REFERENCE_BASE/SOURCE_MAP.json to locate a specific module; do not concatenate original checkouts, binary packages or build directories into prompts.
- Original references remain private and may be absent from a public checkout. Read them only when needed for the task.
- Read docs/XEX_GAPS.md for streaming blockers and docs/VALIDATION_CURRENT.md for current verification.
- Build with build_release.ps1; run tests/run_host_tests.ps1 for host regressions.
- Use the XDK-compatible C++ subset. Respect protocol-specific byte order.
- Do not describe declarations, simulated tests or successful compilation as working xCloud playback.
- After source changes, refresh the index with REFERENCE_BASE/refresh_context.ps1 and update the semantic summary if behavior changed.
