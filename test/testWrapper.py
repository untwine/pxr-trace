import argparse
import os
import platform
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="USD test wrapper")
    parser.add_argument("--diff-compare", default=[], type=Path, action="append")
    parser.add_argument("--baseline-dir", type=Path)
    parser.add_argument(
        "--python-path",
        dest="python_paths",
        default=[],
        type=Path,
        action="append",
    )
    parser.add_argument(
        "--library-path",
        dest="library_paths",
        default=[],
        type=Path,
        action="append",
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
    Path.cwd().joinpath(test_dir).resolve()
    if args.verbose:
        print(f"chdir: {test_dir}")

    # Set up environment variables
    env = os.environ.copy()
    env["PYTHONPATH"] = os.pathsep.join(map(str, args.python_paths))

    env_map = {"win32": "PATH", "darwin": "DYLD_LIBRARY_PATH"}
    env_name = env_map.get(sys.platform, "LD_LIBRARY_PATH")
    env[env_name] = os.pathsep.join(map(str, args.library_paths))

    # Avoid the just-in-time debugger where possible when running tests.
    env["ARCH_AVOID_JIT"] = "1"

    # Execute the command
    cmd = shlex.split(args.cmd)
    print(f"cmd: {cmd}")
    subprocess.call(cmd, shell=False, env=env)

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