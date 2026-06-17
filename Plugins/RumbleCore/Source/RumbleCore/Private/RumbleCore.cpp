// Copyright (c) 2026 [Menars]. All Rights Reserved.

#include "RumbleCore.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

DEFINE_LOG_CATEGORY(LogRumbleCore);

#define LOCTEXT_NAMESPACE "FRumbleCoreModule"

void FRumbleCoreModule::StartupModule()
{
}

void FRumbleCoreModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	UE_LOG(LogRumbleCore, Log, TEXT("RumbleCore module unloaded."));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRumbleCoreModule, RumbleCore)