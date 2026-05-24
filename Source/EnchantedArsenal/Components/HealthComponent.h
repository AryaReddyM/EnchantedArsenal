#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class UProgressBar;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeath, AActor*, Damager);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ENCHANTEDARSENAL_API UHealthComponent : public UActorComponent {
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void ApplyDamage(float Damage, AActor* Damager);
	void SetHealthBar(UProgressBar* InHealthBar);
	
	void ResetHealth();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
	float MaxHealth = 250.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, BlueprintReadOnly, Category = "Health")
	float CurrentHealth;
	UFUNCTION()
	void OnRep_CurrentHealth();

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDeath OnDeath;

	UPROPERTY()
	UProgressBar* HealthBar;
};
