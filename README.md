# Windows file-sharing validation

Small Windows regression experiments supporting investigation of a public
[Okular PDF replacement issue](https://discuss.kde.org/t/paid-request-okular-don-t-lock-file-for-overwrite/49519).

The harness distinguishes overwriting a file, replacing it by rename, and
deleting it before creating a new file at the same path. It compares the file
sharing flags currently used by Poppler with a candidate that adds delete
sharing.

These are operating-system behavior checks. Passing them does not establish
that a complete Okular integration works or that a sponsor has accepted a fix.

The GitHub Actions job uses a standard Windows runner and has a five-minute
timeout. It does not use account secrets, access user files, or contact a
production application.

## First measurements

[Run 34666389447](https://github.com/hawkeye0386/bounty-validation/actions/runs/34666389447)
used Windows Server 2025 (10.0.26100). In-place writes succeeded with both
sharing modes. `MoveFileExW` replacement failed with error 5 in both modes.
`DeleteFileW` failed with error 32 with the original flags; with delete sharing,
deletion and recreation of the pathname both succeeded while the old reader
remained open. The rename result needs further investigation.

A successful job means the measurements completed, not that each operation
succeeded. These observations do not yet verify a Poppler or Okular build.
