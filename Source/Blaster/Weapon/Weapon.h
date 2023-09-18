// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "../BlasterType/Team.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	// 초기값 첨 무기 생성될때
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "EquippedSecondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaulMax")
};

UENUM(BlueprintType)
enum class EFireType : uint8
{
	EFT_HitScan UMETA(DisplayName = "HitScanWeapon"),
	EFT_Projectile UMETA(DisplayName = "ProjectileWeapon"),
	EFT_Shotgun UMETA(DisplayName = "ShotgunWeapon"),

	EFT_MAX UMETA(DisplayName = "DefaulMax"),
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override;
	void ShowPickupWidget(bool bShowWidget);
	virtual void Fire(const FVector& HitTarget);
	// 무기 드랍
	virtual void Dropped();
	void AddAmmo(int32 AmmoToAdd);
	void SetHUDAmmo();
	// 샷건 분산 알고리즘
	FVector TraceEndWithScatter(const FVector& HitTarget);
protected:
	virtual void BeginPlay() override;

	virtual void OnWeaponStateSet();
	virtual void OnEquipped();
	virtual void OnDropped();
	virtual void OnEquippedSecondary();

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);


private:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

	// 줌 FOV 관련
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;


	// 다른 사람이 쓰다 버린? 무기를 먹었을때 해당 무기에 남아있는 탄 개수가 다르기 때문
	UPROPERTY(EditAnywhere)
	int32 Ammo;

	// 탄약에서 하나를 뺄 수 있고 이 무기에 유효한 소유자가 있는지 확인할 수 있다.
	// 유효한 소유자가 있는 경우 해당 소유자의 HUD를 업데이트함
	void SpendRound();

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);

	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);

	UPROPERTY(EditAnywhere)
	int32 MagCapacity;

	// Client-Side Predicting Ammo 관련
	//  탄약에 대한 처리되지 않은 서버 요청 수입니다.
	// spendRound에서 증가하고 ClientUpdateAmmo에서 감소합니다.
	// 즉 Sequence는 탄약이 아직 우리?에게 다시 복제되지 않은 데 소비한 라운드 수를 나타낸다?
	int32 Sequence = 0;


	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

	// 분산 시스템 적용할건지
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;

	UPROPERTY(EditAnywhere)
	ETeam Team;

protected:
	UPROPERTY()
	class ABlasterCharacter* BlasterOwnerCharacter;

	UPROPERTY()
	class ABlasterPlayerController* BlasterOwnerController;
	// Trace end with scatter
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f;
	
	// 분산 시스템에 사용할 구체 반지름
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.f;
	
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;
	
	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 40.f;

	UPROPERTY(Replicated, EditAnywhere)
	bool bUseServerSideRewind = false;

	UFUNCTION()
	void OnPingTooHigh(bool bPingTooHigh);
public:
	UPROPERTY(EditAnywhere)
	EFireType FireType;

	// Texture for the weapon crosshairs
	UPROPERTY(EditAnywhere, Category = Crosshairs)
		class UTexture2D* CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
		class UTexture2D* CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
		class UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
		class UTexture2D* CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
		class UTexture2D* CrosshairsBottom;

	// 자동발사
	UPROPERTY(EditAnywhere, Category = Combat)
		float FireDelay = 0.15f;

	UPROPERTY(EditAnywhere, Category = Combat)
		bool bAutomatic = true;

	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;

	bool bDestroyWeapon = false;

	// Enable or Disable Custom Depth
	void EnableCustomDepth(bool bEnable);

public:	
	void SetWeaponState(EWeaponState State);
	FORCEINLINE USphereComponent* GetAreaSphere()	const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	bool IsAmmoEmpty();
	bool IsAmmoFull();
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	FORCEINLINE bool GetUseScatter() const { return bUseScatter; }
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadShotDamage() const { return HeadShotDamage; }
	FORCEINLINE void SetUseServerSideRewind(bool _Enable) { bUseServerSideRewind = _Enable; }
	FORCEINLINE class UWidgetComponent* GetPickupWidget() const { return PickupWidget; }
	FORCEINLINE ETeam GetTeam() const { return Team; }

};
