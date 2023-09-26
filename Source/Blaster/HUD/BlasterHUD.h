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
	class UTexture2D* CrosshairCenter = nullptr;
	UPROPERTY()
	class UTexture2D* CrosshairLeft = nullptr;
	UPROPERTY()
	class UTexture2D* CrosshairRight = nullptr;
	UPROPERTY()
	class UTexture2D* CrosshairTop = nullptr;
	UPROPERTY()
	class UTexture2D* CrosshairBottom = nullptr;
	UPROPERTY()
	float CrosshairSpread;	// 십자선을 얼마나 벌려야 하는지 알려주는 값(이동하거나 그럴떄)
	FLinearColor CrosshairsColor;
	FHUDPackage()
		: CrosshairCenter(nullptr),
		CrosshairLeft(nullptr),
		CrosshairRight(nullptr),
		CrosshairTop(nullptr),
		CrosshairBottom(nullptr),
		CrosshairSpread(0.0f),
		CrosshairsColor(FLinearColor::White) // 다른 적절한 기본값으로 설정할 수 있음
	{
	}
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
	void AddWeaponSelectOverlay();

public:
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	UPROPERTY(EditAnywhere, Category = "AnnouncementClass")
	TSubclassOf<class UUserWidget> AnnouncementClass;
	
	UPROPERTY(EditAnywhere, Category = "Chat")
	TSubclassOf<class UUserWidget> ChatOverlayClass;
	
	UPROPERTY(EditAnywhere, Category = "Weapon")
	TSubclassOf<class UUserWidget> WeaponSelectOverlayClass;
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
	float ElimAnnouncementTime = 20.f;

	UFUNCTION()
	void ElimAnnouncementTimerFinished(class UElimAnnouncement* MsgToRemove);

	UPROPERTY()
	TArray<class UElimAnnouncement*> ElimMessages;

	void ElimAnnouncementChangePos(class UCanvasPanelSlot* Slot);

	UPROPERTY()
	class UChatOverlay* ChatOverlay;

	UPROPERTY()
	class UWeaponSelectOverlay* WeaponSelectOverlay;

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
	FORCEINLINE class UCharacterOverlay* GetCharacterOverlay() const { return CharacterOverlay; }
	FORCEINLINE class UAnnouncement* GetAnnouncement() const { return Announcement; }
	FORCEINLINE class UChatOverlay* GetChatOverlay() const { return ChatOverlay; }
	FORCEINLINE class UWeaponSelectOverlay* GetWeaponSelectOverlay() const { return WeaponSelectOverlay; }
};
