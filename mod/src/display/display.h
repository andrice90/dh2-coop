#pragma once

namespace dh2::display {

// Horizontal degrees the perspective projection uses this frame.
// At 4:3 this is `stored`. Wider screens keep the 4:3 vertical view and widen the horizontal one.
float EffectiveHorizontalFov(float stored_degrees);

}  // namespace dh2::display
