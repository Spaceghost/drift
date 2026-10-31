# SPDX-License-Identifier: MIT
# Bounded reader for the SHA-pinned MINIX3.3 test image, not arbitrary disks.
from pathlib import Path
import struct,json
class MFS:
 def __init__(self,path,sector):
  self.path=Path(path);self.f=self.path.open('rb');self.base=sector*512;s=self.read(1024,32)
  self.ninodes=struct.unpack_from('<I',s)[0]; self.imap,self.zmap=struct.unpack_from('<HH',s,6);self.log=struct.unpack_from('<H',s,12)[0];self.bs=struct.unpack_from('<H',s,28)[0];self.zs=self.bs<<self.log
  assert struct.unpack_from('<H',s,24)[0]==0x4d5a
  assert self.bs==4096 and self.log==0
  self.ib=(2+self.imap+self.zmap)*self.bs
 def read(self,off,n):self.f.seek(self.base+off);return self.f.read(n)
 def inode(self,ino):
  b=self.read(self.ib+(ino-1)*64,64);mode,nlinks,uid,gid,size,at,mt,ct=struct.unpack_from('<HHHHIIII',b);zones=struct.unpack_from('<10I',b,24);return mode,size,zones
 def blocks(self,z):
  return struct.unpack('<'+str(self.zs//4)+'I',self.read(z*self.zs,self.zs)) if z else [0]*(self.zs//4)
 def chunks(self,ino):
  mode,size,z=self.inode(ino);blocks=list(z[:7]);remain=(size+self.zs-1)//self.zs
  if remain>7:blocks+=list(self.blocks(z[7]))
  if remain>len(blocks):
   for indirect in self.blocks(z[8]):
    if remain<=len(blocks):break
    blocks+=list(self.blocks(indirect))
  assert remain<=len(blocks)
  for x in blocks[:remain]:
   n=min(size,self.zs);yield self.read(x*self.zs,n) if x else bytes(n);size-=n
 def content(self,ino):return b''.join(self.chunks(ino))
 def entries(self,ino):
  b=self.content(ino)
  for off in range(0,len(b),64):
   rec=b[off:off+64];n=struct.unpack_from('<I',rec)[0];name=rec[4:].split(b'\x00')[0].decode('utf-8','replace')
   if n and name not in ('.','..'):yield name,n
 def lookup(self,path):
  ino=1
  for part in Path(path).parts:
   if part=='/':continue
   ino=dict(self.entries(ino))[part]
  return ino
 def list(self,root='/',depth=1):
  def walk(ino,path,d):
   for name,child in self.entries(ino):
    p=path.rstrip('/')+'/'+name;mode,size,z=self.inode(child);yield {'path':p,'mode':oct(mode),'inode':child,'bytes':size}
    if d and mode&0xf000==0x4000:yield from walk(child,p,d-1)
  return list(walk(self.lookup(root),root,depth))
