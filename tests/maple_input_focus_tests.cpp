#include "dcr_input_focus.hpp"
#include <cassert>

using dcrecomp_generated::dcr_focus_allows_input;

static_assert(dcr_focus_allows_input(true, true, true, false));
static_assert(!dcr_focus_allows_input(false, true, true, false));
static_assert(!dcr_focus_allows_input(true, false, true, false));
static_assert(!dcr_focus_allows_input(true, true, false, false));
static_assert(!dcr_focus_allows_input(true, true, true, true));

int main() {
    // Switching to launcher/desktop must always neutralize game inputs,
    // even when a host controller remains physically connected.
    assert(dcr_focus_allows_input(true, true, true, false));
    assert(!dcr_focus_allows_input(true, false, true, false));
    assert(!dcr_focus_allows_input(true, true, false, false));
    assert(!dcr_focus_allows_input(false, false, false, false));
    assert(!dcr_focus_allows_input(true, true, true, true));
}
