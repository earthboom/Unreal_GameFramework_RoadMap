

//
// 이것은 레벨 객체(Level object)다. 레벨이 포함하는 액터 목록, BSP 정보, 그리고 브러시(brush) 목록 등을 담고 있다.
// 모든 레벨은 자신의 Outer로 월드(World)를 가지며, 이때 해당 레벨은 영구 레벨(Persistent Level)로 사용될 수 있다. 
// 하지만, 레벨이 스트리밍되어 로드된 경우, 그 레벨의 소유 월드(OwningWorld)는 해당 레벨이 속한 월드를 나타낸다.
//

/**
 * 레벨은 액터들(조명, 볼륨, 메시 인스턴스 등)의 집합체(Collection
 * 여러 레벨이 월드 안으로 로드되거나 언로드되어 스트리밍 경험을 만들어낼 수 있다.
 * 
 * @see https://docs.unrealengine.com/latest/INT/Engine/Levels
 * @see UActor
 */

 /**
  * 10 - Foundation - CreateWorld - ULevel class
  * kwakkh
  * Level
  * - Level == Collection of Actors
  *     - Examples of Actors
  *         - Light, Static-mesh, Volume, Brush (e.g. BSP brush : bianry-search-partitioning), ...
  *         [ ] explain BSP brush with the editor
  */
UCLASS(MinimalAPI)
class ULevel : public UObject, public IInterface_AssetUserData, public ITextureStreamingContainer, public IEditorPathObjectInterface
{
	GENERATED_BODY()

public:
    // 49 - Foundation - CreateWorld - ULevel::ULevel() constructor
    ULevel::ULevel( const FObjectInitializer& ObjectInitializer )
	:	UObject( ObjectInitializer )
	,	Actors()
	,	OwningWorld(nullptr)
	,	TickTaskLevel(nullptr)
	,	PrecomputedLightVolume(new FPrecomputedLightVolume())
	,	PrecomputedVolumetricLightmap(new FPrecomputedVolumetricLightmap())
	,	RouteActorInitializationState(ERouteActorInitializationState::Preinitialize)
	,	RouteActorInitializationIndex(0)
	,   RouteActorEndPlayForRemoveFromWorldIndex(0)
    {
    
    }

    /** 
	* Is this the persistent level 
	*/
	ENGINE_API bool IsPersistentLevel() const;
    {
        bool bIsPersistent = false;
        if( OwningWorld )
        {
            bIsPersistent = (this == OwningWorld->PersistentLevel);
        }
        return bIsPersistent;
    }

    bool IncrementalRegisterComponents(FRegisterComponentContext& Context)
    {
        // Find next valid actor to process components registration
        while (CurrentActorIndexForIncrementalUpdate < Actors.Num())
        {
            AActor* Actor = Actors[CurrentActorIndexForIncrementalUpdate];
            bool bAllComponentsRegistered = true;
            if (IsValid(Actor))
            {
                if (!Actor->HasActorRegisteredAllComponents() || GOptimizeActorRegistration == 0)
                {
#if PERF_TRACK_DETAILED_ASYNC_STATS
                    FScopeCycleCounterUObject ContextScope(Actor);
#endif
                    if (!FActorPreRegistrationAccessor::HasPreRegisteredAllComponents(Actor))
                    {
                        Actor->PreRegisterAllComponents();
                    }
                    bAllComponentsRegistered = Actor->IncrementalRegisterComponents(NumComponentsToUpdate, &Context);
                }
#if !UE_BUILD_SHIPPING
                else if (GOptimizeActorRegistration == 2)
                {
                    // Verify that there aren't any leftover components
                    Actor->ForEachComponent(false, [](UActorComponent* Component)
                    {
                        ensureMsgf(Component->IsRegistered() || !Component->bAutoRegister, TEXT("Component %s should be registered!"), *Component->GetPathName());
                    });
                }
#endif
            }

            if (bAllComponentsRegistered)
            {
                // All components have been registered for this actor, move to a next one
                CurrentActorIndexForIncrementalUpdate++;
            }

            // If we do an incremental registration return to outer loop after each processed actor 
            // so outer loop can decide whether we want to continue processing this frame
            if (NumComponentsToUpdate != 0)
            {
                break;
            }
        }

        if (CurrentActorIndexForIncrementalUpdate >= Actors.Num())
        {
            if (Context.OnIncrementalRegisterComponentsDone())
            {
                CurrentActorIndexForIncrementalUpdate = 0;
                return true;
            }
        }

        return false;
    }

    // 11 - Foundation - CreateWorld - ULevel's member variables

    /** 
     * Array of all actors in this level, used by FActorIteratorBase and derived classes
     * kwakkh
     * - 레벨은 엑터의 컨테이너
     */
	TArray<TObjectPtr<AActor>> Actors;

    /** 이 레벨이 포함된 레벨 컬렉션을 캐시(Cached)한 것입니다. 이는 월드 내의 모든 컬렉션들을 순회(Looping)하는 것보다 더 빠른 접근을 위함이다. */
     // see FLevelCollection (goto 19)
	FLevelCollection* CachedLevelCollection;

    /** 
	 * 이 레벨을 자신의 Levels 배열에 담고 있는 월드(UWorld)이다.
	 * 이 값은 GetOuter()와 동일하지 않다. 스트리밍 레벨의 GetOuter()는 사용되지 않는 잔재(vestigial) 월드이기 때문이다.
	 * 다른 모든 UObject 참조와 마찬가지로, BeginDestroy() 호출 중에는 접근해서는 안 된다. GC(가비지 컬렉터)가 어떤 순서로든 발생할 수 있기 때문이다.
     * 
     *  kwakkh : OwningWorld vs. OuterPrivate
     *  - 주의. 이 설명은 월드 컴포지션(World Composition) 방식의 레벨 스트리밍 또는 레벨 블루프린트에서의 레벨 로드/언로드 조작을 기반으로 한다.
     *    - 월드 파티션(World Partition)은 개념이 다르며, 일반적으로 이 방식에서는 OwningWorld와 OuterPrivate가 동일합니다.
     *  - Diagram:                                                                                                        
     *       World0(OwningWorld)──[OuterPrivate]──►Package0(World.umap)                                          
     *        ▲                                                                                                  
     *        │                                                                                                  
     *  [OuterPrivate]                                                                                           
     *        │                                                                                                  
     *        │                                                                                                  
     *       Level0(PersistentLevel)                                                                             
     *        │                                                                                                  
     *        │                                                                                                  
     *        ├────Level1──[OuterPrivate]──►World1───[OuterPrivate]───►Package1(Level1.umap)                     
     *        │                                                                                                  
     *        └────Level2───────►World2───────►Package2(Level2.umap)                                               
	 */
	UPROPERTY(Transient)
	TObjectPtr<UWorld> OwningWorld;

    enum class EIncrementalComponentState : uint8
	{
		Init,
		PreRegisterInitialComponents,
		RegisterInitialComponents,
#if WITH_EDITOR
		RunConstructionScripts,
#endif
		Finalize
	};

    /** 레벨 내에서 액터 컴포넌트들을 점진적으로(incrementally) 업데이트하는 현재 단계를 의미 */
	EIncrementalComponentState	IncrementalComponentState;

    /** CurrentActorIndexForUpdateComponents가 참조하는 액터가 PreRegisterAllComponents를 호출했는지 여부 */
    UE_DEPRECATED(5.5, "This property shouldn't be used anymore.")
	uint8 bHasCurrentActorCalledPreRegister:1;

    /** 컴포넌트들이 현재 등록되었는지 여부 */
	uint8 bAreComponentsCurrentlyRegistered:1;

    /** 컴포넌트 업데이트를 위한 액터 배열의 현재 인덱스 
     * kwakkh : ULevel의 ActorList에 있는 액터 인덱스를 추적하여 점진적 업데이트를 지원
    */
	int32 CurrentActorIndexForIncrementalUpdate;

    /** 틱 함수들을 저장하기 위한 데이터 구조체 
     * kwakkh
     * - 액터가 틱을 도는데 있어서 종속적인 관계는 틱이 여기 포함되어서다
    */
	class FTickTaskLevel* TickTaskLevel;

    // goto 9 (UWorld's member variables)
};