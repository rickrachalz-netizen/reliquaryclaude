// RELIQUARY — tiny shared helpers for the native zero-content widgets.
// These live in a named namespace as inline functions (not per-file anonymous
// namespaces) because unity builds merge multiple widget .cpps into one
// translation unit, where duplicate anonymous-namespace definitions collide.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"
#include "RLDataSubsystem.h"
#include "RLDataTypes.h"

namespace RLWidgets
{
	/** Construct a styled text block in the given tree. */
	inline UTextBlock* MakeText(UWidgetTree* Tree, const FString& Text, int32 FontSize = 12, bool bBold = false)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Block->SetText(FText::FromString(Text));
		Block->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", FontSize));
		return Block;
	}

	/** DT_Items display name for an item id, falling back to the raw name. */
	inline FText ItemDisplayName(const URLDataSubsystem* Data, FName ItemId)
	{
		if (Data)
		{
			if (const FRLItemRow* Row = Data->FindItem(ItemId))
			{
				if (!Row->DisplayName.IsEmpty())
				{
					return Row->DisplayName;
				}
			}
		}
		return FText::FromName(ItemId);
	}
}
