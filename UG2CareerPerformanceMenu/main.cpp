#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <MinHook.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <intrin.h>

#if !defined(_M_IX86)
#error Este projeto deve ser compilado como Win32 (x86).
#endif

// NFS Underground 2 v1.2 (SPEED2.EXE, 4.800.512 bytes).
// Etapa 19.1: compatibilidade com FrontB original; copia o icone REAL do Stage 3 em tempo de execucao.
// Preserva os hooks de nome Unique e bloqueio de compra/instalacao da Etapa 19.
// Nao altera FrontB, save, tabelas do jogo ou partRef; intercepta chamadas de compra/instalacao por identidade do pacote privado.
// Mantem 10-13 da Etapa 12. FEngSetVisible e FNG auxiliar continuam desativados.
// O modo 14 nao escreve texto dentro do Draw; modo 15 e experimental.
// Modo 11 e experimental: nao clicar Install Package.
// Durante BuyPerformanceParts::Setup, somente na Extreme confirmada, o filtro
// nativo de desbloqueio e temporariamente negado, levando a rotina a construir
// seu proprio widget de lista vazia (NoPerfPartsWidgy), que exibimos como status.
// Nao cria PerfPart, nao compra nada e nao escreve indicadores de save.
// A caixa Extreme NAO compra/instala pecas: nenhuma escrita no save nem nos indicadores.
// Enderecos e estrutura da opcao baseados no NFSU2Unlimiter/ChoosePerformancePackages.h.
namespace CareerMenu
{
    constexpr uintptr_t kSetupVtableEntry = 0x0079D2E4;
    constexpr uintptr_t kFEDatabase = 0x00850078;
    constexpr uintptr_t kGarageContext = 0x0083898C;
    constexpr uintptr_t kGameHash = 0x0043DB50;
    constexpr uintptr_t kGameMalloc = 0x00575620;
    // Rotinas confirmadas no InGameFunctions.h do NFSU2Unlimiter (v1.2 US).
    constexpr uintptr_t kFEngGetLastButton = 0x0050B8A0;
    constexpr uintptr_t kFEngSetLastButton = 0x0050B8C0;
    constexpr uintptr_t kUIWidgetMenuAddStatOption = 0x0054F820;
    constexpr uintptr_t kUIWidgetMenuSetInitialOption = 0x00539650;
    constexpr uintptr_t kThumbnailScrollerGetCurrentNodeIndex = 0x0051F7E0;
    // Referencias publicadas em NFSU2Unlimiter/UnlimiterStuff.h e BuyPerformanceParts.h.
    constexpr uintptr_t kBuyPerformancePartsSetupVtableEntry = 0x0079D8CC;
    constexpr uintptr_t kUnlockSystemIsPerfPartUnlocked = 0x0052A090;
    constexpr uintptr_t kNoPerfPartsWidgetVtable = 0x0079B660;
    constexpr DWORD kBuyPerfPartWidgetVtable = 0x0079B620; // confirmada pelo trace da Etapa 7
    // NFSU2Unlimiter/UnlimiterStuff.h: slot + 0x0C e BuyPerfPartWidget::Draw.
    // SOMENTE LEITURA do ponteiro da vtable nativa; MinHook atua na funcao apontada.
    constexpr uintptr_t kBuyPerfPartWidgetDrawEntry = 0x0079B62C;
    constexpr uintptr_t kFEngPrintf = 0x00537BE0;
    constexpr uintptr_t kGetLocalizedString = 0x004FFA80; // NFSU2Unlimiter/InGameFunctions.h (v1.2).
    // Enderecos de compra/instalacao do executavel PC 1.2 (referencia nfsu2-gcpc/bridge_win32.cpp).
    // 0x54FE50: BuyPerfPackage, 0x521340: InstallPerfPackage.
    // 0x521B00: BuyPerfPart, 0x5213D0: InstallPerfPart.
    constexpr uintptr_t kCCMBuyPerfPackage = 0x0054FE50;
    constexpr uintptr_t kCCMInstallPerfPackage = 0x00521340;
    constexpr uintptr_t kCCMBuyPerfPart = 0x00521B00;
    constexpr uintptr_t kCCMInstallPerfPart = 0x005213D0;
    constexpr uintptr_t kSelectedPerfPackage = 0x00839BB4; // nfsu2-gcpc bridge_win32.cpp
    // NFSU2Unlimiter/InGameFunctions.h (US v1.2); callbacks passivos, ABI __cdecl.
    constexpr uintptr_t kFEngSetLanguageHashObj = 0x0050C900;
    constexpr uintptr_t kFEngSetScriptObj = 0x0051CF70;
    // Etapa 6: apenas diagnostico da tela FNG separada, sem alterar lista original.
    // Enderecos publicados no NFSU2Unlimiter/InGameFunctions.h (jogo US v1.2).
    constexpr uintptr_t kFEngShowPackage = 0x00555F10;
    constexpr uintptr_t kFEngFindObject = 0x005379C0;
    constexpr uintptr_t kFEngSetVisible = 0x0050CA50;
    constexpr uintptr_t kFEngSetInvisible = 0x0050CA00;
    constexpr DWORD kUniqueRootHash = 0x6096221D; // UG2_UNIQUE_STATUS_ROW
    constexpr DWORD kNativePartsAnchorHash = 0x8D7E0C53; // grupo existente UI_BuyPerformanceParts
    // Fonte: objetos raiz de linhas e textos option_name/option_data do UI_BuyPerformanceParts.json original.
    // Os 12 grupos NAO incluem o grupo das Performance Stats (0x8D7E0C53).
    constexpr DWORD kNativeRowHashes[36] = {
        0x4335C028, 0x4335C027, 0x4335C026, 0x66E2993E, 0x66E2993D, 0x66E2993C,
        0x66E2993B, 0x66E2993A, 0x66E29939, 0x66E29938, 0x66E29937, 0x66E29936,
        0x044F9B53, 0x059980DA, 0x044F9B52, 0x059980D9, 0x044F9B51, 0x059980D8,
        0x9B483B09, 0xB2980BB0, 0x9B483B08, 0xB2980BAF, 0x9B483B07, 0xB2980BAE,
        0x9B483B06, 0xB2980BAD, 0x9B483B05, 0xB2980BAC, 0x9B483B04, 0xB2980BAB,
        0x9B483B03, 0xB2980BAA, 0x9B483B02, 0xB2980BA9, 0x9B483B01, 0xB2980BA8
    };
    constexpr const char* kUniqueFNG = "UI_UniquePerformanceStatus.fng";
    constexpr const char* kNativePartsFNG = "UI_BuyPerformanceParts.fng";
    constexpr const char* kCarouselFNG = "UI_ChoosePerformancePackage.fng";
    constexpr uintptr_t kFECarConfigRef = 0x008389AC;
    constexpr uintptr_t kGetCarTypeInfo = 0x00610110;
    // In NFSU2-Savegame-Editor/technical.md: FUN_0052A280 usa a colecao
    // PlayerProfile + 0x3B0; a garagem de carreira fica em colecao + 0x5954.
    // NAO aplicar 0x5954 diretamente a ThePlayerCareer (0x85AD40): e outra estrutura.
    // O mapeamento depende do SPEED2.EXE v1.2 suportado; somente leitura.
    constexpr uintptr_t kPlayerProfileBase = 0x0083A9E0;
    constexpr SIZE_T kPlayerCollectionOffset = 0x3B0;
    constexpr SIZE_T kCareerGarageOffset = 0x5954;
    constexpr uintptr_t kPlayerCollectionBase = kPlayerProfileBase + kPlayerCollectionOffset;
    constexpr uintptr_t kCareerGarageBase = kPlayerCollectionBase + kCareerGarageOffset;
    constexpr SIZE_T kCareerCarStride = 0x7F8;
    constexpr SIZE_T kCareerCarTypeMaskOffset = 0x14;
    constexpr SIZE_T kCareerCarHashOffset = 0x20;
    constexpr SIZE_T kCareerUniqueFlagsOffset = 0x2F4;
    constexpr SIZE_T kCareerCarLockedOffset = 0x7F4;
    constexpr SIZE_T kCareerCarPurchasedOffset = 0x7F5;
    constexpr uintptr_t kCurrentPackageType = 0x00838994;
    constexpr uintptr_t kIndividualOptionVtable = 0x0079B604;
    constexpr uintptr_t kPackageOptionVtable = 0x0079B60C; // PPPackage, igual Stage 1/2/3
    constexpr uint64_t kExpectedExeSize = 4800512ULL;
    constexpr SIZE_T kOptionSize = 0x4C;

    using SetupFn = void(__thiscall*)(void*);
    using UnlockPerfPartFn = bool(__cdecl*)(int, int, int);
    using NoPerfDrawFn = int(__thiscall*)(DWORD*);
    using GamePrintfFn = int(__cdecl*)(DWORD*, const char*, ...);
    using GetLocalizedStringFn = const char* (__cdecl*)(DWORD);
    // __thiscall x86: this=ECX; os hooks __fastcall recebem EDX no segundo argumento.
    using CCMBuyPartOrPackageFn = void(__thiscall*)(void*, const BYTE*);
    using CCMInstallPartOrPackageFn = void(__thiscall*)(void*, const BYTE*, void*);
    using FEngSetLanguageHashObjFn = void(__cdecl*)(DWORD*, unsigned int);
    using FEngSetScriptObjFn = void(__cdecl*)(DWORD*, const char*, bool);
    using FEngShowPackageFn = void(__cdecl*)(const char*, const char*, unsigned int, int);
    using FEngFindObjectFn = DWORD* (__cdecl*)(const char*, DWORD);
    using FEngVisibilityFn = int(__cdecl*)(DWORD*);
    using FEngGetLastButtonFn = DWORD(__cdecl*)(const char*);
    using FEngSetLastButtonFn = void(__cdecl*)(const char*, DWORD);
    using UIWidgetMenuAddStatOptionFn = void(__thiscall*)(void*, void*);
    using UIWidgetMenuSetInitialOptionFn = void(__thiscall*)(void*, int);
    using BuyPerfPartWidgetDrawFn = void(__thiscall*)(DWORD*);
    using ThumbnailScrollerGetCurrentNodeIndexFn = int(__thiscall*)(void*);
    using AddOptionFn = void(__thiscall*)(void*, DWORD*);
    using GameHashFn = DWORD(__cdecl*)(const char*);
    using GameMallocFn = void* (__cdecl*)(size_t);
    using FECarTypeFn = int(__thiscall*)(void*);
    using GetCarTypeInfoFn = const BYTE* (__cdecl*)(int);

    GetLocalizedStringFn gOriginalGetLocalizedString = nullptr;
    CCMBuyPartOrPackageFn gOriginalBuyPerfPackage = nullptr;
    CCMInstallPartOrPackageFn gOriginalInstallPerfPackage = nullptr;
    CCMBuyPartOrPackageFn gOriginalBuyPerfPart = nullptr;
    CCMInstallPartOrPackageFn gOriginalInstallPerfPart = nullptr;
    bool gStage19MutationGuardsReady = false;
    FEngSetLanguageHashObjFn gStage19OriginalSetLanguageHash = nullptr;
    bool gStage19SetLanguageHookReady = false;
    LONG gStage19SetLanguageMatches = 0;
    LONG gStage19BlockedBuyPackage = 0;
    LONG gStage19BlockedInstallPackage = 0;
    LONG gStage19BlockedBuyPart = 0;
    LONG gStage19BlockedInstallPart = 0;
    bool gCategoryUniqueNameEnabled = false;
    bool gCategoryUniqueNameHookReady = false;
    DWORD gCategoryUniquePartTextHashes[9] = {};
    LONG gCategoryUniqueNameMatches = 0;
    SetupFn gOriginalSetup = nullptr;
    SetupFn gOriginalBuyPartsSetup = nullptr;
    UnlockPerfPartFn gOriginalUnlockPerfPart = nullptr;
    NoPerfDrawFn gOriginalNoPerfDraw = nullptr;
    FEngSetLanguageHashObjFn gOriginalStage14LanguageHash = nullptr;
    FEngSetScriptObjFn gOriginalStage14Script = nullptr;
    AddOptionFn gOriginalAddOption = nullptr;
    FEngGetLastButtonFn gOriginalFEngGetLastButton = nullptr;
    FEngSetLastButtonFn gOriginalFEngSetLastButton = nullptr;
    UIWidgetMenuAddStatOptionFn gOriginalUIWidgetMenuAddStatOption = nullptr;
    UIWidgetMenuSetInitialOptionFn gOriginalUIWidgetMenuSetInitialOption = nullptr;
    BuyPerfPartWidgetDrawFn gOriginalBuyPerfPartWidgetDraw = nullptr;
    bool gStage10DrawAuditEnabled = false; // opt-in: modos 8/9 somente
    bool gStage11PassiveGate = false; // modo 10: observar a construcao nativa SEM filtrar
    bool gStage11NativeFilter = false; // modos 11-13: filtrar partes ANTES da lista, somente Extreme
    bool gStage12Inspect = false; // modos 12/13: examinar objetos do placeholder nativo
    bool gStage12Write = false; // modos 13-15: atualizar nome/status via FEngPrintf apos Setup completo
    bool gStage13Audit = false; // modo 14/15: interceptar exclusivamente NoPerfPartsWidgy::Draw
    bool gStage13PostDrawWrite = false; // modo 15 SOMENTE: reaplicar texto apos Draw nativo
    // Etapa 14: NAO faz mutacoes adicionais na interface, apenas registra chamadas de setters.
    // Etapa 15: somente leitura de memoria de objetos ja validados, sem hooks adicionais.
    // Etapa 16: nunca fabricar PerfPart, nunca copiar widget/stale pointers.
    // Modo 18 mantem o placeholder e apenas observa; modo 19 permite exatamente UMA
    // PerfPart real e desbloqueada pela rotina original, para criar um widget nativo.
    bool gStage16Trace = false;
    bool gStage16SingleRow = false;
    bool gStage17ClearStaleText = false;
    // Etapa 18: Extreme tem o MESMO tipo da opcao Stage 3, em vez de PPCustomizePackage.
    // O backing package e uma copia de leitura de uma estrutura VALIDA de Stage 3,
    // reduzida a um item; jamais e gravado na tabela original do jogo.
    constexpr int kPerfPackageDwords = 95;
    bool gStage18NativePackage = false;
    bool gStage18PackageReady = false;
    DWORD gStage18Source = 0;
    DWORD gStage18Packages[9][kPerfPackageDwords] = {};
    LONG gStage18UniqueDraws = 0;
    LONG gStage18UniqueRewrites = 0;
    LONG gStage17Cleared = 0;
    LONG gStage17Rewrites = 0;
    LONG gStage16Accepted = 0;
    LONG gStage16NativeRows = 0;
    LONG gStage16TraceRows = 0;
    DWORD gStage16ChosenPartIndex = 0;
    DWORD* gStage16Widget = nullptr;
    constexpr uintptr_t kNativePerfWidgetVtable = 0x0079B620;
    bool gStage15Snapshot = false;
    constexpr size_t kStage15SnapshotBytes = 0x60; // menor que 0x64 usado por IsTextObject
    struct Stage15TextSnapshot
    {
        DWORD* object = nullptr;
        BYTE after[kStage15SnapshotBytes] = {};
        bool valid = false;
    };
    Stage15TextSnapshot gStage15NameSnapshot = {};
    Stage15TextSnapshot gStage15StatusSnapshot = {};
    bool gStage15Captured = false;
    bool gStage14Audit = false;
    bool gStage14HooksReady = false;
    bool gStage14WriteCompleted = false;
    DWORD* gStage14Name = nullptr;
    DWORD* gStage14Status = nullptr;
    LONG gStage14LanguageName = 0;
    LONG gStage14LanguageStatus = 0;
    LONG gStage14ScriptName = 0;
    LONG gStage14ScriptStatus = 0;
    DWORD gStage14LastNameHash = 0;
    DWORD gStage14LastStatusHash = 0;
    bool gStage13AfterSetup = false; // contar se callback ocorre APOS a escrita do Setup
    LONG gStage13DrawCalls = 0;
    LONG gStage13DrawMatches = 0;
    LONG gStage13DrawAfterSetup = 0;
    LONG gStage13PostWrites = 0;
    LONG gStage10TotalDraw = 0;
    LONG gStage10DrawWhileExtreme = 0;
    DWORD gStage10NativeHiddenFlags[36] = {};
    DWORD gStage10WidgetHiddenFlags[32] = {};
    bool gNativeDrawHookTried = false;
    bool gNativeDrawHookReady = false;
    LONG gUniqueDrawCalls = 0;
    LONG gUniqueDrawSuppressed = 0;
    HMODULE gModule = nullptr;
    char gIni[MAX_PATH] = {};
    char gLog[MAX_PATH] = {};
    char gExtremeTextKey[96] = "UNIQUES_SHOP_TITLE";
    bool gEnableStageBoxes = false;
    bool gEnableExtremeBox = false;
    bool gTraceFrontendSelection = true;
    bool gEnableExtremeStatusPreview = false;
    // Diagnostico opt-in: 0 nao busca recursos; 1 apenas consulta; 2 tenta abrir a tela.
    // Usar modo 2 somente com copia do FrontB e backup do save, pois e experimental.
    int gUniqueFNGProbeMode = 16; // Etapa 14: auditoria passiva de setters apos Setup
    bool gUniqueFNGShowAttempted = false;
    bool gUniqueFNGRootShown = false;
    // Etapa 7: objetos do painel nativo ocultados somente enquanto o FNG Unique esta visivel.
    struct HiddenNativeRow { DWORD hash; DWORD* object; };
    HiddenNativeRow gHiddenNativeRows[36] = {};
    unsigned int gHiddenNativeRowCount = 0;
    bool gNativeRowsHidden = false;
    // Etapa 8: capturar objetos FEString diretamente dos widgets ativos, nao por hash de template.
    struct LiveWidgetText { DWORD* widget; DWORD* name; DWORD* status; };
    LiveWidgetText gLiveWidgets[16] = {};
    unsigned int gLiveWidgetCount = 0;
    void* gLiveWidgetMenu = nullptr;
    struct HiddenWidgetText { DWORD* widget; DWORD* text; int slot; };
    HiddenWidgetText gHiddenWidgetTexts[32] = {};
    unsigned int gHiddenWidgetTextCount = 0;
    bool gLiveWidgetTextsHidden = false;
    LONG gUniqueFNGProbeSamples = 0;
    bool gAllowScrollerTransitionForPreview = true; // Deslocamento de +1 somente durante abertura da tela.
    LONG gPreviewSetupEntries = 0;
    bool gStatusHooksAttempted = false;
    bool gStatusHooksReady = false;
    DWORD gPreviewSetupThread = 0;
    void* gPreviewSetupMenu = nullptr;
    bool gPreviewNoPerfVtableReady = false; // agora significa: hook Draw nativo instalado
    DWORD* gUniquePreviewWidget = nullptr;
    LONG gPreviewHiddenParts = 0;
    LONG gPreviewStatusRows = 0;
    LONG gPreviewSetups = 0;
    bool gFrontendTraceActive = false;
    bool gFrontendTraceHooksTried = false;
    LONG gFrontendSetCalls = 0;
    LONG gFrontendGetCalls = 0;
    LONG gFrontendAddRows = 0;
    LONG gFrontendSetupCalls = 0;
    LONG gChooseOptionOrdinal = 0;
    int gExtremeOptionOrdinal = -1;
    void* gChooseScreenForTrace = nullptr;
    DWORD gChooseScreenVtable = 0;
    int gLastObservedScrollerIndex = -999;
    DWORD gLastPerformanceButton = 0;
    LONG gTraceScrollerSamples = 0;
    bool gExtremeVisualUnlocked = false;
    bool gAutoDetectInstalledUnique = true;
    bool gExtremeUniqueInstalled = false;
    int gExtremeGarageSlot = -1;
    int gExtremeUniqueCategory = -1;
    DWORD gExtremeCarHash = 0;
    LONG gUniqueSamples = 0;
    bool gDebugLog = true;
    bool gAddOptionHookTried = false;
    bool gAddOptionHookReady = false;
    DWORD gStage3OriginalIconHash = 0; // textura ja existente na UI original; capturada da opcao Stage 3
    DWORD gExtremeTextHash = 0;
    DWORD gReturnToStockHash = 0;
    DWORD gActiveSetupThread = 0;
    void* gActiveSetupScreen = nullptr;
    LONG gHookCalls = 0;
    LONG gExtremeInjections = 0;

    void Log(const char* format, ...)
    {
        if (!gDebugLog) { return; }
        char buffer[512] = {};
        va_list args;
        va_start(args, format);
        vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
        va_end(args);
        HANDLE file = CreateFileA(gLog, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) { return; }
        DWORD written = 0;
        WriteFile(file, buffer, static_cast<DWORD>(strlen(buffer)), &written, nullptr);
        WriteFile(file, "\r\n", 2, &written, nullptr);
        CloseHandle(file);
    }

    bool MemoryAvailable(const void* address, SIZE_T size, bool executable = false)
    {
        MEMORY_BASIC_INFORMATION mbi = {};
        if (VirtualQuery(address, &mbi, sizeof(mbi)) != sizeof(mbi)) { return false; }
        if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) { return false; }
        uintptr_t begin = reinterpret_cast<uintptr_t>(address);
        uintptr_t end = begin + size;
        uintptr_t regionEnd = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        if (end < begin || end > regionEnd) { return false; }
        if (!executable) { return true; }
        DWORD protection = mbi.Protect & 0xFF;
        return protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ || protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
    }

    // Etapa 6: validacao nao intrusiva do FNG. Executa na thread do frontend,
    // a partir de UIWidgetMenu_SetInitialOption (nao em thread de inicializacao).
    // A consulta por si so nao carrega o pacote; NULL antes de ShowPackage e normal.
    DWORD* FindFNGObject(const char* package, DWORD objectHash)
    {
        if (!MemoryAvailable(reinterpret_cast<void*>(kFEngFindObject), 6, true)) { return nullptr; }
        return reinterpret_cast<FEngFindObjectFn>(kFEngFindObject)(package, objectHash);
    }

    // Etapa 7: preservar os objetos nativos, nao interceptar Render/Draw e nao alterar listas/vtables.
    // As chamadas de restauracao so ocorrem se o mesmo objeto ainda pertence ao pacote.
    // FEngSetVisible nao restaura todas as animacoes de SHOW/HIDE; por isso este teste e opt-in.
    const char* UniqueDisplayPartName(int category); // definido adiante; seis tipos suportados

    void UpdateUniqueStatusText()
    {
        if (!MemoryAvailable(reinterpret_cast<void*>(kFEngPrintf), 6, true)) { return; }
        DWORD* name = FindFNGObject(kUniqueFNG, 0xDCCDA0F9); // UG2_UNIQUE_PART_NAME
        DWORD* status = FindFNGObject(kUniqueFNG, 0x53B5167C); // UG2_UNIQUE_PART_STATUS
        if (!name || !status || !MemoryAvailable(name, sizeof(DWORD)) || !MemoryAvailable(status, sizeof(DWORD))) { return; }
        auto print = reinterpret_cast<GamePrintfFn>(kFEngPrintf);
        print(name, "%s", UniqueDisplayPartName(gExtremeUniqueCategory));
        print(status, "%s", "Installed");
        Log("UNIQUE ETAPA7: texto atualizado para %s - Installed", UniqueDisplayPartName(gExtremeUniqueCategory));
    }

    // A rotina BuyPerfPartWidget_Draw (NFSU2Unlimiter/BuyPerfPartWidget.h)
    // usa widget[11] como nome e widget[12] como status. Diferentemente dos objetos
    // obtidos por FindObject, estes ponteiros representam as linhas ATIVAS do menu.
    // Campo bruto observado como flags de objeto FEng; o significado de cada bit ainda
    // NAO esta demonstrado. Captura-se apenas para comparar estado antes/depois.
    DWORD ReadStage10Flags(DWORD* object)
    {
        if (!object || !MemoryAvailable(object, 0x20)) { return 0xFFFFFFFF; }
        return *reinterpret_cast<const DWORD*>(reinterpret_cast<const BYTE*>(object) + 0x1C);
    }

    bool IsTextObject(DWORD* object)
    {
        if (!object || !MemoryAvailable(object, 0x64)) { return false; }
        return *reinterpret_cast<DWORD*>(reinterpret_cast<BYTE*>(object) + 0x18) == 2;
    }

    DWORD* WidgetTextAt(DWORD* widget, unsigned int index)
    {
        if (!widget || !MemoryAvailable(widget, 0x50) || widget[0] != kBuyPerfPartWidgetVtable) { return nullptr; }
        DWORD* text = reinterpret_cast<DWORD*>(static_cast<uintptr_t>(widget[index]));
        return IsTextObject(text) ? text : nullptr;
    }

    void TraceActiveWidgetTexts()
    {
        if (gUniqueFNGProbeMode < 4 || !gLiveWidgetCount) { return; }
        unsigned int valid = 0, distinctNames = 0, distinctStatuses = 0;
        for (unsigned int i = 0; i < gLiveWidgetCount; ++i)
        {
            LiveWidgetText& row = gLiveWidgets[i];
            row.name = WidgetTextAt(row.widget, 11);
            row.status = WidgetTextAt(row.widget, 12);
            if (row.name && row.status) { ++valid; }
            bool newName = row.name != nullptr, newStatus = row.status != nullptr;
            for (unsigned int j = 0; j < i; ++j)
            {
                if (row.name && row.name == gLiveWidgets[j].name) { newName = false; }
                if (row.status && row.status == gLiveWidgets[j].status) { newStatus = false; }
            }
            if (newName) { ++distinctNames; }
            if (newStatus) { ++distinctStatuses; }
            const bool widgetReadable = row.widget && MemoryAvailable(row.widget, 0x50);
            const DWORD nameRaw = widgetReadable ? row.widget[11] : 0;
            const DWORD statusRaw = widgetReadable ? row.widget[12] : 0;
            Log("UNIQUE ETAPA8 WIDGET: %u/%u widget=%p type=%08lX nameRaw=%08lX statusRaw=%08lX name=%p status=%p valido=%d", i + 1, gLiveWidgetCount, row.widget, widgetReadable ? row.widget[0] : 0, nameRaw, statusRaw, row.name, row.status, row.name && row.status ? 1 : 0);
        }
        Log("UNIQUE ETAPA8 RESUMO: menu=%p widgets=%u validos=%u nomesDistintos=%u statusDistintos=%u", gLiveWidgetMenu, gLiveWidgetCount, valid, distinctNames, distinctStatuses);
    }

    void RestoreActiveWidgetTexts()
    {
        if (!gLiveWidgetTextsHidden) { return; }
        const unsigned int count = gHiddenWidgetTextCount;
        gLiveWidgetTextsHidden = false;
        gHiddenWidgetTextCount = 0;
        unsigned int restored = 0, skipped = 0;
        if (MemoryAvailable(reinterpret_cast<void*>(kFEngSetVisible), 6, true))
        {
            auto visible = reinterpret_cast<FEngVisibilityFn>(kFEngSetVisible);
            for (unsigned int i = 0; i < count; ++i)
            {
                HiddenWidgetText& h = gHiddenWidgetTexts[i];
                if (!h.widget || !MemoryAvailable(h.widget, 0x50) || h.widget[0] != kBuyPerfPartWidgetVtable ||
                    WidgetTextAt(h.widget, static_cast<unsigned int>(h.slot)) != h.text)
                {
                    ++skipped;
                    continue;
                }
                visible(h.text);
                ++restored;
            }
        }
        Log("UNIQUE ETAPA8 RESTAURAR: textosAtivos=%u/%u pulados=%u", restored, count, skipped);
    }

    void HideActiveWidgetTexts()
    {
        if ((gUniqueFNGProbeMode != 5 && gUniqueFNGProbeMode != 7 && gUniqueFNGProbeMode != 9) || !gUniqueFNGRootShown || gLiveWidgetTextsHidden || !gLiveWidgetMenu) { return; }
        if (gUniqueFNGProbeMode == 7 && !gNativeDrawHookReady) { Log("UNIQUE ETAPA9: hook Draw indisponivel; ocultacao cancelada"); return; }
        if (!MemoryAvailable(reinterpret_cast<void*>(kFEngSetInvisible), 6, true)) { return; }
        // Se o jogo nao entregou os objetos associados aos widgets, falhar sem mexer neles.
        if (gLiveWidgetCount < 3 || gLiveWidgetCount > 16)
        {
            Log("UNIQUE ETAPA8: quantidade de widgets suspeita (%u), ocultacao cancelada", gLiveWidgetCount);
            return;
        }
        unsigned int uniqueNames = 0, uniqueStatuses = 0;
        for (unsigned int i = 0; i < gLiveWidgetCount; ++i)
        {
            LiveWidgetText& row = gLiveWidgets[i];
            row.name = WidgetTextAt(row.widget, 11);
            row.status = WidgetTextAt(row.widget, 12);
            if (!row.name || !row.status)
            {
                Log("UNIQUE ETAPA8: widget %u sem textos validos; nao ocultou nada", i);
                return;
            }
            bool newName = true, newStatus = true;
            for (unsigned int j = 0; j < i; ++j)
            {
                if (row.name == gLiveWidgets[j].name) { newName = false; }
                if (row.status == gLiveWidgets[j].status) { newStatus = false; }
            }
            if (newName) { ++uniqueNames; }
            if (newStatus) { ++uniqueStatuses; }
        }
        if (uniqueNames < 3 || uniqueStatuses < 3)
        {
            Log("UNIQUE ETAPA8: textos nao parecem clones individuais (nomes=%u status=%u); ocultacao cancelada", uniqueNames, uniqueStatuses);
            return;
        }
        auto invisible = reinterpret_cast<FEngVisibilityFn>(kFEngSetInvisible);
        for (unsigned int i = 0; i < gLiveWidgetCount; ++i)
        {
            LiveWidgetText& row = gLiveWidgets[i];
            const DWORD beforeName = ReadStage10Flags(row.name);
            gHiddenWidgetTexts[gHiddenWidgetTextCount] = { row.widget, row.name, 11 };
            invisible(row.name);
            if (gUniqueFNGProbeMode == 9) { gStage10WidgetHiddenFlags[gHiddenWidgetTextCount] = ReadStage10Flags(row.name); }
            ++gHiddenWidgetTextCount;
            const DWORD beforeStatus = ReadStage10Flags(row.status);
            gHiddenWidgetTexts[gHiddenWidgetTextCount] = { row.widget, row.status, 12 };
            invisible(row.status);
            if (gUniqueFNGProbeMode == 9) { gStage10WidgetHiddenFlags[gHiddenWidgetTextCount] = ReadStage10Flags(row.status); }
            ++gHiddenWidgetTextCount;
            if (gUniqueFNGProbeMode == 9 && i < 3) { Log("ETAPA10 OCULTAR WIDGET: linha=%u nome=%08lX->%08lX status=%08lX->%08lX", i, beforeName, gStage10WidgetHiddenFlags[gHiddenWidgetTextCount - 2], beforeStatus, gStage10WidgetHiddenFlags[gHiddenWidgetTextCount - 1]); }
        }
        gLiveWidgetTextsHidden = true;
        Log("UNIQUE ETAPA8: ocultou %u textos dos %u widgets ATIVOS; nomesDistintos=%u statusDistintos=%u", gHiddenWidgetTextCount, gLiveWidgetCount, uniqueNames, uniqueStatuses);
    }

    void CaptureActiveWidget(void* menu, void* widget)
    {
        if (gUniqueFNGProbeMode < 4 || !gFrontendTraceActive || gExtremeOptionOrdinal < 0 ||
            gLastPerformanceButton != static_cast<DWORD>(gExtremeOptionOrdinal) ||
            !gExtremeUniqueInstalled || !MemoryAvailable(widget, 0x50)) { return; }
        DWORD* row = reinterpret_cast<DWORD*>(widget);
        if (row[0] != kBuyPerfPartWidgetVtable) { return; }
        if (gLiveWidgetMenu != menu)
        {
            // Caso o menu mude sem evento de saida, nao restaurar ponteiros do menu anterior.
            gLiveWidgetMenu = menu;
            gLiveWidgetCount = 0;
            gHiddenWidgetTextCount = 0;
            gLiveWidgetTextsHidden = false;
        }
        if (gLiveWidgetCount >= 16) { Log("UNIQUE ETAPA8: mais de 16 widgets; captura limitada"); return; }
        LiveWidgetText& target = gLiveWidgets[gLiveWidgetCount++];
        target = { row, WidgetTextAt(row, 11), WidgetTextAt(row, 12) };
    }

    void RestoreNativeRows()
    {
        if (!gNativeRowsHidden) { return; }
        unsigned int restored = 0, replaced = 0;
        const unsigned int savedCount = gHiddenNativeRowCount;
        // Desarmar ANTES de chamar APIs do jogo (elas podem disparar callbacks de UI).
        gHiddenNativeRowCount = 0;
        gNativeRowsHidden = false;
        if (MemoryAvailable(reinterpret_cast<void*>(kFEngSetVisible), 6, true))
        {
            auto setVisible = reinterpret_cast<FEngVisibilityFn>(kFEngSetVisible);
            for (unsigned int i = 0; i < savedCount; ++i)
            {
                DWORD* current = FindFNGObject(kNativePartsFNG, gHiddenNativeRows[i].hash);
                if (current && current == gHiddenNativeRows[i].object && MemoryAvailable(current, sizeof(DWORD)))
                {
                    setVisible(current);
                    ++restored;
                }
                else { ++replaced; }
            }
        }
        Log("UNIQUE ETAPA7: restauracao nativa %u/36 objetos; ausentes/trocados=%u", restored, replaced);
    }

    void HideNativeRows()
    {
        if ((gUniqueFNGProbeMode != 3 && gUniqueFNGProbeMode != 5 && gUniqueFNGProbeMode != 7 && gUniqueFNGProbeMode != 9) || gNativeRowsHidden || !gUniqueFNGRootShown) { return; }
        if (gUniqueFNGProbeMode == 7 && !gNativeDrawHookReady) { return; }
        if (!MemoryAvailable(reinterpret_cast<void*>(kFEngSetInvisible), 6, true) ||
            !MemoryAvailable(reinterpret_cast<void*>(kFEngSetVisible), 6, true))
        {
            Log("UNIQUE ETAPA7: funcoes de visibilidade indisponiveis, mantendo lista nativa");
            return;
        }
        // Prevalidar os 36 objetos. Se o frontend nao estiver completo, nao ocultar parcialmente.
        DWORD* resolved[36] = {};
        for (unsigned int i = 0; i < 36; ++i)
        {
            resolved[i] = FindFNGObject(kNativePartsFNG, kNativeRowHashes[i]);
            if (!resolved[i] || !MemoryAvailable(resolved[i], sizeof(DWORD)))
            {
                Log("UNIQUE ETAPA7: objeto nativo ausente idx=%u hash=%08lX; ocultacao cancelada", i, kNativeRowHashes[i]);
                return;
            }
        }
        auto setInvisible = reinterpret_cast<FEngVisibilityFn>(kFEngSetInvisible);
        for (unsigned int i = 0; i < 36; ++i)
        {
            const DWORD before = ReadStage10Flags(resolved[i]);
            gHiddenNativeRows[i] = { kNativeRowHashes[i], resolved[i] };
            setInvisible(resolved[i]);
            if (gUniqueFNGProbeMode == 9)
            {
                gStage10NativeHiddenFlags[i] = ReadStage10Flags(resolved[i]);
                if (i < 6) { Log("ETAPA10 OCULTAR NATIVO: idx=%u hash=%08lX flags=%08lX->%08lX", i, kNativeRowHashes[i], before, gStage10NativeHiddenFlags[i]); }
            }
        }
        gHiddenNativeRowCount = 36;
        gNativeRowsHidden = true;
        Log("UNIQUE ETAPA7: ocultou 12 grupos e 24 textos nativos; painel de Performance Stats preservado");
    }

    void TraceStage10BeforeRestore()
    {
        if (gUniqueFNGProbeMode < 8 || !gUniqueFNGRootShown) { return; }
        unsigned int nativeChanged = 0, nativeChecked = 0, widgetChanged = 0, widgetChecked = 0;
        if (gUniqueFNGProbeMode == 9)
        {
            for (unsigned int i = 0; i < gHiddenNativeRowCount; ++i)
            {
                DWORD* current = FindFNGObject(kNativePartsFNG, gHiddenNativeRows[i].hash);
                if (!current || current != gHiddenNativeRows[i].object) { continue; }
                const DWORD now = ReadStage10Flags(current);
                if (now == 0xFFFFFFFF) { continue; }
                ++nativeChecked;
                if (now != gStage10NativeHiddenFlags[i]) { ++nativeChanged; if (nativeChanged <= 6) { Log("ETAPA10 MUDOU NATIVO: idx=%u hash=%08lX flags=%08lX->%08lX", i, gHiddenNativeRows[i].hash, gStage10NativeHiddenFlags[i], now); } }
            }
            for (unsigned int i = 0; i < gHiddenWidgetTextCount; ++i)
            {
                const HiddenWidgetText& entry = gHiddenWidgetTexts[i];
                if (!entry.widget || WidgetTextAt(entry.widget, static_cast<unsigned int>(entry.slot)) != entry.text) { continue; }
                const DWORD now = ReadStage10Flags(entry.text);
                if (now == 0xFFFFFFFF) { continue; }
                ++widgetChecked;
                if (now != gStage10WidgetHiddenFlags[i]) { ++widgetChanged; if (widgetChanged <= 6) { Log("ETAPA10 MUDOU WIDGET: idx=%u flags=%08lX->%08lX", i, gStage10WidgetHiddenFlags[i], now); } }
            }
        }
        Log("ETAPA10.1 RESUMO: drawAudit=%d drawTotal=%ld drawExtreme=%ld flagsNativosMudaram=%u/%u flagsWidgetsMudaram=%u/%u mode=%d visibleHook=0", gStage10DrawAuditEnabled ? 1 : 0, gStage10TotalDraw, gStage10DrawWhileExtreme, nativeChanged, nativeChecked, widgetChanged, widgetChecked, gUniqueFNGProbeMode);
    }

    void ProbeHideUniqueFNG()
    {
        TraceStage10BeforeRestore();
        if (gUniqueFNGProbeMode >= 6 && gUniqueFNGRootShown)
        {
            Log("UNIQUE ETAPA9 DRAW RESUMO: chamadasUnique=%ld bloqueadas=%ld modo=%d", gUniqueDrawCalls, gUniqueDrawSuppressed, gUniqueFNGProbeMode);
        }
        RestoreActiveWidgetTexts();
        RestoreNativeRows();
        gLiveWidgetCount = 0;
        gLiveWidgetMenu = nullptr;
        if (!gUniqueFNGRootShown) { return; }
        gUniqueFNGRootShown = false;
        DWORD* root = FindFNGObject(kUniqueFNG, kUniqueRootHash);
        if (root && MemoryAvailable(root, sizeof(DWORD)) && MemoryAvailable(reinterpret_cast<void*>(kFEngSetInvisible), 6, true))
        {
            reinterpret_cast<FEngVisibilityFn>(kFEngSetInvisible)(root);
            Log("UNIQUE FNG PROBE: ocultou root ao sair da caixa Unique (sem mexer em Stock)");
        }
        else { Log("UNIQUE FNG PROBE: root nao encontrado durante saida; nenhum ponteiro salvo"); }
    }

    void ProbeMaybeShowUniqueFNG()
    {
        if (!gUniqueFNGProbeMode || gUniqueFNGProbeMode >= 10 || !gFrontendTraceActive || gExtremeOptionOrdinal < 0 ||
            gLastPerformanceButton != static_cast<DWORD>(gExtremeOptionOrdinal) ||
            !gExtremeUniqueInstalled || gExtremeGarageSlot < 0) { return; }
        // Executar depois que a lista original inicializou, sem hooks de compra.
        const LONG count = InterlockedIncrement(&gUniqueFNGProbeSamples);
        if (count > 8) { return; }
        DWORD* nativeAnchor = FindFNGObject(kNativePartsFNG, kNativePartsAnchorHash);
        if (!nativeAnchor || !MemoryAvailable(nativeAnchor, sizeof(DWORD)))
        {
            Log("UNIQUE FNG PROBE: tela nativa ainda nao carregada; teste adiado");
            return;
        }
        DWORD* root = FindFNGObject(kUniqueFNG, kUniqueRootHash);
        Log("UNIQUE FNG PROBE: mode=%d nativeAnchor=%p uniqueRootAntes=%p attempt=%d", gUniqueFNGProbeMode, nativeAnchor, root, gUniqueFNGShowAttempted ? 1 : 0);
        if (gUniqueFNGProbeMode < 2) { return; }
        if (!root && !gUniqueFNGShowAttempted)
        {
            gUniqueFNGShowAttempted = true;
            if (!MemoryAvailable(reinterpret_cast<void*>(kFEngShowPackage), 6, true))
            {
                Log("UNIQUE FNG PROBE: FEngShowPackage indisponivel; nao abriu");
                return;
            }
            Log("UNIQUE FNG PROBE: TENTATIVA de abrir %s; nenhuma alteracao no save", kUniqueFNG);
            reinterpret_cast<FEngShowPackageFn>(kFEngShowPackage)(kUniqueFNG, kCarouselFNG, 0xFF, 0);
            root = FindFNGObject(kUniqueFNG, kUniqueRootHash);
            Log("UNIQUE FNG PROBE: uniqueRootDepois=%p", root);
        }
        if (root && MemoryAvailable(root, sizeof(DWORD)) && MemoryAvailable(reinterpret_cast<void*>(kFEngSetVisible), 6, true))
        {
            reinterpret_cast<FEngVisibilityFn>(kFEngSetVisible)(root);
            gUniqueFNGRootShown = true;
            Log("UNIQUE FNG PROBE: root visivel; este e apenas o teste de abertura, NAO a interface final");
            UpdateUniqueStatusText();
            HideNativeRows();
            HideActiveWidgetTexts();
        }
        else { Log("UNIQUE FNG PROBE: pacote ainda nao foi localizado; sem manipulacao de lista"); }
    }

    bool CheckExeVersion()
    {
        char path[MAX_PATH] = {};
        if (!GetModuleFileNameA(nullptr, path, MAX_PATH)) { return false; }
        const char* name = strrchr(path, '\\');
        name = name ? name + 1 : path;
        if (_stricmp(name, "SPEED2.EXE") != 0) { return false; }
        HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) { return false; }
        LARGE_INTEGER size = {};
        bool ok = GetFileSizeEx(file, &size) != 0;
        CloseHandle(file);
        if (ok) { Log("SPEED2.EXE: %lld bytes", size.QuadPart); }
        return ok && static_cast<uint64_t>(size.QuadPart) == kExpectedExeSize;
    }

    struct RestoreValue
    {
        volatile DWORD* address;
        DWORD original;
        ~RestoreValue() { if (address) { *address = original; } }
    };

    struct ScopedInjection
    {
        ScopedInjection(void* screen, bool enabled)
        {
            if (enabled) { gActiveSetupScreen = screen; gActiveSetupThread = GetCurrentThreadId(); }
        }
        ~ScopedInjection() { gActiveSetupThread = 0; gActiveSetupScreen = nullptr; }
    };

    // Indices 0..8 do jogo: Weight, Transmission, NOS, Engine, Turbo, Suspension,
    // Brakes, ECU, Tires. Categoria 9 (aerodynamics) NAO possui flag Unique.
    const char* UniqueCategoryName(int category)
    {
        static const char* names[] = { "Weight Reduction", "Transmission", "NOS", "Engine", "Turbo", "Suspension", "Brakes", "ECU", "Tires" };
        return category >= 0 && category < 9 ? names[category] : "unsupported";
    }

    // Os nomes sao resolvidos pela MESMA chave real usada no Stage 3.
    // A Etapa 18.1 usou hashes privados sem recursos de localizacao, o que
    // causava fallback visual para "Performance Parts" em vez da categoria.
    const char* CategoryUniqueLabel(int category)
    {
        static const char* names[9] = { "Weight Reduction Unique", "Transmission Unique", "NOS Unique", "Engine Unique", "Turbo Unique", "Suspension Unique", "Brakes Unique", "ECU Unique", "Tires Unique" };
        return category >= 0 && category < 9 ? names[category] : "Unique";
    }

    __declspec(noinline) const char* __cdecl HookGetLocalizedString(DWORD hash)
    {
        // UNIQUES_SHOP_TITLE e uma chave de localizacao vanilla do UG2; o texto pode ser resolvido
        // e armazenado em cache ANTES do latch de selecao do scroller.
        if (gCategoryUniqueNameEnabled && gStage18NativePackage && gStage18PackageReady &&
            gExtremeUniqueInstalled &&
            gExtremeUniqueCategory >= 0 && gExtremeUniqueCategory < 9 &&
            hash == gExtremeTextHash)
        {
            InterlockedIncrement(&gCategoryUniqueNameMatches);
            return CategoryUniqueLabel(gExtremeUniqueCategory);
        }
        return gOriginalGetLocalizedString ? gOriginalGetLocalizedString(hash) : nullptr;
    }

    // Segundo caminho de texto: o frontend tambem pode configurar diretamente
    // uma string pelo hash, sem chamar GetLocalizedString. Mantemos ambos.
    __declspec(noinline) void __cdecl HookStage19SetLanguageHash(DWORD* object, unsigned int hash)
    {
        if (gCategoryUniqueNameEnabled && gStage18NativePackage && gStage18PackageReady &&
            gExtremeUniqueInstalled && gExtremeUniqueCategory >= 0 && gExtremeUniqueCategory < 9 &&
            hash == gExtremeTextHash && IsTextObject(object) &&
            MemoryAvailable(reinterpret_cast<void*>(kFEngPrintf), 6, true))
        {
            static thread_local bool inside = false;
            if (!inside)
            {
                inside = true;
                reinterpret_cast<GamePrintfFn>(kFEngPrintf)(object, "%s", CategoryUniqueLabel(gExtremeUniqueCategory));
                inside = false;
                InterlockedIncrement(&gStage19SetLanguageMatches);
                return;
            }
        }
        if (gStage19OriginalSetLanguageHash) { gStage19OriginalSetLanguageHash(object, hash); }
    }

    void InstallStage19NameHashHook()
    {
        if (!gCategoryUniqueNameEnabled || !gStage18NativePackage || gStage19SetLanguageHookReady) { return; }
        void* target = reinterpret_cast<void*>(kFEngSetLanguageHashObj);
        if (!MemoryAvailable(target, 6, true)) { Log("ETAPA19: SetLanguageHash nao acessivel; manter traducao normal"); return; }
        MH_STATUS status = MH_CreateHook(target, reinterpret_cast<LPVOID>(&HookStage19SetLanguageHash), reinterpret_cast<LPVOID*>(&gStage19OriginalSetLanguageHash));
        if (status != MH_OK) { Log("ETAPA19: falha CreateHook nome por hash=%d", static_cast<int>(status)); return; }
        status = MH_EnableHook(target);
        if (status != MH_OK)
        {
            MH_RemoveHook(target);
            gStage19OriginalSetLanguageHash = nullptr;
            Log("ETAPA19: falha EnableHook nome por hash=%d", static_cast<int>(status));
            return;
        }
        gStage19SetLanguageHookReady = true;
        Log("ETAPA19: hook SetLanguageHash ativo como segunda via para textos Engine Unique/ECU Unique");
    }

    // Nenhuma chamada que referencia a copia privada pode chegar ao CCM do jogo.
    // O bloqueio e baseado no endereco da referencia, NAO no texto e NAO no
    // valor atual do botao; funciona mesmo que a UI dispare o evento atrasado.
    bool IsStage19PrivateReference(const void* reference)
    {
        if (!gStage18NativePackage || !reference) { return false; }
        const uintptr_t address = reinterpret_cast<uintptr_t>(reference);
        const uintptr_t begin = reinterpret_cast<uintptr_t>(&gStage18Packages[0][0]);
        const uintptr_t end = begin + sizeof(gStage18Packages);
        return address >= begin && address < end;
    }

    bool ShouldStage19BlockMutation(const void* reference)
    {
        if (IsStage19PrivateReference(reference)) { return true; }
        // Defesa adicional: em alguns fluxos a UI pode encaminhar uma referencia
        // derivada/copied package, mas o pacote escolhido na tela ainda e o privado.
        // Somente com a Extreme selecionada; nunca afeta Stage 1/2/3/Stock.
        if (!gStage18NativePackage || !gStage18PackageReady || !gExtremeUniqueInstalled ||
            gExtremeOptionOrdinal < 0 ||
            gLastPerformanceButton != static_cast<DWORD>(gExtremeOptionOrdinal) ||
            !MemoryAvailable(reinterpret_cast<void*>(kSelectedPerfPackage), sizeof(DWORD))) { return false; }
        const DWORD selected = *reinterpret_cast<volatile const DWORD*>(kSelectedPerfPackage);
        return IsStage19PrivateReference(reinterpret_cast<void*>(static_cast<uintptr_t>(selected)));
    }

    void Stage19Blocked(const char* eventName, LONG count, const void* reference)
    {
        if (count <= 12) { Log("ETAPA19 BLOQUEADO: %s ref=%p count=%ld (nenhuma escrita pelo CCM)", eventName, reference, count); }
    }

    __declspec(noinline) void __fastcall HookBuyPerfPackage(void* manager, void* /*edx*/, const BYTE* package)
    {
        if (ShouldStage19BlockMutation(package))
        {
            Stage19Blocked("BuyPerfPackage", InterlockedIncrement(&gStage19BlockedBuyPackage), package);
            return;
        }
        if (gOriginalBuyPerfPackage) { gOriginalBuyPerfPackage(manager, package); }
    }

    __declspec(noinline) void __fastcall HookInstallPerfPackage(void* manager, void* /*edx*/, const BYTE* package, void* ride)
    {
        if (ShouldStage19BlockMutation(package))
        {
            Stage19Blocked("InstallPerfPackage", InterlockedIncrement(&gStage19BlockedInstallPackage), package);
            return;
        }
        if (gOriginalInstallPerfPackage) { gOriginalInstallPerfPackage(manager, package, ride); }
    }

    __declspec(noinline) void __fastcall HookBuyPerfPart(void* manager, void* /*edx*/, const BYTE* part)
    {
        if (ShouldStage19BlockMutation(part))
        {
            Stage19Blocked("BuyPerfPart", InterlockedIncrement(&gStage19BlockedBuyPart), part);
            return;
        }
        if (gOriginalBuyPerfPart) { gOriginalBuyPerfPart(manager, part); }
    }

    __declspec(noinline) void __fastcall HookInstallPerfPart(void* manager, void* /*edx*/, const BYTE* part, void* ride)
    {
        if (ShouldStage19BlockMutation(part))
        {
            Stage19Blocked("InstallPerfPart", InterlockedIncrement(&gStage19BlockedInstallPart), part);
            return;
        }
        if (gOriginalInstallPerfPart) { gOriginalInstallPerfPart(manager, part, ride); }
    }

    bool InstallStage19MutationGuards()
    {
        if (!gStage18NativePackage) { return true; }
        if (gStage19MutationGuardsReady) { return true; }
        struct GuardTarget { uintptr_t address; LPVOID hook; LPVOID* original; const char* name; };
        GuardTarget guards[] = {
            { kCCMBuyPerfPackage, reinterpret_cast<LPVOID>(&HookBuyPerfPackage), reinterpret_cast<LPVOID*>(&gOriginalBuyPerfPackage), "BuyPerfPackage" },
            { kCCMInstallPerfPackage, reinterpret_cast<LPVOID>(&HookInstallPerfPackage), reinterpret_cast<LPVOID*>(&gOriginalInstallPerfPackage), "InstallPerfPackage" },
            { kCCMBuyPerfPart, reinterpret_cast<LPVOID>(&HookBuyPerfPart), reinterpret_cast<LPVOID*>(&gOriginalBuyPerfPart), "BuyPerfPart" },
            { kCCMInstallPerfPart, reinterpret_cast<LPVOID>(&HookInstallPerfPart), reinterpret_cast<LPVOID*>(&gOriginalInstallPerfPart), "InstallPerfPart" }
        };
        unsigned int created = 0, enabled = 0;
        for (unsigned int i = 0; i < 4; ++i)
        {
            void* target = reinterpret_cast<void*>(guards[i].address);
            if (!MemoryAvailable(target, 6, true)) { Log("ETAPA19 PROTECAO FALHOU: funcao %s nao acessivel", guards[i].name); break; }
            MH_STATUS result = MH_CreateHook(target, guards[i].hook, guards[i].original);
            if (result != MH_OK) { Log("ETAPA19 PROTECAO FALHOU: hook %s error=%d", guards[i].name, static_cast<int>(result)); break; }
            ++created;
        }
        if (created == 4)
        {
            for (unsigned int i = 0; i < 4; ++i)
            {
                MH_STATUS result = MH_EnableHook(reinterpret_cast<void*>(guards[i].address));
                if (result != MH_OK) { Log("ETAPA19 PROTECAO FALHOU: enable %s error=%d", guards[i].name, static_cast<int>(result)); break; }
                ++enabled;
            }
        }
        if (created != 4 || enabled != 4)
        {
            for (unsigned int i = 0; i < enabled; ++i) { MH_DisableHook(reinterpret_cast<void*>(guards[i].address)); }
            for (unsigned int i = 0; i < created; ++i) { MH_RemoveHook(reinterpret_cast<void*>(guards[i].address)); }
            gOriginalBuyPerfPackage = nullptr;
            gOriginalInstallPerfPackage = nullptr;
            gOriginalBuyPerfPart = nullptr;
            gOriginalInstallPerfPart = nullptr;
            // FAIL CLOSED: nenhuma opcao Extreme e montada sem todos os guards.
            gStage19MutationGuardsReady = false;
            Log("ETAPA19: seguranca incompleta; Extreme DESABILITADA (Stages originais preservados)");
            return false;
        }
        gStage19MutationGuardsReady = true;
        Log("ETAPA19: 4/4 hooks de compra/instalacao ativos; bloqueio por endereco privado, sem alterar o save");
        return true;
    }

    void InstallCategoryUniqueNameHook()
    {
        if (!gCategoryUniqueNameEnabled || !gStage18NativePackage || gCategoryUniqueNameHookReady) { return; }
        void* target = reinterpret_cast<void*>(kGetLocalizedString);
        if (!MemoryAvailable(target, 6, true)) { Log("ETAPA18.1: GetLocalizedString indisponivel, texto padrao preservado"); return; }
        MH_STATUS status = MH_CreateHook(target, reinterpret_cast<LPVOID>(&HookGetLocalizedString), reinterpret_cast<LPVOID*>(&gOriginalGetLocalizedString));
        if (status != MH_OK) { Log("ETAPA18.1: hook de localizacao nao instalado (%d), texto padrao preservado", static_cast<int>(status)); return; }
        status = MH_EnableHook(target);
        if (status != MH_OK)
        {
            MH_RemoveHook(target);
            gOriginalGetLocalizedString = nullptr;
            Log("ETAPA18.1: hook de localizacao nao ativado (%d), texto padrao preservado", static_cast<int>(status));
            return;
        }
        gCategoryUniqueNameHookReady = true;
        Log("ETAPA19.1: hash original UNIQUES_SHOP_TITLE resolvido por categoria somente na Extreme.");
    }

    void ReadUniqueStateForCurrentCar()
    {
        gExtremeUniqueInstalled = false;
        gExtremeGarageSlot = -1;
        gExtremeUniqueCategory = -1;
        gExtremeCarHash = 0;
        if (!gAutoDetectInstalledUnique) { return; }
        if (!MemoryAvailable(reinterpret_cast<void*>(kCurrentPackageType), sizeof(int))) { Log("Unique: categoria do menu inacessivel"); return; }
        const int category = *reinterpret_cast<volatile int*>(kCurrentPackageType);
        gExtremeUniqueCategory = category;
        if (category < 0 || category >= 9) { Log("Unique: categoria %d nao mapeada (sem flag nativa)", category); return; }
        if (!MemoryAvailable(reinterpret_cast<void*>(kFECarConfigRef), sizeof(void*))) { Log("Unique: FECarConfigRef inacessivel"); return; }
        auto* carConfig = *reinterpret_cast<void**>(kFECarConfigRef);
        if (!carConfig || !MemoryAvailable(carConfig, sizeof(void*))) { Log("Unique: FECarConfig nao inicializado"); return; }
        auto** vtable = *reinterpret_cast<void***>(carConfig);
        if (!vtable || !MemoryAvailable(vtable, 2 * sizeof(void*))) { Log("Unique: vtable FECarConfig invalida"); return; }
        void* fn = vtable[1];
        if (!MemoryAvailable(fn, 6, true) || !MemoryAvailable(reinterpret_cast<void*>(kGetCarTypeInfo), 6, true)) { Log("Unique: funcoes de identificacao do carro invalidas"); return; }
        const int carTypeId = reinterpret_cast<FECarTypeFn>(fn)(carConfig);
        if (carTypeId < 0 || carTypeId > 255) { Log("Unique: CarTypeId fora do intervalo plausivel: %d", carTypeId); return; }
        const BYTE* typeInfo = reinterpret_cast<GetCarTypeInfoFn>(kGetCarTypeInfo)(carTypeId);
        if (!typeInfo || !MemoryAvailable(typeInfo, 0xD4)) { Log("Unique: CarTypeInfo inacessivel, id=%d", carTypeId); return; }
        const DWORD carHash = *reinterpret_cast<const DWORD*>(typeInfo + 0xD0);
        if (!carHash) { Log("Unique: identidade do carro zerada, id=%d", carTypeId); return; }
        gExtremeCarHash = carHash;
        int matchingSlots = 0;
        int plausibleSlots = 0;
        bool installed = false;
        int slot = -1;
        const LONG sample = InterlockedIncrement(&gUniqueSamples);
        if (sample == 1) { Log("Unique: profileBase=%p collectionBase=%p garageBase=%p stride=%lX", reinterpret_cast<void*>(kPlayerProfileBase), reinterpret_cast<void*>(kPlayerCollectionBase), reinterpret_cast<void*>(kCareerGarageBase), static_cast<unsigned long>(kCareerCarStride)); }
        for (int i = 0; i < 5; ++i)
        {
            uintptr_t address = kCareerGarageBase + static_cast<uintptr_t>(i) * kCareerCarStride;
            auto* rec = reinterpret_cast<const BYTE*>(address);
            if (!MemoryAvailable(rec, kCareerCarStride)) { if (sample <= 25) { Log("Unique: registro da garagem %d inacessivel", i + 1); } continue; }
            const DWORD mask = *reinterpret_cast<const DWORD*>(rec + kCareerCarTypeMaskOffset);
            const DWORD hash = *reinterpret_cast<const DWORD*>(rec + kCareerCarHashOffset);
            const BYTE locked = rec[kCareerCarLockedOffset];
            const BYTE purchased = rec[kCareerCarPurchasedOffset];
            const DWORD flag = *reinterpret_cast<const DWORD*>(rec + kCareerUniqueFlagsOffset + static_cast<SIZE_T>(category) * sizeof(DWORD));
            if (sample <= 10) { Log("Unique: garage=%d mask=%08lX carHash=%08lX purchased=%u locked=%u %s=%lu", i + 1, mask, hash, purchased, locked, UniqueCategoryName(category), flag); }
            if (mask == 4 && hash != 0) { ++plausibleSlots; }
            if (mask != 0 && mask != 4 && sample <= 3) { Log("Unique: aviso garage=%d tipo inesperado %08lX; nao usar o valor para desbloquear", i + 1, mask); }
            // Evita consultar o carro errado quando existem duas copias do mesmo modelo.
            if (mask == 4 && hash == carHash && purchased != 0 && locked == 0)
            {
                ++matchingSlots;
                installed = flag == 1;
                slot = i;
                if (flag > 1) { Log("Unique: flag nao canonica (%lu) no garage %d", flag, i + 1); installed = false; }
            }
        }
        if (plausibleSlots == 0 && sample <= 25) { Log("Unique: ALERTA nenhum registro de carreira plausivel; verificar base e layout. Mantendo Extreme bloqueada."); }
        if (matchingSlots == 1)
        {
            gExtremeGarageSlot = slot;
            gExtremeUniqueInstalled = installed;
            if (sample <= 25) { Log("Unique: carro selecionado id=%d hash=%08lX, garage=%d, categoria=%s, instalado=%d", carTypeId, carHash, slot + 1, UniqueCategoryName(category), installed ? 1 : 0); }
        }
        else
        {
            if (sample <= 25) { Log("Unique: carro id=%d hash=%08lX com %d registro(s) correspondente(s); falha segura. Pode haver duplicatas ou layout diferente", carTypeId, carHash, matchingSlots); }
        }
    }

    // Removeu-se completamente a vtable experimental da caixa.
    // A opcao usa exclusivamente a PPCustomizePackage nativa.

    // Etapa 18: Stage 3 utiliza PPPackage + package[19]. O original
    // UI_BuyPerformanceParts e responsavel pela apresentacao (sem Setup extra
    // de PPCustomizePackage). Copia independente de 380 bytes para NAO alterar
    // tabela de pacotes, save ou componentes do Stage 3.
    bool PrepareStage18Package(DWORD* originalPackage)
    {
        gStage18PackageReady = false;
        if (!gStage18NativePackage || !gStage19MutationGuardsReady || !gNativeDrawHookReady || !gStage3OriginalIconHash || !originalPackage || gExtremeUniqueCategory < 0 ||
            gExtremeUniqueCategory >= 9 || !MemoryAvailable(originalPackage, sizeof(DWORD) * kPerfPackageDwords))
        {
            if (gStage18NativePackage && !gNativeDrawHookReady) { Log("ETAPA18 BLOQUEADO: Draw nativo nao interceptado; nao montar Extreme com nome/status convencionais"); }
            return false;
        }
        const DWORD count = originalPackage[2];
        if (count < 1 || count > 4 || originalPackage[0] != static_cast<DWORD>(gExtremeUniqueCategory))
        {
            Log("ETAPA18 BLOQUEADO: dados Stage3 incoerentes category=%d packageType=%lu parts=%lu", gExtremeUniqueCategory, originalPackage[0], count);
            return false;
        }
        DWORD* dest = gStage18Packages[gExtremeUniqueCategory];
        memcpy(dest, originalPackage, sizeof(DWORD) * kPerfPackageDwords);
        dest[2] = 1; // uma Unique representativa (somente a primeira linha)
        // Usa um hash REAL de localizacao vanilla (UNIQUES_SHOP_TITLE), nao uma chave do FrontB modificado.
        // HookGetLocalizedString resolve esse hash por categoria quando a Extreme esta selecionada.
        dest[4] = gExtremeTextHash;
        // Somente nome da LINHA. O nome da caixa permanece UNIQUE_NATIVE.
        dest[5] = 0; // nenhum preco exibido nessa representacao
        gStage18Source = static_cast<DWORD>(reinterpret_cast<uintptr_t>(originalPackage));
        gStage18PackageReady = true;
        Log("ETAPA18 PACOTE PRONTO: category=%d fonte=%p copia=%p tipo=%lu nivel=%lu partes=%lu->1 index=%lu", gExtremeUniqueCategory,
            originalPackage, dest, dest[0], dest[1], count, dest[3]);
        return true;
    }

    DWORD* AllocateExtremeOption()
    {
        // Alocador do jogo: liberar objetos de UI cabe ao proprio menu.
        if (!MemoryAvailable(reinterpret_cast<void*>(kGameMalloc), 6, true)) { return nullptr; }
        auto gameMalloc = reinterpret_cast<GameMallocFn>(kGameMalloc);
        auto* option = reinterpret_cast<DWORD*>(gameMalloc(kOptionSize));
        if (!option) { return nullptr; }
        memset(option, 0, kOptionSize);
        option[0] = static_cast<DWORD>(gStage18NativePackage ? kPackageOptionVtable : kIndividualOptionVtable);
        // A vtable do PPCustomizePackage deve permanecer NATIVA; proxy removido por seguranca.
        // Sem GENERIC_EXTREME/DDS customizado: reutiliza o icone recebido da opcao Stage 3.
        // Esse hash pertence ao jogo/FrontB original e nao e calculado a partir de asset do mod.
        option[3] = gStage18NativePackage ? gStage3OriginalIconHash : 0;
        option[11] = gExtremeTextHash;
        option[13] = 0x3F800000; // 1.0f
        option[14] = 0x3F800000;
        option[15] = 0x3F800000;
        option[16] = 0x3F800000;
        option[18] = gStage18NativePackage ? 1 : 0; // PPPackage na etapa 18; PPCustomizePackage nos legados.
        if (gStage18NativePackage) { option[19] = static_cast<DWORD>(reinterpret_cast<uintptr_t>(gStage18Packages[gExtremeUniqueCategory])); }
        auto* bytes = reinterpret_cast<BYTE*>(option);
        bytes[33] = 1;
        // 0 = desbloqueado visualmente, 1 = bloqueado. Nao altera flags de save.
        // IMPORTANTE: com AutoDetectInstalledUnique=1, ExtremeVisualUnlocked NAO e usado.
        // Para testar apenas o aspecto visual: AutoDetectInstalledUnique=0 e ExtremeVisualUnlocked=1.
        const bool visualUnlocked = gAutoDetectInstalledUnique ? gExtremeUniqueInstalled : gExtremeVisualUnlocked;
        bytes[34] = visualUnlocked ? 0 : 1;
        bytes[68] = 1;
        bytes[69] = 1;
        return option;
    }

    void __fastcall HookAddOption(void* menu, void* /*edx*/, DWORD* option)
    {
        if (!gOriginalAddOption) { return; }
        // Insere ANTES de Return to Stock, durante o Setup correto e na mesma thread.
        if (gActiveSetupThread == GetCurrentThreadId() && gActiveSetupScreen == menu && option && MemoryAvailable(option, kOptionSize))
        {
            const int originalOrdinal = static_cast<int>(gChooseOptionOrdinal);
            if (gTraceFrontendSelection && originalOrdinal <= 8) { Log("FE TRACE OPTION: ordinal=%d option=%p type=%lu textHash=%08lX", originalOrdinal, option, option[18], option[11]); }
            if (gStage18NativePackage && originalOrdinal == 3 && option[18] == 1 && option[19])
            {
                gStage3OriginalIconHash = option[3];
                if (!gStage3OriginalIconHash) { Log("FRONTB ORIGINAL: icon Stage 3 ausente, Extreme nao sera criada"); }
                else { Log("FRONTB ORIGINAL: icon Stage 3 capturado=%08lX; nenhuma textura customizada necessaria", gStage3OriginalIconHash); }
                PrepareStage18Package(reinterpret_cast<DWORD*>(static_cast<uintptr_t>(option[19])));
            }
            if (option[18] == 2 && option[11] == gReturnToStockHash)
            {
                DWORD* extreme = (gStage18NativePackage && gStage18PackageReady && gStage3OriginalIconHash) ? AllocateExtremeOption() : nullptr;
                if (gStage18NativePackage && !gStage18PackageReady) { Log("ETAPA18: Extreme nao inserida; nenhuma referencia PPPackage valida para clonar"); }
                if (extreme)
                {
                    gExtremeOptionOrdinal = originalOrdinal;
                    if (gTraceFrontendSelection) { Log("FE TRACE OPTION: Extreme ordinal zero-based=%d, ReturnToStock ordinal=%d (nao presumir que button ID = ordinal)", gExtremeOptionOrdinal, originalOrdinal + 1); }
                    gOriginalAddOption(menu, extreme);
                    LONG count = InterlockedIncrement(&gExtremeInjections);
                    if (count <= 20)
                    {
                        Log("Extreme inserida (tipo=%lu, data=%08lX) antes de Return to Stock", extreme[18], extreme[19]);
                        Log("Extreme sem FrontB modificado: icone=Stage3-original hash=%08lX, text=%s hash=%08lX", gStage3OriginalIconHash, gExtremeTextKey, gExtremeTextHash);
                        Log("Extreme UI: AutoDetect=%d VisualFallback=%d UniqueInstalled=%d MatchedGarage=%d carHash=%08lX LockedByte=%u (0=aberto; 1=bloqueado)", gAutoDetectInstalledUnique ? 1 : 0, gExtremeVisualUnlocked ? 1 : 0, gExtremeUniqueInstalled ? 1 : 0, gExtremeGarageSlot >= 0 ? gExtremeGarageSlot + 1 : 0, gExtremeCarHash, reinterpret_cast<BYTE*>(extreme)[34]);
                    }
                }
                else { Log("Falha ao alocar a opcao Extreme"); }
            }
        }
        gOriginalAddOption(menu, option);
        if (gActiveSetupThread == GetCurrentThreadId() && gActiveSetupScreen == menu) { ++gChooseOptionOrdinal; }
    }

    // Correlacao do indice real do carrossel. O objeto IconScroller fica em screen + 19 DWORDs
    // (veja ChoosePerformancePackages::Setup no NFSU2Unlimiter). NUNCA alteramos o indice.
    // Um erro de correspondencia de tela desativa a consulta, sem tentar interpretar memoria livre.
    int ReadCurrentPackageNodeIndex()
    {
        if (!gChooseScreenForTrace || !MemoryAvailable(gChooseScreenForTrace, 20 * sizeof(DWORD))) { return -999; }
        const DWORD nowVtable = *reinterpret_cast<const DWORD*>(gChooseScreenForTrace);
        if (!gChooseScreenVtable || nowVtable != gChooseScreenVtable) { return -999; }
        if (!MemoryAvailable(reinterpret_cast<void*>(kThumbnailScrollerGetCurrentNodeIndex), 6, true)) { return -999; }
        auto* scroller = reinterpret_cast<DWORD*>(gChooseScreenForTrace) + 19;
        if (!MemoryAvailable(scroller, 2 * sizeof(DWORD))) { return -999; }
        // A propria rotina do jogo calcula o indice atual; nao acessamos offsets desconhecidos.
        const int index = reinterpret_cast<ThumbnailScrollerGetCurrentNodeIndexFn>(kThumbnailScrollerGetCurrentNodeIndex)(scroller);
        return index >= -1 && index <= 64 ? index : -999;
    }

    bool IsPerformancePackageName(const char* pkg)
    {
        if (!pkg || !MemoryAvailable(pkg, 37)) { return false; }
        return strcmp(pkg, "UI_ChoosePerformancePackage.fng") == 0;
    }

    // ===== v0.3.6: escopo EXCLUSIVO da Extreme; nenhuma escrita no save =====
    // Os offsets e os enderecos correspondem ao executavel US 1.2 validado em Start().
    // Se qualquer validacao falhar, o BuyPerformanceParts::Setup original e preservado.
    // O indice do scroller pode avancar para Extreme+1 durante a transicao
    // de ChoosePerformancePackages -> BuyPerformanceParts. A decisao e baseada
    // na selecao previamente confirmada por FEngSetLastButton (button E index = Extreme),
    // e SOMENTE aceita o indice de transicao se esse latch continuar intacto.
    // Nenhum gate altera o save ou as pecas convencionais dos Stages.
    enum PreviewGateFailure : unsigned int
    {
        GateDisabled = 1u << 0, GateHooks = 1u << 1, GateFrontend = 1u << 2,
        GateUnique = 1u << 3, GateOrdinal = 1u << 4, GateButton = 1u << 5,
        GateObservedNode = 1u << 6, GateLiveNode = 1u << 7,
        GateCategory = 1u << 8, GateDatabase = 1u << 9
    };

    unsigned int EvaluateExtremePreviewGate(int& liveNode, int& memoryCategory, DWORD& database)
    {
        unsigned int failed = 0;
        liveNode = ReadCurrentPackageNodeIndex();
        memoryCategory = -1;
        database = 0xFFFFFFFF;
        if (!gEnableExtremeStatusPreview && !gStage11PassiveGate && !gStage11NativeFilter) { failed |= GateDisabled; }
        if (!gStatusHooksReady) { failed |= GateHooks; }
        if (!gFrontendTraceActive) { failed |= GateFrontend; }
        if (!gExtremeUniqueInstalled || gExtremeGarageSlot < 0) { failed |= GateUnique; }
        if (gExtremeOptionOrdinal < 0) { failed |= GateOrdinal; }
        if (gExtremeOptionOrdinal < 0 || gLastPerformanceButton != static_cast<DWORD>(gExtremeOptionOrdinal)) { failed |= GateButton; }
        if (gExtremeOptionOrdinal < 0 || gLastObservedScrollerIndex != gExtremeOptionOrdinal) { failed |= GateObservedNode; }
        const bool exactNode = gExtremeOptionOrdinal >= 0 && liveNode == gExtremeOptionOrdinal;
        const bool transientNode = gAllowScrollerTransitionForPreview && gExtremeOptionOrdinal >= 0 && liveNode == gExtremeOptionOrdinal + 1;
        if (!exactNode && !transientNode) { failed |= GateLiveNode; }
        if (MemoryAvailable(reinterpret_cast<void*>(kCurrentPackageType), sizeof(int))) { memoryCategory = *reinterpret_cast<volatile int*>(kCurrentPackageType); }
        if (gExtremeUniqueCategory < 0 || gExtremeUniqueCategory >= 9 || memoryCategory != gExtremeUniqueCategory) { failed |= GateCategory; }
        if (MemoryAvailable(reinterpret_cast<void*>(kFEDatabase), sizeof(DWORD))) { database = *reinterpret_cast<volatile DWORD*>(kFEDatabase); }
        if (database != 1) { failed |= GateDatabase; }
        return failed;
    }

    // O widget 'NoPerfPartsWidgy' original nao referencia pecas convencionais.
    // Utilizamos sua instancia nativa, substituindo apenas Draw por uma vtable PRIVADA.
    const char* UniqueDisplayPartName(int category)
    {
        switch (category)
        {
        case 1: return "Extreme Transmission Upgrade";
        case 3: return "Extreme Engine Upgrade";
        case 4: return "Extreme Turbo Upgrade";
        case 5: return "Extreme Suspension Upgrade";
        case 6: return "Extreme Brakes Upgrade";
        case 7: return "Extreme ECU Upgrade";
        default: return "Unique Performance Upgrade";
        }
    }

    void Stage17ClearStaleTexts();
    bool Stage17WriteNativeRow(DWORD* row);

    // Etapa 16: preservar o fluxo nativo do Stage 3, sem PerfPart sintetica.
    // No modo 19, a propria rotina Setup do jogo chama FEStatWidget ctor,
    // cria BuyPerfPartWidget (80 bytes) e vincula uma PerfPart autentica da lista.
    // Apenas reescrevemos texto depois do Setup, com validacao estrita da classe.
    // Nao habilitar compras: ainda nao existe hook do handler de Install Package.
    void Stage16FinishNativeRow(void* menu)
    {
        if (!gStage16SingleRow || !menu || menu != gPreviewSetupMenu ||
            gPreviewSetupThread != GetCurrentThreadId()) { return; }
        DWORD* row = gStage16Widget;
        const bool structureOK = gStage16Accepted == 1 && gStage16NativeRows == 1 &&
            gPreviewStatusRows == 0 && row && MemoryAvailable(row, 0x50) &&
            row[0] == kNativePerfWidgetVtable && row[19] != 0;
        if (!structureOK)
        {
            Log("ETAPA16 FALHA_FECHADA: allowed=%ld native=%ld placeholder=%ld row=%p", gStage16Accepted, gStage16NativeRows, gPreviewStatusRows, row);
            return;
        }
        DWORD* name = WidgetTextAt(row, 11);
        DWORD* status = WidgetTextAt(row, 12);
        const bool textOK = name && status && name != status && MemoryAvailable(reinterpret_cast<void*>(kFEngPrintf), 6, true);
        if (!textOK)
        {
            Log("ETAPA16 FALHA_FECHADA: objetos de texto invalidos name=%p status=%p", name, status);
            return;
        }
        auto gamePrintf = reinterpret_cast<GamePrintfFn>(kFEngPrintf);
        const int n = gamePrintf(name, "%s", UniqueDisplayPartName(gExtremeUniqueCategory));
        const int st = gamePrintf(status, "%s", "Installed");
        if (gStage17ClearStaleText) { Stage17ClearStaleTexts(); }
        Log("ETAPA16 STAGE3_MIRROR: widget=%p partRef=%08lX partIndex=%08lX name=%p status=%p printf=%d/%d previewSomente=1", row, row[19], gStage16ChosenPartIndex, name, status, n, st);
        Log("ETAPA17 AVISO: preview nativo. A compra/instalacao NAO esta bloqueada; nao clicar em Install Package.");
    }

    // Etapa 17: atualizar somente os dois textos da linha nativa ativa.
    // O objeto PerfPart permanece autentico; jamais gravamos nele ou no save.
    bool Stage17WriteNativeRow(DWORD* row)
    {
        if (!row || !gStage16SingleRow || gStage16Widget != row || !gExtremeUniqueInstalled ||
            !MemoryAvailable(row, 0x50) || row[0] != kNativePerfWidgetVtable || row[19] == 0 ||
            !MemoryAvailable(reinterpret_cast<void*>(kFEngPrintf), 6, true)) { return false; }
        DWORD* name = WidgetTextAt(row, 11);
        DWORD* status = WidgetTextAt(row, 12);
        if (!name || !status || name == status) { return false; }
        auto print = reinterpret_cast<GamePrintfFn>(kFEngPrintf);
        print(name, "%s", UniqueDisplayPartName(gExtremeUniqueCategory));
        print(status, "%s", "Installed");
        return true;
    }

    // Os textos antigos pertencem ao template UI_BuyPerformanceParts.fng.
    // Se somente uma linha for criada, as 11 anteriores podem permanecer com
    // os seus valores no pacote FNG. Limpa TEXTO dos templates, nao o widget,
    // sem esconder grupos, mexer em scripts ou alterar dados de performance.
    void Stage17ClearStaleTexts()
    {
        if (!gStage17ClearStaleText || !gStage16SingleRow || !gStage16Widget ||
            !MemoryAvailable(reinterpret_cast<void*>(kFEngPrintf), 6, true)) { return; }
        DWORD* liveName = WidgetTextAt(gStage16Widget, 11);
        DWORD* liveStatus = WidgetTextAt(gStage16Widget, 12);
        if (!liveName || !liveStatus) { return; }
        auto print = reinterpret_cast<GamePrintfFn>(kFEngPrintf);
        unsigned int cleaned = 0, missing = 0;
        for (unsigned int i = 12; i < 36; ++i)
        {
            DWORD* item = FindFNGObject(kNativePartsFNG, kNativeRowHashes[i]);
            if (!IsTextObject(item)) { ++missing; continue; }
            if (item == liveName || item == liveStatus) { continue; }
            print(item, "%s", "");
            ++cleaned;
        }
        InterlockedExchange(&gStage17Cleared, static_cast<LONG>(cleaned));
        Log("ETAPA17 LIMPEZA: cleared=%u missing=%u preservedName=%p preservedStatus=%p", cleaned, missing, liveName, liveStatus);
    }

    // Sem substituir ou copiar vtables: intercepta o Draw original do widget de lista vazia.
    // Ponteiro da linha vale SOMENTE durante a exibicao do pacote Unique; e invalidado
    // ANTES de toda troca de caixa, particularmente antes de Return to Stock.
    // Compara o cabecalho do objeto de texto no mesmo ciclo de interface.
    // Nao tenta ler buffers internos desconhecidos nem desreferenciar ponteiros do FNG.
    unsigned int Stage15DiffBytes(const BYTE* a, const BYTE* b, size_t size)
    {
        unsigned int count = 0;
        for (size_t i = 0; i < size; ++i) { if (a[i] != b[i]) { ++count; } }
        return count;
    }

    int Stage15FirstDiff(const BYTE* a, const BYTE* b, size_t size)
    {
        for (size_t i = 0; i < size; ++i) { if (a[i] != b[i]) { return static_cast<int>(i); } }
        return -1;
    }

    DWORD Stage15Hash(const BYTE* data, size_t size)
    {
        DWORD hash = 2166136261u;
        for (size_t i = 0; i < size; ++i) { hash = (hash ^ data[i]) * 16777619u; }
        return hash;
    }

    void Stage15Reset()
    {
        gStage15Captured = false;
        gStage15NameSnapshot = {};
        gStage15StatusSnapshot = {};
    }

    void Stage15Record(DWORD* name, DWORD* status, const BYTE* beforeName, const BYTE* beforeStatus, int printNameResult, int printStatusResult)
    {
        Stage15Reset();
        if (!gStage15Snapshot || !IsTextObject(name) || !IsTextObject(status) || name == status ||
            !MemoryAvailable(name, kStage15SnapshotBytes) || !MemoryAvailable(status, kStage15SnapshotBytes))
        {
            Log("ETAPA15 APOS_ESCRITA: captura abortada (objetos nao sao texto valido)"); return;
        }
        gStage15NameSnapshot.object = name;
        gStage15StatusSnapshot.object = status;
        memcpy(gStage15NameSnapshot.after, name, kStage15SnapshotBytes);
        memcpy(gStage15StatusSnapshot.after, status, kStage15SnapshotBytes);
        gStage15NameSnapshot.valid = true;
        gStage15StatusSnapshot.valid = true;
        gStage15Captured = true;
        Log("ETAPA15 APOS_ESCRITA: captured=1 name=%p status=%p returnNome=%d returnStatus=%d writeDeltaName=%u writeDeltaStatus=%u hashName=%08lX hashStatus=%08lX",
            name, status, printNameResult, printStatusResult,
            Stage15DiffBytes(beforeName, gStage15NameSnapshot.after, kStage15SnapshotBytes),
            Stage15DiffBytes(beforeStatus, gStage15StatusSnapshot.after, kStage15SnapshotBytes),
            Stage15Hash(gStage15NameSnapshot.after, kStage15SnapshotBytes),
            Stage15Hash(gStage15StatusSnapshot.after, kStage15SnapshotBytes));
    }

    void Stage15RecordExit()
    {
        if (!gStage15Snapshot) { return; }
        if (!gStage15Captured || !gStage15NameSnapshot.valid || !gStage15StatusSnapshot.valid)
        {
            Log("ETAPA15 AO_SAIR: captured=0"); Stage15Reset(); return;
        }
        DWORD* name = gStage15NameSnapshot.object;
        DWORD* status = gStage15StatusSnapshot.object;
        const bool nameOK = IsTextObject(name) && MemoryAvailable(name, kStage15SnapshotBytes);
        const bool statusOK = IsTextObject(status) && MemoryAvailable(status, kStage15SnapshotBytes);
        if (!nameOK || !statusOK)
        {
            Log("ETAPA15 AO_SAIR: captured=1 nameValid=%d statusValid=%d (nao comparar objetos invalidados)", nameOK ? 1 : 0, statusOK ? 1 : 0);
            Stage15Reset(); return;
        }
        BYTE nowName[kStage15SnapshotBytes] = {};
        BYTE nowStatus[kStage15SnapshotBytes] = {};
        memcpy(nowName, name, kStage15SnapshotBytes);
        memcpy(nowStatus, status, kStage15SnapshotBytes);
        Log("ETAPA15 AO_SAIR: captured=1 deltaName=%u firstName=%d deltaStatus=%u firstStatus=%d hashName=%08lX hashStatus=%08lX",
            Stage15DiffBytes(gStage15NameSnapshot.after, nowName, kStage15SnapshotBytes), Stage15FirstDiff(gStage15NameSnapshot.after, nowName, kStage15SnapshotBytes),
            Stage15DiffBytes(gStage15StatusSnapshot.after, nowStatus, kStage15SnapshotBytes), Stage15FirstDiff(gStage15StatusSnapshot.after, nowStatus, kStage15SnapshotBytes),
            Stage15Hash(nowName, kStage15SnapshotBytes), Stage15Hash(nowStatus, kStage15SnapshotBytes));
        Stage15Reset();
    }

    // Traces passivos: executam encaminhamento integral sem escrita/consulta de FNG e sem log no callback.
    void __cdecl HookStage14LanguageHash(DWORD* object, unsigned int hash)
    {
        if (gStage14Audit && gStage14WriteCompleted && object)
        {
            if (object == gStage14Name) { InterlockedIncrement(&gStage14LanguageName); gStage14LastNameHash = hash; }
            else if (object == gStage14Status) { InterlockedIncrement(&gStage14LanguageStatus); gStage14LastStatusHash = hash; }
        }
        if (gOriginalStage14LanguageHash) { gOriginalStage14LanguageHash(object, hash); }
    }

    void __cdecl HookStage14Script(DWORD* object, const char* scriptName, bool enabled)
    {
        if (gStage14Audit && gStage14WriteCompleted && object)
        {
            if (object == gStage14Name) { InterlockedIncrement(&gStage14ScriptName); }
            else if (object == gStage14Status) { InterlockedIncrement(&gStage14ScriptStatus); }
        }
        if (gOriginalStage14Script) { gOriginalStage14Script(object, scriptName, enabled); }
    }

    bool PrepareStage14MutationTrace()
    {
        if (gStage14HooksReady) { return true; }
        void* lang = reinterpret_cast<void*>(kFEngSetLanguageHashObj);
        void* script = reinterpret_cast<void*>(kFEngSetScriptObj);
        if (!MemoryAvailable(lang, 6, true) || !MemoryAvailable(script, 6, true)) { Log("ETAPA14: funções de frontend inacessíveis"); return false; }
        MH_STATUS a = MH_CreateHook(lang, reinterpret_cast<void*>(&HookStage14LanguageHash), reinterpret_cast<void**>(&gOriginalStage14LanguageHash));
        if (a != MH_OK) { Log("ETAPA14: CreateHook LanguageHash falhou=%d", static_cast<int>(a)); return false; }
        MH_STATUS b = MH_CreateHook(script, reinterpret_cast<void*>(&HookStage14Script), reinterpret_cast<void**>(&gOriginalStage14Script));
        if (b != MH_OK) { MH_RemoveHook(lang); gOriginalStage14LanguageHash = nullptr; Log("ETAPA14: CreateHook Script falhou=%d", static_cast<int>(b)); return false; }
        a = MH_EnableHook(lang);
        if (a != MH_OK)
        {
            MH_RemoveHook(script); MH_RemoveHook(lang);
            gOriginalStage14LanguageHash = nullptr; gOriginalStage14Script = nullptr;
            Log("ETAPA14: EnableHook LanguageHash falhou=%d", static_cast<int>(a)); return false;
        }
        b = MH_EnableHook(script);
        if (b != MH_OK)
        {
            MH_DisableHook(lang); MH_RemoveHook(script); MH_RemoveHook(lang);
            gOriginalStage14LanguageHash = nullptr; gOriginalStage14Script = nullptr;
            Log("ETAPA14: EnableHook Script falhou=%d", static_cast<int>(b)); return false;
        }
        gStage14HooksReady = true;
        Log("ETAPA14: auditoria passiva de LanguageHashObj + ScriptObj instalada (nenhum hook Draw/Visible)");
        return true;
    }

    void ResetStage14Trace()
    {
        gStage14WriteCompleted = false;
        gStage14Name = nullptr; gStage14Status = nullptr;
        InterlockedExchange(&gStage14LanguageName, 0); InterlockedExchange(&gStage14LanguageStatus, 0);
        InterlockedExchange(&gStage14ScriptName, 0); InterlockedExchange(&gStage14ScriptStatus, 0);
        gStage14LastNameHash = 0; gStage14LastStatusHash = 0;
    }

    void LogStage14Summary(const char* where)
    {
        if (!gStage14Audit) { return; }
        Log("ETAPA14 %s: gravacao=%d langNome=%ld langStatus=%ld scriptNome=%ld scriptStatus=%ld hashNome=%08lX hashStatus=%08lX", where, gStage14WriteCompleted ? 1 : 0, gStage14LanguageName, gStage14LanguageStatus, gStage14ScriptName, gStage14ScriptStatus, gStage14LastNameHash, gStage14LastStatusHash);
    }

    int __fastcall HookNativeNoPerfDraw(DWORD* widget, void* /*edx*/)
    {
        // Chama o codigo original PRIMEIRO. Nenhum log, malloc ou consulta a FNG dentro
        // do callback; no modo 14 os unicos efeitos sao contadores atomicos.
        const int result = gOriginalNoPerfDraw ? gOriginalNoPerfDraw(widget) : 0;
        if (gStage13Audit)
        {
            InterlockedIncrement(&gStage13DrawCalls);
            // A identidade do widget e preservada somente ate a proxima troca de caixa.
            if (!widget || widget != gUniquePreviewWidget || !gFrontendTraceActive ||
                gExtremeOptionOrdinal < 0 || gLastPerformanceButton != static_cast<DWORD>(gExtremeOptionOrdinal) ||
                !gExtremeUniqueInstalled) { return result; }
            InterlockedIncrement(&gStage13DrawMatches);
            if (gStage13AfterSetup) { InterlockedIncrement(&gStage13DrawAfterSetup); }
            if (!gStage13PostDrawWrite) { return result; } // modo 14: SOMENTE LEITURA
            // O modo 15 permanece desabilitado ate validar o 14 no jogo.
            if (!MemoryAvailable(widget, 0x4C) || widget[0] != kNoPerfPartsWidgetVtable ||
                !MemoryAvailable(reinterpret_cast<void*>(kFEngPrintf), 6, true)) { return result; }
            DWORD* name = reinterpret_cast<DWORD*>(static_cast<uintptr_t>(widget[11]));
            DWORD* status = reinterpret_cast<DWORD*>(static_cast<uintptr_t>(widget[12]));
            if (!IsTextObject(name) || !IsTextObject(status) || name == status) { return result; }
            auto print = reinterpret_cast<GamePrintfFn>(kFEngPrintf);
            print(name, "%s", UniqueDisplayPartName(gExtremeUniqueCategory));
            print(status, "%s", "Installed");
            InterlockedIncrement(&gStage13PostWrites);
            return result;
        }
        // Comportamento legado (modos <= 9), preservado para compatibilidade.
        if (!widget || widget != gUniquePreviewWidget || !gFrontendTraceActive ||
            gExtremeOptionOrdinal < 0 || gLastPerformanceButton != static_cast<DWORD>(gExtremeOptionOrdinal) ||
            !gExtremeUniqueInstalled || !MemoryAvailable(widget, 0x4C) ||
            !MemoryAvailable(reinterpret_cast<void*>(kFEngPrintf), 6, true)) { return result; }
        DWORD* left = reinterpret_cast<DWORD*>(widget[11]);
        DWORD* right = reinterpret_cast<DWORD*>(widget[12]);
        auto gamePrintf = reinterpret_cast<GamePrintfFn>(kFEngPrintf);
        if (left && MemoryAvailable(left, sizeof(DWORD))) { gamePrintf(left, "%s", UniqueDisplayPartName(gExtremeUniqueCategory)); }
        if (right && MemoryAvailable(right, sizeof(DWORD))) { gamePrintf(right, "%s", "Installed"); }
        return result;
    }

    // Etapa 12: depois do Setup ORIGINAL, o jogo ja criou a unica linha vazia.
    // Nao usar o Draw como ponto de modificacao: callbacks de Draw podem ser chamados
    // durante a construcao de Stage 1/2/3 e foram associados a travamentos anteriores.
    // Os ponteiros sao volateis: conferir identidade, classe e escopo EM CADA Setup.
    void ApplyStage12NativePlaceholderText(void* menu)
    {
        if (!gStage12Inspect || gPreviewSetupThread != GetCurrentThreadId() || menu != gPreviewSetupMenu) { return; }
        if (!gStage11NativeFilter || !gExtremeUniqueInstalled || !gFrontendTraceActive ||
            gExtremeOptionOrdinal < 0 || gLastPerformanceButton != static_cast<DWORD>(gExtremeOptionOrdinal) ||
            gPreviewStatusRows != 1 || gPreviewHiddenParts < 1)
        {
            Log("ETAPA12: FAIL-CLOSED escopo invalido, rows=%ld denied=%ld button=%lu ordinal=%d", gPreviewStatusRows, gPreviewHiddenParts, gLastPerformanceButton, gExtremeOptionOrdinal);
            return;
        }
        DWORD* widget = gUniquePreviewWidget;
        if (!widget || !MemoryAvailable(widget, 0x4C) || widget[0] != kNoPerfPartsWidgetVtable)
        {
            Log("ETAPA12: FAIL-CLOSED placeholder invalido widget=%p", widget);
            return;
        }
        DWORD* name = reinterpret_cast<DWORD*>(static_cast<uintptr_t>(widget[11]));
        DWORD* status = reinterpret_cast<DWORD*>(static_cast<uintptr_t>(widget[12]));
        const bool nameOk = IsTextObject(name);
        const bool statusOk = IsTextObject(status);
        const DWORD nameType = name && MemoryAvailable(name, 0x1C) ? name[6] : 0xFFFFFFFF;
        const DWORD statusType = status && MemoryAvailable(status, 0x1C) ? status[6] : 0xFFFFFFFF;
        Log("ETAPA12 PLACEHOLDER: widget=%p vtable=%08lX name=%p type=%08lX valid=%d status=%p type=%08lX valid=%d mode=%d", widget, widget[0], name, nameType, nameOk ? 1 : 0, status, statusType, statusOk ? 1 : 0, gUniqueFNGProbeMode);
        if (!gStage12Write) { Log("ETAPA12: INSPECAO sem escrever strings (modo 12)"); return; }
        // Nao fazer escrita parcial nem chamar FEngPrintf sobre objeto de tipo incerto.
        if (!nameOk || !statusOk || name == status || !MemoryAvailable(reinterpret_cast<void*>(kFEngPrintf), 6, true))
        {
            Log("ETAPA12: FAIL-CLOSED FEngPrintf cancelado, ponteiros/tipos invalidos");
            return;
        }
        BYTE beforeName[kStage15SnapshotBytes] = {};
        BYTE beforeStatus[kStage15SnapshotBytes] = {};
        if (gStage15Snapshot)
        {
            memcpy(beforeName, name, kStage15SnapshotBytes);
            memcpy(beforeStatus, status, kStage15SnapshotBytes);
        }
        auto print = reinterpret_cast<GamePrintfFn>(kFEngPrintf);
        const int printNameResult = print(name, "%s", UniqueDisplayPartName(gExtremeUniqueCategory));
        const int printStatusResult = print(status, "%s", "Installed");
        if (gStage15Snapshot) { Stage15Record(name, status, beforeName, beforeStatus, printNameResult, printStatusResult); }
        if (gStage14Audit)
        {
            gStage14Name = name; gStage14Status = status;
            gStage14WriteCompleted = true;
            Log("ETAPA14: escrita concluida; monitorando name=%p status=%p ate sair da Extreme", name, status);
        }
        Log("ETAPA12: TEXTO ESCRITO no placeholder nativo: %s - Installed", UniqueDisplayPartName(gExtremeUniqueCategory));
    }

    bool PrepareUniqueStatusVtable()
    {
        if (gPreviewNoPerfVtableReady) { return true; }
        auto* native = reinterpret_cast<const DWORD*>(kNoPerfPartsWidgetVtable);
        if (!MemoryAvailable(native, 4 * sizeof(DWORD))) { Log("UNIQUE DRAW: vtable nativa inacessivel"); return false; }
        void* original = reinterpret_cast<void*>(native[3]);
        if (!MemoryAvailable(original, 6, true)) { Log("UNIQUE DRAW: funcao Draw nativa invalida"); return false; }
        MH_STATUS result = MH_CreateHook(original, reinterpret_cast<void*>(&HookNativeNoPerfDraw), reinterpret_cast<void**>(&gOriginalNoPerfDraw));
        if (result != MH_OK) { Log("UNIQUE DRAW: CreateHook falhou (%d)", static_cast<int>(result)); return false; }
        result = MH_EnableHook(original);
        if (result != MH_OK) { MH_RemoveHook(original); gOriginalNoPerfDraw = nullptr; Log("UNIQUE DRAW: EnableHook falhou (%d)", static_cast<int>(result)); return false; }
        gPreviewNoPerfVtableReady = true;
        Log("UNIQUE DRAW: hook nativo ativo %p, sem alterar vtables", original);
        return true;
    }

    // Aplica-se apenas durante a execucao original de BuyPerformanceParts::Setup.
    // Essa propria rotina cria uma linha NoPerfPartsWidgy se nenhum item passa
    // pelo filtro (conforme BuyPerformanceParts.h do NFSU2Unlimiter).
    bool __cdecl HookUnlockPerfPart(int filter, int part, int car)
    {
        if (gPreviewSetupThread == GetCurrentThreadId() && gPreviewSetupMenu)
        {
            // Reaproveita o mesmo teste de desbloqueio do Stage 3 e libera apenas
            // UMA PerfPart real. Sem alocacao customizada, sem alteracao no save.
            if (gStage16SingleRow && gStage16Accepted == 0 && gOriginalUnlockPerfPart && gOriginalUnlockPerfPart(filter, part, car))
            {
                gStage16Accepted = 1;
                gStage16ChosenPartIndex = static_cast<DWORD>(part);
                Log("ETAPA16 PERMITIR_UMA: part=%08lX nativeUnlocked=1", gStage16ChosenPartIndex);
                return true;
            }
            const LONG hidden = InterlockedIncrement(&gPreviewHiddenParts);
            if (hidden <= 15) { Log("%s: part unlock negado SOMENTE na lista Extreme (#%ld, part=%08X)", gStage11NativeFilter ? "ETAPA11" : "UNIQUE PREVIEW", hidden, static_cast<unsigned int>(part)); }
            return false;
        }
        return gOriginalUnlockPerfPart ? gOriginalUnlockPerfPart(filter, part, car) : false;
    }

    struct ScopedUniquePreview
    {
        bool active;
        ScopedUniquePreview(void* menu, bool enabled) : active(enabled)
        {
            if (active) { gPreviewSetupMenu = menu; gPreviewSetupThread = GetCurrentThreadId(); }
        }
        ~ScopedUniquePreview() { if (active) { gPreviewSetupThread = 0; gPreviewSetupMenu = nullptr; } }
    };

    void __fastcall HookBuyPerformancePartsSetup(void* menu, void* /*edx*/)
    {
        if (!gOriginalBuyPartsSetup) { return; }
        int liveNode = -999;
        int memoryCategory = -1;
        DWORD database = 0xFFFFFFFF;
        const unsigned int gateFailed = EvaluateExtremePreviewGate(liveNode, memoryCategory, database);
        const bool preview = gateFailed == 0 && (gEnableExtremeStatusPreview || gStage11NativeFilter);
        const LONG entry = InterlockedIncrement(&gPreviewSetupEntries);
        if (entry <= 30)
        {
            // O bitmask indica qual condicao falhou: 1=INI,2=hooks,4=frontend,
            // 8=Unique,16=ordinal,32=botao,64=indice capturado,128=indice ao vivo,
            // 256=categoria,512=FEDatabase. mask=000 significa preview ativa.
            Log("%s GATE: Setup=%ld menu=%p fail=%03X preview=%d button=%lu cachedNode=%d liveNode=%d extremeOrdinal=%d unique=%d garage=%d catTracked=%d catMemory=%d db=%08lX transition=%d", (gStage11PassiveGate || gStage11NativeFilter) ? "ETAPA11" : "UNIQUE", entry, menu, gateFailed, preview ? 1 : 0, gLastPerformanceButton, gLastObservedScrollerIndex, liveNode, gExtremeOptionOrdinal, gExtremeUniqueInstalled ? 1 : 0, gExtremeGarageSlot + 1, gExtremeUniqueCategory, memoryCategory, database, gAllowScrollerTransitionForPreview ? 1 : 0);
        }
        if (gStage11PassiveGate && gateFailed == 0) { Log("ETAPA11: gate ACEITO no modo 10; nenhuma peça sera filtrada"); }
        if (gStage11NativeFilter && gateFailed != 0) { Log("ETAPA11: gate NEGADO no modo 11; Setup original preservado"); }
        if (preview)
        {
            const LONG count = InterlockedIncrement(&gPreviewSetups);
            if (count <= 24) { Log("UNIQUE PREVIEW: Setup #%ld Extreme confirmado, categoria=%s garage=%d (somente informacao)", count, UniqueCategoryName(gExtremeUniqueCategory), gExtremeGarageSlot + 1); }
            InterlockedExchange(&gPreviewHiddenParts, 0);
            InterlockedExchange(&gPreviewStatusRows, 0);
        }
        gUniquePreviewWidget = nullptr; // Descarta a referencia do menu ANTERIOR antes de reconstruir.
        gStage16Widget = nullptr;
        gStage16Accepted = 0;
        gStage16NativeRows = 0;
        gStage16ChosenPartIndex = 0;
        gStage13AfterSetup = false;
        if (gStage14Audit) { ResetStage14Trace(); }
        if (gStage15Snapshot) { Stage15Reset(); }
        ScopedUniquePreview scope(menu, preview);
        gOriginalBuyPartsSetup(menu);
        // Apos SetInitialOption + RefreshHeader, ainda no escopo do Setup da Extreme.
        // Nenhum ponteiro de widget sobrevive a uma nova chamada Setup ou a troca de caixa.
        if (preview && gStage12Inspect) { ApplyStage12NativePlaceholderText(menu); }
        if (preview && gStage16SingleRow) { Stage16FinishNativeRow(menu); }
        if (preview && gStage13Audit)
        {
            gStage13AfterSetup = true;
            Log("ETAPA13 APOS SETUP: drawTotal=%ld matched=%ld afterSetup=%ld postWrites=%ld modo=%d", gStage13DrawCalls, gStage13DrawMatches, gStage13DrawAfterSetup, gStage13PostWrites, gUniqueFNGProbeMode);
        }
        if (preview) { Log("%s: Setup concluido; partes negadas=%ld, placeholders nativos=%ld", gStage11NativeFilter ? "ETAPA11" : "UNIQUE PREVIEW", gPreviewHiddenParts, gPreviewStatusRows); }
        if (preview && gStage16Trace) { Log("ETAPA16 RESUMO: modo=%d reais=%ld permitido=%ld part=%08lX placeholders=%ld negadas=%ld", gUniqueFNGProbeMode, gStage16NativeRows, gStage16Accepted, gStage16ChosenPartIndex, gPreviewStatusRows, gPreviewHiddenParts); }
        if (preview && gStage14Audit) { LogStage14Summary("APOS_SETUP"); }
        else if (gStage11PassiveGate && entry <= 30) { Log("ETAPA11: Setup original preservado; modo de observacao"); }
    }

    bool InstallUniqueStatusHooks()
    {
        if (gStatusHooksAttempted) { return gStatusHooksReady; }
        gStatusHooksAttempted = true;
        if (!gEnableExtremeStatusPreview) { return false; }
        if (!PrepareUniqueStatusVtable()) { return false; }
        auto* entry = reinterpret_cast<void* const*>(kBuyPerformancePartsSetupVtableEntry);
        if (!MemoryAvailable(entry, sizeof(void*))) { Log("UNIQUE PREVIEW: entrada BuyPerformanceParts::Setup inacessivel"); return false; }
        void* target = *entry;
        if (!MemoryAvailable(target, 6, true) || !MemoryAvailable(reinterpret_cast<void*>(kUnlockSystemIsPerfPartUnlocked), 6, true)) { Log("UNIQUE PREVIEW: funcao de Setup/Unlock invalida"); return false; }
        MH_STATUS a = MH_CreateHook(target, reinterpret_cast<void*>(&HookBuyPerformancePartsSetup), reinterpret_cast<void**>(&gOriginalBuyPartsSetup));
        if (a != MH_OK) { Log("UNIQUE PREVIEW: CreateHook Setup falhou (%d)", static_cast<int>(a)); return false; }
        MH_STATUS b = MH_CreateHook(reinterpret_cast<void*>(kUnlockSystemIsPerfPartUnlocked), reinterpret_cast<void*>(&HookUnlockPerfPart), reinterpret_cast<void**>(&gOriginalUnlockPerfPart));
        if (b != MH_OK) { Log("UNIQUE PREVIEW: CreateHook Unlock falhou (%d)", static_cast<int>(b)); MH_RemoveHook(target); gOriginalBuyPartsSetup = nullptr; return false; }
        a = MH_EnableHook(reinterpret_cast<void*>(kUnlockSystemIsPerfPartUnlocked));
        if (a == MH_OK) { b = MH_EnableHook(target); }
        if (a != MH_OK || b != MH_OK)
        {
            MH_DisableHook(target);
            MH_DisableHook(reinterpret_cast<void*>(kUnlockSystemIsPerfPartUnlocked));
            MH_RemoveHook(target);
            MH_RemoveHook(reinterpret_cast<void*>(kUnlockSystemIsPerfPartUnlocked));
            gOriginalBuyPartsSetup = nullptr;
            gOriginalUnlockPerfPart = nullptr;
            Log("UNIQUE PREVIEW: EnableHook falhou (%d/%d); recurso desativado", static_cast<int>(a), static_cast<int>(b));
            return false;
        }
        gStatusHooksReady = true;
        Log("UNIQUE PREVIEW: hooks Setup=%p e Unlock=%08lX instalados, gate=button+cachedNode+liveNode(Extreme ou Extreme+1)+Unique+categoria+FEDatabase", target, static_cast<unsigned long>(kUnlockSystemIsPerfPartUnlocked));
        return true;
    }


    // Etapa 11: hook Setup no modo 10; somente no modo 11 o filtro de unlock e instalado.
    // Nao usa FNG auxiliar e nao modifica o conteudo/vtable dos widgets.
    bool InstallStage11NativeHooks()
    {
        if (gStatusHooksAttempted) { return gStatusHooksReady; }
        gStatusHooksAttempted = true;
        if (!gStage11PassiveGate && !gStage11NativeFilter) { return false; }
        auto* entry = reinterpret_cast<void* const*>(kBuyPerformancePartsSetupVtableEntry);
        if (!MemoryAvailable(entry, sizeof(void*))) { Log("ETAPA11: vtable Setup indisponivel"); return false; }
        void* target = *entry;
        if (!target || !MemoryAvailable(target, 6, true)) { Log("ETAPA11: endereco Setup invalido"); return false; }
        if (gStage11NativeFilter && !MemoryAvailable(reinterpret_cast<void*>(kUnlockSystemIsPerfPartUnlocked), 6, true))
        {
            Log("ETAPA11: UnlockSystem indisponivel; nenhum hook aplicado");
            return false;
        }
        MH_STATUS setup = MH_CreateHook(target, reinterpret_cast<void*>(&HookBuyPerformancePartsSetup), reinterpret_cast<void**>(&gOriginalBuyPartsSetup));
        if (setup != MH_OK) { Log("ETAPA11: erro ao criar hook Setup=%d", static_cast<int>(setup)); return false; }
        if (gStage11NativeFilter)
        {
            MH_STATUS unlock = MH_CreateHook(reinterpret_cast<void*>(kUnlockSystemIsPerfPartUnlocked), reinterpret_cast<void*>(&HookUnlockPerfPart), reinterpret_cast<void**>(&gOriginalUnlockPerfPart));
            if (unlock != MH_OK)
            {
                MH_RemoveHook(target);
                gOriginalBuyPartsSetup = nullptr;
                Log("ETAPA11: erro ao criar hook Unlock=%d", static_cast<int>(unlock));
                return false;
            }
            unlock = MH_EnableHook(reinterpret_cast<void*>(kUnlockSystemIsPerfPartUnlocked));
            if (unlock != MH_OK)
            {
                MH_RemoveHook(reinterpret_cast<void*>(kUnlockSystemIsPerfPartUnlocked));
                MH_RemoveHook(target);
                gOriginalUnlockPerfPart = nullptr;
                gOriginalBuyPartsSetup = nullptr;
                Log("ETAPA11: erro ao ativar hook Unlock=%d", static_cast<int>(unlock));
                return false;
            }
        }
        setup = MH_EnableHook(target);
        if (setup != MH_OK)
        {
            if (gStage11NativeFilter)
            {
                MH_DisableHook(reinterpret_cast<void*>(kUnlockSystemIsPerfPartUnlocked));
                MH_RemoveHook(reinterpret_cast<void*>(kUnlockSystemIsPerfPartUnlocked));
                gOriginalUnlockPerfPart = nullptr;
            }
            MH_RemoveHook(target);
            gOriginalBuyPartsSetup = nullptr;
            Log("ETAPA11: erro ao ativar hook Setup=%d", static_cast<int>(setup));
            return false;
        }
        gStatusHooksReady = true;
        if (gStage13Audit)
        {
            // Usa a mesma funcao nativa da vtable 0079B660, nao altera vtables.
            // Se falhar, permanece no comportamento seguro da Etapa 12.
            if (!PrepareUniqueStatusVtable())
            {
                gStage13Audit = false;
                gStage13PostDrawWrite = false;
                Log("ETAPA13: hook NoPerfDraw falhou; auditoria DESATIVADA, Setup/Unlock permanecem nativos");
            }
        }
        if (gStage14Audit && !PrepareStage14MutationTrace())
        {
            gStage14Audit = false;
            Log("ETAPA14: auditoria indisponivel; filtro nativo continua funcionando");
        }
        Log("ETAPA16: hooks prontos; Setup=1 Unlock=%d trace=%d singleNative=%d Draw=0 VisibleHook=0 FNGauxiliar=0", gStage11NativeFilter ? 1 : 0, gStage16Trace ? 1 : 0, gStage16SingleRow ? 1 : 0);
        return true;
    }

    // Diagnostico SOMENTE LEITURA: intercepta chamadas existentes, imprime os argumentos
    // e encaminha integralmente para as funcoes originais. Nao decide opcoes nem compra nada.
    // Este rastreamento nao pressupoe que a VCALL 0 do PPCustomizePackage seja a selecao.
    void CopyPkgName(const char* source, char (&destination)[64])
    {
        strcpy_s(destination, sizeof(destination), "<inacessivel>");
        if (!source) { strcpy_s(destination, sizeof(destination), "<null>"); return; }
        for (size_t i = 0; i < sizeof(destination) - 1; ++i)
        {
            if (!MemoryAvailable(source + i, 1)) { destination[i] = 0; return; }
            const unsigned char c = static_cast<unsigned char>(source[i]);
            if (c == 0) { destination[i] = 0; return; }
            destination[i] = c >= 32 && c < 127 ? static_cast<char>(c) : '?';
            destination[i + 1] = 0;
        }
    }

    __declspec(noinline) DWORD __cdecl HookFEngGetLastButton(const char* pkg)
    {
        // Primeiro executar a funcao original para preservar seu resultado.
        const DWORD button = gOriginalFEngGetLastButton ? gOriginalFEngGetLastButton(pkg) : 0;
        if (gFrontendTraceActive && gTraceFrontendSelection)
        {
            const LONG count = InterlockedIncrement(&gFrontendGetCalls);
            if (count <= 60)
            {
                char name[64] = {};
                CopyPkgName(pkg, name);
                Log("FETRACE: GetLastButton #%ld package='%s' button=%08lX caller=%p", count, name, button, _ReturnAddress());
            }
        }
        return button;
    }

    __declspec(noinline) void __cdecl HookFEngSetLastButton(const char* pkg, DWORD button)
    {
        if (gFrontendTraceActive && gTraceFrontendSelection)
        {
            const LONG count = InterlockedIncrement(&gFrontendSetCalls);
            if (count <= 60)
            {
                char name[64] = {};
                CopyPkgName(pkg, name);
                Log("FETRACE: SetLastButton #%ld package='%s' button=%08lX caller=%p", count, name, button, _ReturnAddress());
            }
        }
        if (gFrontendTraceActive && IsPerformancePackageName(pkg) &&
            (gExtremeOptionOrdinal < 0 || button != static_cast<DWORD>(gExtremeOptionOrdinal)))
        {
            if (gStage18NativePackage && gExtremeOptionOrdinal >= 0 &&
                gLastPerformanceButton == static_cast<DWORD>(gExtremeOptionOrdinal))
            {
                Log("ETAPA19 AO_SAIR: textoLocalized=%ld textoSetHash=%ld bloqueios=%ld/%ld/%ld/%ld",
                    gCategoryUniqueNameMatches, gStage19SetLanguageMatches,
                    gStage19BlockedBuyPackage, gStage19BlockedInstallPackage,
                    gStage19BlockedBuyPart, gStage19BlockedInstallPart);
            }
            ProbeHideUniqueFNG();
            // Critico: nunca utilizar a referencia ao widget Unique quando o Stock desmontar a lista.
            if (gUniquePreviewWidget)
            {
                if (gStage13Audit) { Log("ETAPA13 RESUMO: drawTotal=%ld matched=%ld afterSetup=%ld postWrites=%ld modo=%d", gStage13DrawCalls, gStage13DrawMatches, gStage13DrawAfterSetup, gStage13PostWrites, gUniqueFNGProbeMode); }
                Log("UNIQUE EXIT: widget invalidado ANTES do botao=%lu", button);
            }
            if (gStage14Audit) { LogStage14Summary("AO_SAIR"); ResetStage14Trace(); }
            if (gStage15Snapshot) { Stage15RecordExit(); }
            gStage13AfterSetup = false;
            gUniquePreviewWidget = nullptr;
            gStage16Widget = nullptr;
            gStage16Accepted = 0;
            gStage16NativeRows = 0;
        }
        if (gOriginalFEngSetLastButton) { gOriginalFEngSetLastButton(pkg, button); }
        if (gFrontendTraceActive && gTraceFrontendSelection && IsPerformancePackageName(pkg))
        {
            gLastPerformanceButton = button;
            const int current = ReadCurrentPackageNodeIndex();
            gLastObservedScrollerIndex = current;
            const LONG samples = InterlockedIncrement(&gTraceScrollerSamples);
            if (samples <= 60)
            {
                Log("FE TRACE SELECT: button=%lu scrollerNodeIndex=%d extremeOrdinal=%d match=%d (apenas observacao)", button, current, gExtremeOptionOrdinal, current >= 0 && current == gExtremeOptionOrdinal ? 1 : 0);
            }
        }
    }

    __declspec(noinline) void __fastcall HookUIWidgetMenuAddStatOption(void* menu, void* /*edx*/, void* widget)
    {
        if (gPreviewSetupThread == GetCurrentThreadId() && menu == gPreviewSetupMenu && widget && MemoryAvailable(widget, 0x4C))
        {
            DWORD* stat = reinterpret_cast<DWORD*>(widget);
            if (stat[0] == kNoPerfPartsWidgetVtable)
            {
                gUniquePreviewWidget = stat; // Ponto de identidade, sem tocar na memoria do widget.
                InterlockedIncrement(&gPreviewStatusRows);
                Log("UNIQUE VIEW: placeholder nativo=%p (%s - Installed), vtable INTACTA", widget, UniqueDisplayPartName(gExtremeUniqueCategory));
            }
            else if (gStage16SingleRow && stat[0] == kNativePerfWidgetVtable && MemoryAvailable(widget, 0x50) && stat[19] != 0)
            {
                gStage16Widget = stat;
                InterlockedIncrement(&gStage16NativeRows);
                Log("ETAPA16 WIDGET_NATIVE: widget=%p vtable=%08lX partRef=%08lX", stat, stat[0], stat[19]);
            }
            else { Log("UNIQUE VIEW: widget inesperado=%08lX (nao modificado)", stat[0]); }
        }
        if (gFrontendTraceActive && gTraceFrontendSelection)
        {
            const LONG count = InterlockedIncrement(&gFrontendAddRows);
            if (count <= 60)
            {
                DWORD widgetClass = 0;
                DWORD perfPart = 0;
                if (widget && MemoryAvailable(widget, 0x50))
                {
                    widgetClass = *reinterpret_cast<const DWORD*>(widget);
                    perfPart = *(reinterpret_cast<const DWORD*>(widget) + 19);
                }
                Log("FE TRACE ROW: AddStatOption #%ld menu=%p widget=%p vtable=%08lX perfPartRef=%08lX caller=%p category=%s UniqueInstalled=%d lastButton=%lu lastNodeIndex=%d", count, menu, widget, widgetClass, perfPart, _ReturnAddress(), UniqueCategoryName(gExtremeUniqueCategory), gExtremeUniqueInstalled ? 1 : 0, gLastPerformanceButton, gLastObservedScrollerIndex);
            }
        }
        if (gOriginalUIWidgetMenuAddStatOption) { gOriginalUIWidgetMenuAddStatOption(menu, widget); }
        // Comparacao nativa Stage 3 versus Extreme: somente enderecos e classe.
        if (gStage16Trace && widget && MemoryAvailable(widget, 0x4C) &&
            (gLastPerformanceButton == 3 || gLastPerformanceButton == 4))
        {
            const LONG sample = InterlockedIncrement(&gStage16TraceRows);
            if (sample <= 40)
            {
                DWORD* row = reinterpret_cast<DWORD*>(widget);
                const bool regular = row[0] == kNativePerfWidgetVtable && MemoryAvailable(row, 0x50);
                const bool empty = row[0] == kNoPerfPartsWidgetVtable;
                DWORD* name = (regular || empty) ? reinterpret_cast<DWORD*>(static_cast<uintptr_t>(row[11])) : nullptr;
                DWORD* status = (regular || empty) ? reinterpret_cast<DWORD*>(static_cast<uintptr_t>(row[12])) : nullptr;
                Log("ETAPA16 COMPARAR: button=%lu type=%08lX row=%p partRef=%08lX name=%p validName=%d status=%p validStatus=%d", gLastPerformanceButton, row[0], row,
                    regular ? row[19] : 0, name, IsTextObject(name) ? 1 : 0, status, IsTextObject(status) ? 1 : 0);
            }
        }
        CaptureActiveWidget(menu, widget);
    }

    __declspec(noinline) void __fastcall HookUIWidgetMenuSetInitialOption(void* menu, void* /*edx*/, int index)
    {
        if (gFrontendTraceActive && gTraceFrontendSelection)
        {
            const LONG count = InterlockedIncrement(&gFrontendSetupCalls);
            if (count <= 40)
            {
                Log("FE TRACE LIST: SetInitialOption #%ld menu=%p index=%d caller=%p addedRows=%ld lastButton=%lu scrollerNodeIndex=%d extremeOrdinal=%d", count, menu, index, _ReturnAddress(), gFrontendAddRows, gLastPerformanceButton, ReadCurrentPackageNodeIndex(), gExtremeOptionOrdinal);
            }
        }
        if (gOriginalUIWidgetMenuSetInitialOption) { gOriginalUIWidgetMenuSetInitialOption(menu, index); }
        if (gUniqueFNGProbeMode >= 4 && menu == gLiveWidgetMenu) { TraceActiveWidgetTexts(); }
        if (gUniqueFNGProbeMode > 0 && gUniqueFNGProbeMode <= 9) { ProbeMaybeShowUniqueFNG(); }
    }

    bool AddPassiveTraceHook(uintptr_t target, void* replacement, void** original, const char* label)
    {
        if (!MemoryAvailable(reinterpret_cast<const void*>(target), 6, true))
        {
            Log("FE TRACE: %s em %08lX inacessivel", label, static_cast<unsigned long>(target));
            return false;
        }
        MH_STATUS status = MH_CreateHook(reinterpret_cast<void*>(target), replacement, original);
        if (status != MH_OK)
        {
            Log("FE TRACE: MH_CreateHook %s falhou: %d", label, static_cast<int>(status));
            return false;
        }
        status = MH_EnableHook(reinterpret_cast<void*>(target));
        if (status != MH_OK)
        {
            Log("FE TRACE: MH_EnableHook %s falhou: %d", label, static_cast<int>(status));
            MH_RemoveHook(reinterpret_cast<void*>(target));
            *original = nullptr;
            return false;
        }
        Log("FE TRACE: hook passivo %s ativo no endereco %08lX", label, static_cast<unsigned long>(target));
        return true;
    }

    // Etapa 10.1: FEngSetVisible NAO e interceptado. A etapa 10 original
    // travou em ambos os modos; sem dump nao sabemos qual hook foi responsavel.
    // Guardamos snapshots apenas nos eventos de entrada/saida do menu.

    // Etapa 9: interceptar a FUNCAO nativa Draw, nao editar entrada alguma da vtable.
    // O alvo vem exclusivamente de uma entrada da vtable, lida apos os outros ASIs iniciarem.
    // Modo 6 e totalmente passivo. Modo 7 suprime apenas Draw de widgets capturados.
    __declspec(noinline) void __fastcall HookBuyPerfPartWidgetDraw(DWORD* widget, void* /*edx*/)
    {
        if (gStage18NativePackage && gStage18PackageReady && widget &&
            MemoryAvailable(widget, 0x50) && widget[0] == kBuyPerfPartWidgetVtable &&
            gExtremeUniqueCategory >= 0 && gExtremeUniqueCategory < 9 &&
            widget[19] == static_cast<DWORD>(reinterpret_cast<uintptr_t>(&gStage18Packages[gExtremeUniqueCategory][3])))
        {
            InterlockedIncrement(&gStage18UniqueDraws);
            // A estrutura tem indice de peca REAL para evitar leituras fora da tabela;
            // o resultado visual (nome/status) vem das flags Unique, NAO dessa peca.
            if (gOriginalBuyPerfPartWidgetDraw) { gOriginalBuyPerfPartWidgetDraw(widget); }
            DWORD* name = WidgetTextAt(widget, 11);
            DWORD* status = WidgetTextAt(widget, 12);
            if (name && status && gExtremeUniqueInstalled && MemoryAvailable(reinterpret_cast<void*>(kFEngPrintf), 6, true))
            {
                auto print = reinterpret_cast<GamePrintfFn>(kFEngPrintf);
                print(name, "%s", UniqueDisplayPartName(gExtremeUniqueCategory));
                print(status, "%s", "Installed");
                InterlockedIncrement(&gStage18UniqueRewrites);
            }
            return;
        }
        // Etapa 17: deixar Draw original atualizar o estado e, em seguida,
        // substituir somente o texto da linha Extreme validada.
        if (gStage17ClearStaleText)
        {
            if (gOriginalBuyPerfPartWidgetDraw) { gOriginalBuyPerfPartWidgetDraw(widget); }
            if (Stage17WriteNativeRow(widget)) { InterlockedIncrement(&gStage17Rewrites); }
            return;
        }
        if (gUniqueFNGProbeMode >= 8)
        {
            // Opcional: somente contadores atomicos. Sem VirtualQuery, leitura
            // de vtable, FEngFindObject ou escrita em log dentro de Draw.
            InterlockedIncrement(&gStage10TotalDraw);
            if (gUniqueFNGRootShown && gExtremeOptionOrdinal >= 0 &&
                gLastPerformanceButton == static_cast<DWORD>(gExtremeOptionOrdinal))
            {
                InterlockedIncrement(&gStage10DrawWhileExtreme);
            }
            if (gOriginalBuyPerfPartWidgetDraw) { gOriginalBuyPerfPartWidgetDraw(widget); }
            return;
        }
        bool matched = false;
        if (widget && gFrontendTraceActive && gUniqueFNGRootShown && gExtremeUniqueInstalled && gExtremeGarageSlot >= 0 &&
            gExtremeOptionOrdinal >= 0 && gLastPerformanceButton == static_cast<DWORD>(gExtremeOptionOrdinal) &&
            gLiveWidgetMenu && gLiveWidgetCount >= 3 && gLiveWidgetCount <= 16 && MemoryAvailable(widget, 0x50) &&
            widget[0] == kBuyPerfPartWidgetVtable)
        {
            for (unsigned int i = 0; i < gLiveWidgetCount; ++i)
            {
                if (gLiveWidgets[i].widget == widget && WidgetTextAt(widget, 11) == gLiveWidgets[i].name &&
                    WidgetTextAt(widget, 12) == gLiveWidgets[i].status)
                {
                    matched = true;
                    break;
                }
            }
        }
        if (matched)
        {
            const LONG sample = InterlockedIncrement(&gUniqueDrawCalls);
            if (sample <= 5)
            {
                DWORD* name = WidgetTextAt(widget, 11);
                DWORD* status = WidgetTextAt(widget, 12);
                const DWORD nameFlags = name ? *reinterpret_cast<const DWORD*>(reinterpret_cast<const BYTE*>(name) + 0x1C) : 0;
                const DWORD statusFlags = status ? *reinterpret_cast<const DWORD*>(reinterpret_cast<const BYTE*>(status) + 0x1C) : 0;
                Log("UNIQUE ETAPA9 DRAW: chamada=%ld widget=%p mode=%d ocultacao=%d nomeFlags=%08lX statusFlags=%08lX", sample, widget, gUniqueFNGProbeMode, gLiveWidgetTextsHidden ? 1 : 0, nameFlags, statusFlags);
            }
            if (gUniqueFNGProbeMode == 7 && gLiveWidgetTextsHidden && gNativeRowsHidden)
            {
                InterlockedIncrement(&gUniqueDrawSuppressed);
                return;
            }
        }
        if (gOriginalBuyPerfPartWidgetDraw) { gOriginalBuyPerfPartWidgetDraw(widget); }
    }

    void InstallNativeDrawHook()
    {
        if (gNativeDrawHookTried || gUniqueFNGProbeMode < 6) { return; }
        if (gUniqueFNGProbeMode >= 10 && !gStage17ClearStaleText && !gStage18NativePackage) { return; }
        if (gUniqueFNGProbeMode >= 8 && !gStage10DrawAuditEnabled && !gStage17ClearStaleText && !gStage18NativePackage)
        {
            Log("ETAPA10.1: hook Draw desativado por seguranca (EnableStage10DrawAudit=0)");
            return;
        }
        gNativeDrawHookTried = true;
        auto* slot = reinterpret_cast<void* const*>(kBuyPerfPartWidgetDrawEntry);
        if (!MemoryAvailable(slot, sizeof(void*)))
        {
            Log("UNIQUE ETAPA9: entrada vtable Draw inacessivel; permanecendo sem hook");
            return;
        }
        void* target = *slot;
        if (!target || !MemoryAvailable(target, 6, true))
        {
            Log("UNIQUE ETAPA9: alvo Draw invalido (%p); permanecendo sem hook", target);
            return;
        }
        MH_STATUS status = MH_CreateHook(target, reinterpret_cast<void*>(&HookBuyPerfPartWidgetDraw), reinterpret_cast<void**>(&gOriginalBuyPerfPartWidgetDraw));
        if (status != MH_OK)
        {
            Log("UNIQUE ETAPA9: MH_CreateHook(Draw) falhou=%d", static_cast<int>(status));
            return;
        }
        status = MH_EnableHook(target);
        if (status != MH_OK)
        {
            Log("UNIQUE ETAPA9: MH_EnableHook(Draw) falhou=%d", static_cast<int>(status));
            MH_RemoveHook(target);
            gOriginalBuyPerfPartWidgetDraw = nullptr;
            return;
        }
        gNativeDrawHookReady = true;
        Log("UNIQUE ETAPA9/10: hook Draw instalado na funcao=%p, vtable[%p] preservada; mode=%d", target, slot, gUniqueFNGProbeMode);
    }

    void InstallFrontendTraceHooks()
    {
        if (gFrontendTraceHooksTried || !gTraceFrontendSelection) { return; }
        gFrontendTraceHooksTried = true;
        int installed = 0;
        installed += AddPassiveTraceHook(kFEngGetLastButton, reinterpret_cast<void*>(&HookFEngGetLastButton), reinterpret_cast<void**>(&gOriginalFEngGetLastButton), "FEngGetLastButton") ? 1 : 0;
        installed += AddPassiveTraceHook(kFEngSetLastButton, reinterpret_cast<void*>(&HookFEngSetLastButton), reinterpret_cast<void**>(&gOriginalFEngSetLastButton), "FEngSetLastButton") ? 1 : 0;
        installed += AddPassiveTraceHook(kUIWidgetMenuAddStatOption, reinterpret_cast<void*>(&HookUIWidgetMenuAddStatOption), reinterpret_cast<void**>(&gOriginalUIWidgetMenuAddStatOption), "UIWidgetMenu_AddStatOption") ? 1 : 0;
        installed += AddPassiveTraceHook(kUIWidgetMenuSetInitialOption, reinterpret_cast<void*>(&HookUIWidgetMenuSetInitialOption), reinterpret_cast<void**>(&gOriginalUIWidgetMenuSetInitialOption), "UIWidgetMenu_SetInitialOption") ? 1 : 0;
        Log("FE TRACE: hooks instalados=%d/4; observando construcao e navegacao SEM filtrar ou instalar pecas", installed);
        if (gUniqueFNGProbeMode >= 8)
        {
            Log("ETAPA10.1: FEngSetVisible SEM hook; auditoria por snapshots de flags. Draw opt-in=%d", gStage10DrawAuditEnabled ? 1 : 0);
        }
        InstallNativeDrawHook();
    }

    bool InstallAddOptionHook(void* screen)
    {
        if (gAddOptionHookTried) { return gAddOptionHookReady; }
        gAddOptionHookTried = true;
        if (!MemoryAvailable(screen, sizeof(void*))) { Log("Screen invalida para localizar AddOption"); return false; }
        void** vtable = *reinterpret_cast<void***>(screen);
        if (!MemoryAvailable(vtable, 7 * sizeof(void*))) { Log("Vtable invalida para localizar AddOption"); return false; }
        void* target = vtable[6]; // (*menu + 24): AddOption no codigo do Unlimiter.
        if (!MemoryAvailable(target, 6, true)) { Log("Funcao virtual AddOption invalida"); return false; }
        MH_STATUS status = MH_CreateHook(target, reinterpret_cast<LPVOID>(&HookAddOption), reinterpret_cast<LPVOID*>(&gOriginalAddOption));
        if (status != MH_OK) { Log("MH_CreateHook(AddOption) falhou: %d", static_cast<int>(status)); return false; }
        status = MH_EnableHook(target);
        if (status != MH_OK) { Log("MH_EnableHook(AddOption) falhou: %d", static_cast<int>(status)); MH_RemoveHook(target); gOriginalAddOption = nullptr; return false; }
        gAddOptionHookReady = true;
        Log("Hook AddOption instalado em %p", target);
        return true;
    }

    void __fastcall HookSetup(void* screen, void* /*edx*/)
    {
        if (!gOriginalSetup) { return; }
        if (!MemoryAvailable(reinterpret_cast<void*>(kFEDatabase), sizeof(DWORD)) || !MemoryAvailable(reinterpret_cast<void*>(kGarageContext), sizeof(DWORD)))
        {
            gOriginalSetup(screen);
            return;
        }

        volatile DWORD* database = reinterpret_cast<volatile DWORD*>(kFEDatabase);
        volatile DWORD* context = reinterpret_cast<volatile DWORD*>(kGarageContext);
        const DWORD db = *database;
        const DWORD mode = *context;
        LONG hits = InterlockedIncrement(&gHookCalls);
        if (hits <= 20) { Log("Setup #%ld: FEDatabase=%lu, Contexto=%lu, StageBoxes=%d, Extreme=%d", hits, db, mode, gEnableStageBoxes ? 1 : 0, gEnableExtremeBox ? 1 : 0); }

        if (gEnableStageBoxes && db == 1 && mode == 1)
        {
            bool installExtreme = gEnableExtremeBox && (!gStage18NativePackage || gStage19MutationGuardsReady) && InstallAddOptionHook(screen);
            if (installExtreme)
            {
                gChooseScreenForTrace = screen;
                gChooseScreenVtable = *reinterpret_cast<const DWORD*>(screen);
                gChooseOptionOrdinal = 0;
                gExtremeOptionOrdinal = -1;
                ProbeHideUniqueFNG();
                gUniqueFNGShowAttempted = false;
                gUniqueFNGProbeSamples = 0;
                gUniquePreviewWidget = nullptr;
                gStage18PackageReady = false;
                gStage18Source = 0;
                gStage3OriginalIconHash = 0;
                gLastPerformanceButton = 0;
                gLastObservedScrollerIndex = -999;
                InterlockedExchange(&gUniqueDrawCalls, 0);
                InterlockedExchange(&gUniqueDrawSuppressed, 0);
                InterlockedExchange(&gStage10TotalDraw, 0);
                InterlockedExchange(&gStage10DrawWhileExtreme, 0);
            }
            if (installExtreme)
            {
                ReadUniqueStateForCurrentCar();
                InstallFrontendTraceHooks();
                if (gStage11PassiveGate || gStage11NativeFilter) { InstallStage11NativeHooks(); }
                else { InstallUniqueStatusHooks(); }
                gFrontendTraceActive = true;
            }
            RestoreValue restore{ context, mode };
            *context = 2;
            ScopedInjection injection(screen, installExtreme);
            if (hits <= 20) { Log("Montando Stage 1/2/3%s na garagem", installExtreme ? " + Extreme (visual)" : ""); }
            gOriginalSetup(screen);
            return;
        }
        gOriginalSetup(screen);
    }

    DWORD WINAPI Start(LPVOID)
    {
        char modulePath[MAX_PATH] = {};
        if (!GetModuleFileNameA(gModule, modulePath, MAX_PATH)) { return 0; }
        char* slash = strrchr(modulePath, '\\');
        if (!slash) { return 0; }
        *(slash + 1) = '\0';
        sprintf_s(gIni, sizeof(gIni), "%sUG2CareerPerformanceMenu.ini", modulePath);
        sprintf_s(gLog, sizeof(gLog), "%sUG2CareerPerformanceMenu.log", modulePath);
        gDebugLog = GetPrivateProfileIntA("Settings", "DebugLog", 1, gIni) != 0;
        bool enabled = GetPrivateProfileIntA("Settings", "Enabled", 1, gIni) != 0;
        gEnableStageBoxes = GetPrivateProfileIntA("Settings", "EnableStageBoxes", 0, gIni) != 0;
        gEnableExtremeBox = GetPrivateProfileIntA("Settings", "EnableExtremeBox", 0, gIni) != 0;
        gTraceFrontendSelection = GetPrivateProfileIntA("Settings", "TraceFrontendSelection", 1, gIni) != 0;
        gEnableExtremeStatusPreview = GetPrivateProfileIntA("Settings", "EnableExtremeStatusPreview", 0, gIni) != 0;
        gUniqueFNGProbeMode = GetPrivateProfileIntA("Settings", "UniqueFNGProbeMode", 18, gIni);
        gStage10DrawAuditEnabled = GetPrivateProfileIntA("Settings", "EnableStage10DrawAudit", 0, gIni) != 0;
        if (gUniqueFNGProbeMode < 0 || gUniqueFNGProbeMode > 21) { gUniqueFNGProbeMode = 0; }
        gStage18NativePackage = gUniqueFNGProbeMode == 21;
        gCategoryUniqueNameEnabled = GetPrivateProfileIntA("Settings", "EnableCategoryUniqueName", 1, gIni) != 0;
        gStage11PassiveGate = gUniqueFNGProbeMode == 10;
        gStage11NativeFilter = gUniqueFNGProbeMode >= 11 && gUniqueFNGProbeMode <= 20;
        gStage12Inspect = gUniqueFNGProbeMode >= 12 && gUniqueFNGProbeMode <= 17;
        gStage12Write = gUniqueFNGProbeMode >= 13 && gUniqueFNGProbeMode <= 17;
        gStage13Audit = gUniqueFNGProbeMode == 14 || gUniqueFNGProbeMode == 15;
        gStage13PostDrawWrite = gUniqueFNGProbeMode == 15;
        gStage14Audit = gUniqueFNGProbeMode == 16;
        gStage15Snapshot = gUniqueFNGProbeMode == 17;
        gStage16Trace = gUniqueFNGProbeMode >= 18 && gUniqueFNGProbeMode <= 20;
        // Opt-in adicional obrigatorio: a interface ainda possui Install Package acionavel.
        gStage16SingleRow = (gUniqueFNGProbeMode == 19 || gUniqueFNGProbeMode == 20) && GetPrivateProfileIntA("Settings", "EnableStage16UnsafeNativeRow", 0, gIni) != 0;
        gStage17ClearStaleText = gUniqueFNGProbeMode == 20 && gStage16SingleRow;
        if (gUniqueFNGProbeMode == 19 && !gStage16SingleRow) { Log("ETAPA16: modo 19 BLOQUEADO. Exige EnableStage16UnsafeNativeRow=1; permanece com filtro de lista vazia."); }
        if (gStage11PassiveGate || gStage11NativeFilter) { gEnableExtremeStatusPreview = false; gStage10DrawAuditEnabled = false; }
        if (gUniqueFNGProbeMode && gEnableExtremeStatusPreview)
        {
            // Nao misturar o filtro de widgets StockFix com o teste do pacote separado.
            gEnableExtremeStatusPreview = false;
            Log("UNIQUE FNG PROBE: filtro antigo desativado durante teste de carregamento");
        }
        if (gUniqueFNGProbeMode) { gTraceFrontendSelection = true; }
        gAllowScrollerTransitionForPreview = GetPrivateProfileIntA("Settings", "AllowScrollerTransitionForPreview", 1, gIni) != 0;
        if (gEnableExtremeStatusPreview) { gTraceFrontendSelection = true; }
        gExtremeVisualUnlocked = GetPrivateProfileIntA("Settings", "ExtremeVisualUnlocked", 0, gIni) != 0;
        gAutoDetectInstalledUnique = GetPrivateProfileIntA("Settings", "AutoDetectInstalledUnique", 1, gIni) != 0;
        // A chave de texto e fixa, original do jogo; nao aceita hash privado do INI.
        strcpy_s(gExtremeTextKey, sizeof(gExtremeTextKey), "UNIQUES_SHOP_TITLE");
        Log("UG2CareerPerformanceMenu Etapa19.1-OriginalFrontB | enabled=%d | StageBoxes=%d | ExtremeBox=%d | UniqueAutoDetect=%d | VisualUnlockedFallback=%d", enabled ? 1 : 0, gEnableStageBoxes ? 1 : 0, gEnableExtremeBox ? 1 : 0, gAutoDetectInstalledUnique ? 1 : 0, gExtremeVisualUnlocked ? 1 : 0);
        if (!enabled) { Log("Desabilitado pelo INI"); return 0; }
        if (!CheckExeVersion()) { Log("EXE nao corresponde ao SPEED2.EXE v1.2 suportado; nenhum hook aplicado"); return 0; }
        if (gEnableExtremeBox && !gEnableStageBoxes) { Log("EnableExtremeBox requer EnableStageBoxes=1; caixa Extreme nao sera inserida"); }
        Sleep(2500); // Permite aos outros ASIs, incluindo Unlimiter, instalar hooks antes.

        if (!MemoryAvailable(reinterpret_cast<void*>(kSetupVtableEntry), sizeof(void*))) { Log("Entrada de vtable do menu inacessivel"); return 0; }
        void* target = *reinterpret_cast<void**>(kSetupVtableEntry);
        if (!MemoryAvailable(target, 6, true)) { Log("Ponteiro de Setup invalido"); return 0; }
        Log("ChoosePerformancePackages::Setup em %p", target);
        MH_STATUS status = MH_Initialize();
        if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) { Log("MH_Initialize falhou: %d", static_cast<int>(status)); return 0; }
        // Hasheamento interno do jogo. A imagem DDS no TexEd deve ter exatamente esse nome.
        if (gEnableExtremeBox)
        {
            if (!MemoryAvailable(reinterpret_cast<void*>(kGameHash), 6, true)) { Log("Funcao de hash do jogo nao disponivel; Extreme desabilitada"); gEnableExtremeBox = false; }
            else
            {
                auto gameHash = reinterpret_cast<GameHashFn>(kGameHash);
                // Icone resolvido durante o Setup a partir da opcao Stage 3, jamais do FrontB modificado.
                gExtremeTextHash = gameHash("UNIQUES_SHOP_TITLE"); // Chave vanilla presente nos language labels do NFSU2.
                // HookGetLocalizedString/SetLanguageHash exibem Engine Unique / ECU Unique na linha correspondente.
                gReturnToStockHash = gameHash("PERFORMANCE_RETURN_TO_STOCK");
            }
        }
        // Falha FECHADA: sem protecao 4/4 nao adiciona a caixa Extreme.
        if (gStage18NativePackage && !InstallStage19MutationGuards()) { gEnableExtremeBox = false; }
        InstallCategoryUniqueNameHook();
        InstallStage19NameHashHook();
        status = MH_CreateHook(target, reinterpret_cast<LPVOID>(&HookSetup), reinterpret_cast<LPVOID*>(&gOriginalSetup));
        if (status != MH_OK) { Log("MH_CreateHook(Setup) falhou: %d", static_cast<int>(status)); return 0; }
        status = MH_EnableHook(target);
        if (status != MH_OK) { Log("MH_EnableHook(Setup) falhou: %d", static_cast<int>(status)); MH_RemoveHook(target); return 0; }
        Log("Hook instalado. Etapa 19.1 FrontB original: ProbeMode=%d categoria=%d hookNome=%d. Guards de compra/instalacao ativos=%d.", gUniqueFNGProbeMode, gCategoryUniqueNameEnabled ? 1 : 0, gCategoryUniqueNameHookReady ? 1 : 0, gStage19MutationGuardsReady ? 1 : 0);
        return 0;
    }
}

BOOL WINAPI DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        CareerMenu::gModule = module;
        DisableThreadLibraryCalls(module);
        HANDLE thread = CreateThread(nullptr, 0, CareerMenu::Start, nullptr, 0, nullptr);
        if (thread) { CloseHandle(thread); }
    }
    return TRUE;
}
