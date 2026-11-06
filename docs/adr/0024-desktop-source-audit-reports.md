# ADR 0024: Full desktop source audit reports

Status: implemented locally, proposed for Joshua's review.

Render the complete asset.audit response in a searchable, clipped table within
its command form. Keep the existing bounded generic JSON preview and summary,
but allow every returned row to be inspected. Search covers IDs, paths, kinds,
status and diagnostic messages using existing ASCII case folding. A status filter
separates valid/invalid/skipped rows; clearing or changing filters clears selection.
Rows retain report ID order. Details show complete diagnostics, source size/entity
counts and selectable SHA-256 tokens where present. Status uses text, not colour.

Keep report browsing transient. It is presentation of an existing shared query;
it does not change project state and needs no new command, persistence or library.
The report belongs to its correlated command draft. Editing its root invalidates
the result; opening another form resets filters/selection on its next generation.
Never retain pointers to report rows across frames. Re-resolve the selected ID in
the current response so replacement reports update details and action eligibility.

Validate-current-source opens asset.validate with the report row ID and submitted
root. Instantiate-audited-bytes is available only for successfully validated prefab
rows and opens asset.instantiate_checked with ID/root/audited digest/parent zero.
Actions copy their arguments before changing the active form and never read files
or call the service on the render thread. Existing service validation and revision,
source-digest and catalogue guards remain authoritative. Audited identities are
not persistent provenance: navigation still resolves the current catalogue.

Reports remain past sequential observations. Navigation leaves the audit form,
so it does not preserve a report window or implicitly resubmit a command. Unsupported
kinds have explanation text and disabled source actions. Empty results remain valid.
The implementation does not claim to solve screen readers, controller navigation,
Unicode normalization or measured hardware performance.

Tests cover access beyond the 4 KiB preview, filtering and action eligibility,
full-response lifecycle, and native SDL selection/navigation with 4096 entries.
Replacement results disable obsolete checked actions. The native test also runs
in X11/Xvfb with visual inspection; Windows and target hardware remain unverified.
