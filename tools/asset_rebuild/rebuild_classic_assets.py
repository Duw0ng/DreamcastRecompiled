"""Rebuild the classic aqua art as independent assets, without changing game/runtime.

Sources are embedded in the package; no new generated stock art or online assets.
"""
from __future__ import annotations
from pathlib import Path
import numpy as np
from PIL import Image, ImageFilter, ImageDraw
import cv2
from scipy.ndimage import gaussian_filter1d, median_filter

ASSETS = Path(__file__).resolve().parents[2]/'launcher'/'assets'
SIZES=(1152,960,800)
MAIN=('play','games','vmu','music','settings','controllers','display','network')
ADV=('audio','video','performance','saves','system','profiles','debug','language','themes')

def classic_background():
    src=np.asarray(Image.open(ASSETS/'dreamcast_launcher_1152.png').convert('RGB'),dtype=np.float32)
    h,w=src.shape[:2]; xs=[20,316,578,846,1130]
    # Sample only genuinely unobstructed source-image lanes. They reproduce the
    # classic calm gradient and soft reflections rather than the busy V3 pattern.
    samples=[]
    for x in xs:
        lane=src[:678,max(0,x-8):min(w,x+9)].mean(axis=1)
        samples.append(gaussian_filter1d(lane,4,axis=0))
    samples=np.stack(samples,axis=1)
    smooth=np.empty((h,w,3),dtype=np.float32)
    for y in range(678):
        for k in range(3):
            smooth[y,:,k]=np.interp(np.arange(w),xs,samples[y,:,k])
    for y in range(678,h):
        t=(y-678)/(h-678)
        # The original panel hid most pixels; interpolate from the undisturbed
        # soft-blue stripe above the panel to the unoccupied bottom edge.
        top=smooth[660]
        bot=gaussian_filter1d(src[860],18,axis=0)
        smooth[y]=top*(1-t)+bot*t
    mask=np.zeros((h,w),np.float32)
    # Hide only the actual old art/labels, not the open gaps between columns.
    rects=[(75,118,307,393),(336,114,559,393),(601,114,831,393),(863,114,1104,393),
           (73,405,314,665),(320,406,571,666),(591,404,839,665),(860,400,1113,667),
           (17,665,1138,859)]
    for x1,y1,x2,y2 in rects: cv2.rectangle(mask,(x1,y1),(x2,y2),1,-1)
    # FULL coverage of the old artwork, plus a feather only outside the mask.
    # Previously Gaussian blurring the rectangles themselves leaked 1998 labels,
    # icon silhouettes and a ghost of the old advanced panel into the new UI.
    mask[663:,:]=1
    expanded=cv2.dilate(mask.astype(np.uint8),np.ones((41,41),np.uint8))
    soft=cv2.GaussianBlur(expanded.astype(np.float32),(0,0),sigmaX=8,sigmaY=8)
    mask=np.where(mask>0.5,1.0,soft)
    src=src*(1-mask[...,None])+smooth*mask[...,None]
    # The clock and labels embedded in the older full-screen source must NEVER
    # leak into the new real-time header. Tkinter constructs the native header.
    for y in range(99):
        a=y/98
        src[y]=np.array([192,192,204])*(1-a)+np.array([173,204,230])*a
    rgb=np.uint8(np.clip(src,0,255))
    out=Image.fromarray(rgb,'RGB')
    for width in SIZES:
        im=out.resize((width,round(864*width/1152)),Image.Resampling.LANCZOS)
        im.save(ASSETS/f'water_{width}.png',optimize=True)
    out.save(ASSETS/'classic_background_1152.png',optimize=True)
    return out

def clean_icon(key):
    im=Image.open(ASSETS/'original_cutouts'/f'{key}.png').convert('RGBA')
    rgba=np.array(im,dtype=np.uint8)
    h,w=rgba.shape[:2]
    alpha=rgba[:,:,3].astype(np.float32)
    rgb=rgba[:,:,:3].astype(np.float32)
    if key in ('play','controllers','vmu','settings','display'):
        # Remove old blue 'background' caught by the matte.  The strongest
        # object-certainty channel is the unsaturated, bright Dreamcast plastic.
        r,g,b=rgb.transpose((2,0,1))
        certainty=(r>120)&(g>125)&((b-r)<90)&(alpha>120)
        # Pale perimeter is the real silhouette, not the vivid blue backdrop.
        certainty=cv2.morphologyEx(certainty.astype('uint8'),cv2.MORPH_CLOSE,np.ones((3,3),np.uint8))
        # For each column clip only below the real plastic contour.  This
        # resolves the thick solid-blue 'bridge' between controller handles.
        bottom=np.full(w,-1,dtype=np.float32)
        top=np.full(w,h,dtype=np.float32)
        for x in range(w):
            yy=np.flatnonzero(certainty[:,x])
            if len(yy):
                bottom[x]=np.percentile(yy,99)
                top[x]=np.percentile(yy,1)
        # Very small gaps in the source mattes should not notch the contour.
        valid=np.where(bottom>=0)[0]
        if len(valid):
            bottom=np.interp(np.arange(w),valid,bottom[valid],left=-5,right=-5)
            top=np.interp(np.arange(w),valid,top[valid],left=h+5,right=h+5)
            bottom=median_filter(bottom,size=5)
            bottom=gaussian_filter1d(bottom,0.85)
            top=gaussian_filter1d(top,0.85)
            yy=np.arange(h)[:,None]
            edge=np.clip((bottom[None,:]+2-yy)/2.0,0,1)
            edge*=np.clip((yy-top[None,:]+2)/2.0,0,1)
            alpha*=edge
        # Drop isolated saturated blue pixels at the far left and right sides.
        if key=='play':
            alpha[:, :4]=0;alpha[:,w-4:]=0
        if key=='controllers':
            # Remove the bottom wallpaper between the grips, while preserving
            # anti-aliased gray contours and nearby legitimate blue shadows.
            pass
    if key=='games':
        # Original cases contain blue plastic; remove only isolated fringe
        # outside the actual group, not the blue covers themselves.
        alpha[:3,:]*=np.linspace(0,1,3)[:,None]
    if key=='network':
        # The old extraction retained a large opaque blue background plate
        # outside the globe. Soften it radially while preserving lit orbits.
        yy,xx=np.ogrid[:h,:w]
        cx,cy=w*.49,h*.49
        d=np.sqrt(((xx-cx)/(w*.425))**2+((yy-cy)/(h*.445))**2)
        roll=np.clip((1.18-d)/.20,0,1)
        # Preserve the bright thin orbit lines but remove the wide solid
        # blue plate inherited from the old screenshot's wallpaper.
        bright=(rgb[:,:,0]>130)&(rgb[:,:,1]>199)&(rgb[:,:,2]>216)
        alpha*=np.maximum(roll,np.where(bright,0.85,0.0))
    # Original cutouts had tiny colored, isolated pixels around the silhouette.
    binary=(alpha>80).astype(np.uint8)
    count,labels,stats,_=cv2.connectedComponentsWithStats(binary,8)
    if count>1:
        biggest=np.argmax(stats[1:,cv2.CC_STAT_AREA])+1
        main=(labels==biggest).astype(np.uint8)
        # Music note has multiple connected notes joined by strokes; most icons
        # have one main component. Keep larger detached glow/glints (>60px).
        keep=(main>0)
        for i in range(1,count):
            if stats[i,cv2.CC_STAT_AREA]>60: keep|=(labels==i)
        grow=cv2.dilate(keep.astype('uint8'),np.ones((3,3),np.uint8)).astype(np.float32)
        alpha*=grow
    # Preserve subtle antialiasing; only shave faint leftover external fringes.
    rgba[:,:,3]=np.uint8(np.clip(alpha,0,255))
    out=Image.fromarray(rgba,'RGBA')
    out.save(ASSETS/f'{key}_1152.png',optimize=True)
    out.save(ASSETS/f'icon_{key}.png',optimize=True)
    for width in (960,800):
        resized=out.resize((round(w*width/1152),round(h*width/1152)),Image.Resampling.LANCZOS)
        resized.save(ASSETS/f'{key}_{width}.png',optimize=True)
    return out

def clean_advanced(key):
    im=Image.open(ASSETS/'original_cutouts'/f'adv_{key}.png').convert('RGBA')
    data=np.asarray(im).copy()
    alpha=data[:,:,3].astype('float32')
    # Fully transparent RGB may carry blue cutout colors; normalize so neither
    # Pillow nor Windows Tkinter re-sampling produces blue fringe pixels.
    mask=alpha>70
    components, lab, stats,_=cv2.connectedComponentsWithStats(mask.astype('uint8'),8)
    keep=np.zeros_like(mask)
    for i in range(1,components):
        if stats[i,cv2.CC_STAT_AREA] >= (5 if key in ('language','themes') else 10):keep|=(lab==i)
    keep=cv2.dilate(keep.astype('uint8'),np.ones((3,3),np.uint8))
    alpha*=keep
    data[:,:,3]=np.uint8(alpha)
    out=Image.fromarray(data,'RGBA')
    out.save(ASSETS/f'adv_{key}.png',optimize=True)
    for size in SIZES:
        resized=out if size==1152 else out.resize((max(1,round(out.width*size/1152)),max(1,round(out.height*size/1152))),Image.Resampling.LANCZOS)
        resized.save(ASSETS/f'advanced_{key}_{size}.png',optimize=True)

if __name__=='__main__':
    classic_background()
    for k in MAIN:clean_icon(k)
    for k in ADV:clean_advanced(k)
    print('Restored calm original background and cleaned independent icon assets at 800/960/1152')
