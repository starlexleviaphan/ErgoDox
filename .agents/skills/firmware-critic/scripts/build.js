const { execSync, spawnSync } = require('child_process');
const path = require('path');
const fs = require('fs');

const repoRoot = path.resolve(__dirname, '../../../../');
const zmkConfigDir = path.join(repoRoot, 'zmk-config');
const firmwareDir = path.join(repoRoot, 'firmware');
const targetRepo = 'starlexleviaphan/zmk-config-ergodox';

function run(cmd, cwd = repoRoot) {
  console.log(`> ${cmd}`);
  return execSync(cmd, { cwd, stdio: 'inherit', encoding: 'utf8' });
}

function runCapture(cmd, cwd = repoRoot) {
  return execSync(cmd, { cwd, encoding: 'utf8' }).trim();
}

console.log('=====================================================');
console.log('🚀 ZMK FIRMWARE BUILD & DEPLOY PIPELINE');
console.log('=====================================================');

// 1. Audit check
console.log('\n[1/5] Running firmware validation audit...');
run(`node ${path.join(__dirname, 'audit.js')}`);

// 2. Commit and push zmk-config if dirty
console.log('\n[2/5] Checking zmk-config git status...');
const status = runCapture('git status --porcelain', zmkConfigDir);
let commitMsg = 'fix: update keymap type_shift and thumb matrix transform';

if (status) {
  console.log('Changes detected in zmk-config:');
  console.log(status);
  console.log('Staging and committing in zmk-config...');
  run('git add -A', zmkConfigDir);
  run(`git commit -m "${commitMsg}"`, zmkConfigDir);
  console.log('Pushing to GitHub (zmk-config-ergodox)...');
  run('git push origin main', zmkConfigDir);
} else {
  console.log('zmk-config working tree clean.');
}

// 3. Wait for GitHub Actions run to start
console.log('\n[3/5] Waiting for GitHub Actions workflow to start...');
let runId = null;
for (let attempt = 1; attempt <= 15; attempt++) {
  try {
    const listOutput = runCapture(`gh run list --repo ${targetRepo} -L 1 --json databaseId,status,createdAt,headBranch`);
    const runs = JSON.parse(listOutput);
    if (runs && runs.length > 0) {
      const latest = runs[0];
      const ageMs = Date.now() - new Date(latest.createdAt).getTime();
      // If run started in the last 3 minutes or is in progress / queued
      if (latest.status === 'in_progress' || latest.status === 'queued' || ageMs < 180000) {
        runId = latest.databaseId;
        console.log(`Found active run ID: ${runId} (status: ${latest.status})`);
        break;
      }
    }
  } catch (err) {
    // retry
  }
  spawnSync('powershell', ['-Command', 'Start-Sleep -Seconds 3']);
}

if (!runId) {
  console.log('Triggering workflow manually via gh workflow run...');
  run(`gh workflow run build.yml --repo ${targetRepo}`);
  spawnSync('powershell', ['-Command', 'Start-Sleep -Seconds 5']);
  const listOutput = runCapture(`gh run list --repo ${targetRepo} -L 1 --json databaseId,status`);
  const runs = JSON.parse(listOutput);
  runId = runs[0].databaseId;
  console.log(`Run started with ID: ${runId}`);
}

// 4. Watch run until completed
console.log(`\n[4/5] Watching GitHub Actions build (Run ID: ${runId})...`);
run(`gh run watch ${runId} --repo ${targetRepo} --exit-status`);

// 5. Download compiled binaries directly to firmware/
console.log(`\n[5/5] Downloading firmware binaries to ${firmwareDir}...`);
if (!fs.existsSync(firmwareDir)) {
  fs.mkdirSync(firmwareDir, { recursive: true });
}

const tempDownloadDir = path.join(repoRoot, '.firmware_temp');
if (fs.existsSync(tempDownloadDir)) {
  fs.rmSync(tempDownloadDir, { recursive: true, force: true });
}

run(`gh run download ${runId} --repo ${targetRepo} -n firmware --dir "${tempDownloadDir}"`);

fs.readdirSync(tempDownloadDir).forEach(f => {
  fs.copyFileSync(path.join(tempDownloadDir, f), path.join(firmwareDir, f));
});
fs.rmSync(tempDownloadDir, { recursive: true, force: true });

console.log('\n=====================================================');
console.log('🎉 BUILD & DOWNLOAD COMPLETED SUCCESSFULLY!');
console.log(`All firmware files are ready in: ${firmwareDir}`);
console.log('=====================================================');
fs.readdirSync(firmwareDir).forEach(f => {
  if (f.endsWith('.uf2')) {
    const stat = fs.statSync(path.join(firmwareDir, f));
    console.log(`  - ${f} (${stat.size} bytes)`);
  }
});
