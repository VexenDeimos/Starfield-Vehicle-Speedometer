// Standalone Starfield land-vehicle circular speedometer (read-only).
// Raw vehicle speed is world-coordinate units per real second. Two in-game scanner runs support 1 U = 1 m.
#include <RE/Starfield.h>
#include <SFSE/SFSE.h>
#include <SFSEMCP/SFSEMenuFramework.hpp>
#include <Telemetry/SampleMath.h>
#include <Telemetry/GaugeLayout.h>
#include <Telemetry/GaugePalette.h>
#include <Telemetry/SpeedUnits.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <fstream>
#include <mutex>

namespace
{
    using Clock = std::chrono::steady_clock;
    constexpr float kPi = 3.14159265358979323846f;
    std::mutex g_lock;
    telemetry::VehicleMeter g_referenceMeter, g_playerMeter;
    telemetry::VehicleSpeedStabilizer g_speedFilter;
    bool g_enabled = true;
    bool g_csv = false;
    bool g_developerDetails = false;
    bool g_diagnosticLogging = false; // opt-in telemetry details; keep release logs quiet
    bool g_showPeak = false;
    int g_speedUnits = 1; // KM/H by default for new users; preserve existing Settings.ini
    float g_metersPerWorldUnit = 1.0f; // field-checked in Starfield: 329 vs 329.3 U, 545 vs 547 U
    int g_transport = 0; // connection label selected by user; no SAD dependency
    int g_palettePreset = 0; // Blue / White / Amber / Custom
    telemetry::palette::Style g_colors = telemetry::palette::kBlue;
    // Only the filled disc uses background opacity. The rim, labels and needle stay visible.
    float g_backgroundOpacityPercent = 226.0f/255.0f*100.0f;
    // Observed REV-8 driving: approximately 10-18 game units/s, NOT 2000.
    // Full-scale is user-adjustable to suit other vehicles/speed mods.
    float g_dialMaximum = 35.0f;
    float g_dialScale = telemetry::gauge::kCompassSize;
    int g_positionPreset = 0; // Compass, Top-left, Top-right, Health, Bottom-right, Custom
    float g_xOffset = telemetry::gauge::kCompassXOffset;
    float g_yOffset = telemetry::gauge::kCompassYOffset; // Y down+, X right+
    float g_customX = 320.0f, g_customY = 700.0f; // absolute screen-pixel center
    bool g_settingsLoaded = false;
    // Display-only unit-aware whole-number hysteresis. Never alters telemetry/CSV.
    bool g_steadyReadout = true;
    // Separate menu preview and outside-vehicle controls; neither changes telemetry.
    bool g_showInSettings = false; // hide while F1 is open unless opted in
    bool g_showOutsideVehicle = false; // optional stationary/idle gauge
    float g_needleResponseSeconds = 0.80f; // cruise damping; true acceleration/stop responds faster
    telemetry::gauge::NeedleSmoother g_needleDisplay;
    Clock::time_point g_needleFrame{};
    int g_displayedSpeed = -1;
    float g_smoothUnits = 0.0f;
    float g_rawUnits = 0.0f;
    float g_3dUnits = 0.0f;
    float g_verticalUnits = 0.0f;
    float g_acceleration = 0.0f;
    float g_peakUnits = 0.0f;
    // Read-only trip accumulator to compare with an in-game marker distance.
    float g_calibrationTripUnits = 0.0f;
    Clock::time_point g_calibrationTripPrevious{};
    bool g_hasValid = false, g_vehicleCamera = false;
    bool g_sampleHeld = false;
    float g_referenceSpeed = 0.0f, g_playerSpeed = 0.0f;
    bool g_referenceReading = false, g_playerReading = false;
    bool g_taskInstalled = false;
    std::uint64_t g_pollTicks = 0;
    Clock::time_point g_lastValid{}, g_lastCsv = Clock::now();
    Clock::time_point g_lastTelemetryPoll{}, g_lastVehicleCamera{}, g_lastDiagnostic{};
    std::uintptr_t g_lastVehicleIdentity = 0;
    // Independent speedometer sampling interval (not USB/Bluetooth input Hz).
    constexpr auto kTelemetryPeriod = std::chrono::milliseconds(50);
    constexpr auto kCameraGrace = std::chrono::milliseconds(350);
    // 0=not polled, 1=outside vehicle, 2=no player, 3=no vehicle reference,
    // 4=first position sample, 5=reference, 6=player fallback,
    // 7=brief camera loss, 8=sample dropout held, 9=stationary sample.
    int g_sensorStatus = 0;
    int g_lastLoggedStatus = -1;

    const char* sensorStatus(int status) noexcept
    {
        switch (status) {
            case 1: return "Outside vehicle camera";
            case 2: return "Player not ready";
            case 3: return "Vehicle ref missing; checking player";
            case 4: return "Waiting for second position sample";
            case 5: return "Vehicle position";
            case 6: return "Player-position fallback (verify)";
            case 7: return "Brief camera dropout (hold)";
            case 8: return "Missing position update (hold)";
            case 9: return "Vehicle position (stationary)";
            default: return "Waiting for telemetry task";
        }
    }
    constexpr const char* kSettingsPath = "Data/SFSE/Plugins/VehicleSpeedometer/Settings.ini";

    void saveSettings() noexcept
    {
        try {
            std::filesystem::create_directories("Data/SFSE/Plugins/VehicleSpeedometer");
            std::ofstream out(kSettingsPath,std::ios::trunc);
            if (!out) { REX::WARN("VehicleSpeedometer: could not save settings"); return; }
            out << "# Vehicle Speedometer v0.2.2-rc; field-checked default 1 world unit = 1 meter\n";
            out << "enabled=" << static_cast<int>(g_enabled) << '\n';
            out << "palettePreset=" << g_palettePreset << '\n';
            out << "backgroundOpacity=" << g_backgroundOpacityPercent << '\n';
            const auto writeRgb = [&](const char* prefix, const telemetry::palette::Rgb& rgb) {
                out << prefix << "R=" << rgb[0] << '\n';
                out << prefix << "G=" << rgb[1] << '\n';
                out << prefix << "B=" << rgb[2] << '\n';
            };
            writeRgb("needle",g_colors.needle);
            writeRgb("number",g_colors.number);
            writeRgb("units",g_colors.units);
            writeRgb("rim",g_colors.rim);
            writeRgb("ticks",g_colors.ticks);
            writeRgb("background",g_colors.background);
            writeRgb("progress",g_colors.progress);
            out << "preset=" << g_positionPreset << '\n';
            out << "xOffset=" << g_xOffset << '\n';
            out << "yOffset=" << g_yOffset << '\n';
            out << "customX=" << g_customX << '\n';
            out << "customY=" << g_customY << '\n';
            out << "size=" << g_dialScale << '\n';
            out << "fullScale=" << g_dialMaximum << '\n';
            out << "speedUnits=" << g_speedUnits << '\n';
            out << "metersPerWorldUnit=" << g_metersPerWorldUnit << '\n';
            out << "steadyNumbers=" << static_cast<int>(g_steadyReadout) << '\n';
            out << "showInSettings=" << static_cast<int>(g_showInSettings) << '\n';
            out << "showOutsideVehicle=" << static_cast<int>(g_showOutsideVehicle) << '\n';
            out << "needleResponseSeconds=" << g_needleResponseSeconds << '\n';
            out << "showPeak=" << static_cast<int>(g_showPeak) << '\n';
            out << "devDetails=" << static_cast<int>(g_developerDetails) << '\n';
            out << "diagnosticLogging=" << static_cast<int>(g_diagnosticLogging) << '\n';
            out << "csv=" << static_cast<int>(g_csv) << '\n';
            out << "transport=" << g_transport << '\n';
        } catch (...) { REX::WARN("VehicleSpeedometer: settings save exception"); }
    }

    void loadSettings() noexcept
    {
        if (g_settingsLoaded) return;
        g_settingsLoaded = true;
        std::ifstream in(kSettingsPath);
        if (!in) return;
        std::string line;
        while (std::getline(in,line)) {
            const auto equal=line.find('=');
            if (equal==std::string::npos || line.empty() || line[0]=='#') continue;
            const auto key=line.substr(0,equal);
            const auto value=line.substr(equal+1);
            // Invalid or nonfinite values are ignored. Preset and offsets are
            // clamped again by the gauge layout resolver on every render.
            char* end=nullptr;
            const float f=std::strtof(value.c_str(),&end);
            if (end==value.c_str() || !std::isfinite(f)) continue;
            if (key=="enabled") g_enabled = f!=0.0f;
            else if (key=="color") {
                // Migration from v0.1.8's two-color menu. Old files had no custom RGB values.
                g_palettePreset=static_cast<int>(std::clamp(f,0.0f,1.0f));
                g_colors=telemetry::palette::preset(g_palettePreset);
            }
            else if (key=="palettePreset") g_palettePreset=static_cast<int>(std::clamp(f,0.0f,3.0f));
            else if (key=="backgroundOpacity") g_backgroundOpacityPercent=std::clamp(f,0.0f,100.0f);
            else if (key=="preset") g_positionPreset=static_cast<int>(std::clamp(f,0.0f,5.0f));
            else if (key=="xOffset") g_xOffset=std::clamp(f,-1500.0f,1500.0f);
            else if (key=="yOffset") g_yOffset=std::clamp(f,-1500.0f,1500.0f);
            else if (key=="customX") g_customX=std::clamp(f,0.0f,7680.0f);
            else if (key=="customY") g_customY=std::clamp(f,0.0f,4320.0f);
            else if (key=="size") g_dialScale=telemetry::gauge::safeScale(f);
            else if (key=="fullScale") g_dialMaximum=std::clamp(f,10.0f,300.0f);
            else if (key=="speedUnits") g_speedUnits=static_cast<int>(std::clamp(f,0.0f,3.0f));
            else if (key=="metersPerWorldUnit") g_metersPerWorldUnit=telemetry::speedunits::safeMetersPerWorldUnit(f);
            else if (key=="metricCalibrationVerified") { /* Legacy v0.2.1 flag ignored: release scale is field-checked. */ }
            else if (key=="steadyNumbers") g_steadyReadout=f!=0.0f;
            else if (key=="showInSettings") g_showInSettings=f!=0.0f;
            else if (key=="showOutsideVehicle") g_showOutsideVehicle=f!=0.0f;
            else if (key=="needleResponseSeconds") g_needleResponseSeconds=std::clamp(f,0.0f,2.0f);
            else if (key=="showPeak") g_showPeak=f!=0.0f;
            else if (key=="devDetails") g_developerDetails=f!=0.0f;
            else if (key=="csv") g_csv=f!=0.0f;
            else if (key=="diagnosticLogging") g_diagnosticLogging=f!=0.0f;
            else if (key=="transport") g_transport=static_cast<int>(std::clamp(f,0.0f,2.0f));
            else {
                const auto loadRgb = [&](const char* prefix, telemetry::palette::Rgb& rgb) {
                    const std::string stem(prefix);
                    if (key==stem+"R") { rgb[0]=telemetry::palette::bounded(f); return true; }
                    if (key==stem+"G") { rgb[1]=telemetry::palette::bounded(f); return true; }
                    if (key==stem+"B") { rgb[2]=telemetry::palette::bounded(f); return true; }
                    return false;
                };
                if (loadRgb("needle",g_colors.needle) ||
                    loadRgb("number",g_colors.number) ||
                    loadRgb("units",g_colors.units) ||
                    loadRgb("rim",g_colors.rim) ||
                    loadRgb("ticks",g_colors.ticks) ||
                    loadRgb("background",g_colors.background) ||
                    loadRgb("progress",g_colors.progress)) continue;
            }
        }
        REX::INFO("VehicleSpeedometer settings loaded: preset={} xOffset={:.0f} yOffset={:.0f} size={:.2f} dialMax={:.0f}u/s units={} metersPerWorldUnit={:.4f} telemetryLog={}",
            g_positionPreset,g_xOffset,g_yOffset,g_dialScale,g_dialMaximum,
            telemetry::speedunits::label(g_speedUnits),g_metersPerWorldUnit,g_diagnosticLogging);
    }

    const char* transport() noexcept
    {
        return g_transport == 1 ? "USB" : g_transport == 2 ? "Bluetooth" : "Unlabeled";
    }
    double seconds(Clock::time_point now) noexcept
    {
        return std::chrono::duration<double>(now.time_since_epoch()).count();
    }

    void status(int value, bool vehicleCamera) noexcept
    {
        {
            std::scoped_lock guard(g_lock);
            g_sensorStatus = value;
            g_vehicleCamera = vehicleCamera;
        }
        if (g_diagnosticLogging && value != g_lastLoggedStatus) {
            REX::INFO("VehicleSpeedometer telemetry: {}", sensorStatus(value));
            g_lastLoggedStatus = value;
        } else if (!g_diagnosticLogging) {
            g_lastLoggedStatus = -1; // emit current status if diagnostics are later enabled
        }
    }

    void clearVehicle() noexcept
    {
        g_referenceMeter.reset();
        g_playerMeter.reset();
        g_speedFilter.reset();
        g_lastVehicleIdentity = 0;
        g_lastVehicleCamera = {};
        std::scoped_lock guard(g_lock);
        g_calibrationTripPrevious={};
        g_calibrationTripUnits=0.0f;
        g_hasValid = false;
        g_sampleHeld = false;
        g_referenceSpeed = g_playerSpeed = 0.0f;
        g_referenceReading = g_playerReading = false;
        g_rawUnits = g_smoothUnits = g_3dUnits = 0.0f;
        g_verticalUnits = g_acceleration = 0.0f;
        g_peakUnits = 0.0f; // peak belongs to this drive
    }

    // Sampling the physics reference on every permanent-task callback can
    // repeatedly sample the same transform before physics has advanced,
    // causing 0-speed readings. Sample a 50ms position window instead.
    void pollGameThread() noexcept
    {
        const auto now = Clock::now();
        if (g_lastTelemetryPoll.time_since_epoch().count() != 0 &&
            now - g_lastTelemetryPoll < kTelemetryPeriod) return;
        g_lastTelemetryPoll = now;
        {
            std::scoped_lock guard(g_lock);
            ++g_pollTicks;
        }
        auto* player = RE::PlayerCharacter::GetSingleton();
        auto* camera = RE::PlayerCamera::GetSingleton();
        const bool cameraNow = camera && camera->QCameraEquals(RE::CameraState::kVehicle);
        if (cameraNow && player) {
            g_lastVehicleCamera = now;
        } else {
            // Preserve a brief camera transition/missing-player frame only
            // after we have already established an active vehicle session.
            if (g_lastVehicleCamera.time_since_epoch().count() != 0 &&
                now - g_lastVehicleCamera < kCameraGrace) {
                status(7,true);
                return;
            }
            clearVehicle();
            status(player ? 1 : 2,false);
            return;
        }

        const double time = seconds(now);
        bool referenceReadable = false;
        telemetry::SpeedSample referenceSample{};
        std::uintptr_t identity = 0;
        if (player->currentProcess && player->currentProcess->middleHigh) {
            auto& occupied = player->currentProcess->middleHigh->occupiedFurniture;
            if (occupied.get_handle() != 0) {
                auto ref = occupied.get();
                if (ref && ref->GetFormID() != 0) {
                    if (auto base = ref->GetBaseObject();
                        base && base->GetFormType() == RE::FormType::kFURN) {
                        const auto p = ref->GetPosition();
                        identity = reinterpret_cast<std::uintptr_t>(ref.get());
                        referenceReadable = true;
                        if (g_lastVehicleIdentity != 0 && g_lastVehicleIdentity != identity) {
                            // New vehicle: never splice two world locations or
                            // carry its speed/peak into the next drive.
                            g_referenceMeter.reset();
                            g_playerMeter.reset();
                            g_speedFilter.reset();
                            std::scoped_lock guard(g_lock);
                            g_calibrationTripPrevious={};
                            g_calibrationTripUnits=0.0f;
                            g_hasValid = false;
                            g_peakUnits = 0.0f;
                        }
                        g_lastVehicleIdentity = identity;
                        referenceSample = g_referenceMeter.observe(identity,{p.x,p.y,p.z},time);
                    }
                }
            }
        }
        // Do NOT discard reference sampling history on a single lost handle.
        // The next valid sample can use the elapsed time across the gap.

        // Separate read-only player-position path as a fallback. Both sources
        // are tracked independently; camera alone is NOT proof of velocity.
        const auto p = player->GetPosition();
        const auto playerSample = g_playerMeter.observe(
            reinterpret_cast<std::uintptr_t>(player),{p.x,p.y,p.z},time);
        const bool refMoving = referenceSample.valid && referenceSample.horizontalUnitsPerSecond >= 5.0;
        const bool playerMoving = playerSample.valid && playerSample.horizontalUnitsPerSecond >= 5.0;
        const bool usePlayer = playerMoving && !refMoving;
        const auto& sample = usePlayer ? playerSample :
            referenceSample.valid ? referenceSample : playerSample;
        const auto output = g_speedFilter.observe(sample.valid,
            sample.horizontalUnitsPerSecond, time);
        {
            std::scoped_lock guard(g_lock);
            g_referenceReading = referenceSample.valid;
            g_playerReading = playerSample.valid;
            g_referenceSpeed = referenceSample.valid ?
                static_cast<float>(referenceSample.horizontalUnitsPerSecond) : 0.0f;
            g_playerSpeed = playerSample.valid ?
                static_cast<float>(playerSample.horizontalUnitsPerSecond) : 0.0f;
            g_sampleHeld = output.held;
            g_hasValid = output.valid;
            if (output.valid) {
                // Raw sensor velocity remains diagnostic only. The main gauge
                // follows the stabilized measured transform velocity.
                if (sample.valid) {
                    // Integrate only consecutive valid position samples to avoid
                    // adding false distance across loading / teleport / dropouts.
                    if (g_calibrationTripPrevious.time_since_epoch().count()!=0) {
                        const float elapsed=std::chrono::duration<float>(now-g_calibrationTripPrevious).count();
                        const float speed=static_cast<float>(sample.horizontalUnitsPerSecond);
                        if (elapsed>0.0f && elapsed<=0.20f && std::isfinite(speed) && speed>=0.5f && speed<250.0f)
                            g_calibrationTripUnits += speed*elapsed;
                    }
                    g_calibrationTripPrevious=now;
                    g_rawUnits = static_cast<float>(sample.horizontalUnitsPerSecond);
                    g_3dUnits = static_cast<float>(sample.threeDimensionalUnitsPerSecond);
                    g_verticalUnits = static_cast<float>(sample.verticalUnitsPerSecond);
                    g_acceleration = sample.accelerationValid ?
                        static_cast<float>(sample.accelerationUnitsPerSecondSquared) : 0.0f;
                    g_peakUnits = std::max(g_peakUnits,g_rawUnits);
                }
                if (!sample.valid) g_calibrationTripPrevious={};
                g_smoothUnits = static_cast<float>(output.speed);
                g_lastValid = now;
            } else {
                g_calibrationTripPrevious={};
            }
        }
        if (output.held) status(8,true);
        else if (!output.valid) status(referenceReadable ? 4 : 3,true);
        else if (usePlayer) status(6,true);
        else if (refMoving) status(5,true);
        else status(9,true);

        if (g_diagnosticLogging &&
            (g_lastDiagnostic.time_since_epoch().count() == 0 ||
             now - g_lastDiagnostic >= std::chrono::seconds(4))) {
            g_lastDiagnostic = now;
            REX::INFO("VehicleSpeedometer sample: referenceReadable={} refValid={} refSpeed={:.1f} playerValid={} playerSpeed={:.1f} source={} shown={:.1f} held={} camera=vehicle",
                referenceReadable, referenceSample.valid,
                referenceSample.horizontalUnitsPerSecond, playerSample.valid,
                playerSample.horizontalUnitsPerSecond,
                usePlayer ? "player" : "reference", output.speed, output.held);
        }
    }

    bool blockedByGameMenu() noexcept
    {
        auto* ui = RE::UI::GetSingleton();
        if (!ui) return true;
        static constexpr const char* names[] = {
            "MainMenu", "PauseMenu", "LoadingMenu", "FaderMenu", "DataMenu",
            "GalaxyStarMapMenu", "SurfaceMapMenu", "MapMenu",
            "ContainerMenu", "BarterMenu", "InventoryMenu", "TerminalMenu",
            "CraftingMenu", "ShipBuilderMenu", "DialogueMenu", "PhotoModeMenu",
            "WorkshopMenu", "FavoritesMenu", "MissionMenu",
            "SleepWaitMenu", "MessageBoxMenu", "ChargenMenu"
        };
        for (const char* name : names) {
            if (ui->IsMenuOpen(RE::BSFixedString(name))) return true;
        }
        return false;
    }

    void record(double t, float raw, float smooth, float threeD, float vertical,
        float accel, float peak, int sensor)
    {
        try {
            std::filesystem::create_directories("Data/SFSE/Plugins/VehicleSpeedometer");
            const char* path = "Data/SFSE/Plugins/VehicleSpeedometer/VehicleSpeedSamples.csv";
            const bool empty = !std::filesystem::exists(path) || std::filesystem::file_size(path) == 0;
            std::ofstream file(path,std::ios::app);
            if (empty) file << "seconds,transport,speed_horizontal_units_s,speed_smoothed_units_s,speed_3d_units_s,vertical_units_s,accel_units_s2,peak_units_s,sensor\n";
            file << t << ',' << transport() << ',' << raw << ',' << smooth << ',' << threeD
                 << ',' << vertical << ',' << accel << ',' << peak << ',' << sensorStatus(sensor) << '\n';
        } catch (...) {}
    }

    ImGuiMCP::ImVec2 radial(float x, float y, float r, float angle) noexcept
    {
        return { x + std::cos(angle)*r, y + std::sin(angle)*r };
    }
    void drawText(ImGuiMCP::ImDrawList* dl,float x,float y,
        const char* s,std::uint32_t c)
    {
        ImGuiMCP::ImDrawListManager::AddText(dl,{x,y},c,s);
    }
    void __stdcall renderHud()
    {
        // The framework's main window is not an ordinary RE::UI menu. Check
        // its actual open state, independent of the active Options tab.
        const auto* settingsWindow = SFSEMenuFramework::GetMainWindow();
        const bool settingsOpen = settingsWindow && settingsWindow->IsOpen.load();
        if (!g_enabled || blockedByGameMenu() || (settingsOpen && !g_showInSettings)) {
            g_displayedSpeed=-1;
            g_needleDisplay.reset();
            g_needleFrame={};
            return;
        }
        float raw,smooth,threeD,vertical,accel,peak,refSpeed,playerSpeed;
        bool refReading,playerReading,held;
        bool valid,camera;
        int statusCode;
        std::uint64_t ticks;
        Clock::time_point last;
        {
            std::scoped_lock guard(g_lock);
            valid = g_hasValid;
            camera = g_vehicleCamera;
            raw = g_rawUnits; smooth = g_smoothUnits; threeD = g_3dUnits;
            vertical = g_verticalUnits; accel = g_acceleration; peak = g_peakUnits;
            statusCode = g_sensorStatus; ticks = g_pollTicks;
            last = g_lastValid;
            refSpeed = g_referenceSpeed; playerSpeed = g_playerSpeed;
            refReading = g_referenceReading; playerReading = g_playerReading;
            held = g_sampleHeld;
        }
        const bool idlePreview = !camera && (g_showOutsideVehicle || (settingsOpen && g_showInSettings));
        if (!camera && !idlePreview) {
            g_displayedSpeed=-1;
            g_needleDisplay.reset();
            g_needleFrame={};
            return;
        }
        if (!camera || Clock::now()-last > std::chrono::milliseconds(650)) valid=false;
        if (!valid) g_displayedSpeed=-1;
        const auto* io = ImGuiMCP::GetIO();
        auto* dl = ImGuiMCP::GetForegroundDrawList();
        if (!dl || !io) return;
        const float scale = telemetry::gauge::safeScale(g_dialScale);
        const float radius = 72.0f*scale;
        const auto center = telemetry::gauge::resolve(
            io->DisplaySize.x,io->DisplaySize.y,scale,g_positionPreset,
            g_customX,g_customY,g_xOffset,g_yOffset);
        const float cx = center.x, cy = center.y;
        // Independent ABGR visual controls. Background alpha never fades the rim/needle.
        const auto accent = telemetry::palette::abgr(g_colors.progress);
        const auto pale = telemetry::palette::abgr(g_colors.units);
        const auto rim = telemetry::palette::abgr(g_colors.rim);
        const auto tickColor = telemetry::palette::abgr(g_colors.ticks);
        const auto needleColor = telemetry::palette::abgr(g_colors.needle);
        const auto numberColor = telemetry::palette::abgr(g_colors.number);
        const auto backgroundColor = telemetry::palette::abgr(
            g_colors.background,g_backgroundOpacityPercent/100.0f);
        if (g_backgroundOpacityPercent > 0.0f) {
            ImGuiMCP::ImDrawListManager::AddCircleFilled(dl,{cx,cy},radius,
                backgroundColor,72);
        }
        ImGuiMCP::ImDrawListManager::AddCircle(dl,{cx,cy},radius,rim,72,2.4f*scale);
        ImGuiMCP::ImDrawListManager::AddCircle(dl,{cx,cy},radius-4.0f*scale,
            telemetry::palette::abgr(g_colors.rim,0.50f),72,0.8f*scale);
        const float angleStart = 150.0f*kPi/180.0f;
        const float angleSpan = 240.0f*kPi/180.0f;
        const float maximum = telemetry::speedunits::convert(
            std::clamp(g_dialMaximum,10.0f,300.0f),g_speedUnits,g_metersPerWorldUnit);
        // The pointer and on-dial number use the same display signal, so
        // neither gets ahead of the other when accelerating. The real
        // telemetry, peak and CSV remain untouched.
        const auto frameNow = Clock::now();
        const float dt = g_needleFrame.time_since_epoch().count()!=0 ?
            std::chrono::duration<float>(frameNow-g_needleFrame).count() : 0.0f;
        g_needleFrame = frameNow;
        const float needleUnits = valid ? g_needleDisplay.observe(smooth,dt,g_needleResponseSeconds) : 0.0f;
        if (!valid) g_needleDisplay.reset();
        const float needleValue=telemetry::speedunits::convert(needleUnits,g_speedUnits,g_metersPerWorldUnit);
        const float proportion=maximum>0.0f ? std::clamp(needleValue/maximum,0.0f,1.0f) : 0.0f;
        for (int i=0;i<=40;++i) {
            const float a = angleStart+angleSpan*static_cast<float>(i)/40.0f;
            const bool major = (i%5)==0;
            const float outer = radius-9.0f*scale;
            const float inner = outer-(major ? 10.0f : 5.0f)*scale;
            ImGuiMCP::ImDrawListManager::AddLine(dl,radial(cx,cy,inner,a),
                radial(cx,cy,outer,a),tickColor,major ? 1.8f*scale : 1.0f*scale);
        }
        for (int i=1;i<=48;++i) {
            const float t0 = static_cast<float>(i-1)/48.0f;
            const float t1 = static_cast<float>(i)/48.0f;
            if (t1 > proportion) break;
            ImGuiMCP::ImDrawListManager::AddLine(dl,
                radial(cx,cy,radius-18.0f*scale,angleStart+angleSpan*t0),
                radial(cx,cy,radius-18.0f*scale,angleStart+angleSpan*t1),
                accent,3.0f*scale);
        }
        const float needle = angleStart+angleSpan*proportion;
        // This needle tracks measured world speed, not input event rate.
        // A second, dark underlay keeps it readable over the dial marks.
        ImGuiMCP::ImDrawListManager::AddLine(dl,{cx,cy},
            radial(cx,cy,radius-23.0f*scale,needle),0xD8101010u,5.0f*scale);
        ImGuiMCP::ImDrawListManager::AddLine(dl,{cx,cy},
            radial(cx,cy,radius-23.0f*scale,needle),needleColor,2.8f*scale);
        ImGuiMCP::ImDrawListManager::AddCircleFilled(dl,{cx,cy},3.5f*scale,needleColor,16);
        char line[135]{};
        if (valid) {
            const float displayed=needleValue;
            const float conversionFactor = telemetry::speedunits::factor(g_speedUnits,g_metersPerWorldUnit);
            g_displayedSpeed = g_steadyReadout ?
                telemetry::gauge::steadyIntegerSpeedScaled(displayed,g_displayedSpeed,conversionFactor) :
                telemetry::gauge::steadyIntegerSpeedScaled(displayed,-1,conversionFactor);
            std::snprintf(line,sizeof(line),"%d",g_displayedSpeed);
        }
        else std::snprintf(line,sizeof(line),"--");
        const float digits = static_cast<float>(std::strlen(line));
        drawText(dl,cx-digits*4.0f,cy+18.0f*scale,line,numberColor);
        const char* unitLabel=telemetry::speedunits::label(g_speedUnits);
        drawText(dl,cx-static_cast<float>(std::strlen(unitLabel))*3.8f*scale,
            cy+36.0f*scale,unitLabel,pale);
        if (idlePreview) drawText(dl,cx-40.0f*scale,cy-23.0f*scale,"NO VEHICLE",pale);
        else if (!valid) drawText(dl,cx-25.0f*scale,cy-23.0f*scale,"WAIT",pale);
        else drawText(dl,cx-23.0f*scale,cy-23.0f*scale,"SPEED",pale);
        if (g_showPeak) {
            std::snprintf(line,sizeof(line),"MAX %.0f",
                telemetry::speedunits::convert(peak,g_speedUnits,g_metersPerWorldUnit));
            drawText(dl,cx-29.0f*scale,cy+radius+4.0f*scale,line,accent);
        }
        if (g_developerDetails) {
            std::snprintf(line,sizeof(line),"Raw %.0f  3D %.0f  V %+.0f",raw,threeD,vertical);
            drawText(dl,cx-radius,cy-radius-36.0f*scale,line,0xFFE1EDF5u);
            std::snprintf(line,sizeof(line),"Accel %+.0f  Ticks %llu",accel,static_cast<unsigned long long>(ticks));
            drawText(dl,cx-radius,cy-radius-20.0f*scale,line,0xFFE1EDF5u);
            std::snprintf(line,sizeof(line),"Ref %s %.0f | Player %s %.0f | Hold %s",
                refReading ? "yes" : "no",refSpeed,playerReading ? "yes" : "no",playerSpeed,
                held ? "yes" : "no");
            drawText(dl,cx-radius,cy+radius+38.0f*scale,line,0xFFE1EDF5u);
            drawText(dl,cx-radius,cy+radius+56.0f*scale,sensorStatus(statusCode),0xFFFFD588u);
        }
        const auto now = Clock::now();
        if (g_csv && valid && now-g_lastCsv >= std::chrono::seconds(1)) {
            record(seconds(now),raw,smooth,threeD,vertical,accel,peak,statusCode);
            g_lastCsv = now;
        }
    }

    void __stdcall renderAppearanceSettings()
    {
        ImGuiMCP::TextUnformatted("Vehicle Speedometer | Appearance");
        ImGuiMCP::TextWrapped("Each element has its own color picker. Changes appear immediately when 'Show gauge while SFSE settings are open' is enabled on the Options page.");
        const char* themes[] = {"Starfield Blue (original)","Starfield White","Amber","Custom"};
        const int previousPreset = g_palettePreset;
        bool changed=ImGuiMCP::Combo("Color preset",&g_palettePreset,themes,4);
        if (g_palettePreset!=previousPreset && g_palettePreset<3) {
            g_colors=telemetry::palette::preset(g_palettePreset);
            // Only a reset button restores the original background transparency.
        }
        bool customEdited=false;
        customEdited |= ImGuiMCP::ColorEdit3("Needle + center cap",g_colors.needle.data());
        customEdited |= ImGuiMCP::ColorEdit3("Speed number",g_colors.number.data());
        customEdited |= ImGuiMCP::ColorEdit3("Units + text",g_colors.units.data());
        customEdited |= ImGuiMCP::ColorEdit3("Outer circle + inner rim",g_colors.rim.data());
        customEdited |= ImGuiMCP::ColorEdit3("Dial tick marks",g_colors.ticks.data());
        customEdited |= ImGuiMCP::ColorEdit3("Progress arc",g_colors.progress.data());
        customEdited |= ImGuiMCP::ColorEdit3("Circle background",g_colors.background.data());
        changed |= ImGuiMCP::SliderFloat("Background opacity (%)",&g_backgroundOpacityPercent,0.0f,100.0f,"%.0f%%");
        if (customEdited) { g_palettePreset=3; changed=true; }
        ImGuiMCP::TextWrapped("0% background opacity is fully transparent. Needle, numbers, ticks and rim remain visible. Color edits automatically select Custom.");
        if (ImGuiMCP::Button("Reset appearance to Starfield Blue")) {
            g_colors=telemetry::palette::kBlue;
            g_palettePreset=0;
            g_backgroundOpacityPercent=226.0f/255.0f*100.0f;
            changed=true;
        }
        if (changed) saveSettings();
    }

    void __stdcall renderSettings()
    {
        ImGuiMCP::TextUnformatted("Vehicle Speedometer | Gameplay HUD");
        bool changed=false;
        changed |= ImGuiMCP::Checkbox("Enable speedometer HUD",&g_enabled);
        changed |= ImGuiMCP::Checkbox("Show gauge while SFSE settings are open",&g_showInSettings);
        changed |= ImGuiMCP::Checkbox("Show gauge outside vehicles (idle)",&g_showOutsideVehicle);
        ImGuiMCP::TextWrapped("Outside a vehicle the gauge reads -- (no vehicle speed). Other game menus and maps always hide it. Enable the SFSE option to preview placement while editing these settings.");
        ImGuiMCP::TextWrapped("Choose colors in the Appearance tab. Position and appearance save separately.");
        ImGuiMCP::TextUnformatted("Position & Size");
        const char* presets[] = {"Beside compass","Top left","Top right",
            "Above health HUD","Bottom right","Custom X/Y"};
        const int oldPreset=g_positionPreset;
        changed |= ImGuiMCP::Combo("HUD position preset",&g_positionPreset,presets,6);
        if (oldPreset!=g_positionPreset) {
            // Match the user's calibrated beside-compass screenshot. Other
            // presets start centered on their own anchor with zero offsets.
            if (g_positionPreset==0) {
                g_xOffset=telemetry::gauge::kCompassXOffset;
                g_yOffset=telemetry::gauge::kCompassYOffset;
                g_dialScale=telemetry::gauge::kCompassSize;
            } else if (g_positionPreset==3) {
                g_xOffset=telemetry::gauge::kHealthXOffset;
                g_yOffset=telemetry::gauge::kHealthYOffset;
                g_dialScale=telemetry::gauge::kHealthSize;
            } else {
                g_xOffset=0.0f;
                g_yOffset=0.0f;
            }
        }
        changed |= ImGuiMCP::SliderFloat("X offset (px) - / +",&g_xOffset,-650.0f,650.0f);
        changed |= ImGuiMCP::SliderFloat("Y offset (px) up / down",&g_yOffset,-650.0f,650.0f);
        if (g_positionPreset==5) {
            changed |= ImGuiMCP::SliderFloat("Custom center X (px)",&g_customX,0.0f,3840.0f);
            changed |= ImGuiMCP::SliderFloat("Custom center Y (px)",&g_customY,0.0f,2160.0f);
        }
        changed |= ImGuiMCP::SliderFloat("Gauge size",&g_dialScale,0.60f,1.80f);
        const int previousUnits=g_speedUnits;
        const char* unitOptions[]={"U/S (raw)","KM/H","MPH","M/S"};
        changed |= ImGuiMCP::Combo("Speed units",&g_speedUnits,unitOptions,4);
        if (previousUnits!=g_speedUnits) { g_displayedSpeed=-1; g_needleDisplay.reset(); }
        changed |= ImGuiMCP::SliderFloat("Dial maximum (raw U/S)",&g_dialMaximum,10.0f,150.0f);
        char scaleLine[200]{};
        std::snprintf(scaleLine,sizeof(scaleLine),"Dial full-scale: %.0f %s (automatic unit conversion)",
            telemetry::speedunits::convert(g_dialMaximum,g_speedUnits,g_metersPerWorldUnit),
            telemetry::speedunits::label(g_speedUnits));
        ImGuiMCP::TextWrapped(scaleLine);
        if (ImGuiMCP::Button("Apply selected preset defaults")) {
            if (g_positionPreset==0) {
                g_xOffset=telemetry::gauge::kCompassXOffset;
                g_yOffset=telemetry::gauge::kCompassYOffset;
                g_dialScale=telemetry::gauge::kCompassSize;
            } else if (g_positionPreset==3) {
                g_xOffset=telemetry::gauge::kHealthXOffset;
                g_yOffset=telemetry::gauge::kHealthYOffset;
                g_dialScale=telemetry::gauge::kHealthSize;
            } else {
                g_xOffset=0.0f;
                g_yOffset=0.0f;
            }
            changed=true;
        }
        if (ImGuiMCP::Button("Reset HUD position and size")) {
            g_positionPreset=0;
            g_xOffset=telemetry::gauge::kCompassXOffset;
            g_yOffset=telemetry::gauge::kCompassYOffset;
            g_customX=320.0f; g_customY=700.0f;
            g_dialScale=telemetry::gauge::kCompassSize;
            g_dialMaximum=35.0f;
            changed=true;
        }
        ImGuiMCP::TextWrapped("X positive moves right; Y positive moves down. Position presets adapt to screen resolution.");
        ImGuiMCP::TextWrapped("U/S is raw world-coordinate units per second. Starfield scanner trips support 1 U = 1 metre, which is used for KM/H, MPH and M/S. Dial scaling follows the selected unit.");
        ImGuiMCP::TextUnformatted("Developer Options (optional)");
        changed |= ImGuiMCP::Checkbox("Extra telemetry on HUD",&g_developerDetails);
        changed |= ImGuiMCP::Checkbox("Write detailed telemetry to log",&g_diagnosticLogging);
        ImGuiMCP::TextWrapped("Detailed telemetry logging is OFF by default. Enable it only while troubleshooting; normal startup and error messages are still logged.");
        changed |= ImGuiMCP::Checkbox("Show max speed below dial",&g_showPeak);
        changed |= ImGuiMCP::SliderFloat("Needle smoothing (seconds)",&g_needleResponseSeconds,0.0f,2.0f);
        ImGuiMCP::TextWrapped("Cruise damping reduces needle shake. Acceleration and stopping respond faster; the on-dial number follows the needle. Raw telemetry and peak remain unchanged. 0 disables visual smoothing.");
        if (ImGuiMCP::Checkbox("Steady whole-number readout (display only)",&g_steadyReadout)) {
            g_displayedSpeed=-1;
            changed=true;
        }
        changed |= ImGuiMCP::Checkbox("Record CSV speed samples",&g_csv);
        const char* labels[] = {"Unlabeled","USB","Bluetooth"};
        changed |= ImGuiMCP::Combo("Connection label (manual)",&g_transport,labels,3);
        ImGuiMCP::TextUnformatted("World-distance calibration");
        ImGuiMCP::TextWrapped("Field-checked metric scale: 1 world unit = 1 metre. Two straight-line REV-8 scanner tests: 329 m vs 329.3 U, and 545 m vs 547 U. These are in-game checks, not an official engine specification.");
        if (ImGuiMCP::SliderFloat("Metres per world unit (advanced)",
                &g_metersPerWorldUnit,0.01f,5.0f,"%.3f m/U")) {
            g_displayedSpeed=-1;
            changed=true;
        }
        if (ImGuiMCP::Button("Restore field-checked scale (1.000 m/U)")) {
            g_metersPerWorldUnit=1.0f;
            g_displayedSpeed=-1;
            changed=true;
        }
        ImGuiMCP::TextWrapped("Custom scale overrides are for advanced testing. Changes apply to metric units, not raw U/S.");
        float trip;
        {
            std::scoped_lock guard(g_lock);
            trip=g_calibrationTripUnits;
        }
        char tripText[180]{};
        std::snprintf(tripText,sizeof(tripText),"Trip: %.1f U | %.1f m at selected scale",
            trip,trip*g_metersPerWorldUnit);
        ImGuiMCP::TextWrapped(tripText);
        if (ImGuiMCP::Button("Reset calibration trip distance")) {
            std::scoped_lock guard(g_lock);
            g_calibrationTripUnits=0.0f;
            g_calibrationTripPrevious={};
        }
        ImGuiMCP::TextWrapped("Compare the trip with a straight-line scanner distance if testing a custom scale. Turns, slopes and jumps can change the comparison.");
        if (ImGuiMCP::Button("Reset drive max speed")) {
            std::scoped_lock guard(g_lock);
            g_peakUnits=0.0f;
        }
        if (changed) saveSettings();
        int sensor;
        std::uint64_t ticks;
        bool camera;
        {
            std::scoped_lock guard(g_lock);
            sensor=g_sensorStatus; ticks=g_pollTicks; camera=g_vehicleCamera;
        }
        char line[160]{};
        std::snprintf(line,sizeof(line),"Sensor: %s | Vehicle camera: %s | Poll ticks: %llu",
            sensorStatus(sensor),camera ? "yes" : "no",static_cast<unsigned long long>(ticks));
        ImGuiMCP::TextWrapped(line);
        ImGuiMCP::TextWrapped("HUD settings are saved automatically. Game menus/maps hide the gauge; SFSE menu visibility and outside-vehicle idle display have independent switches.");
    }

    void onSfseMessage(SFSE::MessagingInterface::Message* msg)
    {
        if (!msg) return;
        if (msg->type == SFSE::MessagingInterface::kPostLoad) {
            loadSettings();
            if (SFSEMenuFramework::IsInstalled()) {
                SFSEMenuFramework::SetSection("Vehicle Speedometer");
                SFSEMenuFramework::AddSectionItem("Options",&renderSettings);
                SFSEMenuFramework::AddSectionItem("Appearance",&renderAppearanceSettings);
                static auto* hud = SFSEMenuFramework::AddHudElement(&renderHud);
                (void)hud;
                REX::INFO("VehicleSpeedometer v0.2.2-rc: field-checked metric scale, quiet diagnostics and independent HUD registered");
            } else {
                REX::WARN("VehicleSpeedometer requires SFSE Menu Framework");
            }
            // Register at PostLoad rather than relying exclusively on PostDataLoad.
            // Safe polling waits for valid camera/player/data objects.
            if (!g_taskInstalled) {
                if (auto* tasks = SFSE::GetTaskInterface()) {
                    tasks->AddPermanentTask(&pollGameThread);
                    g_taskInstalled = true;
                    REX::INFO("VehicleSpeedometer telemetry task registered at PostLoad");
                } else {
                    REX::WARN("VehicleSpeedometer SFSE task interface unavailable at PostLoad");
                }
            }
        }
        if (msg->type == SFSE::MessagingInterface::kPostDataLoad && !g_taskInstalled) {
            if (auto* tasks = SFSE::GetTaskInterface()) {
                tasks->AddPermanentTask(&pollGameThread);
                g_taskInstalled = true;
                REX::INFO("VehicleSpeedometer telemetry task registered at PostDataLoad");
            }
        }
    }
}

SFSE_PLUGIN_LOAD(const SFSE::LoadInterface* sfse)
{
    SFSE::Init(sfse,{.logName = "VehicleSpeedometer"});
    const auto* messaging = SFSE::GetMessagingInterface();
    return messaging && messaging->RegisterListener(onSfseMessage);
}
