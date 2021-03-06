# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Version 1.1.1 - 2021-01-19
### Changed
- Cargo mesh files are no longer have to be in Meshes\UCSO folder.
- More assertive behaviour on messing DLLs in the vessels' API.
- Custom cargoes configuration file are no longer required to be started with CargoCustom.
### Fixed
- CTD when a mesh file has numbers in its name in the cargo configuration files.
- UCSO text on the HUD color not changing with the HUD color in the ShuttlePB and XR2 platform, and some grammatical mistake.
- Incorrect description on some API methods.

## Version 1.1 - 2021-01-12
### Changed
- Cargoes must be attached in order to delete them.
- Cargo numbers when spawned start from 1 instead of 2.
- UCSO modules folder to Modules\UCSO instead of Modules, and removing UCSO from the modules name and sample projects.
- Renamed XR2_UCSOPlatform to XR2_Platform.
- API include files to Orbitersdk\include\UCSO instead of Orbitersdk\include.
- Range and distance are calculated from the surface, not from the center.
### Removed
- SetCargoDeletionRange method as it's no longer needed.

## Version 1.0.1 - 2021-01-05
### Fixed
- Cargo not grappled if its type is static and unpacked is set to false.
### Changed
- Repository structure.
- User manuals.

## Version 1.0 - 2020-03-07
First stable release.

## Alpha 7 - 2019-09-02
### Added
- Source code comments.
- Egypt, US, Europe, China, Russia, and India flags.
### Changed
- Initial spawn name to start with 1.
- The API header and library to 'UCSO'.
- The API header to be C++ interface.

## Alpha 6 - 2019-08-27
### Added
- Packable cargoes, with solar panel and table chair cargo as examples.
- Doors to the slots. Can be opened and closed.
- 2 cargo containers meshes.
- New API method: GetVersion to get the UCSO version. Can be used to verify if UCSO is installed.
- New API method: SetUnpackedGrapple to set if the cargo can be grappled if it's unpacked.
- Additional information to CargoInfo struct.
- Checks in API methods for invalid attachment handles.
- Cargo status is set to landed if it contacted the ground.
- Orbiter crashes if one of the cargo paramateres are missing, with an error message in the log.
- 'EnableFocus' option to the configurations file.
### Changed
- Cargoes are no longer unpackable if they are attached.
- Cargoes can't be grappled if they are unpacked unless set by API method SetUnpackedGrapple.
- SetSlotAttachment method returns boolean to indicate the operation result.
- GrappleResult and ReleaseResult enums.
- Cargoes can't have focus on them.
- Renamed the default mesh from 'UCSO_Cargo' to 'UCSO_Cargo1'.
- Configuration file propery 'MeshName' renamed to 'CargoMesh'.
### Removed
- API method SetCargoColumnLength due to some accuracy problems with Orbiter SDK.
- 'DisplayMessage' option from the configurations file.

## Alpha 5 - 2019-08-22
### Added
- New API methods: SetCargoColumnLength and SetCargoRowLength for the new ground release algorithm.
- A manual and an API walktrough.
### Changed
- Rename SDK to API.
- Ground release algorithm.
- Rename API method GetCargoTotalMass to GetTotalCargoMass.
- Rename API method GetCargoCount to GetAvailableCargoCount.
- Rename API method GetCargoName to GetAvailableCargoName.
- Move the attachment point to under the cargo.
- Meshes and textures structure.

## Alpha 4 - 2019-08-19
### Added
- Release on the ground.
- A static cargo example.
- Normals to the mesh, so as to have the shadows appearing correctly.
### Changed
- The repository structure.
- CargoInfo struct.
- AddCargo method creates the cargo in the attachment point position, to minimize attachment movement.
- GrappleCargo method grapples the nearest cargo.
- Updated touchdown points for Orbiter 2016.
- Updated attachment point.
- The cargos config file.
- The timing algorithm.
### Fixed
- GrappleCargo and UnpackCargo method returns error before checking all cargos.
- Total cargo mass doesn't increase when adding cargo.
- Total cargo mass doesn't decrese when releasing, unpacking, or deleting cargo.

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
- Rename API method 'Init' to 'CreateInstance'.
- The API documentation.
- Use GroundContact method instead of GetFlightStatus to detect if the vessel is landed..
- The Visual Studio solution to proper build settings.

## Alpha 1 - 2019-08-14
Initial release.