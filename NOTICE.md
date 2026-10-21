# Source and data notices

The original game code, authored shaders, build metadata, native tests and
maintenance tools in this repository are offered under the root MIT license,
subject to the exceptions in [LICENSING.md](LICENSING.md).

## Poly Haven height samples

`Source/WarfareFront/Private/WarfareFrontlineScanHeightV47.h` contains a lookup
table derived from **brown_mud_03**, a Poly Haven texture released under CC0 1.0.
The algorithm is original MIT code; the sample data keeps its CC0 status. The
file's original provenance comment is retained.

- Asset: https://polyhaven.com/a/brown_mud_03
- Poly Haven license: https://polyhaven.com/license
- [CC0 1.0 text](LICENSES/CC0-1.0.txt)

## External dependencies and omitted assets

Unreal Engine and NVIDIA plugins are external dependencies, not MIT source in
this repository. No engine implementation, NVIDIA SDK/plugin, model, texture,
animation, audio bank or private credential is supplied as part of this source
publication. The asset-derived Tripo collision/contact tables are omitted; the
replacement interfaces explicitly disable the affected optional loaders.

Previously released binary games and the repository's V86 screenshots retain
their existing terms. Their game license is preserved under `LICENSES/`; the
full component licenses remain in each unmodified binary release. Changing the
source license does not relicense the engine, SDKs, artwork or old packages.
