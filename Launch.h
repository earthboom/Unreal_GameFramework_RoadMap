/**
 * kwakkh
 * - LaunchWindow.cpp
 */

#include "LaunchEngineLoop.h"

/** FPlatformTime::Seconds()가 처음 초기화된 시간 (before main)
 * CoreGlobals.cpp 에 정의된 전역 변수
*/
double GStartTime = FPlatformTime::InitTiming();

/** kwakkh
 * 언리얼 엔진에서 메인 루프를 진행하고 관장하고 있는 전역 변수
 */
 FEngineLoop	GEngineLoop;

/** preinit the engine loop */
// 5 - Foundation - Entry - EnginePreInit
// Kwakkh : 언리얼 엔진에서 메인 루프의 초기화를 시작하는 함수
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
// kwakkh : GEngineLoop 를 통해 엔진의 메인 루프를 실행하는 함수
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
// kwakkh : 언리얼 엔진의 메인 로직 진행 함수
int32 GuardedMain( const TCHAR* CmdLine )
{
	//....

#if !(UE_BUILD_SHIPPING)

	// "-waitforattach" 또는 "-WaitForDebugger"가 지정된 경우 시작을 중지하고 디버거가 연결될 때까지 기다렸다가 계속 진행

    /** kwakkh 
    * 명령어 인자에 "-waitforattach"가 있을 경우 엔진 시작시 디버거가 붙을 때까지 루프를 돌며 대기한다.
	* - 엔진 시작지점부터 디버깅을 진행할 때 유용하게 사용 가능
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
			/**kwakkh
			 * - 여기서 멈춤
			 * - 코드를 이용해 DebugBreakPoint를 설정하는 함수
			 * - 언리얼에서 코드 레벨에서 디버깅을 진행하기 위해 존재하는 함수
			 * - 시작 지점을 디버깅하는 데 매우 유용
			 */
			UE_DEBUG_BREAK();
		}
	}

#endif

	BootTimingPoint("DefaultMain");

	// Super early init code. DO NOT MOVE THIS ANYWHERE ELSE!
    /**
     * kwakkh
	 * - 언리얼 엔진 코멘트에서도 다른 곳으로 이동시키지 말라고 명시되어 있을 정도로 중요한 함수
     * - CoreDelegates, CoreUObjectDelegates, WorldDelegates, ... 
	 * - 위의 델리게이트를 통해 엔진 시작 지점에서 초기화를 진행해야하는 중요 로직을 처리리
     * - 이 Delegate Class들은 기억하기 좋음
     * - 언리얼 엔진은 위와 같이 위임자 클래스를 증명하는 형태로 코드를 주입하는 방법을 제공
     * - 여기서 언리얼 엔진의 시작 지점에 코드를 삽입할 수 있다
     */
	FCoreDelegates::GetPreMainInitDelegate().Broadcast();

	// make sure GEngineLoop::Exit() is always called.
	/** kwakkh
	 * - 구조체의 정의(EngineLoolCleanupGuard)와 변수(CleanupGuard) 선언을 동시해 진행.
	 * - GuardMain 함수가 끝나게 될 경우 CleanupGuard 변수도 사라지게 되고 그 즉시 EngineExit() 함수를 호출
	 * - 이 패턴은 언리얼 엔진에서 자주 사용
	 */
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
        // - 언리얼 엔진의 메인 루프
		// - 종료 요청이 존재하기 전까지 무한루프인 것을 확인할 수 있다.
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
// kwakkh : GuardMain, 즉 엔진의 메인을 호출 후 에러 레벨에 대한 체크를 위해 한 번 래핑을 진행한 함수
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

	// kwakkh : 명령줄을 처리하는 단계
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
	 * SEH란 운영체제(OS)를 통해 하드웨어와 소프트웨어 예외 모두 처리하는 기본 예외 처리 메커니즘
	 * 아래의 코드는 GuardedMainWrapper에서 발생하는 모든 예외사오항에 대해서 기록을 남기고 또는 추가적 처리를 진행
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
// kwakkh : 언리얼 엔진 종료 함수
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
/** kwakkh : 언리얼 엔진의 윈도우 플랫폼 기준 시작 함수 */
int32 WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ char* pCmdLine, _In_ int32 nCmdShow)
{
    int32 Result = LaunchWindowsStartup(hInInstance, hPrevInstance, pCmdLine, nCmdShow, nullptr);
	LaunchWindowsShutdown();
	return Result;
}