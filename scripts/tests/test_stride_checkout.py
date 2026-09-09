"""Observable checkout guards, using disposable local repositories and no network."""
import base64
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
LOCK = json.loads((ROOT / "tools/stride/engine.lock.json").read_text())


def ps_literal(value):
    return "'" + str(value).replace("'", "''") + "'"


@unittest.skipUnless(os.name == "nt" and shutil.which("git") and shutil.which("powershell"),
                     "Windows PowerShell and Git required")
class StrideCheckoutTests(unittest.TestCase):
    def setUp(self):
        self.fixture = Path(tempfile.mkdtemp(prefix="rat-stride-guards-"))
        self.repo = self.fixture / "checkout"
        self.repo.mkdir()

    def tearDown(self):
        target = self.fixture.resolve()
        temporary_root = Path(tempfile.gettempdir()).resolve()
        if target.parent != temporary_root or not target.name.startswith("rat-stride-guards-"):
            raise RuntimeError("Refusing cleanup outside fixture directory")
        # Git marks loose objects read-only on Windows; only this verified fixture is removed.
        def writable_remove(function, path, _error):
            os.chmod(path, 0o700)
            function(path)
        shutil.rmtree(target, onerror=writable_remove)

    def git(self, *args):
        return subprocess.check_output(["git", "-C", str(self.repo), *args], stderr=subprocess.STDOUT).decode().strip()

    def initialize(self):
        self.git("init", "-b", LOCK["localBranch"])
        self.git("config", "user.name", "Fixture")
        self.git("config", "user.email", "fixture@example.invalid")
        self.git("remote", "add", LOCK["upstreamRemote"], LOCK["upstreamUrl"])
        self.git("commit", "--allow-empty", "-m", "baseline")
        return self.git("rev-parse", "HEAD")

    def run_ps(self, script):
        encoded = base64.b64encode(script.encode("utf-16-le")).decode()
        return subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass",
                               "-EncodedCommand", encoded], capture_output=True)

    def bootstrap(self):
        return self.run_ps("& " + ps_literal(ROOT / "scripts/stride/bootstrap.ps1") +
                           " -CheckoutPath " + ps_literal(self.repo))

    def guard(self, baseline):
        return self.run_ps(". " + ps_literal(ROOT / "scripts/stride/common.ps1") +
                           "; $script:StrideLock.upstreamCommit = " + ps_literal(baseline) +
                           "; try { Assert-StrideCheckout " + ps_literal(self.repo) +
                           " } catch { Write-Output $_; exit 1 }")

    def test_foreign_directory_preserved(self):
        marker = self.repo / "keep.txt"
        marker.write_bytes(b"valuable unrelated file")
        self.assertNotEqual(self.bootstrap().returncode, 0)
        self.assertEqual(marker.read_bytes(), b"valuable unrelated file")
        self.assertFalse((self.repo / ".git").exists())

    def test_wrong_remote_preserves_head_and_url(self):
        head = self.initialize()
        self.git("remote", "set-url", "upstream", "https://example.invalid/other.git")
        result = self.guard(head)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"Unexpected upstream URL: https://example.invalid/other.git", result.stdout)
        wiring = self.bootstrap()
        self.assertNotEqual(wiring.returncode, 0)
        self.assertIn(b"Unexpected upstream URL", wiring.stdout + wiring.stderr)
        self.assertEqual(self.git("rev-parse", "HEAD"), head)
        self.assertEqual(self.git("remote", "get-url", "upstream"), "https://example.invalid/other.git")

    def test_dirty_checkout_preserves_files_index_and_head(self):
        head = self.initialize()
        (self.repo / "staged.txt").write_bytes(b"staged data")
        self.git("add", "staged.txt")
        (self.repo / "untracked.txt").write_bytes(b"untracked data")
        status = self.git("status", "--porcelain")
        result = self.guard(head)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(b"Stride checkout has uncommitted files.", result.stdout)
        wiring = self.bootstrap()
        self.assertNotEqual(wiring.returncode, 0)
        self.assertIn(b"Stride checkout has uncommitted files.", wiring.stdout + wiring.stderr)
        self.assertEqual(self.git("status", "--porcelain"), status)
        self.assertEqual(self.git("rev-parse", "HEAD"), head)
        self.assertEqual((self.repo / "staged.txt").read_bytes(), b"staged data")
        self.assertEqual((self.repo / "untracked.txt").read_bytes(), b"untracked data")

    def test_custom_descendant_commit_is_preserved(self):
        baseline = self.initialize()
        self.git("commit", "--allow-empty", "-m", "own engine patch")
        head = self.git("rev-parse", "HEAD")
        result = self.guard(baseline)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn(head.encode(), result.stdout)
        self.assertEqual(self.git("rev-parse", "HEAD"), head)

    def test_other_branch_is_not_switched(self):
        baseline = self.initialize()
        self.git("switch", "-c", "unrelated-work")
        self.assertNotEqual(self.guard(baseline).returncode, 0)
        self.assertEqual(self.git("branch", "--show-current"), "unrelated-work")

    def test_missing_baseline_is_not_reset(self):
        baseline = self.initialize()
        self.assertNotEqual(self.guard("0" * 40).returncode, 0)
        self.assertEqual(self.git("rev-parse", "HEAD"), baseline)

    def test_relative_checkout_follows_powershell_location(self):
        # A PowerShell location change does not update .NET's process working directory.
        result = self.run_ps(". " + ps_literal(ROOT / "scripts/stride/common.ps1") +
                             "; [Environment]::CurrentDirectory = " + ps_literal(ROOT) +
                             "; Push-Location -LiteralPath " + ps_literal(self.fixture) +
                             "; try { Get-StrideCheckoutPath './checkout' } finally { Pop-Location }")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(Path(result.stdout.decode().strip()), self.repo)

    def test_fresh_bootstrap_fetches_exact_fork_patch_not_moving_tip(self):
        baseline = self.initialize()
        self.git("branch", "-m", "master")
        upstream = self.fixture / "upstream.git"
        subprocess.check_call(["git", "clone", "--bare", str(self.repo), str(upstream)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        (self.repo / "editor-patch.txt").write_text("qualified native state")
        self.git("add", "editor-patch.txt")
        self.git("commit", "-m", "qualified editor patch")
        pinned = self.git("rev-parse", "HEAD")
        (self.repo / "future-unqualified.txt").write_text("do not select moving fork tip")
        self.git("add", "future-unqualified.txt")
        self.git("commit", "-m", "later unrelated work")
        fork = self.fixture / "fork.git"
        subprocess.check_call(["git", "clone", "--bare", str(self.repo), str(fork)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        harness = self.fixture / "harness"
        shutil.copytree(ROOT / "scripts/stride", harness / "scripts/stride")
        (harness / "tools/stride").mkdir(parents=True)
        fixture_lock = dict(LOCK, upstreamCommit=baseline, engineCommit=pinned,
                            upstreamUrl=str(upstream), forkUrl=str(fork), forkRemote="origin")
        (harness / "tools/stride/engine.lock.json").write_text(json.dumps(fixture_lock))
        target = self.fixture / "fresh checkout[pin]"
        result = self.run_ps("& " + ps_literal(harness / "scripts/stride/bootstrap.ps1") +
                             " -CheckoutPath " + ps_literal(target))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(subprocess.check_output(["git", "-C", str(target), "rev-parse", "HEAD"]).decode().strip(), pinned)
        self.assertTrue((target / "editor-patch.txt").exists())
        self.assertFalse((target / "future-unqualified.txt").exists())
        self.assertEqual(subprocess.check_output(["git", "-C", str(target), "status", "--porcelain"]).strip(), b"")

    def test_build_enters_literal_bracket_checkout_and_records_artifact(self):
        self.repo = self.fixture / "checkout[rat]"
        self.repo.mkdir()
        baseline = self.initialize()
        (self.repo / ".gitignore").write_text("logs/\neditor.exe\ninvoked-from.txt\n")
        self.git("add", ".gitignore")
        self.git("commit", "-m", "fixture build output exclusions")
        harness = self.fixture / "harness"
        shutil.copytree(ROOT / "scripts/stride", harness / "scripts/stride")
        (harness / "tools/stride").mkdir(parents=True)
        fixture_lock = dict(LOCK, upstreamCommit=baseline, editorExecutable="editor.exe")
        (harness / "tools/stride/engine.lock.json").write_text(json.dumps(fixture_lock))
        # Stub only the compiler: the real wrapper, Git/LFS guards, paths and result writing run.
        result = self.run_ps("function dotnet { if ($args[0] -eq '--version') { '10.0.300' } "
                             "else { [IO.File]::WriteAllText((Join-Path (Get-Location).Path 'editor.exe'), 'fixture'); "
                             "[IO.File]::WriteAllText((Join-Path (Get-Location).Path 'invoked-from.txt'), (Get-Location).Path); "
                             "'fixture compiler success' }; $global:LASTEXITCODE = 0 }; & " +
                             ps_literal(harness / "scripts/stride/build.ps1") +
                             " -CheckoutPath " + ps_literal(self.repo))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertEqual(Path((self.repo / "invoked-from.txt").read_text()), self.repo)
        manifest_path, = (self.repo / "logs/rat-foundation").glob("*/result.json")
        manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
        self.assertTrue(manifest["editorExists"])
        self.assertEqual(Path(manifest["editor"]), self.repo / "editor.exe")
        self.assertEqual(self.git("status", "--porcelain"), "")


if __name__ == "__main__":
    unittest.main()
