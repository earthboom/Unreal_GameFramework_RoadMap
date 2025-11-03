

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

    /** Cached level collection that this level is contained in, for faster access than looping through the collections in the world. */
	FLevelCollection* CachedLevelCollection;
};