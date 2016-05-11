/*
 rFactorSharedMemoryMap.cpp
 by Dan Allongo (daniel.s.allongo@gmail.com)

 Based on the ISI sample code found at http://rfactor.net/web/rf1/devcorner/
 Provides a basic memory map export of the telemetry and scoring data
 The export is nearly 1:1 with redundant/gratuitous data removed from the 
 vehicle info array and the addition of vehicle speed being pre-calculated
 since everyone needs that anyway. Position, rotation, and orientation are
 interpolated in between scoring updates (every 0.5 seconds). True raw values 
 are given when deltaTime == 0.
*/

#include "rFactorSharedMemoryMap.hpp"
#include <math.h>
#include <stdio.h>


// plugin information
unsigned g_uPluginID          = 0;
char     g_szPluginName[]     = PLUGIN_NAME;
unsigned g_uPluginVersion     = 002;
unsigned g_uPluginObjectCount = 1;
InternalsPluginInfo g_PluginInfo;

// interface to plugin information
extern "C" __declspec(dllexport)
const char* __cdecl GetPluginName() { return g_szPluginName; }

extern "C" __declspec(dllexport)
unsigned __cdecl GetPluginVersion() { return g_uPluginVersion; }

extern "C" __declspec(dllexport)
unsigned __cdecl GetPluginObjectCount() { return g_uPluginObjectCount; }

// get the plugin-info object used to create the plugin.
extern "C" __declspec(dllexport)
PluginObjectInfo* __cdecl GetPluginObjectInfo( const unsigned uIndex ) {
  switch(uIndex) {
    case 0:
      return  &g_PluginInfo;
    default:
      return 0;
  }
}


// InternalsPluginInfo class

InternalsPluginInfo::InternalsPluginInfo() {
  // put together a name for this plugin
  sprintf( m_szFullName, "%s - %s", g_szPluginName, InternalsPluginInfo::GetName() );
}

const char*    InternalsPluginInfo::GetName()     const { return SharedMemoryMapPlugin::GetName(); }
const char*    InternalsPluginInfo::GetFullName() const { return m_szFullName; }
const char*    InternalsPluginInfo::GetDesc()     const { return g_szPluginName; }
const unsigned InternalsPluginInfo::GetType()     const { return SharedMemoryMapPlugin::GetType(); }
const char*    InternalsPluginInfo::GetSubType()  const { return SharedMemoryMapPlugin::GetSubType(); }
const unsigned InternalsPluginInfo::GetVersion()  const { return SharedMemoryMapPlugin::GetVersion(); }
void*          InternalsPluginInfo::Create()      const { return new SharedMemoryMapPlugin(); }


// InternalsPlugin class

const char SharedMemoryMapPlugin::m_szName[] = PLUGIN_NAME;
const char SharedMemoryMapPlugin::m_szSubType[] = "Internals";
const unsigned SharedMemoryMapPlugin::m_uID = 1;
const unsigned SharedMemoryMapPlugin::m_uVersion = 3;  // set to 3 for InternalsPluginV3 functionality and added graphical and vehicle info

PluginObjectInfo *SharedMemoryMapPlugin::GetInfo() {
  return &g_PluginInfo;
}

void SharedMemoryMapPlugin::Startup() {
	// init handle and try to create, read if existing
	hMap = INVALID_HANDLE_VALUE;
	hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(rfShared), TEXT(RF_SHARED_MEMORY_NAME));
	if (hMap == NULL) {
		if (GetLastError() == (DWORD)183) {
			hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, TEXT(RF_SHARED_MEMORY_NAME));
			if (hMap == NULL) {
				// unable to create or read existing
				mapped = FALSE;
				return;
			}
		}
	}
	pBuf = (rfShared*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(rfShared));
	if (pBuf == NULL) {
		// failed to map memory buffer
		CloseHandle(hMap);
		mapped = FALSE;
		return;
	}
	mapped = TRUE;
	if (mapped) {
		memset(pBuf, 0, sizeof(rfShared));
	}
	return;
}

void SharedMemoryMapPlugin::Shutdown() {
	// release buffer and close handle
	if (mapped) {
		memset(pBuf, 0, sizeof(rfShared));
	}
	if (pBuf) {
		UnmapViewOfFile(pBuf);
	}
	if (hMap) {
		CloseHandle(hMap);
	}
	mapped = FALSE;
}

void SharedMemoryMapPlugin::StartSession() {
	// zero-out buffer at start of session
	if (mapped) {
		memset(pBuf, 0, sizeof(rfShared));
	}
	cLastTelemUpdate = 0;
	cLastScoringUpdate = 0;
	cDelta = 0;
	inSession = TRUE;
}

void SharedMemoryMapPlugin::EndSession() {
	// zero-out buffer at end of session
	StartSession();
	inSession = FALSE;
}

void SharedMemoryMapPlugin::EnterRealtime() {
	inRealtime = TRUE;
}

void SharedMemoryMapPlugin::ExitRealtime() {
	inRealtime = FALSE;
}

void SharedMemoryMapPlugin::UpdateTelemetry( const TelemInfoV2 &info ) {
	if (mapped) {
		// update clock delta
		if (!cLastTelemUpdate) {
			cLastTelemUpdate = clock();
		}
		cDelta = (float)(clock() - cLastTelemUpdate) / (float)CLOCKS_PER_SEC;
		cLastTelemUpdate = clock();

		// TelemInfoBase
		pBuf->deltaTime = (float)(clock() - cLastScoringUpdate) / (float)CLOCKS_PER_SEC;
		pBuf->lapNumber = info.mLapNumber;
		pBuf->lapStartET = info.mLapStartET;
		strcpy(pBuf->vehicleName, info.mVehicleName);
		strcpy(pBuf->trackName, info.mTrackName);
		pBuf->pos = { info.mPos.x, info.mPos.y, info.mPos.z };
		pBuf->localVel = { info.mLocalVel.x, info.mLocalVel.y, info.mLocalVel.z };
		pBuf->localAccel = { info.mLocalAccel.x, info.mLocalAccel.y, info.mLocalAccel.z };
		pBuf->oriX = { info.mOriX.x, info.mOriX.y, info.mOriX.z };
		pBuf->oriY = { info.mOriY.x, info.mOriY.y, info.mOriY.z };
		pBuf->oriZ = { info.mOriZ.x, info.mOriZ.y, info.mOriZ.z };
		pBuf->localRot = { info.mLocalRot.x, info.mLocalRot.y, info.mLocalRot.z };
		pBuf->localRotAccel = { info.mLocalRotAccel.x, info.mLocalRotAccel.y, info.mLocalRotAccel.z };
		pBuf->speed = sqrtf((info.mLocalVel.x * info.mLocalVel.x) +
			(info.mLocalVel.y * info.mLocalVel.y) +
			(info.mLocalVel.z * info.mLocalVel.z));
		pBuf->gear = info.mGear;
		pBuf->engineRPM = info.mEngineRPM;
		pBuf->engineWaterTemp = info.mEngineWaterTemp;
		pBuf->engineOilTemp = info.mEngineOilTemp;
		pBuf->clutchRPM = info.mClutchRPM;
		pBuf->unfilteredThrottle = info.mUnfilteredThrottle;
		pBuf->unfilteredBrake = info.mUnfilteredBrake;
		pBuf->unfilteredSteering = info.mUnfilteredSteering;
		pBuf->unfilteredClutch = info.mUnfilteredClutch;
		pBuf->steeringArmForce = info.mSteeringArmForce;

		// TelemInfoV2
		pBuf->fuel = info.mFuel;
		pBuf->engineMaxRPM = info.mEngineMaxRPM;
		pBuf->scheduledStops = info.mScheduledStops;
		pBuf->overheating = info.mOverheating;
		pBuf->detached = info.mDetached;
		for (int i = 0; i < 8; i++) {
			pBuf->dentSeverity[i] = info.mDentSeverity[i];
		}
		pBuf->lastImpactET = info.mLastImpactET;
		pBuf->lastImpactMagnitude = info.mLastImpactMagnitude;
		pBuf->lastImpactPos = { info.mLastImpactPos.x, info.mLastImpactPos.y, info.mLastImpactPos.z };

		for (int i = 0; i < 4; i++) {
			// TelemWheel
			pBuf->wheel[i].rotation = info.mWheel[i].mRotation;
			pBuf->wheel[i].suspensionDeflection = info.mWheel[i].mSuspensionDeflection;
			pBuf->wheel[i].rideHeight = info.mWheel[i].mRideHeight;
			pBuf->wheel[i].tireLoad = info.mWheel[i].mTireLoad;
			pBuf->wheel[i].lateralForce = info.mWheel[i].mLateralForce;
			pBuf->wheel[i].gripFract = info.mWheel[i].mGripFract;
			pBuf->wheel[i].brakeTemp = info.mWheel[i].mBrakeTemp;
			pBuf->wheel[i].pressure = info.mWheel[i].mPressure;
			for (int j = 0; j < 3; j++) {
				pBuf->wheel[i].temperature[j] = info.mWheel[i].mTemperature[j];
			}

			//TelemWheelV2
			pBuf->wheel[i].wear = info.mWheel[i].mWear;
			strcpy(pBuf->wheel[i].terrainName, info.mWheel[i].mTerrainName);
			pBuf->wheel[i].surfaceType = info.mWheel[i].mSurfaceType;
			pBuf->wheel[i].flat = info.mWheel[i].mFlat;
			pBuf->wheel[i].detached = info.mWheel[i].mDetached;
		}

		// interpolation of scoring info
		// ScoringInfoBase
		pBuf->currentET += cDelta;

		// ScoringInfoV2
		pBuf->inRealtime = inRealtime;

		for (int i = 0; i < pBuf->numVehicles; i++) {
			// VehicleScoringInfoV2
			// applying acceleration only seems to make the interpolation worse since acceleration changes so quickly
			//pBuf->vehicle[i].localRot.x += pBuf->vehicle[i].localRotAccel.x * cDelta;
			//pBuf->vehicle[i].localRot.y += pBuf->vehicle[i].localRotAccel.y * cDelta;
			//pBuf->vehicle[i].localRot.z += pBuf->vehicle[i].localRotAccel.z * cDelta;
			//pBuf->vehicle[i].localVel.x += pBuf->vehicle[i].localAccel.x * cDelta;
			//pBuf->vehicle[i].localVel.y += pBuf->vehicle[i].localAccel.y * cDelta;
			//pBuf->vehicle[i].localVel.z += pBuf->vehicle[i].localAccel.z * cDelta;
			pBuf->vehicle[i].pos.x += ( (pBuf->vehicle[i].oriX.x * pBuf->vehicle[i].localVel.x) + 
										(pBuf->vehicle[i].oriX.y * pBuf->vehicle[i].localVel.y) + 
										(pBuf->vehicle[i].oriX.z * pBuf->vehicle[i].localVel.z) ) * cDelta;
			pBuf->vehicle[i].pos.y += ( (pBuf->vehicle[i].oriY.x * pBuf->vehicle[i].localVel.x) +
										(pBuf->vehicle[i].oriY.y * pBuf->vehicle[i].localVel.y) +
										(pBuf->vehicle[i].oriY.z * pBuf->vehicle[i].localVel.z) ) * cDelta;
			pBuf->vehicle[i].pos.z += ( (pBuf->vehicle[i].oriZ.x * pBuf->vehicle[i].localVel.x) +
										(pBuf->vehicle[i].oriZ.y * pBuf->vehicle[i].localVel.y) +
										(pBuf->vehicle[i].oriZ.z * pBuf->vehicle[i].localVel.z) ) * cDelta;
			// rotate and normalize orientation vectors (normalizing shouldn't be necessary if this is correct)
			rfVec3 oriX = { 0 };
			rfVec3 oriY = { 0 };
			rfVec3 oriZ = { 0 };
			float oriXlen = 0;
			rfVec3 wRot = { (oriX.x * pBuf->vehicle[i].localRot.x) + (oriX.y * pBuf->vehicle[i].localRot.y) + (oriX.z * pBuf->vehicle[i].localRot.z),
							(oriY.x * pBuf->vehicle[i].localRot.x) + (oriY.y * pBuf->vehicle[i].localRot.y) + (oriY.z * pBuf->vehicle[i].localRot.z), 
							(oriZ.x * pBuf->vehicle[i].localRot.x) + (oriZ.y * pBuf->vehicle[i].localRot.y) + (oriZ.z * pBuf->vehicle[i].localRot.z) };
			// rotate by z
			oriZ.x = pBuf->vehicle[i].oriX.x * cosf(wRot.z * cDelta) - pBuf->vehicle[i].oriX.y * sinf(wRot.z * cDelta);
			oriZ.y = pBuf->vehicle[i].oriX.x * sinf(wRot.z * cDelta) + pBuf->vehicle[i].oriX.y * cosf(wRot.z * cDelta);
			oriZ.z = pBuf->vehicle[i].oriX.z;
			// rotate by y
			oriY.x = oriZ.x * cosf(wRot.y * cDelta) + oriZ.z * sinf(wRot.y * cDelta);
			oriY.y = oriZ.y;
			oriY.z = oriZ.z * cosf(wRot.y * cDelta) - oriZ.x * sinf(wRot.y * cDelta);
			// rotate by x
			oriX.x = oriY.x;
			oriX.y = oriY.y * cosf(wRot.x * cDelta) - oriY.z * sinf(wRot.x * cDelta);
			oriX.z = oriY.y * sinf(wRot.x * cDelta) + oriY.z * cosf(wRot.x * cDelta);
			oriXlen = sqrtf(oriX.x * oriX.x + oriX.y * oriX.y + oriX.z * oriX.z);
			// rotate by z
			oriZ.x = pBuf->vehicle[i].oriY.x * cosf(wRot.z * cDelta) - pBuf->vehicle[i].oriY.y * sinf(wRot.z * cDelta);
			oriZ.y = pBuf->vehicle[i].oriY.x * sinf(wRot.z * cDelta) + pBuf->vehicle[i].oriY.y * cosf(wRot.z * cDelta);
			oriZ.z = pBuf->vehicle[i].oriY.z;
			// rotate by y
			oriY.x = oriZ.x * cosf(wRot.y * cDelta) + oriZ.z * sinf(wRot.y * cDelta);
			oriY.y = oriZ.y;
			oriY.z = oriZ.z * cosf(wRot.y * cDelta) - oriZ.x * sinf(wRot.y * cDelta);
			// rotate by x
			oriX.x = oriY.x;
			oriX.y = oriY.y * cosf(wRot.x * cDelta) - oriY.z * sinf(wRot.x * cDelta);
			oriX.z = oriY.y * sinf(wRot.x * cDelta) + oriY.z * cosf(wRot.x * cDelta);
			oriXlen = sqrtf(oriX.x * oriX.x + oriX.y * oriX.y + oriX.z * oriX.z);
			// rotate by z
			oriZ.x = pBuf->vehicle[i].oriZ.x * cosf(wRot.z * cDelta) - pBuf->vehicle[i].oriZ.y * sinf(wRot.z * cDelta);
			oriZ.y = pBuf->vehicle[i].oriZ.x * sinf(wRot.z * cDelta) + pBuf->vehicle[i].oriZ.y * cosf(wRot.z * cDelta);
			oriZ.z = pBuf->vehicle[i].oriZ.z;
			// rotate by y
			oriY.x = oriZ.x * cosf(wRot.y * cDelta) + oriZ.z * sinf(wRot.y * cDelta);
			oriY.y = oriZ.y;
			oriY.z = oriZ.z * cosf(wRot.y * cDelta) - oriZ.x * sinf(wRot.y * cDelta);
			// rotate by x
			oriX.x = oriY.x;
			oriX.y = oriY.y * cosf(wRot.x * cDelta) - oriY.z * sinf(wRot.x * cDelta);
			oriX.z = oriY.y * sinf(wRot.x * cDelta) + oriY.z * cosf(wRot.x * cDelta);
			oriXlen = sqrtf(oriX.x * oriX.x + oriX.y * oriX.y + oriX.z * oriX.z);
			pBuf->vehicle[i].oriZ = { oriX.x / oriXlen, oriX.y / oriXlen, oriX.z / oriXlen };
			// interpolate speed
			pBuf->vehicle[i].speed = sqrtf( (pBuf->vehicle[i].localVel.x * pBuf->vehicle[i].localVel.x) +
											(pBuf->vehicle[i].localVel.y * pBuf->vehicle[i].localVel.y) +
											(pBuf->vehicle[i].localVel.z * pBuf->vehicle[i].localVel.z) );

			// VehicleScoringInfo
			pBuf->vehicle[i].lapDist -= pBuf->vehicle[i].localVel.z * cDelta;
		}
	}
}

void SharedMemoryMapPlugin::UpdateScoring( const ScoringInfoV2 &info ) {
	if (mapped) {
		cLastScoringUpdate = clock();
		// ScoringInfoBase
		pBuf->session = info.mSession;
		pBuf->currentET = info.mCurrentET;
		pBuf->endET = info.mEndET;
		pBuf->maxLaps = info.mMaxLaps;
		pBuf->lapDist = info.mLapDist;
		pBuf->numVehicles = info.mNumVehicles;

		// ScoringInfoV2
		pBuf->gamePhase = info.mGamePhase;
		pBuf->yellowFlagState = info.mYellowFlagState;
		for (int i = 0; i < 3; i++) {
			pBuf->sectorFlag[i] = info.mSectorFlag[i];
		}
		pBuf->startLight = info.mStartLight;
		pBuf->numRedLights = info.mNumRedLights;
		pBuf->inRealtime = inRealtime;
		strcpy(pBuf->playerName, info.mPlayerName);
		strcpy(pBuf->plrFileName, info.mPlrFileName);
		pBuf->ambientTemp = info.mAmbientTemp;
		pBuf->trackTemp = info.mTrackTemp;
		pBuf->wind = { info.mWind.x, info.mWind.y, info.mWind.z };

		pBuf->vehicle[128] = { 0 };
		for (int i = 0; i < info.mNumVehicles; i++) {
			// VehicleScoringInfo
			strcpy(pBuf->vehicle[i].driverName, info.mVehicle[i].mDriverName);
			strcpy(pBuf->vehicle[i].vehicleName, info.mVehicle[i].mVehicleName);
			pBuf->vehicle[i].totalLaps = info.mVehicle[i].mTotalLaps;
			pBuf->vehicle[i].sector = info.mVehicle[i].mSector;
			pBuf->vehicle[i].finishStatus = info.mVehicle[i].mFinishStatus;
			pBuf->vehicle[i].lapDist = info.mVehicle[i].mLapDist;
			pBuf->vehicle[i].pathLateral = info.mVehicle[i].mPathLateral;
			pBuf->vehicle[i].trackEdge = info.mVehicle[i].mTrackEdge;
			pBuf->vehicle[i].bestSector1 = info.mVehicle[i].mBestSector1;
			pBuf->vehicle[i].bestSector2 = info.mVehicle[i].mBestSector2;
			pBuf->vehicle[i].bestLapTime = info.mVehicle[i].mBestLapTime;
			pBuf->vehicle[i].lastSector1 = info.mVehicle[i].mLastSector1;
			pBuf->vehicle[i].lastSector2 = info.mVehicle[i].mLastSector2;
			pBuf->vehicle[i].lastLapTime = info.mVehicle[i].mLastLapTime;
			pBuf->vehicle[i].curSector1 = info.mVehicle[i].mCurSector1;
			pBuf->vehicle[i].curSector2 = info.mVehicle[i].mCurSector2;
			pBuf->vehicle[i].numPitstops = info.mVehicle[i].mNumPitstops;
			pBuf->vehicle[i].numPenalties = info.mVehicle[i].mNumPenalties;

			// VehicleScoringInfoV2
			pBuf->vehicle[i].isPlayer = info.mVehicle[i].mIsPlayer;
			pBuf->vehicle[i].control = info.mVehicle[i].mControl;
			pBuf->vehicle[i].inPits = info.mVehicle[i].mInPits;
			pBuf->vehicle[i].place = info.mVehicle[i].mPlace;
			strcpy(pBuf->vehicle[i].vehicleClass, info.mVehicle[i].mVehicleClass);
			pBuf->vehicle[i].timeBehindNext = info.mVehicle[i].mTimeBehindNext;
			pBuf->vehicle[i].lapsBehindNext = info.mVehicle[i].mLapsBehindNext;
			pBuf->vehicle[i].timeBehindLeader = info.mVehicle[i].mTimeBehindLeader;
			pBuf->vehicle[i].lapsBehindLeader = info.mVehicle[i].mLapsBehindLeader;
			pBuf->vehicle[i].lapStartET = info.mVehicle[i].mLapStartET;
			pBuf->vehicle[i].pos = { info.mVehicle[i].mPos.x, info.mVehicle[i].mPos.y, info.mVehicle[i].mPos.z };
			pBuf->vehicle[i].localVel = { info.mVehicle[i].mLocalVel.x, info.mVehicle[i].mLocalVel.y, info.mVehicle[i].mLocalVel.z };
			pBuf->vehicle[i].localAccel = { info.mVehicle[i].mLocalAccel.x, info.mVehicle[i].mLocalAccel.y, info.mVehicle[i].mLocalAccel.z };
			pBuf->vehicle[i].oriX = { info.mVehicle[i].mOriX.x, info.mVehicle[i].mOriX.y, info.mVehicle[i].mOriX.z };
			pBuf->vehicle[i].oriY = { info.mVehicle[i].mOriY.x, info.mVehicle[i].mOriY.y, info.mVehicle[i].mOriY.z };
			pBuf->vehicle[i].oriZ = { info.mVehicle[i].mOriZ.x, info.mVehicle[i].mOriZ.y, info.mVehicle[i].mOriZ.z };
			pBuf->vehicle[i].localRot = { info.mVehicle[i].mLocalRot.x, info.mVehicle[i].mLocalRot.y, info.mVehicle[i].mLocalRot.z };
			pBuf->vehicle[i].localRotAccel = { info.mVehicle[i].mLocalRotAccel.x, info.mVehicle[i].mLocalRotAccel.y, info.mVehicle[i].mLocalRotAccel.z };
			pBuf->vehicle[i].speed = sqrtf((info.mVehicle[i].mLocalVel.x * info.mVehicle[i].mLocalVel.x) +
				(info.mVehicle[i].mLocalVel.y * info.mVehicle[i].mLocalVel.y) +
				(info.mVehicle[i].mLocalVel.z * info.mVehicle[i].mLocalVel.z));
		}
	}
}
