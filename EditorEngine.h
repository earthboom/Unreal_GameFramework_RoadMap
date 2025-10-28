#include "World.h"

/** Specifies the goal/source of a UWorld object */
// 14 - Foundation - Entry - EWorldType
/**
 * kwakkh
 * - 현재는 EngineTypes.h 에 위치함
 */
namespace EWorldType
{
	enum Type
	{
		/** An untyped world, in most cases this will be the vestigial worlds of streamed in sub-levels */
		None,

		/** The game world */
		Game,

		/** A world being edited in the editor */
		Editor,

		/** A Play In Editor world */
		PIE,

		/** A preview world for an editor tool */
		EditorPreview,

		/** A preview world for a game */
		GamePreview,

		/** A minimal RPC world for a game */
		GameRPC,

		/** An editor world that was loaded but not currently being edited in the level editor */
		Inactive
	};
}

/** FWorldContext
 * "FWorldContext는 엔진 레벨에서 UWorld들을 다루기 위한 컨텍스트(Context, 문맥/상황)입니다. 
 * 엔진이 월드를 생성하거나 파괴할 때, 우리는 어떤 월드가 어디에 속하는지 명확하게 추적할 수단이 필요합니다.
 * 
 * WorldContext는 일종의 **트랙(Track)**으로 생각할 수 있습니다. 
 * 기본적으로 우리는 레벨을 로드하거나 언로드하는 하나의 트랙을 가지고 있습니다. 
 * 
 * 두 번째 컨텍스트를 추가하는 것은 두 번째 트랙을 추가하는 것과 같습니다. 
 * 이는 월드들이 존재할 수 있는 또 하나의 진행 경로(progression)를 추가하는 것입니다.
 * 
 * **게임 엔진(GameEngine)**의 경우, 동시에 여러 월드를 지원하기로 결정하기 전까지는 하나의 WorldContext만 존재합니다.
 * 
 * **에디터 엔진(EditorEngine)**의 경우, 에디터 월드(EditorWorld)를 위한 컨텍스트 하나와 PIE(Play In Editor) 월드를 위한 컨텍스트 하나, 총 두 개가 있을 수 있습니다.
 * 
 * FWorldContext는 '현재 PIE UWorld*'를 관리하는 방법과 새로운 월드에 연결하거나 이동(traveling)할 때 발생하는 상태(state)를 함께 제공합니다.
 * FWorldContext는 UEngine 클래스 내부용으로 유지되어야 합니다. 
 * 외부 코드는 FWorldContext에 대한 포인터를 유지하거나 직접 관리하려고 시도해서는 안 됩니다. 
 * 외부 코드는 여전히 **UWorld***를 다룰 수 있으며, 엔진 레벨 함수에 **UWorld***를 인자로 전달할 수 있습니다. 
 * 엔진 코드가 주어진 **UWorld***에 해당하는 관련 컨텍스트를 조회할 수 있기 때문입니다.
 * 
 * 편의를 위해, FWorldContext는 **UWorld***에 대한 외부 포인터를 유지 및 관리할 수 있습니다. 
 * 예를 들어, PIE는 UEditorEngine::PlayWorld라는 **UWorld***를 PIE 월드 컨텍스트에 연결할 수 있습니다. 
 * 만약 PIE UWorld가 변경되면, UEditorEngine::PlayWorld 포인터는 자동으로 업데이트될 것입니다. 
 * 이 작업은 AddRef() 및 SetCurrentWorld() 함수를 통해 이루어집니다."
 */
// 13 - Foundation - Entry - FWorldContext
/** kwakkh
 * - Engine.h 에 위치함
 * - 엔진 레벨에서 사용되는 월드의 Context (UEngine, UEditorEngine 등)
 * - FWorldContext를 엔진과 World간의 종속성을 잃는 descriptor로 생각할 수 있음
 * 
 * GameEngine 용
 * - 보통 하나의 World Context가 로비와 같은 세계를 게임 내 World 로 전환할 때, 두 개의 World Context가 존재하는 순간이 있음
 * 
 * EditorEngine 용
 * - 에디터는 플레인 에디터 구조를 가짐
 * - 레벨 뷰포트로 World를 편집할 때, 우리는 보통 하나의 World Context를 마주하게 된다.
 * - PIE를 실행하면 새로운 World Context가 생성되며 동시에 두 가지 World Context가 존재
 * - PIE에서 멀티플레이 게임을 실행하려 할 때, 여러 World Context이 존재할 수 있다.
 * - World Context는 맥락은 엔진을 위한 것이므로 다른 곳에서는 FWorldContext를 유지하지 말라. 
 */
USTRUCT()
struct FWorldContext
{
	GENERATED_USTRUCT_BODY()

    /** Set CurrentWorld and update external reference pointers to reflect this*/
	ENGINE_API void SetCurrentWorld(UWorld *World)
    {
        UWorld* OldWorld = ThisCurrentWorld;
        ThisCurrentWorld = World;

        if (OwningGameInstance)
        {
            OwningGameInstance->OnWorldChanged(OldWorld, ThisCurrentWorld);
        }
    }

    // see EWorldType
    TEnumAsByte<EWorldType::Type>	WorldType;

    // kwakkh : UWorld의 이름과 다른 Context에 별도의 이름을 할당
	FName ContextHandle;

    /**  kwakkh 
     * GameInstance는 하나의 World Context 를 소유하고 있다.
     * GameInstance가 보통 WorldContext하고 1:1 Mapping 되어 있다.
     */
    UPROPERTY()
	TObjectPtr<class UGameInstance> OwningGameInstance;

private:
    // kwakkh : Context가 참조하는 World
	TObjectPtr<UWorld>	ThisCurrentWorld;
}

// 12 - Foundation - Entry - UEngine
/**
 * 모든 엔진 클래스들의 추상 기본 클래스이며, 에디터 및 게임 시스템에 필수적인 핵심 시스템들의 관리를 담당합니다.
 * 또한 특정 엔진 시스템에 사용될 **기본 클래스들(Default Classes)**을 정의합니다.
 */
UCLASS(abstract, config=Engine, defaultconfig, transient, MinimalAPI)
class UEngine : public UObject, public FExec
{
    GENERATED_UCLASS_BODY()

    //...

    // kwakkh : WorldContext 생성 함수
    FWorldContext& CreateNewWorldContext(EWorldType::Type WorldType)
    {
        FWorldContext* NewWorldContext = new FWorldContext;
        WorldList.Add(NewWorldContext);
        NewWorldContext->WorldType = WorldType;
        NewWorldContext->ContextHandle = FName(*FString::Printf(TEXT("Context_%d"), NextWorldContextHandle++));

        return *NewWorldContext;
    }

    /** Initialize the game engine. */
    // 17 - Foundation - Entry - UEngine::Init
    virtual void Init(IEngineLoop* InEngineLoop)
    {
        if (GIsEditor)
        {
            // 에디터가 사용할 WorldContext를 생성하고 처음에는 비어 있던 World를 만듦
            // kwakkh : 여기서 적어도 하나의 에디터 World 가 존재하는지 확인
            FWorldContext &InitialWorldContext = CreateNewWorldContext(EWorldType::Editor);

            // kwakkh : we get into Foundation - CreateWorld
            // UWorld::CreateWorld로 World 생성
            InitialWorldContext.SetCurrentWorld( UWorld::CreateWorld( EWorldType::Editor, true ) );
            GWorld = InitialWorldContext.World();
        }
    }

    /** kwakkh
     * - TIndirectArray 가 뭐지? (TIndirectArray 와 TArray의 차이를 알아보자)
     * - WorldList는 WorldContext를 관리
     * - WorldContext를 관리하고 생성, 파괴의 주체는 엔진이다.
     */
    // see FWorldContext
    TIndirectArray<FWorldContext> WorldList;
    int32 NextWorldContextHandle;
}

// 10 - Foundation - Entry - UEditorEngine
UCLASS(config=Engine, transient, MinimalAPI)
class UEditorEngine : public UEngine
{
public:
	GENERATED_BODY()

public:
    /** 에디터 world에 대한 WorldContext를 반환. 현재로서는 에디터에 항상 정확히 하나만 있을 것이다. */
    // 16 - Foundation - Entry - GetEditorWorldContext
    FWorldContext& GetEditorWorldContext(bool bEnsureIsGWorld = false);
    {
        for (int32 i=0; i < WorldList.Num(); ++i)
        {
            if (WorldList[i].WorldType == EWorldType::Editor)
            {
                ensure(!bEnsureIsGWorld || WorldList[i].World() == GWorld);
                return WorldList[i];
            }
        }

        /**
         * kwakkh
         * - 이 코드 라인에 들어가면 크래시 발생
         * - 여기에 도달해선 안된다.
         * - 초기 에디터 World는 어디서 만들어지는가?
         */
        check(false); // 이미 UEngine::Init() 에서 생성된 World가 있었어야 한다.

        return CreateNewWorldContext(EWorldType::Editor);
    }

    // 10 - Foundation - Entry - UEditorEngine::Tick
    virtual void Tick(float DeltaSeconds, bool bIdleMode) override
    {
        // 15 - Foundation - Entry
        // kwakkh : GWorld의 처음에 어디서 업데이트 되나?
        UWorld* CurrentGWorld = GWorld;

        FWorldContext& EditorContext = GetEditorWorldContext();
    }
};