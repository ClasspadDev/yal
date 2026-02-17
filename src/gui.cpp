#include "gui.hpp"

#include "gfx.h"
#include "loaders/interface.hpp"
#include <forward_list>
#include <map>
#include <memory>
#include <utility>

std::unique_ptr<Executable>
do_gui(std::forward_list<std::unique_ptr<Executable>> &executable_list) {
  gfxInit();
  const auto font = gdispOpenFont("*");
  gwinSetDefaultFont(font);
  gwinSetDefaultStyle(&WhiteWidgetStyle, gFalse);
  gdispClear(GFX_WHITE);

  std::map<int, std::reference_wrapper<std::unique_ptr<Executable>>>
      list_id_to_entry;

  GWidgetInit init;
  gwinWidgetClearInit(&init);

  const auto screen_width = gdispGetWidth();
  const auto screen_height = gdispGetHeight();

  constexpr gCoord border = 3;
  constexpr gCoord widget_height = 25;

  init.g.width = screen_width - border * 2;
  init.g.x = border;
  init.g.y = border;
  init.g.show = gTrue;
  init.g.height = widget_height;

  init.text = "Yet Another Launcher";
  init.g.width -= widget_height + border;
  gwinLabelCreate(nullptr, &init);
  init.g.x += init.g.width + border;
  init.g.width = init.g.height;
  init.text = "X";
  const auto button_exit = gwinButtonCreate(nullptr, &init);
  init.g.x = border;
  init.g.width = screen_width - border * 2;
  init.g.y += init.g.height + border;

  init.g.height = widget_height * 10;
  const auto list_names = gwinListCreate(nullptr, &init, gFalse);
  gwinListSetScroll(list_names, scrollAlways);
  init.g.y += init.g.height + border;
  init.g.height = widget_height;

  for (auto &exe : executable_list) {
    const auto id = gwinListAddItem(list_names, exe->getName().get(), gTrue);
    list_id_to_entry.emplace(id, exe); // only gets reordered on remove
  }

  init.text = "Run";
  const auto button_run = gwinButtonCreate(nullptr, &init);
  init.g.y += init.g.height + border;

  auto save_init = init;
  init.g.width = 75;
  save_init.g.x += init.g.width + border;
  save_init.g.width -= save_init.g.x - init.g.x;
  save_init.text = nullptr;
  init.text = "Path:";
  gwinLabelCreate(nullptr, &init);
  const auto label_path = gwinLabelCreate(nullptr, &save_init);
  save_init.g.y = init.g.y += init.g.height + border;
  init.text = "Version:";
  gwinLabelCreate(nullptr, &init);
  const auto label_version = gwinLabelCreate(nullptr, &save_init);
  save_init.g.y = init.g.y += init.g.height + border;
  init.text = "Author:";
  gwinLabelCreate(nullptr, &init);
  const auto label_author = gwinLabelCreate(nullptr, &save_init);
  init.g.y += init.g.height + border;

  init.g.width = screen_width - border * 2;
  init.text = "Description:";
  gwinLabelCreate(nullptr, &init);
  init.g.y += init.g.height + border;
  init.g.height = screen_height - (init.g.y - border);
  init.text = nullptr;
  const auto label_description = gwinLabelCreate(nullptr, &init);

  GListener listener;
  geventListenerInit(&listener);
  gwinAttachListener(&listener);

  while (true) {
    switch (const auto event = geventEventWait(&listener, gDelayForever);
            event->type) {
    case GEVENT_GWIN_BUTTON: {
      const auto button_event = reinterpret_cast<GEventGWinButton *>(event);
      if (button_event->gwin == button_exit) {
        gfxDeinit();
        return nullptr;
      }
      if (button_event->gwin == button_run) {
        const auto selected = gwinListGetSelected(list_names);
        gfxDeinit();
        if (selected == -1)
          return nullptr;
        return std::move(list_id_to_entry.at(selected).get());
      }
    } break;
    case GEVENT_GWIN_LIST: {
      const auto &list_event = *reinterpret_cast<GEventGWinList *>(event);
      if (list_event.gwin != list_names)
        break;
      const auto &exe = list_id_to_entry.at(list_event.item).get();
      gwinSetText(label_path, exe->getPath().get(), gTrue);
      gwinSetText(label_version, exe->getVersion().get(), gTrue);
      gwinSetText(label_author, exe->getAuthor().get(), gTrue);
      gwinSetText(label_description, exe->getDescription().get(), gTrue);
    } break;
    default:
      break;
    }
  }
}