#include "SteamAPI.h"

#include <fstream>
#include <libloader.hpp>
#include <config/ConEntry.h>
#include <core/Logger.h>
#include <resource/provider/FilesystemResourceProvider.h>
#include <utility/String.h>
#include "Plugin.h"

using namespace chira;
using namespace steam;
using namespace libloader;

CHIRA_CREATE_LOG(STEAM);

ConVar steam_enable{"steam_enable", true, "Initialize Steam API functions.", CON_FLAG_CACHE};

CHIRA_CREATE_PLUGIN(Steam) {
    static inline const std::vector<std::string_view> DEPS;

    void init() override {
        if (steam_enable.getValue<bool>() && (!SteamAPI::Client::initialized() && !SteamAPI::Client::initSteam())) {
            LOG_STEAM.warning("Steam failed to initialize");
        }
    }

    void update() override {
        if (SteamAPI::Client::initialized()) {
            SteamAPI::Client::runCallbacks();
        }
    }

    void deinit() override {
        if (SteamAPI::Client::initialized()) {
            SteamAPI::Client::shutdown();
        }
    }
};
CHIRA_REGISTER_PLUGIN(Steam);

/// Helper function to stop repeating stuff
template<typename T, typename U, typename... Params>
inline T steamFunctionWrapper(const std::string& function, T defaultValue, U steamSingleton, Params... params) {
    if (steamSingleton) {
        auto out = SteamAPI::get().call<T>(function, steamSingleton, std::forward<Params>(params)...);
        if (!out) {
            return defaultValue;
        }
        return *out;
    }
    return defaultValue;
}

/// Helper function to stop repeating stuff
template<typename T, typename... Params>
inline std::string steamFunctionStringWrapper(const std::string& function, const std::string& defaultValue, T steamSingleton, Params... params) {
    if (steamSingleton) {
        auto out = SteamAPI::get().call<const char*>(function, steamSingleton, std::forward<Params>(params)...);
        return out ? std::string{*out} : defaultValue;
    }
    return defaultValue;
}

const library& SteamAPI::get() {
    static library steamBinary{FILESYSTEM_ROOT_FOLDER + "/engine/bin/steam_api" + std::to_string(ENVIRONMENT_TYPE)};
    return steamBinary;
}

void SteamAPI::generateAppIDFile(unsigned int appID) {
    std::ofstream file{"steam_appid.txt"};
    file << std::to_string(appID) << std::endl;
    file.close();
}

void* SteamAPI::Client::get() {
    if (auto client = SteamAPI::get().call<void*>("SteamClient"))
        return *client;
    return nullptr;
}

bool SteamAPI::Client::initSteam() {
    if (SteamAPI::Client::isInitialized)
        return true;
    if (auto out = SteamAPI::get().call<bool>("SteamAPI_Init"); out && *out) {
        SteamAPI::Client::isInitialized = true;
        // Handle callbacks manually
        SteamAPI::get().call<void>("SteamAPI_ManualDispatch_Init");
        return true;
    }
    return false;
}

bool SteamAPI::Client::initialized() {
    return SteamAPI::Client::isInitialized;
}

void SteamAPI::Client::runCallbacks() {
    std::int32_t steamPipe;
    if (auto client = SteamAPI::Client::get())
        steamPipe = steamFunctionWrapper<std::int32_t>("SteamAPI_GetHSteamPipe", 0, client);
    else
        return;
    SteamAPI::get().call<void>("SteamAPI_ManualDispatch_RunFrame", steamPipe);
    CallbackMessage callback{};
    while (steamFunctionWrapper<bool>("SteamAPI_ManualDispatch_GetNextCallback", false, steamPipe, &callback)) {
        switch (static_cast<CallbackMessageType>(callback.callbackType)) {
            using enum CallbackMessageType;
            case GAME_OVERLAY_ACTIVATED:
                // todo(events): add game overlay enable/disable event
                //Events::createEvent("chira::steam::game_overlay_activated", static_cast<bool>(reinterpret_cast<Callbacks::GameOverlayActivated*>(callback.callback)->active));
                break;
            case COMPLETED: {
                void* callbackResult = std::malloc(callback.callbackSize);
                if (bool failed = false; steamFunctionWrapper<bool>(
                        "SteamAPI_ManualDispatch_GetAPICallResult",
                        false,
                        steamPipe,
                        reinterpret_cast<Callbacks::Completed*>(callback.callback)->asyncCallbackId,
                        callbackResult,
                        callback.callback,
                        callback.callbackType,
                        &failed
                    )) {
                    // todo: dispatch callbacks that go through this thing
                    // Dispatch the call result to the registered handler(s) for the
                    // call identified by pCallCompleted->m_hAsyncCall
                }
                std::free(callbackResult);
                break;
            }
            case DLC_INSTALLED:
                // todo(events): add dlc installed event
                //Events::createEvent("chira::steam::dlc_installed", static_cast<std::uint32_t>(reinterpret_cast<Callbacks::DLCInstalled*>(callback.callback)->appID));
                break;
            case FILE_DETAILS_RESULT:
                // todo(events): add file details callback event
                //Events::createEvent("chira::steam::file_details_result", *reinterpret_cast<Callbacks::FileDetailsResult*>(callback.callback));
                break;
        }
        SteamAPI::get().call<void>("SteamAPI_ManualDispatch_FreeLastCallback", steamPipe);
    }
    // Not called automatically anymore!
    SteamAPI::get().call<void>("SteamAPI_ReleaseCurrentThreadMemory");
}

void SteamAPI::Client::shutdown() {
    SteamAPI::get().call<void>("SteamAPI_Shutdown");
}

// -------------------------------- USER -------------------------------- //

void* SteamAPI::User::get() {
    if (auto user = SteamAPI::get().call<void*>("SteamAPI_SteamUser_v021"))
        return *user;
    return nullptr;
}

bool SteamAPI::User::isLoggedOn() {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamUser_BLoggedOn", false, SteamAPI::User::get());
}

std::uint64_t SteamAPI::User::getSteamID() {
    return steamFunctionWrapper<std::uint64_t>("SteamAPI_ISteamUser_GetSteamID", 0, SteamAPI::User::get());
}

bool SteamAPI::User::isBehindNAT() {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamUser_BIsBehindNAT", false, SteamAPI::User::get());
}

int SteamAPI::User::getGameBadgeLevel(bool foil, int series) {
    return steamFunctionWrapper<int>("SteamAPI_ISteamUser_GetGameBadgeLevel", 0, SteamAPI::User::get(), series, foil);
}

int SteamAPI::User::getPlayerSteamLevel() {
    return steamFunctionWrapper<int>("SteamAPI_ISteamUser_GetPlayerSteamLevel", -1, SteamAPI::User::get());
}

bool SteamAPI::User::isPhoneVerified() {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamUser_BIsPhoneVerified", false, SteamAPI::User::get());
}

bool SteamAPI::User::isTwoFactorAuthenticationEnabled() {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamUser_BIsTwoFactorEnabled", false, SteamAPI::User::get());
}

bool SteamAPI::User::isPhoneIdentifying() {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamUser_BIsPhoneIdentifying", false, SteamAPI::User::get());
}

bool SteamAPI::User::isPhoneRequiringVerification() {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamUser_BIsPhoneRequiringVerification", false, SteamAPI::User::get());
}

std::uint64_t SteamAPI::User::getMarketEligibility() {
    return steamFunctionWrapper<std::uint64_t>("SteamAPI_ISteamUser_GetMarketEligibility", 0, SteamAPI::User::get());
}

// -------------------------------- FRIENDS -------------------------------- //

void* SteamAPI::Friends::get() {
    if (auto friends = SteamAPI::get().call<void*>("SteamAPI_SteamFriends_v017"))
        return *friends;
    return nullptr;
}

std::string SteamAPI::Friends::getPersonaName() {
    return steamFunctionStringWrapper("SteamAPI_ISteamFriends_GetPersonaName", "", SteamAPI::Friends::get());
}

std::uint64_t SteamAPI::Friends::setPersonaName(std::string_view name) {
    return steamFunctionWrapper<std::uint64_t>("SteamAPI_ISteamFriends_SetPersonaName", 0, SteamAPI::Friends::get(), name.data());
}

// -------------------------------- UTILS -------------------------------- //

void* SteamAPI::Utils::get() {
    if (auto utils = SteamAPI::get().call<void*>("SteamAPI_SteamUtils_v010"))
        return *utils;
    return nullptr;
}

std::uint32_t SteamAPI::Utils::getSecondsSinceAppActive() {
    return steamFunctionWrapper<std::uint32_t>("SteamAPI_ISteamUtils_GetSecondsSinceAppActive", 0, SteamAPI::Utils::get());
}

std::uint32_t SteamAPI::Utils::getSecondsSinceComputerActive() {
    return steamFunctionWrapper<std::uint32_t>("SteamAPI_ISteamUtils_GetSecondsSinceComputerActive", 0, SteamAPI::Utils::get());
}

std::uint32_t SteamAPI::Utils::getServerRealTime() {
    return steamFunctionWrapper<std::uint32_t>("SteamAPI_ISteamUtils_GetServerRealTime", 0, SteamAPI::Utils::get());
}

std::string SteamAPI::Utils::getIPCountry() {
    return steamFunctionStringWrapper<bool>("SteamAPI_ISteamUtils_GetIPCountry", "", SteamAPI::Utils::get());
}

bool SteamAPI::Utils::getImageSize(int imageID, std::uint32_t* width, std::uint32_t* height) {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamUtils_GetImageSize", false, SteamAPI::Utils::get(), imageID, width, height);
}

bool SteamAPI::Utils::getImageRGBA(int imageID, std::uint8_t* imageBuffer, int imageBufferSize) {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamUtils_GetImageRGBA", false, SteamAPI::Utils::get(), imageID, imageBuffer, imageBufferSize);
}

std::uint8_t SteamAPI::Utils::getCurrentBatteryPower() {
    return steamFunctionWrapper<std::uint8_t>("SteamAPI_ISteamUtils_GetCurrentBatteryPower", 0, SteamAPI::Utils::get());
}

std::uint32_t SteamAPI::Utils::getAppID() {
    return steamFunctionWrapper<std::uint32_t>("SteamAPI_ISteamUtils_GetAppID", 0, SteamAPI::Utils::get());
}

void SteamAPI::Utils::setOverlayNotificationPosition(NotificationPosition position) {
    if (auto utils = SteamAPI::Utils::get())
        SteamAPI::get().call<void>("SteamAPI_ISteamUtils_SetOverlayNotificationPosition", utils, position);
}

std::uint32_t SteamAPI::Utils::getIPCCallCount() {
    return steamFunctionWrapper<std::uint32_t>("SteamAPI_ISteamUtils_GetIPCCallCount", 0, SteamAPI::Utils::get());
}

bool SteamAPI::Utils::isOverlayEnabled() {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamUtils_IsOverlayEnabled", false, SteamAPI::Utils::get());
}

bool SteamAPI::Utils::isRunningInVR() {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamUtils_IsSteamRunningInVR", false, SteamAPI::Utils::get());
}

void SteamAPI::Utils::setOverlayNotificationInset(int horizontalInset, int verticalInset) {
    if (auto utils = SteamAPI::Utils::get())
        SteamAPI::get().call<void>("SteamAPI_ISteamUtils_SetOverlayNotificationInset", utils, horizontalInset, verticalInset);
}

bool SteamAPI::Utils::isBigPictureModeOn() {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamUtils_IsSteamInBigPictureMode", false, SteamAPI::Utils::get());
}

void SteamAPI::Utils::startVRDashboard() {
    if (auto utils = SteamAPI::Utils::get())
        SteamAPI::get().call<void>("SteamAPI_ISteamUtils_StartVRDashboard", utils);
}

bool SteamAPI::Utils::isVRHeadsetStreamingEnabled() {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamUtils_IsVRHeadsetStreamingEnabled", false, SteamAPI::Utils::get());
}

void SteamAPI::Utils::setVRHeadsetStreamingEnabled(bool enabled) {
    if (auto utils = SteamAPI::Utils::get())
        SteamAPI::get().call<void>("SteamAPI_ISteamUtils_SetVRHeadsetStreamingEnabled", utils, enabled);
}

bool SteamAPI::Utils::isRunningOnSteamDeck() {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamUtils_IsSteamRunningOnSteamDeck", false, SteamAPI::Utils::get());
}

void SteamAPI::Utils::setGameLauncherMode(bool launcherMode) {
    if (auto utils = SteamAPI::Utils::get())
        SteamAPI::get().call<void>("SteamAPI_ISteamUtils_SetGameLauncherMode", utils, launcherMode);
}

// -------------------------------- USER STATS -------------------------------- //

void* SteamAPI::UserStats::get() {
    if (auto userStats = SteamAPI::get().call<void*>("SteamAPI_SteamUserStats_v012"))
        return *userStats;
    return nullptr;
}

// -------------------------------- APPS -------------------------------- //

void* SteamAPI::Apps::get() {
    if (auto apps = SteamAPI::get().call<void*>("SteamAPI_SteamApps_v008"))
        return *apps;
    return nullptr;
}

bool SteamAPI::Apps::userOwnsThisAppID() {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamApps_BIsSubscribed", false, SteamAPI::Apps::get());
}

bool SteamAPI::Apps::isLowViolence() {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamApps_BIsLowViolence", false, SteamAPI::Apps::get());
}

bool SteamAPI::Apps::isCybercafe() {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamApps_BIsCybercafe", false, SteamAPI::Apps::get());
}

bool SteamAPI::Apps::isVACBanned() {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamApps_BIsVACBanned", false, SteamAPI::Apps::get());
}

std::string SteamAPI::Apps::getCurrentGameLanguage() {
    return steamFunctionStringWrapper("SteamAPI_ISteamApps_GetCurrentGameLanguage", "", SteamAPI::Apps::get());
}

std::vector<std::string> SteamAPI::Apps::getAvailableGameLanguages() {
    return String::split(steamFunctionStringWrapper("SteamAPI_ISteamApps_GetAvailableGameLanguages", "", SteamAPI::Apps::get()), ',');
}

bool SteamAPI::Apps::isSubscribedApp(std::uint32_t appID) {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamApps_BIsSubscribedApp", false, SteamAPI::Apps::get(), appID);
}

bool SteamAPI::Apps::isDLCInstalled(std::uint32_t appID) {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamApps_BIsDlcInstalled", false, SteamAPI::Apps::get(), appID);
}

std::uint32_t SteamAPI::Apps::getEarliestPurchaseUnixTime(std::uint32_t appID) {
    return steamFunctionWrapper<std::uint32_t>("SteamAPI_ISteamApps_GetEarliestPurchaseUnixTime", 0, SteamAPI::Apps::get(), appID);
}

bool SteamAPI::Apps::isSubscribedFromFreeWeekend() {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamApps_BIsSubscribedFromFreeWeekend", false, SteamAPI::Apps::get());
}

int SteamAPI::Apps::getDLCCount() {
    return steamFunctionWrapper<int>("SteamAPI_ISteamApps_GetDLCCount", 0, SteamAPI::Apps::get());
}

bool SteamAPI::Apps::getDLCData(int dlc, std::uint32_t* appID, bool* available, std::string& name) {
    auto apps = SteamAPI::Apps::get();
    if (!apps) {
        return false;
    }
    char* cname = new char[FilesystemResourceProvider::FILEPATH_MAX_LENGTH];
    auto isValid = SteamAPI::get().call<bool>("SteamAPI_ISteamApps_BGetDLCDataByIndex", apps, dlc, appID, available, cname, static_cast<std::int32_t>(FilesystemResourceProvider::FILEPATH_MAX_LENGTH));
    if (!isValid || !(*isValid)) {
        delete[] cname;
        return false;
    }
    name = cname;
    delete[] cname;
    return true;
}

void SteamAPI::Apps::installDLC(std::uint32_t appID) {
    if (auto apps = SteamAPI::Apps::get())
        SteamAPI::get().call<void>("SteamAPI_ISteamApps_InstallDLC", apps, appID);
}

void SteamAPI::Apps::uninstallDLC(std::uint32_t appID) {
    if (auto apps = SteamAPI::Apps::get())
        SteamAPI::get().call<void>("SteamAPI_ISteamApps_UninstallDLC", apps, appID);
}

std::string SteamAPI::Apps::getCurrentBranch() {
    auto apps = SteamAPI::Apps::get();
    if (!apps) {
        return "";
    }
    char* out = new char[FilesystemResourceProvider::FILEPATH_MAX_LENGTH];
    auto isOnBeta = SteamAPI::get().call<bool>("SteamAPI_ISteamApps_GetCurrentBetaName", apps, out, static_cast<std::int32_t>(FilesystemResourceProvider::FILEPATH_MAX_LENGTH));
    if (!isOnBeta || !(*isOnBeta)) {
        delete[] out;
        return "";
    }
    std::string path{out};
    delete[] out;
    return path;
}

bool SteamAPI::Apps::markContentCorrupt(bool missingFilesOnly) {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamApps_MarkContentCorrupt", false, SteamAPI::Apps::get(), missingFilesOnly);
}

std::vector<std::uint32_t> SteamAPI::Apps::getInstalledDepots(std::uint32_t appID) {
    auto apps = SteamAPI::Apps::get();
    if (!apps) {
        return {};
    }
    auto* out = new std::uint32_t[32]; // GodotSteam uses 32 so we do too
    auto size = SteamAPI::get().call<std::uint32_t>("SteamAPI_ISteamApps_GetInstalledDepots", apps, appID, out, 32);
    if (!size) {
        return {};
    }
    std::vector<std::uint32_t> depots;
    for (unsigned int i = 0; i < size; i++) {
        depots.push_back(out[i]);
    }
    delete[] out;
    return depots;
}

std::string SteamAPI::Apps::getAppInstallPath(std::uint32_t appID) {
    auto apps = SteamAPI::Apps::get();
    if (!apps) {
        return "";
    }
    auto* out = new char[FilesystemResourceProvider::FILEPATH_MAX_LENGTH];
    auto size = SteamAPI::get().call<std::uint32_t>("SteamAPI_ISteamApps_GetAppInstallDir", apps, appID, out, static_cast<std::uint32_t>(FilesystemResourceProvider::FILEPATH_MAX_LENGTH));
    if (!size) {
        return "";
    }
    std::string path{out, *size - 1};
    delete[] out;
    return path;
}

bool SteamAPI::Apps::isAppInstalled(std::uint32_t appID) {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamApps_BIsAppInstalled", false, SteamAPI::Apps::get(), appID);
}

std::uint64_t SteamAPI::Apps::getAppOwner() {
    return steamFunctionWrapper<std::uint64_t>("SteamAPI_ISteamApps_GetAppOwner", 0, SteamAPI::Apps::get());
}

std::string SteamAPI::Apps::getLaunchParameter(std::string_view key) {
    return steamFunctionStringWrapper("SteamAPI_ISteamApps_GetLaunchQueryParam", "", SteamAPI::Apps::get(), key.data());
}

bool SteamAPI::Apps::getDLCDownloadProgress(std::uint32_t appID, std::uint64_t* bytesDownloaded, std::uint64_t* bytesTotal) {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamApps_GetDlcDownloadProgress", false, SteamAPI::Apps::get(), appID, bytesDownloaded, bytesTotal);
}

int SteamAPI::Apps::getAppBuildID() {
    return steamFunctionWrapper<int>("SteamAPI_ISteamApps_GetAppBuildId", 0, SteamAPI::Apps::get());
}

std::uint64_t SteamAPI::Apps::getFileDetails(std::string_view filename) {
    return steamFunctionWrapper<std::uint64_t>("SteamAPI_ISteamApps_GetFileDetails", 0, SteamAPI::Apps::get(), filename.data());
}

bool SteamAPI::Apps::isSubscribedFromFamilySharing() {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamApps_BIsSubscribedFromFamilySharing", false, SteamAPI::Apps::get());
}

bool SteamAPI::Apps::isTimedTrial(std::uint32_t* secondsAllowed, std::uint32_t* secondsPlayed) {
    return steamFunctionWrapper<bool>("SteamAPI_ISteamApps_BIsTimedTrial", false, SteamAPI::Apps::get(), secondsAllowed, secondsPlayed);
}

// -------------------------------- UGC -------------------------------- //

void* SteamAPI::UGC::get() {
    if (auto ugc = SteamAPI::get().call<void*>("SteamAPI_SteamUGC_v016"))
        return *ugc;
    return nullptr;
}
