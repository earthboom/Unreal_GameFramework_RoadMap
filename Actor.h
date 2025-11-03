
/**
 * 액터(Actor)는 레벨 내에 배치되거나 스폰(Spawn)될 수 있는 객체의 기본 클래스
 * 액터는 ActorComponents들의 집합을 포함할 수 있으며, 이 컴포넌트들은 액터가 어떻게 움직이는지, 어떻게 렌더링되는지 등을 제어하는 데 사용된다.
 * 액터의 또 다른 주요 기능은 플레이(게임 실행) 중 네트워크를 통해 속성(Properties) 및 함수 호출을 복제(Replication)하는 역할이다.
 * 
 * 
 * Actor initialization has multiple steps, here's the order of important virtual functions that get called:
 * - UObject::PostLoad: For actors statically placed in a level, the normal UObject PostLoad gets called both in the editor and during gameplay.
 *                      This is not called for newly spawned actors.
 * - UActorComponent::OnComponentCreated: When an actor is spawned in the editor or during gameplay, this gets called for any native components.
 *                                        For blueprint-created components, this gets called during construction for that component.
 *                                        This is not called for components loaded from a level.
 * - AActor::PreRegisterAllComponents: For statically placed actors and spawned actors that have native root components, this gets called now.
 *                                     For blueprint actors without a native root component, these registration functions get called later during construction.
 * - UActorComponent::RegisterComponent: All components are registered in editor and at runtime, this creates their physical/visual representation.
 *                                       These calls may be distributed over multiple frames, but are always after PreRegisterAllComponents.
 *                                       This may also get called later on after an UnregisterComponent call removes it from the world.
 * - AActor::PostRegisterAllComponents: Called for all actors both in the editor and in gameplay, this is the last function that is called in all cases.
 * - AActor::PostActorCreated: When an actor is created in the editor or during gameplay, this gets called right before construction.
 *                             This is not called for components loaded from a level.
 * - AActor::UserConstructionScript: Called for blueprints that implement a construction script.
 * - AActor::OnConstruction: Called at the end of ExecuteConstruction, which calls the blueprint construction script.
 *                           This is called after all blueprint-created components are fully created and registered.
 *                           This is only called during gameplay for spawned actors, and may get rerun in the editor when changing blueprints.
 * - AActor::PreInitializeComponents: Called before InitializeComponent is called on the actor's components.
 *                                    This is only called during gameplay and in certain editor preview windows.
 * - UActorComponent::Activate: This will be called only if the component has bAutoActivate set.
 *                              It will also got called later on if a component is manually activated.
 * - UActorComponent::InitializeComponent: This will be called only if the component has bWantsInitializeComponentSet.
 *                                         This only happens once per gameplay session.
 * - AActor::PostInitializeComponents: Called after the actor's components have been initialized, only during gameplay and some editor previews.
 * - AActor::BeginPlay: Called when the level starts ticking, only during actual gameplay.
 *                      This normally happens right after PostInitializeComponents but can be delayed for networked or child actors.
 *
 * @see https://docs.unrealengine.com/Programming/UnrealArchitecture/Actors
 * @see https://docs.unrealengine.com/Programming/UnrealArchitecture/Actors/ActorLifecycle
 * @see UActorComponent
 */

// 12 - Foundation - CreateWorld - AActor
// haker: 
// - Actor:
//   - AActor == collection of ActorComponents (-> Entity-Component structure)
//   - ActorComponent's examples:
//     - UStaticMeshComponent, USkeletalMeshComponent, UAudioComponent, ... etc.
//   - (networking) 상태 업데이트와 RPC 호출을 전파하기 위한 복제 단위 (액터가 멀티 네트워킹에서 가장 중요한 Replication의 하나의 단위)
//
// - Actor's Initializations (액터의 초기화 순서):
//   1. UObject::PostLoad:
//     - 로딩이 되고 나서 오브젝트를 생성할 중비가 되고 나서 그때 불리는 것이 NewObject의 PostLoad다!
//     - 당신이 AActor를 ULevel 내부에 배치하면, 해당 AActor는 ULevel의 패키지 내에 저장
//     - 게임이 빌드되어 실행될 때, ULevel이 스트리밍되거나 로드되면, 그 안에 배치되었던 AActor가 함께 로드되며 UObject::PostLoad() 함수가 호출된다.
//       - FYI, the UObject::PostLoad() is called when [object need-to-be-spawned]->[asset need-to-be-load]->[after loaded call post-event UObject::PostLoad()]
//   
//   2. UActorComponent::OnComponentCreated:
//     - 액터는 이미 Spawn 되어있다.
//     - UActorComponent별로 발생하는 알림(Notification)
//     - 기억해야 할 한 가지는 '컴포넌트 생성이 먼저' 일어난다는 점!
//   
//   3. AActor::PreRegisterAllComponents:
//     - UActorComponent들을 AActor에 등록하기 위한 사전 이벤트(pre-event)
//     - 한 가지 기억해야 할 점:
//       - *** [UActorComponent Creation] ---then---> [UActorComponent Registration] ***
//   
//   4. AActor::RegisterComponent:
//     - 점진적 등록(incremental-registration): UActorComponent를 여러 프레임에 걸쳐 AActor에 등록
//     - 등록 단계에서 수행되는 작업은 무엇인가?
//       - UActorComponent를 월드들(GameWorld[UWorld], RenderWorld[FScene], PhysicsWorld[FPhysScene] 등)에 등록
//       - 각 월드에서 요구하는 상태를 초기화
//     kwakkh(위 설명이 너무 생략 되었음)
//     - 이 ActorComponent가 Actor에 부착되어 있지만, Game World는 당연히 Actor Component로서 반영 됐으나, RenderWorld, PhysicsWorld 에도 반영을 해줘야 한다.
//     - 내부의 Entity로 하나 소환을 해주거나, 혹은 내가 Game World에 있는 나 자신을 반영하는 또 다른 객체를 하나씩 생성해주게 되는데 그 생성하는 과정을 언리얼에선 "State"를 생성한다고 한다.
//     - 각 World에 분신들을 집어넣는다고 여기면 된다. 
//   
//   5. AActor::PostRegisterAllComponents:
//     - ActorComponent들을 AActor에 등록하는 과정이 완료된 **후의 이벤트(post-event)**다.
//
//   6. AActor::UserConstructionScript:
//     [ ] explain 에디터에서의 UserConstructionScript vs SimpleConstructionScript:
//        - UserConstructionScript: 블루프린트 이벤트 그래프에서 UActorComponent를 생성하기 위해 호출되는 함수
//        - SimpleConstructionScript: 블루프린트 뷰포트에서 UActorComponent를 구성하는 작업(계층 구조 설정) 
//        - (요약: UserConstructionScript는 실행 로직을, SimpleConstructionScript는 계층적 구조를 다룬다.)
//      kwakkh
//     - 동적으로 생성되는 또 다른 방식의 Actor Component가 생성되는 방식
//     - 정적으로 바인드된 것이 아닌, ConstructionScript로 만든 것
//
//   7. AActor::PreInitializeComponents:
//     - AActor에 포함된 UActorComponent들을 초기화하기 위한 사전 이벤트(pre-event)
//     - [UActorComponent Creation]-->[UActorComponent Register]-->[UActorComponent Initialization]
//
//   8. UActorComponent::Activate:
//     - UActorComponent를 초기화하기 전에, 해당 UActorComponent의 활성화(activation)가 먼저 호출된다
//     - 이제 F Tick Function을 등록해서 Component가 Tick이 돌 수 있게 해주는 중요한 요소다. (FActorComponentTickFunction 이게 아닐까?)
//     - 코드 상에는 Initialize 이후에 Activate를 호출한다. (실제론 반대라는데, 어디서 반대로 동작하는지 확인할 수 있을까?)
//
//   9. UActorComponent::InitializeComponent:
//     - UActorComponent의 초기화 과정을 두 단계로 생각할 수 있다. (실제 코드에선 2 -> 1 순이다. 왜 이렇게 설명하는 걸까? 아니면 순서 상관 없는 걸까?)
//       1. Activate
//       2. InitializeComponent
//
//  10. AActor::PostInitializeComponents:
//     - UActorComponent들의 초기화가 모두 완료되었을 때 호출되는 사후 이벤트(post-event)다.
// 
//  11. AActor::Beginplay:
//     - 레벨에 틱(Ticking)이 시작될 때, AActor는 BeginPlay()를 호출. (요약: BeginPlay()는 모든 컴포넌트의 등록 및 초기화가 끝난 후, 게임이 실제로 시작될 때 호출되는 최종 함수.)
//
//  Diagrams:
// ┌─────────────────────┐                                                                    
// │ UObject::PostLoad() │                                                                    
// └─────────┬───────────┘                                                                    
//           │                                                                                
//           │                                                                                
// ┌─────────▼───────────┐      ┌────────────────────────────────────────────────────────────┐
// │ AActor Spawn        ├─────►│                                                            │
// └─────────┬───────────┘      │  UActorComponent Creation:                                 │
//           │                  │   │                                                        │
//           │                  │   └──UActorComponent::OnComponentCreated()                 │
//           │                  │                                                            │
//           │                  ├────────────────────────────────────────────────────────────┤
//           │                  │                                                            │
//           │                  │  UActorComponent Register:                                 │
//           │                  │   │                                                        │
//           │                  │   ├──AActor::PreRegisterAllComponents()                    │
//           │                  │   │                                                        │
//           │                  │   ├──For each UActorComponent in AActor's UActorComponents │
//           │                  │   │   │                                                    │
//           │                  │   │   └──AActor::RegisterComponent()                       │
//           │                  │   │                                                        │
//           │                  │   └──AActor::PostRegisterAllComponents()                   │
//           │                  │                                                            │
//           │                  ├────────────────────────────────────────────────────────────┤
//           │                  │  AActor::UserConstructionScript()                          │
//           │                  ├────────────────────────────────────────────────────────────┤
//           │                  │                                                            │
//           │                  │  UActorComponent Initialization:                           │
//           │                  │   │                                                        │
//           │                  │   ├──AActor::PreInitializeComponents()                     │
//           │                  │   │                                                        │
//           │                  │   ├──For each UActorComponent in AActor's UActorComponents │
//           │                  │   │   │                                                    │
//           │                  │   │   ├──UActorComponent::Activate()                       │
//           │                  │   │   │                                                    │
//           │                  │   │   └──UActorComponent::InitializeComponent()            │
//           │                  │   │                                                        │
//           │                  │   └──AActor::PostInitializeComponents                      │
//           │                  │                                                            │
//           │                  └────────────────────────────────────────────────────────────┘
//           │                                                                                
//  ┌────────▼───────────┐      ┌────────────────────┐                                        
//  │ AActor Preparation ├─────►│ AActor::BeginPlay()│                                        
//  └────────────────────┘      └────────────────────┘                                        

// see Actor's member variables (goto 13) 
UCLASS(BlueprintType, Blueprintable, MigratingAsset, config=Engine, meta=(ShortTooltip="An Actor is an object that can be placed or spawned in the world."), MinimalAPI)
class AActor : public UObject
{
	GENERATED_BODY()
public:
    /** Returns this actor's root component. */
	FORCEINLINE USceneComponent* GetRootComponent() const { return RootComponent; }

    /** Walk up the attachment chain from RootComponent until we encounter a different actor, and return it. 
     * If we are not attached to a component in a different actor, returns nullptr 
     */
	UFUNCTION(BlueprintPure, Category = "Actor")
	ENGINE_API AActor* GetAttachParentActor() const
    {
        if (GetRootComponent() && GetRootComponent()->GetAttachParent())
        {
            return GetRootComponent()->GetAttachParent()->GetOwner();
        }

        return nullptr;
    }

    /** Called before all the components in the Components array are registered, called both in editor and during gameplay */
	ENGINE_API virtual void PreRegisterAllComponents()
    {
        // Here we don't ensure that bHasPreRegisteredAllComponents == false because there are cases where PreRegisterAllComponents() can be called multiple times (SpawnActor is one example)
	    bHasPreRegisteredAllComponents = true;
    }

    /** Return the ULevel that this Actor is part of. */
	UFUNCTION(BlueprintCallable, Category=Level)
	ENGINE_API ULevel* GetLevel() const
    {
        return GetTypedOuter<ULevel>();
    }

    /**
	 * Get a direct reference to the Components set rather than a copy with the null pointers removed.
	 * WARNING: anything that could cause the component to change ownership or be destroyed will invalidate
	 * this array, so use caution when iterating this set!
	 */
	const TSet<UActorComponent*>& GetComponents() const
	{
		return ObjectPtrDecay(OwnedComponents);
	}

    /** Getter for the cached world pointer, will return null if the actor is not actually spawned in a level */
	ENGINE_API virtual UWorld* GetWorld() const override final
    {
        // CDO objects do not belong to a world
        // If the actors outer is destroyed or unreachable we are shutting down and the world should be nullptr
        if (!HasAnyFlags(RF_ClassDefaultObject) && ensureMsgf(GetOuter(), TEXT("Actor: %s has a null OuterPrivate in AActor::GetWorld()"), *GetFullName())
            && !GetOuter()->HasAnyFlags(RF_BeginDestroyed) && !GetOuter()->IsUnreachable())
        {
            if (ULevel* Level = GetLevel())
            {
                return Level->OwningWorld;
            }
        }
        return nullptr;
    }

    /**
	 * Virtual call chain to register all tick functions for the actor class hierarchy
	 * @param bRegister - true to register, false, to unregister
	 */
	ENGINE_API virtual void RegisterActorTickFunctions(bool bRegister)
    {
        check(!IsTemplate());

        if(bRegister)
        {
            if(PrimaryActorTick.bCanEverTick)
            {
                PrimaryActorTick.Target = this;
                PrimaryActorTick.SetTickFunctionEnable(PrimaryActorTick.bStartWithTickEnabled || PrimaryActorTick.IsTickFunctionEnabled());
                PrimaryActorTick.RegisterTickFunction(GetLevel());
            }
        }
        else
        {
            if(PrimaryActorTick.IsTickFunctionRegistered())
            {
                PrimaryActorTick.UnRegisterTickFunction();			
            }
        }

        FActorThreadContext::Get().TestRegisterTickFunctions = this; // we will verify the super call chain is intact. Don't copy and paste this to another actor class!
    }
};