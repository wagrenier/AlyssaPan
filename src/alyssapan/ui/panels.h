#pragma once

#include <memory>
#include <vector>

#include "alyssapan/ui/overlay.h"

namespace alyssapan::ui
{

// The menu bar's content, left to right.
std::vector<std::unique_ptr<Panel>> createPanels(Application &application);

} // namespace alyssapan::ui
