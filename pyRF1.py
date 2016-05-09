"""
pyRF1.py - Defines the shared memory map structures for rFactor
	as exported by rFactorSharedMemoryMap.dll
by Dan Allongo (daniel.s.allongo@gmail.com)

Release History:
2016-05-09: Initial release
"""

from ctypes import *

class rfStruct(Structure):
	_pack_ = 1

class rfGamePhaseEnum(rfStruct):
	_fields_ = [('garage', c_int),
				('warmUp', c_int),
				('gridWalk', c_int),
				('formation', c_int),
				('countdown', c_int),
				('greenFlag', c_int),
				('fullCourseYellow', c_int),
				('sessionStopped', c_int),
				('sessionOver', c_int)]
rfGamePhase = rfGamePhaseEnum(0, 1, 2, 3, 4, 5, 6, 7, 8)

class rfYellowFlagStateEnum(rfStruct):
	_fields_ = [('invalid', c_int),
				('noFlag', c_int),
				('pending', c_int),
				('pitClosed', c_int),
				('pitLeadLap', c_int),
				('pitOpen', c_int),
				('lastLap', c_int),
				('resume', c_int),
				('raceHalt', c_int)]
rfYellowFlagState = rfYellowFlagStateEnum(-1, 0, 1, 2, 3, 4, 5, 6, 7)

class rfSurfaceTypeEnum(rfStruct):
	_fields_ = [('dry', c_int),
				('wet', c_int),
				('grass', c_int),
				('dirt', c_int),
				('gravel', c_int),
				('kerb', c_int)]
rfSurfaceType = rfSurfaceTypeEnum(0, 1, 2, 3, 4, 5)

class rfSectorEnum(rfStruct):
	_fields_ = [('sector3', c_int),
				('sector1', c_int),
				('sector2', c_int)]
rfSector = rfSectorEnum(0, 1, 2)

class rfFinishStatusEnum(rfStruct):
	_fields_ = [('none', c_int),
				('finished', c_int),
				('dnf', c_int),
				('dq', c_int)]
rfFinishStatus = rfFinishStatusEnum(0, 1, 2, 3)

class rfControlEnum(rfStruct):
	_fields_ = [('nobody', c_int),
				('player', c_int),
				('ai', c_int),
				('remote', c_int),
				('replay', c_int)]
rfControl = rfControlEnum(-1, 0, 1, 2, 3)

class rfWheelIndexEnum(rfStruct):
	_fields_ = [('frontLeft', c_int),
				('frontRight', c_int),
				('rearLeft', c_int),
				('rearRight', c_int)]
rfWheelIndex = rfWheelIndexEnum(0, 1, 2, 3)

class rfVec3(rfStruct):
	_fields_ = [('x', c_float),
				('y', c_float),
				('z', c_float)]

class rfWheel(rfStruct):
	_fields_ = [('rotation', c_float),
				('suspensionDeflection', c_float),
				('rideHeight', c_float),
				('tireLoad', c_float),
				('lateralForce', c_float),
				('gripFract', c_float),
				('brakeTemp', c_float),
				('pressure', c_float),
				('temperature', c_float*3),
				('wear', c_float),
				('terrainName', c_char*16),
				('surfaceType', c_int8),
				('flat', c_bool),
				('detached', c_bool)]

class rfVehicleInfo(rfStruct):
	_fields_ = [('driverName', c_char*32),
				('vehicleName', c_char*64),
				('totalLaps', c_short),
				('sector', c_int8),
				('finishStatus', c_int8),
				('lapDist', c_float),
				('pathLateral', c_float),
				('trackEdge', c_float),
				('bestSector1', c_float),
				('bestSector2', c_float),
				('bestLapTime', c_float),
				('lastSector1', c_float),
				('lastSector2', c_float),
				('lastLapTime', c_float),
				('curSector1', c_float),
				('curSector2', c_float),
				('numPitstops', c_short),
				('numPenalties', c_short),
				('isPlayer', c_bool),
				('control', c_int8),
				('inPits', c_bool),
				('place', c_int8),
				('vehicleClass', c_char*32),
				('timeBehindNext', c_float),
				('lapsBehindNext', c_long),
				('timeBehindLeader', c_float),
				('lapsBehindLeader', c_long),
				('lapStartET', c_float),
				('pos', rfVec3),
				('speed', c_float)]

class rfShared(rfStruct):
	_fields_ = [('deltaTime', c_float),
				('lapNumber', c_long),
				('lapStartET', c_float),
				('vehicleName', c_char*64),
				('trackName', c_char*64),
				('pos', rfVec3),
				('localVel', rfVec3),
				('localAccel', rfVec3),
				('oriX', rfVec3),
				('oriY', rfVec3),
				('oriZ', rfVec3),
				('localRot', rfVec3),
				('localRotAccel', rfVec3),
				('speed', c_float),
				('gear', c_long),
				('engineRPM', c_float),
				('engineWaterTemp', c_float),
				('engineOilTemp', c_float),
				('clutchRPM', c_float),
				('unfilteredThrottle', c_float),
				('unfilteredBrake', c_float),
				('unfilteredSteering', c_float),
				('unfilteredClutch', c_float),
				('steeringArmForce', c_float),
				('fuel', c_float),
				('engineMaxRPM', c_float),
				('scheduledStops', c_int8),
				('overheating', c_bool),
				('detached', c_bool),
				('dentSeverity', c_int8*8),
				('lastImpactET', c_float),
				('lastImpactMagnitude', c_float),
				('lastImpactPos', rfVec3),
				('wheel', rfWheel*4),
				('session', c_long),
				('currentET', c_float),
				('endET', c_float),
				('maxLaps', c_long),
				('lapDist', c_float),
				('numVehicles', c_long),
				('gamePhase', c_int8),
				('yellowFlagState', c_int8),
				('sectorFlag', c_int8*3),
				('startLight', c_int8),
				('numRedLights', c_int8),
				('inRealtime', c_bool),
				('playerName', c_char*32),
				('plrFileName', c_char*64),
				('ambientTemp', c_float),
				('trackTemp', c_float),
				('wind', rfVec3),
				('vehicle', rfVehicleInfo*128)]

def mps_to_mph(m):
	return (m * 2.23694)

def mps_to_kph(m):
	return (m * 3.6)

def kpa_to_psi(k):
	return (k * 0.145038)

def c_to_f(c):
	return (c * 1.8) + 32

def l_to_g(l):
	return (l * 0.264172)

rfMapTag = '$rFactorShared$'
rfMapHandle = None

if __name__ == '__main__':
	from mmap import mmap

	rfMapHandle = mmap(fileno=0, length=sizeof(rfShared), tagname=rfMapTag)
	rfMapHandle.seek(0)
	smm = rfShared.from_buffer_copy(rfMapHandle)
	print "{0}".format(smm.numVehicles)
	for d in smm.vehicle:
		print d.driverName, d.vehicleName, d.vehicleClass
