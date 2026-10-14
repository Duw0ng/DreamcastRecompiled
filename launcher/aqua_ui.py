"""Native Tkinter/Aqua dashboard for Dreamcast Recompiled v1.2.

Only the artistic icon images and the water texture are bitmap resources.  The
buttons, hit detection, focus, labels, clock, expandable options and launcher
commands are controlled by Python/Tkinter at runtime. No PIL/PyPI dependency.
"""
from __future__ import annotations

from datetime import datetime
from pathlib import Path
import sys
import tkinter as tk

ASSETS = Path(__file__).resolve().parent / 'assets'
MAIN = ('play','games','vmu','music','settings','controllers','display','network')
ADV = ('audio','video','performance','saves','system','profiles','debug','language','themes')
COLORS = {
    'play':('#e88c62','#ffc77f'),'games':('#2797e9','#85e4ff'),
    'vmu':('#36ab85','#b4ffbd'),'music':('#c854b2','#ffc2f5'),
    'settings':('#9862d5','#e0b6ff'),'controllers':('#21a5c3','#a5ffff'),
    'display':('#c69b43','#ffe3a0'),'network':('#7e61d4','#b0eaff'),
    'audio':('#e67f57','#ffd0a1'),'video':('#3998ef','#a6eaff'),
    'performance':('#32a984','#a6ffa9'),'saves':('#9760d7','#d1b3ff'),
    'system':('#627baf','#b3d0fc'),'profiles':('#d15bbb','#ffc0eb'),
    'debug':('#e46787','#ffc4d4'),'language':('#357adc','#a7dcff'),
    'themes':('#3e96db','#cbebff')
}


def rounded(canvas: tk.Canvas, xy, radius, **kwargs):
    """Native canvas polygon using sampled arc points; no rasterized buttons."""
    x1,y1,x2,y2=xy
    r=min(radius,(x2-x1)/2,(y2-y1)/2)
    from math import sin,cos,pi
    pts=[]
    for cx,cy,start in ((x2-r,y1+r,-pi/2),(x2-r,y2-r,0),
                        (x1+r,y2-r,pi/2),(x1+r,y1+r,pi)):
        for i in range(7):
            t=start+i*pi/12
            pts.extend((cx+r*cos(t),cy+r*sin(t)))
    return canvas.create_polygon(*pts,smooth=False,**kwargs)


class AquaDashboard:
    """Canvas for the dashboard, with native interactions and independent assets."""
    def __init__(self, launcher):
        self.app=launcher
        self.root=launcher.root
        self.width=self._select_width()
        self.scale=self.width/1152
        self.extended=bool(self.app.settings.get('advanced_open', True))
        self.focus='play'
        self.hover=None
        # Mouse selection never falls back to an unrelated, earlier keyboard
        # focus.  The old fallback caused "phantom" highlights in gaps.
        self.pointer_mode='mouse'
        self.images={}
        self.hotspots={}
        self.message=''
        self._clock_job=None
        self.page_frame=None
        self.page_window=None
        self.clock_text=None
        self.clock_date=None
        self.clock_time=None
        self._load_images()
        self.canvas=tk.Canvas(self.root,highlightthickness=0,bd=0,bg=('#111f35' if self.dark else '#518fe2'),
                              width=self.width,height=round(864*self.scale),takefocus=True)
        self.canvas.pack()
        self.canvas.bind('<Motion>',self.motion)
        self.canvas.bind('<Leave>',self.leave)
        self.root.bind('<Destroy>',self._maybe_cancel_clock,add='+')
        self.canvas.bind('<Button-1>',self.click)
        self.root.bind('<Left>',lambda e:self.navigate(-1))
        self.root.bind('<Right>',lambda e:self.navigate(1))
        self.root.bind('<Up>',lambda e:self.navigate(-4))
        self.root.bind('<Down>',lambda e:self.navigate(4))
        self.root.bind('<Tab>',self.tab)
        self.root.bind('<Return>',self.enter)
        self.root.bind('<space>',self.enter)
        self.root.bind('<Control-o>',lambda e:self.app.pick_game())
        self.root.bind('<F1>',lambda e:self.app.help_dialog())
        self.root.bind('<Escape>',lambda e:self.close_inline() if self.page_frame is not None else self.canvas.focus_set())
        self.canvas.focus_set()
        self.root.resizable(False,False)
        self._layout()
        self.draw()
        self.resize_window()
        self._refresh_clock()

    @property
    def dark(self):
        return self.app.settings.get('theme') != 'classic'

    def _tone(self, classic, night):
        return night if self.dark else classic

    def apply_theme(self):
        self._load_images()
        self.canvas.configure(bg=self._tone('#518fe2','#111f35'))
        self.close_inline()
        self.draw()

    def _select_width(self):
        maxw=max(600,self.root.winfo_screenwidth()-80)
        maxh=max(500,self.root.winfo_screenheight()-100)
        supported = tuple(w for w in (1152, 960, 800) if w<=maxw and w*0.75<=maxh)
        preferred = self.app.settings.get('launcher_width', 'auto')
        if preferred in ('800', '960', '1152') and int(preferred) in supported:
            return int(preferred)
        return supported[0] if supported else 800

    def _load_images(self):
        for key in MAIN:
            self.images[key]=tk.PhotoImage(file=str(ASSETS/f'{key}_{self.width}.png'))
        for key in ADV:
            self.images[key]=tk.PhotoImage(file=str(ASSETS/f'advanced_{key}_{self.width}.png'))
        theme_name = self.app.settings.get('theme', 'classic')
        theme = {'classic':'water','dark':'water_night','night':'water_night','starry':'water_starry'}.get(theme_name,'water')
        self.images['background']=tk.PhotoImage(file=str(ASSETS/f'{theme}_{self.width}.png'))
        self.images['swirl']=tk.PhotoImage(file=str(ASSETS/f'swirl_{self.width}.png'))
        for key in MAIN:
            for mode in ('normal','active'):
                self.images[f'button_{key}_{mode}']=tk.PhotoImage(
                    file=str(ASSETS/f'button_{key}_{mode}_{self.width}.png'))

    def p(self,*points):
        return tuple(round(a*self.scale) for a in points)

    def _layout(self):
        self.hotspots.clear()
        for i,key in enumerate(MAIN):
            cx=190+(i%4)*263
            cy=225+(i//4)*270
            # Hit areas never overlap the second row or neighboring controls.
            self.hotspots[key]=(cx-115,cy-105,cx+115,cy+155)
        # entire Advanced Options heading toggles collapse
        self.hotspots['advanced']=(30,675,1121,722)
        if self.extended:
            for i,key in enumerate(ADV):
                cx=99+i*119
                self.hotspots[key]=(cx-54,723,cx+54,829)

    def resize_window(self, force_full=False):
        h=864 if force_full or self.page_frame is not None or self.extended else 735
        self.canvas.configure(width=self.width,height=round(h*self.scale))
        # Never call update_idletasks while a redraw is in progress.  On Windows
        # it painted an empty canvas for up to a second between delete/draw.
        # Preserve window center when collapsing while using a sane initial position.
        geometry=f'{self.width}x{round(h*self.scale)}'
        if not getattr(self,'_placed',False):
            sw,sh=self.root.winfo_screenwidth(),self.root.winfo_screenheight()
            geometry+=f'+{max(15,(sw-self.width)//2)}+{max(15,(sh-round(h*self.scale))//2)}'
            self._placed=True
        if self.root.geometry().split('+')[0] != geometry.split('+')[0] or not hasattr(self,'_window_set'):
            self.root.geometry(geometry)
            self._window_set=True

    def open_inline(self, title: str, label: str | None = None):
        """Host a native settings/library page inside the single Aqua window.

        The water, grey Dreamcast masthead and live clock stay on-screen.
        Ordinary pages do not create Toplevel windows. File pickers remain
        native OS dialogs only when the user explicitly requests a file.
        """
        self.close_inline()
        self.pointer_mode='mouse'
        self.hover=None
        self._set_selection()
        self.canvas.itemconfigure('dashboard', state='hidden')
        # Keep header native and clock live, no screenshot replacement.
        self.resize_window(force_full=True)
        c=self.canvas
        from aqua_ui import rounded
        rounded(c,self.p(22,112,1130,849),27*self.scale,fill=self._tone('#68a8e8','#1d3756'),
                outline=self._tone('#ebf8ff','#587ca0'),width=max(1,round(3*self.scale)),tags='inline_page')
        rounded(c,self.p(29,121,1123,844),23*self.scale,fill=self._tone('#a9d7fc','#263e5e'),
                outline=self._tone('#d6f0ff','#425f81'),width=max(1,round(self.scale)),tags='inline_page')
        bg=self._tone('#b4dffa','#1e3049')
        page=tk.Frame(c,bg=bg,bd=0,highlightthickness=0)
        self.page_frame=page
        self.page_window=c.create_window(*self.p(44,133),window=page,anchor='nw',
                                         width=round(1064*self.scale),height=round(693*self.scale),
                                         tags='inline_page')
        heading=tk.Frame(page,bg=bg,height=46)
        heading.pack(fill='x',padx=8,pady=(4,5))
        tk.Button(heading,text='‹  Inicio',bg=self._tone('#287fc4','#39689c'),fg='white',activebackground='#176ba8',
                  activeforeground='white',relief='flat',bd=0,cursor='hand2',
                  font=('Segoe UI',max(10,round(12*self.scale)),'bold'),
                  command=self.close_inline,padx=16,pady=5).pack(side='left',padx=(0,14))
        tk.Label(heading,text=title,bg=bg,fg=self._tone('#17446e','#ecf3ff'),
                 font=('Segoe UI',max(13,round(22*self.scale)),'bold')).pack(side='left')
        tk.Label(heading,text='  ·  DCRV1.2 Stable Release',bg=bg,fg=self._tone('#4775a2','#b4c8de'),
                 font=('Segoe UI',max(9,round(10*self.scale)))).pack(side='left')
        nav=tk.Frame(page,bg=bg)
        nav.pack(fill='x',padx=8,pady=(1,10))
        quick=[('Juegos','games'),('Vídeo','video'),('Audio','audio'),
               ('Rendimiento','performance'),('Mandos','controllers'),
               ('VMU','vmu'),('Ajustes','settings')]
        for txt,key in quick:
            tk.Button(nav,text=txt,bg=('#2583c9' if key==label else self._tone('#e1f3ff','#304b6a')),
                      fg=('white' if key==label else self._tone('#194a76','#edf5ff')),relief='flat',bd=0,
                      cursor='hand2',activebackground='#74c1f2',
                      font=('Segoe UI',max(8,round(10*self.scale)),'bold'),
                      command=lambda k=key:self.app.activate(k),
                      padx=10,pady=4).pack(side='left',padx=3)
        body=tk.Frame(page,bg=self._tone('#d9efff','#192d46'),bd=1,relief='solid',highlightthickness=0)
        body.pack(fill='both',expand=True,padx=8,pady=(0,8))
        # The page body is a real native Tk frame. Existing windows such as
        # the games library can be embedded without duplicating their logic.
        def on_destroy(event,frm=page):
            if event.widget is frm and self.page_frame is frm:
                self._on_page_destroyed()
        page.bind('<Destroy>',on_destroy,add='+')
        return page,body

    def _on_page_destroyed(self):
        self.page_frame=None
        self.canvas.delete('inline_page')
        self.page_window=None
        self.canvas.itemconfigure('dashboard',state='normal')
        self.resize_window()
        self.canvas.focus_set()

    def close_inline(self):
        if self.page_frame is not None:
            self.page_frame.destroy()
        # Triggering destroy invokes _on_page_destroyed on the frame.

    def toggle_advanced(self):
        self.extended=not self.extended
        self.app.settings['advanced_open']=self.extended
        self.app.persist()
        if not self.extended and self.focus in ADV:
            self.focus='advanced'
        self._layout()
        if not self.extended and self.hover in ADV:
            self.hover=None
        self._set_advanced_visibility()
        self._set_selection()
        self.resize_window()

    def hit_test(self,x,y):
        x/=self.scale;y/=self.scale
        # Advanced heading should be checked first, never blocked by another item.
        for key,b in self.hotspots.items():
            x1,y1,x2,y2=b
            if x1<=x<=x2 and y1<=y<=y2:
                return key
        return None

    def motion(self,e):
        if self.page_frame is not None:return
        new=self.hit_test(e.x,e.y)
        # Pointer control is exclusive: passing through an empty gap must NOT
        # reactivate a stale keyboard/click focus, even for a single frame.
        changed=(self.pointer_mode != 'mouse' or new != self.hover)
        self.pointer_mode='mouse'
        self.hover=new
        if not changed:return
        self.canvas.configure(cursor='hand2' if new else '')
        self._set_selection()

    def leave(self,e=None):
        self.pointer_mode='mouse'
        self.hover=None
        self.canvas.configure(cursor='')
        self._set_selection()

    def click(self,e):
        if self.page_frame is not None:return
        key=self.hit_test(e.x,e.y)
        if not key:return
        self.focus=key
        self.pointer_mode='mouse'
        self.hover=key
        if key=='advanced':
            self.toggle_advanced();return
        self._set_selection()
        self.app.activate(key)

    def navigate(self,offset):
        if self.page_frame is not None:return None
        self.pointer_mode='keyboard'
        self.hover=None
        keys=list(MAIN)+(list(ADV) if self.extended else [])
        if self.focus not in keys:self.focus=keys[0]
        else:self.focus=keys[(keys.index(self.focus)+offset)%len(keys)]
        self._set_selection()
        return 'break'

    def tab(self,e):
        if self.page_frame is not None:return None
        return self.navigate(-1 if e.state & 1 else 1)

    def enter(self,e):
        if self.page_frame is not None:return None
        if self.focus in (*MAIN,*ADV):self.app.activate(self.focus)
        elif self.focus=='advanced':self.toggle_advanced()
        return 'break'

    def set_message(self,text):
        self.message=text
        # Changing the selected game must not clear/recreate the entire screen.
        if getattr(self,'message_id',None):
            msg=(text if len(text)<92 else text[:89]+'...') if not text.startswith(('Elegí un juego','Choose a game')) else ''
            self.canvas.itemconfigure(self.message_id,text=msg)

    def _txt(self,key):
        # Text is generated in Python, not baked into an image.
        if self.app.lang=='en':
            return {'vmu':'VMU / Files','system':'BIOS / System','profiles':'Per-game\nProfiles',
                    'advanced':'Advanced Options'}.get(key,key.capitalize())
        return {'play':'Jugar','games':'Juegos','vmu':'VMU / Archivos',
                'music':'Música','settings':'Ajustes','controllers':'Mandos',
                'display':'Pantalla','network':'Red','performance':'Rendimiento',
                'saves':'Guardados','system':'BIOS / Sistema',
                'profiles':'Perfiles\npor juego','debug':'Diagnóstico',
                'language':'Idioma','themes':'Temas','advanced':'Opciones avanzadas',
                'video':'Vídeo'}.get(key,key.capitalize())

    def _set_selection(self):
        """Update only two existing widgets, never create/delete a canvas item.

        Mouse and keyboard own selection separately. On mouse gaps no icon is
        selected, so the previous keyboard focus cannot flash underneath the
        next hovered item. Normal and active artwork have identical geometry.
        """
        chosen = self.hover if self.pointer_mode == 'mouse' else self.focus
        if chosen == 'advanced' or (chosen in ADV and not self.extended):
            chosen=None
        old=getattr(self,'_selected',None)
        if chosen == old: return
        for key in (old,chosen):
            if key in MAIN and hasattr(self,'button_items'):
                self.canvas.itemconfigure(self.button_items[key],
                    image=self.images[f'button_{key}_{"active" if key==chosen else "normal"}'])
            elif key in ADV and hasattr(self,'advanced_rings'):
                # Native outline only; no old image halo and no ghost copies.
                self.canvas.itemconfigure(self.advanced_rings[key],
                    fill='#ebf9ff' if key==chosen else '#cee9ff',
                    outline=COLORS[key][0] if key==chosen else '#c1e4ff',
                    width=max(1,round((3 if key==chosen else 1)*self.scale)))
        self._selected=chosen

    def _set_advanced_visibility(self):
        # Retained scene. Changes only visibility of the independent panel.
        state='normal' if self.extended else 'hidden'
        self.canvas.itemconfigure('advanced_content',state=state)
        self.canvas.itemconfigure('advanced_expanded',state=state)
        self.canvas.itemconfigure('advanced_collapsed',state='hidden' if self.extended else 'normal')
        if getattr(self,'advanced_arrow',None):
            self.canvas.itemconfigure(self.advanced_arrow,text='⌃' if self.extended else '⌄')
        # The selected item may be in the panel that just closed.
        if getattr(self,'_selected',None) in ADV and not self.extended:
            old=self._selected
            self.canvas.itemconfigure(self.advanced_rings[old],
                fill=self._tone('#cee9ff','#29435f'),outline=self._tone('#c1e4ff','#476a86'),width=max(1,round(self.scale)))
            self._selected=None

    def draw(self):
        """Construct the whole scene only on startup or deliberate language change.

        Regular hover/focus/collapse/clock/status updates use itemconfigure(),
        which avoids a full repaint of the 1152x864 background on Windows.
        """
        c=self.canvas;s=self.scale
        c.delete('scene')
        self._selected=None
        c.create_image(0,0,image=self.images['background'],anchor='nw',tags=('scene','water'))
        # Classic Dreamcast gray header, faithful to the first UI iteration.
        c.create_rectangle(*self.p(0,0,1152,97),fill=self._tone('#b5b3b9','#222a38'),outline='',tags='scene')
        c.create_image(*self.p(67,52),image=self.images['swirl'],tags='scene')
        c.create_text(*self.p(108,53),text='Dreamcast Recompiled',anchor='w',
            fill=self._tone('#26262b','#f3f5ff'),font=('Segoe UI',round(31*s)),tags='scene')
        c.create_text(*self.p(581,54),text='DCRV1.2',anchor='w',
            fill=self._tone('#39406f','#b7d9fa'),font=('Segoe UI',round(22*s)),tags='scene')
        self.clock_date=c.create_text(*self.p(955,63),anchor='e',
            text=datetime.now().strftime('%d/%m/%Y'),fill=self._tone('#25242a','#e4edff'),
            font=('Consolas',round(22*s),'bold'),tags=('scene','clock'))
        self.clock_time=c.create_text(*self.p(1107,63),anchor='e',
            text=datetime.now().strftime('%H:%M:%S'),fill=self._tone('#25242a','#e4edff'),
            font=('Consolas',round(20*s),'bold'),tags=('scene','clock'))
        self.clock_text=self.clock_time
        self._header_scene=set(c.find_all())
        # The original illustrations and classic gradient capsules are
        # independent images. Normal/hover are swapped on ONE canvas item.
        self.button_items={}
        for i,key in enumerate(MAIN):
            cx=190+(i%4)*263
            cy=225+(i//4)*270
            c.create_image(*self.p(cx,cy),image=self.images[key],tags=('scene','icon_'+key))
            label_y=cy+127
            self.button_items[key]=c.create_image(*self.p(cx,label_y),
                image=self.images[f'button_{key}_normal'],tags=('scene','button_'+key))
            txt=self._txt(key)
            fs=19 if len(txt)>12 else 23
            # Separate native text preserves English/Spanish translations.
            c.create_text(*self.p(cx+1,label_y+2),text=txt,fill=self._tone('#44526b','#122438'),
                font=('Segoe UI',max(10,round(fs*s)),'bold'),tags='scene')
            c.create_text(*self.p(cx,label_y),text=txt,fill='#ffffff',
                font=('Segoe UI',max(10,round(fs*s)),'bold'),tags='scene')
        # Two already drawn heights, show just one on collapse.
        rounded(c,self.p(28,678,1124,729),21*s,fill=self._tone('#65a9ef','#223a5b'),outline='#deefff',
                width=max(1,round(2*s)),tags=('scene','advanced_collapsed'))
        rounded(c,self.p(28,678,1124,837),21*s,fill=self._tone('#65a9ef','#223a5b'),outline='#deefff',
                width=max(1,round(2*s)),tags=('scene','advanced_expanded'))
        rounded(c,self.p(42,728,1110,824),18*s,fill=self._tone('#b7ddfb','#1d3250'),outline='#dbefff',
                width=1,tags=('scene','advanced_content'))
        self.advanced_rings={}
        for i,key in enumerate(ADV):
            cx=99+i*119
            if i:
                c.create_line(*self.p(cx-59,745,cx-59,805),fill=self._tone('#93b9df','#567392'),
                              tags=('scene','advanced_content'))
            self.advanced_rings[key]=c.create_oval(*self.p(cx-36,731,cx+36,796),
                fill=self._tone('#cee9ff','#29435f'),outline=self._tone('#c1e4ff','#476a86'),width=1,
                tags=('scene','advanced_content','ring_'+key))
            c.create_image(*self.p(cx,764),image=self.images[key],tags=('scene','advanced_content'))
            c.create_text(*self.p(cx,807),text=self._txt(key),fill=self._tone('#173c6b','#dceeff'),
                    justify='center',font=('Segoe UI',max(8,round(11*s))),
                    tags=('scene','advanced_content'))
        # Always visible heading ABOVE the content layers; clickable in both modes.
        rounded(c,self.p(38,685,1114,724),17*s,fill=self._tone('#8dc4fa','#2a4668'),outline='#b8eaff',
                width=max(1,round(1*s)),tags='scene')
        c.create_oval(*self.p(53,688,90,723),fill='#267dcc',outline='#eafaff',
                      width=max(1,round(2*s)),tags='scene')
        self.advanced_arrow=c.create_text(*self.p(71,706),text='⌃' if self.extended else '⌄',
            fill='white',font=('Segoe UI',round(20*s),'bold'),tags='scene')
        c.create_text(*self.p(106,706),text=self._txt('advanced'),anchor='w',
            fill=self._tone('#163f75','#d8ecff'),font=('Segoe UI',round(17*s),'bold'),tags='scene')
        self.message_id=c.create_text(*self.p(576,111),anchor='n',
            text=('[ '+self.message+' ]' if self.message and not self.message.startswith(('Elegí un juego','Choose a game')) else ''),
            fill=self._tone('#fff9d6','#e8f1ff'),font=('Segoe UI',round(10*s)),tags='scene')
        # Group the homepage separately from the persistent water + header.
        c.addtag_withtag('dashboard','scene')
        for item in self._header_scene:c.dtag(item,'dashboard')
        if self.page_frame is not None:c.itemconfigure('dashboard',state='hidden')
        self._set_advanced_visibility()
        self._set_selection()

    def _maybe_cancel_clock(self,event):
        if event.widget is self.root and self._clock_job is not None:
            # Cancel via Tcl without unregistering the command during Tk's
            # destruction walk (after_cancel in <Destroy> can double-delete).
            try:self.root.tk.call('after','cancel',self._clock_job)
            except tk.TclError:pass
            self._clock_job=None

    def _refresh_clock(self):
        if not self.root.winfo_exists():return
        now=datetime.now()
        if self.clock_date:
            self.canvas.itemconfigure(self.clock_date,text=now.strftime('%d/%m/%Y'))
        if self.clock_time:
            self.canvas.itemconfigure(self.clock_time,text=now.strftime('%H:%M:%S'))
        # A single repeating timer, independent of draw and mouse hover.
        self._clock_job=self.root.after(1000,self._refresh_clock)
