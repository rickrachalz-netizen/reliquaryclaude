#include "RLEssenceShardPickup.h"
#include "RLGameInstance.h"

ARLEssenceShardPickup::ARLEssenceShardPickup()
{
	// A once-ever drop should never be missed: pull from far away.
	MagnetRadius = 1200.f;
}

void ARLEssenceShardPickup::GrantTo(AActor* Collector)
{
	if (URLGameInstance* GI = Cast<URLGameInstance>(GetGameInstance()))
	{
		GI->UnlockEssence(EssenceId);
	}
}
