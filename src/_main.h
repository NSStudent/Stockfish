#ifndef STOCKFISH_EMBEDDED_MAIN_H
#define STOCKFISH_EMBEDDED_MAIN_H

#include <functional>
#include <string_view>

int _main(int argc, char* argv[], std::function<void(std::string_view)> onFailure = {});

#endif
