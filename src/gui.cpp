#include "gui.hpp"

#include "gfx.h"
#include "loaders/elf/loader.hpp"
#include "loaders/interface.hpp"
#include <algorithm>
#include <forward_list>
#include <map>
#include <memory>
#include <utility>
#include <sdk/os/usb.h>
#include <sdk/os/misc.h>

struct usb_thread_params {
  bool exit;
  std::unique_ptr<std::byte[]> &memstorage;
  std::unique_ptr<Executable> ret;
};

static bool usb_connected() {
  return (*reinterpret_cast<const std::uint8_t *>(0xA4050162) & 2);
}

static const GSourceHandle usb_event_source_handle = reinterpret_cast<GSourceHandle>(usb_connected);
constexpr auto USB_LOADED = GEVENT_USER_FIRST;

static short read_bin(void *const out, const short sz) {
    /* KEYSC register holding row #0, which is the AC key */
    auto KEYSC_KIUDATA0 = reinterpret_cast<volatile const std::uint16_t *>(0xa44b0000);
    
    short count = 0;
    while (count < sz) {
      while (USB_PollRX() == 0) {
          //gfxSleepMilliseconds(25);
          ETMU_Sleep(25);
          if (*KEYSC_KIUDATA0 == 1 || !usb_connected())
              return -1;
      }
      short read = 0;
      USB_Read(out, std::min(USB_PollRX(), sz - count), &read);
      count += read;
    }
    return count;
}

static gThreadreturn usb_thread(void *param) {
  auto &params = *reinterpret_cast<usb_thread_params *>(param);

  while (!params.exit) {
    bool usb_status = usb_connected();
    
    if (usb_status) {
      int status;
      do {
          status = USB_Open(0x20);
          if (status == 10){
            USB_ForceClose(true);
            usb_status = false;
            break;
          }
      } while (status == 5);
    }
    if (usb_status) {
      USB_ClearRX();
      constexpr char handshake[] = "USB loader ready";
      USB_Write(handshake, sizeof(handshake));

      size_t file_size;
      if(read_bin(&file_size, sizeof(file_size)) == -1) {
        USB_ForceClose(true);
        continue;
      }
      params.memstorage = std::unique_ptr<std::byte[]>(new std::byte[file_size]);

      bool good = true;
      for (size_t offset = 0, next = 0x100; offset < file_size; offset = next, next += 0x100) {
        const short size = next > file_size ? next - file_size : 0x100;
        if(read_bin(params.memstorage.get() + offset, size) == -1) {
          good = false;
          break;
        }
      }
      USB_ForceClose(true);
      if(!good) {
        params.memstorage = nullptr;
        continue;
      };

      params.ret = std::make_unique<ELFLoader>(params.memstorage.get(), file_size, "\\usb\\memfile.hh3");
     
      for(GSourceListener *listerner = nullptr; (listerner = geventGetSourceListener(usb_event_source_handle, listerner));) {
        auto buffer = geventGetEventBuffer(listerner);
        if (!buffer) continue;
        buffer->type = USB_LOADED;
        geventSendEvent(listerner);
      }
      return 0;
    }

    gfxSleepMilliseconds(100);
  }

  return 0;
}

std::unique_ptr<Executable>
do_gui(std::forward_list<std::unique_ptr<Executable>> &executable_list, std::unique_ptr<std::byte[]> &memstorage) {
  gfxInit();
  usb_thread_params usb_params = {false, memstorage, nullptr};
  auto usb = gfxThreadCreate(nullptr, 1024, gThreadpriorityNormal, usb_thread, &usb_params);
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
  geventAttachSource(&listener, usb_event_source_handle, 0);
  gwinAttachListener(&listener);

  while (true) {
    switch (const auto event = geventEventWait(&listener, gDelayForever);
            event->type) {
    case GEVENT_GWIN_BUTTON: {
      const auto button_event = reinterpret_cast<GEventGWinButton *>(event);
      if (button_event->gwin == button_exit) {
        usb_params.exit = true;
        gfxThreadWait(usb);
        geventEventComplete(&listener);
        gfxDeinit();
        return nullptr;
      }
      if (button_event->gwin == button_run) {
        const auto selected = gwinListGetSelected(list_names);
        usb_params.exit = true;
        gfxThreadWait(usb);
        geventEventComplete(&listener);
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
    case USB_LOADED: {
      gfxThreadWait(usb);
      geventEventComplete(&listener);
      gfxDeinit();
      return std::move(usb_params.ret);
    }
    default:
      break;
    }
  }
}