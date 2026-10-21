# Current source organization

The XCLOUDFORXBOX360 root is the active repository and project. A second `XCloud360-GitHub-Source` tree is unnecessary.

- Active code: `src`, `third_party`, `tests`, and the root XDK project.
- Short context: `AI_CONTEXT.md`.
- Curated reference: `REFERENCE_BASE/AI_REFERENCE.md`.
- Selected index: `REFERENCE_BASE/SOURCE_MAP.json`.
- Refresh script: `REFERENCE_BASE/refresh_context.ps1`.
- Original references and history: private, excluded from initial reading; some still exist at their original paths when needed.
- Local build: `Release/XCloud360.xex`, ignored by Git.
- `package_github.ps1` validates publication candidates in the root without creating another checkout.

Only the curated summary, index, and index generator are allowed in the Git repository under `REFERENCE_BASE`. Do not publish reference checkouts, extractions, packages, SDKs, generated fonts, or local configuration.

Old context found in the export contained incorrect assumptions about `xbox_launcher`, DTLS, and decoding. The root context was rebuilt from inspected sources.

Full references are excluded from default reading. AI assistants should open only the module needed for a specific task.
