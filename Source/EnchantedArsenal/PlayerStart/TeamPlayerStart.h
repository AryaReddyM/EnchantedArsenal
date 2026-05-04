#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "TeamPlayerStart.generated.h"

enum class ETeam : uint8;

UCLASS()
class ENCHANTEDARSENAL_API ATeamPlayerStart : public APlayerStart {
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	ETeam Team;
};