#include "sdl2_imgui.h"
#include "sdl2_stuff.h"
#include "frameinfo.h"
#include <float.h>

extern struct sdlobjs sdl;
extern struct frameinfo fi;

static const char *igsdl2_GetClipboardText(void *arg)
{
    (void)arg;
    if (sdl.clipboard)
        SDL_free(sdl.clipboard);

    sdl.clipboard = SDL_GetClipboardText();
    return sdl.clipboard;
}

static void igsdl2_SetClipboardText(void *arg, const char *text)
{
    (void)arg;
    SDL_SetClipboardText(text);
}

bool igsdl2_ProcessEvent(SDL_Event *e)
{
    ImGuiIO *io = igGetIO();
    switch (e->type) {
        case SDL_MOUSEWHEEL: {
            if (e->wheel.x > 0) io->MouseWheelH += 1;
            if (e->wheel.x < 0) io->MouseWheelH -= 1;
            if (e->wheel.y > 0) io->MouseWheel += 1;
            if (e->wheel.y < 0) io->MouseWheel -= 1;
            return true;
                             }

        case SDL_MOUSEBUTTONDOWN: {
            if (e->button.button == SDL_BUTTON_LEFT) sdl.mouse[0] = true;
            if (e->button.button == SDL_BUTTON_RIGHT) sdl.mouse[1] = true;
            if (e->button.button == SDL_BUTTON_MIDDLE) sdl.mouse[2] = true;
            return true;
                                  }

        case SDL_TEXTINPUT: {
            ImGuiIO_AddInputCharactersUTF8(io, e->text.text);
            return true;
                            }

        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            int key = e->key.keysym.scancode;
            io->KeysDown[key] = (e->type == SDL_KEYDOWN);
            io->KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
            io->KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
            io->KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
            io->KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
            return true;
                        }
        default:
            break;
    }

    return false;
}

void igsdl2_Init(void)
{
    simgui_desc_t simgui_desc = {0};
    simgui_setup(&simgui_desc);

    ImGuiIO *io = igGetIO();
    io->BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io->BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

    io->KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
    io->KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
    io->KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
    io->KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
    io->KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
    io->KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
    io->KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
    io->KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
    io->KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
    io->KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
    io->KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
    io->KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
    io->KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
    io->KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
    io->KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
    io->KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
    io->KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
    io->KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
    io->KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
    io->KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
    io->KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;

    io->SetClipboardTextFn = igsdl2_SetClipboardText;
    io->GetClipboardTextFn = igsdl2_GetClipboardText;
    io->ClipboardUserData = NULL;

    sdl.mouse[ImGuiMouseCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    sdl.mouse[ImGuiMouseCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    sdl.mouse[ImGuiMouseCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
    sdl.mouse[ImGuiMouseCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    sdl.mouse[ImGuiMouseCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    sdl.mouse[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    sdl.mouse[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    sdl.mouse[ImGuiMouseCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);

    printf("igsdl2_Init ok\n");
}

void igsdl2_Shutdown(void)
{
    if (sdl.clipboard)
        SDL_free(sdl.clipboard);

    sdl.clipboard = 0;
    for (int i = 0; i < ImGuiMouseCursor_COUNT; ++i)
        SDL_FreeCursor(sdl.cursor[i]);
        
}

void igsdl2_UpdateMousePosAndButtons(void)
{
    ImGuiIO *io = igGetIO();

    if (io->WantSetMousePos)
        SDL_WarpMouseInWindow(sdl.win, (int)io->MousePos.x, (int)io->MousePos.y);
    else
        io->MousePos = (ImVec2){-FLT_MAX, -FLT_MAX};

    int mx, my;
    uint32_t mb = SDL_GetMouseState(&mx, &my);
    io->MouseDown[0] = sdl.mouse[0] || (mb & SDL_BUTTON(SDL_BUTTON_LEFT));
    io->MouseDown[1] = sdl.mouse[1] || (mb & SDL_BUTTON(SDL_BUTTON_RIGHT));
    io->MouseDown[2] = sdl.mouse[2] || (mb & SDL_BUTTON(SDL_BUTTON_MIDDLE));
    sdl.mouse[0] = 0;
    sdl.mouse[1] = 0;
    sdl.mouse[2] = 0;

    if (fi.vd != VD_WAYLAND) {
        int wx, wy;
        SDL_GetWindowPosition(sdl.win, &wx, &wy);
        SDL_GetGlobalMouseState(&mx, &my);
        mx -= wx;
        my -= wy;
    }

    io->MousePos = (ImVec2){(float)mx, (float)my};

    bool anymouse = igIsAnyMouseDown();
    SDL_CaptureMouse(anymouse);
}

void igsdl2_UpdateMouseCursor(void)
{
    ImGuiIO *io = igGetIO();

    if (io->ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;

    int igcursor = igGetMouseCursor();
    if (io->MouseDrawCursor || igcursor == ImGuiMouseCursor_None) {
        SDL_ShowCursor(false);
    }
    else {
        SDL_SetCursor(sdl.cursor[igcursor]);
        SDL_ShowCursor(true);
    }
}
