#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Spell.generated.h"

enum class ESpellType : uint8;
class USpellData;

UCLASS()
class ENCHANTEDARSENAL_API ASpell : public AActor
{
	GENERATED_BODY()
	
public:
	ASpell();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
	
	void InitFromData();

	UPROPERTY(VisibleAnywhere) 
	USceneComponent* Root = nullptr;
	
	UPROPERTY(VisibleAnywhere) 
	UStaticMeshComponent* MeshComp = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spell Properties", ReplicatedUsing = OnRep_SpellType)
	ESpellType SpellType;
	UFUNCTION()
	void OnRep_SpellType();
	
	UPROPERTY(EditAnywhere)
	USpellData* Data;
	
};