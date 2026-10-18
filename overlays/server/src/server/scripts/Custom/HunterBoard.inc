#include "GameObject.h"
#include "GameObjectAI.h"
#include "HunterBoardInput.h"

namespace HunterContracts {
constexpr uint32 BoardEntry=9100102;
constexpr uint32 BoardText=9100102;
constexpr char BoardOption[]="Deposer un contrat de mercenaire";
struct BoardAI : GameObjectAI {
    using GameObjectAI::GameObjectAI;
    bool Allowed(Player* p) const {
        return p && p->IsInWorld() && p->IsAlive() && !p->IsInCombat() && !p->IsInFlight()
            && p->GetGameObjectIfCanInteractWith(me->GetGUID())==me;
    }
    void Show(Player* p,std::string const& status="Choisissez une cible et une prime en pieces d'or.") {
        ClearGossipMenuFor(p);
        // Menu id is a per-opening nonce. A consumed request cannot be replayed
        // against a new menu, even if another contract has since completed.
        p->PlayerTalkClass->GetGossipMenu().SetMenuId(urand(1,0x7fffffff));
        AddGossipItemFor(p,GOSSIP_ICON_CHAT,BoardOption,GOSSIP_SENDER_MAIN,10,"",0,true);
        AddGossipItemFor(p,GOSSIP_ICON_CHAT,status,GOSSIP_SENDER_MAIN,0);
        SendGossipMenuFor(p,BoardText,me->GetGUID());
    }
    bool OnGossipHello(Player* p) override {
        if(!Allowed(p)){CloseGossipMenuFor(p);return true;}
        Show(p);return true;
    }
    bool OnGossipSelectCode(Player* p,uint32 menu,uint32 index,char const* code) override {
        if(!Allowed(p) || menu!=p->PlayerTalkClass->GetGossipMenu().GetMenuId() || GetGossipActionFor(p,index)!=10) {CloseGossipMenuFor(p);return true;}
        // Consume before touching currency or the database.
        ClearGossipMenuFor(p);
        HunterBoardInput::Request request;
        if(!HunterBoardInput::Parse(code?code:"",request)) {Show(p,"Saisissez un nom et une prime entiere de 1 a 100000 pieces d'or.");return true;}
        ChatHandler h(p->GetSession());std::string error;
        bool success=Commands::CreateInternal(&h,request.name,request.gold,false,&error);
        Show(p,success?"Contrat cree. Prime reservee : "+std::to_string(request.gold)+" pieces d'or.":error.empty()?"Impossible de creer ce contrat. Consultez le message dans le chat.":error);
        return true;
    }
    bool OnGossipSelect(Player* p,uint32,uint32) override {CloseGossipMenuFor(p);return true;}
};
class BoardScript : public GameObjectScript {
public:BoardScript():GameObjectScript("go_hunter_contract_board"){}
GameObjectAI* GetAI(GameObject* go) const override {return new BoardAI(go);}
};
bool CheckBoard(ChatHandler* /*h*/) {
    uint32 count=0;bool pass=true;
    for(auto const& pair:sObjectMgr->GetAllGameObjectData()) {
        auto const& data=pair.second;if(data.id!=BoardEntry)continue;++count;
        auto map=sMapMgr->CreateBaseMap(data.mapId);
        map->LoadGrid(data.spawnPoint.GetPositionX(),data.spawnPoint.GetPositionY());
        auto go=map->GetGameObjectBySpawnId(pair.first);
        float floor=map->GetHeight(data.spawnPoint.GetPositionX(),data.spawnPoint.GetPositionY(),data.spawnPoint.GetPositionZ()+3.0f,true,50.0f);
        bool ok=go && dynamic_cast<BoardAI*>(go->AI()) && go->GetGoType()==GAMEOBJECT_TYPE_QUESTGIVER && std::isfinite(floor) && floor>-500;
        pass=pass && ok;
        TC_LOG_INFO("scripts","Hunter board check {}: spawn={} x={} y={} z={} floor={}",ok?"PASS":"FAIL",pair.first,data.spawnPoint.GetPositionX(),data.spawnPoint.GetPositionY(),data.spawnPoint.GetPositionZ(),floor);
    }
    if(count!=1){TC_LOG_ERROR("scripts","Hunter board check FAIL: expected one permanent board, found {}",count);return false;}
    return pass;
}
}
