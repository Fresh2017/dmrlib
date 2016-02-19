.. _malloc:

malloc: memory allocation
=========================


API
---

.. c:function:: dmr_palloc(void *, type)

   Allocate a 0-initizialized structure with parent context.

.. c:function:: dmr_palloc_size(void *, size)

   Allocate a 0-initizialized untyped buffer with parent context.

.. c:function:: dmr_malloc(type)

   Allocate a 0-initizialized structure.

.. c:function:: dmr_malloc_size(size)

   Allocate a 0-initizialized untyped buffer.

.. c:function:: dmr_realloc(void *, void*, type, size)

   Resize an untyped buffer with parent context.

.. c:function:: dmr_strdup(void *, char *)

   Duplicate a string with parent context.

.. c:function:: dmr_free(void *)

   Free a previously allocated structure.


.. c:macro:: DMR_NULL_CHECK(expr)

   Checks an expression for ``NULL`` returns, sets ``ENOMEM``.

.. c:macro:: DMR_NULL_CHECK_FREE(expr, var)

   Checks an expression for ``NULL`` returns, sets ``ENOMEM`` and frees
   ``var``.
