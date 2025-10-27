#include "EditorEngine.h"

ENGINE_API UEngin* GEngine = NULL;

/**
 * Implements the main engine loop.	
 */

 /**
  * kwakkh
  * 언리얼 엔진에서 메인 루프를 진행하기 위해 별도로 존재하는 클래스.
  * - 엔진의 메인 Tick을 관리하는 것과 동일 
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
    // kwakkh : 메인 루프 초기화의 로직들이 담겨있는 함수
    // - 명령줄을 분석, GIsEditor 설정 등을 담당
    int32 PreInit(const TCHAR* CmdLine)
    {
        //...
    }

    /** Advances the main loop. */
     // 9 - Foundation - Entry - FEngineLoop::Tick
     // kwakkh : 엔진의 메인 루프를 담당하는 함수
	virtual void Tick() override
    {
        // set FApp::CurrentTime, FApp::DeltaTime and potentially wait to enforce max tick rate
		{
            // ...

            /**
             * kwakkh
             * CurrentTime과 DeltaTime을 제공하는 함수
             * - 그 뿐만이 아니라 여러 스레드에 대해서 동기화 진행
             * ex)
             * GameThread와 RenderThread가 병렬로 실행되고 있을 때, 
             * GameThread가 먼저 끝났다고 GameThread가 먼저 실행되면 엔진 전체의 동기화가 꺠져버리기 때문에 
             * FrameNumber와 같은 변수를 컨트롤해 서로를 동기화 한다.
             */
			GEngine->UpdateTimeAndHandleMaxTickRate();
			
            // ...
		}

        // main game engine tick (world, game objects, etc.)
        /**
         * kwakkh
         * 게임의 메인 루프
         * - Lyra를 예로들면, ULyraEditorEngine -> UUnrealEdEngine -> UEditorEngine 순으로 실행
         */
		GEngine->Tick(FApp::GetDeltaTime(), bIdleMode);
    }
}