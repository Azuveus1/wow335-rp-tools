"""Add civil-registry directions to the existing Stormwind guard menus.

Uses the stock Visitor's Center reply (menu 383 / text 882), already loaded
and translated by the client, and adds an explicit destination and map flag.
No replacement of guard AI, existing options, or NPC text is required.
"""
from pathlib import Path
import sys,json,datetime
from Prepare import db,switch
switch('World')
menus=(435,3506);option=50;poi=9100300
english="Visitor's Center / Civil Registry"
french="Centre d'accueil / Registre civil"
def sqltext(s):return "'"+s.replace('\\','\\\\').replace("'","''")+"'"
def snapshot():
 return {name:db.query(query) for name,query in {
  'options':f'SELECT * FROM gossip_menu_option WHERE MenuID IN (435,3506) AND OptionID={option}',
  'option_locales':f'SELECT * FROM gossip_menu_option_locale WHERE MenuID IN (435,3506) AND OptionID={option}',
  'poi':f'SELECT * FROM points_of_interest WHERE ID={poi}',
  'poi_locales':f'SELECT * FROM points_of_interest_locale WHERE ID={poi}'}.items()}
if __name__=='__main__':
 before=db.query('SELECT * FROM gossip_menu_option WHERE MenuID IN (435,3506) AND OptionID<>50 ORDER BY MenuID,OptionID')
 assert db.query('SELECT TextID FROM gossip_menu WHERE MenuID=383')=='882','Stock Visitor Center response changed'
 for menu in menus:
  existing=db.query(f'SELECT OptionText FROM gossip_menu_option WHERE MenuID={menu} AND OptionID={option}')
  if existing and existing!=english:raise RuntimeError('Reserved guard option is already in use')
 existing=db.query(f'SELECT Name FROM points_of_interest WHERE ID={poi}')
 if existing and existing!=english:raise RuntimeError('Reserved map point is already in use')
 if '--apply' in sys.argv:
  backup=Path(__file__).resolve().parents[2]/'build'/('civil-registry-guard-backup-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S')+'.json')
  backup.write_text(json.dumps(snapshot(),ensure_ascii=False,indent=2),encoding='utf-8')
  commands=[f"REPLACE INTO points_of_interest(ID,PositionX,PositionY,Icon,Flags,Importance,Name,VerifiedBuild) VALUES ({poi},-8882.9,614.9,7,99,0,{sqltext(english)},0)",
   f"REPLACE INTO points_of_interest_locale(ID,locale,Name,VerifiedBuild) VALUES ({poi},'frFR',{sqltext(french)},0)"]
  for menu in menus:
   commands.extend([f"REPLACE INTO gossip_menu_option(MenuID,OptionID,OptionIcon,OptionText,OptionBroadcastTextID,OptionType,OptionNpcFlag,ActionMenuID,ActionPoiID) VALUES ({menu},{option},0,{sqltext(english)},0,1,1,383,{poi})",
    f"REPLACE INTO gossip_menu_option_locale(MenuID,OptionID,Locale,OptionText) VALUES ({menu},{option},'frFR',{sqltext(french)})"])
  db.query('START TRANSACTION;'+';'.join(commands)+';COMMIT;')
  assert before==db.query('SELECT * FROM gossip_menu_option WHERE MenuID IN (435,3506) AND OptionID<>50 ORDER BY MenuID,OptionID'),'Existing directions changed'
  assert db.query(f'SELECT COUNT(*) FROM gossip_menu_option WHERE MenuID IN (435,3506) AND OptionID=50 AND ActionMenuID=383 AND ActionPoiID={poi}')=='2'
  print('PASS: explicit FR/EN direction in both Stormwind guard menus; map point at registrar; stock options preserved. Backup:',backup)
 else:print(json.dumps(snapshot(),ensure_ascii=False,indent=2))
