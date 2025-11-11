
/**
 * SceneComponent는 Transform을 가지고 있으며 부착(Attachment)을 지원하지만, 렌더링이나 충돌 기능은 가지고 있지 않다.
 * 이 컴포넌트는 계층 구조 내에서 다른 컴포넌트들의 위치를 오프셋(offset, 간격/기준점)시키기 위한 '더미' 컴포넌트로 유용하다.
 * @see [Scene Components](https://docs.unrealengine.com/latest/INT/Programming/UnrealArchitecture/Actors/Components/index.html#scenecomponents)
 * 
 * 17 - Foundation - CreateWorld - USceneComponent
 * kwakkh
 * USceneComponent는 씬 그래프(scene-graph)를 지원함
 * - 씬 그래프는 계층 구조(hierarchy)를 지원하기 위해서
 * - 대표적인 예시가 트랜스폼(transforms)
 */
UCLASS(ClassGroup=(Utility, Common), BlueprintType, hideCategories=(Trigger, PhysicsVolume), meta=(BlueprintSpawnableComponent, IgnoreCategoryKeywordsInSubclasses, ShortTooltip="A Scene Component is a component that has a scene transform and can be attached to other scene components."), MinimalAPI)
class USceneComponent : public UActorComponent
{
	GENERATED_BODY()

    /** Get the SceneComponent we are attached to. */
	UFUNCTION(BlueprintCallable, Category="Transformation")
	USceneComponent* GetAttachParent() const
    {
        return AttachParent;
    }

    // 18 - Foundation - CreateWorld - USceneComponent's member variables
    // kwakkh : AttachParent와 AttachChildren을 통해, 씬 그래프(scene-graph)를 위한 트리 구조(tree-structure)를 지원한다.

    /** What we are currently attached to. If valid, RelativeLocation etc. are used relative to this object */
	UPROPERTY(ReplicatedUsing = OnRep_AttachParent)
	TObjectPtr<USceneComponent> AttachParent;

    /** List of child SceneComponents that are attached to us. */
	UPROPERTY(ReplicatedUsing = OnRep_AttachChildren, Transient)
	TArray<TObjectPtr<USceneComponent>> AttachChildren;
};