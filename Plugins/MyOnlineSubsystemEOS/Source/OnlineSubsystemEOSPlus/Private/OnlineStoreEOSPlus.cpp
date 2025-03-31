// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineStoreEOSPlus.h"
#include "OnlineSubsystemEOSPlus.h"
#include "OnlineError.h"

FOnlineStoreEOSPlus::FOnlineStoreEOSPlus(FOnlineSubsystemEOSPlus* InSubsystem)
	: EOSPlus(InSubsystem)
{
	BaseStoreInterface = EOSPlus->BaseOSS->GetStoreV2Interface();

	BasePurchaseInterface = EOSPlus->BaseOSS->GetPurchaseInterface();
}

FOnlineStoreEOSPlus::~FOnlineStoreEOSPlus()
{
	if (BaseStoreInterface.IsValid())
	{
		BaseStoreInterface->ClearOnQueryForAvailablePurchasesCompleteDelegates(this);
	}

	if (BasePurchaseInterface.IsValid())
	{
		BasePurchaseInterface->ClearOnUnexpectedPurchaseReceiptDelegates(this);
	}
}

FUniqueNetIdEOSPlusPtr FOnlineStoreEOSPlus::GetNetIdPlus(const FString& SourceId) const
{
	return EOSPlus->UserInterfacePtr->GetNetIdPlus(SourceId);
}

void FOnlineStoreEOSPlus::Initialize()
{
	if (BaseStoreInterface.IsValid())
	{
		BaseStoreInterface->AddOnQueryForAvailablePurchasesCompleteDelegate_Handle(FOnQueryForAvailablePurchasesCompleteDelegate::CreateThreadSafeSP(this, &FOnlineStoreEOSPlus::OnQueryForAvailablePurchasesCompleteBase));
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::Initialize] BaseStoreInterface delegates not bound. Base interface not valid"));
	}

	if (BasePurchaseInterface.IsValid())
	{
		BasePurchaseInterface->AddOnUnexpectedPurchaseReceiptDelegate_Handle(FOnUnexpectedPurchaseReceiptDelegate::CreateThreadSafeSP(this, &FOnlineStoreEOSPlus::OnUnexpectedPurchaseReceiptBase));
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::Initialize] BasePurchaseInterface delegates not bound. Base interface not valid"));
	}
}

//~ Begin IOnlineStoreV2 Interface

void FOnlineStoreEOSPlus::QueryCategories(const FUniqueNetId& UserId, const FOnQueryOnlineStoreCategoriesComplete& Delegate)
{
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(UserId.ToString());
	if (NetIdPlus.IsValid())
	{
		const bool bIsBaseNetIdValid = ensure(NetIdPlus->GetBaseNetId().IsValid());
		const bool bIsBaseStoreInterfaceValid = BaseStoreInterface.IsValid();
		if (bIsBaseNetIdValid && bIsBaseStoreInterfaceValid)
		{
			BaseStoreInterface->QueryCategories(*NetIdPlus->GetBaseNetId(), Delegate);
			return;
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::QueryCategories] Unable to call method in base interface. IsBaseNetIdValid=%s IsBaseStoreInterfaceValid=%s."), *LexToString(bIsBaseNetIdValid), *LexToString(bIsBaseStoreInterfaceValid));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::QueryCategories] Unable to call method in base interface. Unknown user (%s)"), *UserId.ToString());
	}

	EOSPlus->ExecuteNextTick([this, Delegate]() {
		Delegate.ExecuteIfBound(false, TEXT("Unable to call method in base interface."));
	});
}

void FOnlineStoreEOSPlus::GetCategories(TArray<FOnlineStoreCategory>& OutCategories) const
{
	if (BaseStoreInterface.IsValid())
	{
		BaseStoreInterface->GetCategories(OutCategories);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineStoreEOSPlus::GetCategories] Unable to call method in base interface. Base interface not valid."));
	}
}

void FOnlineStoreEOSPlus::QueryOffersByFilter(const FUniqueNetId& UserId, const FOnlineStoreFilter& Filter, const FOnQueryOnlineStoreOffersComplete& Delegate)
{
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(UserId.ToString());
	if (NetIdPlus.IsValid())
	{
		const bool bIsBaseNetIdValid = ensure(NetIdPlus->GetBaseNetId().IsValid());
		const bool bIsBaseStoreInterfaceValid = BaseStoreInterface.IsValid();
		if (bIsBaseNetIdValid && bIsBaseStoreInterfaceValid)
		{
			BaseStoreInterface->QueryOffersByFilter(*NetIdPlus->GetBaseNetId(), Filter, Delegate);
			return;
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::QueryOffersByFilter] Unable to call method in base interface. IsBaseNetIdValid=%s IsBaseStoreInterfaceValid=%s."), *LexToString(bIsBaseNetIdValid), *LexToString(bIsBaseStoreInterfaceValid));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::QueryOffersByFilter] Unable to call method in base interface. Unknown user (%s)"), *UserId.ToString());
	}

	EOSPlus->ExecuteNextTick([this, Delegate]() {
		TArray<FUniqueOfferId> OfferIds;
		Delegate.ExecuteIfBound(false, OfferIds, TEXT("Unable to call method in base interface."));
	});
}

void FOnlineStoreEOSPlus::QueryOffersById(const FUniqueNetId& UserId, const TArray<FUniqueOfferId>& OfferIds, const FOnQueryOnlineStoreOffersComplete& Delegate)
{
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(UserId.ToString());
	if (NetIdPlus.IsValid())
	{
		const bool bIsBaseNetIdValid = ensure(NetIdPlus->GetBaseNetId().IsValid());
		const bool bIsBaseStoreInterfaceValid = BaseStoreInterface.IsValid();
		if (bIsBaseNetIdValid && bIsBaseStoreInterfaceValid)
		{
			BaseStoreInterface->QueryOffersById(*NetIdPlus->GetBaseNetId(), OfferIds, Delegate);
			return;
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::QueryOffersById] Unable to call method in base interface. IsBaseNetIdValid=%s IsBaseStoreInterfaceValid=%s."), *LexToString(bIsBaseNetIdValid), *LexToString(bIsBaseStoreInterfaceValid));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::QueryOffersById] Unable to call method in base interface. Unknown user (%s)"), *UserId.ToString());
	}

	EOSPlus->ExecuteNextTick([this, Delegate]() {
		TArray<FUniqueOfferId> OfferIds;
		Delegate.ExecuteIfBound(false, OfferIds, TEXT("Unable to call method in base interface."));
	});
}

void FOnlineStoreEOSPlus::GetOffers(TArray<FOnlineStoreOfferRef>& OutOffers) const
{
	if (BaseStoreInterface.IsValid())
	{
		BaseStoreInterface->GetOffers(OutOffers);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineStoreEOSPlus::GetOffers] Unable to call method in base interface. Base interface not valid."));
	}
}

TSharedPtr<FOnlineStoreOffer> FOnlineStoreEOSPlus::GetOffer(const FUniqueOfferId& OfferId) const
{
	TSharedPtr<FOnlineStoreOffer> Result;
	
	if(BaseStoreInterface.IsValid())
	{
		Result = BaseStoreInterface->GetOffer(OfferId);
	}
	else
	{
		UE_LOG_ONLINE_ONCE(Warning, TEXT("[FOnlineStoreEOSPlus::GetOffer] Unable to call method in base interface. Base interface not valid."));
	}
	
	return Result;
}

void FOnlineStoreEOSPlus::OnQueryForAvailablePurchasesCompleteBase(bool bWasSuccessful)
{
	TriggerOnQueryForAvailablePurchasesCompleteDelegates(bWasSuccessful);
}

//~ End IOnlineStoreV2 Interface

//~ Begin IOnlinePurchase Interface

bool FOnlineStoreEOSPlus::IsAllowedToPurchase(const FUniqueNetId& UserId)
{
	bool bResult = false;

	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(UserId.ToString());
	if (NetIdPlus.IsValid())
	{
		const bool bIsBaseNetIdValid = ensure(NetIdPlus->GetBaseNetId().IsValid());
		const bool bIsBasePurchaseInterfaceValid = BasePurchaseInterface.IsValid();
		if (bIsBaseNetIdValid && bIsBasePurchaseInterfaceValid)
		{
			bResult = BasePurchaseInterface->IsAllowedToPurchase(*NetIdPlus->GetBaseNetId());
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::IsAllowedToPurchase] Unable to call method in base interface. IsBaseNetIdValid=%s IsBasePurchaseInterfaceValid=%s."), *LexToString(bIsBaseNetIdValid), *LexToString(bIsBasePurchaseInterfaceValid));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::IsAllowedToPurchase] Unable to call method in base interface. Unknown user (%s)"), *UserId.ToString());
	}

	return bResult;
}

void FOnlineStoreEOSPlus::Checkout(const FUniqueNetId& UserId, const FPurchaseCheckoutRequest& CheckoutRequest, const FOnPurchaseCheckoutComplete& Delegate)
{
	EOnlineErrorResult ErrorResult = EOnlineErrorResult::Unknown;
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(UserId.ToString());

	if (NetIdPlus.IsValid())
	{
		const bool bIsBaseNetIdValid = ensure(NetIdPlus->GetBaseNetId().IsValid());
		const bool bIsBasePurchaseInterfaceValid = BasePurchaseInterface.IsValid();
		if (bIsBaseNetIdValid && bIsBasePurchaseInterfaceValid)
		{
			BasePurchaseInterface->Checkout(*NetIdPlus->GetBaseNetId(), CheckoutRequest, Delegate);
			return;
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::Checkout] Unable to call method in base interface. IsBaseNetIdValid=%s IsBasePurchaseInterfaceValid=%s."), *LexToString(bIsBaseNetIdValid), *LexToString(bIsBasePurchaseInterfaceValid));
			ErrorResult = bIsBasePurchaseInterfaceValid ? EOnlineErrorResult::MissingInterface : EOnlineErrorResult::InvalidUser;
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::Checkout] Unable to call method in base interface. Unknown user (%s)"), *UserId.ToString());
		ErrorResult = EOnlineErrorResult::InvalidUser;
	}

	EOSPlus->ExecuteNextTick([this, ErrorResult, Delegate]() {
		TSharedRef<FPurchaseReceipt> Receipt;
		Delegate.ExecuteIfBound(FOnlineError(ErrorResult), Receipt);
	});
}

void FOnlineStoreEOSPlus::Checkout(const FUniqueNetId& UserId, const FPurchaseCheckoutRequest& CheckoutRequest, const FOnPurchaseReceiptlessCheckoutComplete& Delegate)
{
	EOnlineErrorResult ErrorResult = EOnlineErrorResult::Unknown;
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(UserId.ToString());
	if (NetIdPlus.IsValid())
	{
		const bool bIsBaseNetIdValid = ensure(NetIdPlus->GetBaseNetId().IsValid());
		const bool bIsBasePurchaseInterfaceValid = BasePurchaseInterface.IsValid();
		if (bIsBaseNetIdValid && bIsBasePurchaseInterfaceValid)
		{
			BasePurchaseInterface->Checkout(*NetIdPlus->GetBaseNetId(), CheckoutRequest, Delegate);
			return;
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::Checkout] Unable to call method in base interface. IsBaseNetIdValid=%s IsBasePurchaseInterfaceValid=%s."), *LexToString(bIsBaseNetIdValid), *LexToString(bIsBasePurchaseInterfaceValid));
			ErrorResult = bIsBasePurchaseInterfaceValid ? EOnlineErrorResult::MissingInterface : EOnlineErrorResult::InvalidUser;
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::Checkout] Unable to call method in base interface. Unknown user (%s)"), *UserId.ToString());
		ErrorResult = EOnlineErrorResult::InvalidUser;
	}

	EOSPlus->ExecuteNextTick([this, ErrorResult, Delegate]() {
		Delegate.ExecuteIfBound(FOnlineError(ErrorResult));
	});
}

void FOnlineStoreEOSPlus::FinalizePurchase(const FUniqueNetId& UserId, const FString& ReceiptId)
{
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(UserId.ToString());
	if (NetIdPlus.IsValid())
	{
		const bool bIsBaseNetIdValid = ensure(NetIdPlus->GetBaseNetId().IsValid());
		const bool bIsBasePurchaseInterfaceValid = BasePurchaseInterface.IsValid();
		if (bIsBaseNetIdValid && bIsBasePurchaseInterfaceValid)
		{
			BasePurchaseInterface->FinalizePurchase(*NetIdPlus->GetBaseNetId(), ReceiptId);
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::FinalizePurchase] Unable to call method in base interface. IsBaseNetIdValid=%s IsBasePurchaseInterfaceValid=%s."), *LexToString(bIsBaseNetIdValid), *LexToString(bIsBasePurchaseInterfaceValid));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::FinalizePurchase] Unable to call method in base interface. Unknown user (%s)"), *UserId.ToString());
	}
}

void FOnlineStoreEOSPlus::RedeemCode(const FUniqueNetId& UserId, const FRedeemCodeRequest& RedeemCodeRequest, const FOnPurchaseRedeemCodeComplete& Delegate)
{
	EOnlineErrorResult ErrorResult = EOnlineErrorResult::Unknown;
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(UserId.ToString());
	if (NetIdPlus.IsValid())
	{
		const bool bIsBaseNetIdValid = ensure(NetIdPlus->GetBaseNetId().IsValid());
		const bool bIsBasePurchaseInterfaceValid = BasePurchaseInterface.IsValid();
		if (bIsBaseNetIdValid && bIsBasePurchaseInterfaceValid)
		{
			BasePurchaseInterface->RedeemCode(*NetIdPlus->GetBaseNetId(), RedeemCodeRequest, Delegate);
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::RedeemCode] Unable to call method in base interface. IsBaseNetIdValid=%s IsBasePurchaseInterfaceValid=%s."), *LexToString(bIsBaseNetIdValid), *LexToString(bIsBasePurchaseInterfaceValid));
			ErrorResult = bIsBasePurchaseInterfaceValid ? EOnlineErrorResult::MissingInterface : EOnlineErrorResult::InvalidUser;
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::RedeemCode] Unable to call method in base interface. Unknown user (%s)"), *UserId.ToString());
		ErrorResult = EOnlineErrorResult::InvalidUser;
	}

	EOSPlus->ExecuteNextTick([this, ErrorResult, Delegate]() {
		TSharedRef<FPurchaseReceipt> Receipt;
		Delegate.ExecuteIfBound(FOnlineError(ErrorResult), Receipt);
	});
}

void FOnlineStoreEOSPlus::QueryReceipts(const FUniqueNetId& UserId, bool bRestoreReceipts, const FOnQueryReceiptsComplete& Delegate)
{
	EOnlineErrorResult ErrorResult = EOnlineErrorResult::Unknown;
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(UserId.ToString());
	if (NetIdPlus.IsValid())
	{
		const bool bIsBaseNetIdValid = ensure(NetIdPlus->GetBaseNetId().IsValid());
		const bool bIsBasePurchaseInterfaceValid = BasePurchaseInterface.IsValid();
		if (bIsBaseNetIdValid && bIsBasePurchaseInterfaceValid)
		{
			BasePurchaseInterface->QueryReceipts(*NetIdPlus->GetBaseNetId(), bRestoreReceipts, Delegate);
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::QueryReceipts] Unable to call method in base interface. IsBaseNetIdValid=%s IsBasePurchaseInterfaceValid=%s."), *LexToString(bIsBaseNetIdValid), *LexToString(bIsBasePurchaseInterfaceValid));
			ErrorResult = bIsBasePurchaseInterfaceValid ? EOnlineErrorResult::MissingInterface : EOnlineErrorResult::InvalidUser;
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::QueryReceipts] Unable to call method in base interface. Unknown user (%s)"), *UserId.ToString());
		ErrorResult = EOnlineErrorResult::InvalidUser;
	}

	EOSPlus->ExecuteNextTick([this, ErrorResult, Delegate]() {
		Delegate.ExecuteIfBound(FOnlineError(ErrorResult));
	});
}

void FOnlineStoreEOSPlus::GetReceipts(const FUniqueNetId& UserId, TArray<FPurchaseReceipt>& OutReceipts) const
{
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(UserId.ToString());
	if (NetIdPlus.IsValid())
	{
		const bool bIsBaseNetIdValid = ensure(NetIdPlus->GetBaseNetId().IsValid());
		const bool bIsBasePurchaseInterfaceValid = BasePurchaseInterface.IsValid();
		if (bIsBaseNetIdValid && bIsBasePurchaseInterfaceValid)
		{
			BasePurchaseInterface->GetReceipts(*NetIdPlus->GetBaseNetId(), OutReceipts);
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::GetReceipts] Unable to call method in base interface. IsBaseNetIdValid=%s IsBasePurchaseInterfaceValid=%s."), *LexToString(bIsBaseNetIdValid), *LexToString(bIsBasePurchaseInterfaceValid));
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::GetReceipts] Unable to call method in base interface. Unknown user (%s)"), *UserId.ToString());
	}
}

void FOnlineStoreEOSPlus::FinalizeReceiptValidationInfo(const FUniqueNetId& UserId, FString& InReceiptValidationInfo, const FOnFinalizeReceiptValidationInfoComplete& Delegate)
{
	EOnlineErrorResult ErrorResult = EOnlineErrorResult::Unknown;
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(UserId.ToString());
	if (NetIdPlus.IsValid())
	{
		const bool bIsBaseNetIdValid = ensure(NetIdPlus->GetBaseNetId().IsValid());
		const bool bIsBasePurchaseInterfaceValid = BasePurchaseInterface.IsValid();
		if (bIsBaseNetIdValid && bIsBasePurchaseInterfaceValid)
		{
			BasePurchaseInterface->FinalizeReceiptValidationInfo(*NetIdPlus->GetBaseNetId(), InReceiptValidationInfo, Delegate);
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::FinalizeReceiptValidationInfo] Unable to call method in base interface. IsBaseNetIdValid=%s IsBasePurchaseInterfaceValid=%s."), *LexToString(bIsBaseNetIdValid), *LexToString(bIsBasePurchaseInterfaceValid));
			ErrorResult = bIsBasePurchaseInterfaceValid ? EOnlineErrorResult::MissingInterface : EOnlineErrorResult::InvalidUser;
		}
	}
	else
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::FinalizeReceiptValidationInfo] Unable to call method in base interface. Unknown user (%s)"), *UserId.ToString());
		ErrorResult = EOnlineErrorResult::InvalidUser;
	}

	EOSPlus->ExecuteNextTick([this, ErrorResult, Delegate]() {
		Delegate.ExecuteIfBound(FOnlineError(ErrorResult), TEXT(""));
	});
}

void FOnlineStoreEOSPlus::OnUnexpectedPurchaseReceiptBase(const FUniqueNetId& UserId)
{
	FUniqueNetIdEOSPlusPtr NetIdPlus = GetNetIdPlus(UserId.ToString());
	if (!NetIdPlus.IsValid())
	{
		UE_LOG_ONLINE(Warning, TEXT("[FOnlineStoreEOSPlus::OnUnexpectedPurchaseReceiptBase] Unknown user (%s)"), *UserId.ToString());
	}

	TriggerOnUnexpectedPurchaseReceiptDelegates(*NetIdPlus);
}

//~ End IOnlinePurchase Interface