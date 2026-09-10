# Project Codex agents

The TOML files in `agents/` define project roles using the
[official Codex subagent format](https://learn.chatgpt.com/docs/agent-configuration/subagents?surface=app).

| Role | Model | Reasoning | Work |
| --- | --- | --- | --- |
| `rat_explorer` | `gpt-5.6-terra` | `medium` | Read-only code and contract research |
| `rat_implementer` | `gpt-5.6-sol` | `high` | Assigned implementation, verification, and commits |
| `rat_reviewer` | `gpt-6-astra` | `high` | Independent read-only review |

## Setup and use

Merge the `[agents]` values from `config.example.toml` into the local
`.codex/config.toml`. If the file is absent, copy the example to that path.
Preserve existing sections, especially machine-specific Stride MCP settings;
update an existing `[agents]` table instead of adding a duplicate. The local
config is intentionally ignored by Git. The example and role files are tracked,
and role/model configuration stays local to this project.

Codex must trust this exact project folder before it loads project configuration.
If it is not already trusted, trust `C:\5_gamedev\rat-engine` in Codex; the
equivalent scoped personal-config prerequisite is
`[projects.'c:\5_gamedev\rat-engine']` with `trust_level = "trusted"`.
Preserve other personal entries and do not trust a broader parent directory.

Open a new Codex session in this project after installation. Ask the primary
agent to use these roles by name, for example: "Use rat_explorer to trace this
code path and return the owning files." AGENTS.md also requests delegation for
useful independent research/review and the existing multi-step execution flow.
Simple tasks do not require subagents. The limit of three counts spawned threads
and excludes the primary agent; only one implementer may write at a time.

The primary agent coordinates dispatch, independent review, card statuses,
acceptance, and publication. Subagents never delegate further. The primary
model remains selected in the app; no default subagent model or reasoning is
set here, so agents without explicit role or spawn overrides inherit the parent
settings (subject to any existing user configuration).

## Permissions and verification

Explorer and reviewer files request `sandbox_mode = "read-only"`; implementers
inherit permissions. Live permission overrides from the parent session can
override role sandbox defaults, so a role file alone is not a guaranteed
filesystem boundary. Both read-only roles also explicitly prohibit file, Git,
card, editor, runtime, and external-state mutations, including through MCP.

Validate TOML and client configuration, confirm the project configuration layer
is active (a successful configuration check alone can hide an untrusted layer),
then use a new session to confirm all
three roles are discovered and run short tasks without file changes. Inspect
the actual model and reasoning settings in session evidence; a successful parse
alone does not establish role execution. Report unsupported settings, unavailable
models, or ignored roles as acceptance blockers rather than silently substituting
models. Keep GUI and remote CI claims separate from headless checks.
