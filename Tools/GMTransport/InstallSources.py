from pathlib import Path
import shutil,datetime
root=Path(r'C:\TC\TrinityCore')
task=Path(__file__).parent
backup=Path(r'C:\TC')/('transport-source-backup-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
def edit(relative,fn):
    p=root/relative; data=p.read_bytes(); dest=backup/relative; dest.parent.mkdir(parents=True,exist_ok=True); dest.write_bytes(data) if not dest.exists() else None
    s=data.decode('utf-8',errors='surrogateescape').replace('\r\n','\n'); updated=fn(s).encode('utf-8',errors='surrogateescape');
    if updated!=data:p.write_bytes(updated)
def inject(s,old,new):
    if new in s:return s
    assert old in s,old
    return s.replace(old,new,1)
edit('src/server/game/Globals/ObjectMgr.cpp',lambda s:inject(inject(s,'#include "ObjectMgr.h"','#include "ObjectMgr.h"\n#include "GMTransportRoute.h"'),'return Trinity::Containers::MapGetValuePtr(_gameObjectTemplateStore, entry);','if (auto custom = GMTransport::Template(entry)) return custom;\n    return Trinity::Containers::MapGetValuePtr(_gameObjectTemplateStore, entry);'))
edit('src/server/game/Maps/TransportMgr.h',lambda s:inject(inject(s,'#include "DBCStores.h"','#include "DBCStores.h"\n#include "GMTransportRoute.h"'),'TransportTemplates::const_iterator itr = _transportTemplates.find(entry);','if (auto custom = GMTransport::Path(entry)) return custom;\n            TransportTemplates::const_iterator itr = _transportTemplates.find(entry);'))
edit('src/server/game/Maps/TransportMgr.cpp',lambda s:inject(inject(s,'    uint32 oldMSTime = getMSTime();','    GMTransport::LoadNative();\n    uint32 oldMSTime = getMSTime();'),'    TaxiPathNodeList const& path = sTaxiPathNodesByPath[pathId];','    auto editorPath = GMTransport::NativePath(goInfo->entry);\n    TaxiPathNodeList const& path = editorPath ? *editorPath : sTaxiPathNodesByPath[pathId];'))
def transport_multimap(s):
    if '    GMRoute::Pose editorPose;' not in s:
        s=inject(s,'    if (GetKeyFrames().size() <= 1)','    GMRoute::Pose editorPose;\n    if (GetKeyFrames().size() <= 1)')
    begin=s.index('    GMRoute::Pose editorPose;')
    end=s.index('    if (GetKeyFrames().size() <= 1)',begin)
    s=s[:begin]+'''    GMRoute::Pose editorPose;
    if (GMTransport::Position(GetEntry(), m_goValue.Transport.PathProgress, editorPose))
    {
        if (_delayedTeleport) return;
        GMTransport::CustomAnnouncements(GetEntry(), m_goValue.Transport.PathProgress, diff);
        m_goValue.Transport.PathProgress = (m_goValue.Transport.PathProgress + diff) % GetTransportPeriod();
        GMTransport::Position(GetEntry(), m_goValue.Transport.PathProgress, editorPose);
        SetMoving(!editorPose.stopped);
        if (editorPose.map != GetMapId())
        {
            TeleportTransport(editorPose.map, float(editorPose.p.x), float(editorPose.p.y), float(editorPose.p.z), float(editorPose.facing));
            return;
        }
        UpdatePosition(float(editorPose.p.x), float(editorPose.p.y), float(editorPose.p.z), float(editorPose.facing));
        return;
    }
'''+s[end:]
    begin=s.index('    _delayedTeleport = false;',s.index('void Transport::DelayedTeleportTransport()'))
    end=s.index('    for (_passengerTeleportItr',begin)
    s=s[:begin]+'''    _delayedTeleport = false;
    GMRoute::Pose editorPose;
    bool editor = GMTransport::Position(GetEntry(), m_goValue.Transport.PathProgress, editorPose);
    uint32 destinationMap = editor ? editorPose.map : _nextFrame->Node->ContinentID;
    float x = editor ? float(editorPose.p.x) : _nextFrame->Node->Loc.X,
          y = editor ? float(editorPose.p.y) : _nextFrame->Node->Loc.Y,
          z = editor ? float(editorPose.p.z) : _nextFrame->Node->Loc.Z,
          o = editor ? float(editorPose.facing) : _nextFrame->InitialOrientation;
    Map* newMap = sMapMgr->CreateBaseMap(destinationMap);
    GetMap()->RemoveFromMap<Transport>(this, false);
    SetMap(newMap);

'''+s[end:]
    s=s.replace('TeleportTo(_nextFrame->Node->ContinentID, destX, destY, destZ, destO, TELE_TO_NOT_LEAVE_TRANSPORT)', 'TeleportTo(destinationMap, destX, destY, destZ, destO, TELE_TO_NOT_LEAVE_TRANSPORT)')
    # Log only actual editor transitions; useful to verify two- and three-map loops.
    start=s.index('void Transport::DelayedTeleportTransport()');end=s.index('void Transport::UpdatePassengerPositions',start)
    part=s[start:end]
    marker='    GetMap()->AddToMap<Transport>(this);'
    log='    if (editor) TC_LOG_INFO("entities.transport", "GM route {} transitioned to map {} with {} passengers", GetEntry(), destinationMap, _passengers.size());'
    if log not in part:part=part.replace(marker,marker+'\n'+log)
    return s[:start]+part+s[end:]
edit('src/server/game/Entities/Transport/Transport.cpp',transport_multimap)
edit('src/server/scripts/Custom/custom_script_loader.cpp',lambda s:inject(inject(s,'void AddCustomScripts()','void AddSC_gm_transport();\n\nvoid AddCustomScripts()'),'    AddSC_npc_dalaran_greeter();','    AddSC_gm_transport();\n    AddSC_npc_dalaran_greeter();'))
def copy_changed(source,target):
    if not target.exists() or source.read_bytes()!=target.read_bytes():shutil.copy2(source,target)
for name in ['GMTransportRoute.h','GMTransportRoute.cpp','GMTransportAnnouncements.inc']:
    copy_changed(task/'server'/name,root/'src/server/game/Maps'/name)
copy_changed(task/'shared/NativeRoute.h',root/'src/server/game/Maps/GMNativeRoute.h')
copy_changed(task/'shared/RouteMath.h',root/'src/server/game/Maps/GMRouteMath.h')
copy_changed(task/'server/custom_gm_transport.cpp',root/'src/server/scripts/Custom/custom_gm_transport.cpp')
print('Source backup:',backup)

nativeHeader=root/'src/server/game/Maps/GMNativeRoute.h'
nativeData=nativeHeader.read_text().replace('\"RouteMath.h\"','\"GMRouteMath.h\"')
if nativeHeader.read_text()!=nativeData:nativeHeader.write_text(nativeData)

edit('src/server/game/Entities/Transport/Transport.cpp',lambda s:inject(s,'void Transport::DoEventIfAny(KeyFrame const& node, bool departure)\n{','void Transport::DoEventIfAny(KeyFrame const& node, bool departure)\n{\n    GMTransport::NativeAnnouncement(GetEntry(), node.Node->NodeIndex, departure);'))
def smart_announcements(s):
    s=inject(s,'#include "SmartScript.h"','#include "SmartScript.h"\n#include "GMTransportRoute.h"')
    old='if (GetBaseObject() && GMTransport::SuppressTalk'
    new='if (GetBaseObject() && GetBaseObject()->GetTypeId() == TYPEID_GAMEOBJECT && GMTransport::SuppressTalk'
    if 'GMTransport::SuppressTalk' in s:return s.replace(old,new)
    return inject(s,'        case SMART_ACTION_TALK:\n        {','        case SMART_ACTION_TALK:\n        {\n            '+new+'(GetBaseObject()->GetEntry(), e.entryOrGuid, e.source_type, e.event_id)) break;')
edit('src/server/game/AI/SmartScripts/SmartScript.cpp',smart_announcements)
