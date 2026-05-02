#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class UProgressBar;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ENCHANTEDARSENAL_API UHealthComponent : public UActorComponent {
	GENERATED_BODY()

public:
	UHealthComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void ApplyDamage(float Damage);
	void Die();

	void SetHealthBar(UProgressBar* InHealthBar);
	void EnsureHealthBar();

	UFUNCTION()
	void OnRep_CurrentHealth();

	UPROPERTY(EditAnywhere)
	float MaxHealth = 250.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
	float CurrentHealth;

	FOnDeath OnDeath;

	UPROPERTY()
	UProgressBar* HealthBar;
};
