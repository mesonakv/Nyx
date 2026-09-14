#include "Action.h"
#include <cstring>

namespace {

struct ActionName {
    Action action;
    const char* name;
};

const ActionName kActionNames[] = {
    { Action::MoveForward,   "MoveForward" },
    { Action::MoveBackward,  "MoveBackward" },
    { Action::MoveLeft,      "MoveLeft" },
    { Action::MoveRight,     "MoveRight" },
    { Action::Jump,          "Jump" },
    { Action::Dash,          "Dash" },
    { Action::Slide,         "Slide" },
    { Action::PrimaryFire,   "PrimaryFire" },
    { Action::SecondaryFire, "SecondaryFire" },
    { Action::ToggleEditor,  "ToggleEditor" },
    { Action::Pause,         "Pause" },
};

} // anonymous namespace

const char* ActionToString(Action a) {
    for (const auto& e : kActionNames) {
        if (e.action == a) return e.name;
    }
    return "Unknown";
}

bool StringToAction(const char* s, Action& out) {
    if (!s) return false;
    for (const auto& e : kActionNames) {
        if (std::strcmp(e.name, s) == 0) {
            out = e.action;
            return true;
        }
    }
    return false;
}