#include "World.h"

/** Specifies the goal/source of a UWorld object */
// 14 - Foundation - Entry - EWorldType
/**
 * kwakkh
 * - 월드에 대한 다양한 타입을 선언한 열거형
 * - EngineTypes.h 에 위치함
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
 * - 엔진 레벨에서 다양한 UWorld들을 처리하기 위한 Context (UEngine, UEditorEngine 등)
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
 * 
 * - 일반적인 게임 상황이라면 World가 1개 존재하겠지만 에디터의 경우엔 동시에 다양한 맵이 존재할 수 있다.
 * - ex) 에디터에서 Actor의 블루프린트로 간다면 에디터 세상의 맵 1개 블루프린트의 맵 1개 총 2개가 존재하기 된다.
 * -     이런 상황에서 World가 PIE 인지 GameWorld 인지 등 다양한 판단을 엔진에서 하기 위해 존재하는 구조체
 */
USTRUCT()
struct FWorldContext
{
	GENERATED_USTRUCT_BODY()

    /** Set CurrentWorld and update external reference pointers to reflect this*/
    /** kwakkh
     * - WorldContext에 World를 설정하는 함수
     * - 중요점은 이 함수를 통해 WorldContext는 World 자체가 아닌, World를 엔진 레벨에서 컨트롤하기 쉽도록 World를 들고 있는 래핑 구조체라는 것.
     */
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
    // kwakkh : WorldContext가 참조하는 World
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

    // kwakkh : WorldType에 맞는 WorldContext 생성하고 반환하는 함수 (이 함수에서 World를 세팅하지 않음!)
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
    /**
     * kwakkh
     * - 게임 엔진을 초기화하는 함수
     * - Editor 모드일 경우 WorldContext와 World를 Editor 타입으로 만들어 GWorld에 세팅.
     * - 이 함수를 통해 에디터는 최소한 하나의 월드를 보장 받을 수 있다. 
     */
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
     * - TIndirectArray의 경우 TArray와 동일하게 데이터를 저장하는 것은 맞으나 데이터를 추가할 때 해당 데이터를 동적할당을 진행하고 배열에서 제외할 때 메모리에서 제거한다.
     * - WorldList는 WorldContext를 관리
     * - WorldContext를 관리하고 생성, 파괴의 주체는 엔진이다.
     */
    // see FWorldContext
    TIndirectArray<FWorldContext> WorldList;
    int32 NextWorldContextHandle;
}

// 10 - Foundation - Entry - UEditorEngine
/**
 * kwakkh
 * - 언리얼 엔진에서 에디터를 담당하는 클래스
 * - 에디터는 일반적으로 게임에서는 발생하지 않는 다양한 상황이 존대하기 때문에 별도의 클래스가 존재하는 것을 확인할 수 있다.
 */
UCLASS(config=Engine, transient, MinimalAPI)
class UEditorEngine : public UEngine
{
public:
	GENERATED_BODY()

public:
    // 16 - Foundation - Entry - GetEditorWorldContext
    /** kwakkh :에디터 world에 대한 WorldContext를 반환. 현재로서는 에디터에 항상 정확히 하나만 있을 것이다. */
    FWorldContext& GetEditorWorldContext(bool bEnsureIsGWorld = false);
    {
        /** kwakkh
         * - 다양한 월드 타입에서 Editor 타입에 해당하는 월드를 가져오는 로직
         * - 중요한 점은 만일 Editor 월드가 존재하지 않으면 check(false) 가 되어 무조건 크래시가 발동하게 되어 있다.
         * - 언리얼 Editor 월드가 존재하지 않으면 Editor에서 어떤 것도 진행 불가이기 때문에 1개는 반드시 보장되어야 한다.
         */

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

/**
 * TIndirectArray 와 TArray 의 차이
 * - 가장 큰 차이는 **배열이 요소를 소유하고 관리하는 방식**
 * 
 * TArray : **실제 물건(값)**을 상자에 순서대로 넣어 보관하는 **창고**
 * TIndirectArray : 물건은 다른 곳(힙 메모리)에 두고, 그 물건의 **위치가 적힌 쪽지(포인터)**만 순서대로 보관하는 **장부**
 * 
 * ----------------------------
 * TArray<T> 값 배열
 * 
 * 우리가 흔히 아는 표준 배열
 * TArray<FMyStruct> MyArray
 * 
 * - 동작 방식: FMyStruct 타입의 인스턴스(값) 자체가 배열 메모리 안에 순서대로 저장
 * - 장점
 *      1. 데이터가 한곳에 모여있어 캐시 효율이 좋고 접근이 매우 빠르다.
 * - 단점
 *      1. 다형성(Polymorphism) 불가: TArray<FBaseStruct>에 자식 클래스인 FDerivedStruct를 넣으면, 자식 클래스의 데이터가 잘려나가는 "Slicing" 현상이 발생
 *      2. 느린 재배열: 만약 FMyStruct가 매우 큰 구조체라면, 배열 중간에 요소를 추가/삭제할 때마다 이 무거운 데이터를 복사하고 이동시키는 비용이 비싸다.
 * 
 * ----------------------------
 * TIndirectArray<T> 간접 배열
 * 
 * 이름(Indirect)처럼 데이터를 '간접적으로' 가리킨다.
 * TIndirectArray<FMyStruct> MyIndirectArray;
 * 
 * - 동작 방식
 *      1. 요소(FMyStruct)를 추가(Add)하면, TIndirectArray가 힙(Heap) 메모리에 해당 요소를 위한 공간을 new로 할당
 *      2. 배열 자체에는 그 힙 메모리를 가리키는 포인터만 저장
 *      3. 배열에서 요소를 제거하거나 배열 자체가 파괴될 때, TIndirectArray는 자기가 가리키고 있던 힙 메모리를 자동으로 delete 한다. (중요!).
 * - 장점
 *      1. 다형성(Polymorphism) 완벽 지원: TIndirectArray<FBaseStruct>에 FDerivedStruct를 추가할 수 있다. 실제 저장은 포인터(FBaseStruct*)로 이루어지므로 데이터가 잘리지 않는다. 이것이 이 컨테이너의 핵심 존재 이유
 *      2. 빠른 재배열: 배열이 재정렬될 때 실제 무거운 데이터가 아닌 가벼운 포인터만 교환됨.
 * - 단점
 *      1. TArray보다 접근 속도가 약간 느리다. (포인터를 한 번 더 거쳐가야 하므로)
 * 
 * ----------------------------
 * TArray<T*>(포인터 배열)과 TIndirectArray<T> 의 차이가 뭔가?
 * - TArray<FMyStruct*>: 포인터를 저장만 할 뿐, 그 포인터가 가리키는 객체의 생명 주기를 책임지지 않는다. new와 delete를 사용자가 직접 관리해야 하며, 안 그러면 메모리 누수(Memory Leak)가 발생
 * - TIndirectArray<FMyStruct>: 포인터를 저장하며, 그 포인터가 가리키는 객체의 생명 주기(소유권)를 완벽하게 관리한다. Add 할 때 new 해주고, Remove나 소멸자가 호출될 때 delete 해준다.
 * 
 * ----------------------------
 * 언제 사용해야 할까?
 * - TArray<T>를 사용할 때 (95%의 경우):
 *      1. int, FVector, FName, FGameplayTag 등 가벼운 데이터.
 *      2. TObjectPtr<UObject> (어차피 UObject는 포인터로 관리됨).
 *      3. 다형성이 필요 없는 struct를 다룰 때.
 *      4. 고민된다면 일단 TArray를 써라.
 * 
 * TIndirectArray<T>를 사용할 때 (특수한 경우):
 *      1. UObject가 아닌 일반 C++ struct나 class에서 다형성을 구현해야 할 때. (예: TIndirectArray<FBaseShape>에 FSquare, FCircle 등을 담을 때)
 *      2. 배열이 해당 객체의 **소유권(new/delete)**을 명확하게 갖게 하고 싶을 때.
 */