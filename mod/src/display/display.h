#pragma once

namespace dh2::display {

// Horizontal degrees the perspective projection uses this frame.
// At 4:3 this is `stored`. Wider screens keep the 4:3 vertical view and widen the horizontal one.
float EffectiveHorizontalFov(float stored_degrees);

// [Display] DpiAware (default 1): declare the process DPI aware before the game makes its window. Without it, with
// Windows display scaling above 100%, Windows hands the game a scaled-down desktop (2560x1440 at 125% reads
// 2048x1152): the mod's work-area size, the game's resolution list and the picture (stretched) all come out small.
// Called from DllMain (no logging there); OnInit logs what happened.
void MakeDpiAware();

}  // namespace dh2::display
