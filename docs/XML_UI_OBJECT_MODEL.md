# Object model fields

<!-- Generated from assets/object_model/schema.json (scripts/generate_om_schema.py) by tests/src/test_cases/UI/test_xml_ui_api.cpp. Don't edit it here. -->

Every field the printer's object model can have, from @duet3d/objectmodel 3.6.4, for the `om.*` names XML binds to (see [Object model](XML_UI_API.md#object-model)). `[]` stands for an index, e.g. `heat.heaters[1].current`, and `{}` for a name the printer chooses, e.g. a global variable's. A field this doesn't list (one a newer firmware added) can still be used once the printer sends it.

Types: a number, string (text) or boolean (shown as true or false; `|int` gives 1 or 0), an object of the named type, an enum (text, one of its values), or `T[]`, an array of T. A field can also be null, which shows as empty text, or 0.

## boards

| Field | Type |
| --- | --- |
| `boards` | Board[] |
| `boards[].accelerometer` | Accelerometer |
| `boards[].accelerometer.orientation` | number |
| `boards[].accelerometer.points` | number |
| `boards[].accelerometer.runs` | number |
| `boards[].bootloaderFileName` | string |
| `boards[].canAddress` | number |
| `boards[].closedLoop` | BoardClosedLoop |
| `boards[].closedLoop.points` | number |
| `boards[].closedLoop.runs` | number |
| `boards[].directDisplay` | DirectDisplay |
| `boards[].directDisplay.encoder` | DirectDisplayEncoder |
| `boards[].directDisplay.encoder.pulsesPerClick` | number |
| `boards[].directDisplay.screen` | DirectDisplayScreenBase |
| `boards[].directDisplay.screen.colourBits` | number |
| `boards[].directDisplay.screen.contrast` | number |
| `boards[].directDisplay.screen.controller` | DirectDisplayController: ST7920, ST7567, ILI9488 |
| `boards[].directDisplay.screen.height` | number |
| `boards[].directDisplay.screen.resistorRatio` | number |
| `boards[].directDisplay.screen.spiFreq` | number |
| `boards[].directDisplay.screen.width` | number |
| `boards[].drivers` | Driver[] |
| `boards[].drivers[].closedLoop` | DriverClosedLoop |
| `boards[].drivers[].closedLoop.currentFraction` | BoardClosedLoopCurrentFraction |
| `boards[].drivers[].closedLoop.currentFraction.avg` | number |
| `boards[].drivers[].closedLoop.currentFraction.max` | number |
| `boards[].drivers[].closedLoop.positionError` | BoardClosedLoopPositionError |
| `boards[].drivers[].closedLoop.positionError.max` | number |
| `boards[].drivers[].closedLoop.positionError.rms` | number |
| `boards[].drivers[].status` | number |
| `boards[].firmwareDate` | string |
| `boards[].firmwareFileName` | string |
| `boards[].firmwareName` | string |
| `boards[].firmwareVersion` | string |
| `boards[].freeRam` | number |
| `boards[].iapFileNameSBC` | string |
| `boards[].iapFileNameSD` | string |
| `boards[].inductiveSensor` | InductiveSensor |
| `boards[].maxHeaters` | number |
| `boards[].maxMotors` | number |
| `boards[].mcuTemp` | MinMaxCurrent |
| `boards[].mcuTemp.current` | number |
| `boards[].mcuTemp.max` | number |
| `boards[].mcuTemp.min` | number |
| `boards[].name` | string |
| `boards[].shortName` | string |
| `boards[].state` | BoardState: unknown, flashing, flashFailed, resetting, running, timedOut |
| `boards[].supportsDirectDisplay` | boolean |
| `boards[].uniqueId` | string |
| `boards[].v12` | MinMaxCurrent |
| `boards[].v12.current` | number |
| `boards[].v12.max` | number |
| `boards[].v12.min` | number |
| `boards[].vIn` | MinMaxCurrent |
| `boards[].vIn.current` | number |
| `boards[].vIn.max` | number |
| `boards[].vIn.min` | number |
| `boards[].wifiFirmwareFileName` | string |

## directories

| Field | Type |
| --- | --- |
| `directories` | Directories |
| `directories.filaments` | string |
| `directories.firmware` | string |
| `directories.gCodes` | string |
| `directories.macros` | string |
| `directories.menu` | string |
| `directories.system` | string |
| `directories.web` | string |

## fans

| Field | Type |
| --- | --- |
| `fans` | Fan[] |
| `fans[].actualValue` | number |
| `fans[].blip` | number |
| `fans[].frequency` | number |
| `fans[].max` | number |
| `fans[].min` | number |
| `fans[].name` | string |
| `fans[].requestedValue` | number |
| `fans[].rpm` | number |
| `fans[].tachoPpr` | number |
| `fans[].thermostatic` | FanThermostaticControl |
| `fans[].thermostatic.heaters` | number[] |
| `fans[].thermostatic.highTemperature` | number |
| `fans[].thermostatic.lowTemperature` | number |
| `fans[].thermostatic.sensors` | number[] |

## global

| Field | Type |
| --- | --- |
| `global` | {any} |

## heat

| Field | Type |
| --- | --- |
| `heat` | Heat |
| `heat.bedHeaters` | number[] |
| `heat.chamberHeaters` | number[] |
| `heat.coldExtrudeTemperature` | number |
| `heat.coldRetractTemperature` | number |
| `heat.heaters` | Heater[] |
| `heat.heaters[].active` | number |
| `heat.heaters[].avgPwm` | number |
| `heat.heaters[].current` | number |
| `heat.heaters[].extrPwmBoost` | number |
| `heat.heaters[].extrTempBoost` | number |
| `heat.heaters[].max` | number |
| `heat.heaters[].maxBadReadings` | number |
| `heat.heaters[].maxHeatingFaultTime` | number |
| `heat.heaters[].maxTempExcursion` | number |
| `heat.heaters[].min` | number |
| `heat.heaters[].model` | HeaterModel |
| `heat.heaters[].model.coolingExp` | number |
| `heat.heaters[].model.coolingRate` | number |
| `heat.heaters[].model.deadTime` | number |
| `heat.heaters[].model.enabled` | boolean |
| `heat.heaters[].model.fanCoolingRate` | number |
| `heat.heaters[].model.heatingRate` | number |
| `heat.heaters[].model.inverted` | boolean |
| `heat.heaters[].model.maxPwm` | number |
| `heat.heaters[].model.pid` | HeaterModelPID |
| `heat.heaters[].model.pid.d` | number |
| `heat.heaters[].model.pid.i` | number |
| `heat.heaters[].model.pid.overridden` | boolean |
| `heat.heaters[].model.pid.p` | number |
| `heat.heaters[].model.pid.used` | boolean |
| `heat.heaters[].model.standardVoltage` | number |
| `heat.heaters[].monitors` | HeaterMonitor[] |
| `heat.heaters[].monitors[].action` | HeaterMonitorAction |
| `heat.heaters[].monitors[].condition` | HeaterMonitorCondition: disabled, tooHigh, tooLow |
| `heat.heaters[].monitors[].limit` | number |
| `heat.heaters[].monitors[].sensor` | number |
| `heat.heaters[].sensor` | number |
| `heat.heaters[].standby` | number |
| `heat.heaters[].state` | HeaterState: off, standby, active, fault, tuning, offline |

## inputs

| Field | Type |
| --- | --- |
| `inputs` | InputChannel[] |
| `inputs[].active` | boolean |
| `inputs[].axesRelative` | boolean |
| `inputs[].compatibility` | Compatibility: Default, RepRapFirmware, Marlin, Teacup, Sprinter, Repetier, NanoDLP |
| `inputs[].distanceUnit` | DistanceUnit: mm, in |
| `inputs[].drivesRelative` | boolean |
| `inputs[].feedRate` | number |
| `inputs[].inMacro` | boolean |
| `inputs[].lineNumber` | number |
| `inputs[].macroRestartable` | boolean |
| `inputs[].motionSystem` | number |
| `inputs[].name` | string |
| `inputs[].selectedPlane` | number |
| `inputs[].stackDepth` | number |
| `inputs[].state` | InputChannelState: awaitingAcknowledgement, idle, executing, waiting, reading |
| `inputs[].volumetric` | boolean |

## job

| Field | Type |
| --- | --- |
| `job` | Job |
| `job.build` | Build |
| `job.build.currentObject` | number |
| `job.build.m486names` | boolean |
| `job.build.m486numbers` | boolean |
| `job.build.objects` | BuildObject[] |
| `job.build.objects[].cancelled` | boolean |
| `job.build.objects[].name` | string |
| `job.build.objects[].x` | number[] |
| `job.build.objects[].y` | number[] |
| `job.duration` | number |
| `job.file` | GCodeFileInfo |
| `job.file.customInfo` | {any} |
| `job.file.filament` | number[] |
| `job.file.fileName` | string |
| `job.file.generatedBy` | string |
| `job.file.height` | number |
| `job.file.lastModified` | string |
| `job.file.layerHeight` | number |
| `job.file.numLayers` | number |
| `job.file.printTime` | number |
| `job.file.simulatedTime` | number |
| `job.file.size` | number |
| `job.file.thumbnails` | ThumbnailInfo[] |
| `job.file.thumbnails[].data` | string |
| `job.file.thumbnails[].format` | ThumbnailFormat: jpeg, png, qoi |
| `job.file.thumbnails[].height` | number |
| `job.file.thumbnails[].offset` | number |
| `job.file.thumbnails[].size` | number |
| `job.file.thumbnails[].width` | number |
| `job.filePosition` | number |
| `job.lastDuration` | number |
| `job.lastFileAborted` | boolean |
| `job.lastFileCancelled` | boolean |
| `job.lastFileName` | string |
| `job.lastFileSimulated` | boolean |
| `job.lastWarmUpDuration` | number |
| `job.layer` | number |
| `job.layerTime` | number |
| `job.layers` | Layer[] |
| `job.layers[].duration` | number |
| `job.layers[].filament` | number[] |
| `job.layers[].fractionPrinted` | number |
| `job.layers[].height` | number |
| `job.layers[].temperatures` | number[] |
| `job.pauseDuration` | number |
| `job.rawExtrusion` | number |
| `job.timesLeft` | TimesLeft |
| `job.timesLeft.filament` | number |
| `job.timesLeft.file` | number |
| `job.timesLeft.slicer` | number |
| `job.timesLeft.toPause` | number |
| `job.warmUpDuration` | number |

## ledStrips

| Field | Type |
| --- | --- |
| `ledStrips` | LedStrip[] |
| `ledStrips[].board` | number |
| `ledStrips[].pin` | string |
| `ledStrips[].stopMovement` | boolean |
| `ledStrips[].type` | LedStripType: DotStar, NeoPixel_RGB, NeoPixel_RGBW |

## limits

| Field | Type |
| --- | --- |
| `limits` | Limits |
| `limits.axes` | number |
| `limits.axesPlusExtruders` | number |
| `limits.bedHeaters` | number |
| `limits.boards` | number |
| `limits.chamberHeaters` | number |
| `limits.drivers` | number |
| `limits.driversPerAxis` | number |
| `limits.extruders` | number |
| `limits.extrudersPerTool` | number |
| `limits.fans` | number |
| `limits.gpInPorts` | number |
| `limits.gpOutPorts` | number |
| `limits.heaters` | number |
| `limits.heatersPerTool` | number |
| `limits.ledStrips` | number |
| `limits.monitorsPerHeater` | number |
| `limits.portsPerHeater` | number |
| `limits.reportedAxes` | number |
| `limits.restorePoints` | number |
| `limits.sensors` | number |
| `limits.spindles` | number |
| `limits.tools` | number |
| `limits.trackedObjects` | number |
| `limits.triggers` | number |
| `limits.volumes` | number |
| `limits.workplaces` | number |
| `limits.zProbeProgramBytes` | number |
| `limits.zProbes` | number |

## messages

| Field | Type |
| --- | --- |
| `messages` | Message[] |
| `messages[].content` | string |
| `messages[].time` | any |
| `messages[].type` | MessageType |

## move

| Field | Type |
| --- | --- |
| `move` | Move |
| `move.accelerationTime` | number |
| `move.axes` | Axis[] |
| `move.axes[].acceleration` | number |
| `move.axes[].babystep` | number |
| `move.axes[].backlash` | number |
| `move.axes[].current` | number |
| `move.axes[].drivers` | DriverId[] |
| `move.axes[].drivers[].board` | number |
| `move.axes[].drivers[].driver` | number |
| `move.axes[].homed` | boolean |
| `move.axes[].jerk` | number |
| `move.axes[].letter` | AxisLetter: X, Y, Z, U, V, W, A, B, C, D, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,  |
| `move.axes[].machinePosition` | number |
| `move.axes[].max` | number |
| `move.axes[].maxProbed` | boolean |
| `move.axes[].microstepping` | Microstepping |
| `move.axes[].microstepping.interpolated` | boolean |
| `move.axes[].microstepping.value` | number |
| `move.axes[].min` | number |
| `move.axes[].minProbed` | boolean |
| `move.axes[].percentCurrent` | number |
| `move.axes[].percentStstCurrent` | number |
| `move.axes[].phaseStep` | boolean |
| `move.axes[].printingJerk` | number |
| `move.axes[].reducedAcceleration` | number |
| `move.axes[].speed` | number |
| `move.axes[].stepPos` | number |
| `move.axes[].stepsPerMm` | number |
| `move.axes[].userPosition` | number |
| `move.axes[].visible` | boolean |
| `move.axes[].workplaceOffsets` | number[] |
| `move.backlashFactor` | number |
| `move.calibration` | MoveCalibration |
| `move.calibration.final` | MoveDeviations |
| `move.calibration.final.deviation` | number |
| `move.calibration.final.mean` | number |
| `move.calibration.initial` | MoveDeviations |
| `move.calibration.initial.deviation` | number |
| `move.calibration.initial.mean` | number |
| `move.calibration.numFactors` | number |
| `move.compensation` | MoveCompensation |
| `move.compensation.fadeHeight` | number |
| `move.compensation.file` | string |
| `move.compensation.liveGrid` | ProbeGrid |
| `move.compensation.liveGrid.axes` | string[] |
| `move.compensation.liveGrid.maxs` | number[] |
| `move.compensation.liveGrid.mins` | number[] |
| `move.compensation.liveGrid.radius` | number |
| `move.compensation.liveGrid.spacings` | number[] |
| `move.compensation.meshDeviation` | MoveDeviations |
| `move.compensation.meshDeviation.deviation` | number |
| `move.compensation.meshDeviation.mean` | number |
| `move.compensation.probeGrid` | ProbeGrid |
| `move.compensation.probeGrid.axes` | string[] |
| `move.compensation.probeGrid.maxs` | number[] |
| `move.compensation.probeGrid.mins` | number[] |
| `move.compensation.probeGrid.radius` | number |
| `move.compensation.probeGrid.spacings` | number[] |
| `move.compensation.skew` | Skew |
| `move.compensation.skew.compensateXY` | boolean |
| `move.compensation.skew.tanXY` | number |
| `move.compensation.skew.tanXZ` | number |
| `move.compensation.skew.tanYZ` | number |
| `move.compensation.type` | MoveCompensationType: none, mesh |
| `move.currentMove` | CurrentMove |
| `move.currentMove.acceleration` | number |
| `move.currentMove.deceleration` | number |
| `move.currentMove.extrusionRate` | number |
| `move.currentMove.laserPwm` | number |
| `move.currentMove.requestedSpeed` | number |
| `move.currentMove.topSpeed` | number |
| `move.extruders` | Extruder[] |
| `move.extruders[].acceleration` | number |
| `move.extruders[].current` | number |
| `move.extruders[].driver` | DriverId |
| `move.extruders[].driver.board` | number |
| `move.extruders[].driver.driver` | number |
| `move.extruders[].factor` | number |
| `move.extruders[].filament` | string |
| `move.extruders[].filamentDiameter` | number |
| `move.extruders[].jerk` | number |
| `move.extruders[].microstepping` | Microstepping |
| `move.extruders[].microstepping.interpolated` | boolean |
| `move.extruders[].microstepping.value` | number |
| `move.extruders[].nonlinear` | ExtruderNonlinear |
| `move.extruders[].nonlinear.a` | number |
| `move.extruders[].nonlinear.b` | number |
| `move.extruders[].nonlinear.upperLimit` | number |
| `move.extruders[].percentCurrent` | number |
| `move.extruders[].percentStstCurrent` | number |
| `move.extruders[].phaseStep` | boolean |
| `move.extruders[].position` | number |
| `move.extruders[].pressureAdvance` | number |
| `move.extruders[].printingJerk` | number |
| `move.extruders[].rawPosition` | number |
| `move.extruders[].speed` | number |
| `move.extruders[].stepsPerMm` | number |
| `move.idle` | MotorsIdleControl |
| `move.idle.factor` | number |
| `move.idle.timeout` | number |
| `move.keepout` | KeepoutZone[] |
| `move.keepout[].active` | boolean |
| `move.keepout[].coords` | KeepoutZoneCoordinates[] |
| `move.keepout[].coords[].max` | number |
| `move.keepout[].coords[].min` | number |
| `move.kinematics` | Kinematics |
| `move.kinematics.anchors` | number[][] |
| `move.kinematics.crosstalk` | number[] |
| `move.kinematics.deltaRadius` | number |
| `move.kinematics.distalLength` | number |
| `move.kinematics.forwardMatrix` | number[][] |
| `move.kinematics.homedHeight` | number |
| `move.kinematics.inverseMatrix` | number[][] |
| `move.kinematics.minRadius` | number |
| `move.kinematics.name` | KinematicsName: cartesian, coreXY, coreXYU, coreXYUV, coreXZ, markForged, FiveBarScara, Hangprinter, delta, Polar, Rotary delta, Scara, unknown |
| `move.kinematics.printRadius` | number |
| `move.kinematics.proximalLength` | number |
| `move.kinematics.psiLimits` | number[] |
| `move.kinematics.radiusHomed` | number |
| `move.kinematics.radiusMax` | number |
| `move.kinematics.radiusMin` | number |
| `move.kinematics.segmentation` | MoveSegmentation |
| `move.kinematics.segmentation.minSegLength` | number |
| `move.kinematics.segmentation.segmentsPerSec` | number |
| `move.kinematics.thetaLimits` | number[] |
| `move.kinematics.tiltCorrection` | TiltCorrection |
| `move.kinematics.tiltCorrection.correctionFactor` | number |
| `move.kinematics.tiltCorrection.lastCorrections` | number[] |
| `move.kinematics.tiltCorrection.maxCorrection` | number |
| `move.kinematics.tiltCorrection.screwPitch` | number |
| `move.kinematics.tiltCorrection.screwX` | number[] |
| `move.kinematics.tiltCorrection.screwY` | number[] |
| `move.kinematics.towers` | DeltaTower[] |
| `move.kinematics.towers[].angleCorrection` | number |
| `move.kinematics.towers[].diagonal` | number |
| `move.kinematics.towers[].endstopAdjustment` | number |
| `move.kinematics.towers[].xPos` | number |
| `move.kinematics.towers[].yPos` | number |
| `move.kinematics.ttAccMax` | number |
| `move.kinematics.ttSpeedMax` | number |
| `move.kinematics.xOffset` | number |
| `move.kinematics.xTilt` | number |
| `move.kinematics.yOffset` | number |
| `move.kinematics.yTilt` | number |
| `move.limitAxes` | boolean |
| `move.noMovesBeforeHoming` | boolean |
| `move.printingAcceleration` | number |
| `move.queue` | MoveQueueItem[] |
| `move.queue[].gracePeriod` | number |
| `move.queue[].length` | number |
| `move.rotation` | MoveRotation |
| `move.rotation.angle` | number |
| `move.rotation.centre` | number[] |
| `move.shaping` | InputShaping |
| `move.shaping.amplitudes` | number[] |
| `move.shaping.damping` | number |
| `move.shaping.delays` | number[] |
| `move.shaping.frequency` | number |
| `move.shaping.type` | InputShapingType: none, mzv, zvd, zvdd, zvddd, ei2, ei3, custom |
| `move.speedFactor` | number |
| `move.travelAcceleration` | number |
| `move.usingSCurve` | boolean |
| `move.virtualEPos` | number |
| `move.workplaceNumber` | number |

## network

| Field | Type |
| --- | --- |
| `network` | Network |
| `network.corsSite` | string |
| `network.hostname` | string |
| `network.interfaces` | NetworkInterface[] |
| `network.interfaces[].activeProtocols` | NetworkProtocol[]: http, https, ftp, sftp, telnet, ssh |
| `network.interfaces[].actualIP` | string |
| `network.interfaces[].configuredIP` | string |
| `network.interfaces[].dnsServer` | string |
| `network.interfaces[].firmwareVersion` | string |
| `network.interfaces[].gateway` | string |
| `network.interfaces[].mac` | string |
| `network.interfaces[].numReconnects` | number |
| `network.interfaces[].rssi` | number |
| `network.interfaces[].signal` | number |
| `network.interfaces[].speed` | number |
| `network.interfaces[].ssid` | string |
| `network.interfaces[].state` | NetworkInterfaceState: disabled, enabled, initFailed, starting1, starting2, changingMode, establishingLink, obtainingIP, connected, active, idle |
| `network.interfaces[].subnet` | string |
| `network.interfaces[].type` | NetworkInterfaceType: ethernet, wifi |
| `network.interfaces[].wifiCountry` | string |
| `network.name` | string |

## plugins

| Field | Type |
| --- | --- |
| `plugins` | {Plugin} |
| `plugins.{}.author` | string |
| `plugins.{}.data` | any |
| `plugins.{}.dsfFiles` | string[] |
| `plugins.{}.dwcDependencies` | string[] |
| `plugins.{}.dwcFiles` | string[] |
| `plugins.{}.dwcVersion` | string |
| `plugins.{}.homepage` | string |
| `plugins.{}.id` | string |
| `plugins.{}.license` | string |
| `plugins.{}.name` | string |
| `plugins.{}.pid` | number |
| `plugins.{}.rrfVersion` | string |
| `plugins.{}.sbcAutoRestart` | boolean |
| `plugins.{}.sbcConfigFiles` | string[] |
| `plugins.{}.sbcDsfVersion` | string |
| `plugins.{}.sbcExecutable` | string |
| `plugins.{}.sbcExecutableArguments` | string |
| `plugins.{}.sbcExtraExecutables` | string[] |
| `plugins.{}.sbcOutputRedirected` | boolean |
| `plugins.{}.sbcPackageDependencies` | string[] |
| `plugins.{}.sbcPermissions` | SbcPermission[]: none, commandExecution, codeInterceptionRead, codeInterceptionReadWrite, managePlugins, servicePlugins, manageUserSessions, objectModelRead, objectModelReadWrite, registerHttpEndpoints, readFilaments, writeFilaments, readFirmware, writeFirmware, readGCodes, writeGCodes, readMacros, writeMacros, readMenu, writeMenu, readSystem, writeSystem, readWeb, writeWeb, fileSystemAccess, launchProcesses, networkAccess, webcamAccess, gpioAccess, superUser |
| `plugins.{}.sbcPluginDependencies` | string[] |
| `plugins.{}.sbcPythonDependencies` | string[] |
| `plugins.{}.sbcRequired` | boolean |
| `plugins.{}.sdFiles` | string[] |
| `plugins.{}.tags` | string[] |
| `plugins.{}.version` | string |

## sbc

| Field | Type |
| --- | --- |
| `sbc` | SBC |
| `sbc.appArmor` | boolean |
| `sbc.cpu` | CPU |
| `sbc.cpu.avgLoad` | number |
| `sbc.cpu.hardware` | string |
| `sbc.cpu.numCores` | number |
| `sbc.cpu.temperature` | number |
| `sbc.distribution` | string |
| `sbc.distributionBuildTime` | string |
| `sbc.dsf` | DSF |
| `sbc.dsf.buildDateTime` | string |
| `sbc.dsf.httpEndpoints` | HttpEndpoint[] |
| `sbc.dsf.httpEndpoints[].endpointType` | HttpEndpointType: GET, POST, PUT, PATCH, TRACE, DELETE, OPTIONS, WebSocket |
| `sbc.dsf.httpEndpoints[].isUploadRequest` | boolean |
| `sbc.dsf.httpEndpoints[].namespace` | string |
| `sbc.dsf.httpEndpoints[].path` | string |
| `sbc.dsf.httpEndpoints[].unixSocket` | string |
| `sbc.dsf.is64Bit` | boolean |
| `sbc.dsf.pluginSupport` | boolean |
| `sbc.dsf.rootPluginSupport` | boolean |
| `sbc.dsf.userSessions` | UserSession[] |
| `sbc.dsf.userSessions[].accessLevel` | AccessLevel: readOnly, readWrite |
| `sbc.dsf.userSessions[].id` | number |
| `sbc.dsf.userSessions[].origin` | string |
| `sbc.dsf.userSessions[].originId` | number |
| `sbc.dsf.userSessions[].sessionType` | SessionType: local, http, telnet |
| `sbc.dsf.version` | string |
| `sbc.memory` | Memory |
| `sbc.memory.available` | number |
| `sbc.memory.total` | number |
| `sbc.model` | string |
| `sbc.serial` | string |
| `sbc.uptime` | number |

## sensors

| Field | Type |
| --- | --- |
| `sensors` | Sensors |
| `sensors.analog` | AnalogSensor[] |
| `sensors.analog[].beta` | number |
| `sensors.analog[].c` | number |
| `sensors.analog[].highReading` | number |
| `sensors.analog[].lastReading` | number |
| `sensors.analog[].lowReading` | number |
| `sensors.analog[].name` | string |
| `sensors.analog[].offsetAdj` | number |
| `sensors.analog[].port` | string |
| `sensors.analog[].r25` | number |
| `sensors.analog[].rRef` | number |
| `sensors.analog[].slopeAdj` | number |
| `sensors.analog[].state` | TemperatureError: ok, shortCircuit, shortToVcc, shortToGround, openCircuit, timeout, ioError, hardwareError, notReady, invalidOutputNumber, busBusy, badResponse, unknownPort, notInitialised, unknownSensor, overOrUnderVoltage, badVref, badVssa, unknownError |
| `sensors.analog[].type` | AnalogSensorType: thermistor, pt1000, rtdmax31865, thermocouplemax31855, thermocouplemax31856, linearanalog, dht21, dht22, dhthumidity, bme280, bmepressure, bmehumidity, currentloop, ads131.chan0.u, ads131.chan0.b, ads131.chan1, mcutemp, drivers, drivers-duex, unknown |
| `sensors.endstops` | Endstop[] |
| `sensors.endstops[].highEnd` | boolean |
| `sensors.endstops[].probe` | number |
| `sensors.endstops[].triggered` | boolean |
| `sensors.endstops[].type` | EndstopType: inputPin, zProbeAsEndstop, motorStallAny, motorStallIndividual, unknown |
| `sensors.filamentMonitors` | FilamentMonitor[] |
| `sensors.filamentMonitors[].avgPercentage` | number |
| `sensors.filamentMonitors[].calibrated` | PulsedFilamentMonitorCalibrated |
| `sensors.filamentMonitors[].calibrated.mmPerPulse` | number |
| `sensors.filamentMonitors[].calibrated.percentMax` | number |
| `sensors.filamentMonitors[].calibrated.percentMin` | number |
| `sensors.filamentMonitors[].calibrated.totalDistance` | number |
| `sensors.filamentMonitors[].configured` | PulsedFilamentMonitorConfigured |
| `sensors.filamentMonitors[].configured.mmPerPulse` | number |
| `sensors.filamentMonitors[].configured.percentMax` | number |
| `sensors.filamentMonitors[].configured.percentMin` | number |
| `sensors.filamentMonitors[].configured.sampleDistance` | number |
| `sensors.filamentMonitors[].enableMode` | FilamentMonitorEnableMode |
| `sensors.filamentMonitors[].enabled` | boolean |
| `sensors.filamentMonitors[].lastPercentage` | number |
| `sensors.filamentMonitors[].maxPercentage` | number |
| `sensors.filamentMonitors[].minPercentage` | number |
| `sensors.filamentMonitors[].position` | number |
| `sensors.filamentMonitors[].status` | FilamentMonitorStatus: noMonitor, ok, noDataReceived, noFilament, tooLittleMovement, tooMuchMovement, sensorError |
| `sensors.filamentMonitors[].totalExtrusion` | number |
| `sensors.filamentMonitors[].type` | FilamentMonitorType: simple, laser, pulsed, rotatingMagnet, unknown |
| `sensors.gpIn` | GpInputPort[] |
| `sensors.gpIn[].value` | number |
| `sensors.probes` | Probe[] |
| `sensors.probes[].calibA` | number |
| `sensors.probes[].calibB` | number |
| `sensors.probes[].calibrationTemperature` | number |
| `sensors.probes[].deployedByUser` | boolean |
| `sensors.probes[].disablesHeaters` | boolean |
| `sensors.probes[].diveHeight` | number |
| `sensors.probes[].diveHeights` | number[] |
| `sensors.probes[].isCalibrated` | boolean |
| `sensors.probes[].lastStopHeight` | number |
| `sensors.probes[].maxProbeCount` | number |
| `sensors.probes[].measuredHeight` | number |
| `sensors.probes[].offsets` | number[] |
| `sensors.probes[].recoveryTime` | number |
| `sensors.probes[].scanCoefficients` | number[] |
| `sensors.probes[].speeds` | number[] |
| `sensors.probes[].temperatureCoefficients` | number[] |
| `sensors.probes[].threshold` | number |
| `sensors.probes[].tolerance` | number |
| `sensors.probes[].touchMode` | ProbeTouchMode |
| `sensors.probes[].touchMode.active` | boolean |
| `sensors.probes[].touchMode.sensitivity` | number |
| `sensors.probes[].touchMode.threshold` | number |
| `sensors.probes[].touchMode.triggerHeight` | number |
| `sensors.probes[].travelSpeed` | number |
| `sensors.probes[].triggerHeight` | number |
| `sensors.probes[].type` | ProbeType |
| `sensors.probes[].value` | number[] |

## spindles

| Field | Type |
| --- | --- |
| `spindles` | Spindle[] |
| `spindles[].active` | number |
| `spindles[].canReverse` | boolean |
| `spindles[].current` | number |
| `spindles[].frequency` | number |
| `spindles[].idlePwm` | number |
| `spindles[].max` | number |
| `spindles[].maxPwm` | number |
| `spindles[].min` | number |
| `spindles[].minPwm` | number |
| `spindles[].state` | SpindleState: unconfigured, stopped, forward, reverse |
| `spindles[].type` | SpindleType: enaDir, fwdRev |

## state

| Field | Type |
| --- | --- |
| `state` | State |
| `state.atxPower` | boolean |
| `state.atxPowerPort` | string |
| `state.beep` | BeepRequest |
| `state.beep.duration` | number |
| `state.beep.frequency` | number |
| `state.currentTool` | number |
| `state.deferredPowerDown` | boolean |
| `state.displayMessage` | string |
| `state.gpOut` | GpOutputPort[] |
| `state.gpOut[].freq` | number |
| `state.gpOut[].pwm` | number |
| `state.laserPwm` | number |
| `state.logFile` | string |
| `state.logLevel` | LogLevel: debug, info, warn, off |
| `state.machineMode` | MachineMode: FFF, CNC, Laser |
| `state.macroRestarted` | boolean |
| `state.messageBox` | MessageBox |
| `state.messageBox.axisControls` | number |
| `state.messageBox.cancelButton` | boolean |
| `state.messageBox.choices` | string[] |
| `state.messageBox.default` | any |
| `state.messageBox.max` | number |
| `state.messageBox.message` | string |
| `state.messageBox.min` | number |
| `state.messageBox.mode` | MessageBoxMode |
| `state.messageBox.seq` | number |
| `state.messageBox.timeout` | number |
| `state.messageBox.title` | string |
| `state.msUpTime` | number |
| `state.nextTool` | number |
| `state.pluginsStarted` | boolean |
| `state.powerFailScript` | string |
| `state.previousTool` | number |
| `state.restorePoints` | RestorePoint[] |
| `state.restorePoints[].coords` | number[] |
| `state.restorePoints[].extruderPos` | number |
| `state.restorePoints[].fanPwm` | number |
| `state.restorePoints[].feedRate` | number |
| `state.restorePoints[].ioBits` | number |
| `state.restorePoints[].laserPwm` | number |
| `state.restorePoints[].toolNumber` | number |
| `state.startupError` | StartupError |
| `state.startupError.file` | string |
| `state.startupError.line` | number |
| `state.startupError.message` | string |
| `state.status` | MachineStatus: disconnected, starting, updating, off, halted, pausing, paused, resuming, cancelling, processing, simulating, busy, changingTool, idle |
| `state.thisInput` | number |
| `state.time` | string |
| `state.upTime` | number |

## tools

| Field | Type |
| --- | --- |
| `tools` | Tool[] |
| `tools[].active` | number[] |
| `tools[].axes` | number[][] |
| `tools[].extruders` | number[] |
| `tools[].fans` | number[] |
| `tools[].feedForward` | number[] |
| `tools[].feedForwardAdvance` | number |
| `tools[].feedForwardPwm` | number[] |
| `tools[].feedForwardTemp` | number[] |
| `tools[].filamentExtruder` | number |
| `tools[].heaters` | number[] |
| `tools[].isRetracted` | boolean |
| `tools[].mix` | number[] |
| `tools[].name` | string |
| `tools[].number` | number |
| `tools[].offsets` | number[] |
| `tools[].offsetsProbed` | boolean |
| `tools[].retraction` | ToolRetraction |
| `tools[].retraction.extraRestart` | number |
| `tools[].retraction.length` | number |
| `tools[].retraction.speed` | number |
| `tools[].retraction.unretractSpeed` | number |
| `tools[].retraction.zHop` | number |
| `tools[].spindle` | number |
| `tools[].spindleRpm` | number |
| `tools[].standby` | number[] |
| `tools[].state` | ToolState: off, active, standby |

## volumes

| Field | Type |
| --- | --- |
| `volumes` | Volume[] |
| `volumes[].capacity` | number |
| `volumes[].freeSpace` | number |
| `volumes[].mounted` | boolean |
| `volumes[].name` | string |
| `volumes[].openFiles` | number |
| `volumes[].path` | string |
| `volumes[].speed` | number |
