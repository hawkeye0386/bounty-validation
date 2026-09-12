# Vendored Poppler component

`goo/gfile.cc` and `goo/gfile.h` are unmodified copies from Poppler commit `be09305851e326ce189ee516788fd4a9e7d382c6` (2026-09-11), obtained from the official Poppler repository. `COPYING` is the accompanying GNU GPL version 2 license; the source headers state GPL version 2 or later.

The build copies this component unchanged as the baseline, then derives the candidate with a checked replacement of the two exact sharing expressions. `../../poppler-file-share-delete.patch` is the normal-context review artifact for the same two-line change; it is independently applicable to this pinned source.
