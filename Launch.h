#include "LaunchEngineLoop.h"

/** FPlatformTime::Seconds()가 처음 초기화된 시간 (before main)
 * CoreGlobals.cpp 에 정의된 전역 변수
*/
double GStartTime = FPlatformTime::InitTiming();

// 엔진의 루프를 관장하고 있는 싱글톤 객체
FEngineLoop	GEngineLoop;

/** preinit the engine loop */
// 5 - Foundation - Entry - EnginePreInit
int32 EnginePreInit( const TCHAR* CmdLine )
{
	int32 ErrorLevel = GEngineLoop.PreInit( CmdLine );
	return( ErrorLevel );
}

// 7 - Foundation - Entry - EditorInit
int32 EditorInit(IEngineLoop& EngineLoop)
{
    //...
}

/** 
 * Ticks the engine loop 
 */
// 8 - Foundation - Entry - EngineTick
LAUNCH_API void EngineTick( void )
{
	GEngineLoop.Tick();
}

/**
 * Shuts down the engine
 */
LAUNCH_API void EngineExit( void )
{
	// Make sure this is set
	RequestEngineExit(TEXT("EngineExit() was called"));

	GEngineLoop.Exit();
}

/**
 * 이것은 예외 처리(SEH 등)로 감싸진(guarded) 정적(static) 메인 함수.
 * 이렇게 별도의 함수로 분리한 이유는 디버거가 연결되어 있는지 여부에 따라, 디버그 빌드와 릴리즈 빌드에서 각각 다른 방식으로 오류를 처리할 수 있게 하기 위함.
 */
// 4 - Foundation - Entry - GuardMain
int32 GuardedMain( const TCHAR* CmdLine )
{
	//....

#if !(UE_BUILD_SHIPPING)

	// "-waitforattach" 또는 "-WaitForDebugger"가 지정된 경우 시작을 중지하고 디버거가 연결될 때까지 기다렸다가 계속 진행

    /** kwakkh 
    * 명령어 인수를 "-waitforattach"로 전달하는 것은 엔진을 시작할 때 디버깅할 때 유용
    */
	const bool bWaitForDebuggerAndBreak = FParse::Param(CmdLine, TEXT("waitforattach")) || FParse::Param(CmdLine, TEXT("WaitForDebugger")); 
	const bool bWaitForDebugger = bWaitForDebuggerAndBreak || FParse::Param(CmdLine, TEXT("WaitForAttachNoBreak")) || FParse::Param(CmdLine, TEXT("WaitForDebuggerNoBreak"));
	
	if (bWaitForDebugger)
	{
        // kwakkh : 디버거가 연결되면 인루프에서 벗어나게 된다.
		while (!FPlatformMisc::IsDebuggerPresent())
		{
			FPlatformProcess::Sleep(0.1f);
		}

		if (bWaitForDebuggerAndBreak)
		{
            // kwakkh : 여기서 멈춤
            // - 시작 지점을 디버깅하는 데 매우 유용
			UE_DEBUG_BREAK();
		}
	}

#endif

	BootTimingPoint("DefaultMain");

	// Super early init code. DO NOT MOVE THIS ANYWHERE ELSE!
    /**
     * kwakkh
     * - CoreDelegates, CoreUObjectDelegates, WorldDelegates, ... 
     * - 이 Delegate Class들은 기억하기 좋음
     * - 언리얼 엔진은 위와 같이 위임자 클래스를 증명하는 형태로 코드를 주입하는 방법을 제공
     * - 여기서 언리얼 엔진의 시작 지점에 코드를 삽입할 수 있다
     */
	FCoreDelegates::GetPreMainInitDelegate().Broadcast();

	// make sure GEngineLoop::Exit() is always called.
    // kwakkh : 이 패턴은 언리얼 엔진에서 자주 사용
	struct EngineLoopCleanupGuard 
	{ 
		~EngineLoopCleanupGuard()
		{
			// Don't shut down the engine on scope exit when we are running embedded
			// because the outer application will take care of that.
			if (!GUELibraryOverrideSettings.bIsEmbedded)
			{
				EngineExit();
			}
		}
	} CleanupGuard;

    //...

	int32 ErrorLevel = EnginePreInit( CmdLine );

	// exit if PreInit failed.
	if ( ErrorLevel != 0 || IsEngineExitRequested() )
	{
		return ErrorLevel;
	}

	{
		FScopedSlowTask SlowTask(100, NSLOCTEXT("EngineInit", "EngineInit_Loading", "Loading..."));

		// EnginePreInit leaves 20% unused in its slow task.
		// Here we consume 80% immediately so that the percentage value on the splash screen doesn't change from one slow task to the next.
		// (Note, we can't include the call to EnginePreInit in this ScopedSlowTask, because the engine isn't fully initialized at that point)
		SlowTask.EnterProgressFrame(80);

		SlowTask.EnterProgressFrame(20);

#if WITH_EDITOR
		if (GIsEditor)
		{
            /**
             * kwakkh
             * - 우리가 집중하고 있는 것은 엔진 코드를 분석하기 위한 편집기 빌드
             */
			ErrorLevel = EditorInit(GEngineLoop);
		}
		else
#endif
		{
			ErrorLevel = EngineInit();
		}
	}

    // kwakkh : 경과 시간을 계산하는 패턴 (기억해 두면 좋음)
    // FPlatformTime::Seconds() 는 초 단위의 double 값이 나옮
    // GStartTime : FPlatformTime::Seconds()가 처음 초기화된 시간 (언리얼 시작 타임을 캐싱한 변수) 
	double EngineInitializationTime = FPlatformTime::Seconds() - GStartTime;
	
    //...

	// Don't tick if we're running an embedded engine - we rely on the outer
	// application ticking us instead.
	if (!GUELibraryOverrideSettings.bIsEmbedded)
	{
        // kwakkh : IsEngineExitRequested() == GIsRequestingExit
        // - main loop of the engine
		while( !IsEngineExitRequested() )
		{
			EngineTick();
		}
	}

	TRACE_BOOKMARK(TEXT("Tick loop end"));

#if WITH_EDITOR
	if( GIsEditor )
	{
		EditorExit();
	}
#endif
	return ErrorLevel;
}

/**
 * Windows Platform-Specific
 */

// 3 - Foundation - Entry - GuardedMainWrapper
LAUNCH_API int32 GuardedMainWrapper( const TCHAR* CmdLine )
{
	int32 ErrorLevel = 0;
	if ( GEnableInnerException )
	{
#if !PLATFORM_SEH_EXCEPTIONS_DISABLED
	 	__try
#endif
		{
			// Run the guarded code.
			ErrorLevel = GuardedMain( CmdLine );
		}
#if !PLATFORM_SEH_EXCEPTIONS_DISABLED
		__except( FPlatformMisc::GetCrashHandlingType() == ECrashHandlingType::Default ? (ReportCrash( GetExceptionInformation()), EXCEPTION_CONTINUE_SEARCH) : EXCEPTION_CONTINUE_SEARCH )
		{
			// Deliberately do nothing but avoid warning C6322: Empty _except block.
			(void)0;
		}
#endif
	}
	else
	{
		// Run the guarded code.
		ErrorLevel = GuardedMain( CmdLine );
	}
	return ErrorLevel;
}

// 2 - Foundation - Entry - LaunchWindowsStartup
AUNCH_API int32 LaunchWindowsStartup( HINSTANCE hInInstance, HINSTANCE hPrevInstance, char*, int32 nCmdShow, const TCHAR* CmdLine )
{
    int32 ErrorLevel = 0;

    if (!CmdLine)
    {
        /** kwakkh : 리얼 엔진은 또한 명령 라인을 자신만의 스타일로 변환하도록 처리 */
        CmdLine = ::GetCommandLineW();
        if (ProcessCommandLine())
        {
            CmdLine = *GSavedCommandLine;
        }
    }

    //...

    // Main 함수에서도 크래시가 날 수도 있다
    // 어쨋든 크래쉬 덤프를 줘야 한다.
    // 그것을 위한 아래의 코드들
    /** 구조적 예외 처리 (SEH, Structured Exception Handling)를 사용해 충돌을 처리
     * SEH가 OS(커널모드)에 의해 트리거 된다는 것
     * SEH의 작동 방식은 정적이지 않으며, MS와 같은 OS 제공업체에 의해 언제든지 변경될 수 있다.
     * - 관련 OS 보안 자체이기 때문
     * - 관심이 있다면 다음의 순서대로 파악해 보자
     * 1. IDTR 레지스터
     * 2. IDT (인터럽트 디스크립터 테이블)
     * 3. ISR (서비스 중단 루틴)
     * 4. 디스패치 중단
     * 5. 커널 패치 보호
     */
    __try
    {
        GIsGuarded = 1;
        // Run the guarded code.
        ErrorLevel = GuardedMainWrapper(CmdLine);
        GIsGuarded = 0;
    }
    /** kwakkh
     * 예외가 발생하면 여기서 확인
     * - 엔진에서 충돌이 발생하면 정상적으로 여기에서 충돌 보고서가 생성됨
     */
    __except (FPlatformMisc::GetCrashHandlingType() == ECrashHandlingType::Default
                ? ( GEnableInnerException ? EXCEPTION_EXECUTE_HANDLER : ReportCrash(GetExceptionInformation()) )
                :  EXCEPTION_CONTINUE_SEARCH )
    {
        // Crashed.
		ErrorLevel = 1;
		if(GError)
		{
			GError->HandleError();
		}
		LaunchStaticShutdownAfterError();
		FPlatformMallocCrash::Get().PrintPoolsUsage();
		FPlatformMisc::RequestExit( true, TEXT("LaunchWindowsStartup.ExceptionHandler"));
    }

    return ErrorLevel;
}

// 1 - Foundation - Entry - LaunchWindowsShutdown
LAUNCH_API void LaunchWindowsShutdown()
{
	// Final shut down.
	FEngineLoop::AppExit();

	// pause if we should
	if (GShouldPauseBeforeExit)
	{
		Sleep(INFINITE);
	}
}

// 0 - Foundation - Entry - BEGIN
/** kwakkh : 언리얼의 시작 위치 */
int32 WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ char* pCmdLine, _In_ int32 nCmdShow)
{
    int32 Result = LaunchWindowsStartup(hInInstance, hPrevInstance, pCmdLine, nCmdShow, nullptr);
	LaunchWindowsShutdown();
	return Result;
}