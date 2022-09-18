// Copyleft: All rights reversed


#include "CustomizationSavingNameSpace.h"

// Sets default values for this component's properties
UCustomizationSavingNameSpace::UCustomizationSavingNameSpace()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	SaveDestinationRootDirectory = "";
	// ...
}
