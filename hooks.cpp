// DX typedefs
typedef HRESULT(STDMETHODCALLTYPE* Present_t)(
    IDXGISwapChain* pSwapChain,
    UINT SyncInterval,
    UINT Flags
    );

typedef void (STDMETHODCALLTYPE* ExecuteCommandLists_t)(
    ID3D12CommandQueue* pQueue,
    UINT NumCommandLists,
    ID3D12CommandList* const* ppCommandLists
    );


Present_t Original_Present = nullptr;
ExecuteCommandLists_t Original_ExecuteCommandLists = nullptr;
bool g_gameQueueHooked = false;


// ========================================================
// Hooked: ID3D12CommandQueue::ExecuteCommandLists
// ========================================================
void STDMETHODCALLTYPE Hooked_ExecuteCommandLists(
    ID3D12CommandQueue* pQueue,
    UINT NumCommandLists,
    ID3D12CommandList* const* ppCommandLists)
{
    // Track this queue if it's the first time we've seen it
    if (g_renderQueue == nullptr) {
        D3D12_COMMAND_QUEUE_DESC desc = pQueue->GetDesc();

        if (desc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
            g_renderQueue = pQueue;
            g_dx12Context.commandQueue = pQueue;
            Log("ExecuteCommandLists: Storing DIRECT queue 0x%p for rendering", pQueue);
        }
    }

    // Call the original function
    Original_ExecuteCommandLists(pQueue, NumCommandLists, ppCommandLists);
}

// ========================================================
// Hooked Present - Single Present Call Approach
// ========================================================
HRESULT STDMETHODCALLTYPE Hooked_Present(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    g_frameCounter++;

    // ImGui renderer handle the rendering
    ImGuiRenderer::Get().RenderFrame(pSwapChain);

    // Call the original Present
    HRESULT result = Original_Present(pSwapChain, SyncInterval, Flags);

    return result;
}

// ========================================================
// Hook the game's queue
// ========================================================
bool HookCommandQueue(ID3D12CommandQueue* queue)
{
    if (!queue) {
        return false;
    }

    uintptr_t* vtable = *(uintptr_t**)queue;
    if (!vtable) {
        return false;
    }

    void* executeListsFunc = (void*)vtable[10];

    if (MH_CreateHook(executeListsFunc, &Hooked_ExecuteCommandLists,
        (LPVOID*)&Original_ExecuteCommandLists) != MH_OK)
    {
        return false;
    }

    MH_EnableHook(executeListsFunc);
    return true;
}

// ========================================================
// Hook Setup
// ========================================================
void InitializeHooks(IDXGISwapChain* swapChain)
{
    if (!swapChain) {
        return;
    }

    if (MH_Initialize() != MH_OK) {
        return;
    }

    // Initialize the ImGui renderer
    ImGuiRenderer::Get().Initialize();

    // 1) Hook Present
    uintptr_t* swapChainVtable = *(uintptr_t**)swapChain;
    void* presentFunc = (void*)swapChainVtable[8];
    if (MH_CreateHook(presentFunc, &Hooked_Present, (LPVOID*)&Original_Present) == MH_OK) {
        MH_EnableHook(presentFunc);
    }
    else {
        Log("Failed to hook Present");
    }

    // 2) Try to hook ExecuteCommandLists via a temporary queue
    ID3D12Device* device = nullptr;
    if (SUCCEEDED(swapChain->GetDevice(__uuidof(ID3D12Device), (void**)&device))) {
        D3D12_COMMAND_QUEUE_DESC qDesc = {};
        qDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        qDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        ID3D12CommandQueue* tempQueue = nullptr;
        if (SUCCEEDED(device->CreateCommandQueue(&qDesc, IID_PPV_ARGS(&tempQueue)))) {
            if (HookCommandQueue(tempQueue)) {
                Log("Hooked ExecuteCommandLists via temp queue successfully");
            }
            tempQueue->Release();
        }
        device->Release();
    }
}
