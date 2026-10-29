#!/usr/bin/env python3
"""Patch JUCE 7.0.12 so its juceaide helper compiles with the macOS 15 SDK.

CGWindowListCreateImage is marked unavailable in the macOS 15 SDK, and JUCE
7.0.12's native window screenshot helper still calls it. The helper is only
used by JUCE's native-window screenshot API, which Gladden does not use, so it
is stubbed out when building against that SDK. The rest of JUCE is unchanged.
"""

from pathlib import Path
import sys


MARKER = "Gladden macOS 15 SDK workaround"


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} /path/to/JUCE")

    path = (
        Path(sys.argv[1])
        / "modules"
        / "juce_gui_basics"
        / "native"
        / "juce_Windowing_mac.mm"
    )
    if not path.is_file():
        raise SystemExit(f"JUCE source file not found: {path}")

    data = path.read_bytes()
    newline = b"\r\n" if b"\r\n" in data else b"\n"

    def with_newlines(text: str) -> bytes:
        return text.replace("\n", newline.decode()).encode()

    if MARKER.encode() in data:
        print(f"JUCE already patched: {path}")
        return

    old_head = """static Image createNSWindowSnapshot (NSWindow* nsWindow)
{
    JUCE_AUTORELEASEPOOL
"""
    new_head = f"""static Image createNSWindowSnapshot (NSWindow* nsWindow)
{{
   #if defined (MAC_OS_VERSION_15_0) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_VERSION_15_0
    // {MARKER}: CGWindowListCreateImage is unavailable in the macOS 15 SDK.
    // This function is only used by JUCE's native-window screenshot API.
    (void) nsWindow;
    return {{}};
   #else
    JUCE_AUTORELEASEPOOL
"""

    old_tail = """        return result;
    }
}

Image createSnapshotOfNativeWindow (void* nativeWindowHandle)"""
    new_tail = """        return result;
    }
   #endif
}

Image createSnapshotOfNativeWindow (void* nativeWindowHandle)"""

    encoded_head = with_newlines(old_head)
    encoded_tail = with_newlines(old_tail)
    if encoded_head not in data or encoded_tail not in data:
        raise SystemExit(
            "JUCE source does not match the expected 7.0.12 layout; "
            "update the macOS 15 patch"
        )

    data = data.replace(encoded_head, with_newlines(new_head), 1)
    data = data.replace(encoded_tail, with_newlines(new_tail), 1)
    path.write_bytes(data)
    print(f"Patched {path}")


if __name__ == "__main__":
    main()
