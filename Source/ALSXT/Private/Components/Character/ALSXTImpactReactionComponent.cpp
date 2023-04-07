// MIT

#include "Components/Character/ALSXTImpactReactionComponent.h"
#include "Utility/ALSXTStructs.h"
#include "ALSXTCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RandomStream.h"
#include "Utility/AlsMacros.h"
#include "Interfaces/ALSXTCollisionInterface.h"

// Sets default values for this component's properties
UALSXTImpactReactionComponent::UALSXTImpactReactionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);

	// ...
}


// Called when the game starts
void UALSXTImpactReactionComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<AALSXTCharacter>(GetOwner());
	AlsCharacter = Cast<AAlsCharacter>(GetOwner());
	
}


// Called every frame
void UALSXTImpactReactionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Character->GetVelocity().Length() > FGenericPlatformMath::Min(ImpactReactionSettings.CharacterBumpDetectionMinimumVelocity, ImpactReactionSettings.ObstacleBumpDetectionMinimumVelocity))
	{
		ObstacleTrace();
	}
	// ...
}

FGameplayTag UALSXTImpactReactionComponent::GetCharacterVelocity()
{
	if (Character->GetLocomotionMode() == AlsLocomotionModeTags::InAir)
	{
		if (Character->GetVelocity().Length() < 175)
		{
			return ALSXTImpactVelocityTags::Slow;
		}
		else if (Character->GetVelocity().Length() >= 175 && Character->GetVelocity().Length() < 350)
		{
			return ALSXTImpactVelocityTags::Moderate;
		}
		else if (Character->GetVelocity().Length() >= 350 && Character->GetVelocity().Length() < 650)
		{
			return ALSXTImpactVelocityTags::Fast;
		}
		else if (Character->GetVelocity().Length() >= 650 && Character->GetVelocity().Length() < 800)
		{
			return ALSXTImpactVelocityTags::Faster;
		}
		else if (Character->GetVelocity().Length() >= 800)
		{
			return ALSXTImpactVelocityTags::TerminalVelocity;
		}
		else
		{
			return FGameplayTag::EmptyTag;
		}
	}
	else
	{
		FGameplayTag CharacterGait = Character->GetDesiredGait();
		if (CharacterGait == AlsGaitTags::Walking)
		{
			return ALSXTImpactVelocityTags::Slow;
		}
		else if (CharacterGait == AlsGaitTags::Walking)
		{
			return ALSXTImpactVelocityTags::Moderate;
		}
		else
		{
			return ALSXTImpactVelocityTags::Fast;
		}
	}
}

bool UALSXTImpactReactionComponent::ShouldRecieveVelocityDamage()
{
	return Character->GetVelocity().Length() >= 650;
}

float UALSXTImpactReactionComponent::GetBaseVelocityDamage()
{
	FVector2D VelocityRange{ 650.0, 2000.0 };
	FVector2D ConversionRange{ 0.0, 100.0 };
	return FMath::GetMappedRangeValueClamped(VelocityRange, ConversionRange, Character->GetVelocity().Length());
}

void UALSXTImpactReactionComponent::ObstacleTrace()
{
	const auto* Capsule{ Character->GetCapsuleComponent() };
	const auto CapsuleScale{ Capsule->GetComponentScale().Z };
	auto CapsuleRadius{ ImpactReactionSettings.BumpDetectionRadius };
	const auto CapsuleHalfHeight{ Capsule->GetScaledCapsuleHalfHeight() };
	const FVector UpVector{ Character->GetActorUpVector() };
	const FVector StartLocation{ Character->GetActorLocation() + (UpVector * CapsuleHalfHeight / 2)};
	TEnumAsByte<EDrawDebugTrace::Type> BumpDebugMode;
	BumpDebugMode = (ImpactReactionSettings.DebugMode) ? EDrawDebugTrace::ForOneFrame : EDrawDebugTrace::None;
	TArray<FHitResult> HitResults;
	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(Character);
	float VelocityLength{ 0.0f };
	float TraceDistance {0.0f};
	FVector2D VelocityRange{ 199.0, 650.0 };
	FVector2D ConversionRange{ 0.0, 1.0 };
	FVector RangedVelocity = Character->GetVelocity();
	VelocityLength = FMath::GetMappedRangeValueClamped(VelocityRange, ConversionRange, Character->GetVelocity().Length());

	if (ImpactReactionSettings.DebugMode)
	{
		FString VelMsg = "Vel: ";
		VelMsg.Append(FString::SanitizeFloat(Character->GetVelocity().Length()));
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, VelMsg);
	}

	if (Character->GetVelocity().Length() > FGenericPlatformMath::Min(ImpactReactionSettings.CharacterBumpDetectionMinimumVelocity, ImpactReactionSettings.ObstacleBumpDetectionMinimumVelocity))
	{
		if (Character->GetLocomotionAction() == AlsLocomotionActionTags::Sliding)
		{
			CapsuleRadius = CapsuleRadius * 2.0;
			TraceDistance = ImpactReactionSettings.MaxSlideToCoverDetectionDistance;
		}
		else if (!Character->GetMesh()->IsPlayingNetworkedRootMotionMontage())
		{
			TraceDistance = ImpactReactionSettings.MaxBumpDetectionDistance;
		}
		const FVector EndLocation{ StartLocation + (Character->GetVelocity() * TraceDistance) };

		if (UKismetSystemLibrary::CapsuleTraceMultiForObjects(GetWorld(), StartLocation, EndLocation, CapsuleRadius, CapsuleHalfHeight / 2, ImpactReactionSettings.BumpTraceObjectTypes, false, IgnoreActors, BumpDebugMode, HitResults, true, FLinearColor::Green, FLinearColor::Red, 5.0f))
		{
			for (FHitResult HitResult : HitResults)
			{
				
				if (UKismetSystemLibrary::DoesImplementInterface(HitResult.GetActor(), UALSXTCollisionInterface::StaticClass()))
				{
					FHitResult OriginHitResult;
					TArray<AActor*> IgnoreActorsOrigin;
					IgnoreActorsOrigin.Add(HitResult.GetActor());

					if (UKismetSystemLibrary::CapsuleTraceSingleForObjects(GetWorld(), HitResult.ImpactPoint, StartLocation, CapsuleRadius, CapsuleHalfHeight / 2, ImpactReactionSettings.BumpTraceObjectTypes, false, IgnoreActorsOrigin, EDrawDebugTrace::None, OriginHitResult, false, FLinearColor::Green, FLinearColor::Red, 5.0f))
					{
						FDoubleHitResult DoubleHitResult;
						DoubleHitResult.HitResult.HitResult = HitResult;
						IALSXTCollisionInterface::Execute_GetActorMass(HitResult.GetActor(), DoubleHitResult.HitResult.Mass);
						IALSXTCollisionInterface::Execute_GetActorVelocity(HitResult.GetActor(), DoubleHitResult.HitResult.Velocity);
						DoubleHitResult.OriginHitResult.HitResult = OriginHitResult;
						IALSXTCollisionInterface::Execute_GetActorMass(Character, DoubleHitResult.OriginHitResult.Mass);
						IALSXTCollisionInterface::Execute_GetActorVelocity(Character, DoubleHitResult.OriginHitResult.Velocity);

						if (UKismetSystemLibrary::DoesImplementInterface(HitResult.GetActor(), UALSXTCharacterInterface::StaticClass()))
						{
							IALSXTCharacterInterface::Execute_BumpReaction(HitResult.GetActor(), DoubleHitResult, Character->GetDesiredGait(), ALSXTImpactSideTags::Right, ALSXTImpactFormTags::Blunt);
						}
						else if (UKismetSystemLibrary::DoesImplementInterface(HitResult.GetActor(), UALSXTCollisionInterface::StaticClass()))
						{
							IALSXTCollisionInterface::Execute_OnActorImpactCollision(HitResult.GetActor(), DoubleHitResult);
						}

					}

					AALSXTCharacter* HitIsCharacter = Cast<AALSXTCharacter>(HitResult.GetActor());
					if (HitIsCharacter) 
					{
						// ...
					}
					else
					{

					}
					
				}

				if (ImpactReactionSettings.DebugMode)
				{
					FString BumpHit = "Bump: ";
					BumpHit.Append(HitResult.GetActor()->GetName());
					GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, BumpHit);
				}
			}
			
			
		}
	}
	else
	{
		TraceDistance = 0.0f;
		return;
	}
}

// ImpactReaction State
void UALSXTImpactReactionComponent::SetImpactReactionState(const FALSXTImpactReactionState& NewImpactReactionState)
{
	const auto PreviousImpactReactionState{ ImpactReactionState };

	ImpactReactionState = NewImpactReactionState;

	OnImpactReactionStateChanged(PreviousImpactReactionState);

	if ((Character->GetLocalRole() == ROLE_AutonomousProxy) && Character->IsLocallyControlled())
	{
		ServerSetImpactReactionState(NewImpactReactionState);
	}
}

void UALSXTImpactReactionComponent::ServerSetImpactReactionState_Implementation(const FALSXTImpactReactionState& NewImpactReactionState)
{
	SetImpactReactionState(NewImpactReactionState);
}

void UALSXTImpactReactionComponent::ServerProcessNewImpactReactionState_Implementation(const FALSXTImpactReactionState& NewImpactReactionState)
{
	ProcessNewImpactReactionState(NewImpactReactionState);
}

void UALSXTImpactReactionComponent::OnReplicate_ImpactReactionState(const FALSXTImpactReactionState& PreviousImpactReactionState)
{
	OnImpactReactionStateChanged(PreviousImpactReactionState);
}

void UALSXTImpactReactionComponent::OnImpactReactionStateChanged_Implementation(const FALSXTImpactReactionState& PreviousImpactReactionState) {}

// Anticipation Reaction
void UALSXTImpactReactionComponent::AnticipationReaction(const FGameplayTag& Velocity, const FGameplayTag& Side, const FGameplayTag& Form, FVector AnticipationPoint)
{
	// ...
}

void UALSXTImpactReactionComponent::SyncedAnticipationReaction(FVector AnticipationPoint)
{
	// ...
}

// Defensive Reaction
void UALSXTImpactReactionComponent::DefensiveReaction(const FGameplayTag& Velocity, const FGameplayTag& Side, const FGameplayTag& Form, FVector AnticipationPoint)
{
	// ...
}

// Crown Navigation Reaction
void UALSXTImpactReactionComponent::CrowdNavigationReaction(const FGameplayTag& Gait, const FGameplayTag& Side, const FGameplayTag& Form)
{
	// ...
}

// Bump Reaction
void UALSXTImpactReactionComponent::BumpReaction(const FGameplayTag& Gait, const FGameplayTag& Side, const FGameplayTag& Form)
{
	// ...
}

// Impact Reaction
void UALSXTImpactReactionComponent::ImpactReaction(FDoubleHitResult Hit)
{
	StartImpactReaction(Hit);
}

void UALSXTImpactReactionComponent::AttackReaction(FAttackDoubleHitResult Hit)
{
	// if (GetOwnerRole() == ROLE_AutonomousProxy)
	if (Character->GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerAttackReaction(Hit);
	}
	else if (Character->GetLocalRole() == ROLE_SimulatedProxy && Character->GetRemoteRole() == ROLE_Authority)
	{
		StartAttackReaction(Hit);
		// MulticastAttackReaction(Hit);
	}
}

void UALSXTImpactReactionComponent::SyncedAttackReaction(int Index)
{
	// if (GetOwnerRole() == ROLE_AutonomousProxy)
	if (Character->GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerSyncedAttackReaction(Index);
	}
	else if (Character->GetLocalRole() == ROLE_SimulatedProxy && Character->GetRemoteRole() == ROLE_Authority)
	{
		StartSyncedAttackReaction(Index);
		// MulticastSyncedAttackReaction(Index);
	}
}

void UALSXTImpactReactionComponent::ClutchImpactPoint(FDoubleHitResult Hit)
{
	// ...
}

void UALSXTImpactReactionComponent::ImpactFall(FDoubleHitResult Hit)
{
	// ...
}

void UALSXTImpactReactionComponent::AttackFall(FAttackDoubleHitResult Hit)
{
	// ...
}

void UALSXTImpactReactionComponent::SyncedAttackFall(int32 Index)
{
	// ...
}

void UALSXTImpactReactionComponent::BraceForImpact()
{
	// ...
}

void UALSXTImpactReactionComponent::ImpactGetUp(FDoubleHitResult Hit)
{
	// ...
}

void UALSXTImpactReactionComponent::AttackGetUp(FAttackDoubleHitResult Hit)
{
	// ...
}

void UALSXTImpactReactionComponent::SyncedAttackGetUp(int32 Index)
{
	// ...
}

void UALSXTImpactReactionComponent::ImpactResponse(FDoubleHitResult Hit)
{
	// ...
}

void UALSXTImpactReactionComponent::AttackResponse(FAttackDoubleHitResult Hit)
{
	// ...
}

void UALSXTImpactReactionComponent::BodyFallReaction(FAttackDoubleHitResult Hit)
{
	// ...
}

void UALSXTImpactReactionComponent::ImpactTimelineUpdate(float Value)
{
	//...
}

// Error Checks
bool UALSXTImpactReactionComponent::IsAnticipationReactionAllowedToStart(const UAnimMontage* Montage) const
{
	return (Montage != nullptr);
}

bool UALSXTImpactReactionComponent::IsDefensiveReactionAllowedToStart(const UAnimMontage* Montage) const
{
	return (Montage != nullptr);
}

bool UALSXTImpactReactionComponent::IsCrowdNavigationReactionAllowedToStart(const UAnimMontage* Montage) const
{
	return (Montage != nullptr);
}

bool UALSXTImpactReactionComponent::IsBumpReactionAllowedToStart(const UAnimMontage* Montage) const
{
	return (Montage != nullptr);
}

bool UALSXTImpactReactionComponent::IsImpactReactionAllowedToStart(const UAnimMontage* Montage) const
{
	return (Montage != nullptr);
}

bool UALSXTImpactReactionComponent::IsAttackReactionAllowedToStart(const UAnimMontage* Montage) const
{
	return (Montage != nullptr);
}

bool UALSXTImpactReactionComponent::IsSyncedAttackReactionAllowedToStart(const UAnimMontage* Montage) const
{
	return (Montage != nullptr);
}

bool UALSXTImpactReactionComponent::IsClaspImpactPointAllowedToStart(const UAnimMontage* Montage) const
{
	return (Montage != nullptr);
}

bool UALSXTImpactReactionComponent::IsBumpFallAllowedToStart(const UAnimMontage* Montage) const
{
	return (Montage != nullptr);
}

bool UALSXTImpactReactionComponent::IsImpactFallAllowedToStart(const UAnimMontage* Montage) const
{
	return (Montage != nullptr);
}

bool UALSXTImpactReactionComponent::IsAttackFallAllowedToStart(const UAnimMontage* Montage) const
{
	return (Montage != nullptr);
}

bool UALSXTImpactReactionComponent::IsSyncedAttackFallAllowedToStart(const UAnimMontage* Montage) const
{
	return (Montage != nullptr);
}

bool UALSXTImpactReactionComponent::IsImpactResponseAllowedToStart(const UAnimMontage* Montage) const
{
	return (Montage != nullptr);
}

bool UALSXTImpactReactionComponent::IsAttackResponseAllowedToStart(const UAnimMontage* Montage) const
{
	return (Montage != nullptr);
}

// Start Events
void UALSXTImpactReactionComponent::StartAnticipationReaction(const FGameplayTag& Velocity, const FGameplayTag& Side, const FGameplayTag& Form, FVector AnticipationPoint)
{
	// ...
}

void UALSXTImpactReactionComponent::StartDefensiveReaction(const FGameplayTag& Velocity, const FGameplayTag& Side, const FGameplayTag& Form, FVector AnticipationPoint)
{
	// ...
}

void UALSXTImpactReactionComponent::StartBumpReaction(const FGameplayTag& Gait, const FGameplayTag& Side, const FGameplayTag& Form)
{
	// ...
}

void UALSXTImpactReactionComponent::StartCrowdNavigationReaction(const FGameplayTag& Gait, const FGameplayTag& Side, const FGameplayTag& Form)
{
	// ...
}

void UALSXTImpactReactionComponent::StartImpactReaction(FDoubleHitResult Hit)
{
	if (Character->GetLocalRole() <= ROLE_SimulatedProxy)
	{
		return;
	}
	UAnimMontage* Montage{ nullptr };
	TSubclassOf<AActor> ParticleActor{ nullptr };
	UNiagaraSystem* Particle{ nullptr };
	USoundBase* Audio{ nullptr };

	FImpactReactionAnimation ImpactReactionAnimation = SelectImpactReactionMontage(Hit);
	Montage = ImpactReactionAnimation.Montage.Montage;
	Particle = GetImpactReactionParticle(Hit);
	Audio = GetImpactReactionSound(Hit).Sound.Sound;

	if (!ALS_ENSURE(IsValid(Montage)) || !IsImpactReactionAllowedToStart(Montage))
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Montage Invalid"));
		return;
	}

	const auto StartYawAngle{ UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(Character->GetActorRotation().Yaw)) };

	// Clear the character movement mode and set the locomotion action to mantling.

	Character->SetMovementModeLocked(true);

	if (Character->GetLocalRole() >= ROLE_Authority)
	{
		Character->GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Disabled;
		// Character->GetMesh()->SetRelativeLocationAndRotation(BaseTranslationOffset, BaseRotationOffset);
		MulticastStartImpactReaction(Hit, Montage, ParticleActor, Particle, Audio);
	}
	else
	{
		Character->GetCharacterMovement()->FlushServerMoves();

		StartImpactReactionImplementation(Hit, Montage, ParticleActor, Particle, Audio);
		ServerStartImpactReaction(Hit, Montage, ParticleActor, Particle, Audio);
		OnImpactReactionStarted(Hit);
	}
}

void UALSXTImpactReactionComponent::StartAttackReaction(FAttackDoubleHitResult Hit)
{
	// if (Character->GetLocalRole() <= ROLE_SimulatedProxy)
	// {
	// 	return;
	// }
	
	UAnimMontage* Montage{ nullptr };
	FAttackReactionAnimation SelectedAttackReaction = SelectAttackReactionMontage(Hit);
	Montage = SelectedAttackReaction.Montage.Montage;

	if (!ALS_ENSURE(IsValid(Montage)) || !IsImpactReactionAllowedToStart(Montage))
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("Montage Invalid"));
		return;
	}

	UNiagaraSystem* Particle = GetImpactReactionParticle(Hit.DoubleHitResult);
	TSubclassOf<AActor> ParticleActor = GetImpactReactionParticleActor(Hit.DoubleHitResult);
	USoundBase* Audio = GetImpactReactionSound(Hit.DoubleHitResult).Sound.Sound;
	const auto StartYawAngle{ UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(Character->GetActorRotation().Yaw)) };

	Character->SetMovementModeLocked(true);

	// Character->GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Disabled;
		// Character->GetMesh()->SetRelativeLocationAndRotation(BaseTranslationOffset, BaseRotationOffset);
	ImpactReactionParameters.BaseDamage = Hit.BaseDamage;
	ImpactReactionParameters.PlayRate = SelectedAttackReaction.Montage.PlayRate;
	// ImpactReactionParameters.TargetYawAngle = TargetYawAngle;
	ImpactReactionParameters.ImpactType = Hit.DoubleHitResult.ImpactType;
	// ImpactReactionParameters.Stance = Stance;
	ImpactReactionParameters.ImpactVelocity = Hit.Strength;
	ImpactReactionParameters.ImpactReactionAnimation.Montage.Montage = Montage;
	FALSXTImpactReactionState NewImpactReactionState;
	NewImpactReactionState.ImpactReactionParameters = ImpactReactionParameters;
	SetImpactReactionState(NewImpactReactionState);

	// StartImpactReactionImplementation(Hit.DoubleHitResult, Montage, ParticleActor, Particle, Audio);

	if (Character->GetLocalRole() >= ROLE_Authority)
	{
		ServerStartImpactReaction(Hit.DoubleHitResult, Montage, ParticleActor, Particle, Audio);
	}
	else
	{
		Character->GetCharacterMovement()->FlushServerMoves();
		MulticastStartImpactReaction(Hit.DoubleHitResult, Montage, ParticleActor, Particle, Audio);
		OnImpactReactionStarted(Hit.DoubleHitResult);
	}
}

void UALSXTImpactReactionComponent::StartSyncedAttackReaction(int32 Index)
{
	// ...
}

void UALSXTImpactReactionComponent::StartClutchImpactPoint(FDoubleHitResult Hit)
{
	// ...
}

void UALSXTImpactReactionComponent::StartImpactFall(FDoubleHitResult Hit)
{
	// ...
}

void UALSXTImpactReactionComponent::StartAttackFall(FAttackDoubleHitResult Hit)
{
	// ...
}

void UALSXTImpactReactionComponent::StartSyncedAttackFall(int32 Index)
{
	// ...
}

void UALSXTImpactReactionComponent::StartBraceForImpact()
{
	// ...
}

void UALSXTImpactReactionComponent::StartImpactGetUp(FDoubleHitResult Hit)
{
	// ...
}

void UALSXTImpactReactionComponent::StartAttackGetUp(FAttackDoubleHitResult Hit)
{
	// ...
}

void UALSXTImpactReactionComponent::StartSyncedAttackGetUp(int32 Index)
{
	// ...
}

void UALSXTImpactReactionComponent::StartImpactResponse(FDoubleHitResult Hit)
{
	// ...
}

void UALSXTImpactReactionComponent::StartAttackResponse(FAttackDoubleHitResult Hit)
{
	// ...
}

void UALSXTImpactReactionComponent::BumpFall() {}

//Parameters
UALSXTImpactReactionSettings* UALSXTImpactReactionComponent::SelectImpactReactionSettings_Implementation()
{
	return nullptr;
}

FAnticipationPose UALSXTImpactReactionComponent::SelectAnticipationMontage_Implementation(const FGameplayTag& Strength, const FGameplayTag& Side, const FGameplayTag& Form, const FGameplayTag& Health)
{
	FAnticipationPose SelectedAnticipationPose;
	return SelectedAnticipationPose;
}

FAnticipationPose UALSXTImpactReactionComponent::SelectDefensiveMontage_Implementation(const FGameplayTag& Strength, const FGameplayTag& Side, const FGameplayTag& Form, const FGameplayTag& Health)
{
	FAnticipationPose SelectedAnticipationPose;
	return SelectedAnticipationPose;
}

FBumpReactionAnimation UALSXTImpactReactionComponent::SelectBumpReactionMontage_Implementation(const FGameplayTag& Velocity, const FGameplayTag& Side, const FGameplayTag& Form)
{
	FBumpReactionAnimation SelectedMontage;
	return SelectedMontage;
}

FBumpReactionAnimation UALSXTImpactReactionComponent::SelectCrowdNavigationReactionMontage_Implementation(const FGameplayTag& Velocity, const FGameplayTag& Side, const FGameplayTag& Form)
{
	FBumpReactionAnimation SelectedMontage;
	return SelectedMontage;
}

FAttackReactionAnimation UALSXTImpactReactionComponent::SelectAttackReactionMontage_Implementation(FAttackDoubleHitResult Hit)
{
	UALSXTImpactReactionSettings* SelectedImpactReactionSettings = SelectImpactReactionSettings();
	TArray<FAttackReactionAnimation> Montages = SelectedImpactReactionSettings->AttackReactionAnimations;
	TArray<FAttackReactionAnimation> FilteredMontages;
	FAttackReactionAnimation SelectedAttackReactionAnimation;
	// TArray<FGameplayTag> TagsArray = { Hit.Strength, Hit.DoubleHitResult.ImpactSide, Hit.DoubleHitResult.ImpactForm };
	TArray<FGameplayTag> TagsArray = { ALSXTActionStrengthTags::Light, ALSXTImpactSideTags::Left, ALSXTImpactFormTags::Blunt };
	FGameplayTagContainer TagsContainer = FGameplayTagContainer::CreateFromArray(TagsArray);

	// Return is there are no Montages
	if (Montages.Num() < 1 || !Montages[0].Montage.Montage)
	{
		return SelectedAttackReactionAnimation;
	}

	// Filter Montages based on Tag parameters
	for (auto Montage : Montages)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(Montage.ImpactStrength);
		CurrentTagsContainer.AppendTags(Montage.ImpactSide);
		CurrentTagsContainer.AppendTags(Montage.ImpactForm);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			FilteredMontages.Add(Montage);
		}
	}

	// Return if there are no filtered Montages
	if (FilteredMontages.Num() < 1 || !FilteredMontages[0].Montage.Montage)
	{
		return SelectedAttackReactionAnimation;
	}

	// If more than one result, avoid duplicates
	if (FilteredMontages.Num() > 1)
	{
		// If FilteredMontages contains LastAttackReactionAnimation, remove it from FilteredMontages array to avoid duplicates
		if (FilteredMontages.Contains(LastAttackReactionAnimation))
		{
			int IndexToRemove = FilteredMontages.Find(LastAttackReactionAnimation);
			FilteredMontages.RemoveAt(IndexToRemove, 1, true);
		}

		//Shuffle Array
		for (int m = FilteredMontages.Num() - 1; m >= 0; --m)
		{
			int n = FMath::Rand() % (m + 1);
			if (m != n) FilteredMontages.Swap(m, n);
		}

		// Select Random Array Entry
		int RandIndex = FMath::RandRange(0, (FilteredMontages.Num() - 1));
		SelectedAttackReactionAnimation = FilteredMontages[RandIndex];
		LastAttackReactionAnimation = SelectedAttackReactionAnimation;
		return SelectedAttackReactionAnimation;
	}
	else
	{
		SelectedAttackReactionAnimation = FilteredMontages[0];
		LastAttackReactionAnimation = SelectedAttackReactionAnimation;
		return SelectedAttackReactionAnimation;
	}
	return SelectedAttackReactionAnimation;
}

FImpactReactionAnimation UALSXTImpactReactionComponent::SelectImpactReactionMontage_Implementation(FDoubleHitResult Hit)
{
	UALSXTImpactReactionSettings* SelectedImpactReactionSettings = SelectImpactReactionSettings();
	TArray<FImpactReactionAnimation> Montages = SelectedImpactReactionSettings->ImpactReactionAnimations;
	TArray<FImpactReactionAnimation> FilteredMontages;
	FImpactReactionAnimation SelectedAttackReactionAnimation;
	// TArray<FGameplayTag> TagsArray = { Hit.Strength, Hit.DoubleHitResult.ImpactSide, Hit.DoubleHitResult.ImpactForm };
	TArray<FGameplayTag> TagsArray = { ALSXTActionStrengthTags::Light, ALSXTImpactSideTags::Left, ALSXTImpactFormTags::Blunt };
	FGameplayTagContainer TagsContainer = FGameplayTagContainer::CreateFromArray(TagsArray);

	// Return is there are no Montages
	if (Montages.Num() < 1 || !Montages[0].Montage.Montage)
	{
		return SelectedAttackReactionAnimation;
	}

	// Filter Montages based on Tag parameters
	for (auto Montage : Montages)
	{
		FGameplayTagContainer CurrentTagsContainer;
		CurrentTagsContainer.AppendTags(Montage.ImpactVelocity);
		CurrentTagsContainer.AppendTags(Montage.ImpactSide);
		CurrentTagsContainer.AppendTags(Montage.ImpactForm);

		if (CurrentTagsContainer.HasAll(TagsContainer))
		{
			FilteredMontages.Add(Montage);
		}
	}

	// Return if there are no filtered Montages
	if (FilteredMontages.Num() < 1 || !FilteredMontages[0].Montage.Montage)
	{
		return SelectedAttackReactionAnimation;
	}

	// If more than one result, avoid duplicates
	if (FilteredMontages.Num() > 1)
	{
		// If FilteredMontages contains LastAttackReactionAnimation, remove it from FilteredMontages array to avoid duplicates
		if (FilteredMontages.Contains(LastImpactReactionAnimation))
		{
			int IndexToRemove = FilteredMontages.Find(LastImpactReactionAnimation);
			FilteredMontages.RemoveAt(IndexToRemove, 1, true);
		}

		//Shuffle Array
		for (int m = FilteredMontages.Num() - 1; m >= 0; --m)
		{
			int n = FMath::Rand() % (m + 1);
			if (m != n) FilteredMontages.Swap(m, n);
		}

		// Select Random Array Entry
		int RandIndex = FMath::RandRange(0, (FilteredMontages.Num() - 1));
		SelectedAttackReactionAnimation = FilteredMontages[RandIndex];
		LastImpactReactionAnimation = SelectedAttackReactionAnimation;
		return SelectedAttackReactionAnimation;
	}
	else
	{
		SelectedAttackReactionAnimation = FilteredMontages[0];
		LastImpactReactionAnimation = SelectedAttackReactionAnimation;
		return SelectedAttackReactionAnimation;
	}
	return SelectedAttackReactionAnimation;
}

FSyncedAttackAnimation UALSXTImpactReactionComponent::GetSyncedMontage_Implementation(int Index)
{
	UALSXTCombatSettings* SelectedCombatSettings = IALSXTCombatInterface::Execute_GetCombatSettings(this);
	TArray<FSyncedAttackAnimation> Montages = SelectedCombatSettings->SyncedAttackAnimations;
	TArray<FSyncedAttackAnimation> FilteredMontages;

	if (ALS_ENSURE(IsValid(Montages[Index].SyncedMontage.TargetSyncedMontage.Montage)))
	{
		// FSyncedAttackAnimation SelectedSyncedAttackReactionAnimation = Montages[Index];
		return Montages[Index];
	}
	else
	{
		FSyncedAttackAnimation EmptySyncedAttackAnimation;
		return EmptySyncedAttackAnimation;
	}

}

FImpactReactionAnimation UALSXTImpactReactionComponent::SelectClaspImpactPointMontage_Implementation(FDoubleHitResult Hit)
{
	FImpactReactionAnimation SelectedMontage;
	return SelectedMontage;
}

FAnticipationPose UALSXTImpactReactionComponent::SelectSteadyMontage_Implementation(const FGameplayTag& Side)
{
	FAnticipationPose SelectedMontage;
	return SelectedMontage;
}

FActionMontageInfo UALSXTImpactReactionComponent::SelectImpactFallMontage_Implementation(FDoubleHitResult Hit)
{
	FActionMontageInfo SelectedMontage{ nullptr };
	return SelectedMontage;
}

FActionMontageInfo UALSXTImpactReactionComponent::SelectAttackFallMontage_Implementation(FDoubleHitResult Hit)
{
	FActionMontageInfo SelectedMontage{ nullptr };
	return SelectedMontage;
}

UAnimMontage* UALSXTImpactReactionComponent::SelectImpactFallenPose_Implementation(FDoubleHitResult Hit)
{
	UAnimMontage* SelectedMontage{ nullptr };
	return SelectedMontage;
}

UAnimMontage* UALSXTImpactReactionComponent::SelectAttackFallenPose_Implementation(FDoubleHitResult Hit)
{
	UAnimMontage* SelectedMontage{ nullptr };
	return SelectedMontage;
}

FActionMontageInfo UALSXTImpactReactionComponent::SelectImpactGetUpMontage_Implementation(FDoubleHitResult Hit)
{
	FActionMontageInfo SelectedMontage{ nullptr };
	return SelectedMontage;
}

FActionMontageInfo UALSXTImpactReactionComponent::SelectAttackGetUpMontage_Implementation(FDoubleHitResult Hit)
{
	FActionMontageInfo SelectedMontage{ nullptr };
	return SelectedMontage;
}

FResponseAnimation UALSXTImpactReactionComponent::SelectImpactResponseMontage_Implementation(FAttackDoubleHitResult Hit)
{
	FResponseAnimation SelectedMontage;
	return SelectedMontage;
}

FResponseAnimation UALSXTImpactReactionComponent::SelectAttackResponseMontage_Implementation(FAttackDoubleHitResult Hit)
{
	FResponseAnimation SelectedMontage;
	return SelectedMontage;
}

// RPCs

void UALSXTImpactReactionComponent::ServerBumpReaction_Implementation(const FGameplayTag& Gait, const FGameplayTag& Side, const FGameplayTag& Form)
{
	MulticastBumpReaction(Gait, Side, Form);
	Character->ForceNetUpdate();
}

void UALSXTImpactReactionComponent::MulticastBumpReaction_Implementation(const FGameplayTag& Gait, const FGameplayTag& Side, const FGameplayTag& Form)
{
	StartBumpReaction(Gait, Side, Form);
}

void UALSXTImpactReactionComponent::ServerCrowdNavigationReaction_Implementation(const FGameplayTag& Gait, const FGameplayTag& Side, const FGameplayTag& Form)
{
	MulticastCrowdNavigationReaction(Gait, Side, Form);
	Character->ForceNetUpdate();
}

void UALSXTImpactReactionComponent::MulticastCrowdNavigationReaction_Implementation(const FGameplayTag& Gait, const FGameplayTag& Side, const FGameplayTag& Form)
{
	StartCrowdNavigationReaction(Gait, Side, Form);
}

void UALSXTImpactReactionComponent::ServerAnticipationReaction_Implementation(const FGameplayTag& Velocity, const FGameplayTag& Side, const FGameplayTag& Form, FVector AnticipationPoint)
{
	MulticastAnticipationReaction(Velocity, Side, Form, AnticipationPoint);
	Character->ForceNetUpdate();
}

void UALSXTImpactReactionComponent::MulticastAnticipationReaction_Implementation(const FGameplayTag& Velocity, const FGameplayTag& Side, const FGameplayTag& Form, FVector AnticipationPoint)
{
	StartAnticipationReaction(Velocity, Side, Form, AnticipationPoint);
}

void UALSXTImpactReactionComponent::ServerDefensiveReaction_Implementation(const FGameplayTag& Velocity, const FGameplayTag& Side, const FGameplayTag& Form, FVector AnticipationPoint)
{
	MulticastDefensiveReaction(Velocity, Side, Form, AnticipationPoint);
	Character->ForceNetUpdate();
}

void UALSXTImpactReactionComponent::MulticastDefensiveReaction_Implementation(const FGameplayTag& Velocity, const FGameplayTag& Side, const FGameplayTag& Form, FVector AnticipationPoint)
{
	StartDefensiveReaction(Velocity, Side, Form, AnticipationPoint);
}

void UALSXTImpactReactionComponent::ServerImpactReaction_Implementation(FDoubleHitResult Hit)
{
	MulticastImpactReaction(Hit);
	Character->ForceNetUpdate();
}

void UALSXTImpactReactionComponent::MulticastImpactReaction_Implementation(FDoubleHitResult Hit)
{
	StartImpactReaction(Hit);
}

void UALSXTImpactReactionComponent::ServerAttackReaction_Implementation(FAttackDoubleHitResult Hit)
{
	// MulticastAttackReaction(Hit);
	StartAttackReaction(Hit);
	Character->ForceNetUpdate();
}

bool UALSXTImpactReactionComponent::ServerAttackReaction_Validate(FAttackDoubleHitResult Hit)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastAttackReaction_Implementation(FAttackDoubleHitResult Hit)
{
	StartAttackReaction(Hit);
}

void UALSXTImpactReactionComponent::ServerSyncedAttackReaction_Implementation(int32 Index)
{
	// MulticastSyncedAttackReaction(Index);
	StartSyncedAttackReaction(Index);
	Character->ForceNetUpdate();
}

bool UALSXTImpactReactionComponent::ServerSyncedAttackReaction_Validate(int32 Index)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastSyncedAttackReaction_Implementation(int32 Index)
{
	StartSyncedAttackReaction(Index);
}

//

void UALSXTImpactReactionComponent::ServerClutchImpactPoint_Implementation(FDoubleHitResult Hit)
{
	// MulticastStartClutchImpactPoint(Hit);
	StartClutchImpactPoint(Hit);
	Character->ForceNetUpdate();
}

bool UALSXTImpactReactionComponent::ServerClutchImpactPoint_Validate(FDoubleHitResult Hit)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastClutchImpactPoint_Implementation(FDoubleHitResult Hit)
{
	StartClutchImpactPoint(Hit);
}

void UALSXTImpactReactionComponent::ServerImpactFall_Implementation(FDoubleHitResult Hit)
{
	// MulticastStartImpactFall(Hit);
	StartImpactFall(Hit);
	Character->ForceNetUpdate();
}

bool UALSXTImpactReactionComponent::ServerImpactFall_Validate(FDoubleHitResult Hit)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastImpactFall_Implementation(FDoubleHitResult Hit)
{
	StartImpactFall(Hit);
}

void UALSXTImpactReactionComponent::ServerAttackFall_Implementation(FAttackDoubleHitResult Hit)
{
	// MulticastAttackFall(Hit);
	StartAttackFall(Hit);
	Character->ForceNetUpdate();
}

bool UALSXTImpactReactionComponent::ServerAttackFall_Validate(FAttackDoubleHitResult Hit)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastAttackFall_Implementation(FAttackDoubleHitResult Hit)
{
	StartAttackFall(Hit);
}

void UALSXTImpactReactionComponent::ServerSyncedAttackFall_Implementation(int32 Index)
{
	// MulticastStartClutchImpactPoint(Hit);
	StartSyncedAttackFall(Index);
	Character->ForceNetUpdate();
}

bool UALSXTImpactReactionComponent::ServerSyncedAttackFall_Validate(int32 Index)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastSyncedAttackFall_Implementation(int32 Index)
{
	StartSyncedAttackFall(Index);
}

void UALSXTImpactReactionComponent::ServerBraceForImpact_Implementation()
{
	// MulticastBraceForImpact();
	StartBraceForImpact();
	Character->ForceNetUpdate();
}

bool UALSXTImpactReactionComponent::ServerBraceForImpact_Validate()
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastBraceForImpact_Implementation()
{
	StartBraceForImpact();
}

void UALSXTImpactReactionComponent::ServerImpactFallLand_Implementation(FDoubleHitResult Hit)
{
	// MulticastImpactFallLand(Hit);
	// StartImpactFallLand(Hit);
	Character->ForceNetUpdate();
}

bool UALSXTImpactReactionComponent::ServerImpactFallLand_Validate(FDoubleHitResult Hit)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastImpactFallLand_Implementation(FDoubleHitResult Hit)
{
	// StartImpactFallLand(Hit);
}

void UALSXTImpactReactionComponent::ServerAttackFallLand_Implementation(FAttackDoubleHitResult Hit)
{
	// MulticastAttackFallLand(Hit);
	// StartAttackFallLand(Hit);
	Character->ForceNetUpdate();
}

bool UALSXTImpactReactionComponent::ServerAttackFallLand_Validate(FAttackDoubleHitResult Hit)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastAttackFallLand_Implementation(FAttackDoubleHitResult Hit)
{
	// StartAttackFallLand(Hit);
}

void UALSXTImpactReactionComponent::ServerSyncedAttackFallLand_Implementation(int32 Index)
{
	// MulticastSyncedAttackFallLand(Index);
	// StartSyncedAttackFallLand(Index);
	Character->ForceNetUpdate();
}

bool UALSXTImpactReactionComponent::ServerSyncedAttackFallLand_Validate(int32 Index)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastSyncedAttackFallLand_Implementation(int32 Index)
{
	// StartSyncedAttackFallLand(Index);
}

void UALSXTImpactReactionComponent::ServerImpactGetUp_Implementation(FDoubleHitResult Hit)
{
	// MulticastImpactGetUp(Hit);
	StartImpactGetUp(Hit);
	Character->ForceNetUpdate();
}

bool UALSXTImpactReactionComponent::ServerImpactGetUp_Validate(FDoubleHitResult Hit)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastImpactGetUp_Implementation(FDoubleHitResult Hit)
{
	StartImpactGetUp(Hit);
}

void UALSXTImpactReactionComponent::ServerAttackGetUp_Implementation(FAttackDoubleHitResult Hit)
{
	// MulticastAttackGetUp(Hit);
	StartAttackGetUp(Hit);
	Character->ForceNetUpdate();
}

bool UALSXTImpactReactionComponent::ServerAttackGetUp_Validate(FAttackDoubleHitResult Hit)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastAttackGetUp_Implementation(FAttackDoubleHitResult Hit)
{
	StartAttackGetUp(Hit);
}

void UALSXTImpactReactionComponent::ServerSyncedAttackGetUp_Implementation(int32 Index)
{
	// MulticastSyncedAttackGetUp(Index);
	StartSyncedAttackGetUp(Index);
	Character->ForceNetUpdate();
}

bool UALSXTImpactReactionComponent::ServerSyncedAttackGetUp_Validate(int32 Index)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastSyncedAttackGetUp_Implementation(int32 Index)
{
	StartSyncedAttackGetUp(Index);
}

void UALSXTImpactReactionComponent::ServerImpactResponse_Implementation(FDoubleHitResult Hit)
{
	// MulticastImpactResponse(Hit);
	StartImpactResponse(Hit);
	Character->ForceNetUpdate();
}

bool UALSXTImpactReactionComponent::ServerImpactResponse_Validate(FDoubleHitResult Hit)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastImpactResponse_Implementation(FDoubleHitResult Hit)
{
	StartImpactResponse(Hit);
}

void UALSXTImpactReactionComponent::ServerAttackResponse_Implementation(FAttackDoubleHitResult Hit)
{
	// MulticastAttackResponse(Hit);
	StartAttackResponse(Hit);
	Character->ForceNetUpdate();
}

bool UALSXTImpactReactionComponent::ServerAttackResponse_Validate(FAttackDoubleHitResult Hit)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastAttackResponse_Implementation(FAttackDoubleHitResult Hit)
{
	StartAttackResponse(Hit);
}

// Start RPCs

void UALSXTImpactReactionComponent::ServerStartAnticipationReaction_Implementation(FActionMontageInfo Montage, FVector AnticipationPoint)
{
	if (IsImpactReactionAllowedToStart(Montage.Montage))
	{
		MulticastStartAnticipationReaction(Montage, AnticipationPoint);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartAnticipationReaction_Validate(FActionMontageInfo Montage, FVector AnticipationPoint)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartAnticipationReaction_Implementation(FActionMontageInfo Montage, FVector AnticipationPoint)
{
	StartAnticipationReactionImplementation(Montage, AnticipationPoint);
}

void UALSXTImpactReactionComponent::ServerStartDefensiveReaction_Implementation(FActionMontageInfo Montage, USoundBase* Audio, FVector AnticipationPoint)
{
	if (IsImpactReactionAllowedToStart(Montage.Montage))
	{
		MulticastStartDefensiveReaction(Montage, Audio, AnticipationPoint);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartDefensiveReaction_Validate(FActionMontageInfo Montage, USoundBase* Audio, FVector AnticipationPoint)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartDefensiveReaction_Implementation(FActionMontageInfo Montage, USoundBase* Audio, FVector AnticipationPoint)
{
	StartDefensiveReactionImplementation(Montage, Audio, AnticipationPoint);
}

void UALSXTImpactReactionComponent::ServerStartBumpReaction_Implementation(FDoubleHitResult Hit, FActionMontageInfo Montage, TSubclassOf<AActor> ParticleActor, UNiagaraSystem* Particle, USoundBase* Audio)
{
	if (IsImpactReactionAllowedToStart(Montage.Montage))
	{
	MulticastStartBumpReaction(Hit, Montage, ParticleActor, Particle, Audio);
	Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartBumpReaction_Validate(FDoubleHitResult Hit, FActionMontageInfo Montage, TSubclassOf<AActor> ParticleActor, UNiagaraSystem* Particle, USoundBase* Audio)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartBumpReaction_Implementation(FDoubleHitResult Hit, FActionMontageInfo Montage, TSubclassOf<AActor> ParticleActor, UNiagaraSystem* Particle, USoundBase* Audio)
{
	StartBumpReactionImplementation(Hit, Montage, ParticleActor, Particle, Audio);
}

void UALSXTImpactReactionComponent::ServerStartCrowdNavigationReaction_Implementation(FDoubleHitResult Hit, FActionMontageInfo Montage, TSubclassOf<AActor> ParticleActor, UNiagaraSystem* Particle, USoundBase* Audio)
{
	if (IsImpactReactionAllowedToStart(Montage.Montage))
	{
		MulticastStartCrowdNavigationReaction(Hit, Montage, ParticleActor, Particle, Audio);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartCrowdNavigationReaction_Validate(FDoubleHitResult Hit, FActionMontageInfo Montage, TSubclassOf<AActor> ParticleActor, UNiagaraSystem* Particle, USoundBase* Audio)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartCrowdNavigationReaction_Implementation(FDoubleHitResult Hit, FActionMontageInfo Montage, TSubclassOf<AActor> ParticleActor, UNiagaraSystem* Particle, USoundBase* Audio)
{
	StartCrowdNavigationReactionImplementation(Hit, Montage, ParticleActor, Particle, Audio);
}

void UALSXTImpactReactionComponent::ServerStartImpactReaction_Implementation(FDoubleHitResult Hit, UAnimMontage* Montage, TSubclassOf<AActor> ParticleActor, UNiagaraSystem* Particle, USoundBase* Audio)
{
	if (IsImpactReactionAllowedToStart(Montage))
	{
		MulticastStartImpactReaction(Hit, Montage, ParticleActor, Particle, Audio);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartImpactReaction_Validate(FDoubleHitResult Hit, UAnimMontage* Montage, TSubclassOf<AActor> ParticleActor, UNiagaraSystem* Particle, USoundBase* Audio)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartImpactReaction_Implementation(FDoubleHitResult Hit, UAnimMontage* Montage, TSubclassOf<AActor> ParticleActor, UNiagaraSystem* Particle, USoundBase* Audio)
{
	StartImpactReactionImplementation(Hit, Montage, ParticleActor, Particle, Audio);
}

void UALSXTImpactReactionComponent::ServerStartAttackReaction_Implementation(FAttackDoubleHitResult Hit, FActionMontageInfo Montage, TSubclassOf<AActor> ParticleActor, UNiagaraSystem* Particle, USoundBase* Audio)
{
	if (IsImpactReactionAllowedToStart(Montage.Montage))
	{
		MulticastStartAttackReaction(Hit, Montage, ParticleActor, Particle, Audio);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartAttackReaction_Validate(FAttackDoubleHitResult Hit, FActionMontageInfo Montage, TSubclassOf<AActor> ParticleActor, UNiagaraSystem* Particle, USoundBase* Audio)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartAttackReaction_Implementation(FAttackDoubleHitResult Hit, FActionMontageInfo Montage, TSubclassOf<AActor> ParticleActor, UNiagaraSystem* Particle, USoundBase* Audio)
{
	StartAttackReactionImplementation(Hit, Montage, ParticleActor, Particle, Audio);
}

void UALSXTImpactReactionComponent::ServerStartSyncedAttackReaction_Implementation(FActionMontageInfo Montage)
{
	if (IsImpactReactionAllowedToStart(Montage.Montage))
	{
	MulticastStartSyncedAttackReaction(Montage);
	Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartSyncedAttackReaction_Validate(FActionMontageInfo Montage)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartSyncedAttackReaction_Implementation(FActionMontageInfo Montage)
{
	StartSyncedAttackReactionImplementation(Montage);
}

void UALSXTImpactReactionComponent::ServerStartClutchImpactPoint_Implementation(UAnimMontage* Montage, FVector ImpactPoint)
{
	if (IsImpactReactionAllowedToStart(Montage))
	{
		MulticastStartClutchImpactPoint(Montage, ImpactPoint);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartClutchImpactPoint_Validate(UAnimMontage* Montage, FVector ImpactPoint)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartClutchImpactPoint_Implementation(UAnimMontage* Montage, FVector ImpactPoint)
{
	StartClutchImpactPointImplementation(Montage, ImpactPoint);
}

void UALSXTImpactReactionComponent::ServerStartImpactFall_Implementation(FDoubleHitResult Hit, FActionMontageInfo Montage)
{
	if (IsImpactReactionAllowedToStart(Montage.Montage))
	{
		MulticastStartImpactFall(Hit, Montage);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartImpactFall_Validate(FDoubleHitResult Hit, FActionMontageInfo Montage)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartImpactFall_Implementation(FDoubleHitResult Hit, FActionMontageInfo Montage)
{
	StartImpactFallImplementation(Hit, Montage);
}

void UALSXTImpactReactionComponent::ServerStartAttackFall_Implementation(FAttackDoubleHitResult Hit, FActionMontageInfo Montage)
{
	if (IsImpactReactionAllowedToStart(Montage.Montage))
	{
		MulticastStartAttackFall(Hit, Montage);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartAttackFall_Validate(FAttackDoubleHitResult Hit, FActionMontageInfo Montage)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartAttackFall_Implementation(FAttackDoubleHitResult Hit, FActionMontageInfo Montage)
{
	StartAttackFallImplementation(Hit, Montage);
}

void UALSXTImpactReactionComponent::ServerStartSyncedAttackFall_Implementation(FActionMontageInfo Montage)
{
	if (IsImpactReactionAllowedToStart(Montage.Montage))
	{
		MulticastStartSyncedAttackFall(Montage);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartSyncedAttackFall_Validate(FActionMontageInfo Montage)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartSyncedAttackFall_Implementation(FActionMontageInfo Montage)
{
	StartSyncedAttackFallImplementation(Montage);
}

void UALSXTImpactReactionComponent::ServerStartBraceForImpact_Implementation(UAnimMontage* Montage)
{
	if (IsImpactReactionAllowedToStart(Montage))
	{
		MulticastStartBraceForImpact(Montage);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartBraceForImpact_Validate(UAnimMontage* Montage)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartBraceForImpact_Implementation(UAnimMontage* Montage)
{
	StartBraceForImpactImplementation(Montage);
}

void UALSXTImpactReactionComponent::ServerStartImpactFallLand_Implementation(FDoubleHitResult Hit, FActionMontageInfo Montage)
{
	if (IsImpactReactionAllowedToStart(Montage.Montage))
	{
		MulticastStartImpactFallLand(Hit, Montage);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartImpactFallLand_Validate(FDoubleHitResult Hit, FActionMontageInfo Montage)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartImpactFallLand_Implementation(FDoubleHitResult Hit, FActionMontageInfo Montage)
{
	StartImpactFallLandImplementation(Hit, Montage);
}

void UALSXTImpactReactionComponent::ServerStartAttackFallLand_Implementation(FAttackDoubleHitResult Hit, FActionMontageInfo Montage)
{
	if (IsImpactReactionAllowedToStart(Montage.Montage))
	{
		MulticastStartAttackFallLand(Hit, Montage);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartAttackFallLand_Validate(FAttackDoubleHitResult Hit, FActionMontageInfo Montage)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartAttackFallLand_Implementation(FAttackDoubleHitResult Hit, FActionMontageInfo Montage)
{
	StartAttackFallLandImplementation(Hit, Montage);
}

void UALSXTImpactReactionComponent::ServerStartSyncedAttackFallLand_Implementation(FActionMontageInfo Montage)
{
	if (IsImpactReactionAllowedToStart(Montage.Montage))
	{
		MulticastStartSyncedAttackFallLand(Montage);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartSyncedAttackFallLand_Validate(FActionMontageInfo Montage)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartSyncedAttackFallLand_Implementation(FActionMontageInfo Montage)
{
	StartSyncedAttackFallLandImplementation(Montage);
}

void UALSXTImpactReactionComponent::ServerStartImpactGetUp_Implementation(FDoubleHitResult Hit, FActionMontageInfo Montage)
{
	if (IsImpactReactionAllowedToStart(Montage.Montage))
	{
		MulticastStartImpactGetUp(Hit, Montage);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartImpactGetUp_Validate(FDoubleHitResult Hit, FActionMontageInfo Montage)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartImpactGetUp_Implementation(FDoubleHitResult Hit, FActionMontageInfo Montage)
{
	StartImpactGetUpImplementation(Hit, Montage);
}

void UALSXTImpactReactionComponent::ServerStartAttackGetUp_Implementation(FAttackDoubleHitResult Hit, FActionMontageInfo Montage)
{
	if (IsImpactReactionAllowedToStart(Montage.Montage))
	{
		MulticastStartAttackGetUp(Hit, Montage);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartAttackGetUp_Validate(FAttackDoubleHitResult Hit, FActionMontageInfo Montage)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartAttackGetUp_Implementation(FAttackDoubleHitResult Hit, FActionMontageInfo Montage)
{
	StartAttackGetUpImplementation(Hit, Montage);
}

void UALSXTImpactReactionComponent::ServerStartSyncedAttackGetUp_Implementation(FActionMontageInfo Montage)
{
	if (IsImpactReactionAllowedToStart(Montage.Montage))
	{
		MulticastStartSyncedAttackGetUp(Montage);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartSyncedAttackGetUp_Validate(FActionMontageInfo Montage)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartSyncedAttackGetUp_Implementation(FActionMontageInfo Montage)
{
	StartSyncedAttackGetUpImplementation(Montage);
}

void UALSXTImpactReactionComponent::ServerStartImpactResponse_Implementation(FDoubleHitResult Hit, FActionMontageInfo Montage)
{
	if (IsImpactReactionAllowedToStart(Montage.Montage))
	{
		MulticastStartImpactResponse(Hit, Montage);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartImpactResponse_Validate(FDoubleHitResult Hit, FActionMontageInfo Montage)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartImpactResponse_Implementation(FDoubleHitResult Hit, FActionMontageInfo Montage)
{
	StartImpactResponseImplementation(Hit, Montage);
}

void UALSXTImpactReactionComponent::ServerStartAttackResponse_Implementation(FAttackDoubleHitResult Hit, FActionMontageInfo Montage)
{
	if (IsImpactReactionAllowedToStart(Montage.Montage))
	{
		MulticastStartAttackResponse(Hit, Montage);
		Character->ForceNetUpdate();
	}
}

bool UALSXTImpactReactionComponent::ServerStartAttackResponse_Validate(FAttackDoubleHitResult Hit, FActionMontageInfo Montage)
{
	return true;
}

void UALSXTImpactReactionComponent::MulticastStartAttackResponse_Implementation(FAttackDoubleHitResult Hit, FActionMontageInfo Montage)
{
	StartAttackResponseImplementation(Hit, Montage);
}

// Start Implementations

void UALSXTImpactReactionComponent::StartAnticipationReactionImplementation(FActionMontageInfo Montage, FVector AnticipationPoint)
{
	// ...
}

void UALSXTImpactReactionComponent::StartDefensiveReactionImplementation(FActionMontageInfo Montage, USoundBase* Audio, FVector AnticipationPoint)
{
	// ...
}

void UALSXTImpactReactionComponent::StartBumpReactionImplementation(FDoubleHitResult Hit, FActionMontageInfo Montage, TSubclassOf<AActor> ParticleActor, UNiagaraSystem* Particle, USoundBase* Audio)
{
	// ...
}

void UALSXTImpactReactionComponent::StartCrowdNavigationReactionImplementation(FDoubleHitResult Hit, FActionMontageInfo Montage, TSubclassOf<AActor> ParticleActor, UNiagaraSystem* Particle, USoundBase* Audio)
{
	// ...
}

void UALSXTImpactReactionComponent::StartImpactReactionImplementation(FDoubleHitResult Hit, UAnimMontage* Montage, TSubclassOf<AActor> ParticleActor, UNiagaraSystem* Particle, USoundBase* Audio)
{
	//if (IsImpactReactionAllowedToStart(Montage) && Character->GetMesh()->GetAnimInstance()->Montage_Play(Montage, 1.0f))
	if (IsImpactReactionAllowedToStart(Montage))
	{
		//Anticipation
		FALSXTDefensiveModeState DefensiveModeState;
		DefensiveModeState.Mode = Character->GetDefensiveMode();
		DefensiveModeState.Location = Hit.HitResult.HitResult.Location;
		Character->SetDefensiveModeState(DefensiveModeState);
		// Character->SetFacialExpression();

		Character->GetMesh()->GetAnimInstance()->Montage_Play(Montage, 1.0f);
		// ImpactReactionState.TargetYawAngle = TargetYawAngle;
		FALSXTImpactReactionState CurrentImpactReactionState = GetImpactReactionState();
		// CurrentImpactReactionState.ImpactReactionParameters.TargetYawAngle = TargetYawAngle;
		// CurrentImpactReactionState.ImpactReactionParameters.Target = PotentialAttackTarget;

		UAudioComponent* AudioComponent{ nullptr };

		//Calculate Rotation from Normal Vector
		FVector UpVector = Hit.HitResult.HitResult.GetActor()->GetRootComponent()->GetUpVector();
		FVector NormalVector = Hit.HitResult.HitResult.ImpactNormal;
		FVector RotationAxis = FVector::CrossProduct(UpVector, NormalVector);
		RotationAxis.Normalize();
		float DotProduct = FVector::DotProduct(UpVector, NormalVector);
		float RotationAngle = acosf(DotProduct);
		FQuat Quat = FQuat(RotationAxis, RotationAngle);
		FQuat RootQuat = Hit.HitResult.HitResult.GetActor()->GetRootComponent()->GetComponentQuat();
		FQuat NewQuat = Quat * RootQuat;
		FRotator NewRotation = NewQuat.Rotator();

		if (Audio)
		{
			if (GetWorld()->WorldType == EWorldType::EditorPreview)
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), Audio, Hit.HitResult.HitResult.ImpactPoint,
					1.0f, 1.0f);
			}
			else
			{
				AudioComponent = UGameplayStatics::SpawnSoundAtLocation(GetWorld(), Audio, Hit.HitResult.HitResult.ImpactPoint,
					NewRotation,
					1.0f, 1.0f);
			}
		}
		if (UKismetSystemLibrary::IsValidClass(ParticleActor))
		{
			// ServerSpawnParticleActor(Hit, ParticleActor);
			// MulticastSpawnParticleActor(Hit, ParticleActor);
			SpawnParticleActorImplementation(Hit, ParticleActor);
		}
		Character->SetDesiredPhysicalAnimationMode(ALSXTPhysicalAnimationModeTags::Hit, Hit.HitResult.HitResult.BoneName);
		AlsCharacter->SetLocomotionAction(AlsLocomotionActionTags::HitReaction);
		// Character->GetMesh()->AddImpulseAtLocation(Hit.HitResult.Impulse, Hit.HitResult.HitResult.ImpactPoint, Hit.HitResult.HitResult.BoneName);
		Character->GetMesh()->AddImpulseToAllBodiesBelow(Hit.HitResult.Impulse * 1000, Hit.HitResult.HitResult.BoneName, false, true);
		Character->SetDesiredPhysicalAnimationMode(ALSXTPhysicalAnimationModeTags::None, "pelvis");

		// Character->ALSXTRefreshRotationInstant(StartYawAngle, ETeleportType::None);

		// Crouch(); //Hack
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("IsImpactReactionNOTAllowedToStart"));
	}
}

void UALSXTImpactReactionComponent::StartAttackReactionImplementation(FAttackDoubleHitResult Hit, FActionMontageInfo Montage, TSubclassOf<AActor> ParticleActor, UNiagaraSystem* Particle, USoundBase* Audio)
{
	// ...
}

void UALSXTImpactReactionComponent::StartSyncedAttackReactionImplementation(FActionMontageInfo Montage)
{
	// ...
}

void UALSXTImpactReactionComponent::StartClutchImpactPointImplementation(UAnimMontage* Montage, FVector ImpactPoint)
{
	// ...
}

void UALSXTImpactReactionComponent::StartImpactFallImplementation(FDoubleHitResult Hit, FActionMontageInfo Montage)
{
	// ...
}

void UALSXTImpactReactionComponent::StartAttackFallImplementation(FAttackDoubleHitResult Hit, FActionMontageInfo Montage)
{
	// ...
}

void UALSXTImpactReactionComponent::StartSyncedAttackFallImplementation(FActionMontageInfo Montage)
{
	// ...
}

void UALSXTImpactReactionComponent::StartBraceForImpactImplementation(UAnimMontage* Montage)
{
	// ...
}

void UALSXTImpactReactionComponent::StartImpactFallLandImplementation(FDoubleHitResult Hit, FActionMontageInfo Montage)
{
	// ...
}

void UALSXTImpactReactionComponent::StartAttackFallLandImplementation(FAttackDoubleHitResult Hit, FActionMontageInfo Montage)
{
	// ...
}

void UALSXTImpactReactionComponent::StartSyncedAttackFallLandImplementation(FActionMontageInfo Montage)
{
	// ...
}

void UALSXTImpactReactionComponent::StartImpactGetUpImplementation(FDoubleHitResult Hit, FActionMontageInfo Montage)
{
	// ...
}

void UALSXTImpactReactionComponent::StartAttackGetUpImplementation(FAttackDoubleHitResult Hit, FActionMontageInfo Montage)
{
	// ...
}

void UALSXTImpactReactionComponent::StartSyncedAttackGetUpImplementation(FActionMontageInfo Montage)
{
	// ...
}

void UALSXTImpactReactionComponent::StartImpactResponseImplementation(FDoubleHitResult Hit, FActionMontageInfo Montage)
{
	// ...
}

void UALSXTImpactReactionComponent::StartAttackResponseImplementation(FAttackDoubleHitResult Hit, FActionMontageInfo Montage)
{
	// ...
}

// Spawn Particle Actor

bool UALSXTImpactReactionComponent::ServerSpawnParticleActor_Validate(FDoubleHitResult Hit, TSubclassOf<AActor> ParticleActor)
{
	return true;
}

void UALSXTImpactReactionComponent::ServerSpawnParticleActor_Implementation(FDoubleHitResult Hit, TSubclassOf<AActor> ParticleActor)
{
	SpawnParticleActorImplementation(Hit, ParticleActor);
}

void UALSXTImpactReactionComponent::MulticastSpawnParticleActor_Implementation(FDoubleHitResult Hit, TSubclassOf<AActor> ParticleActor)
{
	if (UKismetSystemLibrary::IsValidClass(ParticleActor))
	{

		//Calculate Rotation from Normal Vector
		FVector UpVector = Hit.HitResult.HitResult.GetActor()->GetRootComponent()->GetUpVector();
		FVector NormalVector = Hit.HitResult.HitResult.ImpactNormal;
		FVector RotationAxis = FVector::CrossProduct(UpVector, NormalVector);
		RotationAxis.Normalize();
		float DotProduct = FVector::DotProduct(UpVector, NormalVector);
		float RotationAngle = acosf(DotProduct);
		FQuat Quat = FQuat(RotationAxis, RotationAngle);
		FQuat RootQuat = Hit.HitResult.HitResult.GetActor()->GetRootComponent()->GetComponentQuat();
		FQuat NewQuat = Quat * RootQuat;
		FRotator NewRotation = NewQuat.Rotator();

		FTransform SpawnTransform = FTransform(NewRotation, Hit.HitResult.HitResult.Location, { 1.0f, 1.0f, 1.0f });
		AActor* SpawnedActor;
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// GetWorld()->SpawnActor<AActor>(ParticleActor->StaticClass(), SpawnTransform, SpawnInfo);
		SpawnedActor = GetWorld()->SpawnActor<AActor>(ParticleActor->StaticClass(), SpawnTransform, SpawnInfo);

		if (IsValid(SpawnedActor))
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, SpawnedActor->GetActorLocation().ToString());
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("SpawnedActor Not Valid"));
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("ParticleActor Invalid"));
	}
}

void UALSXTImpactReactionComponent::SpawnParticleActorImplementation(FDoubleHitResult Hit, TSubclassOf<AActor> ParticleActor)
{
	if (UKismetSystemLibrary::IsValidClass(ParticleActor))
	{

		//Calculate Rotation from Normal Vector
		FVector UpVector = Hit.HitResult.HitResult.GetActor()->GetRootComponent()->GetUpVector();
		FVector NormalVector = Hit.HitResult.HitResult.ImpactNormal;
		FVector RotationAxis = FVector::CrossProduct(UpVector, NormalVector);
		RotationAxis.Normalize();
		float DotProduct = FVector::DotProduct(UpVector, NormalVector);
		float RotationAngle = acosf(DotProduct);
		FQuat Quat = FQuat(RotationAxis, RotationAngle);
		FQuat RootQuat = Hit.HitResult.HitResult.GetActor()->GetRootComponent()->GetComponentQuat();
		FQuat NewQuat = Quat * RootQuat;
		FRotator NewRotation = NewQuat.Rotator();

		FTransform SpawnTransform = FTransform(NewRotation, Hit.HitResult.HitResult.Location, { 1.0f, 1.0f, 1.0f });
		AActor* SpawnedActor;
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		SpawnedActor = GetWorld()->SpawnActor<AActor>(ParticleActor, SpawnTransform, SpawnInfo);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, TEXT("ParticleActor Invalid"));
	}
}

// Refresh

void UALSXTImpactReactionComponent::RefreshAnticipationReaction(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshAnticipationReactionPhysics(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshDefensiveReaction(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshDefensiveReactionPhysics(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshCrowdNavigationReaction(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshCrowdNavigationReactionPhysics(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshBumpReaction(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshBumpReactionPhysics(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshImpactReaction(const float DeltaTime)
{
	if (Character->GetLocomotionAction() != AlsLocomotionActionTags::HitReaction)
	{
		StopImpactReaction();
		Character->ForceNetUpdate();
	}
	else
	{
		RefreshImpactReactionPhysics(DeltaTime);
	}
}

void UALSXTImpactReactionComponent::RefreshImpactReactionPhysics(const float DeltaTime)
{
	float Offset = ImpactReactionSettings.RotationOffset;
	auto ComponentRotation{ Character->GetCharacterMovement()->UpdatedComponent->GetComponentRotation() };
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	auto TargetRotation{ PlayerController->GetControlRotation() };
	TargetRotation.Yaw = TargetRotation.Yaw + Offset;
	TargetRotation.Pitch = ComponentRotation.Pitch;
	TargetRotation.Roll = ComponentRotation.Roll;

	if (ImpactReactionSettings.RotationInterpolationSpeed <= 0.0f)
	{
		TargetRotation.Yaw = ImpactReactionState.ImpactReactionParameters.TargetYawAngle;

		Character->GetCharacterMovement()->MoveUpdatedComponent(FVector::ZeroVector, TargetRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
	else
	{
		TargetRotation.Yaw = UAlsMath::ExponentialDecayAngle(UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(TargetRotation.Yaw)),
			ImpactReactionState.ImpactReactionParameters.TargetYawAngle, DeltaTime,
			ImpactReactionSettings.RotationInterpolationSpeed);

		Character->GetCharacterMovement()->MoveUpdatedComponent(FVector::ZeroVector, TargetRotation, false);
	}
}

void UALSXTImpactReactionComponent::RefreshAttackReaction(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshAttackReactionPhysics(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshSyncedAttackReaction(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshSyncedAttackReactionPhysics(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshBumpFallReaction(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshBumpFallReactionPhysics(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshImpactFallReaction(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshImpactFallReactionPhysics(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshAttackFallReaction(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshAttackFallReactionPhysics(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshSyncedAttackFallReaction(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::RefreshSyncedAttackFallReactionPhysics(const float DeltaTime)
{
	// ...
}

void UALSXTImpactReactionComponent::StopAnticipationReaction()
{
	// ...
}

void UALSXTImpactReactionComponent::StopDefensiveReaction()
{
	// ...
}

void UALSXTImpactReactionComponent::StopCrowdNavigationReaction()
{
	// ...
}

void UALSXTImpactReactionComponent::StopBumpReaction()
{
	// ...
}

void UALSXTImpactReactionComponent::StopImpactReaction()
{
	if (Character->GetLocalRole() >= ROLE_Authority)
	{
		Character->GetCharacterMovement()->NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
	}

	Character->SetMovementModeLocked(false);
	// Character->Cast<UAlsCharacterMovementComponent>(GetCharacterMovement())->SetMovementModeLocked(false);
	// Character->GetCharacterMovement<UAlsCharacterMovementComponent>()->SetMovementModeLocked(false);

	// ULocalPlayer* LocalPlayer = Character->GetWorld()->GetFirstLocalPlayerFromController();
	// 
	// APlayerController* PlayerController = Character->GetWorld()->GetFirstPlayerController();
	// 
	// if (Character->IsPlayerControlled())
	// {
	// 	Character->EnableInput(PlayerController);
	// }

	OnImpactReactionEnded();
}

void UALSXTImpactReactionComponent::StopAttackReaction()
{
	// ...
}

void UALSXTImpactReactionComponent::StopSyncedAttackReaction()
{
	// ...
}

void UALSXTImpactReactionComponent::StopBumpFallReaction()
{
	// ...
}

void UALSXTImpactReactionComponent::StopImpactFallReaction()
{
	// ...
}

void UALSXTImpactReactionComponent::StopAttackFallReaction()
{
	// ...
}

void UALSXTImpactReactionComponent::StopSyncedAttackFallReaction()
{
	// ...
}

void UALSXTImpactReactionComponent::OnImpactReactionEnded_Implementation() {}
