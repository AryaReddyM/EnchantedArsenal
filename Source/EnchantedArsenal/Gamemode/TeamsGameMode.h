#pragma once

#include "CoreMinimal.h"
#include "ArsenalGameMode.h"
#include "TeamsGameMode.generated.h"

UCLASS()
class ENCHANTEDARSENAL_API ATeamsGameMode : public AArsenalGameMode {
	GENERATED_BODY()

public:
	ATeamsGameMode();

	virtual void GenericPlayerInitialization(AController* Controller) override;

	virtual void Logout(AController* Exiting) override;
	
	virtual void HandleMatchHasStarted() override;
	
	void HandleScore(AActor* Killer);
};