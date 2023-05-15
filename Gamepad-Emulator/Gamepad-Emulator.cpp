#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Xinput.h>
#include <ViGEm/Client.h>
#include <iostream>

int returnK(int num) {
    return (num != 0) ? 1 : 0;
}
BYTE keyStates[256] = { 0 };
BYTE mouseButtons[2] = { 0 };

PVIGEM_CLIENT client = vigem_alloc();
PVIGEM_TARGET target = vigem_target_x360_alloc();
auto err = vigem_connect(client);
XUSB_REPORT report = {};

LRESULT CALLBACK Hook(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT* pKeyboardStruct = (KBDLLHOOKSTRUCT*)lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            keyStates[pKeyboardStruct->vkCode] = 0x80;
        }
        else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
        {
            keyStates[pKeyboardStruct->vkCode] = 0x00;
        }

        report.sThumbLY = ((keyStates['S'] & 0x80) * 10000) - ((keyStates['W'] & 0x80) * 10000);
        report.sThumbLX = ((keyStates['A'] & 0x80) * 10000) - ((keyStates['D'] & 0x80) * 10000);

        report.wButtons =
            returnK(keyStates[162]) * XUSB_GAMEPAD_B
            | returnK(keyStates[VK_SPACE]) * XUSB_GAMEPAD_A
            | returnK(keyStates[0x51]) * XUSB_GAMEPAD_LEFT_SHOULDER
            | (returnK(keyStates[0x31]) + returnK(keyStates[0x32])) * XUSB_GAMEPAD_Y
            | returnK(keyStates[VK_TAB]) * XUSB_GAMEPAD_START
            | returnK(keyStates['M']) * XUSB_GAMEPAD_BACK
            | returnK(keyStates[160]) * XUSB_GAMEPAD_LEFT_THUMB
            | returnK(keyStates[0x42]) * XUSB_GAMEPAD_RIGHT_THUMB
            | returnK(keyStates[0x5A]) * (XUSB_GAMEPAD_LEFT_SHOULDER + XUSB_GAMEPAD_RIGHT_SHOULDER)
            | returnK(keyStates[0x34]) * XUSB_GAMEPAD_DPAD_UP
            | returnK(keyStates[0x56]) * XUSB_GAMEPAD_DPAD_LEFT
            | returnK(keyStates[0x47]) * XUSB_GAMEPAD_DPAD_RIGHT
            | returnK(keyStates[0x4E]) * XUSB_GAMEPAD_DPAD_DOWN
            | returnK(keyStates[0x45]) * XUSB_GAMEPAD_X;


        err = vigem_target_add(client, target);

        err = vigem_target_x360_update(client, target, report);
        // std::cout << pKeyboardHookStruct->vkCode;
        if (pKeyboardStruct->vkCode)
        {
            return 1;
        }

    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}


int dx = 0;
int dy = 0;
int last_x = 0;
int last_y = 0;

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        MSLLHOOKSTRUCT* pMouseStruct = (MSLLHOOKSTRUCT*)lParam;
        switch (wParam)
        {
        case WM_LBUTTONDOWN:
            mouseButtons[0] = 1;
            break;
        case WM_LBUTTONUP:
            mouseButtons[0] = 0;
            break;
        case WM_RBUTTONDOWN:
            mouseButtons[1] = 1;
            break;
        case WM_RBUTTONUP:
            mouseButtons[1] = 0;
            break;
        }
        int x = pMouseStruct->pt.x;
        int y = pMouseStruct->pt.y;
        dx = x - last_x;
        dy = y - last_y;
        last_x = x;
        last_y = y;

        report.sThumbRX = (dx == last_x) ? 1 : dx * 1;
        report.sThumbRY = (dy == last_y) ? 0 : -dy * 1;

        report.bRightTrigger = (mouseButtons[0]) ? 32767 : 0;
        report.bLeftTrigger = (mouseButtons[1]) ? 32767 : 0;

        err = vigem_target_add(client, target);

        err = vigem_target_x360_update(client, target, report);

        if (!returnK(keyStates['P'])) { return 1; }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}


int main()
{
    HANDLE hProcess = GetCurrentProcess();
    SetPriorityClass(hProcess, REALTIME_PRIORITY_CLASS);

    HHOOK hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);
    HHOOK hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, Hook, NULL, 0);

    while (true) {
        err = vigem_target_x360_update(client, target, report);
        HHOOK hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, NULL, 0);
        HHOOK hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, Hook, NULL, 0);
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    UnhookWindowsHookEx(hKeyboardHook);
}
