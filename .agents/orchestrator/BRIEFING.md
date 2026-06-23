# BRIEFING — 2026-06-23T08:32:00+02:00

## Mission
Build a self-contained, browser-based clone of the Apollo-8 / ACCELER8 native simulator in HTML + vanilla JS under web_sim.

## 🔒 My Identity
- Archetype: teamwork_preview_orchestrator
- Roles: orchestrator, user_liaison, human_reporter, successor
- Working directory: c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\orchestrator
- Original parent: main agent
- Original parent conversation ID: 839a0d18-7034-49e0-860b-8451426095aa

## 🔒 My Workflow
- **Pattern**: Project
- **Scope document**: c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\orchestrator\PROJECT.md
1. **Decompose**: Decompose requirements into milestones (R1-R5) and E2E testing track.
2. **Dispatch & Execute**:
   - **Delegate (sub-orchestrator)**: For large milestones.
   - **Direct (iteration loop)**: Spawn Explorer -> Worker -> Reviewer -> Challenger -> Auditor for individual milestones.
3. **On failure**:
   - Retry: nudge stuck agent or re-send task
   - Replace: spawn fresh agent with partial progress
   - Skip: proceed without (only if non-critical)
   - Redistribute: split stuck agent's remaining work
   - Redesign: re-partition decomposition
   - Escalate: report to parent (sub-orchestrators only, last resort)
4. **Succession**: Self-succeed at spawn count 16, write handoff.md, spawn successor.
- **Work items**:
  1. Initialize PLAN.md and PROJECT.md [done]
  2. Milestone 1: Exploration & Physics Extraction [done]
  3. Milestone 2: Core Physics & Automated Tests [in-progress]
  4. Milestone 3: UI Layout & Canvas Renderers [pending]
  5. Milestone 4: Integration & E2E Verification [pending]
- **Current phase**: 1
- **Current focus**: Milestone 2: Core Physics & Automated Tests

## 🔒 Key Constraints
- Operate strictly in /ponytail mode: build the minimum that works; no unrequested abstractions; mark simplifications with `// ponytail:` comment.
- Ensure all subagents spawn with /ponytail mode.
- Dispatch-only: NEVER write, modify, or create source code files directly.
- NEVER run build/test commands myself — require workers to do so.
- Forensic Auditor verdict is a BINARY VETO — violation means failure.
- Never reuse a subagent after it has delivered its handoff — always spawn fresh.

## Current Parent
- Conversation ID: 839a0d18-7034-49e0-860b-8451426095aa
- Updated: not yet

## Key Decisions Made
- Use Project Pattern with separate Implementation and E2E Testing tracks.

## Team Roster
| Agent | Type | Work Item | Status | Conv ID |
|-------|------|-----------|--------|---------|
| explorer_m1 | teamwork_preview_explorer | Milestone 1 C++ Code Exploration | completed | 73e44b58-0eb9-44de-abf3-fe474fe90540 |
| worker_m2 | teamwork_preview_worker | Milestone 2 Physics Implementation | in-progress | 61159e25-7df6-402f-9434-fc7cb2681f2b |

## Succession Status
- Succession required: yes
- Spawn count: 2 / 16
- Pending subagents: 61159e25-7df6-402f-9434-fc7cb2681f2b
- Predecessor: none
- Successor: not yet spawned

## Active Timers
- Heartbeat cron: 9fd5ff5e-b85f-4ead-a2b9-d0f857875b9d/task-13
- Safety timer: none

## Artifact Index
- c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\orchestrator\plan.md — Implementation plan
- c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\orchestrator\progress.md — Progress and liveness log
- c:\Users\thatw\Documents\Apollo-8\DashBoard\.agents\orchestrator\PROJECT.md — Scope and milestones index
