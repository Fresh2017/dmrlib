.. _queue:

queue: Berkeley queues
======================

This file defines five types of data structures: singly-linked lists, lists,
simple queues, tail queues and XOR simple queues.

API
---

Singly-linked list
^^^^^^^^^^^^^^^^^^
 
A singly-linked list is headed by a single forward pointer. The elements are
singly linked for minimum space and pointer manipulation overhead at the
expense of :math:`\mathcal{O}(n)` removal for arbitrary elements. New elements
can be added to the list after an existing element or at the head of the list.
Elements being removed from the head of the list should use the explicit macro
for this purpose for optimum efficiency. A singly-linked list may only be
traversed in the forward direction.  Singly-linked lists are ideal for
applications with large datasets and few or no removals or for implementing a
LIFO queue.

.. c:macro:: DMR_SLIST_HEAD(name, type)
.. c:macro:: DMR_SLIST_HEAD_INITIALIZER(head)
.. c:macro:: DMR_SLIST_ENTRY(type)
.. c:macro:: DMR_SLIST_FIRST(head)
.. c:macro:: DMR_SLIST_END(head)
.. c:macro:: DMR_SLIST_EMPTY(head)
.. c:macro:: DMR_SLIST_NEXT(elm, field)
.. c:macro:: DMR_SLIST_FOREACH(var, head, field)
.. c:macro:: DMR_SLIST_FOREACH_SAFE(var, head, field, tvar)
.. c:macro:: DMR_SLIST_INIT(head)
.. c:macro:: DMR_SLIST_INSERT_AFTER(slistelm, elm, field)
.. c:macro:: DMR_SLIST_INSERT_HEAD(head, elm, field)
.. c:macro:: DMR_SLIST_REMOVE_AFTER(elm, field)
.. c:macro:: DMR_SLIST_REMOVE_HEAD(head, field)
.. c:macro:: DMR_SLIST_REMOVE(head, elm, type, field)


List
^^^^

A list is headed by a single forward pointer (or an array of forward pointers
for a hash table header). The elements are doubly linked so that an arbitrary
element can be removed without a need to traverse the list. New elements can be
added to the list before or after an existing element or at the head of the
list. A list may only be traversed in the forward direction.

.. c:macro:: DMR_LIST_HEAD(name, type)
.. c:macro:: DMR_LIST_HEAD_INITIALIZER(head)
.. c:macro:: DMR_LIST_ENTRY(type)
.. c:macro:: DMR_LIST_FIRST(head)
.. c:macro:: DMR_LIST_END(head)
.. c:macro:: DMR_LIST_EMPTY(head)
.. c:macro:: DMR_LIST_NEXT(elm, field)
.. c:macro:: DMR_LIST_FOREACH(var, head, field)
.. c:macro:: DMR_LIST_FOREACH_SAFE(var, head, field, tvar)
.. c:macro:: DMR_LIST_INIT(head)
.. c:macro:: DMR_LIST_INSERT_AFTER(listelm, elm, field)
.. c:macro:: DMR_LIST_INSERT_BEFORE(listelm, elm, field)
.. c:macro:: DMR_LIST_INSERT_HEAD(head, elm, field)
.. c:macro:: DMR_LIST_REMOVE(elm, field)
.. c:macro:: DMR_LIST_REPLACE(elm, elm2, field)


Simple queue
^^^^^^^^^^^^

A simple queue is headed by a pair of pointers, one to the head of the list and
the other to the tail of the list. The elements are singly linked to save
space, so elements can only be removed from the head of the list. New elements
can be added to the list before or after an existing element, at the head of
the list, or at the end of the list. A simple queue may only be traversed in
the forward direction.

.. c:macro:: DMR_SIMPLEQ_HEAD(name, type)
.. c:macro:: DMR_SIMPLEQ_HEAD_INITIALIZER(head)
.. c:macro:: DMR_SIMPLEQ_ENTRY(type)
.. c:macro:: DMR_SIMPLEQ_FIRST(head)
.. c:macro:: DMR_SIMPLEQ_END(head)
.. c:macro:: DMR_SIMPLEQ_EMPTY(head)
.. c:macro:: DMR_SIMPLEQ_NEXT(elm, field)
.. c:macro:: DMR_SIMPLEQ_FOREACH(var, head, field)
.. c:macro:: DMR_SIMPLEQ_FOREACH_SAFE(var, head, field, tvar)
.. c:macro:: DMR_SIMPLEQ_INIT(head)
.. c:macro:: DMR_SIMPLEQ_INSERT_HEAD(head, elm, field)
.. c:macro:: DMR_SIMPLEQ_INSERT_TAIL(head, elm, field)
.. c:macro:: DMR_SIMPLEQ_INSERT_AFTER(head, listelm, elm, field)
.. c:macro:: DMR_SIMPLEQ_REMOVE_HEAD(head, field)
.. c:macro:: DMR_SIMPLEQ_REMOVE_AFTER(head, elm, field)
.. c:macro:: DMR_SIMPLEQ_CONCAT(head1, head2)


XOR simple queue
^^^^^^^^^^^^^^^^

An XOR simple queue is used in the same way as a regular simple queue.  The
difference is that the head structure also includes a "cookie" that is XOR'd
with the queue pointer (first, last or next) to generate the real pointer
value.

.. c:macro:: DMR_XSIMPLEQ_HEAD(name, type)
.. c:macro:: DMR_XSIMPLEQ_ENTRY(type)
.. c:macro:: DMR_XSIMPLEQ_XOR(head, ptr)
.. c:macro:: DMR_XSIMPLEQ_FIRST(head)
.. c:macro:: DMR_XSIMPLEQ_END(head)
.. c:macro:: DMR_XSIMPLEQ_EMPTY(head)
.. c:macro:: DMR_XSIMPLEQ_NEXT(head, elm, field)
.. c:macro:: DMR_XSIMPLEQ_FOREACH(var, head, field)
.. c:macro:: DMR_XSIMPLEQ_FOREACH_SAFE(var, head, field, tvar)
.. c:macro:: DMR_XSIMPLEQ_INIT(head)
.. c:macro:: DMR_XSIMPLEQ_INSERT_HEAD(head, elm, field)
.. c:macro:: DMR_XSIMPLEQ_INSERT_TAIL(head, elm, field)
.. c:macro:: DMR_XSIMPLEQ_INSERT_AFTER(head, listelm, elm, field)
.. c:macro:: DMR_XSIMPLEQ_REMOVE_HEAD(head, field)
.. c:macro:: DMR_XSIMPLEQ_REMOVE_AFTER(head, elm, field)


Tail queue
^^^^^^^^^^

A tail queue is headed by a pair of pointers, one to the head of the list and
the other to the tail of the list. The elements are doubly linked so that an
arbitrary element can be removed without a need to traverse the list. New
elements can be added to the list before or after an existing element, at the
head of the list, or at the end of the list. A tail queue may be traversed in
either direction.
 
.. c:macro:: DMR_TAILQ_HEAD(name, type)
.. c:macro:: DMR_TAILQ_HEAD_INITIALIZER(head)
.. c:macro:: DMR_TAILQ_ENTRY(type)
.. c:macro:: DMR_TAILQ_FIRST(head)
.. c:macro:: DMR_TAILQ_END(head)
.. c:macro:: DMR_TAILQ_NEXT(elm, field)
.. c:macro:: DMR_TAILQ_LAST(head, headname)
.. c:macro:: DMR_TAILQ_PREV(elm, headname, field)
.. c:macro:: DMR_TAILQ_EMPTY(head)
.. c:macro:: DMR_TAILQ_FOREACH(var, head, field)
.. c:macro:: DMR_TAILQ_FOREACH_SAFE(var, head, field, tvar)
.. c:macro:: DMR_TAILQ_FOREACH_REVERSE(var, head, headname, field)
.. c:macro:: DMR_TAILQ_FOREACH_REVERSE_SAFE(var, head, headname, field, tvar)
.. c:macro:: DMR_TAILQ_INIT(head)
.. c:macro:: DMR_TAILQ_INSERT_HEAD(head, elm, field)
.. c:macro:: DMR_TAILQ_INSERT_TAIL(head, elm, field)
.. c:macro:: DMR_TAILQ_INSERT_AFTER(head, listelm, elm, field)
.. c:macro:: DMR_TAILQ_INSERT_BEFORE(listelm, elm, field)
.. c:macro:: DMR_TAILQ_REMOVE(head, elm, field)
.. c:macro:: DMR_TAILQ_REPLACE(head, elm, elm2, field)
.. c:macro:: DMR_TAILQ_CONCAT(head1, head2, field)
