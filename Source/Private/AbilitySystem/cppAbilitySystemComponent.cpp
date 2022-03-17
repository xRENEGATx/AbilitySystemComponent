// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/cppAbilitySystemComponent.h"
#include "AbilitySystem/cppDA_WeaponAbility.h"

#include "AbilitySystem/cppEffect.h"

#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GamePlayStatics.h"
#include "Kismet/KismetMathLibrary.h"

#include "Components/SkeletalMeshComponent.h"
#include "Core/cppAVD_GameInstance.h"
#include "StaticFunctionLib/AVD_StaticFunctions.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"

#include "Interfaces/cppInterface_Attack.h"

#include "Engine/ActorChannel.h"

#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY(LogAbilitySystem);
 
// Sets default values for this component's properties
UcppAbilitySystemComponent::UcppAbilitySystemComponent():
	UseAbilitiesManualSetup(false),
	StartAttackTraceSocket("Attack"),
	AttackSpeed(1.0f),
	TimeToResetCombo(5.0f),
	RightAttackNotify("AttackRight"),
	LeftAttackNotify("AttackLeft"),
	SkillActivateNotify("SkillActivate"),
	NextComboNotify("NextCombo"),
	ComboStepEndNotify("ComboStepEnd"),
	TraceRadius(20.0f),
	TraceRadiusFindEnemy(300.0f),
	bDebugAttackTrace(false),
	CurrentComboIndex(0),
	CurrentAbility(nullptr)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;

	//Ability slots init
	if (UseAbilitiesManualSetup == true)
	{
		ActiveAbilitiesArray.SetNum(ActiveAbilitiesDataAssetArray.Num());
		PassiveAbilitiesArray.SetNum(PassiveAbilitiesDataAssetArray.Num());		
	}
	else
	{
		ActiveAbilitiesArray.SetNum(2);
		PassiveAbilitiesArray.SetNum(4);

		ActiveAbilitiesDataAssetArray.SetNum(ActiveAbilitiesArray.Num());
		PassiveAbilitiesDataAssetArray.SetNum(PassiveAbilitiesArray.Num());
	}	

	//Init arays
	DebuffsArray.SetNum(5);
	BuffsArray.SetNum(5);
}

#pragma region =====<REPLICATOR>=====

void UcppAbilitySystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UcppAbilitySystemComponent, ActiveAbility);
	DOREPLIFETIME(UcppAbilitySystemComponent, WeaponAbility);
	DOREPLIFETIME(UcppAbilitySystemComponent, CurrentComboIndex);
	DOREPLIFETIME(UcppAbilitySystemComponent, ActiveAbilitiesArray);
	DOREPLIFETIME(UcppAbilitySystemComponent, PassiveAbilitiesArray);
	DOREPLIFETIME(UcppAbilitySystemComponent, DebuffsArray);
	DOREPLIFETIME(UcppAbilitySystemComponent, BuffsArray);
}

#pragma endregion =====<REPLICATOR>=====

#pragma region =====<FUNCTIONS>=====

//Init AbilitySystemComponent
void UcppAbilitySystemComponent::InitAbilityComponent(FName AbilityID, E_AbilityTypeToLoad AbilityToLoad, int32 AbilityIndexArray, FName CategoryNameToLoad)
{
	UE_LOG(LogAbilitySystem, Log, TEXT("SUCCESS: Sync loading ability begin"));

	SyncLoadAbilityDataAsset(AbilityID, AbilityToLoad, AbilityIndexArray, CategoryNameToLoad);
}

void UcppAbilitySystemComponent::SyncLoadAbilityDataAsset(FName AbilityID, E_AbilityTypeToLoad AbilityToLoad, int32 AbilityIndexArray, FName CategoryNameToLoad)
{
	//<------------<Local variables>-------------------->
	FSoftObjectPath l_RefAtDataAsset;
	
	//<------------<FPrimaryAssetID>-------------------->
	FPrimaryAssetId l_PrimaryAssetID;
	l_PrimaryAssetID.PrimaryAssetType = CategoryNameToLoad;
	l_PrimaryAssetID.PrimaryAssetName = AbilityID;
	
	//<------------<StreamableManagerAndReference>-------------------->
	FStreamableManager &AssetLoader = UAVD_StaticFunctions::GetAVDGameInstance(this)->StreamableManager;
	l_RefAtDataAsset = UKismetSystemLibrary::GetSoftObjectReferenceFromPrimaryAssetId(l_PrimaryAssetID).ToSoftObjectPath();
	
	//<------------<SyncLoadDataAsset>-------------------->
	AssetLoader.RequestSyncLoad(l_RefAtDataAsset);
	
	bool l_BroadCastResult = false;
	UcppDA_WeaponAbility* l_WeaponAbilityDataAsset = nullptr;
	UcppDA_Ability* l_AbilityToAdd = nullptr;

	switch(AbilityToLoad)
	{
		case E_AbilityTypeToLoad::Ability_Weapon:

			l_WeaponAbilityDataAsset = Cast<UcppDA_WeaponAbility>(StaticLoadObject(UcppDA_WeaponAbility::StaticClass(), NULL, *l_RefAtDataAsset.ToString()));

			if (l_WeaponAbilityDataAsset != nullptr)
			{
				WeaponAbility = l_WeaponAbilityDataAsset->SkillData;

				UE_LOG(LogAbilitySystem, Log, TEXT("SUCCESS: Current weapon ability inited"));

				l_BroadCastResult = true;
			}
			else
			{
				UE_LOG(LogAbilitySystem, Error, TEXT("ERROR: Current weapon ability not inited"));
			}

			if (OnAbilitySystemInited.IsBound())
			{
				OnAbilitySystemInited.Broadcast(l_BroadCastResult);
			}

		break;

		case E_AbilityTypeToLoad::Ability_Active:

			l_AbilityToAdd = Cast<UcppDA_Ability>(StaticLoadObject(UcppDA_Ability::StaticClass(), NULL, *l_RefAtDataAsset.ToString()));

			if (l_AbilityToAdd != nullptr)
			{
				ActiveAbilitiesArray[AbilityIndexArray] = l_AbilityToAdd->AbilityContent;

				UE_LOG(LogAbilitySystem, Log, TEXT("SUCCESS: Active ability added to slot"));

			}
			else
			{
				UE_LOG(LogAbilitySystem, Error, TEXT("ERROR: Active ability not added in slot"));
			}

			if (OnActiveAbilityToSlot.IsBound())
			{
				OnActiveAbilityToSlot.Broadcast(ActiveAbilitiesArray[AbilityIndexArray], AbilityIndexArray);
			}

		break;

		case E_AbilityTypeToLoad::Ability_Passive:

			l_AbilityToAdd = Cast<UcppDA_Ability>(StaticLoadObject(UcppDA_Ability::StaticClass(), NULL, *l_RefAtDataAsset.ToString()));

			if (l_AbilityToAdd != nullptr)
			{
				PassiveAbilitiesArray[AbilityIndexArray] = l_AbilityToAdd->AbilityContent;

				UE_LOG(LogAbilitySystem, Log, TEXT("SUCCESS: Passive ability added to slot"));

			}
			else
			{
				UE_LOG(LogAbilitySystem, Error, TEXT("ERROR: Passive ability not added in slot"));
			}

			if (OnPassiveAbilityToSlot.IsBound())
			{
				OnPassiveAbilityToSlot.Broadcast(PassiveAbilitiesArray[AbilityIndexArray], AbilityIndexArray);
			}

		break;

		case E_AbilityTypeToLoad::Ability_None:
			
			//DO NOTHING

		break;
	}
}

void UcppAbilitySystemComponent::AddLoadedActiveAbilityToArray(FAbilityContent Ability, int32 AbilityIndexArray)
{
	ActiveAbilitiesArray[AbilityIndexArray] = Ability;

	if (OnActiveAbilityToSlot.IsBound() == true)
	{
		OnActiveAbilityToSlot.Broadcast(ActiveAbilitiesArray[AbilityIndexArray], AbilityIndexArray);
	}
}

void UcppAbilitySystemComponent::PlayWeaponAbilityMontage(UPARAM(ref) UAnimInstance * const &AnimInstance, E_PlayerAbility Ability)
{
	if (AnimInstance == nullptr)
	{
		UE_LOG(LogAbilitySystem, Error, TEXT("ERROR: AnimInstance is nullptr"));
		return;
	}

	UAnimMontage* l_MontageToPlay = nullptr;
	ActorsToIgnor.Empty(0);

	//Check activated ability
	switch (Ability)
	{
		case E_PlayerAbility::Ability_Block:

			if (WeaponAbility.Block.AnimMontage == nullptr)
			{
				UE_LOG(LogAbilitySystem, Error, TEXT("ERROR: Current Block Ability is nullptr"));
				return;
			}

			//Set montage to play
			l_MontageToPlay = WeaponAbility.Block.AnimMontage;

			//Set Active ability for work with effects via notify
			ActiveAbility = WeaponAbility.Block;

		break;

		case E_PlayerAbility::Ability_Evade:

			if (WeaponAbility.Evade.AnimMontage == nullptr)
			{
				UE_LOG(LogAbilitySystem, Error, TEXT("ERROR: Current Evade Ability is nullptr"));
				return;
			}

			//Set montage to play
			l_MontageToPlay = WeaponAbility.Evade.AnimMontage;

			//Set Active ability for work with effects via notify
			ActiveAbility = WeaponAbility.Evade;

		break;

		case E_PlayerAbility::Ability_Ulta:

			if (WeaponAbility.Ulta.AnimMontage == nullptr)
			{
				UE_LOG(LogAbilitySystem, Error, TEXT("ERROR: Current Ulta Ability is nullptr"));
				return;
			}

			//Set montage to play
			l_MontageToPlay = WeaponAbility.Ulta.AnimMontage;

			//Set Active ability for work with effects via notify
			ActiveAbility = WeaponAbility.Ulta;

		break;

		case E_PlayerAbility::Ability_Combo:

			if (WeaponAbility.Combo.ComboSteps.IsValidIndex(CurrentComboIndex) == false)
			{
				UE_LOG(LogAbilitySystem, Error, TEXT("ERROR: CurrentIndex is not valid. Index is %i"), CurrentComboIndex);
				return;
			}

			//Set montage to play
			l_MontageToPlay = WeaponAbility.Combo.ComboSteps[CurrentComboIndex].AnimMontage;

			if (l_MontageToPlay == nullptr)
			{
				UE_LOG(LogAbilitySystem, Error, TEXT("ERROR: Montage Combo %i is nullptr"), CurrentComboIndex);
				return;
			}

			//Server side
			if (GetOwnerRole() == ROLE_Authority)
			{
				//Stop and invalidate combo reset timer
				if (ResetComboTimer.IsValid())
				{
					GetWorld()->GetTimerManager().ClearTimer(ResetComboTimer);
				}
				
				//Set Active ability for work with effects via notify
				ActiveAbility = WeaponAbility.Combo.ComboSteps[CurrentComboIndex];
				
				if (GetWorld() != nullptr)
				{
					//Start combo reset timer
					GetWorld()->GetTimerManager().SetTimer(ResetComboTimer, this, &UcppAbilitySystemComponent::ResetCombo, TimeToResetCombo);
				}
			}					

		break;
	}

	if (GetOwnerRole() != ROLE_Authority)
	{
		//Turn off move and rotation on client for disable lag
		l_MontageToPlay->bEnableRootMotionRotation = false;
		l_MontageToPlay->bEnableRootMotionTranslation = false;		
	}

	//Play montage
	AnimInstance->Montage_Play(l_MontageToPlay, AttackSpeed);
}

void UcppAbilitySystemComponent::PlayActiveAbilityMontage(UPARAM(ref) UAnimInstance* const &AnimInstance, int32 AbliltyIndex)
{
	if (ActiveAbilitiesArray.Num() > 0 && ActiveAbilitiesArray.IsValidIndex(AbliltyIndex))
	{
		ActiveAbility = ActiveAbilitiesArray[AbliltyIndex].Ability;
		
		if (ActiveAbilitiesArray[AbliltyIndex].Ability.UseAnimMontageForAbility == false)
		{
			if (GetOwnerRole() == ROLE_Authority)
			{
				SkillActivate();
			}

			return;
		}		
	}	

	//Check AnimInstance
	if (AnimInstance == nullptr)
	{
		UE_LOG(LogAbilitySystem, Error, TEXT("ERROR: AnimInstance is nullptr"));
		return;
	}
	
	UAnimMontage* l_MontageToPlay = nullptr;
	l_MontageToPlay = ActiveAbilitiesArray[AbliltyIndex].Ability.AnimMontage;

	//Check Ability montage
	if (l_MontageToPlay == nullptr)
	{
		UE_LOG(LogAbilitySystem, Error, TEXT("ERROR: Ability montage %i is nullptr"), AbliltyIndex);
		return;
	}

	if (GetOwnerRole() != ROLE_Authority)
	{
		//Turn off move and rotation on client for disable lag
		l_MontageToPlay->bEnableRootMotionRotation = false;
		l_MontageToPlay->bEnableRootMotionTranslation = false;
	}

	AnimInstance->Montage_Play(l_MontageToPlay, AttackSpeed);
}

void UcppAbilitySystemComponent::TraceAndMakeDamage(FName AttackName)
{	
	//Check intarface
	IcppInterface_AbilitySystem* l_AbilityInterface = Cast<IcppInterface_AbilitySystem>(GetOwner());
	
	if (l_AbilityInterface == nullptr)
	{
		UE_LOG(LogAbilitySystem, Error, TEXT("ERROR: AbilityInterface not implemented in Actor owner"));
		return;
	}

	//<----------------------<Trace params>----------------------------------->
	FVector l_StartTrace = l_AbilityInterface->Interface_GetTraceStart();
	FVector l_EndTrace;

	if (AttackName == RightAttackNotify)
	{
		l_EndTrace = l_AbilityInterface->Interface_GetRightTraceEnd();
	}
	else
	{
		l_EndTrace = l_AbilityInterface->Interface_GetLeftTraceEnd();
	}
	
	ActorsToIgnor.Add(GetOwner());
	TArray<FHitResult> l_HitedActorArray;
	
	//Calculate debug method
	EDrawDebugTrace::Type l_TraceDebug = (bDebugAttackTrace == true) ? EDrawDebugTrace::Type::ForDuration : EDrawDebugTrace::Type::None;
	//<----------------------<End>----------------------------------->
	
	//Emmit line trace
	UKismetSystemLibrary::SphereTraceMultiForObjects(this, l_StartTrace, l_EndTrace, TraceRadius, DamageTypesArray, false, ActorsToIgnor, l_TraceDebug, l_HitedActorArray, true);

	if (l_HitedActorArray.Num() > 0)
	{
		for (FHitResult &HitResult : l_HitedActorArray)
		{
			if (HitResult.Actor.Get() == nullptr)
			{
				UE_LOG(LogAbilitySystem, Warning, TEXT("WARNING: HittedActor is nullptr"));
				return;
			}

			ActorsToIgnor.Add(HitResult.Actor.Get());
			FDamageEvent DamageEvent;

			IcppInterface_AbilitySystem* l_AttackInterface = Cast<IcppInterface_AbilitySystem>(HitResult.Actor.Get());
			//This checking need for blueprints
			bool l_bInterfaceImpResult = UKismetSystemLibrary::DoesImplementInterface(HitResult.Actor.Get(), UcppInterface_AbilitySystem::StaticClass());

			if (l_AttackInterface == nullptr && l_bInterfaceImpResult == false)
			{
				UE_LOG(LogAbilitySystem, Warning, TEXT("WARNING: InterfaceAbilitySystem not implemented in Target"));
				return;
			}

			//Check AbilityEffect
			if (ActiveAbility.AbilityEffect == nullptr)
			{
				UE_LOG(LogAbilitySystem, Warning, TEXT("WARNING: Skill has no effect"));
			}

			float l_Damage = l_AbilityInterface->Interface_GetDamage() + ActiveAbility.AbilityDamage;
			float l_CritChance = l_AbilityInterface->Interface_GetCritChance() + ActiveAbility.AbilityCritChance;
			float l_CritDamage = l_AbilityInterface->Interface_GetCritDamage() + ActiveAbility.AbilityCritDamage;
			float l_DebuffChance = l_AbilityInterface->Interface_GetDebuffChance();

			FSetEffectStruct l_Debuff;
			l_Debuff.IncomingEffect = ActiveAbility.AbilityEffect;
			l_Debuff.IncomingEffectName = *ActiveAbility.EffectName.ToString();
			l_Debuff.IncomingEffectDescription = *ActiveAbility.EffectDescription.ToString();
			l_Debuff.AbilityIcon = ActiveAbility.AbilityIcon;

			IcppInterface_AbilitySystem::Execute_Interface_ApplyDamage(HitResult.Actor.Get(), GetOwner(), l_Damage, l_CritChance, l_CritDamage, l_DebuffChance, false, l_Debuff);
		}
	}
}

void UcppAbilitySystemComponent::SkillActivate()
{
	if (ActiveAbility.AbilityEffect.Get() == nullptr)
	{
		UE_LOG(LogAbilitySystem, Error, TEXT("ERROR: ActiveAbility has't ability effect"));
		return;
	}

	if (ActiveAbility.AbilityLogic.Get() == nullptr)
	{
		UE_LOG(LogAbilitySystem, Error, TEXT("ERROR: ActiveAbility has't AbilityLogic "));
		return;
	}

	if (CurrentAbility->IsValidLowLevel())
	{
		UE_LOG(LogAbilitySystem, Warning, TEXT("WARNING: CurrentAbility is not nullptr, destroing"));

		//Call end logic if other ability in progress
		CurrentAbility->AbilityEndLogic();
	}

	FActorSpawnParameters l_SpawnParameters;
	l_SpawnParameters.Owner = GetOwner();
	FTransform l_Spawntransform = FTransform(GetOwner()->GetActorRotation(), GetOwner()->GetActorLocation(), FVector(1.0f, 1.0f, 1.0f));

	CurrentAbility = Cast<AcppAbility>(GetWorld()->SpawnActor(ActiveAbility.AbilityLogic.Get(), &l_Spawntransform, l_SpawnParameters));

	if (CurrentAbility == nullptr)
	{
		UE_LOG(LogAbilitySystem, Error, TEXT("ERROR: Can't create New Ability "));
		return;
	}

	FSetEffectStruct l_EffectToActivate;
	l_EffectToActivate.IncomingEffect = ActiveAbility.AbilityEffect;
	l_EffectToActivate.IncomingEffectDescription = *ActiveAbility.EffectDescription.ToString();
	l_EffectToActivate.AbilityIcon = ActiveAbility.AbilityIcon;

	CurrentAbility->AttachToActor(GetOwner(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	CurrentAbility->AbilityStartLogic(GetOwner(), l_EffectToActivate);
}

void UcppAbilitySystemComponent::ResetCombo()
{
	CurrentComboIndex = 0;
}

void UcppAbilitySystemComponent::IncreaseComboIndex()
{
	CurrentComboIndex += 1;

	//Check current combo step and if more than CurrentWeaponAbility->Combo.ComboSteps.Max() reset value
	if (CurrentComboIndex > WeaponAbility.Combo.ComboSteps.Num() - 1)
	{
		CurrentComboIndex = 0;
	}
}

void UcppAbilitySystemComponent::FindEnemyInRadius()
{
	//<----------------------<Trace params>----------------------------------->
	FVector l_StartSphereTrace = GetOwner()->GetActorLocation();
	FVector l_EndSphereTrace = l_StartSphereTrace + 1;
	TArray<AActor*> l_ActorsToIgnoreArray;
	l_ActorsToIgnoreArray.Add(GetOwner());

	//Calculate debug method
	EDrawDebugTrace::Type l_TraceDebug = (bDebugAttackTrace == true) ? EDrawDebugTrace::Type::ForDuration : EDrawDebugTrace::Type::None;
	TArray<FHitResult> l_HitedActorArray;
	//<----------------------<Trace params end>----------------------------------->

	UKismetSystemLibrary::SphereTraceMultiForObjects(this, l_StartSphereTrace, l_EndSphereTrace, TraceRadiusFindEnemy, FindTypesArray, false, l_ActorsToIgnoreArray, l_TraceDebug, l_HitedActorArray, true);

	if (l_HitedActorArray.Num() > 0)
	{
		float l_DistanceToNearestEnemy = 0.0f;
		AActor* l_Enemy = nullptr;

		for (int32 i = 0; i < l_HitedActorArray.Num(); ++i)
		{
			if (i == 0)
			{
				l_DistanceToNearestEnemy = (GetOwner()->GetActorLocation() - l_HitedActorArray[i].Actor.Get()->GetActorLocation()).Size();
				l_Enemy = l_HitedActorArray[i].Actor.Get();
			}
			else if (i != 0)
			{
				float l_DistanceBeatweenOwnerAndEnemy = (GetOwner()->GetActorLocation() - l_HitedActorArray[i].Actor.Get()->GetActorLocation()).Size();

				if (l_DistanceBeatweenOwnerAndEnemy < l_DistanceToNearestEnemy)
				{
					l_DistanceToNearestEnemy = l_DistanceBeatweenOwnerAndEnemy;
					//Clear variable
					l_Enemy = nullptr;
					l_Enemy = l_HitedActorArray[i].Actor.Get();
				}
			}
		}

		if (l_Enemy == nullptr)
		{	
			UE_LOG(LogAbilitySystem, Warning, TEXT("WARNING: Cant find enemy in this radius"));
			return;
		}

		FRotator l_ToEnemy = UKismetMathLibrary::FindLookAtRotation(GetOwner()->GetActorLocation(), l_Enemy->GetActorLocation());
		l_ToEnemy.Pitch = 0.0f;
		l_ToEnemy.Roll = 0.0f;

		GetOwner()->SetActorRotation(l_ToEnemy);
	}
}

void UcppAbilitySystemComponent::SetEffect(E_Effect Type, FSetEffectStruct Effect, AActor* EffectCauser)
{
	if (Effect.IncomingEffect == nullptr || Effect.AbilityIcon == nullptr)
	{
		UE_LOG(LogAbilitySystem, Error, TEXT("ERROR: IncomingEffect or AbilityIcon is nullptr"));
		return;
	}

	E_EffectTypes l_IncomingEffectType = Effect.IncomingEffect.GetDefaultObject()->EffectType;
	FActorSpawnParameters l_SpawnParameters;
	l_SpawnParameters.Owner = GetOwner();
	FTransform l_Spawntransform = GetOwner()->GetActorTransform();
	
	AcppEffect* l_IncomingEffect = nullptr;
	l_IncomingEffect = Cast<AcppEffect>(GetWorld()->SpawnActor(Effect.IncomingEffect.Get(), &l_Spawntransform, l_SpawnParameters));

	if (l_IncomingEffect == nullptr)
	{
		UE_LOG(LogAbilitySystem, Error, TEXT("ERROR: Cant create Effect object"));
		return;
	}

	l_IncomingEffect->AttachToActor(GetOwner(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	l_IncomingEffect->SetFlags(RF_StrongRefOnFrame);

	l_IncomingEffect->SetEffectEffectName(Effect.IncomingEffectName);
	l_IncomingEffect->SetEffectDescription(Effect.IncomingEffectDescription);
	l_IncomingEffect->SetIcon(Effect.AbilityIcon);

	switch (Type)
	{
	case E_Effect::Effect_Buff:
		
		AddEffectToArray(E_Effect::Effect_Buff, BuffsArray, l_IncomingEffect, EffectCauser, l_IncomingEffectType);
		OnRep_BuffsArray();

		break;

	case E_Effect::Effect_Debuff:
		
		AddEffectToArray(E_Effect::Effect_Debuff, DebuffsArray, l_IncomingEffect, EffectCauser, l_IncomingEffectType);
		OnRep_DebuffsArray();

		break;
	}	
}

void UcppAbilitySystemComponent::OnRep_BuffsArray()
{
	//Delegat
	if (OnBuffArrayUpdated.IsBound() == true)
	{
		OnBuffArrayUpdated.Broadcast(BuffsArray);
	}
}

void UcppAbilitySystemComponent::OnRep_DebuffsArray()
{
	//Delegat
	if (OnDebuffArrayUpdated.IsBound() == true)
	{
		OnDebuffArrayUpdated.Broadcast(DebuffsArray);
	}
}

bool UcppAbilitySystemComponent::AddEffectToArray(E_Effect Type, TArray<AcppEffect*> &EffectArray, AcppEffect* Effect, AActor* &EffectCauser, E_EffectTypes IncomingType)
{
	for (int32 i = 0; i < EffectArray.Num(); ++i)
	{
		if (!EffectArray[i]->IsValidLowLevel())
		{
			EffectArray[i] = Effect;
			EffectArray[i]->EffectStartLogic(GetOwner(), EffectCauser, GetOwner());

			UE_LOG(LogAbilitySystem, Log, TEXT("LOG: BUFF ValidLowLevel worked"));
			return true;
		}
		else if (EffectArray[i]->IsValidLowLevel() && IncomingType == EffectArray[i]->EffectType)
		{
			EffectArray[i]->EffectEndLogic();
			EffectArray[i] = Effect;
			EffectArray[i]->EffectStartLogic(GetOwner(), EffectCauser, GetOwner());

			UE_LOG(LogAbilitySystem, Log, TEXT("LOG: Buff rewrited"));
			return true;
		}
		//Check if go through whole array and all elements are full
		else if (i == (EffectArray.Num() - 1))
		{
			if (EffectArray[0]->IsValidLowLevel())
			{
				EffectArray[0]->EffectEndLogic();
				EffectArray[0] = Effect;
				EffectArray[0]->EffectStartLogic(GetOwner(), EffectCauser, GetOwner());
				
				return true;
			}
		}
	}

	return false;
}

void UcppAbilitySystemComponent::ClearAllDebuffs()
{
	for (AcppEffect* &Debuff : DebuffsArray)
	{
		if (Debuff->IsValidLowLevel())
		{
			Debuff->EffectEndLogic();
		}
	}
}

void UcppAbilitySystemComponent::ClearAllBuffs()
{
	for (AcppEffect* &Buff : BuffsArray)
	{
		if (Buff->IsValidLowLevel())
		{
			Buff->EffectEndLogic();
		}		
	}
}

void UcppAbilitySystemComponent::ClearDynamicTimers()
{
	for (size_t i = 0; i < TimerHandleArray.Num(); i++)
	{
		if (TimerHandleArray[i].IsValid() == true)
		{
			GetWorld()->GetTimerManager().ClearTimer(TimerHandleArray[i]);
		}		
	}
}

#pragma endregion =====<FUNCTIONS>=====

#pragma region =====<BINDS>=====

void UcppAbilitySystemComponent::MakeAction(FName ActionName)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		//Attack via line trace
		if (ActionName == RightAttackNotify || ActionName == LeftAttackNotify)
		{
			TraceAndMakeDamage(ActionName);
		}
		//Skill activation
		else if (ActionName == SkillActivateNotify)
		{
			SkillActivate();
		}
		//Next combo step
		else if (ActionName == NextComboNotify)
		{
			IncreaseComboIndex();
		}
		//End combo
		else if (ActionName == ComboStepEndNotify)
		{
			if (OnComboStepEnd.IsBound())
			{
				OnComboStepEnd.Broadcast();
			}
		}
	}	
}

UFUNCTION(Category = "AbilitySystemComponent|Bind")
void UcppAbilitySystemComponent::BIND_SERVER_AddEffect()
{

}

#pragma endregion =====<BINDS>=====

#pragma region =====<MAIN>=====

// Called when the game starts
void UcppAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void UcppAbilitySystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UcppAbilitySystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (GetWorld() != nullptr)
	{
		ClearDynamicTimers();		
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	}	
}

#pragma endregion =====<MAIN>=====
