#pragma once

#include <ESPBase.h>

#include "knx_touch_config.h"

class KnxTouchWeb {
 public:
  using ChangeHandler = void (*)(void* context);
  using ModeHandler = bool (*)(bool active, void* context);
  using ModeReader = bool (*)(void* context);

  bool registerRoutes(ESPBase& espBase, KnxTouchConfigStore& store,
                      ChangeHandler handler, void* context = nullptr,
                      ModeHandler modeHandler = nullptr,
                      ModeReader modeReader = nullptr);

 private:
  static void handlePage(WebRequest& request, WebResponse& response, void* context);
  static void handleSave(WebRequest& request, WebResponse& response, void* context);
  static void handleActive(WebRequest& request, WebResponse& response, void* context);
  static void handlePassive(WebRequest& request, WebResponse& response, void* context);
  void render(WebResponse& response, const char* message, bool error,
              WebRequest* submitted = nullptr, const char* errorField = nullptr);

  KnxTouchConfigStore* store_ = nullptr;
  ChangeHandler changeHandler_ = nullptr;
  void* changeContext_ = nullptr;
  ModeHandler modeHandler_ = nullptr;
  ModeReader modeReader_ = nullptr;
};
