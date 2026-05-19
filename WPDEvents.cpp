#include "WPDEvents.h"
#include "WPDManager.h"
#include <PortableDevice.h> 
#include <windows.h>
#include <dbt.h>
#include <QString>

#define MESSAGE_EXIT_WPDUSBDETECTOR WM_USER+1
GUID USBGUID = { 0x25dbce51, 0x6c8f, 0x4a72, 0x8a, 0x6d, 0xb5, 0x4c, 0x2b, 0x4f, 0xc8, 0x35 };


WPDUSBDetector::WPDUSBDetector(WPDManager & manager) :
    m_manager(manager), m_hWnd(NULL), m_hDeviceNotify(NULL), m_arrivalCount(0)
{
}

WPDUSBDetector::~WPDUSBDetector()
{
    PostMessage(m_hWnd, MESSAGE_EXIT_WPDUSBDETECTOR, 0, 0);
    wait();
}

void WPDUSBDetector::run()
{
    initialize();

    int ret = 0;
    MSG msg;
    forever{
        ret = GetMessage(&msg, NULL, 0, 0);
        TranslateMessage(&msg);	
        DispatchMessage(&msg);
        if (ret <= 0)
            break;
    }

    finalize();
}

void WPDUSBDetector::initialize()
{
    const wchar_t CLASS_NAME[] = L"Device Detector";

    WNDCLASSEX wx;
    ZeroMemory(&wx, sizeof(wx));
    wx.cbSize = sizeof(WNDCLASSEX);
    wx.hInstance = reinterpret_cast<HINSTANCE>(GetModuleHandle(0));
    wx.lpfnWndProc = reinterpret_cast<WNDPROC>(connectedUSBCallback);
    wx.lpszClassName = reinterpret_cast<LPCSTR>(CLASS_NAME);

    if (RegisterClassEx(&wx)) {
        m_hWnd = CreateWindowEx(0, wx.lpszClassName, wx.lpszClassName, 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(0), this);

        if (m_hWnd) {
            DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;
            ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
            NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
            NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
            NotificationFilter.dbcc_classguid = USBGUID;

            m_hDeviceNotify = RegisterDeviceNotification(m_hWnd, &NotificationFilter, DEVICE_NOTIFY_WINDOW_HANDLE);
        }
    }
}

void WPDUSBDetector::finalize()
{
    UnregisterDeviceNotification(m_hDeviceNotify);
    DestroyWindow(m_hWnd);
    m_hDeviceNotify = NULL;
    m_hWnd = NULL;
}

LRESULT WPDUSBDetector::connectedUSBCallback(HWND__ *hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WPDUSBDetector* detector = nullptr;
    LRESULT lRet = 1;

    switch (message) {
    case WM_CREATE:
        detector = static_cast<WPDUSBDetector *>(reinterpret_cast<CREATESTRUCT *>(lParam)->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(detector));
        break;

    case WM_DEVICECHANGE:
        if (wParam == DBT_DEVICEARRIVAL) {
            detector = reinterpret_cast<WPDUSBDetector *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            if (detector)
                detector->m_arrivalCount = 2;
        }
        else if (wParam == DBT_DEVNODES_CHANGED) {
            detector = reinterpret_cast<WPDUSBDetector *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            if (detector && detector->m_arrivalCount > 0 && --detector->m_arrivalCount == 0) {
                detector->m_manager.refreshDevices();
            }
        }
        break;

    case MESSAGE_EXIT_WPDUSBDETECTOR:
        PostQuitMessage(0);
        break;

    default:
        lRet = DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return lRet;
}


WPDDeviceEventsCallback::WPDDeviceEventsCallback(WPDManager &manager) :
    m_ref(0), m_manager(manager)
{
}

WPDDeviceEventsCallback::~WPDDeviceEventsCallback()
{
}

HRESULT WPDDeviceEventsCallback::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    HRESULT hr = S_OK;
    if (ppvObj == NULL) {
        hr = E_INVALIDARG;
        return hr;
    }

    if ((riid == IID_IUnknown) || (riid == IID_IPortableDeviceEventCallback)) {
        AddRef();
        *ppvObj = this;
    }
    else {
        hr = E_NOINTERFACE;
    }
    return hr;
}

ULONG WPDDeviceEventsCallback::AddRef()
{
    return InterlockedIncrement(&m_ref);
}

ULONG WPDDeviceEventsCallback::Release()
{
    long ref = InterlockedDecrement(&m_ref);
    if (ref == 0)
    {
        delete this;
        return 0;
    }
    return ref;
}

HRESULT WPDDeviceEventsCallback::OnEvent(IPortableDeviceValues * eventParameters)
{
    if (eventParameters != nullptr) {
        HRESULT hr = E_FAIL;
        QString deviceID, parentID, objectID;
        PWSTR tempStr = nullptr;
        GUID guid = GUID_NULL;

        hr = eventParameters->GetStringValue(WPD_EVENT_PARAMETER_PNP_DEVICE_ID, &tempStr);
        if (SUCCEEDED(hr))
            deviceID = QString::fromWCharArray(tempStr);
        CoTaskMemFree(tempStr);
        tempStr = nullptr;

        hr = eventParameters->GetStringValue(WPD_OBJECT_PARENT_ID, &tempStr);
        if (SUCCEEDED(hr))
            parentID = QString::fromStdWString(tempStr);
        CoTaskMemFree(tempStr);
        tempStr = nullptr;

        hr = eventParameters->GetStringValue(WPD_OBJECT_ID, &tempStr);
        if (SUCCEEDED(hr))
            objectID = QString::fromStdWString(tempStr);
        CoTaskMemFree(tempStr);
        tempStr = nullptr;

        hr = eventParameters->GetGuidValue(WPD_EVENT_PARAMETER_EVENT_ID, &guid);
        if (SUCCEEDED(hr))
        {
            if (guid == WPD_EVENT_DEVICE_REMOVED || guid == WPD_EVENT_DEVICE_RESET) {
                m_manager.removeDevice(deviceID);
            }
            else if (guid == WPD_EVENT_OBJECT_ADDED) {
                m_manager.addObject(deviceID, parentID, objectID);
            }
            else if (guid == WPD_EVENT_OBJECT_UPDATED) {
                m_manager.updateObject(deviceID, objectID);
            }
            else if (guid == WPD_EVENT_OBJECT_REMOVED) {
                m_manager.removeObject(deviceID, objectID);
            }
        }
    }

    return S_OK;
}
