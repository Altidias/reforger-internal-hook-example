# offsets (29/05/2025):

- Game: ArmaReforgerSteam.exe + 0x1D521A8
- World: Game + 130
  
- CameraManager_Weak: Game + 308
- CameraManager: CameraManager_Weak + 8 <-- Contains FOV data for playerCamera

- LocalPlayerController_Weak: World + 378
- LocalPlayerController: LocalPlayerController_Weak + 8
- LocalPlayerCamera: LocalPlayerController + C0 <-- Contains position

- EngineWin: Game + B8
- RendererImpl: EngineWin + 148
- SwapChain: RendererImpl + 114548
- IDXGISwapChain: SwapChain + C8

or

- IDXGISwapChain Static: gameoverlayrenderer64.dll+163348
- RendererImpl Static: ArmaReforgerSteam.exe+1C84F40
