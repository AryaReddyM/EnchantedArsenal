// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/VerticalBox.h"
#include "OnlineSessionSettings.h"
#include "LobbyName.h"
#include "MultiplayerSessionsSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Kismet/KismetStringLibrary.h"
#include "Components/Button.h"
#include "LobbyList.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHideMenu);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShowMenu);

UCLASS()
class MULTIPLAYERSESSIONS_API ULobbyList : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void MenuSetup();

protected:
	virtual bool Initialize() override;

public:
	void OnFindSessions(const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);

	void UpdateLobbyList();

	UFUNCTION()
	void JoinClickedSession();

	UFUNCTION()
	void BackButtonClicked();

	UPROPERTY(meta = (BindWidget))
	UVerticalBox* LobbyContainer;

	UPROPERTY(meta = (BindWidget))
	UButton* BackButton;

	FTimerHandle UpdateLobbyTimerHandle;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> LobbyName;

	ULobbyName* LobbyNameRef;

	UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	FString Username;

	bool JoinButtonBinded;

	UPROPERTY(BlueprintAssignable)
	FOnHideMenu HideMenu;
	UPROPERTY(BlueprintAssignable)
	FOnShowMenu ShowMenu;
};
