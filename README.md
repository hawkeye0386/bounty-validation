# Windows file-sharing validation

Small Windows regression experiments supporting investigation of a public
[Okular PDF replacement issue](https://discuss.kde.org/t/paid-request-okular-don-t-lock-file-for-overwrite/49519).

`windows-file-sharing-diagnostics` preserves the direct Win32 measurements for overwriting, `MoveFileExW` replacement, and delete/recreate under the original and candidate sharing flags. It records outcomes rather than treating them as a Poppler integration test.

## First measurements

[Run 34666389447](https://github.com/hawkeye0386/bounty-validation/actions/runs/34666389447)
used Windows Server 2025 (10.0.26100). In-place writes succeeded with both
sharing modes. `MoveFileExW` replacement failed with error 5 in both modes.
`DeleteFileW` failed with error 32 with the original flags; with delete sharing,
deletion and recreation of the pathname both succeeded while the old reader
remained open. The rename result needs further investigation.

## Pinned Poppler `GooFile` regression

`goo-file-baseline-delete-recreate` and `goo-file-candidate-delete-recreate`
compile Poppler's pinned `goo/gfile.cc` and `goo/gfile.h` twice: once unchanged
and once with the two-line `FILE_SHARE_DELETE` patch. The build uses minimal
Windows-only `config.h` and export headers, including `HAVE_FSEEK64=1`, so no
PDF parser, Qt component, or desktop application is built.

The baseline test requires `DeleteFile` to fail with
`ERROR_SHARING_VIOLATION` (32) while a `GooFile` reader is open, then verifies
that both open `GooFile` readers still read byte `A`. The candidate test covers
both `GooFile::open` overloads: narrow ASCII and wide Unicode. It requires
delete, `CREATE_NEW`, and writing byte `B` to succeed, verifies the original
reader still reads `A`, then verifies a newly opened `GooFile` reads `B`.

These tests demonstrate only Windows file-sharing behavior and the `GooFile`
component. They do not test PDF parsing, atomic replacement behavior beyond the
diagnostic measurement, Okular's document lifetime, or automatic reload
behavior.

The [component run](https://github.com/hawkeye0386/bounty-validation/actions/runs/34667189676)
at commit `38e81fa` passed all three CTest entries on Windows Server 2025
(10.0.26100), including the baseline and candidate checks for both filename
overloads. The original diagnostics reproduced the first run's results.

## Okular automatic reload

A separate [actual Okular regression](okular-refresh/README.md) passes rapid and delayed delete/recreate cycles with both the default Linux watcher and requested QFileSystemWatcher backend. This adds Linux application evidence; it does not establish the combined Windows 11 application outcome.
