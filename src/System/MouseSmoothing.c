// MOUSE SMOOTHING.C
// (C) 2020 Iliyas Jorio
// This file is part of Bugdom. https://github.com/jorio/bugdom

#include "game.h"
#include "mousesmoothing.h"

static const int kAccumulatorWindowNanoseconds = 10 * 1e6;	// 10 ms

#define DELTA_MOUSE_MAX_SNAPSHOTS 64

static bool gNeedInit = true;

typedef struct
{
	uint64_t timestamp;
	float dx;
	float dy;
} DeltaMouseSnapshot;

static struct
{
	DeltaMouseSnapshot snapshots[DELTA_MOUSE_MAX_SNAPSHOTS];
	int ringStart;
	int ringLength;
	float dxAccu;
	float dyAccu;
} gState;

//-----------------------------------------------------------------------------

static void PopOldestSnapshot(void)
{
	gState.dxAccu -= gState.snapshots[gState.ringStart].dx;
	gState.dyAccu -= gState.snapshots[gState.ringStart].dy;

	gState.ringStart = (gState.ringStart + 1) % DELTA_MOUSE_MAX_SNAPSHOTS;
	gState.ringLength--;

	if (gState.ringLength <= 0)
	{
		GAME_ASSERT(fabsf(gState.dxAccu) < EPS);
		GAME_ASSERT(fabsf(gState.dyAccu) < EPS);
		MouseSmoothing_ResetState();
	}
}

void MouseSmoothing_ResetState(void)
{
	gState.ringLength = 0;
	gState.ringStart = 0;
	gState.dxAccu = 0;
	gState.dyAccu = 0;
}

void MouseSmoothing_StartFrame(void)
{
	if (gNeedInit)
	{
		MouseSmoothing_ResetState();
		gNeedInit = false;
	}

	uint64_t now = SDL_GetTicksNS();
	uint64_t cutoffTimestamp = now - kAccumulatorWindowNanoseconds;

	// Purge old snapshots
	while (gState.ringLength > 0 &&
		   gState.snapshots[gState.ringStart].timestamp < cutoffTimestamp)
	{
		PopOldestSnapshot();
	}
}

void MouseSmoothing_OnMouseMotion(const SDL_MouseMotionEvent* motion)
{
	// ignore mouse input if user has alt-tabbed away from the game
	if (!(SDL_GetWindowFlags(gSDLWindow) & SDL_WINDOW_INPUT_FOCUS))
	{
		return;
	}

	if (gState.ringLength == DELTA_MOUSE_MAX_SNAPSHOTS)
	{
//		SDL_Log("%s: buffer full!!", __func__);
		PopOldestSnapshot();				// make room at start of ring buffer
	}

	int i = (gState.ringStart + gState.ringLength) % DELTA_MOUSE_MAX_SNAPSHOTS;
	gState.ringLength++;

	gState.snapshots[i].timestamp = motion->timestamp;
	gState.snapshots[i].dx = motion->xrel;
	gState.snapshots[i].dy = motion->yrel;

	gState.dxAccu += motion->xrel;
	gState.dyAccu += motion->yrel;
}

void MouseSmoothing_GetDelta(int* dxOut, int* dyOut)
{
	*dxOut = gState.dxAccu;
	*dyOut = gState.dyAccu;
}
