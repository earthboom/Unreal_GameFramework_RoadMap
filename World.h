

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

/** Subsystems은 특정 엔진 구조체의 생명 주기(lifetime)를 공유하는 자동 인스턴스화(auto instanced) 클래스
 * 
 *	현재 지원되는 서브시스템 생명 주기:
 *		Engine		 -> UEngineSubsystem 상속
 *		Editor		 -> UEditorSubsystem 상속
 *		GameInstance -> UGameInstanceSubsystem 상속
 *		World		 -> UWorldSubsystem 상속
 *		LocalPlayer	 -> ULocalPlayerSubsystem 상속
 *
 *
 * 일반적인 접근 예시
 * 1. 단일 구현 클래스 접근 예시:
 * 		class UMySystem : public UGameInstanceSubsystem
 *	    접근 방법:
 *		UGameInstance* GameInstance = ...;
 *		UMySystem* MySystem = GameInstance->GetSubsystem<UMySystem>();
 *
 * 2. Null 체크를 포함한 접근 예시:
 *      GameInstance가 Null(널)일 가능성으로부터 보호해야 하는 경우 :
 *		UGameInstance* GameInstance = ...;
 *		UMyGameSubsystem* MySubsystem = UGameInstance::GetSubsystem<MyGameSubsystem>(GameInstance);
 *
 * 인터페이스 기반 접근 예시
 * - 여러 구현체를 가질 수 있는 Interface를 정의할 수도 있다.
 * 
 * 인터페이스 예시:
 *  인터페이스: 
 *      MySystemInterface
 *  두 개의 구체적인 파생 클래스:
 *      MyA : public MySystemInterface
 *      MyB : public MySystemInterface
 *
 *	접근 방법:
 *		UGameInstance* GameInstance = ...;
 *		const TArray<UMyGameSubsystem*>& MySubsystems = GameInstance->GetSubsystemArray<MyGameSubsystem>();
 * 
 * 21 - Foundation - CreateWorld - USubsystem
 * kwakkh : Subsystem.h 에 존재
 * - USubsystem은 언리얼 엔진의 컴포넌트(component) 하나의 생명 주기(생성/파괴)를 따르는 시스템.
 * - types of subsystems:
 *   1. Engine        -> UEngineSubsystem 상속
 *   2. Editor        -> UEditorSubsystem 상속
 *   3. GameInstance  -> UGameInstanceSubsystem 상속
 *   4. World         -> UWorldSubsystem 상속
 *   5. LocalPlayer   -> ULocalPlayerSubsystem 상속
 * 
 * - 엔진의 UWorld와 같은 컴포넌트들의 생명 주기를 수동으로 관리하는 것은 번거롭다.
 *   - 만약 Subsystem을 사용한다면, 이 모든 것이 매우 쉽고 편리해질 것!
 */
UCLASS(Abstract, MinimalAPI)
class USubsystem : public UObject
{
	GENERATED_BODY()

    /** Override to control if the Subsystem should be created at all.
	 * For example you could only have your system created on servers.
	 * It's important to note that if using this is becomes very important to null check whenever getting the Subsystem.
	 *
	 * Note: This function is called on the CDO prior to instances being created!
	 */
	virtual bool ShouldCreateSubsystem(UObject* Outer) const { return true; }

	/** Implement this for initialization of instances of the system */
	virtual void Initialize(FSubsystemCollectionBase& Collection) {}

	/** Implement this for deinitialization of instances of the system */
	virtual void Deinitialize() {}

private:
    // haker: we are interested UWorld's [FObjectSubsystemCollection<UWorldSubsystem>]
    // - each subsystem has its owner like this
    // see FObjectSubsystemCollection (goto 22)
    FSubsystemCollectionBase* InternalOwningSubsystem;
}

/**
 * UWorldSubsystem
 * UWorld와 생명 주기를 공유하며 자동으로 인스턴스화(instanced)되고 초기화되는 시스템들의 기본 클래스
 * 21 - Foundation - CreateWorld - UWorldSubsystem
 * WorldSubsystem.h 에 존재
 * see USubsystem (goto 21)
 */
UCLASS(Abstract, MinimalAPI)
class UWorldSubsystem : public USubsystem
{
	GENERATED_BODY()

    /** 월드 컴포넌트들(예: 라인 배치기 및 모든 레벨 컴포넌트들)의 업데이트가 완료된 후에 호출된다. */
	virtual void OnWorldComponentsUpdated(UWorld& World) {}
}

/** 
 * Indicates the type of a level collection, used in FLevelCollection.
 * 20 - Foundation - CreateWorld - ELevelCollectionType
 * kwakkh : 해당 타입(Type)들에 얽매이지 않고, 동적인 레벨(Dynamic Level)과 정적인 레벨(Static Level)처럼 단순하게 이해하는게 좋다.
 * EngineTypes.h 에 존재
 */
enum class ELevelCollectionType : uint8
{
	/**
	 * 이것은 일반적인 게임플레이에 사용되며, 복제된 모든 컬렉션의 원본이 되는 동적 레벨들이다.
	 * 이 컬렉션은 월드의 영구 레벨(Persistent Level)과, 동적 또는 복제되는 게임플레이 액터를 포함하는 모든 스트리밍 레벨들을 담게 된다.
	 * 이 컬렉션은 게임플레이 월드와 에디터 월드 모두에서 항상 존재한다.
	 */
	DynamicSourceLevels,

	/** 
	 * 게임의 요청이 있을 경우 DynamicSourceLevels로부터 복제된, 게임플레이와 관련된 레벨들이다.
	 * 이 컬렉션은 레벨들이 실제로 복제된 경우에만 존재한다.
	 */
	DynamicDuplicatedLevels,

	/**
	 * 이 레벨들은 원본 레벨들과 복제된 레벨들 사이에서 공유된다.
	 * 이 레벨들은 정적인 지오메트리와 복제되거나 게임플레이의 영향을 받지 않는 기타 시각 요소들만 포함해야 한다.
	 * 메모리를 절약하기 위해 이 레벨들은 복제되지 않을 것이다.
	 * 만약 s.World.CreateStaticLevelCollection 값이 0이라면, 이 컬렉션은 생성되지 않으며 정적 레벨들은 동적 레벨로 취급된다.
	 */
	StaticLevels,

	MAX
};

/**
 * UWorld 내에서 특정 ELevelCollectionType 유형의 레벨 그룹을 포함하며, 이 레벨들을 적절히 틱(tick)하고 업데이트하는 데 필요한 컨텍스트(Context)를 담고 있다. 
 * 이 객체는 이동만 가능(move-only).
 * 
 * 19 - Foundation - CreateWorld - FLevelCollection
 * kwakkh : FLevelCollection은 ELevelCollectionType을 기반으로 하는 컬렉션(Collection)이다.
 */
USTRUCT()
struct FLevelCollection
{
    /** The type of this collection. */
    // see ELevelCollectionType (goto 20)
	ELevelCollectionType CollectionType;

    //...

    /**
	 * The persistent level associated with this collection.
	 * The source collection and the duplicated collection will have their own instances.
     * kwakkh : usually OwnerWorld's PersistentLevel
	 */
	UPROPERTY()
	TObjectPtr<class ULevel> PersistentLevel;

	/** All the levels in this collection. */
	UPROPERTY()
	TSet<TObjectPtr<ULevel>> Levels;
}

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

#if WITH_EDITORONLY_DATA
	/** 현재 편집 중인 레벨을 가리키는 포인터. 이 레벨은 Levels 배열에 포함되어야 하며, 게임에서는 영구 레벨(PersistentLevel)과 동일해야 한다. */
	UPROPERTY(Transient)
	TObjectPtr<class ULevel> CurrentLevel;
#endif

    /** 현재 이 월드에 포함된 레벨들의 배열. Hard References(강한 참조)를 피하기 위해 디스크에 직렬화되지 않는다. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<class ULevel>> Levels;

	/** 현재 이 월드에 포함된 레벨 컬렉션들의 배열 */
	UPROPERTY(Transient, NonTransactional, Setter = None, Getter = None)
	TArray<FLevelCollection> LevelCollections;

    /** DefaultPhysicsVolume used for whole game **/
	UPROPERTY(Transient)
	TObjectPtr<APhysicsVolume> DefaultPhysicsVolume;

    /** 전체 게임에 사용되는 기본 물리 볼륨(DefaultPhysicsVolume)
     * - 특정 범위만 정하고 싶음
     */
	FPhysScene*	PhysicsScene;

    // PSC Pooling END
    // see UWorldSubsystem (goto 21)
	FObjectSubsystemCollection<UWorldSubsystem> SubsystemCollection;

    /** line batchers: */
    // kwakkh: debug lines
    // - ULineBatchComponents are resided in UWorld's subobjects
    TObjectPtr<class ULineBatchComponent> LineBatcher;
    TObjectPtr<class ULineBatchComponent> PersistentLineBatcher;
    TObjectPtr<class ULineBatchComponent> ForegroundLineBatcher;

    /**
     * kwakkh : let's wrap up what we have looked through classes:
     *                                                                                ┌───WorldSubsystem0       
     *                                                        ┌────────────────────┐  │                         
     *                                                 World──┤SubsystemCollections├──┼───WorldSubsystem1       
     *                                                   │    └────────────────────┘  │                         
     *                                                   │                            └───WorldSubsystem2       
     *             ┌─────────────────────────────────────┴────┐                                                 
     *             │                                          │                                                 
     *           Level0                                     Level1                                              
     *             │                                          │                                                 
     *         ┌───┴────┐                                 ┌───┴────┐                                            
     *         │ Actor0 ├────Component0(RootComponent)    │ Actor0 ├─────Component0(RootComponent)              
     *         ├────────┤     │                           ├────────┤      │                                     
     *         │ Actor1 │     ├─Component1                │ Actor1 │      │   ┌──────┐                          
     *         ├────────┤     │                           ├────────┤      └───┤Actor2├──RootComponent           
     *         │ Actor2 │     └─Component2                │ Actor2 │          └──────┘   │                      
     *         ├────────┤                                 ├────────┤                     ├──Component0          
     *         │ Actor3 │                                 │ Actor3 │                     │                      
     *         └────────┘                                 └────────┘                     ├──Component1          
     *                                                                                   │   │                  
     *                                                                                   │   └──Component2      
     *                                                                                   │                      
     *                                                                                   └──Component3          
     * search 'goto 4'
     */
};