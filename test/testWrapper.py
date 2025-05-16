import argparse
import os
import platform
import shlex
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="USD test wrapper")
    parser.add_argument('--stderr-redirect', type=Path)
    parser.add_argument("--diff-compare", default=[], type=Path, action="append")
    parser.add_argument("--testenv-dir", type=Path)
    parser.add_argument("--baseline-dir", type=Path)
    parser.add_argument("--expected-return-code", type=int, default=0)
    parser.add_argument(
        "--env-var", dest="env_vars", default=[], type=str, action="append",
    )
    parser.add_argument("--verbose", "-v", action="store_true")
    parser.add_argument("cmd", metavar="CMD", type=str)
    args = parser.parse_args()

    # Ensure baseline directory is provided if using diff comparison
    if args.diff_compare and not args.baseline_dir:
        sys.stderr.write(
            "Error: --baseline-dir is required with --diff-compare."
        )
        sys.exit(1)

    # Create a temporary directory for testing
    test_dir = Path(tempfile.mkdtemp())
    os.chdir(test_dir)
    if args.verbose:
        print(f"chdir: {test_dir}")

    # Copy test environment directory if provided
    if args.testenv_dir and args.testenv_dir.is_dir():
        if args.verbose:
            print(f"copying testenv dir: {args.testenv_dir}")
        try:
            shutil.copytree(args.testenv_dir, Path.cwd(), dirs_exist_ok=True)
        except Exception as err:
            sys.stderr.write(f"Error: copying testenv directory: {err}\n")
            sys.exit(1)

    # Set up environment variables
    env = os.environ.copy()
    for env_var in args.env_vars:
        if '=' not in env_var:
            sys.stderr.write(f"Error: Invalid env var '{env_var}'\n")
            sys.exit(1)
        key, value = env_var.split('=', 1)
        env[key] = value

    # Avoid the just-in-time debugger where possible when running tests.
    env["ARCH_AVOID_JIT"] = "1"

    # Execute the command
    cmd = shlex.split(args.cmd)
    stderr = args.stderr_redirect.open('w') if args.stderr_redirect else None
    try:
        print(f"cmd: {cmd}")
        retcode = subprocess.call(
            cmd, shell=False, env=env, stdin=subprocess.DEVNULL,
            stdout=sys.stdout, stderr=(stderr or sys.stderr)
        )

        # Convert negative return codes (signals) to standard exit codes
        retcode = 128 + abs(retcode) if retcode < 0 else retcode

    finally:
        if stderr:
            stderr.close()

    # Check if return code matches expected return code
    if retcode != args.expected_return_code:
        if args.verbose:
            sys.stderr.write(
                f"Error: return code {retcode} doesn't match "
                f"expected {args.expected_return_code} (EXPECTED_RETURN_CODE)."
            )
        sys.exit(1)

    # Perform diff comparison if requested
    if args.diff_compare:
        diff_cmd = (
            ["fc.exe", "/t"] if platform.system() == "Windows"
            else ["diff", "--strip-trailing-cr"]
        )
        if not diff_cmd[0]:
            sys.stderr.write(
                "Error: could not find \"diff\" or \"fc.exe\" tool. "
                "Make sure it's in your PATH.\n"
            )
            sys.exit(1)

        # Some test envs are contained within a non-specific
        # subdirectory, if that exists then use it for the baselines
        baseline_dir = args.baseline_dir / "non-specific"
        if not baseline_dir.is_dir():
            baseline_dir = args.baseline_dir

        for file_name in args.diff_compare:
            path = baseline_dir / file_name
            cmd = diff_cmd + [str(path), str(file_name)]
            if args.verbose:
                print(f"diffing with {cmd}")

            if subprocess.call(cmd) != 0:
                sys.stderr.write(
                    f"Error: diff for {file_name} failed (DIFF_COMPARE)."
                )
                sys.exit(1)

    sys.exit(0)
