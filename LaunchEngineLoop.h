#include "EditorEngine.h"

ENGINE_API UEngin* GEngine = NULL;

/**
 * Implements the main engine loop.	
 */
class FEngineLoop
#if WITH_ENGINE
	: public IEngineLoop
#endif
{
public:
    /**
     * Pre-Initialize the main loop - parse command line, sets up GIsEditor, etc.
     */
    // 6 - Foundation - Entry - FEngineLoop::PreInit
    int32 PreInit(const TCHAR* CmdLine)
    {
        //...
    }

    /** Advances the main loop. */
     // 9 - Foundation - Entry - FEngineLoop::Tick
	virtual void Tick() override
    {
        // set FApp::CurrentTime, FApp::DeltaTime and potentially wait to enforce max tick rate
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_FEngineLoop_UpdateTimeAndHandleMaxTickRate);
			GEngine->UpdateTimeAndHandleMaxTickRate();
			GEngine->SetSimulationLatencyMarkerStart(CurrentFrameCounter);
		}

        // main game engine tick (world, game objects, etc.)
		GEngine->Tick(FApp::GetDeltaTime(), bIdleMode);
    }
}