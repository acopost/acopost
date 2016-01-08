/*   Date : 2011/03/04, version 2.0                                         */
/*   Copyright (C) 2012, Jingchao Chen                                      */
/*   This library was written at Donghua University, China                  */
/*   Contact: chen-jc@dhu.edu.cn or chenjingchao@yahoo.com                  */
/*   Copyright (C) 2012-2016, Giulio Paci                                   */
/*                                                                          */
/* Permission to use, copy, modify, and distribute this software and its    */
/* documentation with or without modifications and for any purpose and      */
/* without fee is hereby granted, provided that any copyright notices       */
/* appear in all copies and that both those copyright notices and this      */
/* permission notice appear in supporting documentation, and that the       */
/* names of the contributors or copyright holders not be used in            */
/* advertising or publicity pertaining to distribution of the software      */
/* without specific prior permission.                                       */
/*                                                                          */
/* THE CONTRIBUTORS AND COPYRIGHT HOLDERS OF THIS SOFTWARE DISCLAIM ALL     */
/* WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED           */
/* WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL THE         */
/* CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT    */
/* OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS   */
/* OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE    */
/* OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE   */
/* OR PERFORMANCE OF THIS SOFTWARE                                          */

/* This file was initially downloaded from                                  */
/* https://docs.google.com/file/d/0BxAfEASHYgsIUFE0amdRWTFFbXM/edit?pli=1   */
/* and then modified by Giulio Paci, in order to make it fully C compliant  */
/* and to provide a more flexible API with respect to qsort.                */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "eqsort.h"

void memswap(void *a, void *b, size_t n)
{
	{
	register size_t __N = n;
	do
	{
		register char t;
		t = *(char*)a;
		*(char*)a = *(char*)b;
		*(char*)b = t;
		a = (char*)a+1;
		b = (char*)b+1;
	} while ( --__N );
	}
}

#define swap(a,b)				\
	{					\
		register size_t __N = es;	\
		char *__A = a;			\
		char *__B = b;			\
		do				\
		{				\
			register char t;	\
			t = *__A;		\
			*__A++ = *__B;		\
			*__B++ = t;		\
		} while ( --__N );		\
	}

#define p 16
#define beta1 256
#define beta2 512
// Symmetry Partition Sort
void SymPartitionSort(void *a, ssize_t s, size_t n, size_t es, int (*cmp)(const void *,const void *,void *), void *data)
{   char *pm,*pb,*pc,*pi,*pj;
    int i,v,vL,m,left,right,sp,eq,ineq,rc;
    left=right=0;
    while(1){
        if(n < 8){ //Insertion sort on small arrays
             for (s=1; s < n; s++)
		for (pb = (char*)a+s*es; cmp(pb-es,pb,data) > 0; ) {
                        swap(pb,pb-es); pb-=es; 
                        if(pb <= (char*)a) break;
                }
             return;
        }
        m= s<0 ? -s:s;
        if(m <= 2){//First,middle,last items are ordered and placed 1st,2nd and last
            v = beta2 > n ? n : 63;
            pc=(char*)a+(v-1)*es;
            pm=(char*)a+es; 
            swap(pm,(char*)a+(v/2)*es);
            if(cmp(a, pm,data) > 0) {swap(a,pm);}
	    if((cmp(pm, pc,data) > 0)) {
                      swap(pm,pc);
                      if((cmp(a, pm,data) > 0)) {swap(a,pm);}
                }
                left=right=1; pc-=es;
            }
            else{
               v=m > n/beta1 ? n : p*m-1;
               if(s < 0) {  //Move sorted items to left end
                      if(v<n) {left=m; s=-s;}
                      else    {left=(m+1)/2; right=m/2;} 
                      memswap(a, (char*)a+(n-m)*es, left*es);
                      left--;
               }
               if(s>0){
                      pb=(char*)a+m*es; pc=(char*)a+v*es;  
                      if(v < n){ //Extract sampling items 
                          sp=(n/v)*es; pj=pb; pi=pb;  
                          for(; pi < pc; pi+=es, pj+=sp) swap(pi,pj);
                      }
                      i=right=m/2; //Right move sorted items
                      do{ pb-=es; pc-=es; swap(pb,pc); i--;} while (i);
                      left=(m-1)/2; 
               }
               pm=(char*)a+left*es; pc=pm+(v-m)*es;
            }
//Fat partition begins
        pb=pi=pm+es;  
        do {
		while ( (rc=cmp(pb,pm,data)) < 0 ) pb+=es;
            if(pb >= pc) break;
            if(rc==0){
				if(pi!=pb) {swap(pb,pi);}
                 pi+=es; pb+=es;
                 continue;
            }
            while ((rc=cmp(pc,pm,data)) > 0 ) pc-=es;
            if(pb >= pc) break;
            swap(pb,pc);
            if(rc==0){
				if(pi!=pb) { swap(pb,pi);}
                pi+=es; 
            }
            pb+=es; pc-=es;
        } while (pb <= pc);
//Move equal-key items
        eq=pi-pm, ineq=pb-pi;
        if( ineq < eq) pi=pm+ineq;
        pc=pb;
        while (pm < pi ) { pc-=es; swap(pc,pm); pm+=es;} 
//Fat partition ends
            vL=(pb-(char*)a)/es; 
            if(right < v-vL) SymPartitionSort(pb, -right, v-vL, es, cmp, data);
            vL=vL-eq/es; 
            if(v < n){
		if(left < vL) SymPartitionSort(a, left,vL,es,cmp, data);
                s=v;  //Remove tail recursion
            }
            else{
                if(left >= vL) return;
                s=left; n=vL; //Remove tail recursion
            }
    }
}

// Adaptive Symmetry Partition Sort
void eqsort(void *a, size_t n, size_t es, int (*cmp)(const void *,const void *,void *),void *data)
{   char *pb,*pc,*pi,*pj;
    int i,j,ne,rc,D_inv,left,m,Rev=0;
  
//Find 1st run
    ne = n * es;
    for (i=es; i < ne; i+=es){
	    if((rc=cmp((char*)a+i-es,(char*)a+i,data)) != 0 ){
             if(Rev==0) Rev= rc < 0 ? 1 : -1;//Rev=1: increasing, -1: decreasing
             else if(rc*Rev > 0) break;
         }
    }
    D_inv= Rev*(i/es);   //D_inv: difference of inversions & orders
    for(j=i+es; j < ne; j+=(97*es)){
	    if((rc=cmp((char*)a+j-es,(char*)a+j,data)) < 0) D_inv++;
         if(rc>0) D_inv--;
    }
    pb=(char*)a+i-es;
    if(abs(D_inv) > n/512 ) {     
         if(Rev*D_inv < 0) {pb=a; Rev=-Rev;}  //If 1st run is reverse, re-find it
            pc=(char*)a+n*es; pj=pb;
            while(1){
                pj=pj+10*es; pi=pj-es;
                if(pj >= pc) break;
                while (pj < pc && Rev*cmp(pj-es, pj,data) <=0) pj+=es; //Find next run foreward
                while (pi > pb && Rev*cmp(pi-es, pi,data) <=0) pi-=es; //Find next run backward
                if(pj-pi < 4*es) continue;
                if(pb!=a) { //Find knots in 1st and 2nd run 
                      j=((pj-pi)/es)/2;
                      m=((pb-(char*)a)/es)/4;
                      if (j > m ) j=m;
                      for(i=0; i<j; i++) if(Rev*cmp(pb-i*es,pi+i*es,data) <= 0) break;
                      if(i>=j) continue;
                      pb=pb+(1-i)*es; pi=pi+i*es;
                }
                // Merge two runs by moving 2nd knot to 1st knot 
                if(pi!=pb) while(pi < pj ) { swap(pb,pi); pb+=es; pi+=es;}
                else pb=pj;
                pb=(char*)pb-es;
            }
    }   
    left=((char*)pb-(char*)a)/es+1;
    if(Rev==-1){ //if the longest run reverse, reverse it
        pc=a;
        while(pc < pb ) {swap(pc,pb); pc=(char*)pc+es; pb=(char*)pb-es; }
    }
    if(left < n) SymPartitionSort(a, left, n, es, cmp, data);
}
