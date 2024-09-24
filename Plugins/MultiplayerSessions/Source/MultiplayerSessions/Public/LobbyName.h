// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "LobbyName.generated.h"

UCLASS()
class MULTIPLAYERSESSIONS_API ULobbyName : public UUserWidget
{
	GENERATED_BODY()

public:
	void UpdateLobby(FString Username);

	UPROPERTY(meta = (BindWidget))
	UTextBlock* LobbyNameText;

	UPROPERTY(meta = (BindWidget))
	UButton* LobbyJoinButton;
};
