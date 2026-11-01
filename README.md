# UvekulleAnders

This repository contains three independent Visual Studio solutions:

| Directory | Solution | Purpose |
|---|---|---|
| `AIML_ImageExtractor/` | `AIML_ImageExtractor.slnx` | MFC application for browsing images and video, labeling species, and exporting datasets |
| `SpeciesModelTrainer/` | `SpeciesModelTrainer.slnx` | Separate MFC trainer and Python training worker |
| `GTest/` | `AIML_ImageExtractor_Tests.slnx` | Unit tests for application components, using vendored GoogleTest |

Each solution owns its project files, source, and resources. The test project compiles selected application sources from `AIML_ImageExtractor/`; those files remain owned by the application. No source or resource assets are currently shared between solutions, so a `SharedResources/` directory is not needed.

See the [application README](AIML_ImageExtractor/README.md) and [trainer README](SpeciesModelTrainer/README.md) for build and usage instructions. Repository-level configuration, license, and developer guidance remain at the repository root.
