#include "laya/ui/app.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <audio_file> [lyrics.lrc]\n";
        return 1;
    }

    std::filesystem::path audio_path = argv[1];
    std::filesystem::path lrc_path = (argc >= 3) ? argv[2] : audio_path.replace_extension(".lrc");

    laya::ui::App app(audio_path, lrc_path);
    app.run();

    return 0;
}