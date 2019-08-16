# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Alpha 3 - 2019-08-16
### Added
- Cargo mesh.
- Config files.
- GetCargoName method to get the cargo name.
- AddCargo method to add a cargo to the vessel.
- GetCargoInfo method to get cargo information.
### Fixed
- Crash when using a resource.
- Container mass not added when creating the cargo from scenario editor.
### Changed
- UnpackCargo method can unpack cargo from distance.
- GrappleCargo method checks if the cargo isn't attached to another vessel.
- Compile using ISO C++ 17 to have std::filesystem.

## Alpha 2 - 2019-08-14
### Added
- Remove a slot using SetSlotAttachment method.
### Fixed
- DetachChild doesn't apply the passed velocity (Thanks to Woo482).
### Changed
- Rename SDK method 'Init' to 'CreateInstance'.
- The SDK documentation.
- Use GroundContact method instead of GetFlightStatus to detect if the vessel is landed..
- The Visual Studio solution to proper build settings.

## Alpha 1 - 2019-08-14
Initial release.