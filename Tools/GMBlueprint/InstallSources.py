from pathlib import Path
import datetime,shutil
root=Path(r'C:/TC/TrinityCore')
task=Path(__file__).parent
backup=Path(r'C:/TC')/('blueprint-source-backup-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
for source,relative in [(task/'server/cs_gobject.cpp','src/server/scripts/Commands/cs_gobject.cpp'),(task/'shared/GizmoMath.h','src/server/game/Maps/GMBlueprintMath.h')]:
 target=root/relative
 if target.exists() and source.read_bytes()==target.read_bytes():continue
 if target.exists():
  saved=backup/relative;saved.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(target,saved)
 shutil.copy2(source,target)
print('Source backup:',backup)

def patch(relative,change):
 target=root/relative;before=target.read_bytes();text=before.decode('utf-8').replace('\r\n','\n');after=change(text).encode('utf-8')
 if after!=before:
  saved=backup/relative;saved.parent.mkdir(parents=True,exist_ok=True);saved.write_bytes(before)
  target.write_bytes(after)
def owner_header(s):
 anchor='    virtual float GetOrientation() const = 0;'
 added=anchor+'\n    virtual G3D::Matrix3 GetRotation() const { return G3D::Matrix3::fromEulerAnglesZYX(GetOrientation(), 0, 0); }'
 if added in s:return s
 assert anchor in s;return s.replace(anchor,added,1)
def owner_impl(s):
 anchor='    float GetOrientation() const override { return _owner->GetOrientation(); }'
 added=anchor+"""
    G3D::Matrix3 GetRotation() const override
    {
        // Preserve transport collision behavior; static tilted props use their full orientation.
        QuaternionData q = _owner->GetWorldRotation();
        if (!_owner->GetTransport() && _owner->GetGoType() != GAMEOBJECT_TYPE_MAP_OBJ_TRANSPORT && _owner->GetGoType() != GAMEOBJECT_TYPE_TRANSPORT && (std::abs(q.x) > 1e-6f || std::abs(q.y) > 1e-6f))
            return G3D::Matrix3(G3D::Quat(q.x, q.y, q.z, q.w));
        return GameObjectModelOwnerBase::GetRotation();
    }"""
 if added in s:return s
 assert anchor in s;return s.replace(anchor,added,1)
patch('src/common/Collision/Models/GameObjectModel.h',owner_header)
patch('src/common/Collision/Models/GameObjectModel.cpp',lambda s:s.replace('G3D::Matrix3::fromEulerAnglesZYX(modelOwner->GetOrientation(), 0, 0)','modelOwner->GetRotation()').replace('G3D::Matrix3::fromEulerAnglesZYX(owner->GetOrientation(), 0, 0)','owner->GetRotation()'))
patch('src/server/game/Entities/GameObject/GameObject.cpp',owner_impl)
