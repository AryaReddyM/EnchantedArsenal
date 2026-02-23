#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SpellInstance.generated.h"

class USpellData;
class AArsenalCharacter;

UCLASS()
class ENCHANTEDARSENAL_API USpellInstance : public UObject {
	GENERATED_BODY()
	
public:
	UPROPERTY()
	USpellData* Data;
	
	AArsenalCharacter* GetOwningCharacter() const;

	void Initialize(USpellData* InData);
};