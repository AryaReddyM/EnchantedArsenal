// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "OnlineSessionSettings.h"
#include "LobbyName.generated.h"

class ULobbyList;

UCLASS()
class MULTIPLAYERSESSIONS_API ULobbyName : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const FOnlineSessionSearchResult& InResult, ULobbyList* InOwner);

	UFUNCTION()
	void OnJoinClicked();

	UPROPERTY(meta = (BindWidget))
	UTextBlock* LobbyNameText;

	UPROPERTY(meta = (BindWidget))
	UButton* LobbyJoinButton;

private:
	FOnlineSessionSearchResult Result;

	UPROPERTY()
	ULobbyList* Owner = nullptr;
};
