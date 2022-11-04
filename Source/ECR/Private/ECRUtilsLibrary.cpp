// Copyleft: All rights reversed


#include "ECRUtilsLibrary.h"

#include "GUI/ECRGUIPlayerController.h"
#include "Kismet/GameplayStatics.h"

AECRGUIPlayerController* UECRUtilsLibrary::GetGUISupervisor(const UWorld* World)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	return Cast<AECRGUIPlayerController>(PlayerController);
}
