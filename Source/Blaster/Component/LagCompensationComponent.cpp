// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"
#include "../Character/BlasterCharacter.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "../Weapon/Weapon.h"
#include "../Blaster.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}


void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();


}


void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ���������� �����ؾߵ�
	SaveFramePackage();
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : Character;

	if (Character)
	{
		// ���������� ����ϴ� GetWorld()->GetTimeSeconds()�� ȣ���ϸ� ���� �ð��� �޾ƿ´�.
		Package.Time = GetWorld()->GetTimeSeconds();
		Package.Character = Character;
		for (auto& BoxPair : Character->GetHitColisionBoxes())
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();
			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime)
{
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	// 0���� 1 ���̰� �Ǿ����
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Distance, 0.f, 1.f);

	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;

	for (auto& YoungerPair : YoungerFrame.HitBoxInfo)
	{
		const FName& BoxInfoName = YoungerPair.Key;

		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox = YoungerFrame.HitBoxInfo[BoxInfoName];

		FBoxInformation InterpBoxInfo;
		// ��ŸŸ�� ��ġ�� 1�� ������ ��ŸŸ���� ������� �ʴ´ٴ°�
		// Hit�� ���̷� �� Older, Young�� �����Ͽ� Hit �κ��� �߷�����.
		InterpBoxInfo.Location = FMath::VInterpTo(OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction);
		InterpBoxInfo.Rotation = FMath::RInterpTo(OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction);
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent;

		InterpFramePackage.HitBoxInfo.Add(BoxInfoName, InterpBoxInfo);
	}

	return InterpFramePackage;
}

// �ش� �Լ��� ���� Package�Ű������� ������ HitPackage�̴�
FServerSideRewindResult ULagCompensationComponent::ConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter, 
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation)
{
	if (HitCharacter == nullptr)
		return FServerSideRewindResult();

	FFramePackage CurrentFrame;
	// ���� �������� �ڽ� �������� �����ϱ�����(���߿� �ǵ����� �ʿ�)
	CacheBoxPositions(HitCharacter, CurrentFrame);
	// ������ Package�� ����� ĳ������ �浹ü���� ���� �����Ѵ�.
	MoveBoxes(HitCharacter, Package);
	// �ڽ� �浹 ó���� �ؾ��ϱ� ������ ĳ���� �޽� �浹�ȵǵ��� ����
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// ���� �Ӹ��� ���� �浹�� Ȱ��ȭ
	UBoxComponent* HeadBox = HitCharacter->GetHitColisionBoxFromFName(FName("Head"));
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

	FHitResult ConfirmHitResult;
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;
	UWorld* World = GetWorld();

	if (World)
	{
		World->LineTraceSingleByChannel(ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECC_HitBox
		);
		// ������ ���� ��� �浹ü�� �浹 �ǵ��� �����߱� ������ ���⼭ �浹�� �Ǿ��ٸ� ������ ��弦�̴�
		// ��弦�̶�� return
		if (ConfirmHitResult.bBlockingHit)
		{
			if (ConfirmHitResult.Component.IsValid())
			{
				UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
				if (Box)
				{
					DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()),
						FColor::Red, false, 8.f);
				}
			}

			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			// ���ߵǾ��� ��弦�̴ٶ�� ����
			return FServerSideRewindResult{ true, true };
		}
		// ��弦�� �ƴ϶�� �������� üũ
		else
		{
			for (auto& HitBoxPair : HitCharacter->GetHitColisionBoxes())
			{
				if (HitBoxPair.Value != nullptr)
				{
					HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
				}
			}
			World->LineTraceSingleByChannel(ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox

			);

			// ��� ������ �κ� ������
			if (ConfirmHitResult.bBlockingHit)
			{
				if (ConfirmHitResult.Component.IsValid())
				{
					UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
					if (Box)
					{
						DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()),
							FColor::Blue, false, 8.f);
					}
				}
				ResetHitBoxes(HitCharacter, CurrentFrame);
				EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
				// ���ߵư� ��弦�� �ƴ϶�� ����
				return FServerSideRewindResult{ true, false };
			}
		}
	}

	// ������ ���
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	// ���ߵ��� �ʾҰ� ��弦�� �ƴ϶�� ����
	return FServerSideRewindResult{ false, false };
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit(const FFramePackage& Package, ABlasterCharacter* HitCharacter,
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	if (HitCharacter == nullptr)
		return FServerSideRewindResult();

	FFramePackage CurrentFrame;
	// ���� �������� �ڽ� �������� �����ϱ�����(���߿� �ǵ����� �ʿ�)
	CacheBoxPositions(HitCharacter, CurrentFrame);
	// ������ Package�� ����� ĳ������ �浹ü���� ���� �����Ѵ�.
	MoveBoxes(HitCharacter, Package);
	// �ڽ� �浹 ó���� �ؾ��ϱ� ������ ĳ���� �޽� �浹�ȵǵ��� ����
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	// ���� �Ӹ��� ���� �浹�� Ȱ��ȭ
	UBoxComponent* HeadBox = HitCharacter->GetHitColisionBoxFromFName(FName("Head"));
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

	// �浹ü ��� ����
	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithCollision = true;	// �浹�� �����ϰ� ù ��° ��Ʈ���� �����ϱ� ���� ��θ� ���� �������� �����Դϴ�.
	PathParams.DrawDebugTime = 5.f;			// ����� ������ �Ⱓ
	PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;	// ����� ����� �Ⱓ �ɼ�
	PathParams.LaunchVelocity = InitialVelocity;	// ���� ���� �� �ʱ� �߻� �ӵ��Դϴ�.(����� �ӵ�)
	PathParams.MaxSimTime = MaxRecordTime;			// ���� �߻�ü�� �ִ� �ùķ��̼� �ð�
	PathParams.ProjectileRadius = 5.f;		// �浹�� ������ �� ���Ǵ� �߻�ü �ݰ�. <= 0�̸� �� ������ ��� ���.
	PathParams.SimFrequency = 15.f;			// ǰ�� 10 ~ 30 ���̸� ����
	PathParams.StartLocation = TraceStart;	// ���� ��ġ
	PathParams.TraceChannel = ECC_HitBox;	// ����� Ʈ���̽� ä��
	PathParams.ActorsToIgnore.Add(GetOwner());	//�浹 ���� �� ������ ����

	FPredictProjectilePathResult PathResult;
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	// ��弦 �������
	if (PathResult.HitResult.bBlockingHit)
	{
		if (PathResult.HitResult.Component.IsValid())
		{
			UBoxComponent* Box = Cast<UBoxComponent>(PathResult.HitResult.Component);
			if (Box)
			{
				DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()),
					FColor::Red, false, 8.f);
			}
		}
		// ����⶧���� �ٽ� ���󺹱�
		ResetHitBoxes(HitCharacter, CurrentFrame);
		EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
		// ���ߵǾ��� ��弦�̴ٶ�� ����
		return FServerSideRewindResult{ true, true };
	}
	// ��弦�� �ƴ϶�� �������� üũ
	else
	{
		// ���� �浹 ���� ����
		for (auto& HitBoxPair : HitCharacter->GetHitColisionBoxes())
		{
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}
		}

		UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

		if (PathResult.HitResult.bBlockingHit)
		{
			if (PathResult.HitResult.Component.IsValid())
			{
				UBoxComponent* Box = Cast<UBoxComponent>(PathResult.HitResult.Component);
				if (Box)
				{
					DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()),
						FColor::Blue, false, 8.f);
				}
			}

			ResetHitBoxes(HitCharacter, CurrentFrame);
			EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
			// ���ߵǾ��� ��弦�̴ٶ�� ����
			return FServerSideRewindResult{ true, false };
		}
	}

	// ������ ���
	ResetHitBoxes(HitCharacter, CurrentFrame);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
	// ���ߵǾ��� ��弦�̴ٶ�� ����
	return FServerSideRewindResult{ false, false };
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit(const TArray<FFramePackage>& FramePackages, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations)
{
	for (auto Frame : FramePackages)
	{
		if (Frame.Character == nullptr)
			return FShotgunServerSideRewindResult();
	}

	FShotgunServerSideRewindResult ShotgunResult;

	TArray<FFramePackage> CurrentFrames;
	for (auto Frame : FramePackages)
	{
		FFramePackage CurrentFrame;
		CurrentFrame.Character = Frame.Character;
		CacheBoxPositions(Frame.Character, CurrentFrame);
		MoveBoxes(Frame.Character, Frame);
		// �ڽ� �浹 ó���� �ؾ��ϱ� ������ ĳ���� �޽� �浹�ȵǵ��� ����
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::NoCollision);
		CurrentFrames.Add(CurrentFrame);
	}
	for (auto Frame : FramePackages)
	{
		// ���� �Ӹ��� ���� �浹�� Ȱ��ȭ
		UBoxComponent* HeadBox = Frame.Character->GetHitColisionBoxFromFName(FName("Head"));
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
	}

	UWorld* World = GetWorld();
	// ��弦 üũ
	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

		if (World)
		{
			World->LineTraceSingleByChannel(ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
			);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor());
			if (BlasterCharacter)
			{
				if (ConfirmHitResult.Component.IsValid())
				{
					UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
					if (Box)
					{
						DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()),
							FColor::Red, false, 8.f);
					}
				}
				if (ShotgunResult.HeadShots.Contains(BlasterCharacter))
				{
					ShotgunResult.HeadShots[BlasterCharacter]++;
				}
				else
				{
					ShotgunResult.HeadShots.Emplace(BlasterCharacter, 1);
				}
			}
		}
	}
	// ��� �浹ü�� Ȱ��ȭ ��Ų ���� ��� �ڽ��� ��Ȱ��ȭ�Ѵ�.
	for (auto Frame : FramePackages)
	{
		for (auto& HitBoxPair : Frame.Character->GetHitColisionBoxes())
		{
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}
		}

		UBoxComponent* HeadBox = Frame.Character->GetHitColisionBoxFromFName(FName("Head"));
		HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	// �ٵ� üũ
	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f;

		if (World)
		{
			World->LineTraceSingleByChannel(ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
			);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ConfirmHitResult.GetActor());
			if (BlasterCharacter)
			{
				if (ConfirmHitResult.Component.IsValid())
				{
					UBoxComponent* Box = Cast<UBoxComponent>(ConfirmHitResult.Component);
					if (Box)
					{
						DrawDebugBox(GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()),
							FColor::Blue, false, 8.f);
					}
				}
				if (ShotgunResult.BodyShots.Contains(BlasterCharacter))
				{
					ShotgunResult.BodyShots[BlasterCharacter]++;
				}
				else
				{
					ShotgunResult.BodyShots.Emplace(BlasterCharacter, 1);
				}
			}
		}
	}

	for (auto& Frame : CurrentFrames)
	{
		// ������ ���
		// ������ �����ߴ� �÷��̾� �浹ü�� ���� �������� ������ ����
		ResetHitBoxes(Frame.Character, Frame);
		EnableCharacterMeshCollision(Frame.Character, ECollisionEnabled::QueryAndPhysics);
	}
	return ShotgunResult;
}

void ULagCompensationComponent::CacheBoxPositions(ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage)
{
	if (HitCharacter == nullptr)
		return;

	for (auto& HitBoxPair : HitCharacter->GetHitColisionBoxes())
	{
		if (HitBoxPair.Value != nullptr)
		{
			FBoxInformation BoxInfo;
			BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
			BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
			BoxInfo.BoxExtent = HitBoxPair.Value->GetScaledBoxExtent();
			OutFramePackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
		}
	}
}

void ULagCompensationComponent::MoveBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr)
		return;

	for (auto& HitBoxPair : HitCharacter->GetHitColisionBoxes())
	{
		if (HitBoxPair.Value != nullptr)
		{
			const FBoxInformation* BoxValue = Package.HitBoxInfo.Find(HitBoxPair.Key);

			if (BoxValue)
			{
				HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
				HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
				HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
			}
		}
	}
}

void ULagCompensationComponent::ResetHitBoxes(ABlasterCharacter* HitCharacter, const FFramePackage& Package)
{
	if (HitCharacter == nullptr)
		return;

	for (auto& HitBoxPair : HitCharacter->GetHitColisionBoxes())
	{
		if (HitBoxPair.Value != nullptr)
		{
			const FBoxInformation* BoxValue = Package.HitBoxInfo.Find(HitBoxPair.Key);

			if (BoxValue)
			{
				HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
				HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
				HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled)
{
	if (HitCharacter && HitCharacter->GetMesh())
	{
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
	}

}

void ULagCompensationComponent::SaveFramePackage()
{
	// ���������� �����ؾߵ�
	if (Character == nullptr || !Character->HasAuthority())
		return;

	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
		ShowFramePackage(ThisFrame, FColor::Red);
	}
	else
	{
		// ���� �ֱٿ� ����� �������� �ð����� ���� ������ �������� �ð��� ���ش�.
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;

		while (HistoryLength > MaxRecordTime)
		{
			// ���� ������ ������ ����
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			// ���� ������ �����͸� �����߱� ������ HistoryLength�� �ٽ� �����ؾ��Ѵ�.
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}
		// ������ Length�ð��� Max���� �۴ٸ� ��� FramePackage������ ����
		FFramePackage ThisFrame;
		SaveFramePackage(ThisFrame);
		FrameHistory.AddHead(ThisFrame);
		// �Ʒ� �Լ��� ����� �������� �浹ü�� Ȯ���ϱ� ���Ѱ� �ʿ信 ���� ����
		//ShowFramePackage(ThisFrame, FColor::Red);
	}
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color)
{
	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox(
			GetWorld(),
			BoxInfo.Value.Location,
			BoxInfo.Value.BoxExtent,
			FQuat(BoxInfo.Value.Rotation),
			Color,
			false,
			4.f
		);
	}
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime)
{
	bool bReturn = HitCharacter == nullptr ||
		HitCharacter->GetLagCompensationComponent() == nullptr ||
		HitCharacter->GetLagCompensationComponent()->FrameHistory.GetHead() == nullptr ||
		HitCharacter->GetLagCompensationComponent()->FrameHistory.GetTail() == nullptr;

	if (bReturn)
		return FFramePackage();
	// ���� ���θ� Ȯ���ϱ� ���� ������ ��Ű��
	FFramePackage FrameToCheck;

	bool bShouldInterpolate = true;

	// HitCharacter�� ������ �����丮
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensationComponent()->FrameHistory;
	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float NewestHistoryTime = History.GetHead()->GetValue().Time;

	if (OldestHistoryTime > HitTime)
	{
		// OldestHistoryTime�� HitTime���� ũ�ٸ� ���� �� �ǰ��⸦ �����ϱ⿡�� ������
		return FFramePackage();
	}
	if (OldestHistoryTime == HitTime)
	{
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}

	if (NewestHistoryTime <= HitTime)
	{
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	// Young, Old ��� Head�� ���� �ֽ� �����͸� �����ִ� �����ͷ� ���
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;

	while (Older->GetValue().Time > HitTime)
	{
		if (Older->GetNextNode() == nullptr)
			break;

		// ���⿡ ���Դٸ� Older���� �� �ð��밡 �ڿ��ִٴ� �ǹ��̹Ƿ�
		// Older�� Next���� �����ؾ���(�� ������ �����͸� Ȯ���ؾ���)
		Older = Older->GetNextNode();

		// ������ �ٽ� üũ�ߴµ� �׷��� HitTime���� �ֽ��̶�� Young�����͸� Older �����ͷ� ����
		// ��忡������ �������� �ð��� üũ�ϴ°�
		if (Older->GetValue().Time > HitTime)
		{
			Younger = Older;
		}
	}

	// ���� ���ɼ��� ������ �׷��� üũ�ؾߵ�
	if (Older->GetValue().Time == HitTime)
	{
		FrameToCheck = Older->GetValue();
		bShouldInterpolate = false;
	}

	if (bShouldInterpolate)
	{
		// Young, Old ������ ���� ����
		FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
	}

	FrameToCheck.Character = HitCharacter;

	return FrameToCheck;
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);

	return ConfirmHit(FrameToCheck, HitCharacter, TraceStart, HitLocation);
}

FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind(ABlasterCharacter* HitCharacter, 
	const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FFramePackage FrameToCheck = GetFrameToCheck(HitCharacter, HitTime);

	return ProjectileConfirmHit(FrameToCheck, HitCharacter, TraceStart, InitialVelocity, HitTime);
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind(const TArray<class ABlasterCharacter*>& HitCharacters, 
	const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	TArray<FFramePackage> FramesToCheck;

	for (ABlasterCharacter* HitCharacter : HitCharacters)
	{
		FramesToCheck.Add(GetFrameToCheck(HitCharacter, HitTime));
	}

	return ShotgunConfirmHit(FramesToCheck, TraceStart, HitLocations);
}


void ULagCompensationComponent::ServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, 
	const FVector_NetQuantize& HitLocation, float HitTime, AWeapon* DamageCauser)
{
	FServerSideRewindResult Confirm = ServerSideRewind(HitCharacter, TraceStart, HitLocation, HitTime);

	if (Character && HitCharacter && DamageCauser && Confirm.bHitConfirmed)
	{
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			DamageCauser->GetDamage(),
			Character->Controller,
			DamageCauser,
			UDamageType::StaticClass()
		);
	}
}


void ULagCompensationComponent::ProjectileServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, 
	const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	FServerSideRewindResult Confirm = ProjectileServerSideRewind(HitCharacter, TraceStart, InitialVelocity, HitTime);

	if (Character && HitCharacter && Confirm.bHitConfirmed)
	{
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			Character->GetEquippedWeapon()->GetDamage(),
			Character->Controller,
			Character->GetEquippedWeapon(),
			UDamageType::StaticClass()
		);
	}
}

void ULagCompensationComponent::ShotgunServerScoreRequest_Implementation(const TArray<class ABlasterCharacter*>& HitCharacters,
	const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime)
{
	FShotgunServerSideRewindResult Confirm = ShotgunServerSideRewind(HitCharacters, TraceStart, HitLocations, HitTime);

	for (auto& HitCharacter : HitCharacters)
	{
		if (HitCharacter == nullptr || Character->GetEquippedWeapon() == nullptr || Character == nullptr)
			continue;

		float TotalDamage = 0.f;
		if (Confirm.HeadShots.Contains(HitCharacter))
		{
			float HeadShotDamage = Confirm.HeadShots[HitCharacter] * Character->GetEquippedWeapon()->GetDamage();
			TotalDamage += HeadShotDamage;
		}
		if (Confirm.BodyShots.Contains(HitCharacter))
		{
			float BodyShotDamage = Confirm.BodyShots[HitCharacter] * Character->GetEquippedWeapon()->GetDamage();
			TotalDamage += BodyShotDamage;
		}
		UGameplayStatics::ApplyDamage(
			HitCharacter,
			TotalDamage,
			Character->Controller,
			Character->GetEquippedWeapon(),
			UDamageType::StaticClass()
		);
	}
}