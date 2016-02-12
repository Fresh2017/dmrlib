/*	$NetBSD: tree.h,v 1.8 2004/03/28 19:38:30 provos Exp $	*/
/*	$OpenBSD: tree.h,v 1.7 2002/10/17 21:51:54 art Exp $	*/
/* $FreeBSD$ */

/*-
 * Copyright 2002 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_DMR_TREE_H_
#define	_DMR_TREE_H_

#include <sys/cdefs.h>

/*
 * This file defines data structures for different types of trees:
 * splay trees and red-black trees.
 *
 * A splay tree is a self-organizing data structure.  Every operation
 * on the tree causes a splay to happen.  The splay moves the requested
 * node to the root of the tree and partly rebalances it.
 *
 * This has the benefit that request locality causes faster lookups as
 * the requested nodes move to the top of the tree.  On the other hand,
 * every lookup causes memory writes.
 *
 * The Balance Theorem bounds the total access time for m operations
 * and n inserts on an initially empty tree as O((m + n)lg n).  The
 * amortized cost for a sequence of m accesses to a splay tree is O(lg n);
 *
 * A red-black tree is a binary search tree with the node color as an
 * extra attribute.  It fulfills a set of conditions:
 *	- every search path from the root to a leaf consists of the
 *	  same number of black nodes,
 *	- each red node (except for the root) has a black parent,
 *	- each leaf node is black.
 *
 * Every operation on a red-black tree is bounded as O(lg n).
 * The maximum height of a red-black tree is 2lg (n+1).
 */

#define DMR_SPLAY_HEAD(name, type)						\
struct name {								\
	struct type *sph_root; /* root of the tree */			\
}

#define DMR_SPLAY_INITIALIZER(root)						\
	{ NULL }

#define DMR_SPLAY_INIT(root) do {						\
	(root)->sph_root = NULL;					\
} while (/*CONSTCOND*/ 0)

#define DMR_SPLAY_ENTRY(type)						\
struct {								\
	struct type *spe_left; /* left element */			\
	struct type *spe_right; /* right element */			\
}

#define DMR_SPLAY_LEFT(elm, field)		(elm)->field.spe_left
#define DMR_SPLAY_RIGHT(elm, field)		(elm)->field.spe_right
#define DMR_SPLAY_ROOT(head)		(head)->sph_root
#define DMR_SPLAY_EMPTY(head)		(DMR_SPLAY_ROOT(head) == NULL)

/* DMR_SPLAY_ROTATE_{LEFT,RIGHT} expect that tmp hold DMR_SPLAY_{RIGHT,LEFT} */
#define DMR_SPLAY_ROTATE_RIGHT(head, tmp, field) do {			\
	DMR_SPLAY_LEFT((head)->sph_root, field) = DMR_SPLAY_RIGHT(tmp, field);	\
	DMR_SPLAY_RIGHT(tmp, field) = (head)->sph_root;			\
	(head)->sph_root = tmp;						\
} while (/*CONSTCOND*/ 0)
	
#define DMR_SPLAY_ROTATE_LEFT(head, tmp, field) do {			\
	DMR_SPLAY_RIGHT((head)->sph_root, field) = DMR_SPLAY_LEFT(tmp, field);	\
	DMR_SPLAY_LEFT(tmp, field) = (head)->sph_root;			\
	(head)->sph_root = tmp;						\
} while (/*CONSTCOND*/ 0)

#define DMR_SPLAY_LINKLEFT(head, tmp, field) do {				\
	DMR_SPLAY_LEFT(tmp, field) = (head)->sph_root;			\
	tmp = (head)->sph_root;						\
	(head)->sph_root = DMR_SPLAY_LEFT((head)->sph_root, field);		\
} while (/*CONSTCOND*/ 0)

#define DMR_SPLAY_LINKRIGHT(head, tmp, field) do {				\
	DMR_SPLAY_RIGHT(tmp, field) = (head)->sph_root;			\
	tmp = (head)->sph_root;						\
	(head)->sph_root = DMR_SPLAY_RIGHT((head)->sph_root, field);	\
} while (/*CONSTCOND*/ 0)

#define DMR_SPLAY_ASSEMBLE(head, node, left, right, field) do {		\
	DMR_SPLAY_RIGHT(left, field) = DMR_SPLAY_LEFT((head)->sph_root, field);	\
	DMR_SPLAY_LEFT(right, field) = DMR_SPLAY_RIGHT((head)->sph_root, field);\
	DMR_SPLAY_LEFT((head)->sph_root, field) = DMR_SPLAY_RIGHT(node, field);	\
	DMR_SPLAY_RIGHT((head)->sph_root, field) = DMR_SPLAY_LEFT(node, field);	\
} while (/*CONSTCOND*/ 0)

/* Generates prototypes and inline functions */

#define DMR_SPLAY_PROTOTYPE(name, type, field, cmp)				\
void name##_DMR_SPLAY(struct name *, struct type *);			\
void name##_DMR_SPLAY_MINMAX(struct name *, int);				\
struct type *name##_DMR_SPLAY_INSERT(struct name *, struct type *);		\
struct type *name##_DMR_SPLAY_REMOVE(struct name *, struct type *);		\
									\
/* Finds the node with the same key as elm */				\
static __inline struct type *						\
name##_DMR_SPLAY_FIND(struct name *head, struct type *elm)			\
{									\
	if (DMR_SPLAY_EMPTY(head))						\
		return(NULL);						\
	name##_DMR_SPLAY(head, elm);					\
	if ((cmp)(elm, (head)->sph_root) == 0)				\
		return (head->sph_root);				\
	return (NULL);							\
}									\
									\
static __inline struct type *						\
name##_DMR_SPLAY_NEXT(struct name *head, struct type *elm)			\
{									\
	name##_DMR_SPLAY(head, elm);					\
	if (DMR_SPLAY_RIGHT(elm, field) != NULL) {				\
		elm = DMR_SPLAY_RIGHT(elm, field);				\
		while (DMR_SPLAY_LEFT(elm, field) != NULL) {		\
			elm = DMR_SPLAY_LEFT(elm, field);			\
		}							\
	} else								\
		elm = NULL;						\
	return (elm);							\
}									\
									\
static __inline struct type *						\
name##_DMR_SPLAY_MIN_MAX(struct name *head, int val)			\
{									\
	name##_DMR_SPLAY_MINMAX(head, val);					\
        return (DMR_SPLAY_ROOT(head));					\
}

/* Main splay operation.
 * Moves node close to the key of elm to top
 */
#define DMR_SPLAY_GENERATE(name, type, field, cmp)				\
struct type *								\
name##_DMR_SPLAY_INSERT(struct name *head, struct type *elm)		\
{									\
    if (DMR_SPLAY_EMPTY(head)) {						\
	    DMR_SPLAY_LEFT(elm, field) = DMR_SPLAY_RIGHT(elm, field) = NULL;	\
    } else {								\
	    int __comp;							\
	    name##_DMR_SPLAY(head, elm);					\
	    __comp = (cmp)(elm, (head)->sph_root);			\
	    if(__comp < 0) {						\
		    DMR_SPLAY_LEFT(elm, field) = DMR_SPLAY_LEFT((head)->sph_root, field);\
		    DMR_SPLAY_RIGHT(elm, field) = (head)->sph_root;		\
		    DMR_SPLAY_LEFT((head)->sph_root, field) = NULL;		\
	    } else if (__comp > 0) {					\
		    DMR_SPLAY_RIGHT(elm, field) = DMR_SPLAY_RIGHT((head)->sph_root, field);\
		    DMR_SPLAY_LEFT(elm, field) = (head)->sph_root;		\
		    DMR_SPLAY_RIGHT((head)->sph_root, field) = NULL;	\
	    } else							\
		    return ((head)->sph_root);				\
    }									\
    (head)->sph_root = (elm);						\
    return (NULL);							\
}									\
									\
struct type *								\
name##_DMR_SPLAY_REMOVE(struct name *head, struct type *elm)		\
{									\
	struct type *__tmp;						\
	if (DMR_SPLAY_EMPTY(head))						\
		return (NULL);						\
	name##_DMR_SPLAY(head, elm);					\
	if ((cmp)(elm, (head)->sph_root) == 0) {			\
		if (DMR_SPLAY_LEFT((head)->sph_root, field) == NULL) {	\
			(head)->sph_root = DMR_SPLAY_RIGHT((head)->sph_root, field);\
		} else {						\
			__tmp = DMR_SPLAY_RIGHT((head)->sph_root, field);	\
			(head)->sph_root = DMR_SPLAY_LEFT((head)->sph_root, field);\
			name##_DMR_SPLAY(head, elm);			\
			DMR_SPLAY_RIGHT((head)->sph_root, field) = __tmp;	\
		}							\
		return (elm);						\
	}								\
	return (NULL);							\
}									\
									\
void									\
name##_DMR_SPLAY(struct name *head, struct type *elm)			\
{									\
	struct type __node, *__left, *__right, *__tmp;			\
	int __comp;							\
\
	DMR_SPLAY_LEFT(&__node, field) = DMR_SPLAY_RIGHT(&__node, field) = NULL;\
	__left = __right = &__node;					\
\
	while ((__comp = (cmp)(elm, (head)->sph_root)) != 0) {		\
		if (__comp < 0) {					\
			__tmp = DMR_SPLAY_LEFT((head)->sph_root, field);	\
			if (__tmp == NULL)				\
				break;					\
			if ((cmp)(elm, __tmp) < 0){			\
				DMR_SPLAY_ROTATE_RIGHT(head, __tmp, field);	\
				if (DMR_SPLAY_LEFT((head)->sph_root, field) == NULL)\
					break;				\
			}						\
			DMR_SPLAY_LINKLEFT(head, __right, field);		\
		} else if (__comp > 0) {				\
			__tmp = DMR_SPLAY_RIGHT((head)->sph_root, field);	\
			if (__tmp == NULL)				\
				break;					\
			if ((cmp)(elm, __tmp) > 0){			\
				DMR_SPLAY_ROTATE_LEFT(head, __tmp, field);	\
				if (DMR_SPLAY_RIGHT((head)->sph_root, field) == NULL)\
					break;				\
			}						\
			DMR_SPLAY_LINKRIGHT(head, __left, field);		\
		}							\
	}								\
	DMR_SPLAY_ASSEMBLE(head, &__node, __left, __right, field);		\
}									\
									\
/* Splay with either the minimum or the maximum element			\
 * Used to find minimum or maximum element in tree.			\
 */									\
void name##_DMR_SPLAY_MINMAX(struct name *head, int __comp) \
{									\
	struct type __node, *__left, *__right, *__tmp;			\
\
	DMR_SPLAY_LEFT(&__node, field) = DMR_SPLAY_RIGHT(&__node, field) = NULL;\
	__left = __right = &__node;					\
\
	while (1) {							\
		if (__comp < 0) {					\
			__tmp = DMR_SPLAY_LEFT((head)->sph_root, field);	\
			if (__tmp == NULL)				\
				break;					\
			if (__comp < 0){				\
				DMR_SPLAY_ROTATE_RIGHT(head, __tmp, field);	\
				if (DMR_SPLAY_LEFT((head)->sph_root, field) == NULL)\
					break;				\
			}						\
			DMR_SPLAY_LINKLEFT(head, __right, field);		\
		} else if (__comp > 0) {				\
			__tmp = DMR_SPLAY_RIGHT((head)->sph_root, field);	\
			if (__tmp == NULL)				\
				break;					\
			if (__comp > 0) {				\
				DMR_SPLAY_ROTATE_LEFT(head, __tmp, field);	\
				if (DMR_SPLAY_RIGHT((head)->sph_root, field) == NULL)\
					break;				\
			}						\
			DMR_SPLAY_LINKRIGHT(head, __left, field);		\
		}							\
	}								\
	DMR_SPLAY_ASSEMBLE(head, &__node, __left, __right, field);		\
}

#define DMR_SPLAY_NEGINF	-1
#define DMR_SPLAY_INF	1

#define DMR_SPLAY_INSERT(name, x, y)	name##_DMR_SPLAY_INSERT(x, y)
#define DMR_SPLAY_REMOVE(name, x, y)	name##_DMR_SPLAY_REMOVE(x, y)
#define DMR_SPLAY_FIND(name, x, y)		name##_DMR_SPLAY_FIND(x, y)
#define DMR_SPLAY_NEXT(name, x, y)		name##_DMR_SPLAY_NEXT(x, y)
#define DMR_SPLAY_MIN(name, x)		(DMR_SPLAY_EMPTY(x) ? NULL	\
					: name##_DMR_SPLAY_MIN_MAX(x, DMR_SPLAY_NEGINF))
#define DMR_SPLAY_MAX(name, x)		(DMR_SPLAY_EMPTY(x) ? NULL	\
					: name##_DMR_SPLAY_MIN_MAX(x, DMR_SPLAY_INF))

#define DMR_SPLAY_FOREACH(x, name, head)					\
	for ((x) = DMR_SPLAY_MIN(name, head);				\
	     (x) != NULL;						\
	     (x) = DMR_SPLAY_NEXT(name, head, x))

/* Macros that define a red-black tree */
#define DMR_RB_HEAD(name, type)						\
struct name {								\
	struct type *rbh_root; /* root of the tree */			\
}

#define DMR_RB_INITIALIZER(root)						\
	{ NULL }

#define DMR_RB_INIT(root) do {						\
	(root)->rbh_root = NULL;					\
} while (/*CONSTCOND*/ 0)

#define DMR_RB_BLACK	0
#define DMR_RB_RED		1
#define DMR_RB_ENTRY(type)							\
struct {								\
	struct type *rbe_left;		/* left element */		\
	struct type *rbe_right;		/* right element */		\
	struct type *rbe_parent;	/* parent element */		\
	int rbe_color;			/* node color */		\
}

#define DMR_RB_LEFT(elm, field)		(elm)->field.rbe_left
#define DMR_RB_RIGHT(elm, field)		(elm)->field.rbe_right
#define DMR_RB_PARENT(elm, field)		(elm)->field.rbe_parent
#define DMR_RB_COLOR(elm, field)		(elm)->field.rbe_color
#define DMR_RB_ROOT(head)			(head)->rbh_root
#define DMR_RB_EMPTY(head)			(DMR_RB_ROOT(head) == NULL)

#define DMR_RB_SET(elm, parent, field) do {					\
	DMR_RB_PARENT(elm, field) = parent;					\
	DMR_RB_LEFT(elm, field) = DMR_RB_RIGHT(elm, field) = NULL;		\
	DMR_RB_COLOR(elm, field) = DMR_RB_RED;					\
} while (/*CONSTCOND*/ 0)

#define DMR_RB_SET_BLACKRED(black, red, field) do {				\
	DMR_RB_COLOR(black, field) = DMR_RB_BLACK;				\
	DMR_RB_COLOR(red, field) = DMR_RB_RED;					\
} while (/*CONSTCOND*/ 0)

#ifndef DMR_RB_AUGMENT
#define DMR_RB_AUGMENT(x)	do {} while (0)
#endif

#define DMR_RB_ROTATE_LEFT(head, elm, tmp, field) do {			\
	(tmp) = DMR_RB_RIGHT(elm, field);					\
	if ((DMR_RB_RIGHT(elm, field) = DMR_RB_LEFT(tmp, field)) != NULL) {	\
		DMR_RB_PARENT(DMR_RB_LEFT(tmp, field), field) = (elm);		\
	}								\
	DMR_RB_AUGMENT(elm);						\
	if ((DMR_RB_PARENT(tmp, field) = DMR_RB_PARENT(elm, field)) != NULL) {	\
		if ((elm) == DMR_RB_LEFT(DMR_RB_PARENT(elm, field), field))	\
			DMR_RB_LEFT(DMR_RB_PARENT(elm, field), field) = (tmp);	\
		else							\
			DMR_RB_RIGHT(DMR_RB_PARENT(elm, field), field) = (tmp);	\
	} else								\
		(head)->rbh_root = (tmp);				\
	DMR_RB_LEFT(tmp, field) = (elm);					\
	DMR_RB_PARENT(elm, field) = (tmp);					\
	DMR_RB_AUGMENT(tmp);						\
	if ((DMR_RB_PARENT(tmp, field)))					\
		DMR_RB_AUGMENT(DMR_RB_PARENT(tmp, field));			\
} while (/*CONSTCOND*/ 0)

#define DMR_RB_ROTATE_RIGHT(head, elm, tmp, field) do {			\
	(tmp) = DMR_RB_LEFT(elm, field);					\
	if ((DMR_RB_LEFT(elm, field) = DMR_RB_RIGHT(tmp, field)) != NULL) {	\
		DMR_RB_PARENT(DMR_RB_RIGHT(tmp, field), field) = (elm);		\
	}								\
	DMR_RB_AUGMENT(elm);						\
	if ((DMR_RB_PARENT(tmp, field) = DMR_RB_PARENT(elm, field)) != NULL) {	\
		if ((elm) == DMR_RB_LEFT(DMR_RB_PARENT(elm, field), field))	\
			DMR_RB_LEFT(DMR_RB_PARENT(elm, field), field) = (tmp);	\
		else							\
			DMR_RB_RIGHT(DMR_RB_PARENT(elm, field), field) = (tmp);	\
	} else								\
		(head)->rbh_root = (tmp);				\
	DMR_RB_RIGHT(tmp, field) = (elm);					\
	DMR_RB_PARENT(elm, field) = (tmp);					\
	DMR_RB_AUGMENT(tmp);						\
	if ((DMR_RB_PARENT(tmp, field)))					\
		DMR_RB_AUGMENT(DMR_RB_PARENT(tmp, field));			\
} while (/*CONSTCOND*/ 0)

/* Generates prototypes and inline functions */
#define	DMR_RB_PROTOTYPE(name, type, field, cmp)				\
	DMR_RB_PROTOTYPE_INTERNAL(name, type, field, cmp,)
#define	DMR_RB_PROTOTYPE_STATIC(name, type, field, cmp)			\
	DMR_RB_PROTOTYPE_INTERNAL(name, type, field, cmp, __unused static)
#define DMR_RB_PROTOTYPE_INTERNAL(name, type, field, cmp, attr)		\
attr void name##_DMR_RB_INSERT_COLOR(struct name *, struct type *);		\
attr void name##_DMR_RB_REMOVE_COLOR(struct name *, struct type *, struct type *);\
attr struct type *name##_DMR_RB_REMOVE(struct name *, struct type *);	\
attr struct type *name##_DMR_RB_INSERT(struct name *, struct type *);	\
attr struct type *name##_DMR_RB_FIND(struct name *, struct type *);		\
attr struct type *name##_DMR_RB_NFIND(struct name *, struct type *);	\
attr struct type *name##_DMR_RB_NEXT(struct type *);			\
attr struct type *name##_DMR_RB_PREV(struct type *);			\
attr struct type *name##_DMR_RB_MINMAX(struct name *, int);			\
									\

/* Main rb operation.
 * Moves node close to the key of elm to top
 */
#define	DMR_RB_GENERATE(name, type, field, cmp)				        \
	DMR_RB_GENERATE_INTERNAL(name, type, field, cmp,)
#define	DMR_RB_GENERATE_STATIC(name, type, field, cmp)			    \
	DMR_RB_GENERATE_INTERNAL(name, type, field, cmp, __unused static)
#define DMR_RB_GENERATE_INTERNAL(name, type, field, cmp, attr)		\
attr void								                            \
name##_DMR_RB_INSERT_COLOR(struct name *head, struct type *elm)		\
{									                                \
	struct type *parent, *gparent, *tmp;				            \
	while ((parent = DMR_RB_PARENT(elm, field)) != NULL &&		    \
	    DMR_RB_COLOR(parent, field) == DMR_RB_RED) {			    \
		gparent = DMR_RB_PARENT(parent, field);			            \
		if (parent == DMR_RB_LEFT(gparent, field)) {		        \
			tmp = DMR_RB_RIGHT(gparent, field);			            \
			if (tmp && DMR_RB_COLOR(tmp, field) == DMR_RB_RED) {	\
				DMR_RB_COLOR(tmp, field) = DMR_RB_BLACK;	        \
				DMR_RB_SET_BLACKRED(parent, gparent, field);        \
				elm = gparent;				                        \
				continue;				                            \
			}						                                \
			if (DMR_RB_RIGHT(parent, field) == elm) {		        \
				DMR_RB_ROTATE_LEFT(head, parent, tmp, field);       \
				tmp = parent;				                        \
				parent = elm;				                        \
				elm = tmp;				                            \
			}						                                \
			DMR_RB_SET_BLACKRED(parent, gparent, field);	        \
			DMR_RB_ROTATE_RIGHT(head, gparent, tmp, field);	        \
		} else {						                            \
			tmp = DMR_RB_LEFT(gparent, field);			            \
			if (tmp && DMR_RB_COLOR(tmp, field) == DMR_RB_RED) {	\
				DMR_RB_COLOR(tmp, field) = DMR_RB_BLACK;	        \
				DMR_RB_SET_BLACKRED(parent, gparent, field);        \
				elm = gparent;				                        \
				continue;				                            \
			}						                                \
			if (DMR_RB_LEFT(parent, field) == elm) {		        \
				DMR_RB_ROTATE_RIGHT(head, parent, tmp, field);      \
				tmp = parent;				                        \
				parent = elm;				                        \
				elm = tmp;				                            \
			}						                                \
			DMR_RB_SET_BLACKRED(parent, gparent, field);	        \
			DMR_RB_ROTATE_LEFT(head, gparent, tmp, field);	        \
		}							                                \
	}								                                \
	DMR_RB_COLOR(head->rbh_root, field) = DMR_RB_BLACK;			    \
}									                                \
									                                \
attr void								                            \
name##_DMR_RB_REMOVE_COLOR(struct name *head, struct type *parent, struct type *elm) \
{									                                \
	struct type *tmp;						                        \
	while ((elm == NULL || DMR_RB_COLOR(elm, field) == DMR_RB_BLACK) &&	\
	    elm != DMR_RB_ROOT(head)) {					                \
		if (DMR_RB_LEFT(parent, field) == elm) {			        \
			tmp = DMR_RB_RIGHT(parent, field);			            \
			if (DMR_RB_COLOR(tmp, field) == DMR_RB_RED) {		    \
				DMR_RB_SET_BLACKRED(tmp, parent, field);	        \
				DMR_RB_ROTATE_LEFT(head, parent, tmp, field);       \
				tmp = DMR_RB_RIGHT(parent, field);		            \
			}						                                \
			if ((DMR_RB_LEFT(tmp, field) == NULL ||		            \
			    DMR_RB_COLOR(DMR_RB_LEFT(tmp, field), field) == DMR_RB_BLACK) &&\
			    (DMR_RB_RIGHT(tmp, field) == NULL ||		        \
			    DMR_RB_COLOR(DMR_RB_RIGHT(tmp, field), field) == DMR_RB_BLACK)) {\
				DMR_RB_COLOR(tmp, field) = DMR_RB_RED;		        \
				elm = parent;				                        \
				parent = DMR_RB_PARENT(elm, field);		            \
			} else {					                            \
				if (DMR_RB_RIGHT(tmp, field) == NULL ||	            \
				    DMR_RB_COLOR(DMR_RB_RIGHT(tmp, field), field) == DMR_RB_BLACK) {\
					struct type *oleft;		                        \
					if ((oleft = DMR_RB_LEFT(tmp, field))           \
					    != NULL)			                        \
						DMR_RB_COLOR(oleft, field) = DMR_RB_BLACK;  \
					DMR_RB_COLOR(tmp, field) = DMR_RB_RED;	        \
					DMR_RB_ROTATE_RIGHT(head, tmp, oleft, field);   \
					tmp = DMR_RB_RIGHT(parent, field);	            \
				}					                                \
				DMR_RB_COLOR(tmp, field) = DMR_RB_COLOR(parent, field);\
				DMR_RB_COLOR(parent, field) = DMR_RB_BLACK;	        \
				if (DMR_RB_RIGHT(tmp, field))		                \
					DMR_RB_COLOR(DMR_RB_RIGHT(tmp, field), field) = DMR_RB_BLACK;\
				DMR_RB_ROTATE_LEFT(head, parent, tmp, field);       \
				elm = DMR_RB_ROOT(head);			                \
				break;					                            \
			}						                                \
		} else {						                            \
			tmp = DMR_RB_LEFT(parent, field);			            \
			if (DMR_RB_COLOR(tmp, field) == DMR_RB_RED) {		    \
				DMR_RB_SET_BLACKRED(tmp, parent, field);	        \
				DMR_RB_ROTATE_RIGHT(head, parent, tmp, field);      \
				tmp = DMR_RB_LEFT(parent, field);		            \
			}						                                \
			if ((DMR_RB_LEFT(tmp, field) == NULL ||		            \
			    DMR_RB_COLOR(DMR_RB_LEFT(tmp, field), field) == DMR_RB_BLACK) &&\
			    (DMR_RB_RIGHT(tmp, field) == NULL ||		        \
			    DMR_RB_COLOR(DMR_RB_RIGHT(tmp, field), field) == DMR_RB_BLACK)) {\
				DMR_RB_COLOR(tmp, field) = DMR_RB_RED;		        \
				elm = parent;				                        \
				parent = DMR_RB_PARENT(elm, field);		            \
			} else {					                            \
				if (DMR_RB_LEFT(tmp, field) == NULL ||	            \
				    DMR_RB_COLOR(DMR_RB_LEFT(tmp, field), field) == DMR_RB_BLACK) {\
					struct type *oright;		                    \
					if ((oright = DMR_RB_RIGHT(tmp, field))         \
					    != NULL)			                        \
						DMR_RB_COLOR(oright, field) = DMR_RB_BLACK; \
					DMR_RB_COLOR(tmp, field) = DMR_RB_RED;	        \
					DMR_RB_ROTATE_LEFT(head, tmp, oright, field);   \
					tmp = DMR_RB_LEFT(parent, field);	            \
				}					                                \
				DMR_RB_COLOR(tmp, field) = DMR_RB_COLOR(parent, field);\
				DMR_RB_COLOR(parent, field) = DMR_RB_BLACK;	        \
				if (DMR_RB_LEFT(tmp, field))		                \
					DMR_RB_COLOR(DMR_RB_LEFT(tmp, field), field) = DMR_RB_BLACK;\
				DMR_RB_ROTATE_RIGHT(head, parent, tmp, field);      \
				elm = DMR_RB_ROOT(head);			                \
				break;					                            \
			}						                                \
		}							                                \
	}								                                \
	if (elm)							                            \
		DMR_RB_COLOR(elm, field) = DMR_RB_BLACK;			        \
}									                                \
									                                \
attr struct type *							\
name##_DMR_RB_REMOVE(struct name *head, struct type *elm)			\
{									\
	struct type *child, *parent, *old = elm;			\
	int color;							\
	if (DMR_RB_LEFT(elm, field) == NULL)				\
		child = DMR_RB_RIGHT(elm, field);				\
	else if (DMR_RB_RIGHT(elm, field) == NULL)				\
		child = DMR_RB_LEFT(elm, field);				\
	else {								\
		struct type *left;					\
		elm = DMR_RB_RIGHT(elm, field);				\
		while ((left = DMR_RB_LEFT(elm, field)) != NULL)		\
			elm = left;					\
		child = DMR_RB_RIGHT(elm, field);				\
		parent = DMR_RB_PARENT(elm, field);				\
		color = DMR_RB_COLOR(elm, field);				\
		if (child)						\
			DMR_RB_PARENT(child, field) = parent;		\
		if (parent) {						\
			if (DMR_RB_LEFT(parent, field) == elm)		\
				DMR_RB_LEFT(parent, field) = child;		\
			else						\
				DMR_RB_RIGHT(parent, field) = child;	\
			DMR_RB_AUGMENT(parent);				\
		} else							\
			DMR_RB_ROOT(head) = child;				\
		if (DMR_RB_PARENT(elm, field) == old)			\
			parent = elm;					\
		(elm)->field = (old)->field;				\
		if (DMR_RB_PARENT(old, field)) {				\
			if (DMR_RB_LEFT(DMR_RB_PARENT(old, field), field) == old)\
				DMR_RB_LEFT(DMR_RB_PARENT(old, field), field) = elm;\
			else						\
				DMR_RB_RIGHT(DMR_RB_PARENT(old, field), field) = elm;\
			DMR_RB_AUGMENT(DMR_RB_PARENT(old, field));		\
		} else							\
			DMR_RB_ROOT(head) = elm;				\
		DMR_RB_PARENT(DMR_RB_LEFT(old, field), field) = elm;		\
		if (DMR_RB_RIGHT(old, field))				\
			DMR_RB_PARENT(DMR_RB_RIGHT(old, field), field) = elm;	\
		if (parent) {						\
			left = parent;					\
			do {						\
				DMR_RB_AUGMENT(left);			\
			} while ((left = DMR_RB_PARENT(left, field)) != NULL); \
		}							\
		goto color;						\
	}								\
	parent = DMR_RB_PARENT(elm, field);					\
	color = DMR_RB_COLOR(elm, field);					\
	if (child)							\
		DMR_RB_PARENT(child, field) = parent;			\
	if (parent) {							\
		if (DMR_RB_LEFT(parent, field) == elm)			\
			DMR_RB_LEFT(parent, field) = child;			\
		else							\
			DMR_RB_RIGHT(parent, field) = child;		\
		DMR_RB_AUGMENT(parent);					\
	} else								\
		DMR_RB_ROOT(head) = child;					\
color:									\
	if (color == DMR_RB_BLACK)						\
		name##_DMR_RB_REMOVE_COLOR(head, parent, child);		\
	return (old);							\
}									\
									\
/* Inserts a node into the RB tree */					\
attr struct type *							\
name##_DMR_RB_INSERT(struct name *head, struct type *elm)			\
{									\
	struct type *tmp;						\
	struct type *parent = NULL;					\
	int comp = 0;							\
	tmp = DMR_RB_ROOT(head);						\
	while (tmp) {							\
		parent = tmp;						\
		comp = (cmp)(elm, parent);				\
		if (comp < 0)						\
			tmp = DMR_RB_LEFT(tmp, field);			\
		else if (comp > 0)					\
			tmp = DMR_RB_RIGHT(tmp, field);			\
		else							\
			return (tmp);					\
	}								\
	DMR_RB_SET(elm, parent, field);					\
	if (parent != NULL) {						\
		if (comp < 0)						\
			DMR_RB_LEFT(parent, field) = elm;			\
		else							\
			DMR_RB_RIGHT(parent, field) = elm;			\
		DMR_RB_AUGMENT(parent);					\
	} else								\
		DMR_RB_ROOT(head) = elm;					\
	name##_DMR_RB_INSERT_COLOR(head, elm);				\
	return (NULL);							\
}									\
									\
/* Finds the node with the same key as elm */				\
attr struct type *							\
name##_DMR_RB_FIND(struct name *head, struct type *elm)			\
{									\
	struct type *tmp = DMR_RB_ROOT(head);				\
	int comp;							\
	while (tmp) {							\
		comp = cmp(elm, tmp);					\
		if (comp < 0)						\
			tmp = DMR_RB_LEFT(tmp, field);			\
		else if (comp > 0)					\
			tmp = DMR_RB_RIGHT(tmp, field);			\
		else							\
			return (tmp);					\
	}								\
	return (NULL);							\
}									\
									\
/* Finds the first node greater than or equal to the search key */	\
attr struct type *							\
name##_DMR_RB_NFIND(struct name *head, struct type *elm)			\
{									\
	struct type *tmp = DMR_RB_ROOT(head);				\
	struct type *res = NULL;					\
	int comp;							\
	while (tmp) {							\
		comp = cmp(elm, tmp);					\
		if (comp < 0) {						\
			res = tmp;					\
			tmp = DMR_RB_LEFT(tmp, field);			\
		}							\
		else if (comp > 0)					\
			tmp = DMR_RB_RIGHT(tmp, field);			\
		else							\
			return (tmp);					\
	}								\
	return (res);							\
}									\
									\
/* ARGSUSED */								\
attr struct type *							\
name##_DMR_RB_NEXT(struct type *elm)					\
{									\
	if (DMR_RB_RIGHT(elm, field)) {					\
		elm = DMR_RB_RIGHT(elm, field);				\
		while (DMR_RB_LEFT(elm, field))				\
			elm = DMR_RB_LEFT(elm, field);			\
	} else {							\
		if (DMR_RB_PARENT(elm, field) &&				\
		    (elm == DMR_RB_LEFT(DMR_RB_PARENT(elm, field), field)))	\
			elm = DMR_RB_PARENT(elm, field);			\
		else {							\
			while (DMR_RB_PARENT(elm, field) &&			\
			    (elm == DMR_RB_RIGHT(DMR_RB_PARENT(elm, field), field)))\
				elm = DMR_RB_PARENT(elm, field);		\
			elm = DMR_RB_PARENT(elm, field);			\
		}							\
	}								\
	return (elm);							\
}									\
									\
/* ARGSUSED */								\
attr struct type *							\
name##_DMR_RB_PREV(struct type *elm)					\
{									\
	if (DMR_RB_LEFT(elm, field)) {					\
		elm = DMR_RB_LEFT(elm, field);				\
		while (DMR_RB_RIGHT(elm, field))				\
			elm = DMR_RB_RIGHT(elm, field);			\
	} else {							\
		if (DMR_RB_PARENT(elm, field) &&				\
		    (elm == DMR_RB_RIGHT(DMR_RB_PARENT(elm, field), field)))	\
			elm = DMR_RB_PARENT(elm, field);			\
		else {							\
			while (DMR_RB_PARENT(elm, field) &&			\
			    (elm == DMR_RB_LEFT(DMR_RB_PARENT(elm, field), field)))\
				elm = DMR_RB_PARENT(elm, field);		\
			elm = DMR_RB_PARENT(elm, field);			\
		}							\
	}								\
	return (elm);							\
}									\
									\
attr struct type *							\
name##_DMR_RB_MINMAX(struct name *head, int val)				\
{									\
	struct type *tmp = DMR_RB_ROOT(head);				\
	struct type *parent = NULL;					\
	while (tmp) {							\
		parent = tmp;						\
		if (val < 0)						\
			tmp = DMR_RB_LEFT(tmp, field);			\
		else							\
			tmp = DMR_RB_RIGHT(tmp, field);			\
	}								\
	return (parent);						\
}

#define DMR_RB_NEGINF	-1
#define DMR_RB_INF	1

#define DMR_RB_INSERT(name, x, y)	name##_DMR_RB_INSERT(x, y)
#define DMR_RB_REMOVE(name, x, y)	name##_DMR_RB_REMOVE(x, y)
#define DMR_RB_FIND(name, x, y)	name##_DMR_RB_FIND(x, y)
#define DMR_RB_NFIND(name, x, y)	name##_DMR_RB_NFIND(x, y)
#define DMR_RB_NEXT(name, x, y)	name##_DMR_RB_NEXT(y)
#define DMR_RB_PREV(name, x, y)	name##_DMR_RB_PREV(y)
#define DMR_RB_MIN(name, x)		name##_DMR_RB_MINMAX(x, DMR_RB_NEGINF)
#define DMR_RB_MAX(name, x)		name##_DMR_RB_MINMAX(x, DMR_RB_INF)

#define DMR_RB_FOREACH(x, name, head)					\
	for ((x) = DMR_RB_MIN(name, head);					\
	     (x) != NULL;						\
	     (x) = name##_DMR_RB_NEXT(x))

#define DMR_RB_FOREACH_FROM(x, name, y)					\
	for ((x) = (y);							\
	    ((x) != NULL) && ((y) = name##_DMR_RB_NEXT(x), (x) != NULL);	\
	     (x) = (y))

#define DMR_RB_FOREACH_SAFE(x, name, head, y)				\
	for ((x) = DMR_RB_MIN(name, head);					\
	    ((x) != NULL) && ((y) = name##_DMR_RB_NEXT(x), (x) != NULL);	\
	     (x) = (y))

#define DMR_RB_FOREACH_REVERSE(x, name, head)				\
	for ((x) = DMR_RB_MAX(name, head);					\
	     (x) != NULL;						\
	     (x) = name##_DMR_RB_PREV(x))

#define DMR_RB_FOREACH_REVERSE_FROM(x, name, y)				\
	for ((x) = (y);							\
	    ((x) != NULL) && ((y) = name##_DMR_RB_PREV(x), (x) != NULL);	\
	     (x) = (y))

#define DMR_RB_FOREACH_REVERSE_SAFE(x, name, head, y)			\
	for ((x) = DMR_RB_MAX(name, head);					\
	    ((x) != NULL) && ((y) = name##_DMR_RB_PREV(x), (x) != NULL);	\
	     (x) = (y))

#endif	/* _DMR_TREE_H_ */
