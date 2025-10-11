#include "core/discord_integration.hpp"
#include <fstream>
#include <filesystem>

const std::string token_file = "discord_tokens.txt";

namespace core {

static bool save_tokens(const std::string& access, const std::string& refresh, int64_t expires_at) {
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

static bool load_tokens(std::string& access, std::string& refresh, int64_t& expires_at) {
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

        // Example: Set initial rich presence
        auto title = std::string("The Remedy");
        auto artist = std::string("Abandoned Pools");

        this->set_new_song(title, artist);
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
    // activity.SetName("Audio Visualizer C++"); this does nothing
    activity.SetType(discordpp::ActivityTypes::Listening);
    activity.SetDetails(title);
    activity.SetState(artist);;
    activity.SetApplicationId(APPLICATION_ID);
    activity.SetStatusDisplayType(discordpp::StatusDisplayTypes::State);

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
        });
        return true;
    }
    });
    return true;
}
};