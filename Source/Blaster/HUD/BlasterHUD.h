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
	class UTexture2D* CrosshairCenter;
	class UTexture2D* CrosshairLeft;
	class UTexture2D* CrosshairRight;
	class UTexture2D* CrosshairTop;
	class UTexture2D* CrosshairBottom;
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
	void AddCharacterOverlay();

public:
	virtual void DrawHUD() override;

public:
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	
private:
	FHUDPackage HUDPackage;

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor CrosshairColor);

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f;

	class UCharacterOverlay* CharacterOverlay;
public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
	FORCEINLINE UCharacterOverlay* GetCharacterOverlay() const { return CharacterOverlay; }
};
