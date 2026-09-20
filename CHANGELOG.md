# Changelog

All notable changes to TempoFlow will be documented here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and releases use [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Changed

- Raised the minimum CMake version to 3.25 for CMake Presets schema version 6.
- Pinned JUCE and GitHub Actions to immutable commit SHAs.
- Enforced defined metadata types, RFC 3339 timestamps, and safe integer conversion in preset validation.
- Bounded preset discovery, file reads, and JSON complexity before processing untrusted input.
- Isolated Codecov OIDC access from the job that builds and executes pull-request code.

### Added

- Typed preset runtime model with bounded file loading and focused tests.
- Informational native C++ coverage reporting through Codecov.
- JUCE 9.0.2 CMake dependency and Windows Debug/Release presets.
- Toolchain verification executable.
- TempoFlow Preset Format 1.0 specification, JSON Schema, and reference presets.
- Public repository governance, contribution, security, automation, and Copilot configuration.
- Reusable semantic preset validator with positive, negative, and reference-preset tests.
- Reproducible Visual Studio 2026 CI configuration and development setup helper.
- Native C++ line-coverage reporting through Microsoft Code Coverage and Codecov.
