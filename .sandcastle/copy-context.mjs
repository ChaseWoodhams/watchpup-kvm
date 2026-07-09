const [, , source, worktree] = process.argv;

console.log("No extra context copied yet.");
console.log(`Source: ${source ?? "(not provided)"}`);
console.log(`Worktree: ${worktree ?? "(not provided)"}`);
