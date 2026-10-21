# Licensing

The root [LICENSE](LICENSE) is the MIT licence for the original source code
published in this repository. It does not replace the terms supplied with an
existing game download or grant rights in omitted third-party technology and
assets.

## Original code published under MIT

The reviewed game-source files are enumerated in
[source-manifest.json](source-manifest.json). The manifest records their paths,
sizes, hashes, licence expressions and publication edits. It covers the original game modules in
`Source/`, included project configuration and build metadata, and any original
project shaders listed there. The repository's original maintenance tools in
`tools/`, tests, workflows and new source/build documentation also use MIT.

One source header contains separately licensed data:
`Source/WarfareFront/Private/WarfareFrontlineScanHeightV47.h` includes 16,384
height samples derived from Poly Haven's `brown_mud_03` asset under **CC0 1.0**.
The project's algorithm is MIT; those samples retain their CC0 provenance.
Its manifest licence expression is `MIT AND CC0-1.0`, not a claim that all of
its contents were authored here under MIT. The CC0 text and attribution are
retained in [LICENSES](LICENSES/).

This is a development source snapshot. It is not the frozen input used to build
the V86 installer. Publication of the original code does not claim that the
complete V86 game can be reproduced from this checkout.

## Existing binary releases

V86 and earlier downloadable game packages retain the licence and third-party
notices shipped with each package. V86's original licence is preserved unchanged
at [LICENSES/LicenseRef-WarfareFront-Binary-V86.txt](LICENSES/LicenseRef-WarfareFront-Binary-V86.txt).
Its installed name remains `GAME-LICENSE.txt`. New source licensing does not
change the bytes, tags, licence grants or restrictions of those existing
downloads. Historical release notes in `release/history.json` and `CHANGELOG.md`
remain historical statements about those releases.

The V86 screenshots in `docs/media/` retain their existing screenshot and
underlying asset rights. They are not MIT-licensed game assets, texture sources
or grants to extract and redistribute third-party components. Their capture
provenance is recorded in that directory.

## Dependencies and assets not licensed here

- **Unreal Engine:** the engine, its tools and any engine implementation are not
  included or relicensed. Obtain Unreal Engine from Epic under its applicable
  terms. Original project code calling its APIs does not grant rights in the
  engine itself.
- **NVIDIA:** proprietary DLSS, NGX, Streamline and other vendor SDK/plugin code
  or binaries are not included or relicensed. Obtain matching dependencies from
  their official distribution and keep their licences and notices.
- **Game content:** the full `Content/` and `ArtSource/` trees, models, animations,
  textures, generated illustrations, audio, fonts and cooked data containers are
  not published as part of this source release. No MIT asset licence is implied.
  Separately granted rights under third-party asset licences remain intact.
- **Tripo-derived data:** the reviewed Tripo output geometry and its derived
  contact tables are excluded. Where a code interface is retained, the published
  replacement is an original, geometry-free fallback under MIT. It reports
  `SourceDataAvailable=false`, and the dependent optional visual loaders are
  disabled. These replacements are not the original generated models or contact
  data and do not reproduce their visual result. No Tripo output is relicensed
  as MIT by these interface replacements.
- **Other third parties:** no proprietary engine/plugin code is made MIT merely
  because the game depends on it. Any third-party source later accepted into the
  repository must be identified, retain its original licence and be excluded
  from the default MIT grant where necessary.

The existing game packages contain their own `THIRD-PARTY-NOTICES.md` and
`Licenses/` material. A source user must also retain the notices supplied with
the dependencies they obtain. The root licence applies only to rights held by
the original code's contributors; it is not an endorsement by Epic Games,
NVIDIA, Armor Games or the creators of Warfare 1917 / Warfare 1944.

See [BUILDING.md](docs/BUILDING.md) for dependency and build limits.
