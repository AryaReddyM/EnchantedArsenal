#include "SpellInstance.h"
#include "SpellData.h"
#include "EnchantedArsenal/Character/ArsenalCharacter.h"

AArsenalCharacter* USpellInstance::GetOwningCharacter() const {
	return Cast<AArsenalCharacter>(GetOuter());
}

void USpellInstance::Initialize(USpellData* InData) {
	Data = InData;
}