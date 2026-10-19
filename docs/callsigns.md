---
read_when: Changing Settings, callsign detection, transcript colors, flags, or QRZ links
---

# Callsigns and display settings

Open Settings beside Close all tabs. Its navigation pages group text/callsigns,
CW decoding, monitoring and diagnostics. Changes apply immediately. Both hosts
save the display options inside the CW Notebook module instance.

Detect Callsigns and Deduce Country defaults on. Delimited, plausible callsigns
with a recognized CTY prefix receive an inline flag and a recent callsign tag.
The last 24 unique callsigns are retained in the displayed transcript's tag row;
scroll the row horizontally when crowded. A received JS8 header call is also
included. Click a tag to open its encoded callsign on QRZ.com in the default
browser. No online lookup occurs during decoding, detection or hovering.

RTTY applies a narrow ambiguity check. If a recognized callsign occurs at least
twice and another token occurs once with one extra leading letter, the longer
token keeps its received text but receives no flag or tag. For example, two
`YL3NU` occurrences plus one `LYL3NU` leave the latter unannotated. This does not
establish which call was transmitted. If the longer token also repeats, it is
annotated normally. Portable calls are excluded from this check. Completed
tokens from the same displayed transcript supply evidence; provisional text
does not. Counts saturate at two and track at most 256 distinct tokens. When
that budget is exhausted, untracked tokens retain the ordinary behavior.

This is a display safeguard, not a decoding correction. No leading letters are
removed or substituted, and copied reports/logs retain the original text. Later
reception can remove an earlier ambiguous flag; selection is cleared if its
display indices would otherwise become stale. Ordinary appends keep selection.

Country is an offline prefix/exact-call estimate from the bundled 23 March 2024
CTY snapshot. Portable prefixes override a home call; maritime/airborne calls
do not establish a land country. Inferred country is separate from a received
grid, remote report or verified station identity. CW provisional drafts do not
establish callsign tags. Unknown or unfinished text is left alone.

Highlight HAM QSO terms defaults on and can be disabled independently. Calling,
identity, report and courtesy terms have distinct colors. Callsigns have their
own color while detection is enabled. Labels and received text remain readable
without depending on color alone.

Original reception text is never changed by flags or highlighting. Native flags
render beside callsigns without adding fonts to the host. Selected annotations
copy as Unicode flags; full reception reports and CW logs retain the received
text. Text selection, Cmd+C, right-click copying, Follow and the waterfall input
guard remain in the original host multiline widget.
