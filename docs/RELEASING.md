# Release maintenance

This is the binary game's distribution and support repository. The game source,
Unreal project and proprietary assets are not published here. The tools in this
repository regenerate documentation; they do not rebuild the game or establish
reproducible binary provenance.

## Documentation source

`release/current.json` holds the current release's facts and bilingual copy.
`release/history.json` preserves the original notes of every public release.
Draft releases are excluded from the public changelog. Run:

```text
python tools/render_release_docs.py
python -m unittest discover -s tests -v
python tools/render_release_docs.py --check
```

Review generated README, release body, start instructions and installed notes
together. Commit these outputs with their source change. CI rejects drift and
does not have publishing permissions. Repository text is UTF-8 without BOM and
uses LF; the installed filename mapping must remain explicit.

The V86 installer installs the release attachment `ReleaseNotes.md` as
`ReleaseNotes.txt`. That mapping is intentional. The generator also supplies an
identical installed copy, so future packaging can consume one source.

Do not upload regenerated V86 documents over the original release assets: the
original checksum file and the installer already embed their old bytes. The
new repository documentation describes the existing release; a changed installer
or payload belongs to a new release.

## Release identity and files

Keep historical tags unchanged. Use `vMAJOR.MINOR.PATCH` consistently for future
tags once that policy is adopted, and `Warfare Front V<version> — Windows x64`
for titles. The public download link currently targets the latest **release
page**, which works without rewriting the installer name by hand.

A stable `Warfare-Front-Setup.exe` alias is optional for a future release. Add it
before publishing, include it in the checksum manifest, and verify it is
byte-identical to that release's versioned installer. Do not link to an alias
before it exists. Keep the versioned download URLs embedded in the installer;
an old installer must never combine new data volumes with an old manifest.

## Signing

V86 is unsigned. Its SHA-256 checks confirm byte equality, not an independent
publisher identity. Obtain an eligible code-signing identity before describing
any future release as signed. Sign the game's own launchers and binaries first,
then configure Inno Setup's named `SignTool` and `SignedUninstaller=yes`, compile,
verify the installer and uninstaller signatures, and calculate final hashes.
Do not re-sign third-party DLLs or change files after signing.

Keep certificate keys and service credentials outside the repository. Never
paste them into an issue or commit. Microsoft Artifact Signing (formerly
Trusted Signing) has region and identity eligibility requirements; do not assume
an Australian personal account is eligible. A valid signature still does not
guarantee that SmartScreen will never display a reputation warning.

- [Inno Setup SignTool](https://jrsoftware.org/ishelp/topic_setup_signtool.htm)
- [Signed uninstaller](https://jrsoftware.org/ishelp/topic_setup_signeduninstaller.htm)
- [Microsoft signing eligibility](https://learn.microsoft.com/en-us/azure/artifact-signing/quickstart)
- [SmartScreen reputation](https://learn.microsoft.com/windows/apps/package-and-deploy/smartscreen-reputation)

## Immutable releases

Enabling repository immutable releases affects future publications; it does not
retroactively lock V86. This maintenance change does not enable that setting or
delete any draft or historical release. The setting requires an authorized
administrator operation.

Once enabled, create a draft, upload **all** installers, data volumes, final
checksums and documents, verify the complete set, then publish. Published assets
and the release tag are locked. GitHub's resulting attestation binds that
release snapshot; it does not prove that this closed-source game was rebuilt
from public source, nor that its binaries are harmless.

- [Enable release immutability](https://docs.github.com/en/code-security/how-tos/secure-your-supply-chain/establish-provenance-and-integrity/prevent-release-changes)
- [Immutable release behaviour](https://docs.github.com/en/code-security/concepts/supply-chain-security/immutable-releases)
- [Verify release integrity](https://docs.github.com/en/code-security/how-tos/secure-your-supply-chain/secure-your-dependencies/verify-release-integrity)

## Updates and package size

V85 to V86 retained 1,150 of 1,156 payload files unchanged, but the six changed
files account for 14,738,215,829 bytes, or 98.459% of V86's installed payload.
The changed main `.ucas` is 14,500,154,704 bytes. Renaming whole files with hashes
will therefore not create a useful small update by itself.

Repeat the whole-file comparison against two retained private manifests with:

```text
python tools/compare_payload_manifests.py old-payload-manifest.json new-payload-manifest.json
```

This reads only manifest JSON, not the game containers. It reports unchanged,
changed, added and removed files, and the cost of replacing whole files.

Retain a frozen baseline and evaluate stable chunking or a V86-to-next-version
patch against the actual IoStore containers. Require old-install verification,
download integrity, space checks, interrupted-update recovery, rollback and a
real upgraded Shipping launch before offering it to players. A file-manifest
comparison cannot predict the size of a binary delta.

V86 already uses compressed Pak/IoStore data and an outer LZMA2 archive. Its
payload contains no PDBs. The accumulation of AlwaysCook directories merits a
resource dependency audit, but a directory named after an old version may still
be used by runtime soft paths. Do not delete such directories based on their
name or copy PAK-only patch settings into IoStore without testing.

- [Epic patch baselines](https://dev.epicgames.com/documentation/en-us/unreal-engine/how-to-create-a-patch-in-unreal-engine)
- [BuildPatchServices](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/BuildPatchServices)
- [Valve package and patch practices](https://partner.steamgames.com/doc/sdk/uploading?l=english)

## Retention and support

Do not delete old releases merely to make the list shorter. Keep at least a
known working fallback and the private frozen build inputs needed for any
supported patch baseline. Archiving or deleting the abandoned v82.21 draft
requires a separate maintainer decision. Changing old titles or adding a
superseded note is optional; never change existing versioned download URLs.

CPU, RAM and GPU performance minimums still need a low-end hardware test.
Neither Unreal Editor's recommended specs nor a developer's target for an
RTX 3060 establish this game's measured minimum. Preserve the distinction
between focused checks and a complete natural campaign playthrough.
