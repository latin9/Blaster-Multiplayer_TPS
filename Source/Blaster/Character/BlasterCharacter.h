// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../BlasterType/TurningInPlace.h"
#include "../Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PostInitializeComponents()	override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void PlayFireMontage(bool bAiming);
	void PlayElimMontage();
	// 시뮬레이션 프록시에 대한 캐릭터 회전의 델타를 확인할 때 틱 기능 대신 이것을 사용
	virtual void OnRep_ReplicatedMovement() override;

	// 게임 모드는 서버에만 존재하고 서버에서 Elim을 실행하기 때문에 여긴 서버에 해당
	void Elim();
	// 죽는것은 신뢰해야됨 중요한작업
	// 서버에서만 실행되면 안된다.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();
	
	UFUNCTION(Server, Reliable)
	void ServerElimDestroyed();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElimDestroyed();


	virtual void Destroyed() override;

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	// E 버튼 클릭(장비 장착)
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	float CalculateSpeed();
	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimproxiesTurn();
	virtual void Jump() override;
	void FireButtonPressed();
	void FireButtonReleased();
	void PlayHitReactMontage();
	
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	void UpdateHUDHealth();

protected:

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(EditAnywhere, Category = "Player Name")
	FString LocalPlayerName = TEXT("Unknown Player");

	// 서버에서 변경될때 모든 클라이언트에서 변경되고 무기에 대한 포인터를 복제할 수 있다.
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	// LastWeapon에는 복제되기전의 OverlappingWeapon이 들어간다
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat;

	// 서버 RPC를 신뢰할 수 있도록 Reliable 선언
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;

	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* ElimMontage;

	void HideCameraIfCharacterClose();
	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	bool bRotateRootBone;
	float TurnThreshold = 0.25f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

	// Player Health
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	// 현재 체력
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health();

	class ABlasterPlayerController* BlasterPlayerController;

	bool bElimmed = false;

	FTimerHandle ElimTimer;
	// 무조건 초기값으로만 설정가능
	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;
	void ElimTimerFinished();

	// Dissolve Effect
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;

	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;
	
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	// 런타임에 생성하는 동적 인스턴스를 저장하기 위해 사용
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	// 블루프린트에 설정된 머티리얼 인스턴스, 동적 미터리얼 인스턴스와 함께 사용
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

	// Elimbot
	UPROPERTY(EditAnywhere)
	class UParticleSystem* ElimBotEffect;

	UPROPERTY(VisibleAnywhere)
	class UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere)
	class USoundCue* ElimBotSound;


public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget()	const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
public:
	UFUNCTION(Client, Reliable)
	void ClientSetName(const FString& Name);

	UFUNCTION(Server, Reliable)
	void ServerSetPlayerName(const FString& PlayerName);


};
