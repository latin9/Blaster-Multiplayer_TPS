// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
public:
	UPROPERTY()
	class UTexture2D* CrosshairCenter;
	UPROPERTY()
	class UTexture2D* CrosshairLeft;
	UPROPERTY()
	class UTexture2D* CrosshairRight;
	UPROPERTY()
	class UTexture2D* CrosshairTop;
	UPROPERTY()
	class UTexture2D* CrosshairBottom;
	UPROPERTY()
	float CrosshairSpread;	// 십자선을 얼마나 벌려야 하는지 알려주는 값(이동하거나 그럴떄)
	FLinearColor CrosshairsColor;
};

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()


protected:
	virtual void BeginPlay() override;

public:
	virtual void DrawHUD() override;
	void AddCharacterOverlay();
	void AddAnnouncement();
	void AddElimAnnouncement(FString Attacker, FString Victim);
	void AddChatOverlay();

public:
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	UPROPERTY(EditAnywhere, Category = "AnnouncementClass")
	TSubclassOf<class UUserWidget> AnnouncementClass;
	
	UPROPERTY(EditAnywhere, Category = "Chat")
	TSubclassOf<class UUserWidget> ChatOverlayClass;
private:
	UPROPERTY()
	class APlayerController* OwningPlayer;

	FHUDPackage HUDPackage;

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f;
	
	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;
	
	UPROPERTY()
	class UAnnouncement* Announcement;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UElimAnnouncement> ElimAnnouncementClass;

	UPROPERTY(EditAnywhere)
	float ElimAnnouncementTime = 3.f;

	UFUNCTION()
	void ElimAnnouncementTimerFinished(class UElimAnnouncement* MsgToRemove);

	UPROPERTY()
	TArray<class UElimAnnouncement*> ElimMessages;

	void ElimAnnouncementChangePos(class UCanvasPanelSlot* Slot);

	UPROPERTY()
	class UChatOverlay* ChatOverlay;

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
	FORCEINLINE class UCharacterOverlay* GetCharacterOverlay() const { return CharacterOverlay; }
	FORCEINLINE class UAnnouncement* GetAnnouncement() const { return Announcement; }
	FORCEINLINE class UChatOverlay* GetChatOverlay() const { return ChatOverlay; }
};
