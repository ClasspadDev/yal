#pragma once

#include "loaders/interface.hpp"
#include <forward_list>
#include <memory>

std::unique_ptr<Executable>
do_gui(std::forward_list<std::unique_ptr<Executable>> &list);