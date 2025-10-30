

/**
 * **객체 인스턴스(Object Instance)**의 상태를 설명하는 플래그들
 * 이 열거형을 수정할 때는, 반드시 LexToString 구현도 업데이트해야 한다!
 */
// 8 - Foundation - CreateWorld - EObjectFlags
// ObjectMacros.h
enum EObjectFlags
{
    //...

    RF_Transactional			=0x00000008,	///< Object is transactional.

    //...
};

/**
 * Transactional 의 의미
 * 프로그래밍이나 데이터베이스에서 말하는 트랜잭션과 동일하게, **작업의 원자성(Atomicity)**과 복구 가능성을 보장하는 개념
 * 
 * 언리얼 엔진에서의 트랜잭션은 **"데이터베이스의 트랜잭션과 유사하게, 여러 단계를 거치는 하나의 작업 단위를 생성하고, 이 작업 단위를 완벽하게 되돌릴 수 있게 기록하는 시스템"**
 * - 주된 대상: 에디터에서 사용자가 수행하는 모든 변경 행위(예: 액터 이동, 속성 수정, 컴포넌트 추가/삭제 등).
 * - 주된 목적: 사용자가 Ctrl+Z (실행 취소)를 눌렀을 때, 이전 상태로 정확하게 되돌아가도록 보장하는 것.
 */

/** 
 * Low level implementation of UObject, should not be used directly in game code 
 */
// 7 - Foundation - CreateWorld - UObjectBase
// UObjectBase.h
// UObject의 가장 기본이 되는 부모 클래스
class UObjectBase
{
    /** Flags used to track and report various object states. This needs to be 8 byte aligned on 32-bit
	    platforms to reduce memory waste */
    // kwakkh : UObject의 동작이나 속성을 메타데이터 형식으로 정의하기 위한 **비트 플래그(bit flags)**
    // - UObject의 속성값들에 필요한 것들을 마킹해 놓은게 다 들어가 있다고 보면 된다.
	EObjectFlags ObjectFlags;


    /** 이 객체가 위치하고 있는 상위 객체 */
    /**
     * kwakkh : "이전에 이야기했듯이, 기본적으로 (객체의 OuterPrivate는) UPackage로 설정된다.
     * - 하지만 시간이 지나면서 언리얼 엔진은 에셋 간의 의존성(dependency)을 줄이기 위한 많은 기능을 지원하기 시작했다는 점에 주목해야 한다.
     *      - **OFPA (One File Per Actor = 엑터마다 하나씩 파일을 가진다.)**가 이러한 대표적인 예시 중 하나
     *      - 과거에는 AActor가 ULevel 내부에 위치했으며, 그 UPackage는 단순히 레벨 에셋 파일이어서 이해하기 쉬웠다.
     *      - 그러나 OFPA가 도입된 후에는 **간접적인 단계(indirection)**가 추가되어, 이제 더 이상 모든 AActor가 ULevel 파일에 저장되지 않고, 외부 경로(External path)를 가진 별도의 파일에 저장된다.
     * - 전반적인 패턴은 유지되고 있지만, 엔진이 발전하면서 실제 동작을 이해하기 위한 간접적인 단계와 복잡성이 추가되었다는 것.
     * - 지금은, OuterPrivate가 일반적으로 UPackage로 설정된다는 것만 이해하시면 충분!"
     */
	ObjectPtr_Private::TNonAccessTrackedObjectPtr<UObject> OuterPrivate;
};

/**
 * Provides utility functions for UObject, this class should not be used directly
 */
// 6 - Foundation - CreateWorld - UObjectBaseUtility
// UObjectBaseUtility.h
class UObjectBaseUtility : public UObjectBase
{
    /**
	 * Traverses the outer chain searching for the next object of a certain type.  (T must be derived from UObject)
	 *
	 * @param	Target class to search for
	 * @return	a pointer to the first object in this object's Outer chain which is of the correct type.
	 */
	COREUOBJECT_API UObject* GetTypedOuter(UClass* Target) const
    {
        UObject* Result = NULL;
        for ( UObject* NextOuter = GetOuter(); Result == NULL && NextOuter != NULL; NextOuter = NextOuter->GetOuter() )
        {
            if ( NextOuter->IsA(Target) )
            {
                Result = NextOuter;
            }
        }
        return Result;
    }

    /**
	 * Traverses the outer chain searching for the next object of a certain type.  (T must be derived from UObject)
	 *
	 * @return	a pointer to the first object in this object's Outer chain which is of the correct type.
	 */
	template<typename T>
	T* GetTypedOuter() const
	{
		return (T *)GetTypedOuter(T::StaticClass());
	}

    /**
	 * Determines whether this object is a template object by checking flags on the object and the outer chain.
	 *
	 * @param	TemplateTypes	Specific flags to look for, the default checks class default and non-class templates
	 * @return	true if this object is a template object that can be used as an archetype
	 */
    // 54 - Foundation - CreateWorld - UObjectBaseUtility::IsTemplate()
	COREUOBJECT_API bool IsTemplate(EObjectFlags TemplateTypes = RF_ArchetypeObject|RF_ClassDefaultObject) const;
    {
        for (const UObjectBaseUtility* TestOuter = this; TestOuter; TestOuter = TestOuter->GetOuter() )
        {
            if ( TestOuter->HasAnyFlags(TemplateTypes) )
                return true;
        }

        return false;
    }
};

/** 
 * The base class of all UE objects. The type of an object is defined by its UClass.
 * This provides support functions for creating and using objects, and virtual functions that should be overridden in child classes.
 * 
 * @see https://docs.unrealengine.com/ProgrammingAndScripting/ProgrammingWithCPP/UnrealArchitecture/Objects
 */
// 5 - Foundation - CreateWorld - UObject
class UObject : public UObjectBaseUtility
{

};