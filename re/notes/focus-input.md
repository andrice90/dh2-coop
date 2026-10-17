# Focus-only DirectInput

**Progress:** done. MAP3B pair: ESCAPE, walk, and mouse look stay on the window in front. Focus gain/loss is logged, including after DirectInput unacquires. F7, F8, and F9 still toggle through `GetAsyncKeyState`.

## Problem

Local multi-window tests share one keyboard. A scan code sent to one window was read by every game. The games keep simulating when they are not in front (`cApplication::m_bAllowBackground`, set by `AssertBackground`). On two PCs this does not matter.

## What the engine does

`InputInit` (EngineDll7 RVA `0x34ae90`, source string `Platform\Win32\Input\InputDX.cpp`) calls `DirectInput8Create` version `0x800` and builds a `cInputDX` object (`FUN_103485e0`). Runtime caller of `DirectInput8Create` is `EngineDll7.dll+0x34c64e`.

`cInput::Init` / `Acquire` / `UnAcquire` / `PollDevices` (RVAs `0xa6720`, `0xa6740`, `0xa6750`, `0xa68b0`) return immediately. The DX overrides do the work.

Keyboard setup `FUN_10344fd0` and mouse setup `FUN_10345440`:

- `IDirectInput8::CreateDevice` is vtable slot 3 (`+0x0c`) on the object at `input+0x212c`. Runtime: one `GUID_SysKeyboard` device and one `GUID_SysMouse` device. They share one device vtable (`DINPUT8.dll` `GetDeviceState` `+0xde80`, `GetDeviceData` `+0xdcd0`).
- `SetDataFormat` is slot 11 (`+0x2c`).
- `SetCooperativeLevel` is slot 13 (`+0x34`) with flags **decimal 10 = `0xA` = `DISCL_NONEXCLUSIVE | DISCL_BACKGROUND`**. Runtime log: `flags=0xa (NONEXCLUSIVE|BACKGROUND)` for both devices, hwnd = the game window (`D3D8` `hDeviceWindow` is the same hwnd).
- `SetProperty` slot 6 (`+0x18`) sets a 64-event buffer (`dwSize 0x14`, `dwHeaderSize 0x10`, `dwData 0x40`).

Reads, only while the wrapper's acquired flag at `+0xc` is set:

- Keyboard `FUN_1034a090`: `GetDeviceData` (slot 10, `+0x28`) with `cbObjectData 0x14`. Runtime caller `EngineDll7.dll+0x34a111`.
- Mouse `FUN_1034a380`: `GetDeviceData` the same way, then `GetDeviceState` (slot 9, `+0x24`) of 0x14 bytes. Runtime caller `EngineDll7.dll+0x34a459`, `cb=20`.
- `DIERR_INPUTLOST` (`-0x7ff8ffe2`) is logged as `Input Lost` and the wrapper re-acquires. `Acquire` succeeds while the window is behind because the cooperative level is `DISCL_BACKGROUND`. That is why a background instance still saw keys.

`cInputDX::Acquire` (`FUN_10348720`, RVA `0x348720`) calls wrapper vtable `+0x14` (which calls COM `Acquire` at device `+0x1c`). `cInputDX::UnAcquire` (`FUN_103488b0`, RVA `0x3488b0`, log `Input unacquire`) calls wrapper `+0x18` (COM `Unacquire` at `+0x20`). Main7's focus handlers (`0x13c30` / `0x13d70` / `0x13ea0`) call input vtable `+0x0c` / `+0x10`, which land on those two. After `UnAcquire` the read functions skip `GetDeviceData`, so a focus change is not visible from the device hooks alone.

## Hook

`mod/src/engine/dinput_hooks.cpp`, installed from `DllMain` (before `EngineDll7` calls `DirectInput8Create`).

- Inline hook on the ASI loader's `DirectInput8Create`, then on `IDirectInput8::CreateDevice`.
- Vtable hooks on the system keyboard/mouse device: `GetDeviceState`, `GetDeviceData`, `SetCooperativeLevel` (logged once per device), `Release`.
- While `[Coop] InputOnlyWhenFocused` is 1 (default) and the D3D device window (`d3d8::DeviceWindow`, else the `SetCooperativeLevel` hwnd) is not the foreground window: `GetDeviceState` zeroes the caller's buffer and returns `DI_OK`; `GetDeviceData` drains the device buffer, reports 0 events, and returns `DI_OK`. In front, both pass through. The drain is so keys that arrived while the window was behind do not replay on the next focus.
- One log line when the hook is installed: `dinput: hook installed InputOnlyWhenFocused=1`.
- One log line each time that window gains or loses input. The lifecycle thread samples this as well, because the game stops calling `GetDeviceData` after it unacquires.
- F7 / F8 / F9 are unchanged (`GetAsyncKeyState` in `overlay_module.cpp` and `coop.cpp`, each with its own foreground check).

## Test

Worktree `D:\scary hunt\dh2coop-input`, `run-coop-local.ps1 -Map MAP3B`. Host pid 22644 (`game`), client pid 21844 (`game2`). 2026-10-17 16:10–16:12.

`game-input.ps1 -ProcessId 22644 -Keys ESCAPE` (foreground ok). Host opened the pause menu. Client stayed in the level (knife, no pause menu).

- `re/notes/coop-local/focus-input-escape-host.png`
- `re/notes/coop-local/focus-input-escape-client.png`

Host log: `dinput: window gained input (hwnd=0x130788)` at 16:11:30. Client had logged `window lost input` at startup and did not log `gained` for that ESCAPE.

Second ESCAPE closed the host menu. Then `-ProcessId 21844 -Keys W:2000` and `MOUSE:200,0`.

Host log `coop: remote 1 'P2'`: before, `pos=(-3.42,-2.91,-68.28) yaw=180.228`; after, `pos=(-3.42,-3.24,-80.64) yaw=196.368` (moved about 12 m on Z, yaw +16°, in line with 200 DirectInput units). Client log `coop: remote 0 'Host'` stayed `pos=(-3.42,-2.91,-68.28) yaw=180.228`. Client logged `window gained input (hwnd=0xd077e)` at 16:11:50. On that build the host did not log `lost` for the switch, because device reads stop after unacquire.

Second launch (host pid 7412, client pid 620, same day 16:22) with the lifecycle sample: switching the foreground window from host to client logged `dinput: window lost input (hwnd=0x640772)` on the host at 16:23:00.161 and `dinput: window gained input (hwnd=0x1d0774)` on the client at 16:23:00.134.

F7 / F8 / F9 on the focused client (`keybd_event`, client log 16:24:11–16:24:12): `overlay: visible=0`, `overlay: markers=0`, `coop: F9 respawn at (-3.4,-2.7,-68.3) health 100.0 ok=true`. Those keys are `GetAsyncKeyState` and are not part of the DirectInput hooks.

## Proposed MEMORY facts

- `cInputDX` keyboard and mouse use `DISCL_NONEXCLUSIVE|DISCL_BACKGROUND` (`SetCooperativeLevel` flags `0xA`). Evidence: `FUN_10344fd0` / `FUN_10345440` pass decimal `10`; `dh2coop.log` `dinput: SetCooperativeLevel keyboard/mouse ... flags=0xa (NONEXCLUSIVE|BACKGROUND)`.
- Keyboard input is buffered `GetDeviceData` (`cbObjectData 0x14`, 64-event buffer) in `FUN_1034a090` (RVA `0x34a090`), only while the device wrapper `+0xc` is acquired. Mouse `FUN_1034a380` (RVA `0x34a380`) calls `GetDeviceData` then `GetDeviceState` of `0x14` bytes. Runtime: `GetDeviceData cb=20 caller=EngineDll7.dll+0x34a111`, `GetDeviceState cb=20 caller=EngineDll7.dll+0x34a459`.
- `DIERR_INPUTLOST` (`-0x7ff8ffe2`) makes those readers call Acquire again, and Acquire succeeds in the background because of the cooperative level above. Evidence: the `Input Lost` branch in both functions, plus the flags.
- `cInputDX::Acquire` RVA `0x348720`, `cInputDX::UnAcquire` RVA `0x3488b0` (`Input unacquire`). COM Acquire/Unacquire on the device are vtable `+0x1c` / `+0x20` (`FUN_10344cf0` / `FUN_10344e20`).
