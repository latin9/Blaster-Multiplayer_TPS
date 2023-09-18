// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxExtent;

	FBoxInformation()	:
		Location(FVector::ZeroVector),
		Rotation(FRotator::ZeroRotator),
		BoxExtent(FVector::ZeroVector)
	{

	}
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	// 정보가 저장된 시간
	UPROPERTY()
	float Time;

	// 히트박스 정보를 담은 Map 
	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;

	UPROPERTY()
	class ABlasterCharacter* Character;

	FFramePackage() :
		Time(0.f),
		Character(nullptr)
	{

	}
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;	// 적중 되었는지

	UPROPERTY()
	bool bHeadShot;	// 헤드샷 적중인지

	FServerSideRewindResult()
		: bHitConfirmed(false), 
		bHeadShot(false)
	{
	}

	FServerSideRewindResult(bool InHitConfirmed, bool InHeadShot)
		: bHitConfirmed(InHitConfirmed), bHeadShot(InHeadShot)
	{
	}
};

USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<class ABlasterCharacter*, uint32> HeadShots;
	
	UPROPERTY()
	TMap<class ABlasterCharacter*, uint32> BodyShots;

	FShotgunServerSideRewindResult()
	{
	}
};



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	friend class ABlasterCharacter;

public:	
	ULagCompensationComponent();
protected:
	virtual void BeginPlay() override;
public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	void SaveFramePackage(FFramePackage& Package);
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);
	void CacheBoxPositions(class ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage);
	void MoveBoxes(class ABlasterCharacter* HitCharacter, const FFramePackage& Package);
	void ResetHitBoxes(class ABlasterCharacter* HitCharacter, const FFramePackage& Package);
	void EnableCharacterMeshCollision(class ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);
	void SaveFramePackage();

	// HitScan
	FServerSideRewindResult ConfirmHit(const FFramePackage& Package, class ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation);

	// Projectile
	FServerSideRewindResult ProjectileConfirmHit(const FFramePackage& Package, class ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime);

	// 샷건
	FShotgunServerSideRewindResult ShotgunConfirmHit(const TArray<FFramePackage>& FramePackages,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations);
	FFramePackage GetFrameToCheck(class ABlasterCharacter* HitCharacter, float HitTime);
	
public:
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color);

	// HitScan
	FServerSideRewindResult ServerSideRewind(class ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation, float HitTime);

	// Projectile
	FServerSideRewindResult ProjectileServerSideRewind(class ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime);

	// Shotgun
	FShotgunServerSideRewindResult ShotgunServerSideRewind(const TArray<class ABlasterCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime);

	// HitScan
	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(
		class ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime,
		class AWeapon* DamageCauser
	);

	// Projectile
	UFUNCTION(Server, Reliable)
	void ProjectileServerScoreRequest(
		class ABlasterCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);

	// Shotgun
	UFUNCTION(Server, Reliable)
	void ShotgunServerScoreRequest(
		const TArray<class ABlasterCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations, 
		float HitTime
	);

private:
	UPROPERTY()
	class ABlasterCharacter* Character;

	UPROPERTY()
	class ABlasterPlayerController* Controller;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.f;

public:

		
};
