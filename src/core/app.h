#ifndef APP_H
#define APP_H

#include <future>
#include <memory>
#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "core/app_state.h"
#include "ui/main_view.h"

namespace gradient_editor {

class App {
public:
    App();
    ~App();

    void run(std::promise<HWND>&& hwnd_promise);
    void renderFrame();

private:
    void cleanup();

    void readSettings();
    void writeSettings();

    std::unique_ptr<MainView> m_main_view;
};

}  // namespace gradient_editor

#endif  // APP_H
