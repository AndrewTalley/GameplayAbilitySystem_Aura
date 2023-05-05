// Copyright Notice


#include "Player/AuraPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Interaction/EnemyInterface.h"

AAuraPlayerController::AAuraPlayerController()
{
	// Replication = Changes on the server are replicated down to all clients.
	bReplicates = true;
}

void AAuraPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	CursorTrace();
}

void AAuraPlayerController::CursorTrace()
{
	FHitResult CursorHit;
	GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, false, CursorHit);
	if (!CursorHit.bBlockingHit) return;

	LastActor = ThisActor;
	ThisActor = Cast<IEnemyInterface>(CursorHit.GetActor());
	
	/**
	* Line trace from cursor. There are several scenarios:
	* A. LastActor is null && ThisActor is null
	*	- Do nothing
	* B. LastActor is null && ThisActor is valid
	*	- Highlight ThisActor
	* C. LastActor is valid && ThisActor is null
	*	- UnHighlight LastActor
	* D. Both Actors are valid, but LastActor != ThisActor
	*	- UnHighlight LastActor, and Highlight ThisActor
	* E. Both Actors are valid, and are the same actor
	*	- Do nothing
	*/

	if (LastActor == nullptr)
	{
		if (ThisActor != nullptr)
		{
			// Case B - LastActor is null && ThisActor is valid
			ThisActor->HighlightActor();
		}
		else
		{
			// Case A - both are null
			// Do Nothing
		}
	}
	else // LastActor is valid
	{
		if (ThisActor == nullptr)
		{
			// Case C - LastActor is valid && ThisActor is null
			LastActor->UnHighlightActor();
		}
		else // Both Actors are valid
		{
			if (LastActor != ThisActor)
			{
				// Case D - Both Actors are valid, but LastActor != ThisActor
				LastActor->UnHighlightActor();
				ThisActor->HighlightActor();
			}
			else
			{
				// Case E - Both Actors are valid, and are the same actor
				// Do Nothing
			}
		}
	}
}

void AAuraPlayerController::BeginPlay()
{
	Super::BeginPlay();
	check(AuraContext);

	/*******************
	* Enhanced Input
	* We are getting the subsystem to allow us to use the Enhanced Input funcationality.
	* Allowing the Mouse Cursor to be shown on the screen.
	* Never Lock or Hide the mouse cursor.
	********************/
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	check(Subsystem);
	Subsystem->AddMappingContext(AuraContext, 0);

	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	FInputModeGameAndUI InputModeData;
	InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputModeData.SetHideCursorDuringCapture(false);
	SetInputMode(InputModeData);
}

void AAuraPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	/*******************
	* Enhanced Input - Bind
	* Casting: A cast is an operator that forces one data type to be converted into another data type.
	* InputCompnent is being casted to UEnhancedInputComponent type.
	* Binds the Move Function to the Move Action.
	********************/

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);

	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAuraPlayerController::Move);
}

void AAuraPlayerController::Move(const FInputActionValue& InputActionValue)
{
	/*******************
	* Enhanced Input - Move
	* Rotation = Getting the Control Rotation.
	* YawRotation = Pitch (X): 0, Yaw (Y): Rotation from Player Controller, Roll (Z): 0
	* ForwardDirection & RightDirection return vectors of the X & Y Axis (World Direction).
	* APawn::AddMovementInput = Add movement input along the given world direction vector.
	* Scaling by X & Y of InputAxisVector. Moving Left or Down create a negative scaling value.
	********************/

	const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		ControlledPawn->AddMovementInput(ForwardDirection, InputAxisVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, InputAxisVector.X);
	}
}