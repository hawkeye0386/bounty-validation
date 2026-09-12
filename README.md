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
