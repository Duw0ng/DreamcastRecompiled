"""Self-contained standalone VMU home menu, virtual pet, two original minigames.

This is an Aqua app, *not* an emulated VMU MCU. It never writes the game's
flash .bin nor .lcd transport; persistent settings live in launcher config.
"""
from __future__ import annotations
from pathlib import Path
import datetime as dt
import json
import os
import time
from dataclasses import dataclass, field
from vmu_pixels import LCD
from vmu_inspector import read_vmu

HOME=('RELOJ','ARCHIVOS','AQUAPET','JUEGOS','ESTADO')
PET_MENU=('ALIMENTAR','JUGAR','LIMPIAR','DORMIR','ESTADO','OBJETOS')
GAMES=('ATRAPA','MEMORIA')
DIRECTIONS=('UP','RIGHT','DOWN','LEFT')


def clamp(x):return max(0,min(100,int(x)))


def initial_pet(now:float|None=None):
    return {'name':'AQUAPET','full':70,'happy':70,'clean':80,'energy':80,
            'level':1,'xp':0,'launches':0,'items':[],
            'last_tick':float(time.time() if now is None else now),
            'seen_history':[], 'history_initialized':False}


class PetStore:
    def __init__(self,path:Path):
        self.path=Path(path)
        self.invalid=False
        self.pet=self._load()

    def _load(self):
        if not self.path.exists():return initial_pet()
        try:
            if self.path.stat().st_size>64*1024:raise ValueError('Archivo de mascota demasiado grande')
            data=json.loads(self.path.read_text(encoding='utf-8'))
            if not isinstance(data,dict) or data.get('format')!=1 or not isinstance(data.get('pet'),dict):
                raise ValueError('Formato desconocido')
            out=initial_pet()
            p=data['pet']
            for key in ('full','happy','clean','energy'):
                if type(p.get(key)) is int:out[key]=clamp(p[key])
            for key in ('xp','level','launches'):
                if type(p.get(key)) is int:out[key]=max(0,min(p[key],1_000_000))
            if isinstance(p.get('items'),list):
                out['items']=list(dict.fromkeys(str(x)[:30] for x in p['items'][:50]))
            if isinstance(p.get('seen_history'),list):
                out['seen_history']=[str(s)[:400] for s in p['seen_history'][-250:]]
            out['history_initialized']=p.get('history_initialized') is True
            if isinstance(p.get('last_tick'),(int,float)):
                out['last_tick']=max(0,min(float(p['last_tick']),time.time()+60))
            return out
        except (OSError,ValueError,TypeError,OverflowError):
            # A corrupted file must not be silently overwritten.
            self.invalid=True
            return initial_pet()

    def save(self):
        if self.invalid:return False
        self.path.parent.mkdir(parents=True,exist_ok=True)
        temp=self.path.with_suffix('.json.tmp')
        try:
            with temp.open('w',encoding='utf-8') as f:
                json.dump({'format':1,'pet':self.pet},f,ensure_ascii=False,indent=2)
                f.flush();os.fsync(f.fileno())
            os.replace(temp,self.path)
            return True
        except OSError:
            try:temp.unlink(missing_ok=True)
            except OSError:pass
            return False

    def elapsed(self,now:float|None=None):
        now=time.time() if now is None else now
        hours=max(0,min(24,int((now-self.pet['last_tick'])//3600)))
        if not hours:return False
        self.pet['full']=clamp(self.pet['full']-hours)
        self.pet['happy']=clamp(self.pet['happy']-hours//3)
        self.pet['clean']=clamp(self.pet['clean']-hours//4)
        self.pet['energy']=clamp(self.pet['energy']+hours//2)
        self.pet['last_tick']=now
        self.save();return True

    def reward(self,score:int):
        p=self.pet
        p['xp']+=int(score)
        p['level']=1+p['xp']//60
        p['happy']=clamp(p['happy']+8)
        self.save()

    def action(self,name):
        p=self.pet
        if name=='ALIMENTAR':p['full']=clamp(p['full']+20);p['clean']=clamp(p['clean']-3)
        if name=='LIMPIAR':p['clean']=clamp(p['clean']+30)
        if name=='DORMIR':p['energy']=clamp(p['energy']+25)
        self.save()

    def track_launcher_history(self,settings_path:Path):
        """Only recorded launches, never inferred gameplay time or achievements."""
        if not settings_path.is_file():
            if not self.pet['history_initialized']:
                self.pet['history_initialized']=True
                self.pet['seen_history']=[]
                self.save()
            return 0
        try:
            if settings_path.stat().st_size>2_000_000:return 0
            settings=json.loads(settings_path.read_text(encoding='utf-8'))
            history=settings.get('history',[])
            if not isinstance(history,list):return 0
            valid=[x for x in history[-250:] if isinstance(x,dict) and x.get('mode')=='play'
                   and isinstance(x.get('game'),str) and isinstance(x.get('when'),str)]
        except (OSError,ValueError,TypeError):return 0
        signatures=[x['when']+'|'+x['game'] for x in valid]
        p=self.pet
        if not p['history_initialized']:
            p['history_initialized']=True
            p['seen_history']=signatures[-250:]
            self.save()
            return 0
        previous=set(p['seen_history'])
        fresh=[x for x,sig in zip(valid,signatures) if sig not in previous]
        for item in fresh:
            game=Path(item['game']).stem.upper()
            if 'CHUCHU' in game:gift='QUESITO'
            elif 'SONIC' in game:gift='ZAPATILLAS'
            elif 'TETRIS' in game:gift='CUBO'
            elif 'RESIDENT EVIL' in game:gift='LINTERNA'
            else:gift='ESTRELLA'
            if gift not in p['items']:p['items'].append(gift)
            p['launches']+=1;p['xp']+=5;p['happy']=clamp(p['happy']+2)
        if fresh:p['level']=1+p['xp']//60
        p['seen_history']=signatures[-250:]
        if fresh or len(previous)!=len(signatures):self.save()
        return len(fresh)


class VMUShell:
    """No graphical toolkit dependency; deterministic screens and button events."""
    def __init__(self,store:PetStore):
        self.store=store
        self.power=False
        self.page='home';self.index=0;self.detail=None
        self.vmu_path:Path|None=None
        self.notes=''
        self.frame_count=0
        self.game='';self.score=0;self.player=2;self.target=2;self.drop_y=4
        self.sequence=[];self.seq_position=0;self.memory_reveal=0
        self._last_menu='home'

    def turn_on(self):
        self.power=True;self.page='home';self.index=0;self.notes='HOLA!'

    def turn_off(self):self.power=False

    def set_vmu(self,path:Path|None):
        self.vmu_path=Path(path) if path is not None else None

    def files(self):
        if not self.vmu_path:return None
        try:return read_vmu(self.vmu_path)
        except (OSError,ValueError):return None

    def _menu(self):
        if self.page=='home':return HOME
        if self.page=='pet':return PET_MENU
        if self.page=='games':return GAMES
        return ()

    def press(self,key:str):
        key=key.upper()
        if key=='SLEEP':
            self.turn_off() if self.power else self.turn_on();return
        if not self.power:return
        if key=='MODE':self.page='home';self.index=0;self.notes='';return
        if key=='B':
            if self.page in ('home',):self.turn_off()
            elif self.page in ('catch','memory'):self.page='games';self.index=0
            elif self.page=='file_detail':self.page='files'
            elif self.page in ('pet_stats','pet_items'):self.page='pet';self.index=0
            else:self.page='home';self.index=0
            self.notes='';return
        if self.page=='catch':
            if key=='LEFT':self.player=max(0,self.player-1)
            elif key=='RIGHT':self.player=min(4,self.player+1)
            elif key=='A':
                if self.drop_y>=20 and self.player==self.target:
                    self.score+=1;self.target=(self.target+2+self.score)%5;self.drop_y=5
                    if self.score==3:
                        self.store.reward(18);self.page='game_result';self.notes='GANASTE +18'
                else:self.notes='FALLASTE'
            return
        if self.page=='memory':
            if key not in DIRECTIONS:return
            if self.memory_reveal>0:return
            if key==self.sequence[self.seq_position]:
                self.seq_position+=1
                if self.seq_position==len(self.sequence):
                    self.store.reward(15);self.page='game_result';self.notes='GANASTE +15'
            else:self.page='game_result';self.notes='OTRA VEZ'
            return
        if self.page=='game_result':
            if key=='A':self.page='games';self.index=0
            return
        if self.page=='clock' or self.page=='status':return
        if self.page=='pet_stats':
            if key in ('LEFT','UP'):self.index=(self.index-1)%4
            if key in ('RIGHT','DOWN'):self.index=(self.index+1)%4
            return
        if self.page=='pet_items':
            if key in ('LEFT','UP'):self.index=max(0,self.index-1)
            if key in ('RIGHT','DOWN'):self.index=min(max(0,len(self.store.pet['items'])-1),self.index+1)
            return
        if self.page in ('files','file_detail'):
            report=self.files()
            entries=report.entries if report else []
            if self.page=='files':
                if key=='DOWN':self.index=min(max(0,len(entries)-1),self.index+1)
                if key=='UP':self.index=max(0,self.index-1)
                if key=='A' and entries:self.page='file_detail'
            return
        menu=self._menu()
        if key in ('RIGHT','DOWN'):self.index=(self.index+1)%len(menu)
        elif key in ('LEFT','UP'):self.index=(self.index-1)%len(menu)
        elif key=='A':
            name=menu[self.index]
            if self.page=='home':
                self.page={'RELOJ':'clock','ARCHIVOS':'files','AQUAPET':'pet',
                           'JUEGOS':'games','ESTADO':'status'}[name]
                self.index=0
            elif self.page=='pet':
                if name=='JUGAR':self.page='games';self.index=0
                elif name=='ESTADO':self.page='pet_stats';self.index=0
                elif name=='OBJETOS':self.page='pet_items';self.index=0
                else:self.store.action(name);self.notes='OK '+name
            elif self.page=='games':
                self.game=name;self.score=0;self.player=2;self.target=1
                self.drop_y=5;self.notes=''
                if name=='ATRAPA':self.page='catch'
                elif name=='MEMORIA':
                    self.page='memory';self.memory_reveal=13;self.seq_position=0
                    # Deterministic per attempt, no global random/time dependency.
                    seed=(self.store.pet['xp']+self.frame_count)//5
                    self.sequence=[DIRECTIONS[(seed+i*7+i*i*2)%4] for i in range(3)]

    def tick(self):
        self.frame_count+=1
        if self.page=='catch':
            if self.frame_count%3==0:self.drop_y+=1
            if self.drop_y>26:
                self.drop_y=5;self.target=(self.target+3)%5
        if self.page=='memory' and self.memory_reveal>0:
            self.memory_reveal-=1
        if self.frame_count%240==0:self.store.elapsed()

    def render(self,now=None)->bytes:
        l=LCD()
        if not self.power:return l.bitmap()
        p=self.store.pet
        if self.page=='home':
            l.text(24,1,'AQUA VMU',center=True)
            l.rect(0,8,48,17)
            l.text(24,12,HOME[self.index],center=True)
            l.text(24,27,'< A:OK >',center=True)
        elif self.page=='clock':
            now=now or dt.datetime.now()
            l.text(24,2,'RELOJ',center=True)
            l.text(24,13,now.strftime('%H:%M:%S'),center=True)
            l.text(24,24,now.strftime('%d/%m/%y'),center=True)
        elif self.page=='pet':
            l.text(24,0,'AQUAPET',center=True)
            l.pet(x=14,y=6,eyes=self.frame_count%12!=0)
            # Rewards based on recorded launcher activity appear on the creature.
            items=p['items']
            if 'ZAPATILLAS' in items:l.rect(17,21,8,2,fill=True)
            if 'CUBO' in items:l.rect(36,12,5,5,fill=True)
            if 'QUESITO' in items:l.rect(4,11,5,4,fill=True)
            if 'LINTERNA' in items:l.line(34,19,43,15)
            if 'ESTRELLA' in items:l.heart(2,2)
            l.text(24,21,PET_MENU[self.index],center=True)
            l.text(24,27,f'LV{p["level"]} XP{p["xp"]}',center=True)
        elif self.page=='pet_stats':
            l.text(24,1,'AQUAPET',center=True)
            labels=(('COMIDA','full'),('FELIZ','happy'),('LIMPIO','clean'),('ENERGIA','energy'))
            label,key=labels[self.index]
            l.text(24,10,label,center=True)
            l.rect(3,19,42,8)
            l.rect(5,21,round(p[key]*38/100),4,fill=True)
            l.text(24,27,f'{p[key]}%',center=True)
        elif self.page=='pet_items':
            l.text(24,1,'OBJETOS',center=True)
            items=p['items']
            if not items:l.text(24,15,'NINGUNO',center=True)
            else:
                l.text(24,13,items[self.index][:11],center=True)
                l.text(24,24,f'{self.index+1}/{len(items)}',center=True)
        elif self.page=='games':
            l.text(24,1,'MINIJUEGOS',center=True)
            l.rect(1,9,46,16)
            l.text(24,15,GAMES[self.index],center=True)
            l.text(24,27,'A:OK B:VOL',center=True)
        elif self.page=='catch':
            l.text(24,0,f'ATRAPA {self.score}/3',center=True)
            for x in range(5):l.line(4+x*9,7,4+x*9,28)
            fx=4+self.target*9
            l.rect(fx-2,self.drop_y,5,4,fill=True)
            l.rect(4+self.player*9-3,27,7,4,fill=True)
        elif self.page=='memory':
            l.text(24,1,'MEMORIA',center=True)
            if self.memory_reveal>0:
                l.text(24,11,'RECUERDA',center=True)
                arrows={'UP':'U','RIGHT':'R','DOWN':'D','LEFT':'L'}
                l.text(24,20,''.join(arrows[x] for x in self.sequence),center=True)
            else:
                l.text(24,12,'TU TURNO!',center=True)
                l.text(24,23,f'{self.seq_position}/3',center=True)
        elif self.page=='game_result':
            l.rect(1,4,46,20)
            l.text(24,11,self.notes[:11],center=True)
            l.text(24,26,'A:MENU',center=True)
        elif self.page=='files':
            report=self.files()
            l.text(24,1,'ARCHIVOS',center=True)
            if not report:l.text(24,13,'SIN VMU',center=True)
            elif not report.entries:l.text(24,13,'VACIA',center=True)
            else:
                i=min(self.index,len(report.entries)-1)
                ent=report.entries[i]
                l.text(24,12,ent.filename[:11],center=True)
                l.text(24,22,f'{i+1}/{len(report.entries)} {ent.blocks}B',center=True)
        elif self.page=='file_detail':
            report=self.files()
            l.text(24,1,'DETALLE',center=True)
            if report and report.entries:
                ent=report.entries[min(self.index,len(report.entries)-1)]
                l.text(24,11,ent.filename[:11],center=True)
                l.text(24,20,f'{ent.blocks} BLOQ',center=True)
                l.text(24,27,'FAT:'+ent.chain_status[:7],center=True)
        elif self.page=='status':
            report=self.files()
            l.text(24,1,'ESTADO VMU',center=True)
            if report:
                l.text(24,11,f'LIBRE {report.free_blocks}',center=True)
                l.text(24,20,f'SAVES {len(report.entries)}',center=True)
            else:l.text(24,13,'SIN MEMORIA',center=True)
            l.text(24,27,f'NIVEL {p["level"]}',center=True)
        return l.bitmap()
