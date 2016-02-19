.. _tree:

tree: data trees
================

This module defines data structures for different types of trees: splay trees
and red-black trees.


API
---


Splay tree
^^^^^^^^^^

A splay tree is a self-organizing data structure.  Every operation on the tree
causes a splay to happen.  The splay moves the requested node to the root of
the tree and partly rebalances it.

This has the benefit that request locality causes faster lookups as the
requested nodes move to the top of the tree.  On the other hand, every lookup
causes memory writes.

The Balance Theorem bounds the total access time for :math:`m` operations and
:math:`n` inserts on an initially empty tree as :math:`\mathcal{O}((m +
n)\log{} n)`. The amortized cost for a sequence of :math:`m` accesses to a
splay tree is :math:`\mathcal{O}(\log{} n)`.

.. c:macro:: DMR_SPLAY_HEAD(name, type)
.. c:macro:: DMR_SPLAY_INITIALIZER(root)
.. c:macro:: DMR_SPLAY_INIT(root)
.. c:macro:: DMR_SPLAY_ENTRY(type)
.. c:macro:: DMR_SPLAY_LEFT(elm, field)
.. c:macro:: DMR_SPLAY_RIGHT(elm, field)
.. c:macro:: DMR_SPLAY_ROOT(head)
.. c:macro:: DMR_SPLAY_EMPTY(head)
.. c:macro:: DMR_SPLAY_ROTATE_RIGHT(head, tmp, field)
.. c:macro:: DMR_SPLAY_LINKLEFT(head, tmp, field)
.. c:macro:: DMR_SPLAY_LINKRIGHT(head, tmp, field)
.. c:macro:: DMR_SPLAY_ASSEMBLE(head, node, left, right, field)
.. c:macro:: DMR_SPLAY_GENERATE(name, type, field, cmp)
.. c:macro:: DMR_SPLAY_NEGINF
.. c:macro:: DMR_SPLAY_INF
.. c:macro:: DMR_SPLAY_INSERT(name, x, y)
.. c:macro:: DMR_SPLAY_REMOVE(name, x, y)
.. c:macro:: DMR_SPLAY_FIND(name, x, y)
.. c:macro:: DMR_SPLAY_NEXT(name, x, y)
.. c:macro:: DMR_SPLAY_MIN(name, x)
.. c:macro:: DMR_SPLAY_MAX(name, x)
.. c:macro:: DMR_SPLAY_FOREACH(x, name, head)


Red-black tree
^^^^^^^^^^^^^^

A red-black tree is a binary search tree with the node color as an
extra attribute.  It fulfills a set of conditions:

 * every search path from the root to a leaf consists of the same number of
   black nodes,
 * each red node (except for the root) has a black parent,
 * each leaf node is black.

Every operation on a red-black tree is bounded as :math:`\mathcal{O}(\log{}
n)`. The maximum height of a red-black tree is :math:`2\log{(n+1)}`.

.. c:macro:: DMR_RB_BLACK
.. c:macro:: DMR_RB_RED
.. c:macro:: DMR_RB_NEGINF
.. c:macro:: DMR_RB_INF
.. c:macro:: DMR_RB_HEAD(name, type)
.. c:macro:: DMR_RB_INITIALIZER(root)
.. c:macro:: DMR_RB_INIT(root)
.. c:macro:: DMR_RB_ENTRY(type)
.. c:macro:: DMR_RB_LEFT(elm, field)
.. c:macro:: DMR_RB_RIGHT(elm, field)
.. c:macro:: DMR_RB_PARENT(elm, field)
.. c:macro:: DMR_RB_COLOR(elm, field)
.. c:macro:: DMR_RB_ROOT(head)
.. c:macro:: DMR_RB_EMPTY(head)
.. c:macro:: DMR_RB_SET(elm, parent, field)
.. c:macro:: DMR_RB_SET_BLACKRED(black, red, field)
.. c:macro:: DMR_RB_AUGMENT(x)
.. c:macro:: DMR_RB_ROTATE_LEFT(head, elm, tmp, field)
.. c:macro:: DMR_RB_ROTATE_RIGHT(head, elm, tmp, field)
.. c:macro:: DMR_RB_PROTOTYPE(name, type, field, cmp)
.. c:macro:: DMR_RB_PROTOTYPE_STATIC(name, type, field, cmp)
.. c:macro:: DMR_RB_GENERATE(name, type, field, cmp)
.. c:macro:: DMR_RB_GENERATE_STATIC(name, type, field, cmp)
.. c:macro:: DMR_RB_INSERT(name, x, y)
.. c:macro:: DMR_RB_REMOVE(name, x, y)
.. c:macro:: DMR_RB_FIND(name, x, y)
.. c:macro:: DMR_RB_NFIND(name, x, y)
.. c:macro:: DMR_RB_NEXT(name, x, y)
.. c:macro:: DMR_RB_PREV(name, x, y)
.. c:macro:: DMR_RB_MIN(name, x)
.. c:macro:: DMR_RB_MAX(name, x)
.. c:macro:: DMR_RB_FOREACH(x, name, head)
.. c:macro:: DMR_RB_FOREACH_FROM(x, name, y)
.. c:macro:: DMR_RB_FOREACH_SAFE(x, name, head, y)
.. c:macro:: DMR_RB_FOREACH_REVERSE(x, name, head)
.. c:macro:: DMR_RB_FOREACH_REVERSE_FROM(x, name, y)
.. c:macro:: DMR_RB_FOREACH_REVERSE_SAFE(x, name, head, y)
