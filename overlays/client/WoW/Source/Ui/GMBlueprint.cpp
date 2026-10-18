#include "Engine/Source/Tempest/C4Quaternion.h"
#include "Tools/GMBlueprint/shared/GizmoMath.h"
#include "Tools/GMTransport/shared/RouteProjection.h"
#include "GMBlueprint.h"
#include "GMTransport.h"
#include <cmath>
#include <cstring>
#include <string>
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapDoodadDef.h"
#include "WoW/Source/WorldClient/MapObjDef.h"
#include "WoW/Source/WorldClient/World.h"

namespace {
constexpr float MaxDistance = 100.0f;
constexpr float TwoPi = 6.28318530718f;
struct Preview {
    CMapBaseObj* object = nullptr;
    int32_t entry = 0;
    int32_t map = -1;
    C3Vector ground = {0.0f, 0.0f, 0.0f};
    GMGizmo::Quat rotation;
    float facing = 0.0f;
    float targetFacing = 0.0f;
    float height = 0.0f;
    float scale = 1.0f;
    bool pending = false;
    bool valid = false;
    bool wmo = false;
    std::string name;
    std::string error;
} preview;
GMGizmo::Drag gizmo;
GMGizmo::RotateDrag rotationDrag;
bool controlsEnabled = false;
bool heightUp = false, heightDown = false, anchorLocked = false;
float anchorX = 0.0f, anchorY = 0.0f;

bool ShiftHeld() { return EventIsKeyDown(KEY_LSHIFT) || EventIsKeyDown(KEY_RSHIFT); }
bool FineHeld() { return EventIsKeyDown(KEY_LCONTROL) || EventIsKeyDown(KEY_RCONTROL); }
void LockAnchor() {
    EventInputGetMousePosition(&anchorX, &anchorY);
    anchorLocked = true;
}
int32_t WheelInput(const void* data, void*) {
    if (!controlsEnabled || !preview.object || !CGGameUI::s_inWorld || !ShiftHeld()) return 1;
    const int32_t wheel = static_cast<const EVENT_DATA_MOUSE*>(data)->m_wheelDistance;
    if(gizmo.axis || rotationDrag.axis) return 0;
    if (wheel) {
        LockAnchor();
        const float step = (FineHeld() ? 0.1f : 1.0f) * TwoPi / 360.0f;
        preview.targetFacing = std::fmod(preview.targetFacing + (wheel > 0 ? step : -step) + TwoPi, TwoPi);
    }
    return 0; // Consume only the editor gesture; do not zoom the camera.
}
int32_t KeyDownInput(const void* data, void*) {
    const KEY key = static_cast<const EVENT_DATA_KEY*>(data)->m_key;
    if (key != KEY_UP && key != KEY_DOWN) return 1;
    if (!controlsEnabled || !preview.object || !CGGameUI::s_inWorld || !ShiftHeld()) return 1;
    if(gizmo.axis || rotationDrag.axis) return 0;
    if (key == KEY_UP) heightUp = true;
    else heightDown = true;
    LockAnchor();
    return 0;
}
int32_t KeyUpInput(const void* data, void*) {
    const KEY key = static_cast<const EVENT_DATA_KEY*>(data)->m_key;
    if (key == KEY_UP && heightUp) { heightUp = false; return 0; }
    if (key == KEY_DOWN && heightDown) { heightDown = false; return 0; }
    return 1;
}
int32_t FocusInput(const void*, void*) {
    heightUp = heightDown = false;
    gizmo = {};rotationDrag = {};
    return 1;
}
int32_t Controls(lua_State* L) {
    controlsEnabled = lua_toboolean(L, 1) != 0;
    if (!controlsEnabled) {heightUp = heightDown = false;gizmo = {};rotationDrag = {};}
    return 0;
}

CGPlayer_C* Player() {
    if (!CGGameUI::s_inWorld)
        return nullptr;
    return static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(
        ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));
}

bool Finite(const C3Vector& p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}

C3Vector Position() {
    return C3Vector(preview.ground.x, preview.ground.y, preview.ground.z + preview.height);
}

bool InRange(CGPlayer_C* player, const C3Vector& position) {
    const C3Vector delta = position - player->GetPosition();
    return Finite(position) && delta.x * delta.x + delta.y * delta.y + delta.z * delta.z <= MaxDistance * MaxDistance;
}

void UpdateTransform() {
    if (!preview.object)
        return;
    C44Matrix matrix;
    matrix.Identity();
    C4Quaternion q;q.x=float(preview.rotation.x);q.y=float(preview.rotation.y);q.z=float(preview.rotation.z);q.w=float(preview.rotation.w);
    matrix.Rotate(q);
    matrix.Scale(preview.scale);
    const C3Vector p = Position();
    matrix.d0 = p.x;
    matrix.d1 = p.y;
    matrix.d2 = p.z;
    if (preview.wmo) {
        CMapObjDef::UpdateMatrix(static_cast<CMapObjDef*>(preview.object), matrix);
    } else {
        CMapDoodadDef* doodad = static_cast<CMapDoodadDef*>(preview.object);
        CMap::UpdateDoodadDefTransform(doodad, matrix);
        if (doodad->m_model) {
            doodad->m_model->m_baseAlpha = 0.45f;
            doodad->m_model->m_diffuseTintR = preview.valid ? 0.35f : 1.0f;
            doodad->m_model->m_diffuseTintG = preview.valid ? 1.0f : 0.25f;
            doodad->m_model->m_diffuseTintB = preview.valid ? 1.0f : 0.25f;
        }
    }
    World::ObjectEnableCollision(preview.object, 0);
}

void ResolveStats(GameObjectStats_C* stats) {
    preview.pending = false;
    if (!stats || !Player() || World::QueryMapId() != preview.map) {
        preview.error = "Objet introuvable ou personnage hors du monde.";
        return;
    }
    GameObjectDisplayInfoRec* display = g_gameObjectDisplayInfoDB.GetRecord(stats->m_displayID);
    if (!display || !display->m_modelName || !display->m_modelName[0] ||
        !std::isfinite(stats->m_scale) || stats->m_scale <= 0.0f) {
        preview.error = "Cet objet ne possede pas de modele utilisable.";
        return;
    }
    preview.name = stats->GetName(0) ? stats->GetName(0) : "GameObject";
    preview.scale = stats->m_scale;
    const char* ext = std::strrchr(display->m_modelName, '.');
    preview.wmo = ext && _stricmp(ext, ".wmo") == 0;
    const int16_t doodadSet = 0;
    float radius = 0.0f;
    preview.object = CWorld::ObjectCreate(display->m_modelName, Position(), preview.facing,
        0, 0, 0, 1, &doodadSet, &radius, 0);
    if (!preview.object) {
        preview.error = "Impossible de charger le modele.";
        return;
    }
    if (!preview.wmo && !static_cast<CMapDoodadDef*>(preview.object)->m_model) {
        CWorld::ObjectDestroy(preview.object);
        preview.object = nullptr;
        preview.error = "Modele absent des fichiers du client.";
        return;
    }
    World::ObjectEnableCollision(preview.object, 0);
    UpdateTransform();
}

void StatsReady(int32_t entry, const WOWGUID&, void*, bool success) {
    if (!preview.pending || preview.entry != entry)
        return;
    ResolveStats(success ? g_gameObjectDBCache.GetRecord(entry, WOWGUID(), nullptr, nullptr, false) : nullptr);
}

int32_t Begin(lua_State* L) {
    const double entry = luaL_checknumber(L, 1);
    if (!std::isfinite(entry) || entry < 1 || entry > 2147483647.0 || std::floor(entry) != entry)
        return luaL_error(L, "GMBlueprintBegin: invalid GameObject entry");
    GMBlueprintCancel();
    CGPlayer_C* player = Player();
    if (!player) {
        lua_pushboolean(L, false);
        return 1;
    }
    preview.entry = static_cast<int32_t>(entry);
    preview.map = World::QueryMapId();
    preview.ground = player->GetPosition();
    preview.facing = player->GetFacing();
    preview.targetFacing = preview.facing;
    preview.rotation=GMGizmo::Rotation(3,preview.facing);
    preview.pending = true;
    // One callback per request; Cancel removes it before replacing the preview.
    GameObjectStats_C* stats = g_gameObjectDBCache.GetRecord(preview.entry, WOWGUID(), StatsReady, nullptr, true);
    if (stats)
        ResolveStats(stats);
    lua_pushboolean(L, true);
    return 1;
}

bool CursorRay(GMGizmo::Ray& ray) {
    auto frame=CGWorldFrame::s_currentWorldFrame;auto device=g_theGxDevicePtr;
    if(!frame || !frame->m_camera || !device) return false;
    float x,y;EventInputGetMousePosition(&x,&y);C3Vector a,b;
    C44Matrix savedP=device->m_projection,savedV=device->m_xforms[GxXform_View].m_mtx[device->m_xforms[GxXform_View].m_level];
    frame->m_camera->SetupWorldProjection(frame->m_viewportDDC);
    bool valid=frame->GetLineSegment(x,y,&a,&b)!=0;
    device->XformSetProjection(savedP);device->XformSetView(savedV);
    if(!valid || !Finite(a) || !Finite(b)) return false;
    ray={{a.x,a.y,a.z},{b.x-a.x,b.y-a.y,b.z-a.z}};return true;
}
void EndGizmo() {gizmo={};rotationDrag={};LockAnchor();}
int32_t GizmoDrag(lua_State* L) {
    int axis=int(luaL_checkinteger(L,1));
    if(!axis) {EndGizmo();lua_pushboolean(L,1);return 1;}
    auto player=Player();GMGizmo::Ray ray;auto p=Position();
    bool valid=controlsEnabled && player && preview.object && World::ObjectIsLoaded(preview.object) && preview.map==World::QueryMapId() && CursorRay(ray);
    gizmo={};rotationDrag={};
    if(valid) valid=axis>=8 && axis<=10?rotationDrag.Begin({p.x,p.y,p.z},axis-7,preview.rotation,ray):gizmo.Begin({p.x,p.y,p.z},axis,ray);
    if(valid) {preview.ground=p;preview.height=0;preview.valid=InRange(player,p);heightUp=heightDown=false;preview.targetFacing=preview.facing;LockAnchor();}
    lua_pushboolean(L,valid);return 1;
}
int32_t GizmoProjection(lua_State* L) {
    auto frame=CGWorldFrame::s_currentWorldFrame;auto device=g_theGxDevicePtr;
    if(!controlsEnabled || !preview.object || !World::ObjectIsLoaded(preview.object) || !frame || !frame->m_camera || !device || !Player()) return 0;
    auto p=Position();auto c=frame->m_camera->m_position;
    GMRoute::Vec origin{p.x,p.y,p.z},camera{c.x,c.y,c.z};
    double size=std::fmax(0.75,std::fmin(12.0,GMRoute::Length(origin-camera)*0.13));
    C44Matrix savedP=device->m_projection,savedV=device->m_xforms[GxXform_View].m_mtx[device->m_xforms[GxXform_View].m_level];
    frame->m_camera->SetupWorldProjection(frame->m_viewportDDC);
    C44Matrix matrix=device->m_xforms[GxXform_View].m_mtx[device->m_xforms[GxXform_View].m_level]*device->m_projection;
    device->XformSetProjection(savedP);device->XformSetView(savedV);
    double x,y;if(!GMRoute::Project(matrix,origin,camera,x,y)) return 0;
    lua_newtable(L);lua_pushnumber(L,x);lua_rawseti(L,-2,1);lua_pushnumber(L,y);lua_rawseti(L,-2,2);
    for(int axis=1;axis<=3;++axis) {
        auto tip=origin;if(axis==1)tip.x+=size;else if(axis==2)tip.y+=size;else tip.z+=size;
        if(GMRoute::Project(matrix,tip,camera,x,y)) {lua_newtable(L);lua_pushnumber(L,x);lua_rawseti(L,-2,1);lua_pushnumber(L,y);lua_rawseti(L,-2,2);lua_rawseti(L,-2,axis+2);}
    }
    auto projectPoint=[&](GMRoute::Vec point) {
        double px,py;if(!GMRoute::Project(matrix,point,camera,px,py)) {lua_pushboolean(L,0);return;}
        lua_newtable(L);lua_pushnumber(L,px);lua_rawseti(L,-2,1);lua_pushnumber(L,py);lua_rawseti(L,-2,2);
    };
    if(luaL_optinteger(L,1,0)==0) {
        for(int plane=4;plane<=6;++plane) {
            GMRoute::Vec a{},b{};if(plane==4){a.x=1;b.y=1;}else if(plane==5){a.x=1;b.z=1;}else{a.y=1;b.z=1;}
            auto point=[&](double u,double v){return origin+a*(u*size)+b*(v*size);};
            double px,py;if(!GMRoute::Project(matrix,point(0.35,0.35),camera,px,py)) continue;
            lua_newtable(L);lua_pushnumber(L,px);lua_rawseti(L,-2,1);lua_pushnumber(L,py);lua_rawseti(L,-2,2);
            projectPoint(point(0.22,0.22));lua_rawseti(L,-2,3);projectPoint(point(0.48,0.22));lua_rawseti(L,-2,4);
            projectPoint(point(0.48,0.48));lua_rawseti(L,-2,5);projectPoint(point(0.22,0.48));lua_rawseti(L,-2,6);
            lua_rawseti(L,-2,plane+2);
        }
        projectPoint(origin);lua_rawseti(L,-2,9);
    } else {
        lua_newtable(L);
        for(int axis=1;axis<=3;++axis) {
            lua_newtable(L);
            for(int i=0;i<=64;++i) {auto unit=GMGizmo::RingPoint(axis,TwoPi*i/64);projectPoint(origin+GMRoute::Vec{unit.x,unit.y,unit.z}*(size*0.85));lua_rawseti(L,-2,i+1);}
            lua_rawseti(L,-2,axis);
        }
        lua_rawseti(L,-2,10);
    }
    lua_pushinteger(L,rotationDrag.axis?rotationDrag.axis+7:gizmo.axis);return 2;
}

int32_t Tick(lua_State* L) {
    if (!preview.entry)
        return 0;
    CGPlayer_C* player = Player();
    if (!player || World::QueryMapId() != preview.map) {
        GMBlueprintCancel();
        return 0;
    }
    float elapsed = static_cast<float>(luaL_optnumber(L, 2, 0.0));
    elapsed = std::isfinite(elapsed) ? std::fmax(0.0f, std::fmin(0.1f, elapsed)) : 0.0f;
    if(gizmo.axis || rotationDrag.axis) {
        if(!EventIsButtonDown(MOUSE_BUTTON_LEFT)) EndGizmo();
        else {GMGizmo::Ray ray;GMGizmo::Vec p;
            if(gizmo.axis && CursorRay(ray) && gizmo.Update(ray,FineHeld(),p) && std::abs(p.x)<=17000 && std::abs(p.y)<=17000 && std::abs(p.z)<=10000) {
                preview.ground=C3Vector(float(p.x),float(p.y),float(p.z));preview.height=0;preview.valid=InRange(player,Position());
            }
            GMGizmo::Quat q;
            if(rotationDrag.axis && CursorRay(ray) && rotationDrag.Update(ray,FineHeld(),q)) {
                preview.rotation=q;preview.facing=float(GMGizmo::Yaw(q));preview.targetFacing=preview.facing;
            }
        }
    }
    if (controlsEnabled && preview.object) {
        const float previousFacing=preview.facing;
        const float delta = std::remainder(preview.targetFacing - preview.facing, TwoPi);
        preview.facing = std::fmod(preview.facing + delta * (1.0f - std::exp(-25.0f * elapsed)) + TwoPi, TwoPi);
        if (std::fabs(delta) < 0.00001f) preview.facing = preview.targetFacing;
        preview.rotation=GMGizmo::Multiply(GMGizmo::Rotation(3,std::remainder(preview.facing-previousFacing,TwoPi)),preview.rotation);
        if (!gizmo.axis && !rotationDrag.axis && ShiftHeld() && (heightUp || heightDown)) {
            LockAnchor();
            const float speed = FineHeld() ? 0.1f : 1.0f;
            preview.height = std::fmax(-50.0f, std::fmin(50.0f, preview.height + (int(heightUp) - int(heightDown)) * speed * elapsed));
        }
    }
    if (anchorLocked) {
        float x = anchorX, y = anchorY;
        EventInputGetMousePosition(&x, &y);
        if (std::fabs(x - anchorX) + std::fabs(y - anchorY) > 0.00001f) anchorLocked = false;
    }
    if (!gizmo.axis && !rotationDrag.axis && lua_toboolean(L, 1) && !anchorLocked && preview.object && CGWorldFrame::s_currentWorldFrame) {
        float x = -1.0f, y = -1.0f;
        EventInputGetMousePosition(&x, &y);
        C3Vector start = {0.0f, 0.0f, 0.0f};
        C3Vector end = start, hit = start;
        float distance = 1.0f;
        preview.valid = false;
        // Lua OnUpdate can run with UI projection matrices active. Match the
        // camera setup/restoration in CGWorldFrame::HitTestPoint before unprojecting.
        CGWorldFrame* frame = CGWorldFrame::s_currentWorldFrame;
        CGxDevice* device = g_theGxDevicePtr;
        const C44Matrix savedProjection = device->m_projection;
        const C44Matrix savedView = device->m_xforms[GxXform_View].m_mtx[device->m_xforms[GxXform_View].m_level];
        frame->m_camera->SetupWorldProjection(frame->m_viewportDDC);
        const bool hasRay = frame->GetLineSegment(x, y, &start, &end) != 0;
        device->XformSetProjection(savedProjection);
        device->XformSetView(savedView);
        if (hasRay && Finite(start) && Finite(end) &&
            World::Intersect(start, end, &hit, &distance, 0x100151, nullptr) &&
            std::isfinite(distance) && distance >= 0.0f && distance <= 1.0f && Finite(hit)) {
            preview.ground = hit;
            preview.valid = InRange(player, Position());
        }
    } else if (preview.valid) {
        preview.valid = InRange(player, Position());
    }
    UpdateTransform();
    return 0;
}

int32_t Adjust(lua_State* L) {
    float angle = static_cast<float>(luaL_checknumber(L, 1));
    float height = static_cast<float>(luaL_checknumber(L, 2));
    if (!std::isfinite(angle) || !std::isfinite(height))
        return luaL_error(L, "GMBlueprintAdjust: finite values required");
    if (preview.object) {
        preview.rotation=GMGizmo::Multiply(GMGizmo::Rotation(3,angle),preview.rotation);
        preview.facing = float(GMGizmo::Yaw(preview.rotation));
        preview.targetFacing = preview.facing;
        preview.height = std::fmax(-50.0f, std::fmin(50.0f, preview.height + height));
        if (CGPlayer_C* player = Player())
            preview.valid = preview.valid && InRange(player, Position());
        UpdateTransform();
    }
    return 0;
}

int32_t State(lua_State* L) {
    const bool loaded = preview.object && World::ObjectIsLoaded(preview.object);
    const C3Vector p = Position();
    lua_pushinteger(L, preview.entry);
    lua_pushboolean(L, loaded && preview.valid);
    lua_pushboolean(L, preview.pending || (preview.object && !loaded));
    lua_pushinteger(L, preview.map);
    lua_pushnumber(L, p.x);
    lua_pushnumber(L, p.y);
    lua_pushnumber(L, p.z);
    lua_pushnumber(L, preview.facing);
    lua_pushstring(L, preview.name.c_str());
    lua_pushstring(L, preview.error.c_str());
    lua_pushboolean(L, preview.wmo);
    lua_pushnumber(L,preview.rotation.x);lua_pushnumber(L,preview.rotation.y);lua_pushnumber(L,preview.rotation.z);lua_pushnumber(L,preview.rotation.w);
    return 15;
}

int32_t Cancel(lua_State*) {
    GMBlueprintCancel();
    return 0;
}
} // namespace

void GMBlueprintCancel() {
    GMTransportClearPreview();
    controlsEnabled = heightUp = heightDown = anchorLocked = false;
    gizmo = {};rotationDrag = {};
    if (preview.pending)
        g_gameObjectDBCache.CancelCallback(preview.entry, StatsReady, nullptr);
    if (preview.object)
        CWorld::ObjectDestroy(preview.object);
    preview = Preview();
}

void GMBlueprintRegisterScriptFunctions() {
    GMTransportRegister();
    FrameScript_RegisterFunction("GMBlueprintControls", Controls);
    FrameScript_RegisterFunction("GMBlueprintGizmo", GizmoProjection);
    FrameScript_RegisterFunction("GMBlueprintGizmoDrag", GizmoDrag);
    EventRegisterEx(EVENT_ID_MOUSEWHEEL, WheelInput, nullptr, 2.0f);
    EventRegisterEx(EVENT_ID_KEYDOWN, KeyDownInput, nullptr, 2.0f);
    EventRegisterEx(EVENT_ID_KEYUP, KeyUpInput, nullptr, 2.0f);
    EventRegisterEx(EVENT_ID_FOCUS, FocusInput, nullptr, 2.0f);
    FrameScript_RegisterFunction("GMBlueprintBegin", Begin);
    FrameScript_RegisterFunction("GMBlueprintTick", Tick);
    FrameScript_RegisterFunction("GMBlueprintAdjust", Adjust);
    FrameScript_RegisterFunction("GMBlueprintState", State);
    FrameScript_RegisterFunction("GMBlueprintCancel", Cancel);
}

void GMBlueprintUnregisterScriptFunctions() {
    GMTransportUnregister();
    GMBlueprintCancel();
    FrameScript_UnregisterFunction("GMBlueprintControls");
    FrameScript_UnregisterFunction("GMBlueprintGizmo");
    FrameScript_UnregisterFunction("GMBlueprintGizmoDrag");
    EventUnregisterEx(EVENT_ID_MOUSEWHEEL, WheelInput, nullptr, 0xFFFFFFFF);
    EventUnregisterEx(EVENT_ID_KEYDOWN, KeyDownInput, nullptr, 0xFFFFFFFF);
    EventUnregisterEx(EVENT_ID_KEYUP, KeyUpInput, nullptr, 0xFFFFFFFF);
    EventUnregisterEx(EVENT_ID_FOCUS, FocusInput, nullptr, 0xFFFFFFFF);
    FrameScript_UnregisterFunction("GMBlueprintBegin");
    FrameScript_UnregisterFunction("GMBlueprintTick");
    FrameScript_UnregisterFunction("GMBlueprintAdjust");
    FrameScript_UnregisterFunction("GMBlueprintState");
    FrameScript_UnregisterFunction("GMBlueprintCancel");
}
