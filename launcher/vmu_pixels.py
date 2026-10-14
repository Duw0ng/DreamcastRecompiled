"""Tiny dependency-free 48x32 monochrome framebuffer and bitmap font.

The physical VMU LCD format is 48x32, MSB-first, 192 bytes total.
"""
from __future__ import annotations

W, H = 48, 32
FONT = {
 ' ':('000','000','000','000','000'),
 'A':('010','101','111','101','101'), 'B':('110','101','110','101','110'),
 'C':('011','100','100','100','011'), 'D':('110','101','101','101','110'),
 'E':('111','100','110','100','111'), 'F':('111','100','110','100','100'),
 'G':('011','100','101','101','011'), 'H':('101','101','111','101','101'),
 'I':('111','010','010','010','111'), 'J':('001','001','001','101','010'),
 'K':('101','101','110','101','101'), 'L':('100','100','100','100','111'),
 'M':('101','111','111','101','101'), 'N':('101','111','111','111','101'),
 'O':('010','101','101','101','010'), 'P':('110','101','110','100','100'),
 'Q':('010','101','101','111','011'), 'R':('110','101','110','101','101'),
 'S':('011','100','010','001','110'), 'T':('111','010','010','010','010'),
 'U':('101','101','101','101','111'), 'V':('101','101','101','101','010'),
 'W':('101','101','111','111','101'), 'X':('101','101','010','101','101'),
 'Y':('101','101','010','010','010'), 'Z':('111','001','010','100','111'),
 '0':('111','101','101','101','111'), '1':('010','110','010','010','111'),
 '2':('110','001','111','100','111'), '3':('110','001','110','001','110'),
 '4':('101','101','111','001','001'), '5':('111','100','110','001','110'),
 '6':('011','100','110','101','010'), '7':('111','001','010','010','010'),
 '8':('010','101','010','101','010'), '9':('010','101','011','001','110'),
 ':':('000','010','000','010','000'), '.':('000','000','000','000','010'),
 '-':('000','000','111','000','000'), '/':('001','001','010','100','100'),
 '+':('000','010','111','010','000'), '>':('100','010','001','010','100'),
 '<':('001','010','100','010','001'), '!':('010','010','010','000','010'),
 '?':('110','001','010','000','010'), '%':('101','001','010','100','101'),
 '*':('101','010','111','010','101'), '=':('000','111','000','111','000'),
 '(':('010','100','100','100','010'), ')':('010','001','001','001','010'),
 '_':('000','000','000','000','111'),
}

class LCD:
    def __init__(self):
        self.p = bytearray(W * H)

    def dot(self, x:int, y:int, color=1):
        if 0 <= x < W and 0 <= y < H:
            self.p[y * W + x] = 1 if color else 0

    def rect(self,x,y,w,h,fill=False,color=1):
        if w<=0 or h<=0:return
        for xx in range(x,x+w):
            self.dot(xx,y,color);self.dot(xx,y+h-1,color)
        for yy in range(y,y+h):
            self.dot(x,yy,color);self.dot(x+w-1,yy,color)
        if fill:
            for yy in range(y+1,y+h-1):
                for xx in range(x+1,x+w-1):self.dot(xx,yy,color)

    def line(self,x1,y1,x2,y2,color=1):
        dx=abs(x2-x1);dy=-abs(y2-y1)
        sx=1 if x1<x2 else -1;sy=1 if y1<y2 else -1
        error=dx+dy
        while True:
            self.dot(x1,y1,color)
            if x1==x2 and y1==y2:break
            e2=2*error
            if e2>=dy:error+=dy;x1+=sx
            if e2<=dx:error+=dx;y1+=sy

    def text(self, x:int, y:int, value:str, center=False):
        s=value.upper().replace('Á','A').replace('É','E').replace('Í','I').replace('Ó','O').replace('Ú','U').replace('Ñ','N')
        if center:x-=(len(s)*4-1)//2
        for index,char in enumerate(s):
            pattern=FONT.get(char,FONT['?'])
            for yy,row in enumerate(pattern):
                for xx,bit in enumerate(row):
                    if bit=='1':self.dot(x+index*4+xx,y+yy)

    def bitmap(self)->bytes:
        b=bytearray(W * H // 8)
        for i,v in enumerate(self.p):
            if v:b[i//8]|=1<<(7-i%8)
        return bytes(b)

    def heart(self,x,y):
        for xx,yy in [(1,0),(2,0),(4,0),(5,0),(0,1),(3,1),(6,1),(0,2),(6,2),
                      (1,3),(5,3),(2,4),(4,4),(3,5)]:self.dot(x+xx,y+yy)

    def pet(self,x=14,y=10,eyes=True):
        # Original AquaPet creature (not an emulated Sega Chao / VMU app).
        self.rect(x+2,y+1,15,11,fill=True)
        self.rect(x+1,y+3,17,7,fill=True)
        for xx,yy in ((4,-1),(5,-2),(14,-1),(13,-2)):
            self.dot(x+xx,y+yy)
        if eyes:
            for xx in (6,13):
                self.rect(x+xx,y+4,2,3,fill=True,color=0)
        self.line(x+8,y+9,x+11,y+9,color=0)
        self.dot(x,y+12);self.dot(x+18,y+12)
        self.rect(x+4,y+11,3,3,fill=True)
        self.rect(x+12,y+11,3,3,fill=True)
