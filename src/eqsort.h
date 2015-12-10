/*   Date : 2011/03/04, version 2.0                                         */
/*   Copyright (C) 2012, Jingchao Chen                                      */
/*   This library was written at Donghua University, China                  */
/*   Contact: chen-jc@dhu.edu.cn or chenjingchao@yahoo.com                  */
/*   Copyright (C) 2012-2015, Giulio Paci                                   */
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

#ifndef ADPSYMMETRYSORT_H
#define ADPSYMMETRYSORT_H


void memswap(void *a, void *b, size_t n);

void eqsort(void *a, size_t n, size_t es, int (*cmp)(const void *,const void *,void *),void *data);

#endif
