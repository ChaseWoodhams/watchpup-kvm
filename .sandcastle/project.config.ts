import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, resolve } from "node:path";

const configDir = dirname(fileURLToPath(import.meta.url));

const promptPaths = {
  issue: ".sandcastle/prompts/issue.md",
  review: ".sandcastle/prompts/review.md",
  feedback: ".sandcastle/prompts/feedback.md"
};

const readPrompt = (relativePath: string): string =>
  readFileSync(resolve(configDir, "..", relativePath), "utf8").trim();

const config = {
  repoRoot: "C:\\code\\watchpup-kvm",
  worktreeRoot: "C:\\agent-workspaces\\watchpup-kvm",
  repoFullName: "ChaseWoodhams/watchpup-kvm",
  labels: {
    readyForAgent: "ready-for-agent",
    needsAdvisor: "needs-advisor",
    advisorReviewed: "advisor-reviewed",
    advisorHumanAttention: "advisor-human-attention",
    needsReview: "needs-review",
    approved: "ready-for-human-merge",
    changesRequested: "claude-changes-requested",
    reviewBlocked: "claude-review-blocked",
    needsHumanLoopReview: "needs-human-loop-review"
  },
  checks: [
    "npm run typecheck",
    "npm run build"
  ],
  maxReworkAttempts: 3,
  sandcastle: {
    prompts: promptPaths,
    issuePrompt: (input: {
      readonly issueNumber: number;
      readonly issueTitle: string;
      readonly issueBody: string;
    }) => `${readPrompt(promptPaths.issue)}

Assigned issue #${input.issueNumber}: ${input.issueTitle}

${input.issueBody}`,
    reviewPrompt: (input: {
      readonly pullNumber: number;
      readonly pullTitle: string;
      readonly baseBranch: string;
      readonly headBranch: string;
    }) => `${readPrompt(promptPaths.review)}

Pull request #${input.pullNumber}: ${input.pullTitle}
Base branch: ${input.baseBranch}
Head branch: ${input.headBranch}`,
    feedbackPrompt: (input: {
      readonly pullNumber: number;
      readonly pullTitle: string;
      readonly gateComment: string;
      readonly attempt: number;
    }) => `${readPrompt(promptPaths.feedback)}

Pull request #${input.pullNumber}: ${input.pullTitle}
Attempt: ${input.attempt}

Blocking gate feedback:

${input.gateComment}`
  }
};

export default config;
