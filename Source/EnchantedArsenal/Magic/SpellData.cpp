#include "SpellData.h"

float USpellData::GetDamage(const FGameplayTagContainer& ActiveTags) const {
	float Total = Damage;
	for (const FTagModifier& Mod : TagModifiers) {
		if (ActiveTags.HasTag(Mod.Tag)) {
			Total += Mod.BonusDamage;
		}
	}
	return Total;
}