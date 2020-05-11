// Copyright Epic Games, Inc. All Rights Reserved.

#include "BatteryCollectorGameMode.h"
#include "BatteryCollectorCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "SpawnVolume.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PawnMovementComponent.h"

ABatteryCollectorGameMode::ABatteryCollectorGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	PrimaryActorTick.bCanEverTick = true;
	
	Decayrate = 0.01f;
}

void ABatteryCollectorGameMode::BeginPlay()
{
	Super::BeginPlay();

	// find all spawn volume Actors
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnVolume::StaticClass(), FoundActors);

	for (auto Actor : FoundActors)
	{
		ASpawnVolume* SpawnVolumeActor = Cast<ASpawnVolume>(Actor);
		if(SpawnVolumeActor)
		{
			SpawnVolumeActors.AddUnique(SpawnVolumeActor);
		}
	}
	
	SetCurrentState(EBatteryPlayState::EPlaying);

	ABatteryCollectorCharacter* MyCharacter = Cast<ABatteryCollectorCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if(MyCharacter)
	{
		PowerToWin = (MyCharacter->GetInitialPower()) * 1.25f;
	}

	if(HUDWidgetClass != nullptr)
	{
		CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), HUDWidgetClass);
		if(CurrentWidget != nullptr)
		{
			CurrentWidget->AddToViewport();
		}
	}

}


void ABatteryCollectorGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Check that we are using the battery collector character
	ABatteryCollectorCharacter* MyCharacter = Cast<ABatteryCollectorCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if(MyCharacter)
	{
		// player won
		if(MyCharacter->GetCurrentPower() > PowerToWin)
		{
			SetCurrentState(EBatteryPlayState::EWon);
		}
		// if the character's power is positive
		else if(MyCharacter->GetCurrentPower() > 0)
		{
			// decrease the character's power using the decay rate
			MyCharacter->UpdatePower(-DeltaTime * Decayrate * (MyCharacter->GetInitialPower()));
		}
		else
		{
			SetCurrentState(EBatteryPlayState::EGameOver);
		}
	}
}

float ABatteryCollectorGameMode::GetPowerToWin() const
{
	return  PowerToWin;
}

EBatteryPlayState ABatteryCollectorGameMode::GetCurrentState() const
{
	return CurrentState;
}

void ABatteryCollectorGameMode::SetCurrentState(EBatteryPlayState NewState)
{
	CurrentState = NewState;
	HandleNewState(CurrentState);
}

void ABatteryCollectorGameMode::HandleNewState(EBatteryPlayState const NewState)
{
	switch (NewState)
	{
		case EBatteryPlayState::EPlaying:
		{
				for (ASpawnVolume* Volume : SpawnVolumeActors)
				{
					Volume->SetSpawningActive(true);
				}
		}
		break;
		// if we won the game
		case EBatteryPlayState::EWon:
		{
				for (ASpawnVolume* Volume : SpawnVolumeActors)
				{
					Volume->SetSpawningActive(false);
				}
		}
		break;
		// if we lost the game
		case EBatteryPlayState::EGameOver:
		{
				// spawn volume deactivate
				for (ASpawnVolume* Volume : SpawnVolumeActors)
				{
					Volume->SetSpawningActive(false);
				}
				// block player input
				APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
				if(PlayerController)
				{
					PlayerController->SetCinematicMode(true,false, false,true,true);
				}
				// ragdoll the character
				ACharacter* MyCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
				if(MyCharacter)
				{
					MyCharacter->GetMesh()->SetSimulatePhysics(true);
					MyCharacter->GetMovementComponent()->MovementState.bCanJump = false;
				}
				
				
		}
		break;

		default:
		case EBatteryPlayState::EUnknown:
		{}
	}
	
}

