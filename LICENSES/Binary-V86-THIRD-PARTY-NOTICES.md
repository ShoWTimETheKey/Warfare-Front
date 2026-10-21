# Warfare Front — credits and third-party notices

Warfare Front is an independent First World War trench-strategy game. It is not an official Warfare 1917 / Warfare 1944 release, and is not endorsed by their creators or Armor Games. The original games informed reference study; their SWFs, extracted code, graphics and audio are not supplied as game assets.

## Engine and graphics technology

Warfare Front uses Unreal® Engine. Unreal® is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere.

Unreal® Engine, Copyright 1998 – 2026, Epic Games, Inc. All rights reserved.

This game integrates NVIDIA DLSS Super Resolution, Ray Reconstruction, Frame Generation and NVIDIA Reflex through NVIDIA SDKs and Streamline. NVIDIA, the NVIDIA logo, GeForce and RTX are trademarks and/or registered trademarks of NVIDIA Corporation in the U.S. and other countries. Use of these technologies does not imply endorsement. NVIDIA components remain proprietary software under their accompanying licenses. The Streamline open-source license does not relicense NVIDIA's proprietary DLSS binaries.

The Unreal integration source was adapted to report stable relative filenames in Direct3D presentation-error diagnostics, preserving error details and line numbers while omitting the build machine's private directory. NVIDIA's proprietary runtime DLLs remain unmodified.

See `Licenses/NVIDIA/` for the applicable SDK license texts, and `Licenses/UnrealEngine-ThirdParty/` for the engine's third-party component notices. Those notices also cover libraries compiled into the executable; they are not additional tools or editable game assets. They may describe optional engine components not exercised by this game.

Portions of this software are copyright © The FreeType Project (http://www.freetype.org). All rights reserved.

Unmodified Microsoft Visual C++ v14 release runtime components and the signed x64 redistributable installer are included for Windows compatibility. Their Microsoft license is in `Licenses/Microsoft/`; no compiler or debug runtime is supplied. These Microsoft components are not relicensed under the game license.

## Recorded voices and sound design

The following two CC BY 3.0 sources have been modified for this game. Attribution, source links, modification statements and the full license accompany the release:

- **“Soldier voice acting” by kurt**, [OpenGameArt](https://opengameart.org/content/soldier-voice-acting), [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/). Modified by complete-utterance/take selection, silence trimming, filtering, gain and fades, resampling, mono conversion, saturation and short outdoor-reflection treatment. These are actor performances, not historical combat recordings.
- **“2 High Quality Explosions” by Michel Baradari (apollo-music.de)**, archived by qubodup, [OpenGameArt](https://opengameart.org/content/2-high-quality-explosions), [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/). Modified by excerpting, filtering, layering with other effects, gain/fades, mono conversion, resampling and spatial/reflection treatment; used in blast, cannon and distant-battle sound designs.

Additional recorded sources are used under **CC0 1.0**. Credits are retained with thanks:

| Source | Creator / performer |
| --- | --- |
| [The Free Firearm Sound Library](https://opengameart.org/content/the-free-firearm-sound-library) | Ben Jaszczak, Brian Nelson, Kevin Heras and Matthew Nanney |
| [Voiceover Pack, 40 lines](https://opengameart.org/content/voiceover-pack-40-lines) | Kenney; male performance by Jeffrey M. Smith |
| [German voices](https://opengameart.org/content/german-voices-0) | Manuel Senfft; commissioned by Andrettin |
| [Male grunt/yelling sounds](https://opengameart.org/content/male-gruntyelling-sounds) | HaelDB and the four male performers credited by the source author; the author's CC0 option is used |
| [Battle – Take cover GD, sound 800514](https://freesound.org/people/TyrantTim/sounds/800514/) | Timothy Whiting / TyrantTim |
| [01 go go go.wav, sound 443442](https://freesound.org/people/balloonhead/sounds/443442/) | balloonhead |
| [wind1](https://opengameart.org/content/wind1) | Luke.RUSTLTD |
| [25 CC0 mud SFX](https://opengameart.org/content/25-cc0-mud-sfx) | rubberduck |
| [Rain (loopable)](https://opengameart.org/content/rain-loopable) | Ylmir |
| [Truck Engine Idle Loops, sound 187564](https://freesound.org/people/qubodup/sounds/187564/) and [mechanical sound 199282](https://freesound.org/people/qubodup/sounds/199282/) | qubodup |
| [R08-07-Men Cheering at Event.wav, sound 480793](https://freesound.org/people/craigsmith/sounds/480793/) | Craig Smith / USC Cinema Optical Sound Effects Library; CC0 1.0 |
| [Attention Whistle, sound 320140](https://freesound.org/people/OwlStorm/sounds/320140/) | Ashe Kirk / Owlish Media (OwlStorm); CC0 1.0 |
| [R29-46-Men Yell and Cheer at Sporting Event.wav, sound 479969](https://freesound.org/people/craigsmith/sounds/479969/) | Craig Smith / USC Cinema Optical Sound Effects Library; CC0 1.0 |
| [short whistle it tunnel.aiff, sound 109354](https://freesound.org/people/SoundCollectah/sounds/109354/) | SoundCollectah; CC0 1.0 |
| [Interface Sounds 1.0](https://kenney.nl/assets/interface-sounds) | Kenney; CC0 1.0 |

These recordings are edited, mixed and spatially treated for the game. Weapon and vehicle effects are representative sound designs, not claims of exact recordings of every named historical weapon. Nonverbal casualty sounds are acted performances.

The V61 console cues adapt Kenney's `click_001`, `click_002`, `switch_003` and `switch_004` through filtering, layering and conversion to 48 kHz stereo PCM. Kenney's original pack notice is included in [Kenney-Interface-Sounds-CC0.txt](Licenses/Assets/Kenney-Interface-Sounds-CC0.txt); the complete CC0 dedication is in `Licenses/Assets/CC0-1.0.txt`.

The V51 outcome ensembles use edited sections of two recorded male groups and two short recorded human whistles above, obtained as the sources' public high-quality MP3 renditions. They are filtered, faded, stereo-mixed and treated with short outdoor reflections; their original pitch and speed are retained. The groups are vintage staged sporting-event sound effects, not authenticated First World War recordings. They are combined with six new original synthetic character calls: three British English and three German. There are four final ensemble takes in total, two per army. Their source credits, recording hashes and processing summary are included in [the V51 audio provenance](Licenses/Assets/WarfareFront-Audio-V51-Provenance.json). The result music remains a separate original instrumental arrangement using the credited CC0 orchestral samples below. No Warfare 1944 sound, whistle melody or soundtrack recording is included. The full CC0 dedication is supplied in `Licenses/Assets/CC0-1.0.txt`.

Additional English, German, French and Russian battlefield voices, including German-language Austro-Hungarian calls, are fictional synthetic performances made locally with [Qwen3-TTS VoiceDesign](https://github.com/QwenLM/Qwen3-TTS), with no real-person voice reference or cloning. The model/code use Apache 2.0; its license is retained for provenance. Neither the voice-generation model nor its inference runtime is part of the game download.

The national expansion's original illustrated portraits, support pictures and maps are historical-inspired artwork, not archival photographs. Its character, equipment, terrain and motion derivatives retain the underlying sources' existing terms. The French, Russian and Austro-Hungarian outcome ensembles combine original synthetic calls with the credited CC0 crowds and whistles above. Their selected source identities and processing notes are recorded in [the national expansion provenance](Licenses/Assets/WarfareFront-FRRU-V82-Provenance.json).

## Music

Original game arrangements use recorded instrument samples from [VS Chamber Orchestra Community Edition](https://github.com/sgossner/VSCO-2-CE), **CC0 1.0**: Sam Gossner and Simon Dalzell recorded the instruments; Elan Hickler / Soundemote edited the samples. The arrangements, note sequences, envelopes, mix and transitions were authored for Warfare Front. No original Warfare soundtrack is included.

The V82.8 national score adds twelve original arrangements: three intensity states each for Britain, Germany, France and Russia. Its 54 selected recorded instrument samples come from VSCO Community Edition at [revision 440300901dfe9275fd84e0b7763af1f8443ae62e](https://github.com/sgossner/VSCO-2-CE/tree/440300901dfe9275fd84e0b7763af1f8443ae62e), under the same CC0 1.0 dedication. Processing includes selected note excerpts, modest pitch changes, articulation envelopes, stereo placement, equalization, short reflections and level control. The national motifs and note sequences are original project compositions, not historical anthems or copied game music. The full source dedication is included in [VSCO-CC0.txt](Licenses/Assets/VSCO-CC0.txt).

## Characters, animation and surfaces

- **Human Base Meshes v1.4.1**, Blender Studio / Dan Ulrich and community contributors, [Blender demo assets](https://www.blender.org/download/demo-files/), CC0 1.0. Adapted anatomical foundations, topology and skinning.
- **Józef Giza uniform**, Virtual Museums of Małopolska / RDW MIC, scanned and reconstructed by **Marcin Solarz**, CC0 1.0. Modified garment-scan foundations; not represented as an unmodified historical British/German 1917 uniform.
- **Universal Animation Library Standard**, [Quaternius](https://quaternius.com/packs/universalanimationlibrary.html), CC0 1.0. Retargeted and adapted locomotion/death foundations; other poses, transitions, equipment and reactions are project-authored adaptations.
- **CMU Motion Capture subject 113, take 24**, Carnegie Mellon University, via RancidMilk's FBX conversion. Adapted grenade-throw motion. The source permits use, modification and redistribution within products, but the motion data itself must not be sold separately, including converted forms. Preserve these terms when redistributing the complete game. See `Licenses/Assets/LICENSE-CMU.txt` and [CMU Motion Capture](http://mocap.cs.cmu.edu/).
- **Poly Haven materials**, [CC0 1.0](https://polyhaven.com/license): Brown Mud 02, Brown Mud 03 and Aerial Mud 1 by Rob Tuytel; Weathered Brown Planks and Rust Coarse 01, photography by Dimitrios Savva and processing by Rico Cilliers; Hessian 230, photography by colormass and processing by Rico Cilliers; Brown Leather and Poly Wool Herringbone from Poly Haven. Maps are adapted through materials, UVs, tinting, physical roughness/metallic treatment and layered weathering. The source assets remain CC0.
- **V51 regional ground scans**, Poly Haven, CC0 1.0: [Ground Grey](https://polyhaven.com/a/ground_grey) and [Snow 03](https://polyhaven.com/a/snow_03) by Rob Tuytel; [Brick Gravel](https://polyhaven.com/a/brick_gravel) by Dimitrios Savva. Each surface retains its matching diffuse, DirectX normal, roughness, displacement and ambient-occlusion channels. Material-channel packing, stochastic projection and wet-weather response are project adaptations. These scans are material sources, not surveys of specific First World War battle sites. Source-channel identities and credits are included in [the V51 art provenance](Licenses/Assets/WarfareFront-Art-V51-Provenance.json).

Vehicle, railway, trench and equipment modeling, gameplay shaders, particle composition and related integration were authored for this game using the credited source foundations. Third-party asset rights are not converted into ownership of their creators' trademarks.

V37 tank identification paint and crew assignments are original composite game designs based on period marking formats, not exact replicas of named historical vehicles. Their lettering uses the already credited Stardos Stencil font, converted to conforming surface geometry. Revised tank weathering retains the existing credited material foundations; no additional third-party texture pack is introduced.

## Typography and original illustration

**Noto Sans CJK** and **Noto Serif CJK** (Noto project), **Stardos Stencil** (Vernon Adams), and **Oswald** (the copyright holders listed in its OFL notice) are used under the **SIL Open Font License 1.1**. Their full original copyright, reserved-font-name and license notices are in `Licenses/Fonts/`. Glyph textures and engraved/paint-worn rendering are game treatments; this notice does not rename or relicense the source fonts.

V82.10 adds **Libre Franklin** (the Libre Franklin Project Authors), **Courier Prime** (the Courier Prime Project Authors), **Big Shoulders Stencil** (the Big Shoulders Project Authors), **LXGW WenKai** (LXGW and the Klee Project Authors), and **UnifrakturMaguntia** (j. 'mach' wust and Peter Wiegel), and a new SemiBold **Oswald** face, under the **SIL Open Font License 1.1**. Full original notices are supplied in `Licenses/Fonts/V8210-*-OFL.txt`. Oswald, Libre Franklin and Big Shoulders variable fonts are instantiated at the selected weight coordinates; Big Shoulders uses display optical size 72. No additional glyph design edits are applied. Courier Prime, LXGW WenKai and UnifrakturMaguntia retain unmodified upstream font data.

Portraits, campaign maps, interface art, field studies and selected effect/texture inputs include original AI-generated illustrations commissioned for this project, with project editing and runtime composition. Illustrative reconstructions of historical commanders and events are not authenticated archival photographs. Original reference-game assets were not imported into these illustrations.

V51 adds four campaign interlude illustrations, two distant battlefield backgrounds and one four-portrait unit atlas. They are original generated historical reconstructions made for this project, with project treatment and integration, not archival photographs, historical documents or evidence of the depicted events. Their identities and roles are listed in [the V51 art provenance](Licenses/Assets/WarfareFront-Art-V51-Provenance.json).

V52 adds an original generated headquarters map and a two-star command-status atlas. The map uses public-domain [Natural Earth](https://www.naturalearthdata.com/) land and river data as a geographic reference; **Made with Natural Earth.** Primary source authors are Tom Patterson, Nathaniel Vaughn Kelso and the Natural Earth contributors. Cropping, projection, small-river annotations, generated relief and paper treatment, labels, mission pins and dated strategic overlays are project adaptations. Natural Earth's [published terms](https://www.naturalearthdata.com/about/terms-of-use/) identify the source data as public domain. This is an illustrative strategic reconstruction, not a period document or surveyed trench map. The gold and silver stars are original game illustrations, not extracted reference-game sprites. Source identities and illustration hashes appear in [the V52 art provenance](Licenses/Assets/WarfareFront-Art-V52-Provenance.json); the geography notice is in [NaturalEarth-PUBLIC-DOMAIN.txt](Licenses/Assets/NaturalEarth-PUBLIC-DOMAIN.txt).

The V57 geographically registered operation tiers adapt real 1917 military sheets:

- **Service Géographique de l'Armée, *France 1:80 000 / Cambrai* (1917), ETH-Bibliothek Zürich, K P 688438: 13, [DOI 10.3931/e-rara-149340](https://doi.org/10.3931/e-rara-149340), [Public Domain Mark](https://www.e-rara.ch/zut/content/titleinfo/32107714)**.
- **Service Géographique de l'Armée, *France 1:80 000 / Arras - Douai* (1917), ETH-Bibliothek Zürich, K P 688438: 7-8, [DOI 10.3931/e-rara-149335](https://doi.org/10.3931/e-rara-149335), Public Domain Mark 1.0**.

These period maps were cropped, approximately registered to the campaign projection, darkened and feathered; their original survey ink is preserved in the close operation views. They are not surveyed reconstructions of the game's trenches or mission boundaries. The V57 broader relief context also incorporates Mapzen terrain-derived EU-DEM layers: **Produced using Copernicus data and information funded by the European Union — EU-DEM layers.** Natural Earth remains credited above. The corresponding source identifiers, bounds, modifications and hashes are recorded in the project's V57 campaign-art provenance.

V61 extends this period-map treatment across the operations table and its loading views:

- **Service geographique de l'armee, *France 1:80 000* sheets (1917), ETH-Bibliothek Zurich, digitized through e-rara**, covering Saint-Omer/Lille, Arras/Douai, Amiens, Cambrai and Montdidier. The source records identify these sheets with the **Public Domain Mark 1.0**. Each scan's source URL and hash are retained in [the V61 provenance](Licenses/Assets/WarfareFront-V61-Provenance.json).
- **Stanfords, *Stanford's half-inch map of the battle front: Valenciennes, Maubeuge, Avesnes* (1917), National Library of Wales, record 5003807**, obtained from [the library's Wikimedia Commons contribution](https://commons.wikimedia.org/wiki/File:Stanford%27s_half-inch_map_of_the_battle_front_Valenciennes,_Maubeuge,_Avesnes_(5003807).jpg). The source identifies this map as public domain and applies the [Public Domain Mark 1.0](https://creativecommons.org/publicdomain/mark/1.0/).

Runtime adaptations exclude scan collars, approximately register the maps to the campaign projection, blend their overlaps and normalize the individual scans' paper-white luminance to the game's ink/paper palette. Original downloaded scan files remain unchanged. At sheet margins, historical coverage blends into the game's cartographic background; it is not a continuous historical survey of every visible area. Texture padding, mip generation and compression prepare the sheets for rendering. Their historical markings belong to the original sheets' dates; game orders, mission boundaries and current front lines are separate illustrative overlays. Neither archive endorses the game. Public-domain source material retains its public-domain status.

V61's main table view retains the established V54 Western Front artwork, natively 4096 x 2872. Its newly authored 8192-square overview remains an imported fallback; the five regional and twenty operation background plates are registered local layers. These are original cartographic treatments informed by the credited Natural Earth and relief sources, not newly discovered period documents. Its menu image is an original generated illustration. The separately generated map-table concept is a development reference, not a runtime historical map. The [V61 provenance](Licenses/Assets/WarfareFront-V61-Provenance.json) distinguishes these roles and lists source dimensions without claiming a higher native resolution.

The V37 painted portraits of Henry Horne, Otto von Below, Adolph von Carlowitz and Bruno von Mudra are generated likeness reconstructions. Identity research used Library of Congress Bain Collection records for [Horne](https://www.loc.gov/pictures/item/2014710150/), [Carlowitz](https://www.loc.gov/pictures/item/2014697264/) and [Mudra](https://www.loc.gov/pictures/item/2014698615/) (the archive states no known restrictions on publication), and the [Wien Museum portrait of Below](https://sammlung.wienmuseum.at/en/object/107831-general-der-infanterie-otto-von-below/) (CC0). These reference photographs are not distributed as game portraits or included in the release archive.

V64 replaces the changing operation sheets with one continuous, original Western Front atlas using the previously credited Natural Earth and relief sources. An 8 by 8 virtual-texture layout assigns higher source density to the playable front. The terrain hatching, settlement footprints and communications are illustrative; they are not a survey of individual wartime positions. Its cloth capture flags reuse the game's credited national flag paintings. The shrapnel support portrait and revised support sight are original generated bitmap illustrations. Source identities and dimensions are recorded in `Licenses/Assets/WarfareFront-V64-Atlas-Provenance.json`.

The individual third-party licenses take precedence for their components. The game license does not restrict rights those licenses independently grant, and does not grant any broader rights to proprietary Unreal Engine or NVIDIA software.

The original Warfare Front wordmark and WF monogram use custom proportions and outlines derived from **Roboto Slab**, copyright 2011 Google Inc., licensed under **Apache License 2.0**. The complete source license is included in `Licenses/Fonts/RobotoSlab-Apache2.txt`. The font is a letterform source; the composite game identity and illustrations were authored for this project.

V76 uses paired 4K **Bark Brown 02** (Rob Tuytel) and **Hessian 380** (Rico Cilliers / colormass) base-colour, DirectX normal and roughness scans from [Poly Haven](https://polyhaven.com/), under **CC0 1.0**. Original sources are [Bark Brown 02](https://polyhaven.com/a/bark_brown_02) and [Hessian 380](https://polyhaven.com/a/hessian_380). Tree geometry, fracture grain, gameplay material layering, environment concepts and achievement illustrations are original project work. Achievement illustrations are fictional game emblems, not replicas of official decorations. Reference Warfare 1917 / 1944 source and art were used for private analysis and are not distributed.

V83.7 adds original armoured-train geometry, versioned vehicle wear and small plate-impact effects. Its national deed chest badge, train portrait and two tribute emblems are original generated illustrations; no film stills, recognisable fictional characters or game logos are distributed. Six short armour-contact waveforms are original seeded plate/body sound design and are not archival recordings. Existing source licences continue to apply to the reused materials and engine components.

V83.8 adds original per-chassis vehicle and wreck geometry derivatives, wear/normal atlases, running-gear impressions and a wreck-only volumetric soot field. They reuse the credited paired material and density-atlas foundations under their existing terms. Generated armour concept boards are private development references; they are not archival photographs or distributed runtime textures.

V83.9 adds fourteen original generated operation-map illustrations and nine original armour-contact sound designs. The illustrations are historical reconstructions with approximate operational marks, not archival military surveys. Versioned terrain and plate-fragment materials retain the credited source textures and mesh foundations under their existing licences.

V84.4.11 replaces those synthetic armour-contact sounds with short edits of recorded steel Foley under **CC0 1.0**: **Metal Plate Hit 1, 2 and 3** by loganzsound ([784152](https://freesound.org/people/loganzsound/sounds/784152/), [784153](https://freesound.org/people/loganzsound/sounds/784153/), [784154](https://freesound.org/people/loganzsound/sounds/784154/)) and **Metal Hit 2** by dslrguide ([321482](https://freesound.org/people/dslrguide/sounds/321482/)). Thick-plate and wheelbarrow strikes are excerpted, filtered, rapidly damped and layered at their original pitch. Public HQ previews are the source format. These are designed armour contacts, not recordings of First World War ammunition striking armour. Source hashes and modifications accompany the game in `Licenses/Assets/WarfareFront-Audio-V84411-Provenance.json`; the included CC0 1.0 text continues to apply.

V83.10 adds original procedural vehicle wear maps and a derived combustion-volume shader. Existing models, source surface maps and their licences are retained. These visual derivatives do not replace the original component credits above.

V84.4.8 battlefield timber adapts four Poly Haven scans under CC0 1.0: Dead Tree Trunk 02 (Jenelle van Heerden / Rico Cilliers), Dead Tree Trunk, Tree Stump 01 and Tree Stump 02 (Rob Tuytel). Their source files and hashes are recorded in `Licenses/Assets/WarfareFront-V8448-Timber-Provenance.json`; attribution and source links are in the accompanying notice.

V84.4.9 ground projection, snow response, conforming tree-root earth, sandbag and field-structure derivatives, and persistent explosion-scorch geometry are project adaptations. They reuse the credited paired mud, regional ground, Snow 03, Hessian 380, timber, rust and cloth sources under their existing terms. Generated environment target boards are development references, not archival photographs or runtime terrain surveys. The underlying third-party licences remain unchanged.

V84.4.9 continuous snow caps use [Snow 02](https://polyhaven.com/a/snow_02) by Rob Tuytel, [CC0 1.0](https://polyhaven.com/license), at 4K with its paired diffuse, DirectX normal, roughness, displacement and ambient-occlusion scans. The two-metre scan is a surface source, not a survey of a particular battle site. Channel packing and the snow/mud/weather coverage are project adaptations. Original channel identities and hashes are included in `Licenses/Assets/WarfareFront-V8449-Snow-Provenance.json`.
