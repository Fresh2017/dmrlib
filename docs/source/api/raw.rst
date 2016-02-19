.. _raw:

raw: raw byte buffers
=====================


Data types
----------

.. c:type:: dmr_raw

   Raw byte buffer with preallocated size.

.. c:member:: uint8_t                 *dmr_raw.buf

   The actual buffer.

.. c:member:: uint64_t                 dmr_raw.len

   Number of bytes stored in the buffer.

.. c:member:: int64_t                  dmr_raw.allocated

   Number of bytes allocated in the buffer.

.. c:member:: DMR_TAILQ_ENTRY(dmr_raw) dmr_raw.entries

   If this raw buffer is part of a c:type:`dmr_rawq`, this stores pointers to 
   the sibling entries.

.. c:type:: dmr_rawq

   Zero or more :c:type:`dmr_raw` buffers in a tail queue.

.. c:member:: DMR_TAILQ_HEAD dmr_rawq.head

   Head of the tail queue.

.. c:member:: size_t dmr_rawq.limit

   Maximum number of buffers in the tail queue.


.. c:type:: void (*dmr_raw_cb)(dmr_raw *, void *)


API
---

.. c:function:: dmr_raw *dmr_raw_new(uint64_t len)

   Setup a new raw buffer.

.. c:function:: void dmr_raw_free(dmr_raw *raw)

   Destroy a raw buffer.

.. c:function:: int dmr_raw_add(dmr_raw *raw, const void *buf, size_t len)

   Add to the raw buffer.

.. c:function:: int dmr_raw_add_hex(dmr_raw *raw, const void *buf, size_t len)

   Add to the raw buffer and hex encode.

.. c:function:: int dmr_raw_add_uint8(dmr_raw *raw, uint8_t in)

   Add an uint8_t to the buffer.

.. c:function:: int dmr_raw_add_uint16(dmr_raw *raw, uint16_t in)

   Add an uint16_t to the buffer.

.. c:function:: int dmr_raw_add_uint24(dmr_raw *raw, uint24_t in)

   Add an uint24_t to the buffer.

.. c:function:: int dmr_raw_add_uint32(dmr_raw *raw, uint32_t in)

   Add an uint32_t to the buffer.

.. c:function:: int dmr_raw_add_uint32_le(dmr_raw *raw, uint32_t in)

   Add an uint32_t to the buffer (little endian).

.. c:function:: int dmr_raw_add_uint64(dmr_raw *raw, uint64_t in)

   Add an uint64_t to the buffer.

.. c:function:: int dmr_raw_add_xuint8(dmr_raw *raw, uint8_t in)

   Add a hex encoded uint8_t to the buffer.

.. c:function:: int dmr_raw_add_xuint16(dmr_raw *raw, uint16_t in)

   Add a hex encoded uint16_t to the buffer.

.. c:function:: int dmr_raw_add_xuint24(dmr_raw *raw, uint24_t in)

   Add a hex encoded uint24_t to the buffer.

.. c:function:: int dmr_raw_add_xuint32(dmr_raw *raw, uint32_t in)

   Add a hex encoded uint32_t to the buffer.

.. c:function:: int dmr_raw_add_xuint32_le(dmr_raw *raw, uint32_t in)

   Add a hex encoded uint32_t to the buffer (little endian).

.. c:function:: int dmr_raw_add_xuint64(dmr_raw *raw, uint64_t in)

   Add a hex encoded uint64_t to the buffer.

.. c:function:: int dmr_raw_addf(dmr_raw *raw, size_t len, const char *fmt, ...)

   Add a formatted string to the buffer.

.. c:function:: int dmr_raw_grow(dmr_raw *raw, uint64_t len)

   Resize a raw buffer.

.. c:function:: int dmr_raw_grow_add(dmr_raw *raw, uint64_t add)

   Resize a raw buffer if `add` number of bytes can't be added.

.. c:function:: int dmr_raw_reset(dmr_raw *raw)

   Reset a raw buffer.

.. c:function:: int dmr_raw_zero(dmr_raw *raw)

   Zero a raw buffer, maitaining the size.

.. c:function:: dmr_rawq *dmr_rawq_new(size_t limit)

   Setup a new raw queue.
   
   A limit of 0 means unlimited queue size.

.. c:function:: void dmr_rawq_free(dmr_rawq *rawq)

   Destroy a raw queue.

.. c:function:: int dmr_rawq_add(dmr_rawq *rawq, dmr_raw *raw)

   Add a buffered element to the raw queue.

.. c:function:: int dmr_rawq_addb(dmr_rawq *rawq, uint8_t *buf, size_t len)

   Add a buffer to the raw queue.

.. c:function:: int dmr_rawq_each(dmr_rawq *rawq, dmr_raw_cb cb, void *userdata)

   Iterate over all the items in a raw queue.

.. c:function:: bool dmr_rawq_empty(dmr_rawq *rawq)

   Check if the rawq is empty.

.. c:function:: size_t dmr_rawq_size(dmr_rawq *rawq)

   Size of the rawq.

.. c:function:: dmr_raw *dmr_rawq_shift(dmr_rawq *rawq)

   Shift the first element from the raw queue.

.. c:function:: int dmr_rawq_unshift(dmr_rawq *rawq, dmr_raw *raw)

   Prepend a raw buffer to the raw queue.
