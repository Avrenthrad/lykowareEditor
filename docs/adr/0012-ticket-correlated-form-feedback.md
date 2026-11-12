# ADR 0012: Ticket-correlated form feedback

Status: implemented locally, proposed for Joshua's review.

## Decision

Keep a toolkit-independent, UI-owned FormFeedback lifecycle. Track the active
draft generation, submitted arguments, pending queue ticket and structured result.
A response may update form feedback only if its generation and ticket both match.
Reset local feedback on draft navigation; do not cancel or automatically retry
commands already submitted. Retain normal global status/snapshot processing for
all completions, including replies from abandoned forms.

Attach generation metadata to the desktop outgoing/pending envelopes, never to
the wire request. Disable form editing and Execute while that form is pending,
while preserving Back/search navigation. Show local admission failures and permit
explicit retry. Reject duplicate begin calls even if UI disable behavior changes.

Preserve drafts after service failures. Display matching service field messages
under controls, plus a form-level summary and expandable structured details.
Batch errors stay at batch level because the existing service returns no row
index; do not guess one. Argument changes clear obsolete feedback. Stale revisions
request the existing snapshot refresh and require review before explicit retry.

Show successful query data through an expandable UTF-8-safe 4 KiB JSON preview.
A truncation marker explains incomplete previews; retain the original structured
response until feedback resets. Do not change protocol, service, persistence or
history behavior. Feedback/navigation are transient UI state.

## Validation and limits

Model tests cover wrong tickets, old-generation replies for the same command,
duplicate submissions, pre-ticket rejection, accepted-ticket protection, draft
invalidation, explicit retry and Unicode preview bounds. Native SDL input tests
exercise shortcut-conflict rejection, draft/revision preservation, correction and
successful retry, followed by another rejection. Existing CLI/MCP, queue, docking,
layout and transaction interaction tests remain in the full desktop suite.
Windows, target hardware and screen-reader behavior remain unverified.
