#include "core/discord_integration.hpp"

namespace core {
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


        this->update_presence("Testing");
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

void DiscordIntegration::update_presence(std::string state) {
    discordpp::Activity activity;
    activity.SetName("Audio Visualizer C++");
    activity.SetType(discordpp::ActivityTypes::Playing);
    activity.SetState(state);
    activity.SetDetails("Using Discord SDK with C++17");
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