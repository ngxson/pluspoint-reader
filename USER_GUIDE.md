# PlusPoint

This is a fork of [crosspoint-reader](https://github.com/crosspoint-reader/crosspoint-reader) that I created out of curiousity. The main motivation is to add some coplex features that will have slim chance of being upstreamed.

For the name of this project, "Plus" has a double meaning: It means both "being more" and "my love for C++"

Philosophical design behind this fork:
- We only target **low-level** changes, things that does not directly alter the UI/UX
- We try to introduce these feature as a wrapper, avoid too many modifications on the original code
- For any changes fall outside of these category, please send your PR to upstream project

Things that this fork is set out to support:
- [ ] Emulation (for a better DX)
- [ ] Allow runtime app, similar to a Flipper Zero (TBD how to do it)
