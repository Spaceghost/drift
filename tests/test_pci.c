/* Independent config-space model. SPDX-License-Identifier: MIT */
#include "drift_pci.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct model {uint32_t bdf[3], cfg[3][64], writes;} model;
static uint32_t read_config(void *context,uint32_t bdf,uint32_t offset)
{
    model *m=context;
    assert(offset<256 && !(offset&3));
    for(unsigned i=0;i<3;++i) if(m->bdf[i]==bdf) return m->cfg[i][offset/4];
    return UINT32_MAX;
}
static void write_config(void *context,uint32_t bdf,uint32_t offset,uint32_t value)
{
    model *m=context;
    assert(offset==4 && !(value&0xffff0000));
    ++m->writes;
    for(unsigned i=0;i<3;++i) if(m->bdf[i]==bdf) m->cfg[i][offset/4]=value;
}
int main(void)
{
    model m={{8,11,0x107},{ {0} },0};
    drift_pci p={&m,read_config,write_config};
    drift_pci_device d={0};
    drift_pci_bar bar={0};
    uint32_t offset=0;
    m.cfg[0][0]=0x100e8086; m.cfg[0][3]=0x00800000; m.cfg[0][2]=0x02000001;
    m.cfg[1][0]=0x10421af4; m.cfg[1][2]=0x01000001;
    m.cfg[2][0]=0x813910ec;
    assert(drift_pci_find(&p,0,255,0xffff,0xffff,0,&d)==0 && d.bdf==8);
    assert(drift_pci_find(&p,0,255,0xffff,0xffff,1,&d)==0 && d.bdf==11 && d.class_code==0x010000);
    /* Function7 without function0 must not be discovered. */
    assert(drift_pci_find(&p,0,255,0xffff,0xffff,2,&d)==1);
    assert(drift_pci_find(&p,0,0,0x1af4,0x1042,0,&d)==0 && d.bdf==11);
    assert(drift_pci_find(&p,1,0,0,0,0,&d)==-1);
    assert(drift_pci_find(&p,0,256,0,0,0,&d)==-1);
    assert(drift_pci_find(NULL,0,255,0,0,0,&d)==-1);
    assert(drift_pci_cap(&p,8,9,0,&offset)==1);
    m.cfg[0][1]=0xffff0010; m.cfg[0][13]=0x40;
    m.cfg[0][16]=0x4809; m.cfg[0][18]=0x5009; m.cfg[0][20]=0x11;
    assert(drift_pci_cap(&p,8,9,0,&offset)==0 && offset==0x40);
    assert(drift_pci_cap(&p,8,9,1,&offset)==0 && offset==0x48);
    assert(drift_pci_cap(&p,8,0x11,0,&offset)==0 && offset==0x50);
    assert(drift_pci_cap(&p,8,9,2,&offset)==1);
    m.cfg[0][20]=0x4011;
    assert(drift_pci_cap(&p,8,9,2,&offset)==-2);
    m.cfg[0][20]=0x4111;
    assert(drift_pci_cap(&p,8,9,2,&offset)==-2);
    m.cfg[0][13]=0x3c;
    assert(drift_pci_cap(&p,8,9,0,&offset)==-2);
    m.cfg[0][13]=0xfc; m.cfg[0][63]=9;
    assert(drift_pci_cap(&p,8,9,0,&offset)==0 && offset==0xfc);
    m.cfg[0][4]=0x8000000c; m.cfg[0][5]=0x1234;
    assert(drift_pci_get_bar(&p,8,0,&bar)==0 && bar.address==UINT64_C(0x123480000000) && bar.width==64 && bar.prefetch==1 && !bar.io);
    assert(drift_pci_get_bar(&p,8,1,&bar)==-1);
    m.cfg[0][6]=0xc001;
    assert(drift_pci_get_bar(&p,8,2,&bar)==0 && bar.address==0xc000 && bar.io && bar.width==32);
    assert(drift_pci_get_bar(&p,8,6,&bar)==-1);
    m.cfg[0][7]=0x80000002;
    assert(drift_pci_get_bar(&p,8,3,&bar)==-2);
    m.cfg[0][3]=0x00010000;
    assert(drift_pci_get_bar(&p,8,2,&bar)==-1);
    assert(drift_pci_enable(&p,8,7)==0 && m.writes==1 && m.cfg[0][1]==0x17);
    assert(drift_pci_enable(&p,8,8)==-1 && m.writes==1);
    assert(drift_pci_enable(&p,32,7)==1 && m.writes==1);
    puts("pci: discovery, multifunction, capability loops, BARs and W1C checks passed");
    return 0;
}
