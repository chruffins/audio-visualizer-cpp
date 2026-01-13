#include "core/discord_integration.hpp"
#include "music/library_views.hpp"
#include <fstream>
#include <filesystem>

const std::string token_file = "discord_tokens.txt";

namespace core {

static bool save_tokens(const std::string& access, const std::string& refresh, int32_t expires_at) {
    try {
        std::ofstream ofs(token_file, std::ios::trunc);
        if (!ofs) return false;
        ofs << "access_token=" << access << "\n";
        ofs << "refresh_token=" << refresh << "\n";
        ofs << "expires_at=" << expires_at << "\n";
        return true;
    } catch (...) {
        return false;
    }
}

static bool load_tokens(std::string& access, std::string& refresh, int32_t& expires_at) {
    if (!std::filesystem::exists(token_file)) return false;
    std::ifstream ifs(token_file);
    if (!ifs) return false;
    std::string line;
    while (std::getline(ifs, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        auto key = line.substr(0,pos);
        auto val = line.substr(pos+1);
        if (key == "access_token") access = val;
        else if (key == "refresh_token") refresh = val;
        else if (key == "expires_at") {
            try { expires_at = std::stoll(val); } catch(...) { expires_at = 0; }
        }
    }
    return !access.empty() || !refresh.empty();
}

bool DiscordIntegration::init() {
    static bool initialized = false;
    if (initialized) return true; // Already initialized

    this->client = std::make_shared<discordpp::Client>();

    /*
    client->AddLogCallback([](auto message, auto severity) {
      std::cout << "[" << EnumToString(severity) << "] " << message << std::endl;
    }, discordpp::LoggingSeverity::Info);
    */

    // Set up status callback to monitor client connection
    this->client->SetStatusChangedCallback([this](discordpp::Client::Status status, discordpp::Client::Error error, int32_t errorDetail) {
      std::cout << "🔄 Status changed: " << discordpp::Client::StatusToString(status) << std::endl;

      if (status == discordpp::Client::Status::Ready) {
        std::cout << "✅ Client is ready! You can now call SDK functions.\n";
      } else if (error != discordpp::Client::Error::None) {
        std::cerr << "❌ Connection Error: " << discordpp::Client::ErrorToString(error) << " - Details: " << errorDetail << std::endl;
      }
    });

    initialized = true;
    return true;
}

void DiscordIntegration::shutdown() {
    // Render your application state here
}

void DiscordIntegration::set_new_song(std::string& title, std::string& artist) {
    discordpp::Activity activity;
    discordpp::ActivityAssets assets;
    discordpp::ActivityTimestamps timestamps;
    assets.SetSmallImage("logo");
    assets.SetSmallText("Audio Visualizer C++");
    assets.SetLargeImage("https://coverartarchive.org/release-group/3c7430a6-798f-3060-8539-4d22a92aaffe/front");
    // assets.SetLargeText("Abandoned Pools - Humanistic");

    // activity.SetName("Audio Visualizer C++"); this does nothing
    activity.SetType(discordpp::ActivityTypes::Listening);
    activity.SetDetails(title);
    activity.SetState(artist);
    activity.SetApplicationId(APPLICATION_ID);
    activity.SetStatusDisplayType(discordpp::StatusDisplayTypes::State);
    activity.SetAssets(assets);
    timestamps.SetStart(std::time(nullptr));
    timestamps.SetEnd(std::time(nullptr) + 200); // Example: song ends in 200 seconds
    activity.SetTimestamps(timestamps);

    client->UpdateRichPresence(activity, [](auto result) {
        if (result.Successful()) {
            std::cout << "✅ Rich presence updated successfully.\n";
        } else {
            std::cerr << "❌ Failed to update rich presence: " << result.Error() << std::endl;
        }
        });
}

void DiscordIntegration::setSongPresence(const music::SongView &song) {
    if (client->GetStatus() != discordpp::Client::Status::Ready) {
        std::cerr << "❌ Client not ready.\n";
        return;
    }

    discordpp::Activity activity;
    discordpp::ActivityAssets assets;
    discordpp::ActivityTimestamps timestamps;
    assets.SetSmallImage("logo");
    assets.SetSmallText("Audio Visualizer C++");
    assets.SetLargeImage("https://coverartarchive.org/release-group/3c7430a6-798f-3060-8539-4d22a92aaffe/front");
    // assets.SetLargeText("Abandoned Pools - Humanistic");

    // activity.SetName("Audio Visualizer C++"); this does nothing
    activity.SetType(discordpp::ActivityTypes::Listening);
    activity.SetDetails(song.title);
    activity.SetState(song.artist);
    activity.SetApplicationId(APPLICATION_ID);
    activity.SetStatusDisplayType(discordpp::StatusDisplayTypes::State);
    activity.SetAssets(assets);
    timestamps.SetStart(std::time(nullptr));
    timestamps.SetEnd(std::time(nullptr) + song.duration); // Example: song ends in song.duration seconds
    activity.SetTimestamps(timestamps);

    client->UpdateRichPresence(activity, [](auto result) {
        if (result.Successful()) {
            std::cout << "✅ Rich presence updated successfully.\n";
        } else {
            std::cerr << "❌ Failed to update rich presence: " << result.Error() << std::endl;
        }
        });
}

void DiscordIntegration::update_presence(std::string state) {
    discordpp::Activity activity;
    activity.SetName("Audio Visualizer C++");
    activity.SetType(discordpp::ActivityTypes::Listening);
    activity.SetState(state);
    activity.SetDetails("The Remedy\nAbandoned Pools");
    activity.SetApplicationId(APPLICATION_ID);
    
    client->UpdateRichPresence(activity, [](auto result) {
      if (result.Successful()) {
        std::cout << "✅ Rich presence updated successfully.\n";
      } else {
        std::cerr << "❌ Failed to update rich presence: " << result.Error() << std::endl;
      }
    });
}

bool DiscordIntegration::authorize()
{
    if (!this->client) {
      std::cerr << "❌ Client not initialized. Call init() first.\n";
      return false;
    }

    auto codeVerifier = client->CreateAuthorizationCodeVerifier();
    discordpp::AuthorizationArgs args{};
    args.SetClientId(APPLICATION_ID);
    args.SetScopes(discordpp::Client::GetDefaultPresenceScopes());
    args.SetCodeChallenge(codeVerifier.Challenge());
    // args.SetIntegrationType(discordpp::IntegrationType::UserInstall);

    // Try to load existing tokens
    std::string accessToken, refreshToken;
    int32_t expiresAt = 0;

    bool tokens_loaded = load_tokens(accessToken, refreshToken, expiresAt);

    if (tokens_loaded) {
        std:: cout << "Loaded tokens from file. Access token expires at " << expiresAt << " (current time: " << std::time(nullptr) << ")\n";
        if (expiresAt > static_cast<int32_t>(std::time(nullptr))) {
            std::cout << "🔑 Access token is still valid. Updating token and connecting...\n";
            client->UpdateToken(discordpp::AuthorizationTokenType::Bearer, accessToken, [this](discordpp::ClientResult result) {
              if(result.Successful()) {
                std::cout << "🔑 Token updated, connecting to Discord...\n";
                this->client->Connect();
              }
            });
            return true;
        } else {
            std::cout << "Access token expired. Refreshing...\n";
            client->RefreshToken(APPLICATION_ID, refreshToken, [this](auto result, std::string accessToken,
            std::string refreshToken, auto tokenType, int32_t newExpiresIn, std::string scope) {
                if (result.Successful()) {
                    std::cout << "🔄 Token refreshed successfully! New access token expires in " << newExpiresIn << "\n";
                    client->UpdateToken(discordpp::AuthorizationTokenType::Bearer, accessToken, [this](discordpp::ClientResult result) {
                        if (result.Successful()) {
                            std::cout << "🔑 Token updated, connecting to Discord...\n";
                            this->client->Connect();
                        }
                    });

                    // Save new tokens to file
                    save_tokens(accessToken, refreshToken, newExpiresIn + static_cast<int32_t>(std::time(nullptr)));
                } else {
                    std::cerr << "❌ Failed to refresh token: " << result.Error() << std::endl;
                }
            });
        }
    }

    // Begin authentication process
    client->Authorize(args, [this, codeVerifier](auto result, auto code, auto redirectUri) {
    if (!result.Successful()) {
        std::cerr << "❌ Authentication Error: " << result.Error() << std::endl;
        return false;
    } else {
        std::cout << "✅ Authorization successful! Getting access token...\n";

        // Exchange auth code for access token
        client->GetToken(APPLICATION_ID, code, codeVerifier.Verifier(), redirectUri,
        [this](discordpp::ClientResult result,
        std::string accessToken,
        std::string refreshToken,
        discordpp::AuthorizationTokenType tokenType,
        int32_t expiresIn,
        std::string scope) {
            std::cout << "🔓 Access token received! Establishing connection...\n";
            // Next Step: Update the token and connect
            client->UpdateToken(discordpp::AuthorizationTokenType::Bearer,  accessToken, [this](discordpp::ClientResult result) {
              if(result.Successful()) {
                std::cout << "🔑 Token updated, connecting to Discord...\n";
                this->client->Connect();
              }
            });

            // Save tokens to file
            save_tokens(accessToken, refreshToken, expiresIn + static_cast<int32_t>(std::time(nullptr)));
        });
        return true;
    }
    });
    return true;
}
};