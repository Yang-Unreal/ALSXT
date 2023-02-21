// MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utility/ALSXTStructs.h"
#include "ALSXTCharacter.h"
#include "AlsCharacter.h"
#include "Chaos/ChaosEngineInterface.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/AudioComponent.h" 
#include "Settings/ALSXTImpactReactionSettings.h"
#include "State/ALSXTImpactReactionState.h" 
#include "ImpactReactionComponent.generated.h"

class AAlsCharacter;
class AALSXTCharacter;


UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALSXT_API UImpactReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UImpactReactionComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadOnly, Category = "ALS|Als Character", Meta = (AllowPrivateAccess))
	AALSXTCharacter* Character{ Cast<AALSXTCharacter>(GetOwner()) };

	AAlsCharacter* AlsCharacter{ Cast<AAlsCharacter>(GetOwner()) };

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "ALS|Movement System")
	bool CanReact();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "ALS|Als Character")
	UNiagaraSystem* GetImpactReactionParticle(FDoubleHitResult Hit);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "ALS|Als Character")
	USoundBase* GetImpactReactionSound(FDoubleHitResult Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, Meta = (AllowPrivateAccess))
	FALSXTImpactReactionState ImpactReactionState;

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	void ImpactReaction(FDoubleHitResult Hit);

	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	void AttackReaction(FAttackDoubleHitResult Hit);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Als Character")
	UAnimMontage* SelectAttackReactionMontage(FAttackDoubleHitResult Hit);

	UFUNCTION(BlueprintNativeEvent, Category = "ALS|Als Character")
	UAnimMontage* SelectImpactReactionMontage(FDoubleHitResult Hit);

	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	UALSXTImpactReactionSettings* SelectImpactReactionSettings(const FGameplayTag& Location);

	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	UALSXTAttackReactionSettings* SelectAttackReactionSettings(const FGameplayTag& Location);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "ALS|Als Character")
	void OnAttackReactionStarted(FAttackDoubleHitResult Hit);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "ALS|Als Character")
	void OnImpactReactionStarted(FDoubleHitResult Hit);

	// Desired UnarmedAttack

private:
	bool IsImpactReactionAllowedToStart(const UAnimMontage* Montage) const;

	void StartAttackReaction(FAttackDoubleHitResult Hit);
	
	void StartImpactReaction(FDoubleHitResult Hit);

	UFUNCTION(Server, Reliable)
	void ServerAttackReaction(FAttackDoubleHitResult Hit);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastAttackReaction(FAttackDoubleHitResult Hit);

	UFUNCTION(Server, Reliable)
	void ServerImpactReaction(FDoubleHitResult Hit);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastImpactReaction(FDoubleHitResult Hit);

	UFUNCTION(Server, Reliable)
	void ServerStartImpactReaction(FHitResult Hit, UAnimMontage* Montage, UNiagaraSystem* Particle, USoundBase* Audio);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartImpactReaction(FHitResult Hit, UAnimMontage* Montage, UNiagaraSystem* Particle, USoundBase* Audio);

	void StartImpactReactionImplementation(FHitResult Hit, UAnimMontage* Montage, UNiagaraSystem* Particle, USoundBase* Audio);

	void RefreshImpactReaction(float DeltaTime);

	void RefreshImpactReactionPhysics(float DeltaTime);

public:
	UFUNCTION(BlueprintCallable, Category = "ALS|Als Character")
	void StopImpactReaction();

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	void OnImpactReactionEnded();

};
