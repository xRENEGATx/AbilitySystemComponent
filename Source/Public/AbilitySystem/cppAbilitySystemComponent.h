// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Structures/cppStruct_Ability.h"
#include "Structures/cppStruct_AbilityWeapon.h"
#include "AbilitySystem/cppDA_Ability.h"
#include "AbilitySystem/cppInterface_AbilitySystem.h"
#include "cppAbilitySystemComponent.generated.h"

class USkeletalMeshComponent;
class UcppAVD_GameInstance;
class UGameplayStatics;
class UAVD_StaticFunctions;
class UKismetSystemLibrary;
class AcppEffect;

DECLARE_LOG_CATEGORY_EXTERN(LogAbilitySystem, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInited, bool, Success);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDebuffAS, TArray<AcppEffect*>, DebuffList);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBuffAS, TArray<AcppEffect*>, BuffList);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FComboDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAbilityAdded, FAbilityContent, Ability, int32, IndexArray);

UENUM(BlueprintType)
enum class E_PlayerAbility:uint8
{
	Ability_Block		UMETA(DisplayName = "Block"),
	Ability_Evade		UMETA(DisplayName = "Evade"),
	Ability_Ulta		UMETA(DisplayName = "Ulta"),
	Ability_Combo		UMETA(DisplayName = "Combo"),
	Ability_HighSkill	UMETA(DisplayName = "HighSkill"),
	Ability_LowSkill	UMETA(DisplayName = "LowSkill")
};

UENUM(BlueprintType)
enum class E_AbilityTypeToLoad :uint8
{
	Ability_Weapon	UMETA(DisplayName = "WeaponAbility"),
	Ability_Active	UMETA(DisplayName = "ActiveAbility"),
	Ability_Passive	UMETA(DisplayName = "PassiveAbility"),
	Ability_None	UMETA(DisplayName = "None")
};

UENUM(BlueprintType)
enum class E_Effect :uint8
{
	Effect_Buff		UMETA(DisplayName = "Buff"),
	Effect_Debuff	UMETA(DisplayName = "Debuff"),
};

USTRUCT(BlueprintType)
struct FStruct_EffectArrayRep
{
	GENERATED_BODY();

	//Now Empty, but in future replace with BuffArray and DebuffArray

};

USTRUCT(BlueprintType)
struct FStruct_ActiveAbilityRep
{
	GENERATED_BODY();

	//Now Empty, but in future replace with ActiveAbility

};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AVD_ONLINE_API UcppAbilitySystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UcppAbilitySystemComponent();	


#pragma region =====<VARIABLES>=====

//<---------------------------<Data Assets>------------------------------------>
	/** For manual configuration */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystemComponent|Variable|ForManualSetUPAbilities")
		bool UseAbilitiesManualSetup;

	/** For manual configuration */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystemComponent|Variable|ForManualSetUPAbilities", meta = (EditCondition = "UseAbilitiesManualSetup"))
		TArray<UcppDA_Ability*> PassiveAbilitiesDataAssetArray;

	/** For manual configuration */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystemComponent|Variable|ForManualSetUPAbilities", meta = (EditCondition = "UseAbilitiesManualSetup"))
		TArray<UcppDA_Ability*> ActiveAbilitiesDataAssetArray;

//<---------------------------<End>------------------------------------>
protected: 

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystemComponent|Variable", meta = (AllowPrivateAccess = "true"))
		FName StartAttackTraceSocket;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystemComponent|Variable", meta = (AllowPrivateAccess = "true"))
		float AttackSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystemComponent|Variable", meta = (AllowPrivateAccess = "true"))
		float TimeToResetCombo;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystemComponent|Variable|ActionsNames", meta = (AllowPrivateAccess = "true"))
		FName RightAttackNotify;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystemComponent|Variable|ActionsNames", meta = (AllowPrivateAccess = "true"))
		FName LeftAttackNotify;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystemComponent|Variable|ActionsNames", meta = (AllowPrivateAccess = "true"))
		FName SkillActivateNotify;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystemComponent|Variable|ActionsNames", meta = (AllowPrivateAccess = "true"))
		FName NextComboNotify;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystemComponent|Variable|ActionsNames", meta = (AllowPrivateAccess = "true"))
		FName ComboStepEndNotify;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystemComponent|Variable|Trace", meta = (AllowPrivateAccess = "true"))
		float TraceRadius;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystemComponent|Variable|Trace", meta = (AllowPrivateAccess = "true"))
		float TraceRadiusFindEnemy;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystemComponent|Variable|Trace", meta = (AllowPrivateAccess = "true"))
		TArray<TEnumAsByte<EObjectTypeQuery>> DamageTypesArray;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystemComponent|Variable|Trace", meta = (AllowPrivateAccess = "true"))
		TArray<TEnumAsByte<EObjectTypeQuery>> FindTypesArray;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AbilitySystemComponent|Variable|Trace|Debug", meta = (AllowPrivateAccess = "true"))
		bool bDebugAttackTrace;
	
private:

	//<---------------------------<Abilities>------------------------------------>
	
	/** For network replication*/
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "AbilitySystemComponent|Variable|ActiveAbilities", meta = (AllowPrivateAccess = "true"))
		TArray<FAbilityContent> ActiveAbilitiesArray;	
	
	/** For network replication*/
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "AbilitySystemComponent|Variable|PassiveAbilities", meta = (AllowPrivateAccess = "true"))
		TArray<FAbilityContent> PassiveAbilitiesArray;

	//<---------------------------<End>------------------------------------>

	//Remember this structure need for make damage and other ability effects
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "AbilitySystemComponent|Variable", meta = (AllowPrivateAccess = "true"))
		FcppStruct_Ability ActiveAbility;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "AbilitySystemComponent|Variable", meta = (AllowPrivateAccess = "true"))
		FcppStruct_AbilityWeapon WeaponAbility;

	//Current combo step
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "AbilitySystemComponent|Variable", meta = (AllowPrivateAccess = "true"))
		int32 CurrentComboIndex;

	//Combo reset timer
	UPROPERTY()
		FTimerHandle ResetComboTimer;	

	//Array of ignor actors for multytrace
	UPROPERTY()
		TArray<AActor*>ActorsToIgnor;

	//<-------------------------<Buffs/Debuffs>--------------------------------->
	UPROPERTY(ReplicatedUsing = "OnRep_BuffsArray", BlueprintReadOnly, Category = "StatComponent|Buff", meta = (AllowPrivateAccess = "true"))
		TArray<AcppEffect*> BuffsArray;
	
	UPROPERTY(ReplicatedUsing = "OnRep_DebuffsArray", BlueprintReadOnly, Category = "StatComponent|Debuff", meta = (AllowPrivateAccess = "true"))
		TArray<AcppEffect*> DebuffsArray;
	
	//<---------------------------<End>------------------------------------>
	
	TArray<FTimerHandle> TimerHandleArray;

	//<-------------------------<Ability>--------------------------------->

	UPROPERTY(BlueprintReadOnly, Category = "AbilitySystemComponent|Variable|Trace|Debug", meta = (AllowPrivateAccess = "true"))
		AcppAbility* CurrentAbility;

#pragma endregion =====<VARIABLES>=====
	
#pragma region =====<FUNCTIONS>=====

public:

	/**Init AbilitySystem after init WeaponComponent and other skill additions
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilitySystemComponent|InitAbylitySystem")
		void InitAbilityComponent(FName AbilityID, E_AbilityTypeToLoad AbilityToLoad = E_AbilityTypeToLoad::Ability_Weapon, int32 AbilityIndexArray = 0, FName CategoryNameToLoad = "Skills");

	/**Use this function to add loaded ability, for example, from chest
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilitySystemComponent|InitAbylitySystem")
		void AddLoadedActiveAbilityToArray(FAbilityContent Ability, int32 AbilityIndexArray = 0);
	
	/**This function play weapon ability
	*@param Ability	Ability to start
	*/
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemComponent|InitAbylitySystem")
		void PlayWeaponAbilityMontage(UPARAM(ref) UAnimInstance* const &AnimInstance, E_PlayerAbility Ability);

	/**This function play active ability
	*@param AbliltyIndex	IndexOfability
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilitySystemComponent|InitAbylitySystem")
		void PlayActiveAbilityMontage(UPARAM(ref) UAnimInstance* const &AnimInstance, int32 AbliltyIndex);

	FORCEINLINE FcppStruct_AbilityWeapon GetWeaponAbility() const{ return WeaponAbility; }

	FORCEINLINE UAnimMontage * GetComboStepMontage() const { return WeaponAbility.Combo.ComboSteps[CurrentComboIndex].AnimMontage; }

	/**Get Ability cooldown
	*/
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemComponent|Getters")
	FORCEINLINE float GetPlayerAbilityCooldown(E_PlayerAbility Ability) const
	{
		float l_ReturnValue = 0.0;

		switch (Ability)
		{
			case E_PlayerAbility::Ability_Block:

				l_ReturnValue = WeaponAbility.Block.CoolDown;

				break;

			case E_PlayerAbility::Ability_Combo:

				l_ReturnValue = 0.0f;

			case E_PlayerAbility::Ability_Evade:

				l_ReturnValue = WeaponAbility.Evade.CoolDown;
				
				break;

			case E_PlayerAbility::Ability_Ulta:

				l_ReturnValue = WeaponAbility.Ulta.CoolDown;

				break;

			case E_PlayerAbility::Ability_HighSkill:

				if (ActiveAbilitiesArray[1].Ability.AbilityIcon == nullptr || ActiveAbilitiesArray[1].Ability.CoolDown == 0.0f)
				{
					l_ReturnValue = 0.1f;
				}
				else
				{
					l_ReturnValue = ActiveAbilitiesArray[1].Ability.CoolDown;
				}				

				break;

			case E_PlayerAbility::Ability_LowSkill:

				if (ActiveAbilitiesArray[0].Ability.AbilityIcon == nullptr || ActiveAbilitiesArray[0].Ability.CoolDown == 0.0f)
				{
					l_ReturnValue = 0.1f;
				}
				else
				{
					l_ReturnValue = ActiveAbilitiesArray[0].Ability.CoolDown;
				}

				break;
		}

		return l_ReturnValue;
	}

	/**Get ActiveAbility
	*/
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemComponent|Getters")
	FORCEINLINE FcppStruct_Ability GetActiveSkill() const { return ActiveAbility; }
	   
	UFUNCTION(BlueprintCallable, Category = "AbilitySystemComponent|Getters")
	FORCEINLINE int32 GetComboIndex() const { return CurrentComboIndex; }

	UFUNCTION(BlueprintCallable, Category = "AbilitySystemComponent|Getters")
	FORCEINLINE FName GetComboEndNotify() { return ComboStepEndNotify; }

	/**This function trace and deal damage
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilitySystemComponent|Skills")
		void TraceAndMakeDamage(FName AttackName);

	/**This function activate skill logic
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilitySystemComponent|Skills")
		void SkillActivate();
	
	/**Timer function
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilitySystemComponent|Skills")
		void ResetCombo();

	//Server side. This function update battle mode during block for axemple
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Player|Skills|CoolDown")
		void FindEnemyInRadius();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "StatComponent|BuffDebuff")
		void ClearAllDebuffs();

	/** Call from owner */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "StatComponent|BuffDebuff")
		void SetEffect(E_Effect Type, FSetEffectStruct Effect, AActor* EffectCauser);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "StatComponent|BuffDebuff")
		void ClearAllBuffs();

	/**Make action by name function 
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "StatComponent|Action")
		void MakeAction(FName ActionName);

	UFUNCTION(BlueprintCallable, Category = "StatComponent|Action")
	FORCEINLINE TArray<AcppEffect*> GetBuffs() { return BuffsArray; }

	UFUNCTION(BlueprintCallable, Category = "StatComponent|Action")
	FORCEINLINE TArray<AcppEffect*> GetDeBuffs() { return DebuffsArray; }

	UFUNCTION()
		void OnRep_BuffsArray();

	UFUNCTION()
		void OnRep_DebuffsArray();


private:

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilitySystemComponent|Skills")
		void IncreaseComboIndex();

	UFUNCTION()
		void ClearDynamicTimers();

	UFUNCTION(BlueprintCallable, Category = "AbilitySystemComponent|Buffs")
		bool AddEffectToArray(E_Effect Type, TArray<AcppEffect*> &EffectArray, AcppEffect* Effect, AActor* &EffectCauser, E_EffectTypes IncomingType);

	/**Async load DataAsset
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AbilitySystemComponent|InitAbylitySystem")
		void SyncLoadAbilityDataAsset(FName AbilityID, E_AbilityTypeToLoad AbilityToLoad, int32 AbilityIndexArray = 0, FName CategoryNameToLoad = "Skills");

#pragma endregion =====<FUNCTIONS>=====

#pragma region =====<DELEGATES>=====

public:
	/**Delegate event for Blueprints
	*/
	UPROPERTY(BlueprintAssignable, Category = "AbilitySystemComponent|Delegates")
		FInited OnAbilitySystemInited;
		
	UPROPERTY(BlueprintAssignable, Category = "AbilitySystemComponent|Delegates")
		FAbilityAdded OnActiveAbilityToSlot;

	UPROPERTY(BlueprintAssignable, Category = "AbilitySystemComponent|Delegates")
		FAbilityAdded OnPassiveAbilityToSlot;

	UPROPERTY(BlueprintAssignable, Category = "AbilitySystemComponent|Delegates")
		FComboDelegate OnComboStepEnd;

	UPROPERTY(BlueprintAssignable, Category = "AbilitySystemComponent|Delegates")
		FComboDelegate OnComboReseted;

	UPROPERTY(BlueprintAssignable, Category = "AbilitySystemComponent|Delegates")
		FDebuffAS OnDebuffArrayUpdated;

	UPROPERTY(BlueprintAssignable, Category = "AbilitySystemComponent|Delegates")
		FBuffAS	OnBuffArrayUpdated;

#pragma endregion =====<DELEGATES>=====

#pragma region =====<BINDS>=====

	UFUNCTION(Category = "AbilitySystemComponent|Bind")
		void BIND_SERVER_AddEffect();

#pragma endregion =====<BINDS>=====


#pragma region =====<MAIN>=====

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#pragma endregion =====<MAIN>=====		
};
