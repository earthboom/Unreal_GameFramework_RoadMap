

/** 월드의 초기화를 할 때 필요한 파라미터를 모아 놓은 구조체 */
// 3 - Foundation - CreateWorld - FWorldInitializationValues
/**
 * kwakkh
 * - 이 패턴(struct)을 코드를 더 읽기 쉽게 하기 위해 여러 매개변수를 하나로 캡슐화하는 구조체라고 생각해라
 * - 이 구조체는 월드를 생성하는데 필요한 모든 옵션을 담고 있다.
 */
struct FWorldInitializationValues
{
    //...

    /** Scene(physics, rendering)을 생성하는지에 대한 여부 */
    // kwakkh : 우리가 월드들(render world, physics world, ...)을 생성할지에 대한 여부
	uint32 bInitializeScenes:1;

    /** Physics Scene을 생성해야 하는지 여부. 이 옵션이 고려되려면 **bInitializeScenes**가 반드시 참(true)이어야 한다. */
	uint32 bCreatePhysicsScene:1;

    /** 이 월드 내에서 충돌 트레이스(Collision Trace) 호출이 유효한지 여부 */
	uint32 bEnableTraceCollision:1;

    //...
};

/** 
 * **월드(World)**는 액터(Actor)와 컴포넌트(Component)들이 존재하며 렌더링되는 맵 또는 샌드박스(Sandbox)를 나타내는 최상위 객체
 * 
 * 하나의 월드는 **영구적인 단일 레벨(Persistent Level)**과, 볼륨(Volumes)이나 블루프린트 함수를 통해 로드 및 언로드되는 스트리밍 레벨(Streaming Levels) 목록으로 구성될 수 있다.
 * 혹은 월드 컴포지션(World Composition) 기능으로 조직화된 여러 레벨들의 집합일 수도 있다.
 * 
 * **독립 실행형 게임(Standalone game)**에서는 일반적으로 단 하나의 월드만 존재한다. 
 * 다만, 대상 월드와 현재 월드가 모두 존재하는 심리스 영역 전환(seamless area transitions) 중에는 일시적으로 두 개의 월드가 존재할 수 있다.
 * 
 * 에디터에서는 여러 월드가 동시에 존재한다. 
 * - 여기에는 편집 중인 레벨뿐만 아니라, 
 * - PIE(Play In Editor) 인스턴스 각각, 
 * - 상호작용 가능한 렌더링 뷰포트가 있는 각종 에디터 툴 등 훨씬 더 많은 월드가 포함됩니다.
 *
 */
// 4 - Foundation - CreateWorld - UWorld class
UCLASS(MinimalAPI, customConstructor, config=Engine)
class UWorld final : public UObject, public FNetworkNotify
{
    // 하위 호환성(Backwards compatibility)을 위한 레거시(Legacy, 유산) 코드
    // 2 - Foundation - CreateWorld - UWorld::InitializationValues
	using InitializationValues = FWorldInitializationValues;

    /**
	 * 새로운 UWorld를 생성하고 그 포인터를 반환하는 정적(Static) 함수
	 */
    // 1 - Foundation - CreateWorld - UWorld::CreateWorld
	static UE_API UWorld* CreateWorld(
        const EWorldType::Type InWorldType, 
        bool bInformEngineOfWorld, 
        FName WorldName = NAME_None, 
        UPackage* InWorldPackage = NULL, 
        bool bAddToRoot = true, 
        ERHIFeatureLevel::Type InFeatureLevel = ERHIFeatureLevel::Num, 
        // InIVS : 월드를 만들 때 초기화 관련된 변수들의 중요한 플래그를 담아 놓은 파라미터
        const InitializationValues* InIVS = nullptr,
        bool bInSkipInitWorld = false)
    {
        //...

        /**
         * kwakkh
         * UPackage의 내용은 이후 비동기 로딩(AsyncLoading)을 다룰 때 자세히 다룰 예정
         * - 지금은 UWorld의 파일 형식을 정의한다고 생각하자 (월드를 가지고 있는 하나의 Asset 파일, 파일을 저장하는 하나의 매개체의 클래스)
         * - 반드시 기억할 것은 모든 월드가 개별 패키지와 1:1로 매핑된다는 것.
         *      - 월드 내의 모든 데이터는 파일로 직렬화(Serialization)될 필요가 있기 떄문에 이는 자연스러운 일
         *      - 인리얼 엔진에서 파일을 저장한다는 것은 Package, 즉 UPackage를 저장한다는 의미
         * - 다른 한 가지는 UObject가 OuterPrivate를 가진다는 점
         *      - OuterPrivate는 객체가 어디에 위치하는지 추론하게 해준다.
         *      - 일반적으로 OuterPrivate는 UPackage로 설정된다.
         *          - 객체가 위치하는 곳(resides in) == 객체가 위치하는 파일을 의미
         */
        UPackage* WorldPackage = InWorldPackage;
        if ( !WorldPackage )
        {
            // kwakkh : UWorld는 자신의 OuterPrivate로 패키지(Package)를 필요로 하며, **직렬화(Serialization)**될 수 있어야 한다.
            // 직렬화(Serialization) : 하나의 Property의 데이터들을 파일에 쓰는 과정 혹은 이제 패킷에다가 뭔가 쓰는 행위 자체를 의미.
            WorldPackage = CreatePackage(nullptr);
        }

        if (InWorldType == EWorldType::PIE)
        {
            /** 
             * kwakkh : UObjectBase의 ObjectFlags와 마찬가지로, UPackage의 속성(attribute) 또한 비슷한 방식으로 **플래그(Flags)**를 통해 설정될 수 있다.
             */
            WorldPackage->SetPackageFlags(PKG_PlayInEditor);
        }


        // 해당 패키지가 월드(World)를 포함하고 있음을 표시. 
        // 이 작업은 직렬화(Serialization) 시점이 아니라 바로 지금 일어나야 하는데, 예를 들어 참조 에셋 브라우저(referenced assets browser)가 올바르게 작동하기 위해서
        /**
         * kwakkh : "Transient"는 언리얼 엔진에서 **일시적잉ㄴ, 휘발성의**라는 의미를 가진다.
         * - 만약 "직렬화(Serialization)"되지 않아야 할 속성(Property)이나, Asset이 있다면, 이를 **Transient**로 표시
         * - Transient == (직렬화 대상이 아님을 표시하는 메타데이터)
         */
        if ( WorldPackage != GetTransientPackage() )
        {
            WorldPackage->ThisContainsMap();
        }

        // Create new UWorld, ULevel and UModel.
        const FString WorldNameString = (WorldName != NAME_None) ? WorldName.ToString() : TEXT("Untitled");

        UWorld* NewWorld = NewObject<UWorld>(WorldPackage, *WorldNameString);
        NewWorld->SetFlags(RF_Transactional);
        NewWorld->WorldType = InWorldType;
        NewWorld->SetFeatureLevel(InFeatureLevel);
        NewWorld->InitializeNewWorld(InIVS ? *InIVS : UWorld::InitializationValues().CreatePhysicsScene(InWorldType != EWorldType::Inactive).ShouldSimulatePhysics(false).EnableTraceCollision(true).CreateNavigation(InWorldType == EWorldType::Editor).CreateAISystem(InWorldType == EWorldType::Editor), bInSkipInitWorld);

        // Clear the dirty flags set during SpawnActor and UpdateLevelComponents
        WorldPackage->SetDirtyFlag(false);

        //...

        if ( bAddToRoot )
        {
            // Add to root set so it doesn't get garbage collected.
            NewWorld->AddToRoot();
        }

        // Tell the engine we are adding a world (unless we are asked not to)
        if( ( GEngine ) && ( bInformEngineOfWorld == true ) )
        {
            GEngine->WorldAdded( NewWorld );
        }

        return NewWorld;
    }

    // 9 - Foundation - CreateWorld - UWorld's member variables

    /** 
     * 이 월드를 로드할 때 사용되었던 URL
     * kwakkh
     * - URL을 패키지 경로(package path)라고 생각
     * - e.g. Game\Map\Seoul\Seoul.umap
     */
	FURL URL;

    /**
     * 이 월드의 유형(Type). 월드가 어떤 상황(Context)에서 사용되고 있는지를 설명 (Editor, Game, Preview etc.)
     * kwakkh
     * - TEnumAsByte는 열거형에 대한 bit 연산을 지원하기 위한 헬퍼 래퍼(Helper Wrapper) 클래스 (이 클래스를 한 번 살펴보는 걸 추천)
     * - 조언 : Bit 연산을 다루는 능력을 키우자.
     */
	TEnumAsByte<EWorldType::Type> WorldType;

    /**
	 * 이 컬렉션(UWorld)과 연관된 영구 레벨(Persistent Level)이다.
	 * 소스 컬렉션(원본 월드)과 복제된 컬렉션(복제된 월드)은 각각 자신만의 인스턴스를 가지게 된다.
     * kwakkh
     * - 월드 정보에 대한 간략한 설명
	 */
	UPROPERTY()
	TObjectPtr<class ULevel> PersistentLevel;
};